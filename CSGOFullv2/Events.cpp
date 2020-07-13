#include "precompiled.h"
#include "GameEventListener.h"
#include "Events.h"
#include "Targetting.h"
#include "EncryptString.h"
#include "Overlay.h"
#include "CreateMove.h"
#include "HitChance.h"
#include "LocalPlayer.h"
#include "Globals.h"
#include <sstream>
#include "Draw.h"
#include "INetchannelInfo.h"
#include "AutoWall.h"
#include "cx_strenc.h"
#include "Eventlog.h"
#include "UsedConvars.h"
#include "StatsTracker.h"
#include <map>
#include "WaypointSystem.h"

#include "Adriel/asuswall.hpp"
#include "Adriel/adr_util.hpp"
#include "Adriel/stdafx.hpp"

#include "Adriel/clantag_changer.hpp"

#define ANTI_AA_PITCH 70.0f
#define ANTI_AA_YAW 45.0f

std::vector<GameEvent_PlayerHurt_t> g_GameEvent_PlayerHurt_Queue;
std::vector<GameEvent_Impact_t> g_GameEvent_Impact_Queue;
std::vector<GameEvent_PlayerDeath_t> g_GameEvent_PlayerDeath_Queue;

//char *spritespurplelaser1vmtstr_ = new char[25]{ 9, 10, 8, 19, 14, 31, 9, 85, 10, 15, 8, 10, 22, 31, 22, 27, 9, 31, 8, 75, 84, 12, 23, 14, 0 }; /*sprites/purplelaser1.vmt*/

bool g_bBombExploded = false; 

void IncrementMissedShots(CPlayerrecord *_playerRecord, CShotrecord *_shotRecord, ShotResult &_ShotResult)
{
	// was this player shot using far esp/server side
	if (_shotRecord->m_bDoesNotFoundTowardsStats)
		return;

	// did we already count a miss?
	if (_ShotResult.m_bCountedMiss)
		return;

	// increase missed shots
	++_playerRecord->m_iShotsMissed;

	if (_ShotResult.m_bShotAtMovingResolver)
	{
		++_playerRecord->m_iShotsMissed_MovingResolver;
	}
	else if (_ShotResult.m_bShotAtBalanceAdjust)
	{
		// save previous balance adjust results
		_playerRecord->m_BackupBalanceAdjustResults = _playerRecord->m_BalanceAdjustResults;

		++_playerRecord->m_iShotsMissed_BalanceAdjust;
		_playerRecord->m_BalanceAdjustResults |= BALANCE_ADJUST_MISSED;
	}

	// save previous resolve mode
	_playerRecord->m_iBackupOldResolveMode = _playerRecord->m_iOldResolveMode;

	// save old resolve mode
	_playerRecord->m_iOldResolveMode = _playerRecord->m_iResolveMode;

	// save previous resolve side
	_playerRecord->m_iBackupOldResolveSide = _playerRecord->m_iOldResolveSide;

	// save old resolve side
	_playerRecord->m_iOldResolveSide = _playerRecord->m_iResolveSide;

	// save old desync results
	_playerRecord->m_BackupDesyncresults = _playerRecord->m_DesyncResults;

	// save previous resolve side
	_playerRecord->m_iBackupOldResolveSide = _playerRecord->m_iOldResolveSide;

	// save old force not legit
	_playerRecord->m_bOldForceNotLegit = _playerRecord->m_bForceNotLegit;

	CTickrecord* _tickRecordExists = _playerRecord->FindTickRecord(_shotRecord->m_Tickrecord);

	// save old hitscan settings
	if (_tickRecordExists)
		_shotRecord->m_Tickrecord->m_bShotAndMissed = true;

	// force not legit for now until better solution is made
	//if (_playerRecord->m_bLegit && _playerRecord->m_iShotsMissed > 2)
	//	_playerRecord->m_bForceNotLegit = true;


	bool _appliedSafePoint = false;
	int _newResolveSide = _playerRecord->m_iResolveSide;

	if (_playerRecord->m_iResolveMode != RESOLVE_MODE_MANUAL)
	{
#ifdef NO_35
		if (_playerRecord->m_iResolveSide == ResolveSides::NEGATIVE_60)
			_newResolveSide = ResolveSides::POSITIVE_60;
		else
			_newResolveSide = ResolveSides::NEGATIVE_60;
#else
		++_newResolveSide;
#endif

		// if the player is using the moving in place lby meme, then switch to the opposite resolve side
		// if we don't, then as soon as this increments to resolve side the standing in place resolver will switch it right back to the side we just missed
		// TODO FIXME: add override for moving lby meme resolver so that we can disable it when needed
#if 0
		if (_tickRecordExists->m_bIsUsingMovingLBYMeme)
		{
			int _OppositeSide = g_LagCompensation.GetOppositeResolveSide(_shotRecord->m_iResolveSide);
			if (_OppositeSide != _shotRecord->m_iResolveSide)
				_newResolveSide = _OppositeSide;
		}
#endif

		if (_newResolveSide >= ResolveSides::MAX_RESOLVE_SIDES)
			_newResolveSide = 0;

		// if using safe point, we can assume that they are using a significantly different delta
#if 0
		if (variable::get().ragebot.b_safe_point)
		{
			if (_shotRecord->m_iTargetHitbox != HITBOX_HEAD || variable::get().ragebot.b_safe_point_head)
			{
				switch (_shotRecord->m_iResolveSide)
				{
				case ResolveSides::POSITIVE_35:
				case ResolveSides::NEGATIVE_35:
					_newResolveSide = ResolveSides::POSITIVE_60;
					_appliedSafePoint = true;
					break;
				case ResolveSides::POSITIVE_60:
				case ResolveSides::NEGATIVE_60:
					_newResolveSide = ResolveSides::POSITIVE_35;
					_appliedSafePoint = true;
					break;
				default:
					//TODO: FIX ME: deal with ResolveSide::NONE
					break;
				};
			}
		}
#endif
	}

	if (_shotRecord->m_bUsedBodyHitResolveDelta)
	{
		if (!_ShotResult.m_bCountedBodyHitMiss)
		{
			_ShotResult.m_bCountedBodyHitMiss = true;

			auto &stance = _playerRecord->Impact.ResolveStances[_shotRecord->m_iBodyHitResolveStance];
			if (++stance.m_iShotsMissed >= 2 || fabsf(stance.m_flDesyncDelta) < 5.0f)
			{
				stance.m_bIsBodyHitResolved = false;
			}
#ifndef NO_35
			else if (!_appliedSafePoint && _playerRecord->m_iResolveMode != RESOLVE_MODE_MANUAL)
			{
				//switch the resolve side to use to the opposite of the one we shot at if it is body hit resolved

				_newResolveSide = g_LagCompensation.GetOppositeResolveSide(_shotRecord->m_iResolveSide);
				if (_newResolveSide == _shotRecord->m_iResolveSide)
					_newResolveSide = ResolveSides::NEGATIVE_35;
			}
#endif
		}
	}

	// now, rescan the new side and see if we will still miss it
#ifndef NO_35
	if (_tickRecordExists && _playerRecord->m_iResolveMode != RESOLVE_MODE_MANUAL)
	{
		CBaseEntity* m_pEntity = _playerRecord->m_pEntity;
		PlayerBackup_t *backup = new PlayerBackup_t(m_pEntity);
		if (backup)
		{
			_tickRecordExists->m_PlayerBackup.RestoreData();

			m_pEntity->WritePoseParameters(_tickRecordExists->m_flPredictedPoseParameters[_newResolveSide]);
			m_pEntity->SetAbsAnglesDirect(QAngle(0.0f, _tickRecordExists->m_flPredictedGoalFeetYaw[_newResolveSide], 0.0f));

			AllowSetupBonesToUpdateAttachments = false;
			m_pEntity->InvalidateBoneCache();
			m_pEntity->SetLastOcclusionCheckFlags(0);
			m_pEntity->SetLastOcclusionCheckFrameCount(Interfaces::Globals->framecount);
			m_pEntity->ReevaluateAnimLOD();
			m_pEntity->SetupBonesRebuilt(nullptr, -1, BONE_USED_BY_HITBOX, Interfaces::Globals->curtime);

			//m_pEntity->DrawHitboxesFromCache(ColorRGBA(255, 0, 0, 255), TICKS_TO_TIME(2), m_pEntity->GetBoneAccessor()->GetBoneArrayForWrite());

			CPlayerrecord::s_Shot &LastShot = LocalPlayer.ShotRecord->m_bIsInflictor ? _playerRecord->LastShotAsEnemy : _playerRecord->LastShot;
			Vector vecFurthestImpact = LastShot.m_vecMainImpactPos;
			float _furthestDistance = -1.0f;

			for (const auto& pos : LastShot.m_vecImpactPositions)
			{
				float _dist = (pos - _shotRecord->m_vecLocalEyePos).Length();
				if (_dist > _furthestDistance)
				{
					vecFurthestImpact = pos;
					_furthestDistance = _dist;
				}
			}

			trace_t tr;
			Ray_t ray;
			Vector vecDir = vecFurthestImpact - LocalPlayer.ShootPosition;
			VectorNormalizeFast(vecDir);
			ray.Init(_shotRecord->m_vecLocalEyePos, vecFurthestImpact + vecDir * 40.0f);
			Interfaces::EngineTrace->ClipRayToEntity(ray, MASK_SHOT, (IHandleEntity*)m_pEntity, &tr);

			if (tr.m_pEnt == m_pEntity)
			{
				//The new resolve mode is still hittable, find one that isn't
				for (int i = 0; i < ResolveSides::MAX_RESOLVE_SIDES; ++i)
				{
					if (i == _newResolveSide || i == ResolveSides::NONE)
						continue;

					m_pEntity->WritePoseParameters(_tickRecordExists->m_flPredictedPoseParameters[i]);
					m_pEntity->SetAbsAnglesDirect(QAngle(0.0f, _tickRecordExists->m_flPredictedGoalFeetYaw[i], 0.0f));

					AllowSetupBonesToUpdateAttachments = false;
					m_pEntity->InvalidateBoneCache();
					m_pEntity->SetLastOcclusionCheckFlags(0);
					m_pEntity->SetLastOcclusionCheckFrameCount(Interfaces::Globals->framecount);
					m_pEntity->ReevaluateAnimLOD();
					m_pEntity->SetupBonesRebuilt(nullptr, -1, BONE_USED_BY_HITBOX, Interfaces::Globals->curtime);

					Interfaces::EngineTrace->ClipRayToEntity(ray, MASK_SHOT, (IHandleEntity*)m_pEntity, &tr);

					if (!tr.m_pEnt)
					{
						//Yay we found a resolve side that we can't hit anymore from the shot
						_newResolveSide = (ResolveSides)i;
						//try not to shoot at the last one we missed
						if (_newResolveSide == _playerRecord->m_iBackupOldResolveSide)
							continue;
						else
							break;
					}
				}
			}

			backup->RestoreData();
			delete backup;
		}
	}
#endif

	_playerRecord->SetResolveSide((ResolveSides)_newResolveSide, __FUNCTION__);

	// mark as unresolved
	_playerRecord->m_bResolved = false;

	// store that we counted this as a miss in case they are found to be resolved later
	_ShotResult.m_bCountedMiss = true;
	_ShotResult.m_MissFlags &= ~HIT_IS_RESOLVED;
}

// NOTE: Hook IsBoxVisible to return true, to always draw beams :D
void DoBeams(const Vector& endPos, ColorRGBAFloat color)
{
#if 0
    BeamInfo_t beamInfo;

    EncStr(spritespurplelaser1vmtstr_, 24);

    beamInfo.m_nType = TE_BEAMPOINTS;
    beamInfo.m_pszModelName = spritespurplelaser1vmtstr_;
    beamInfo.modelindex = -1;
    beamInfo.m_flHaloScale = 0.f;
    beamInfo.m_flLife = 2.f;
    beamInfo.m_flWidth = 3.f;
    beamInfo.m_flEndWidth = 3.f;
    beamInfo.m_flAmplitude = 0.f;
    beamInfo.m_flFadeLength = 2.5f;
    beamInfo.m_flBrightness = 255.f;
    beamInfo.m_flSpeed = BeamSpeedTxt.flValue;
    beamInfo.m_nStartFrame = 0;
    beamInfo.m_flFrameRate = 0.f;
    beamInfo.m_flRed = clamp(color.r, 0.f, 255.f);
    beamInfo.m_flGreen = clamp(color.g, 0.f, 255.f);
    beamInfo.m_flBlue = clamp(color.b, 0.f, 255.f);
    beamInfo.m_nSegments = 2;
    beamInfo.m_bRenderable = true;
    beamInfo.m_nFlags = FBEAM_ONLYNOISEONCE | FBEAM_NOTILE | FBEAM_HALOBEAM;
    beamInfo.m_vecStart = LocalPlayer.Entity->Weapon_ShootPosition() + Vector(1.f, 0.f, 0.f);
    beamInfo.m_vecEnd = endPos;

    Beam_t* newBeam = Interfaces::Beams->CreateBeamPoints(beamInfo);

    if (newBeam != nullptr)
    {
        Interfaces::Beams->DrawBeam(newBeam);
    }

    EncStr(spritespurplelaser1vmtstr_, 24);
#endif
}

extern void autobuy_logic();
extern void FillImpactTempEntResultsAndResolveFromPlayerHurt(CBaseEntity* EntityHit, CPlayerrecord* pCPlayer, CShotrecord* shotrecord, bool CalledFromQueuedEventProcessor);
extern void ResolveFromPlayerHurt_ManualShot(CPlayerrecord* pCPlayer, CShotrecord* shotrecord);

void CreateEventListeners()
{
	//decrypts(0)
    CGameEventListener *ImpactListener = new CGameEventListener(XorStr("bullet_impact"), GameEvent_BulletImpact, false);
    CGameEventListener *PlayerHurtListener = new CGameEventListener(XorStr("player_hurt"), GameEvent_PlayerHurt, false);
    CGameEventListener *PlayerDeath = new CGameEventListener(XorStr("player_death"), GameEvent_PlayerDeath, false);
    CGameEventListener *ItemPurchase = new CGameEventListener(XorStr("item_purchase"), GameEvent_ItemPurchase, false);
	CGameEventListener *RoundStart = new CGameEventListener(XorStr("round_start"), GameEvent_RoundStart, false);
	CGameEventListener *SwitchTeam = new CGameEventListener(XorStr("switch_team"), GameEvent_SwitchTeam, false);
	CGameEventListener *StartHalftime = new CGameEventListener(XorStr("start_halftime"), GameEvent_StartHalftime, false);
	CGameEventListener *RoundMVP = new CGameEventListener(XorStr("round_mvp"), GameEvent_RoundMVP, false);
	CGameEventListener *RoundEnd = new CGameEventListener(XorStr("round_end"), GameEvent_RoundEnd, false);
	CGameEventListener *BombExploded = new CGameEventListener(XorStr("bomb_exploded"), GameEvent_BombExploded, false);
	CGameEventListener *PlayerFire = new CGameEventListener(XorStr("weapon_fire"), GameEvent_WeaponFire, false);
	//encrypts(0)
}

void GameEvent_RoundStart(CGameEvent* gameEvent)
{
	ResetAllWaypointTemporaryFlags();

	if (!LocalPlayer.Entity || !gameEvent)
		return;

	// force cl_predict to 1 on round_start
	cl_predict.GetVar()->SetValue(1);

	if (variable::get().misc.clantag.b_enabled.get())
	{
		clantag_changer::get().reset();
	}

	// todo: nit; make this when a player spawns. If someone joins mid-game before this event is called
	// it won't autobuy.
	autobuy_logic();

	// reset death notices
	if (variable::get().misc.b_preserve_killfeed)
	{
		//decrypts(0)
		CCSGO_DeathNotice *death_notice = (CCSGO_DeathNotice *)Interfaces::Hud->FindElement(XorStr("CCSGO_HudDeathNotice"));
		//encrypts(0)

		if (death_notice)
		{
			// lea     ecx, [eax-14h]
			// test	   ecx, ecx
			// call	   ClearDeathNotices
			uintptr_t death_notice_struct = (uintptr_t)((uintptr_t)death_notice - 0x14);

			if (death_notice_struct)
			{
				using ClearDeathNotices_t = void*(__thiscall *)(DWORD);
				ClearDeathNotices_t ClearDeathNotices = (ClearDeathNotices_t)StaticOffsets.GetOffset(_ClearDeathNotices).GetOffset();

				ClearDeathNotices(death_notice_struct);
			}
		}
	}

	asuswall::get().game_event(gameEvent);

#ifdef INCLUDE_LEGIT
	legitbot::get().game_event(gameEvent);
#endif

	g_Visuals.game_event(gameEvent);

	g_bBombExploded = false;
}

void GameEvent_PlayerDeath(CGameEvent* gameEvent)
{
	if(!LocalPlayer.Entity || !gameEvent)
		return;

	bool keep_killfeed = variable::get().misc.b_preserve_killfeed;

	//decrypts(0)
	const int user_id = gameEvent->GetInt(XorStr("userid"));
	const int attacker_id = gameEvent->GetInt(XorStr("attacker"));
	const int assister_id = gameEvent->GetInt(XorStr("assister"));
	//encrypts(0)

	const int local_player = Interfaces::EngineClient->GetLocalPlayer();
	const int killed = Interfaces::EngineClient->GetPlayerForUserID(user_id);
	const int killer = Interfaces::EngineClient->GetPlayerForUserID(attacker_id);
	const int assister = Interfaces::EngineClient->GetPlayerForUserID(assister_id);

	CBaseEntity* killed_entity = Interfaces::ClientEntList->GetBaseEntity(killed);
	CBaseEntity* killer_entity = Interfaces::ClientEntList->GetBaseEntity(killer);
	CPlayerrecord *_playerRecord = g_LagCompensation.GetPlayerrecord(killed_entity);
	CPlayerrecord *_killerPlayerRecord = g_LagCompensation.GetPlayerrecord(killer_entity);

	GameEvent_PlayerDeath_t ev;
	ev.user_id = user_id;
	ev.attacker_id = attacker_id;
	ev.assister_id = assister_id;
	g_GameEvent_PlayerDeath_Queue.push_back(ev);

	//decrypts(0)
	CCSGO_DeathNotice *death_notice = (CCSGO_DeathNotice *)Interfaces::Hud->FindElement(XorStr("CCSGO_HudDeathNotice"));
	//encrypts(0)

	// make sure we have valid hud elements/structs
	if (death_notice)
	{	
		// if we killed or assisted in killing them
		if (keep_killfeed)
		{
			if (killed != local_player)
			{
				if (killer == local_player || (variable::get().misc.b_include_assists_killfeed && assister == local_player))
				{
					death_notice->local_player_modifier = variable::get().misc.f_preserve_killfeed / 5.f;
				}
			}
			else
			{
				death_notice->local_player_modifier = 1.5f;
			}
		}
		else
		{
			death_notice->local_player_modifier = 1.5f;
		}
	}
	
	if (!LocalPlayer.Entity)
		return;

#ifdef INCLUDE_LEGIT
	legitbot::get().game_event(gameEvent);
#endif
}

void GameEvent_ItemPurchase(CGameEvent* gameEvent)
{
	// ignore when we can
	if (!LocalPlayer.Entity || !gameEvent)
		return;

	// we don't want the feature
	if (variable::get().visuals.i_buy_logs <= 0)
		return;

	// get buyer

	//decrypts(0)
	CBaseEntity* _buyer = Interfaces::ClientEntList->GetBaseEntity(Interfaces::EngineClient->GetPlayerForUserID(gameEvent->GetInt(XorStr("userid"))));
	//encrypts(0)

	// skip teammates
	if (!_buyer || _buyer->GetTeam() == LocalPlayer.Entity->GetTeam())
		return;

	//decrypts(0)
	std::string weapon = gameEvent->GetString(XorStr("weapon"));
	//encrypts(0)
	std::string name = adr_util::sanitize_name((char*)_buyer->GetName().data());

	// construct event
	//decrypts(0)
	g_Eventlog.AddEventElement(0x01, XorStr("\'"));
	g_Eventlog.AddEventElement(0x01, name);
	g_Eventlog.AddEventElement(0x01, XorStr("\'"));
	g_Eventlog.AddEventElement(0x01, XorStr(" bought a "));
	//encrypts(0)
	// todo: nit; add "highlight essential weapons" to color the weapon if it matches a list of items (aka awp, auto, flashbang etc)
	g_Eventlog.AddEventElement(0x0D, weapon);

	// output event
	g_Eventlog.OutputEvent(variable::get().visuals.i_buy_logs);
}

//Checks to see if hitgrouphit can be hit through the specified hitgroup in the shotrecord
unsigned GetShotMissReason(CBaseEntity* _Entity, CShotrecord *_shotRecord, const Vector& impactpos, const CPlayerrecord::s_Shot &LastShot, trace_t* output = nullptr, int _server_hitgrouphit = HITGROUP_GENERIC)
{
	// get playerrecord
	Vector mainimpactpos = impactpos;
	CPlayerrecord* _playerRecord = g_LagCompensation.GetPlayerrecord(_Entity);
	CTickrecord* _tickRecord = _playerRecord->FindTickRecord(_shotRecord->m_Tickrecord);
	PlayerBackup_t *_playerBackup = nullptr;
#ifdef _DEBUG
	matrix3x4_t *test = new matrix3x4_t[MAXSTUDIOBONES];
#endif
	if (_tickRecord && Interfaces::ClientEntList->EntityExists(_Entity) && _Entity->GetRefEHandle().ToUnsignedLong() == _tickRecord->m_PlayerBackup.RefEHandle)
	{
		_playerBackup = new PlayerBackup_t(_Entity);
		_tickRecord->m_PlayerBackup.RestoreData();
		auto _boneCache = _Entity->GetCachedBoneData();
		memcpy(_boneCache->Base(), _tickRecord->m_PlayerBackup.CachedBoneMatrices, sizeof(matrix3x4_t) * max(0, min(_boneCache->Count(), _tickRecord->m_PlayerBackup.CachedBonesCount)));
		//prevent further bone setups unless mask is bad
		_Entity->SetLastBoneSetupTime(Interfaces::Prediction->InPrediction() ? Interfaces::Prediction->m_SavedVars.curtime : Interfaces::Globals->curtime);
		_Entity->GetBoneAccessor()->SetReadableBones(_tickRecord->m_PlayerBackup.ReadableBones);
		_Entity->GetBoneAccessor()->SetWritableBones(_tickRecord->m_PlayerBackup.WritableBones);
		//for some reason we stop shooting, the bones are not being setup/restored properly. set them up again
		AllowSetupBonesToUpdateAttachments = false;
		_Entity->InvalidateBoneCache();
		_Entity->SetLastOcclusionCheckFlags(0);
		_Entity->SetLastOcclusionCheckFrameCount(Interfaces::Globals->framecount);
		_Entity->ReevaluateAnimLOD();
		_Entity->SetupBonesRebuilt(nullptr, -1, BONE_USED_BY_HITBOX, Interfaces::Globals->curtime);
#ifdef _DEBUG
		memcpy(test, _boneCache->Base(), sizeof(matrix3x4_t) * _boneCache->Count());
#endif
	}

	if (!_playerBackup)
	{
#ifdef _DEBUG
		delete[] test;
#endif
		return MISS_SHOULDNT_HAVE_MISSED;
	}

	// init locals
	trace_t tr;
	Ray_t ray;

	if (_server_hitgrouphit == HITGROUP_GENERIC)
	{
		//use the furthest impact from the local player when we didn't hit the player, otherwise use the impact where we hit them
		//this rules out if our autowall is fucked up and allows us to only trace as far as the server impact instead of 8192.0f
		const Vector *_vecFurthestImpact = &mainimpactpos;
		float _furthestDistance = -1.0f;

		for (const auto& pos : LastShot.m_vecImpactPositions)
		{
			float _dist = (pos - _shotRecord->m_vecLocalEyePos).Length();
			if (_dist > _furthestDistance)
			{
				_vecFurthestImpact = &pos;
				_furthestDistance = _dist;
			}
		}
		mainimpactpos = *_vecFurthestImpact;
	}

	Vector vecDirFromLocalPlayerToImpact = mainimpactpos - _shotRecord->m_vecLocalEyePos;
	VectorNormalizeFast(vecDirFromLocalPlayerToImpact);
	const float flLeniency = _server_hitgrouphit == HITGROUP_GENERIC ? 2.0f : 40.0f; 
	//add leniency here as desired. by default there is big leniency when we hit the player so that 
	//when we hit the intended hitgroup but they arent technically resolved due to intersecting angles we don't show missed due to bad resolve even though we hit the right hitgroup
	//this is technically wrong, ideally we don't want leniency here at all but there may be a possibility that if we counted it as a miss and then switched resolve modes to something that doesn't hit at all, we miss
	Vector vecRayEndPos = mainimpactpos + (vecDirFromLocalPlayerToImpact * flLeniency);

	ray.Init(_shotRecord->m_vecLocalEyePos, vecRayEndPos);

#if defined _DEBUG || defined INTERNAL_DEBUG
	if (_server_hitgrouphit == HITGROUP_GENERIC)
	{
		//Interfaces::DebugOverlay->AddLineOverlayAlpha(_shotRecord->m_vecLocalEyePos, vecRayEndPos, 255, 0, 255, 25, false, 10.f);
		//Interfaces::DebugOverlay->AddBoxOverlay(vecRayEndPos, { -2.f, -2.f, -2.f }, { 2.f, 2.f, 2.f }, angZero, 255, 0, 255, 25, 10.f);
	}
	else
	{
		//Interfaces::DebugOverlay->AddLineOverlayAlpha(_shotRecord->m_vecLocalEyePos, vecRayEndPos, 255, 255, 0, 25, false, 10.f);
		//Interfaces::DebugOverlay->AddBoxOverlay(vecRayEndPos, { -2.f, -2.f, -2.f }, { 2.f, 2.f, 2.f }, angZero, 255, 255, 0, 25, 10.f);
	}
#endif

	// draw bullettracers
#if 0
	if (g_Convars.Visuals.misc_bullettracer->GetBool())
		//if(true)
	{
		// set beam color
		if (_Entity->GetTeam() == 2)
			Interfaces::DebugOverlay->AddLineOverlay(_shotRecord->m_vecLocalEyePos, _shotRecord->m_vecLocalEyePos + (vecDirFromLocalPlayerToImpact * (_shotRecord->m_vecLocalEyePos.Dist(mainimpactpos))), 255, 0, 0, true, 10.f);
		else
			Interfaces::DebugOverlay->AddLineOverlay(_shotRecord->m_vecLocalEyePos, _shotRecord->m_vecLocalEyePos + (vecDirFromLocalPlayerToImpact * (_shotRecord->m_vecLocalEyePos.Dist(mainimpactpos))), 0, 192, 255, true, 10.f);
	}
#endif


	Interfaces::EngineTrace->ClipRayToEntity(ray, MASK_SHOT, (IHandleEntity*)_Entity, &tr);
	_playerBackup->RestoreData();
	delete _playerBackup;

#if defined _DEBUG || defined INTERNAL_DEBUG
	printf("[getmissreason] server hitgroup: %s | shotrec hitgroup: %s | trace hitgroup: %s\n", get_hitgroup_name(_server_hitgrouphit).data(), get_hitgroup_name(_shotRecord->m_iTargetHitgroup).data(), get_hitgroup_name(tr.hitgroup).data());
#endif

	// set trace output
	if (output)
		*output = tr;

	unsigned _Flags = 0;

	// check to see if the server hitgroup matches the local hitgroup
	if (_server_hitgrouphit != HITGROUP_GENERIC)
	{
		//override the trace ray in case there is a floating point inaccuracy
		if (_server_hitgrouphit == _shotRecord->m_iTargetHitgroup)
			return MISS_SHOULDNT_HAVE_MISSED;

		//allow lenience with head/neck differences
		if (_server_hitgrouphit == HITGROUP_HEAD && tr.hitgroup == HITGROUP_NECK || _server_hitgrouphit == HITGROUP_NECK && tr.hitgroup == HITGROUP_HEAD)
			return MISS_SHOULDNT_HAVE_MISSED;

		//check to see if the local hitgroup matches the server hitgroup from the server's ray
		if (_server_hitgrouphit == tr.hitgroup)
			return MISS_SHOULDNT_HAVE_MISSED;

		if (tr.m_pEnt)
			_Flags |= MISS_FROM_BAD_HITGROUP;

		_Flags |= MISS_FROM_BADRESOLVE;

#ifdef _DEBUG
		delete[] test;
#endif

		return _Flags;
	}

	// ray was completely off the target
	if (!tr.m_pEnt)
	{
		_Flags |= MISS_FROM_SPREAD;
		_Flags |= MISS_RAY_OFF_TARGET;

#ifdef _DEBUG
		//Interfaces::DebugOverlay->ClearAllOverlays();
		//Interfaces::DebugOverlay->AddLineOverlay(_shotRecord->m_vecLocalEyePos, mainimpactpos, 255, 0, 0, true, 10.f);
		//_Entity->DrawHitboxesFromCache(ColorRGBA(255, 0, 0, 255), 10.00f, test );
		//Interfaces::EngineClient->ClientCmd_Unrestricted("lastinv", 0);
#endif

#ifdef _DEBUG
		delete[] test;
#endif

		return _Flags;
	}

	// check to see if the ray we got from the server lines up with the local hitgroups
	//if (_shotRecord->m_iTargetHitgroup != tr.hitgroup)
	//{
	//	_Flags |= MISS_FROM_SPREAD;
	//	_Flags |= MISS_FROM_BAD_HITGROUP;

	//	return _Flags;
	//}

	_Flags |= MISS_SHOULDNT_HAVE_MISSED;

#ifdef _DEBUG
	delete[] test;
#endif

	return _Flags;
}

void OnPlayerHurt(CPlayerrecord* _playerRecord, ShotResult &_ShotResult, int hitgroup, int damage)
{
	if (_ShotResult.m_bAwaitingImpact)
	{
#if defined _DEBUG || defined INTERNAL_DEBUG
		printf("Warning: Can't process OnPlayerHurt because impact event was never acked!\n");
#endif
		return;
	}

	CShotrecord *_shotRecord = LocalPlayer.ShotRecord;
	CPlayerrecord::s_Shot &LastShot = _ShotResult.m_bIsInflictor ? _ShotResult.m_pInflictedEntity->ToPlayerRecord()->LastShotAsEnemy : _playerRecord->LastShot;

	// didn't receive a player_hurt event yet
	//if (LastShot.m_iTickHitPlayer != Interfaces::Globals->tickcount)
	{
		// set tick
		LastShot.m_iTickHitPlayer = g_ClientState->m_ClockDriftMgr.m_nServerTick;

		// draw hitboxes
		//if (g_Convars.Visuals.visuals_draw_hit->GetBool())
#if defined _DEBUG || defined INTERNAL_DEBUG
		//_playerRecord->m_pEntity->DrawHitboxesFromCache(ColorRGBA(0, 255, 0.0f, 75.0f), 2.5f, _shotRecord->m_BoneCache);
#endif

		trace_t _traceResult;

		_ShotResult.m_iHitgroupHit = hitgroup;
		_ShotResult.m_iDamageGiven = damage;

		// we only increase shots missed if it wasn't spread that caused miss

		_ShotResult.m_MissFlags = _ShotResult.m_bInflictorIsLocalPlayer ?
			GetShotMissReason(_playerRecord->m_pEntity, _shotRecord, LastShot.m_vecMainImpactPos, LastShot, &_traceResult, hitgroup)
			: MISS_SHOULDNT_HAVE_MISSED;
		_ShotResult.m_MissFlags |= HIT_PLAYER;
		_ShotResult.m_iHitgroupShotAt = _traceResult.hitgroup;

		bool _resetShotsMissed = false, _resetBalanceMissed = false;

		// restore missed status for this tick record
		if (_playerRecord->FindTickRecord(_shotRecord->m_Tickrecord))
			_shotRecord->m_Tickrecord->m_bShotAndMissed = false;

		// if they were resolved correctly
		if (!(_ShotResult.m_MissFlags & MISS_FROM_BADRESOLVE))
		{
			if (!_ShotResult.m_bDoesNotFoundTowardsStats)
			{
				_ShotResult.m_MissFlags &= ~MISS_SHOULDNT_HAVE_MISSED;
				_ShotResult.m_MissFlags |= HIT_IS_RESOLVED;

				if (_ShotResult.m_bCountedMiss)
				{
					// reset missed shots since they are now resolved
					--_playerRecord->m_iShotsMissed;
					_resetShotsMissed = true;

					if (_ShotResult.m_bShotAtMovingResolver)
					{
						--_playerRecord->m_iShotsMissed_MovingResolver;
					}
					else if (_ShotResult.m_bShotAtBalanceAdjust)
					{
						--_playerRecord->m_iShotsMissed_BalanceAdjust;
						_playerRecord->m_BalanceAdjustResults = _playerRecord->m_BackupBalanceAdjustResults;
						_resetBalanceMissed = true;
					}

					// restore original resolve mode
					_playerRecord->m_iResolveMode = _playerRecord->m_iOldResolveMode;

					// restore old resolve mode
					_playerRecord->m_iOldResolveMode = _playerRecord->m_iBackupOldResolveMode;

					// save original resolve side
					_playerRecord->SetResolveSide(_playerRecord->m_iOldResolveSide, __FUNCTION__);

					// save original old resolve side
					_playerRecord->m_iOldResolveSide = _playerRecord->m_iBackupOldResolveSide;

					// restore old desync flags
					_playerRecord->m_DesyncResults = _playerRecord->m_BackupDesyncresults;

					// restore original force not legit value
					_playerRecord->m_bForceNotLegit = _playerRecord->m_bOldForceNotLegit;
				}

				// reset shotrecord
				LocalPlayer.ShotRecord = nullptr;

				// mark as resolved
				_playerRecord->m_bResolved = true;

				//Re-enable body hit resolver
				if (_ShotResult.m_bInflictorIsLocalPlayer && _ShotResult.m_bUsedBodyHitResolveDelta)
				{
					auto& Stance = _playerRecord->Impact.ResolveStances[_ShotResult.m_iBodyHitResolveStance];
					_ShotResult.m_MissFlags &= ~MISS_SHOULDNT_HAVE_MISSED;
					_ShotResult.m_MissFlags |= HIT_IS_RESOLVED;
					Stance.m_bIsBodyHitResolved = true;
					if (_ShotResult.m_bCountedBodyHitMiss)
						Stance.m_iShotsMissed = max(0, Stance.m_iShotsMissed - 1);

					// mark as resolved
					_playerRecord->m_bResolved = true;
				}
			}

			//enable this always if you want to resolve even if they are already resolved
			//if (!_ShotResult.m_bInflictorIsLocalPlayer ||
			//	( (!_ShotResult.m_bIsBodyHitResolved || (Interfaces::Globals->realtime - _playerRecord->Impact.ResolveStances[_ShotResult.m_iBodyHitResolveStance].m_flLastResolveTime)	> 1.0f)
			//		|| _playerRecord->m_iShotsMissed > 1))
			FillImpactTempEntResultsAndResolveFromPlayerHurt(_ShotResult.m_pInflictedEntity, _playerRecord, _shotRecord, false);

			if (_resetShotsMissed)
				_playerRecord->m_iShotsMissed = 0;

			//if (_resetBalanceMissed)
			//	_playerRecord->m_iShotsMissed_BalanceAdjust = 0;
		}
		else
		{
			if (!_ShotResult.m_bDoesNotFoundTowardsStats)
			{
				//This will not get called when using body hit resolver or forwardtracking

				if (!_ShotResult.m_bCountedMiss)
				{
					// We didn't increment a miss for some reason so do it now
					IncrementMissedShots(_playerRecord, LocalPlayer.ShotRecord, _ShotResult);
				}

				// decrease missed shots if we incremented it
				--_playerRecord->m_iShotsMissed;
			}

			FillImpactTempEntResultsAndResolveFromPlayerHurt(_ShotResult.m_pInflictedEntity, _playerRecord, _shotRecord, false);
		}
	}
}

void GameEvent_PlayerHurt_ProcessQueuedEvent(const GameEvent_PlayerHurt_t &gameEvent)
{
	// ignore when we can
	if (!LocalPlayer.Entity)
		return;

	// get involved entities
	CBaseEntity* _inflictedEntity = Interfaces::ClientEntList->GetBaseEntity(Interfaces::EngineClient->GetPlayerForUserID(gameEvent.targetuserid));
	CBaseEntity* _inflictorEntity = Interfaces::ClientEntList->GetBaseEntity(Interfaces::EngineClient->GetPlayerForUserID(gameEvent.attackeruserid));

	// skip invalid
	if (!_inflictedEntity || !_inflictorEntity)
		return;

	// get playerrecord
	CPlayerrecord* _inflictedRecord = g_LagCompensation.GetPlayerrecord(_inflictedEntity);
	CPlayerrecord* _inflictorRecord = g_LagCompensation.GetPlayerrecord(_inflictorEntity);

	// don't do stuff if we have no info
	if (!_inflictedRecord || !_inflictorRecord)
		return;

	bool _inflictorIsLocalPlayer = _inflictorRecord->m_pEntity == LocalPlayer.Entity;

	// only show damage done to other players
	if (_inflictorIsLocalPlayer)
	{
		g_Visuals._iHurtTime = GetTickCount();

		// play hitmarker sound
		if (variable::get().visuals.b_hit_sound)
		{
			//decrypts(0)
			Interfaces::Surface->Play_Sound(XorStr("buttons\\arena_switch_press_02.wav"));
			//encrypts(0)
		}
	}

	// get hitgroup from event
	const int hitgroup = gameEvent.hitgroup;
	const int damage = gameEvent.dmg_health;
	const int healthleft = gameEvent.health;
	const std::string weapon = gameEvent.weapon;

	std::string hitgroup_name = get_hitgroup_name(hitgroup);

	LocalPlayer.ShotMutex.Lock();

	// reset current record
	LocalPlayer.ShotRecord = nullptr;

	// get shot records
	const auto& ShotRecords = _inflictorRecord->ShotRecords;

	//float latency = Interfaces::EngineClient->GetNetChannelInfo()->GetAvgLatency(FLOW_OUTGOING);
	float besttimedelta = FLT_MAX;
	CShotrecord* bestshotrecord = nullptr;

	// loop through all shots
	for (auto& _shot : ShotRecords)
	{
		// too long ago, ignore
		float timedelta = fabsf(Interfaces::Globals->realtime - _shot->m_flRealTime);
		if (timedelta > 1.0f)
			continue;

		if (_shot->m_pInflictedEntity == _inflictedEntity && _shot->m_pInflictorEntity == _inflictorEntity && (!_shot->m_bAck || (_shot->m_bMissed && _shot->m_flRealTimeAck == Interfaces::Globals->realtime)))
		{
			float _predictedReceiveTime = _shot->m_flRealTime + _shot->m_flLatency;
			float _predictedActualDelta = fabsf(Interfaces::Globals->realtime - _predictedReceiveTime);

			if (_predictedActualDelta < besttimedelta)
			{
				bestshotrecord = _shot;
				besttimedelta = _predictedActualDelta;
			}
		}
	}

	if (bestshotrecord)
	{
#if defined DEBUG_SHOTS || defined _DEBUG || defined INTERNAL_DEBUG
		if (!bestshotrecord->m_bAck || !bestshotrecord->m_bImpactEventAcked)
		{
			printf("Warning: shot record has no known impact event!\n");
		}
#endif
		bestshotrecord->m_bAck = true;
		bestshotrecord->m_bMissed = false;
		bestshotrecord->m_iActualHitgroup = hitgroup;
		bestshotrecord->m_flRealTimeAck = Interfaces::Globals->curtime;
		LocalPlayer.ShotRecord = bestshotrecord;
	}


	// found valid shotrecord
	if (LocalPlayer.ShotRecord)
	{
		//fill in TE_EffectDispatch results
		for (auto& ev = g_QueuedImpactEvents.begin(); ev != g_QueuedImpactEvents.end(); ++ev)
		{
			if (ev->m_EntityHit == _inflictedEntity && Interfaces::Globals->realtime == ev->m_flRealTime)
			{
				int approximated_hitgroup = MTargetting.HitboxToHitgroup(ev->m_iActualHitbox);
				if (approximated_hitgroup == hitgroup 
					|| (approximated_hitgroup == HITGROUP_NECK && hitgroup == HITGROUP_HEAD)
					|| (approximated_hitgroup == HITGROUP_HEAD && hitgroup == HITGROUP_NECK))
				{
					LocalPlayer.ShotRecord->m_bTEImpactEffectAcked = true;
					LocalPlayer.ShotRecord->m_iActualHitbox = ev->m_iActualHitbox;
					g_QueuedImpactEvents.erase(ev);
					break;
				}
#ifdef _DEBUG
				else
				{
					printf("WARNING: queued impact has wrong hitgroup!\n");
				}
#endif
			}
		}


		if (_inflictorIsLocalPlayer)
		{
			CTickrecord *_tickRecord = _inflictedRecord->FindTickRecord(LocalPlayer.ShotRecord->m_Tickrecord);
			std::string victim_name = adr_util::sanitize_name((char*)_inflictedRecord->m_pEntity->GetName().data());
			std::string resolve_name = g_Visuals.GetResolveType(_inflictedRecord, _tickRecord);

			int backtrack = 0;

			if (_tickRecord && LocalPlayer.ShotRecord)
			{
				backtrack = LocalPlayer.ShotRecord->m_iTickCountWhenWeShot - LocalPlayer.ShotRecord->m_iEnemySimulationTickCount;
			}

			// update health for dormant esp - TODO: see if this makes any trouble
			if (_inflictedRecord->m_pEntity && _inflictedRecord->m_pEntity->GetDormant())
			{
				*(int*)((DWORD)_inflictedRecord->m_pEntity + g_NetworkedVariables.Offsets.m_iHealth) = healthleft;
			}

			// track stats
			if (hitgroup == HITGROUP_HEAD || hitgroup == HITGROUP_NECK)
				++g_StatsTracker.m_head_hits;
			else if (hitgroup == HITGROUP_CHEST || hitgroup == HITGROUP_STOMACH)
				++g_StatsTracker.m_body_hits;
			else if (hitgroup == HITGROUP_LEFTARM || hitgroup == HITGROUP_RIGHTARM)
				++g_StatsTracker.m_arms_hits;
			else if (hitgroup == HITGROUP_LEFTLEG || hitgroup == HITGROUP_RIGHTLEG)
				++g_StatsTracker.m_lower_hits;

			// if the enemy is dead
			if (healthleft <= 0)
			{
				if (hitgroup == HITGROUP_HEAD || hitgroup == HITGROUP_NECK)
					++g_StatsTracker.m_headshots;
				else
					++g_StatsTracker.m_baims;

				if (_tickRecord)
				{
					if (_tickRecord->m_bFiredBullet)
					{
						if (hitgroup == HITGROUP_HEAD)
							++g_StatsTracker.m_shotbt_headshots;
						else
							++g_StatsTracker.m_shotbt_baims;
					}
				}
			}

			// log damage
			if (variable::get().visuals.i_shot_logs > 0)
			{
#if defined DEBUG_SHOTS || defined _DEBUG || defined INTERNAL_DEBUG
				g_Eventlog.AddEventElement(0x04, "[HL] ");
				g_Eventlog.AddEventElement(0x04, std::to_string(LocalPlayer.ShotRecord->m_flRealTimeAck)); // fuck VS 
				g_Eventlog.AddEventElement(0x04, " | ");
#endif

				// setup our string
				if (backtrack != 0)
				{
					if (!weapon.empty())
					{
						if (weapon[0] == 't' && weapon[2] == 's')
						{
							//decrypts(0)
							g_Eventlog.AddEventElement(0x04, XorStr("tased "));
							g_Eventlog.AddEventElement(0x04, std::to_string(backtrack));
							if (backtrack > 0)
								g_Eventlog.AddEventElement(0x04, XorStr(" backtrack ticks on \'"));
							else
								g_Eventlog.AddEventElement(0x04, XorStr(" forwardtrack ticks on \'"));
							//encrypts(0)
						}
						else if (weapon[0] == 'k' && weapon[2] == 'i')
						{
							//decrypts(0)
							g_Eventlog.AddEventElement(0x04, XorStr("stabbed "));
							g_Eventlog.AddEventElement(0x04, std::to_string(backtrack));
							if (backtrack > 0)
								g_Eventlog.AddEventElement(0x04, XorStr(" backtrack ticks on \'"));
							else
								g_Eventlog.AddEventElement(0x04, XorStr(" forwardtrack ticks on \'"));
							//encrypts(0)
						}
						else
						{
							//decrypts(0)
							g_Eventlog.AddEventElement(0x04, XorStr("hit a "));
							g_Eventlog.AddEventElement(0x04, std::to_string(backtrack));
							if (backtrack > 0)
								g_Eventlog.AddEventElement(0x04, XorStr("-tick backtrack on \'"));
							else
								g_Eventlog.AddEventElement(0x04, XorStr("-tick forwardtrack on \'"));
							//encrypts(0)
						}
					}

					g_Eventlog.AddEventElement(0x04, victim_name);
					//decrypts(0)
					g_Eventlog.AddEventElement(0x04, XorStr("\'"));
					//encrypts(0)

					// gear/generic
					if (hitgroup_name[0] != 'G')
					{
						//decrypts(0)
						g_Eventlog.AddEventElement(0x04, XorStr(" \'s "));
						//encrypts(0)
						g_Eventlog.AddEventElement(0x04, hitgroup_name);
					}
				}
				else
				{
					if (!weapon.empty())
					{
						if (weapon[0] == 'i' && weapon[2] == 'f')
						{
							//decrypts(0)
							g_Eventlog.AddEventElement(0x04, XorStr("burned \'"));
							//encrypts(0)
						}
						else if (weapon[0] == 't' && weapon[2] == 's')
						{
							//decrypts(0)
							g_Eventlog.AddEventElement(0x04, XorStr("tased \'"));
							//encrypts(0)
						}
						else if (weapon[0] == 'h' && weapon[2] == 'g')
						{
							//decrypts(0)
							g_Eventlog.AddEventElement(0x04, XorStr("naded \'"));
							//encrypts(0)
						}
						else
						{
							//decrypts(0)
							g_Eventlog.AddEventElement(0x04, XorStr("hit \'"));
							//encrypts(0)
						}
					}

					g_Eventlog.AddEventElement(0x04, victim_name);
					g_Eventlog.AddEventElement(0x04, "\'");

					// gear/generic
					if (hitgroup_name[0] != 'G')
					{
						//decrypts(0)
						g_Eventlog.AddEventElement(0x04, XorStr(" \'s "));
						g_Eventlog.AddEventElement(0x04, hitgroup_name);
						//encrypts(0)
					}
				}

				//decrypts(0)
				g_Eventlog.AddEventElement(0x04, XorStr(" for "));
				g_Eventlog.AddEventElement(0x04, std::to_string(damage));
				g_Eventlog.AddEventElement(0x04, XorStr(" dmg ( "));
				g_Eventlog.AddEventElement(0x04, std::to_string(max(0, healthleft)));
				g_Eventlog.AddEventElement(0x04, XorStr(" remaining ) "));
				//encrypts(0)

				// with/without a resolved
				// 'NONE'
				if (!resolve_name.empty() && resolve_name[0] != 'N')
				{
					//decrypts(0)
					g_Eventlog.AddEventElement(0x04, XorStr("via "));
					//encrypts(0)
					g_Eventlog.AddEventElement(0x04, resolve_name);
				}

				// check to see if they shot
				if (_tickRecord && _tickRecord->m_bFiredBullet)
				{
					//decrypts(0)
					g_Eventlog.AddEventElement(0x04, XorStr(" | SHOT"));
					//encrypts(0)
				}

				// check to see if they're real tick
				if (_tickRecord && _tickRecord->m_bRealTick)
				{
					//decrypts(0)
					g_Eventlog.AddEventElement(0x04, XorStr(" | REAL"));
					//encrypts(0)
				}

				g_Eventlog.OutputEvent(variable::get().visuals.i_shot_logs);
			}
		}


		bool _ShotRecordIsInflictor = LocalPlayer.ShotRecord->m_bIsInflictor;
		ShotResult *_shotResult = nullptr;

		for (auto& a : LocalPlayer.m_ShotResults)
		{
			if (a.first == Interfaces::Globals->realtime && a.second.m_bIsInflictor == _ShotRecordIsInflictor)
			{
				if (a.second.m_pInflictedEntity == _inflictedEntity && a.second.m_pInflictorEntity == _inflictorEntity)
				{
					_shotResult = &a.second;
					break;
				}
			}
		}

		if (!_shotResult)
		{

#if defined _DEBUG || defined INTERNAL_DEBUG
			printf("WARNING IN EVENTS.CPP: THIS MESSAGE SHOULD NOT HAPPEN! REPORT IT!\n");
			//this means that there wasn't an impact event that was found for this fire event. 
			//This shouldn't happen under the current code path since we process all impacts before processing player hurt events
#endif

			// didn't receive impact yet, queue this hit
			ShotResult *pShotResult = new ShotResult;
			if (pShotResult)
			{
				ShotResult& _ShotResult = *pShotResult;

				// this hasn't been drawn yet
				_ShotResult.m_bAcked = false;
				// wait for impact
				_ShotResult.m_bAwaitingImpact = true;
				_ShotResult.m_pInflictorEntity = LocalPlayer.ShotRecord->m_pInflictorEntity;
				_ShotResult.m_pInflictedEntity = LocalPlayer.ShotRecord->m_pInflictedEntity;
				// store time created
				_ShotResult.m_flTimeCreated = Interfaces::Globals->realtime;
				_ShotResult.m_MissFlags = 0;
				_ShotResult.m_iHitgroupShotAt = LocalPlayer.ShotRecord->m_iTargetHitgroup;
				_ShotResult.m_iHitgroupHit = hitgroup;
				_ShotResult.m_iDamageGiven = damage;
				_ShotResult.m_bDoesNotFoundTowardsStats = LocalPlayer.ShotRecord->m_bDoesNotFoundTowardsStats;
				_ShotResult.m_bUsedBodyHitResolveDelta = LocalPlayer.ShotRecord->m_bUsedBodyHitResolveDelta;
				_ShotResult.m_iBodyHitResolveStance = LocalPlayer.ShotRecord->m_iBodyHitResolveStance;
				_ShotResult.m_iTickCountWhenWeShot = LocalPlayer.ShotRecord->m_iTickCountWhenWeShot;
				_ShotResult.m_iEnemySimulationTickCount = LocalPlayer.ShotRecord->m_iEnemySimulationTickCount;
				_ShotResult.m_bIsInflictor = LocalPlayer.ShotRecord->m_bIsInflictor;
				_ShotResult.m_bInflictorIsLocalPlayer = _inflictorIsLocalPlayer;
				_ShotResult.m_bShotAtBalanceAdjust = LocalPlayer.ShotRecord->m_bShotAtBalanceAdjust;
				_ShotResult.m_bShotAtFreestanding = LocalPlayer.ShotRecord->m_bShotAtFreestanding;
				_ShotResult.m_bShotAtMovingResolver = LocalPlayer.ShotRecord->m_bShotAtMovingResolver;
#ifdef _DEBUG
				if ((int)_ShotResult.m_bUsedBodyHitResolveDelta < 0 || (int)_ShotResult.m_bUsedBodyHitResolveDelta > 1 || _ShotResult.m_iBodyHitResolveStance < 0 || _ShotResult.m_iBodyHitResolveStance > 6)
				{
					printf("[playerhurt_process] FUCKED SHOT RESULT | isbodyhitresolved: %d | stance: %d | notfoundtowardsstats: %d\n", _ShotResult.m_bUsedBodyHitResolveDelta, _ShotResult.m_iBodyHitResolveStance, _ShotResult.m_bDoesNotFoundTowardsStats);
					//DebugBreak();
				}
#endif

				LocalPlayer.m_ShotResults.emplace(Interfaces::Globals->realtime, _ShotResult);

				if (LocalPlayer.m_ShotResults.size() > 10 || Interfaces::Globals->realtime - LocalPlayer.m_ShotResults[0].m_flTimeCreated > 1.0f)
					LocalPlayer.m_ShotResults.erase(0);

				delete pShotResult;
			}
		}
		else
		{
			// store the entity that was hit
			_shotResult->m_pInflictedEntity = _inflictedEntity;
			if (_shotResult->m_iHitgroupHit == -1)
				_shotResult->m_iHitgroupHit = hitgroup;
			OnPlayerHurt(_inflictedRecord, *_shotResult, hitgroup, damage);
		}
	}
	// manual shot/stabs or burns
	else if (_inflictorIsLocalPlayer)
	{
		if (variable::get().visuals.i_shot_logs > 0 && _inflictedEntity != LocalPlayer.Entity)
		{
			if (!weapon.empty())
			{
				if (weapon[0] == 't' && weapon[2] == 's')
				{
					//decrypts(0)
					g_Eventlog.AddEventElement(0x04, XorStr("tased \'"));
					//encrypts(0)
				}
				else if (weapon[0] == 'k' && weapon[2] == 'i')
				{
					//decrypts(0)
					g_Eventlog.AddEventElement(0x04, XorStr("stabbed \'"));
					//encrypts(0)
				}
				else if (weapon[0] == 'h' && weapon[2] == 'g')
				{
					//decrypts(0)
					g_Eventlog.AddEventElement(0x04, XorStr("naded \'"));
					//encrypts(0)
				}
				else if (weapon[0] == 'i' && weapon[2] == 'f')
				{
					//decrypts(0)
					g_Eventlog.AddEventElement(0x04, XorStr("burned \'")); // dasdas
					//encrypts(0)
				}
				else
				{
					//decrypts(0)
					g_Eventlog.AddEventElement(0x04, XorStr("hit \'"));
					//encrypts(0)
				}
			}

			std::string victim_name = adr_util::sanitize_name((char*)_inflictedEntity->GetName().data());

			g_Eventlog.AddEventElement(0x04, victim_name);
			//decrypts(0)
			g_Eventlog.AddEventElement(0x04, XorStr("\'"));
			//encrypts(0)


			// gear/generic
			if (hitgroup_name[0] != 'G')
			{
				//decrypts(0)
				g_Eventlog.AddEventElement(0x04, XorStr(" \'s "));
				//encrypts(0)
				g_Eventlog.AddEventElement(0x04, hitgroup_name);
			}

			//decrypts(0)
			g_Eventlog.AddEventElement(0x04, XorStr(" for "));
			g_Eventlog.AddEventElement(0x04, std::to_string(damage));
			g_Eventlog.AddEventElement(0x04, XorStr(" dmg ( "));
			g_Eventlog.AddEventElement(0x04, std::to_string(max(0, healthleft)));
			g_Eventlog.AddEventElement(0x04, XorStr(" remaining ) "));
			//encrypts(0)

			g_Eventlog.OutputEvent(variable::get().visuals.i_shot_logs);
		}
	}
	LocalPlayer.ShotMutex.Unlock();
}

void GameEvent_PlayerHurt(CGameEvent* gameEvent)
{
	if (!LocalPlayer.Entity || !gameEvent)
		return;

	GameEvent_PlayerHurt_t ev;
	//decrypts(0)
	ev.targetuserid = gameEvent->GetInt(XorStr("userid"));
	ev.attackeruserid = gameEvent->GetInt(XorStr("attacker"));
	ev.hitgroup = gameEvent->GetInt(XorStr("hitgroup"), -1);
	ev.dmg_health = gameEvent->GetInt(XorStr("dmg_health"), -1);
	ev.health = gameEvent->GetInt(XorStr("health"), -1);
	ev.weapon = gameEvent->GetString(XorStr("weapon"), "");
	//encrypts(0)

#if defined DEBUG_SHOTS || defined _DEBUG || defined INTERNAL_DEBUG
	if (!LocalPlayer.Entity->IsSpectating())
	{
		const int color = (Interfaces::EngineClient->GetPlayerForUserID(ev.attackeruserid) == Interfaces::EngineClient->GetLocalPlayer()) ? 0x6 : 0x7;

		g_Eventlog.AddEventElement(0x7, "[PH] ");
		g_Eventlog.AddEventElement(color, std::to_string(Interfaces::Globals->realtime));
		g_Eventlog.AddEventElement(color, " | ");
		g_Eventlog.AddEventElement(color, std::to_string(Interfaces::EngineClient->GetPlayerForUserID(ev.attackeruserid)));
		g_Eventlog.AddEventElement(color, " -> ");
		g_Eventlog.AddEventElement(color, std::to_string(Interfaces::EngineClient->GetPlayerForUserID(ev.targetuserid)));

		g_Eventlog.OutputEvent(1);
	}
#endif

	if (variable::get().visuals.i_hurt_logs > 0)
	{
		int user_id = Interfaces::EngineClient->GetPlayerForUserID(ev.targetuserid);
		if (user_id == Interfaces::EngineClient->GetLocalPlayer())
		{
#if defined DEBUG_SHOTS || defined _DEBUG || defined INTERNAL_DEBUG
			g_Eventlog.AddEventElement(0xF, "[HT] ");
#endif

			CBaseEntity* attacker = (CBaseEntity*)Interfaces::ClientEntList->GetClientEntity(Interfaces::EngineClient->GetPlayerForUserID(ev.attackeruserid));
			if (attacker != nullptr)
			{
				// world damage
				if (attacker->index == 0)
				{
					if (!g_bBombExploded)
					{
						//decrypts(0)
						g_Eventlog.AddEventElement(0xF, XorStr("you received fall damage"));
						//encrypts(0)
					}
					else
					{
						//decrypts(0)
						g_Eventlog.AddEventElement(0xF, XorStr("you took bomb damage"));
						//encrypts(0)
					}
				}
				else if(attacker == LocalPlayer.Entity)
				{
					if (!ev.weapon.empty())
					{
						if (ev.weapon[0] == 'i' && ev.weapon[2] == 'f')
						{
							//decrypts(0)
							g_Eventlog.AddEventElement(0xF, XorStr("you burned yourself"));
							//encrypts(0)
						}
						if (ev.weapon[0] == 'h' && ev.weapon[2] == 'g')
						{
							//decrypts(0)
							g_Eventlog.AddEventElement(0xF, XorStr("you naded yourself"));
							//encrypts(0)
						}
						if (ev.weapon[0] == 'd' && ev.weapon[2] == 'c')
						{
							//decrypts(0)
							g_Eventlog.AddEventElement(0xF, XorStr("you decoyed yourself"));
							//encrypts(0)
						}
					}
				}
				else if(attacker != LocalPlayer.Entity)
				{
					g_Eventlog.AddEventElement(0xF, adr_util::sanitize_name((char*)attacker->GetName().data()));
					
					if (!ev.weapon.empty())
					{
						if (ev.weapon[0] == 't' && ev.weapon[2] == 's') // taser
						{
							//decrypts(0)
							g_Eventlog.AddEventElement(0xF, XorStr(" tased you"));
							//encrypts(0)
						}
						else if (ev.weapon[0] == 'i' && ev.weapon[2] == 'f') // inferno
						{
							//decrypts(0)
							g_Eventlog.AddEventElement(0xF, XorStr(" burned you"));
							//encrypts(0)
						}
						else if (ev.weapon[0] == 'k' && ev.weapon[2] == 'i') // knife
						{
							//decrypts(0)
							g_Eventlog.AddEventElement(0xF, XorStr(" stabbed you"));
							//encrypts(0)
						}
						else if (ev.weapon[0] == 'h' && ev.weapon[2] == 'g')
						{
							//decrypts(0)
							g_Eventlog.AddEventElement(0xF, XorStr(" naded you"));
							//encrypts(0)
						}
						else if (ev.weapon[0] == 'd' && ev.weapon[2] == 'c')
						{
							//decrypts(0)
							g_Eventlog.AddEventElement(0xF, XorStr(" decoyed you"));
							//encrypts(0)
						}
						else
						{
							if (ev.hitgroup > 0 && ev.hitgroup <= HITGROUP_GEAR)
							{
								//decrypts(0)
								g_Eventlog.AddEventElement(0xF, XorStr(" hit you in the "));
								//encrypts(0)
								g_Eventlog.AddEventElement(0xF, get_hitgroup_name(ev.hitgroup).data());
							}
						}
					}
				}

				//decrypts(0)
				g_Eventlog.AddEventElement(0xF, XorStr(" for "));
				g_Eventlog.AddEventElement(0xF, std::to_string(ev.dmg_health));
				g_Eventlog.AddEventElement(0xF, XorStr(" dmg"));
				//encrypts(0)

				g_Eventlog.OutputEvent(variable::get().visuals.i_hurt_logs);
			}
		}
	}

	g_GameEvent_PlayerHurt_Queue.push_back(ev);
}

void GameEvent_PlayerDeath_ProcessQueuedEvent(const GameEvent_PlayerDeath_t& gameEvent)
{
	//const int local_player = Interfaces::EngineClient->GetLocalPlayer();
	const int killed = Interfaces::EngineClient->GetPlayerForUserID(gameEvent.user_id);
	const int killer = Interfaces::EngineClient->GetPlayerForUserID(gameEvent.attacker_id);
	//const int assister = Interfaces::EngineClient->GetPlayerForUserID(gameEvent.assister_id);

	CBaseEntity* killed_entity = Interfaces::ClientEntList->GetBaseEntity(killed);
	CBaseEntity* killer_entity = Interfaces::ClientEntList->GetBaseEntity(killer);
	CPlayerrecord *_playerRecord = g_LagCompensation.GetPlayerrecord(killed_entity);
	//CPlayerrecord *_killerPlayerRecord = g_LagCompensation.GetPlayerrecord(killer_entity);


	if (_playerRecord)
	{
		_playerRecord->m_pKilledByEntity = killer_entity;
		_playerRecord->m_iLastKilledTickcount = g_ClientState->m_ClockDriftMgr.m_nServerTick;
	}
}

void GameEvent_PlayerHurt_PreProcessQueuedEvent(const GameEvent_PlayerHurt_t& gameEvent)
{
	// get involved entities
	CBaseEntity* _inflictedEntity = Interfaces::ClientEntList->GetBaseEntity(Interfaces::EngineClient->GetPlayerForUserID(gameEvent.targetuserid));
	CBaseEntity* _inflictorEntity = Interfaces::ClientEntList->GetBaseEntity(Interfaces::EngineClient->GetPlayerForUserID(gameEvent.attackeruserid));

	// skip invalid
	if (!_inflictedEntity || !_inflictorEntity)
		return;

	// get playerrecord
	CPlayerrecord* _inflictedRecord = g_LagCompensation.GetPlayerrecord(_inflictedEntity);
	CPlayerrecord* _inflictorRecord = g_LagCompensation.GetPlayerrecord(_inflictorEntity);

	// don't do stuff if we have no info
	if (!_inflictedRecord || !_inflictorRecord)
		return;

	bool _inflictorIsLocalPlayer = _inflictorRecord->m_pEntity == LocalPlayer.Entity;

	//If the inflictor is the local player, the inflicted entity is already set when the shot record was created, so we don't need to preprocess this until the future when we fix handling multiple hit enemies at once
	if (_inflictorIsLocalPlayer)
		return;

	LocalPlayer.ShotMutex.Lock();

	// reset current record
	LocalPlayer.ShotRecord = nullptr;

	// get shot records
	const auto& ShotRecords = _inflictorRecord->ShotRecords;

	//float latency = Interfaces::EngineClient->GetNetChannelInfo()->GetAvgLatency(FLOW_OUTGOING);
	float besttimedelta = FLT_MAX;
	CShotrecord* bestshotrecord = nullptr;
	CBaseEntity *besttarget;

	// loop through all shots
	for (auto& _shot : ShotRecords)
	{
		// too long ago, ignore
		float timedelta = fabsf(Interfaces::Globals->realtime - _shot->m_flRealTime);
		if (timedelta > 1.0f)
			continue;

		if (!_shot->m_bAck && _shot->m_bInflictorIsLocalPlayer == _inflictorIsLocalPlayer)
		{
			bool _isValid = false;

			/*
			if (_inflictorIsLocalPlayer)
			{
				if (_shot->m_pInflictedEntity == _inflictedEntity)
					_isValid = true;
			}
			else
			*/
			{
				if (!_shot->m_pInflictedEntity && _shot->m_pInflictorEntity == _inflictorEntity)
					_isValid = true;
			}

			if (_isValid)
			{
				float _predictedReceiveTime = _shot->m_flRealTime + _shot->m_flLatency;
				float _predictedActualDelta = fabsf(Interfaces::Globals->realtime - _predictedReceiveTime);

				if (_predictedActualDelta < besttimedelta)
				{
					bestshotrecord = _shot;
					besttimedelta = _predictedActualDelta;
				}
			}
		}
	}

	if (bestshotrecord)
	{
		bestshotrecord->m_pInflictedEntity = _inflictedEntity;
		bestshotrecord->m_iTargetHitgroup = gameEvent.hitgroup;
		bestshotrecord->m_iActualHitgroup = gameEvent.hitgroup;
	}

	LocalPlayer.ShotMutex.Unlock();
}

void GameEvent_Impact_ProcessQueuedEvent(const GameEvent_Impact_t& gameEvent)
{
	// get impact
	const Vector &_impact = gameEvent.impactpos;
	const int _iUser = Interfaces::EngineClient->GetPlayerForUserID(gameEvent.attackeruserid);

	// localplayer shot
	bool _InflictorWasLocalPlayer = _iUser == LocalPlayer.Entity->index;
	//if (_InflictorWasLocalPlayer)
	{
		// draw serverside impacts
		if (variable::get().visuals.b_impacts && _InflictorWasLocalPlayer)
			// todo: nit; make a timed directx box overlay
			Interfaces::DebugOverlay->AddBoxOverlay(_impact, Vector(-2, -2, -2), Vector(2, 2, 2), QAngle(0, 0, 0), 0, 0, 255, 127, 4);


		CPlayerrecord &_inflictor = *g_LagCompensation.GetPlayerrecord(_iUser);
		if (!&_inflictor)
			return;

		LocalPlayer.ShotMutex.Lock();

		// reset current record
		LocalPlayer.ShotRecord = nullptr;

		// get shot records
		const auto& ShotRecords = _inflictor.ShotRecords;

		//float latency = Interfaces::EngineClient->GetNetChannelInfo()->GetAvgLatency(FLOW_OUTGOING);
		float besttimedelta = FLT_MAX;
		CShotrecord* bestshotrecord = nullptr;

		// loop through all shots
		for (auto& _shot : ShotRecords)
		{
			// too long ago, ignore
			float timedelta = fabsf(Interfaces::Globals->realtime - _shot->m_flRealTime);
			if (timedelta > 1.0f)
				continue;

			float _predictedReceiveTime = _shot->m_flRealTime + _shot->m_flLatency;
			float _predictedActualDelta = fabsf(Interfaces::Globals->realtime - _predictedReceiveTime);

			if (_predictedActualDelta < besttimedelta)
			{
				bestshotrecord = _shot;
				besttimedelta = _predictedActualDelta;
			}
		}

		if (bestshotrecord)
		{
			Vector vecDir = _impact - bestshotrecord->m_vecLocalEyePos;
			VectorNormalizeFast(vecDir);
			QAngle angs;
			VectorAngles(vecDir, angs);
			NormalizeAngles(angs);

			// if we are getting another impact event for the same tick then just use the one we already acknowledged
			// we can't shoot more than 1 bullet at a time, you know
			if (bestshotrecord->m_flRealTimeAck == Interfaces::Globals->realtime)
			{
#ifdef _DEBUG
				if (weapon_accuracy_nospread.GetVar()->GetBool())
				{
					Vector vecDir2 = _impact - bestshotrecord->m_vecLocalEyePos;

					VectorNormalizeFast(vecDir2);
					QAngle angs2;
					VectorAngles(vecDir2, angs2);
					NormalizeAngles(angs2);
					if (fabsf(AngleDiff(angs2.x, bestshotrecord->m_angLocalEyeAngles.x)) > 1.2f || fabsf(AngleDiff(angs2.y, bestshotrecord->m_angLocalEyeAngles.y)) > 1.2f)
					{
						printf("ERROR: received impact event for a different shot\n");
					}
				}
#endif

				//Check to see if this is the same impact creation time by comparing the impact angles to the shot angles
				//if (fabsf(AngleDiff(angs.x, _shot->m_ImpactAngles.x)) < 0.05f && fabsf(AngleDiff(angs.y, _shot->m_ImpactAngles.y)) < 0.05f)
				//{
				LocalPlayer.ShotRecord = bestshotrecord;
				//}
				//else
				//{
					//printf("shot doesn't match angles\n");
				//}
			}
			else
			{
				if (!bestshotrecord->m_bAck)
				{
					bestshotrecord->m_bAck = true;
					bestshotrecord->m_bMissed = true;
					bestshotrecord->m_bImpactEventAcked = true;
					bestshotrecord->m_flRealTimeAck = Interfaces::Globals->realtime;
					bestshotrecord->m_ImpactAngles = angs;
					LocalPlayer.ShotRecord = bestshotrecord;
				}
			}
		}

		// we shot at a player
		if (LocalPlayer.ShotRecord)
		{
			if (!LocalPlayer.ShotRecord->m_pInflictedEntity)
			{
				auto& var = variable::get();

				if (!_InflictorWasLocalPlayer && var.ragebot.b_antiaim && var.ragebot.b_enabled)
				{
					if (LocalPlayer.m_iLastAASwapTick != g_ClientState->m_ClockDriftMgr.m_nServerTick)
					{
						Vector vecDirFromLocalPlayerToImpact = _impact - LocalPlayer.ShotRecord->m_vecLocalEyePos;
						VectorNormalizeFast(vecDirFromLocalPlayerToImpact);
						QAngle angle;
						VectorAngles(vecDirFromLocalPlayerToImpact, angle);
						QAngle angletous = CalcAngle(LocalPlayer.ShotRecord->m_vecLocalEyePos, LocalPlayer.Entity->GetEyePosition());
						float fovy = fabsf(AngleDiff(angle.y, angletous.y));
						float fovx = fabsf(AngleDiff(angle.x, angletous.x));
						if (fovy < ANTI_AA_YAW && fovx < ANTI_AA_PITCH)
						{
							//they shot near us, swap our side
#if 1
							auto style = LocalPlayer.GetDesyncStyle();
							var.ragebot.moving.i_desync_style = !style;
							var.ragebot.standing.i_desync_style = !style;
							var.ragebot.in_air.i_desync_style = !style;
							var.ragebot.minwalking.i_desync_style = !style;
							LocalPlayer.m_iLastAASwapTick = g_ClientState->m_ClockDriftMgr.m_nServerTick;
#endif
						}
					}
				}
			}
			else
			{
				// get player
				CBaseEntity* _Entity = Interfaces::ClientEntList->PlayerExists(LocalPlayer.ShotRecord->m_pInflictedEntity);

				// get playerrecord
				CPlayerrecord* _playerRecord = g_LagCompensation.GetPlayerrecord(_Entity);

				// invalid player
				if (!_Entity)
				{
					LocalPlayer.ShotMutex.Unlock();
					return;
				}

				CPlayerrecord::s_Shot &LastShot = LocalPlayer.ShotRecord->m_bIsInflictor ? _playerRecord->LastShotAsEnemy : _playerRecord->LastShot;

				// valid playerrecord and player
				if (_playerRecord)
				{
					bool _HaventSetLastImpactPos = LastShot.m_iTickHitWall != Interfaces::Globals->tickcount && LastShot.m_iTickHitPlayer != Interfaces::Globals->tickcount;

					if (_HaventSetLastImpactPos)
					{
						LastShot.m_vecMainImpactPos = _impact;
						LastShot.m_bReceivedExactImpactPos = false;
						LastShot.m_vecImpactPositions.clear();

						//store all of the impact positions for this shot
						for (const auto& ev : g_GameEvent_Impact_Queue)
						{
							if (ev.attackeruserid == gameEvent.attackeruserid)
								LastShot.m_vecImpactPositions.push_back(ev.impactpos);
						}
					}

					Vector vecDirFromLocalPlayerToImpact = _impact - LocalPlayer.ShotRecord->m_vecLocalEyePos;
					VectorNormalizeFast(vecDirFromLocalPlayerToImpact);
					trace_t tr;
					//UTIL_TraceLine(LocalPlayer.ShotRecord->m_vecLocalEyePos - vecDirFromLocalPlayerToImpact, LocalPlayer.ShotRecord->m_vecLocalEyePos + vecDirFromLocalPlayerToImpact, MASK_ALL, LocalPlayer.Entity, COLLISION_GROUP_NONE, &tr);
					CTraceFilterSimple filter;
					filter.pSkip = nullptr;
					CTraceFilterInterited_DisablePlayers inheritedfilter(filter, COLLISION_GROUP_NONE);
					Ray_t ray(_impact - vecDirFromLocalPlayerToImpact, _impact + vecDirFromLocalPlayerToImpact);
					Interfaces::EngineTrace->TraceRay(ray, MASK_SHOT, &inheritedfilter, &tr);

					if (!tr.m_pEnt || (tr.m_pEnt->index != 0 && (tr.m_pEnt->GetSolidFlags() & FSOLID_NOT_SOLID || tr.contents == CONTENTS_EMPTY || tr.contents & CONTENTS_SLIME || tr.contents & CONTENTS_WATER || tr.contents & CONTENTS_MONSTER || tr.contents & CONTENTS_HITBOX)))
					{
						LastShot.m_vecMainImpactPos = _impact;
						LastShot.m_bReceivedExactImpactPos = true;
						//Interfaces::DebugOverlay->AddBoxOverlay(_impact, Vector(-5, -5, -5), Vector(5, 5, 5), angZero, 0, 255, 0, 255, 3.0f);
					}

					// only use first impact
					if (LastShot.m_iTickHitWall != g_ClientState->m_ClockDriftMgr.m_nServerTick)
					{
						// update impact tick
						LastShot.m_iTickHitWall = g_ClientState->m_ClockDriftMgr.m_nServerTick;

						if (!((_Entity->GetAlive() || (_playerRecord->m_iLastKilledTickcount == g_ClientState->m_ClockDriftMgr.m_nServerTick /*&& LastShot.m_iTickHitWall != Interfaces::Globals->tickcount*/))))
						{
							int fucker = 0;
						}

						auto& var = variable::get();
						if (!_InflictorWasLocalPlayer && var.ragebot.b_antiaim && var.ragebot.b_enabled)
						{
							if (LocalPlayer.m_iLastAASwapTick != g_ClientState->m_ClockDriftMgr.m_nServerTick)
							{
								Vector vecDirFromLocalPlayerToImpact = _impact - LocalPlayer.ShotRecord->m_vecLocalEyePos;
								VectorNormalizeFast(vecDirFromLocalPlayerToImpact);
								QAngle angle;
								VectorAngles(vecDirFromLocalPlayerToImpact, angle);
								QAngle angletous = CalcAngle(LocalPlayer.ShotRecord->m_vecLocalEyePos, LocalPlayer.Entity->GetEyePosition());
								float fovy = fabsf(AngleDiff(angle.y, angletous.y));
								float fovx = fabsf(AngleDiff(angle.x, angletous.x));
								if (fovy < ANTI_AA_YAW && fovx < ANTI_AA_PITCH)
								{
#if 1
									//they shot near us, swap our side
									auto style = LocalPlayer.GetDesyncStyle();
									var.ragebot.moving.i_desync_style = !style;
									var.ragebot.standing.i_desync_style = !style;
									var.ragebot.in_air.i_desync_style = !style;
									var.ragebot.minwalking.i_desync_style = !style;
									LocalPlayer.m_iLastAASwapTick = g_ClientState->m_ClockDriftMgr.m_nServerTick;
#endif
								}
							}
						}

						//If the player is still alive, count all impacts, otherwise only count the first and oldest impact
						//FIXME TODO: dump this shot impact if the enemy killed the player
						if (_Entity->GetAlive() || (_playerRecord->m_iLastKilledTickcount == g_ClientState->m_ClockDriftMgr.m_nServerTick /*&& LastShot.m_iTickHitWall != Interfaces::Globals->tickcount*/))
						{
							// don't process the tick if we hit the player and processed it already
							// note: this check is useless now because we process impacts before player hurt events
							//if (LastShot.m_iTickHitPlayer != Interfaces::Globals->tickcount)
							{
								/*
								bool _targetIsInflictor = LocalPlayer.ShotRecord->m_bIsInflictor;
								auto &_ShotResultEnd = std::end(LocalPlayer.m_ShotResults);
								auto result = std::find_if(LocalPlayer.m_ShotResults.begin(), LocalPlayer.m_ShotResults.end(), [_Entity, _targetIsInflictor](const std::pair<float, ShotResult>& a) {
									return a.first == Interfaces::Globals->realtime && a.second.m_pEntity == _Entity && a.second.m_bAwaitingImpact && a.second.m_bIsInflictor == _targetIsInflictor;
								});*/

								ShotResult *_NewShotResult = nullptr;
								ShotResult *_ShotResult;

								/*
								if (result != _ShotResultEnd)
								{
									// we received a shot before an impact, so don't create a new shot result
									_ShotResult = &result->second;
								}
								else
								*/
								{
									_NewShotResult = new ShotResult;
									_ShotResult = _NewShotResult;
								}

								// this hasn't been acked yet
								_ShotResult->m_bAcked = false;

								// no longer awaiting for an impact
								_ShotResult->m_bAwaitingImpact = false;

								_ShotResult->m_pInflictorEntity = LocalPlayer.ShotRecord->m_pInflictorEntity;
								_ShotResult->m_pInflictedEntity = LocalPlayer.ShotRecord->m_pInflictedEntity;

								// store time created
								_ShotResult->m_flTimeCreated = Interfaces::Globals->realtime;

								trace_t server_trace_results;

								// check for spread miss
								_ShotResult->m_MissFlags = _InflictorWasLocalPlayer ? GetShotMissReason(_Entity, LocalPlayer.ShotRecord, (Vector&)_impact, LastShot, &server_trace_results) : MISS_SHOULDNT_HAVE_MISSED;

#if defined _DEBUG || defined INTERNAL_DEBUG
								if (_ShotResult->m_MissFlags & MISS_RAY_OFF_TARGET)
								{
#if 0
									//DebugBreak();
									//Interfaces::DebugOverlay->ClearAllOverlays();
									//_Entity->DrawHitboxesFromCache(ColorRGBA(255, 0, 0, 200), 4.0f, LocalPlayer.ShotRecord->m_Tickrecord->m_PlayerBackup.CachedBoneMatrices);

									//draw shot position
									Interfaces::DebugOverlay->AddBoxOverlay(LocalPlayer.ShotRecord->m_vecLocalEyePos, Vector(-0.1, -0.1, -0.1), Vector(0.1, 0.1, 0.1), LocalPlayer.ShotRecord->m_angLocalEyeAngles, 255, 0, 0, 150, 3.0f);

									//draw line to server impact position
									Vector vecDirFromLocalPlayerToImpact = _impact - LocalPlayer.ShotRecord->m_vecLocalEyePos;
									VectorNormalizeFast(vecDirFromLocalPlayerToImpact);
									Interfaces::DebugOverlay->AddLineOverlay(LocalPlayer.ShotRecord->m_vecLocalEyePos, LocalPlayer.ShotRecord->m_vecLocalEyePos + (vecDirFromLocalPlayerToImpact * 2048.0f), 0, 255, 255, true, 10.0f);

									//draw line to where we wanted to shoot at
									Vector vecDirShotAngles;
									AngleVectors(LocalPlayer.ShotRecord->m_angLocalEyeAngles, &vecDirShotAngles);
									VectorNormalizeFast(vecDirShotAngles);
									Interfaces::DebugOverlay->AddLineOverlay(LocalPlayer.ShotRecord->m_vecLocalEyePos, LocalPlayer.ShotRecord->m_vecLocalEyePos + (vecDirShotAngles * 512.0f), 255, 0, 0, true, 10.0f);
#endif
								}
#endif

								// we haven't counted a miss yet
								_ShotResult->m_bCountedMiss = false;
								_ShotResult->m_bCountedBodyHitMiss = false;

								// store the resolve mode we used for this shot
								_ShotResult->m_ResolveMode = LocalPlayer.ShotRecord->m_iResolveMode;
								_ShotResult->m_ResolveSide = LocalPlayer.ShotRecord->m_iResolveSide;
								_ShotResult->m_bEnemyFiredBullet = LocalPlayer.ShotRecord->m_bEnemyFiredBullet;
								_ShotResult->m_bEnemyIsNotChoked = LocalPlayer.ShotRecord->m_bEnemyIsNotChoked;
								_ShotResult->m_bLegit = LocalPlayer.ShotRecord->m_bLegit;
								_ShotResult->m_bForceNotLegit = LocalPlayer.ShotRecord->m_bForceNotLegit;
								_ShotResult->m_iHitgroupShotAt = server_trace_results.hitgroup;
								_ShotResult->m_bDoesNotFoundTowardsStats = LocalPlayer.ShotRecord->m_bDoesNotFoundTowardsStats;
								_ShotResult->m_bUsedBodyHitResolveDelta = LocalPlayer.ShotRecord->m_bUsedBodyHitResolveDelta;
								_ShotResult->m_iBodyHitResolveStance = LocalPlayer.ShotRecord->m_iBodyHitResolveStance;
								_ShotResult->m_iTickCountWhenWeShot = LocalPlayer.ShotRecord->m_iTickCountWhenWeShot;
								_ShotResult->m_iEnemySimulationTickCount = LocalPlayer.ShotRecord->m_iEnemySimulationTickCount;
								_ShotResult->m_bIsInflictor = LocalPlayer.ShotRecord->m_bIsInflictor;
								_ShotResult->m_bInflictorIsLocalPlayer = _InflictorWasLocalPlayer;
								_ShotResult->m_bForwardTracked = LocalPlayer.ShotRecord->m_bForwardTracked;
								_ShotResult->m_bShotAtBalanceAdjust = LocalPlayer.ShotRecord->m_bShotAtBalanceAdjust;
								_ShotResult->m_bShotAtFreestanding = LocalPlayer.ShotRecord->m_bShotAtFreestanding;
								_ShotResult->m_bShotAtMovingResolver = LocalPlayer.ShotRecord->m_bShotAtMovingResolver;

								// track stats
#if 0
								if (_InflictorWasLocalPlayer)
								{
									if (_ShotResult->m_MissFlags != 0 &&
										!(_ShotResult->m_MissFlags & MISS_FROM_BAD_HITGROUP) &&
										!(_ShotResult->m_MissFlags & HIT_PLAYER) &&
										!(_ShotResult->m_MissFlags & MISS_SHOULDNT_HAVE_MISSED))
									{
										if (LocalPlayer.ShotRecord->m_iTargetHitgroup == HITGROUP_HEAD || LocalPlayer.ShotRecord->m_iTargetHitgroup == HITGROUP_NECK)
											++g_StatsTracker.m_head_misses;
										else if (LocalPlayer.ShotRecord->m_iTargetHitgroup == HITGROUP_CHEST || LocalPlayer.ShotRecord->m_iTargetHitgroup == HITGROUP_STOMACH)
											++g_StatsTracker.m_body_misses;
										else if (LocalPlayer.ShotRecord->m_iTargetHitgroup == HITGROUP_LEFTARM || LocalPlayer.ShotRecord->m_iTargetHitgroup == HITGROUP_RIGHTARM)
											++g_StatsTracker.m_arms_misses;
										else if (LocalPlayer.ShotRecord->m_iTargetHitgroup == HITGROUP_LEFTLEG || LocalPlayer.ShotRecord->m_iTargetHitgroup == HITGROUP_RIGHTLEG)
											++g_StatsTracker.m_lower_misses;
									}
								}
#endif

								if (_InflictorWasLocalPlayer && (variable::get().ragebot.b_make_all_misses_count || _ShotResult->m_MissFlags & MISS_SHOULDNT_HAVE_MISSED))
								{
									IncrementMissedShots(_playerRecord, LocalPlayer.ShotRecord, *_ShotResult);
								}

								/*
								if (result != _ShotResultEnd)
								{
									// we received impact after player hurt, so now run the queued up hurt event
									OnPlayerHurt(_playerRecord, *_ShotResult, _ShotResult->m_iHitgroupHit, _ShotResult->m_iDamageGiven);
								}
								else
								*/
								{
									LocalPlayer.m_ShotResults.emplace(Interfaces::Globals->realtime, *_ShotResult);

									if (LocalPlayer.m_ShotResults.size() > 10 || Interfaces::Globals->realtime - LocalPlayer.m_ShotResults[0].m_flTimeCreated > 1.0f)
										LocalPlayer.m_ShotResults.erase(0);
								}

								if (_NewShotResult)
									delete _NewShotResult;
							}
						}
					}
				}
				// no specific player targeted
				else if (_InflictorWasLocalPlayer)
				{
					//check if we manually fired
					if (fabsf(Interfaces::Globals->realtime - LocalPlayer.ManualFireShotInfo.m_flLastManualShotRealTime) < 1.0f)
					{
						// only draw first impact
						if (LocalPlayer.ManualFireShotInfo.m_iTickHitWall != g_ClientState->m_ClockDriftMgr.m_nServerTick)
						{
							// update impact tick
							LocalPlayer.ManualFireShotInfo.m_iTickHitWall = g_ClientState->m_ClockDriftMgr.m_nServerTick;

							// don't process the tick if we hit the player
							if (LocalPlayer.ManualFireShotInfo.m_iTickHitPlayer != g_ClientState->m_ClockDriftMgr.m_nServerTick)
							{
								//Vector vecDirFromLocalPlayerToImpact = _impact - LocalPlayer.ManualFireShotInfo.m_vecLocalEyePosition;
								//VectorNormalizeFast(vecDirFromLocalPlayerToImpact);
								//trace_t tr;
								//UTIL_TraceLine(_impact - vecDirFromLocalPlayerToImpact, _impact + vecDirFromLocalPlayerToImpact, MASK_ALL, LocalPlayer.Entity, COLLISION_GROUP_NONE, &tr);

								//if (!tr.m_pEnt || tr.m_pEnt->IsPlayer() || tr.contents == CONTENTS_EMPTY || tr.contents & CONTENTS_SLIME || tr.contents & CONTENTS_WATER || tr.contents & CONTENTS_MONSTER || tr.contents & CONTENTS_HITBOX)
								LocalPlayer.ManualFireShotInfo.m_vecLastImpactPos = _impact;
							}
						}
					}
				}
			}
		}
		LocalPlayer.ShotMutex.Unlock();
	}
}

void GameEvent_BulletImpact(CGameEvent* gameEvent)
{
	// ignore when we can
	if (!LocalPlayer.Entity || !gameEvent)
		return;

	GameEvent_Impact_t ev;
	//decrypts(0)
	ev.impactpos.x = gameEvent->GetFloat(XorStr("x"));
	ev.impactpos.y = gameEvent->GetFloat(XorStr("y"));
	ev.impactpos.z = gameEvent->GetFloat(XorStr("z"));
	ev.attackeruserid = gameEvent->GetInt(XorStr("userid"));
	//encrypts(0)

#if defined _DEBUG || defined INTERNAL_DEBUG
	if (!LocalPlayer.Entity->IsSpectating())
	{
		const int color = (Interfaces::EngineClient->GetPlayerForUserID(ev.attackeruserid) == Interfaces::EngineClient->GetLocalPlayer()) ? 0x6 : 0x8;

		g_Eventlog.AddEventElement(0x8, "[BI] ");
		g_Eventlog.AddEventElement(color, std::to_string(Interfaces::Globals->realtime));
		g_Eventlog.AddEventElement(color, " | ");
		g_Eventlog.AddEventElement(color, std::to_string(Interfaces::EngineClient->GetPlayerForUserID(ev.attackeruserid)));
		g_Eventlog.AddEventElement(color, " @ ");
		g_Eventlog.AddEventElement(color, std::to_string(ev.impactpos.x));
		g_Eventlog.AddEventElement(color, ", ");
		g_Eventlog.AddEventElement(color, std::to_string(ev.impactpos.y));
		g_Eventlog.AddEventElement(color, ", ");
		g_Eventlog.AddEventElement(color, std::to_string(ev.impactpos.z));
		g_Eventlog.OutputEvent(1);
	}
#endif

	g_GameEvent_Impact_Queue.push_back(ev);
}

void GameEvent_SwitchTeam(CGameEvent* gameEvent)
{
	// ignore when we can
	if (!LocalPlayer.Entity || !gameEvent)
		return;

	// fix clantags not being set on switching teams
	if (variable::get().misc.clantag.b_enabled.get())
	{
		clantag_changer::get().reset();
	}
}

void GameEvent_StartHalftime(CGameEvent* gameEvent)
{
	// ignore when we can
	if (!LocalPlayer.Entity || !gameEvent)
		return;
}


void GameEvent_RoundMVP(CGameEvent* gameEvent)
{
	// ignore when we can
	if (!LocalPlayer.Entity || !gameEvent)
		return;
}

void GameEvent_RoundEnd(CGameEvent* gameEvent)
{
	ResetAllWaypointTemporaryFlags();

	if (!LocalPlayer.Entity || !gameEvent)
		return;

	//decrypts(0)
	//const int winner_id = gameEvent->GetInt(XorStr("winner"));
	////encrypts(0)
	//
	//// winner - winner team/user id
	//
	//if (variable::get().misc.i_clantag > 0)
	//{
	//	g_Advertising.m_bNeedToReapply = true;
	//
	//	if (winner_id == Interfaces::EngineClient->GetLocalPlayer() || winner_id == LocalPlayer.Entity->GetTeam())
	//	{
	//		// force static clantag so people can read the chat that tapped them all game
	//		g_Advertising.m_forceStatic = true;
	//	}
	//	else
	//	{
	//		g_Advertising.m_forceStatic = false;
	//	}
	//}
}

void GameEvent_BombExploded(CGameEvent* gameEvent)
{
	if (!LocalPlayer.Entity || !gameEvent)
		return;

	g_bBombExploded = true;
}

void GameEvent_WeaponFire(CGameEvent* gameEvent)
{
	if (!LocalPlayer.Entity || !gameEvent)
		return;

#if defined DEBUG_SHOTS || defined _DEBUG || defined INTERNAL_DEBUG
	if(!LocalPlayer.Entity->IsSpectating())
	{
		const int index = Interfaces::EngineClient->GetPlayerForUserID(gameEvent->GetInt("userid"));
		const int color = (index == Interfaces::EngineClient->GetLocalPlayer()) ? 0x6 : 0x3;

		g_Eventlog.AddEventElement(0x3, "[WF] ");
		g_Eventlog.AddEventElement(color, std::to_string(Interfaces::Globals->realtime));
		g_Eventlog.AddEventElement(color, " | ");
		g_Eventlog.AddEventElement(color, std::to_string(index));
		g_Eventlog.OutputEvent(1);
	}
#endif
}
