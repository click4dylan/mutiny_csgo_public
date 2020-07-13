#include "precompiled.h"
#include "Aimbot_imi.h"
#include "AutoBone_imi.h"
#include "Eventlog.h"
#include "LocalPlayer.h"
#include "WeaponController.h"
#include "Assistance.h"
#include "Events.h"
#include "UsedConvars.h"
#include "INetchannelInfo.h"
#include "Removals.h"
#include "datamap.h"

#include "Adriel/stdafx.hpp"
#include "Adriel/renderer.hpp"
#include "Adriel/adr_util.hpp"
#include "Adriel/console.hpp"

#include "Adriel/input.hpp"

CRagebot g_Ragebot;
CRagebot m_RagebotState;
//CLegitbot_imi g_Legitbot;

void SmoothAngle(QAngle& From, QAngle& To, float Percent)
{
	QAngle VecDelta = From - To;
	NormalizeAngles(VecDelta);

	VecDelta *= Percent;
	To = From - VecDelta;
}

void CRagebot::ClearAimbotTarget()
{
	LocalPlayer.UseDoubleTapHitchance = false;
	m_LastTarget.Reset();
	m_bHasTarget = false;
}

bool CRagebot::IsValidTarget(int i)
{
	return i != INVALID_PLAYER && IsValidTarget(Interfaces::ClientEntList->GetBaseEntity(i));
}
bool CRagebot::IsValidTarget(CBaseEntity* Player)
{
	if (!LocalPlayer.Entity)
		return false;

	return Player && Player->IsPlayer() && !Player->GetImmune() && Player->IsEnemy(LocalPlayer.Entity);
}
bool CRagebot::IsValidTarget(CPlayerrecord* Player)
{
	return IsValidTarget(Player->m_pEntity);
}

void CRagebot::SetLowestFovEntity()
{
	START_PROFILING
	m_LowestFOVTarget.Reset();

	for (int i = 1; i <= MAX_PLAYERS; i++)
	{
		// get player record
		CPlayerrecord* _playerRecord = &m_PlayerRecords[i];

		// invalid playerrecord
		if (!_playerRecord)
			continue;

		auto _Entity = _playerRecord->m_pEntity;

		if (!Interfaces::ClientEntList->EntityExists(_Entity))
			continue;

		bool _IsValidFarESPTarget = _Entity && _playerRecord->IsValidFarESPAimbotTarget();

		// invalid/dead entity
		if (!_Entity || (!_Entity->GetAlive() && !_IsValidFarESPTarget))
			continue;

		// not a valid target
		if (!_IsValidFarESPTarget && !_playerRecord->IsValidTarget())
			continue;

		if (!_Entity->GetAlive())
			continue;

		Target_s _Target = Target_s();
		_Target.m_iEntIndex = _Entity->index;
		_Target.m_flFOV = GetFov(LocalPlayer.ShootPosition, _Entity->GetEyePosition(), LocalPlayer.CurrentEyeAngles);

		if (_Target.m_flFOV < m_LowestFOVTarget.m_flFOV || m_LowestFOVTarget.m_iEntIndex == INVALID_PLAYER)
		{
			Vector EyePos = _Entity->GetLocalOriginDirect() - LocalPlayer.Entity->GetLocalOriginDirect();
			VectorAngles(EyePos, _Target.m_AtTargetAngle);

			m_LowestFOVTarget = _Target;
		}
	}

	END_PROFILING
}

void CRagebot::GetTargettableEntityListUnsorted(std::vector<Target_s>&list, Target_s& lowestfovtarget)
{
	START_PROFILING
	list.clear();
	lowestfovtarget.Reset();

	// loop through all streamed entities
	for (int i = 1; i <= MAX_PLAYERS; i++)
	{
		// get player record
		CPlayerrecord* _playerRecord = &m_PlayerRecords[i];

		// get entity
		auto _Entity = _playerRecord->m_pEntity;

		if (!Interfaces::ClientEntList->EntityExists(_Entity))
			continue;

		bool _IsValidFarESPTarget = _Entity && _playerRecord->IsValidFarESPAimbotTarget();

		// invalid/dead entity
		if (!_Entity || (!_Entity->GetAlive() && !_IsValidFarESPTarget))
			continue;

		// not a valid target
		if (!_IsValidFarESPTarget && !_playerRecord->IsValidTarget())
			continue;

		if (!_Entity->GetAlive())
			continue;

		Target_s _Target;
		_Target.m_Weight = 0;
		_Target.m_iEntIndex = _Entity->index;
		_Target.m_flFOV = GetFov(LocalPlayer.ShootPosition, _Entity->GetEyePosition(), LocalPlayer.CurrentEyeAngles);

		Vector EyePos = _Entity->GetLocalOriginDirect() - LocalPlayer.Current_Origin;
		VectorAngles(EyePos, _Target.m_AtTargetAngle);

		// todo: nit; somehow incorporate v4's sorting into this. not in beta though, got no time atm.
		// until then, we'll just use the best record if available or best fov
		auto _record = _playerRecord->GetBestRecord(false);
		if (_record)
			_Target.m_Weight = _record->GetWeight();

		if (_Target.m_flFOV < lowestfovtarget.m_flFOV || lowestfovtarget.m_iEntIndex == INVALID_PLAYER)
		{
			lowestfovtarget = _Target;
		}

		// add target
		list.emplace_back(_Target);
	}
	END_PROFILING
}

void CRagebot::SortTargettableEntityList(std::vector<Target_s>&list)
{
	START_PROFILING

	// check if all weights are the same, if they are, resort to using FOV instead to prevent aimbots from not shooting at proper targets
	if (list.size() > 1)
	{
		auto target_to_compare = list[0].m_Weight;
		if (std::all_of(list.begin(), list.end(), [target_to_compare](Target_s& a) { return a.m_Weight == target_to_compare; }))
		{
			std::sort(list.begin(), list.end(), [](Target_s& lhs, Target_s& rhs) { return lhs.m_flFOV < rhs.m_flFOV; });
		}
		else
		{
			std::sort(list.begin(), list.end(), [](Target_s& lhs, Target_s& rhs) { return lhs.m_Weight > rhs.m_Weight; });
		}
	}
	END_PROFILING
}

void CRagebot::TargetEntities()
{
	Target_s _lowestFOV;
	bool _gotFreshList = false;
	if (m_flLastSortTime != Interfaces::Globals->curtime)
	{
		//First, get a fresh list of players
		std::vector<Target_s> _freshList;
		GetTargettableEntityListUnsorted(_freshList, _lowestFOV);
		SortTargettableEntityList(_freshList);
		m_flLastSortTime = Interfaces::Globals->curtime;

		//If there are none available, just clear our lists and exit
		if (_freshList.empty())
		{
			ClearAimbotTarget();
			ClearPossibleTargets();
			ClearIgnorePlayerIndex();
			return;
		}

		for (auto i = 0; i < m_PossibleTargets.size(); ++i)
		{
			Target_s &old = m_PossibleTargets[i];
			if (std::find(_freshList.begin(), _freshList.end(), old) == _freshList.end())
			{
				//no longer a valid target
				if (m_LowestFOVTarget.m_iEntIndex == old.m_iEntIndex)
					m_LowestFOVTarget = _lowestFOV;
				//erase this target from the list of scannable targets
				m_PossibleTargets.erase(m_PossibleTargets.begin() + i);
				if (m_iPossibleTargetIndex > i)
					m_iPossibleTargetIndex = max(0, m_iPossibleTargetIndex - 1); //decrease the index since we removed an entity
			}
		}

		if (m_PossibleTargets.empty())
		{
			m_PossibleTargets = _freshList;
			m_LowestFOVTarget = _lowestFOV;
			m_iPossibleTargetIndex = 0;

			if (m_PossibleTargets.empty())
			{
				ClearAimbotTarget();
				ClearPossibleTargets();
				ClearIgnorePlayerIndex();
				return; //still empty
			}
		}

		_gotFreshList = true;
	}
	else
	{
		if (m_PossibleTargets.empty())
		{
			ClearAimbotTarget();
			ClearPossibleTargets();
			ClearIgnorePlayerIndex();
			return;
		}
	}

	int MaxTargetsPerTick = variable::get().ragebot.i_targets_per_tick;
	if (m_iIgnorePlayerIndex != -1) //we are excluding a person that we already scanned this tick
		++MaxTargetsPerTick;

	int _maxTargets = min(m_iPossibleTargetIndex + MaxTargetsPerTick, (int)m_PossibleTargets.size());
	//unsigned char _numChecked = 0;
	//unsigned char _CheckedEntIndexes[MAX_PLAYERS + 1]{ 0 };
	//bool _foundTarget = false;

	for (; m_iPossibleTargetIndex < _maxTargets; ++m_iPossibleTargetIndex)
	{
		auto target = &m_PossibleTargets[m_iPossibleTargetIndex];
		if (target->m_iEntIndex != m_iIgnorePlayerIndex)
		{
			//++_numChecked;
			//_CheckedEntIndexes[target->m_iEntIndex] = 1;

			if (TargetEntity(target))
			{
			//	_foundTarget = true;
				break;
			}
		}
		else
		{
			//if (target->m_iEntIndex <= MAX_PLAYERS)
			//{
			//	++_numChecked;
			//	_CheckedEntIndexes[target->m_iEntIndex] = 1;
			//}
		}
	}

	if (m_iPossibleTargetIndex >= (int)m_PossibleTargets.size())
	{
		// scanned all targets. note: there is a flaw here
		// if we have for example 2 targets per tick and only 1 was valid anymore this tick, we will have only scanned 1 person instead of 2
		// todo: come up with a better way than what we used to to deal with this case that doesn't break the whole ragebot
		ClearPossibleTargets();
		ClearIgnorePlayerIndex();
		if (_gotFreshList)
			m_LowestFOVTarget = _lowestFOV;
	}
}

bool CRagebot::TargetEntity(Target_s* Target)
{
	CBaseEntity* _Entity = Interfaces::ClientEntList->GetBaseEntity(Target->m_iEntIndex);

	if (!_Entity)
	{
		m_LastTarget.Reset();
		return false;
	}

	// net player record for entity
	CPlayerrecord* _playerRecord = g_LagCompensation.GetPlayerrecord(_Entity);

	// no playerrecord or spectating or no target tick
	if (!_playerRecord || !_playerRecord->m_pEntity || (_playerRecord->m_bSpectating && !_playerRecord->IsValidFarESPAimbotTarget()) || !_playerRecord->m_TargetRecord)
	{
		m_LastTarget.Reset();
		return false;
	}

	if (g_AutoBone.Run_AutoBone(_Entity))
	{
		m_LastTarget = *Target;
		m_LastTarget.m_pBestHitbox = g_AutoBone.m_pBestHitbox;
		//m_iLastChecked = 0;
		return true;
	}

	m_LastTarget.Reset();
	return false;
}

bool CRagebot::Finalize()
{
	m_iIgnorePlayerIndex = -1;

	if (m_LastTarget.m_iEntIndex == INVALID_PLAYER || g_AutoBone.m_pBestHitbox == nullptr)
		return false;

	CPlayerrecord* _playerRecord = &m_PlayerRecords[m_LastTarget.m_iEntIndex];
	WeaponInfo_t* wpn_data		 = LocalPlayer.CurrentWeapon->GetCSWpnData();

	if (!wpn_data || !_playerRecord || !_playerRecord->m_pEntity)
	{
		ClearAimbotTarget();
		return false;
	}

	Vector& vecHitboxPos = g_AutoBone.m_pBestHitbox->origin;

#ifndef IMI_MENU
	g_Visuals.m_lastBestAimbotPos = vecHitboxPos;
	g_Visuals.m_lastBestDamage = g_AutoBone.m_pBestHitbox->damage;
	g_Visuals.m_prioritize_body = (g_AutoBone.m_pBestHitbox->actual_hitbox_due_to_penetration != HITBOX_HEAD && g_AutoBone.m_pBestHitbox->actual_hitbox_due_to_penetration != HITBOX_LOWER_NECK);
#endif

#if defined _DEBUG || defined INTERNAL_DEBUG 
	Interfaces::DebugOverlay->AddTextOverlay(Vector(vecHitboxPos.x, vecHitboxPos.y, vecHitboxPos.z - 1.0f), 0.05f, "Aimbot Target");
	Interfaces::DebugOverlay->AddBoxOverlay(vecHitboxPos, Vector(-2, -2, -2), Vector(1, 1, 1), LocalPlayer.CurrentEyeAngles, 255, 0, 0, 255, 0.05f);
#endif

	if (g_WeaponController.WeaponDidFire(CurrentUserCmd.cmd->buttons | IN_ATTACK) != 1 || !variable::get().ragebot.b_autofire)
		return false;
	
	m_angAimAngles = g_AutoBone.m_pBestHitbox->localplayer_eyeangles;
	return true;
}

void CRagebot::Run()
{
	m_angAimAngles = LocalPlayer.FinalEyeAngles;
	m_bTracedEnemy = false;
	g_Info.UsingForwardTrack = false;
	m_iIgnorePlayerIndex = -1;

	if (IsValidTarget(m_LastTarget.m_iEntIndex))
	{
		m_iIgnorePlayerIndex = m_LastTarget.m_iEntIndex;
		CPlayerrecord* _playerRecord = g_LagCompensation.GetPlayerrecord(m_LastTarget.m_iEntIndex);

		if (_playerRecord && _playerRecord->m_pEntity && _playerRecord->m_pEntity->GetAlive() && _playerRecord->m_pEntity->IsEnemy(LocalPlayer.Entity)
			&& !((_playerRecord->m_bSpectating || _playerRecord->m_bDormant) && !_playerRecord->IsValidFarESPAimbotTarget()) && _playerRecord->m_TargetRecord)
		{
			if (TargetEntity(&m_LastTarget))
			{
				SetLowestFovEntity();
				m_bHasTarget = Finalize();
				return;
			}
		}
	}

	ClearAimbotTarget();

	if (m_iPossibleTargetIndex == 0)
	{
		GetTargettableEntityListUnsorted(m_PossibleTargets, m_LowestFOVTarget);
		SortTargettableEntityList(m_PossibleTargets);
		m_flLastSortTime = Interfaces::Globals->realtime;
	}
	else
		SetLowestFovEntity();

	TargetEntities();

	m_bHasTarget = Finalize();
}

extern void FillImpactTempEntResultsAndResolveFromPlayerHurt(CBaseEntity* EntityHit, CPlayerrecord* pCPlayer, CShotrecord* shotrecord, bool CalledFromQueuedEventProcessor);

void CRagebot::OutputMissedShots()
{
	// we want event
	CPlayerrecord *localrecord = &m_PlayerRecords[LocalPlayer.Entity->index];
	LocalPlayer.ShotMutex.Lock();

	// Run all queued shot resolver events
	if (!g_QueuedImpactResolveEvents.empty())
	{
		for (auto& _Impact : g_QueuedImpactResolveEvents)
		{
			FillImpactTempEntResultsAndResolveFromPlayerHurt(_Impact.m_EntityHit, _Impact.pCPlayer, &_Impact.m_ShotRecord, true);
		}
		g_QueuedImpactResolveEvents.clear();
	}
	// don't show missed shots via manual fire
	float shot_time = std::fabsf(LocalPlayer.ManualFireShotInfo.m_flLastManualShotRealTime - Interfaces::Globals->realtime);
	if (shot_time < 1.f)
	{
		LocalPlayer.m_ShotResults.clear();
		LocalPlayer.ShotMutex.Unlock();
		return;
	}

	// detect fake fires
	LocalPlayer.Get(&LocalPlayer);
#if 0 
	// todo: nit; get way to detect predictable/networked clip one from the current weapon, this currently gives an access violation
	if (LocalPlayer.CurrentWeapon != nullptr)
	{
		typedescription_t *td = FindFlatFieldByName(XorStr("m_iClip1"), LocalPlayer.CurrentWeapon->GetPredDescMap());
		if (td != nullptr)
		{
			int m_nServerCommandsAcknowledged = Interfaces::Prediction->m_Split[0].m_nServerCommandsAcknowledged;
			void* slot = LocalPlayer.Entity->GetPredictedFrame(m_nServerCommandsAcknowledged - 1);

			if (slot != nullptr)
			{
				int predicted_clip = *(int*)((byte *)slot + td->flatOffset[TD_OFFSET_PACKED]);
				int networked_clip = LocalPlayer.CurrentWeapon->GetClipOne();

				if (predicted_clip != networked_clip)
				{
#if _DEBUG 
					logger::add(LERROR, XorStr("Cheat int differs (m_Clip1 net %i pred %i) diff(%i)\n"), networked_clip, predicted_clip, predicted_clip - networked_clip);
#endif
					fake_fired = true;
				}
			}
		}
	}
#else
	bool fake_fired = LocalPlayer.FakeFired && sv_infinite_ammo.GetVar()->GetInt() < 1;
#endif

	if(variable::get().visuals.i_shot_logs > 0)
	{
		for (auto& _ShotResultIterator : LocalPlayer.m_ShotResults)
		{
			auto &_ShotResult = _ShotResultIterator.second;

			// miss caused due to fake fire/shot dropped
			if (fake_fired)
			{
				// set it to false again after acknowledging the fake fire
				LocalPlayer.FakeFired = false;
#if defined DEBUG_SHOTS || defined _DEBUG || defined INTERNAL_DEBUG

				g_Eventlog.AddEventElement(0x2, XorStr("[OM] "));
#endif
				g_Eventlog.AddEventElement(0x2, XorStr("missed [shot dropped!] "));
			}

			if (_ShotResult.m_bAcked || !_ShotResult.m_bInflictorIsLocalPlayer)
				continue;

			_ShotResult.m_bAcked = true;

			auto flags = _ShotResult.m_MissFlags;

			// skip over hit and resolved shit (only if we didn't fake fire, otherwise we need to replay the shot 
			if (!fake_fired)
			{
				if (flags & HIT_PLAYER && flags & HIT_IS_RESOLVED)
					continue;
			}

			//decrypts(0)
			std::string victim_name = XorStr("!invalid!");
			//encrypts(0)

			CBaseEntity* _Entity = Interfaces::ClientEntList->EntityExists(_ShotResult.m_pInflictedEntity) ? _ShotResult.m_pInflictedEntity : nullptr;
			if (_Entity)
			{
				victim_name = adr_util::sanitize_name((char*)_Entity->GetName().data());
			}

			int color = (flags & HIT_PLAYER) && (flags & MISS_FROM_BAD_HITGROUP) ? 0x9 : 0x2;

			bool unresolved = _ShotResult.m_ResolveMode != RESOLVE_MODE_NONE && (flags & MISS_FROM_BADRESOLVE || (flags & MISS_SHOULDNT_HAVE_MISSED && !(flags & HIT_PLAYER)));
			std::string resolve_name{};
			std::string hitgroup_name = get_hitgroup_name(_ShotResult.m_iHitgroupShotAt);

			if (!fake_fired)
			{
#if defined DEBUG_SHOTS || defined _DEBUG || defined INTERNAL_DEBUG
				g_Eventlog.AddEventElement(color, "[OM] ");
				g_Eventlog.AddEventElement(color, std::to_string(_ShotResult.m_flTimeCreated));
				g_Eventlog.AddEventElement(color, " | ");
#endif
				//decrypts(0)
				g_Eventlog.AddEventElement(color, XorStr("missed "));
				//encrypts(0)
			}

			// todo: nit; miss due to far esp/dormancy
			if (_ShotResult.m_bAwaitingImpact)
			{
				//decrypts(0)
				g_Eventlog.AddEventElement(color, XorStr("[ missing impact! miss was ignored] "));
				//encrypts(0)
			}
			else
			{
				// invalid miss since MISS_FROM_SPREAD is used as a default flag
				if (flags == MISS_FROM_SPREAD)
				{
					//decrypts(0)
					g_Eventlog.AddEventElement(color, XorStr("[ invalid! ] "));
					//encrypts(0)
				}
				else
				{
					resolve_name = g_Visuals.GetResolveType(_ShotResult);

					if (unresolved)
					{
						//decrypts(0)
						g_Eventlog.AddEventElement(color, XorStr("[ resolver ] "));
						//encrypts(0)
					}
					else
					{
						if (flags & MISS_FROM_BAD_HITGROUP && flags & HIT_PLAYER)
						{
							//decrypts(0)
							g_Eventlog.AddEventElement(color, XorStr("[ wrong hitbox ] "));
							//encrypts(0)
						}

						if (flags & MISS_AUTOWALL)
						{
							//decrypts(0)
							g_Eventlog.AddEventElement(color, XorStr("[ wall interference ] "));
							//encrypts(0)
						}

						// we only want to show if we're missing via spread/inaccuracy if we haven't hit them
						if (!(flags & HIT_PLAYER))
						{
							if (weapon_accuracy_nospread.GetVar()->GetInt() < 1)
							{
								if (flags & MISS_FROM_SPREAD)
								{
									//decrypts(0)
									g_Eventlog.AddEventElement(color, XorStr("[ spread ] "));
									//encrypts(0)
								}
							}
							if (flags & MISS_RAY_OFF_TARGET)
							{
								//decrypts(0)
								g_Eventlog.AddEventElement(color, XorStr("[ inaccuracy ] "));
								//encrypts(0)
							}
						}
					}
				}
			}

			// if we havent hit a reason to miss yet, don't draw out a reason
			if (g_Eventlog.Size() == 1)
			{
				g_Eventlog.ClearEvent();

				LocalPlayer.m_ShotResults.clear();
				LocalPlayer.ShotMutex.Unlock();
				continue;
			}

			int backtrack = 0;
			if (LocalPlayer.ShotRecord != nullptr)
			{
				backtrack = _ShotResult.m_iTickCountWhenWeShot - LocalPlayer.ShotRecord->m_iEnemySimulationTickCount;
			}

			// missed a x-tick backtrack on '[name]''s [hitgroup]
			if (backtrack > 0)
			{
				//decrypts(0)
				g_Eventlog.AddEventElement(color, XorStr("a "));
				g_Eventlog.AddEventElement(color, std::to_string(backtrack));
				g_Eventlog.AddEventElement(color, XorStr("-tick backtrack on \'"));
				g_Eventlog.AddEventElement(color, victim_name);
				//encrypts(0)
			}
			else if (backtrack < 0)
			{
				//decrypts(0)
				g_Eventlog.AddEventElement(color, XorStr("a "));
				g_Eventlog.AddEventElement(color, std::to_string(-backtrack));
				g_Eventlog.AddEventElement(color, XorStr("-tick forwardtrack on \'"));
				g_Eventlog.AddEventElement(color, victim_name);
				//encrypts(0)
			}
			//missed '[name]'
			else
			{
				//decrypts(0)
				g_Eventlog.AddEventElement(color, XorStr("\'"));
				g_Eventlog.AddEventElement(color, victim_name);
				//encrypts(0)
			}
			
			//decrypts(0)
			g_Eventlog.AddEventElement(color, XorStr("\'"));
			//encrypts(0)

			// gear/generic
			if (hitgroup_name[0] != 'G' && !(flags & MISS_FROM_BAD_HITGROUP))
			{
				//decrypts(0)
				g_Eventlog.AddEventElement(color, XorStr(" \'s "));
				//encrypts(0)
				g_Eventlog.AddEventElement(color, hitgroup_name);
			}

			// wrong hitbox clarification
			if (flags & MISS_FROM_BAD_HITGROUP && flags & HIT_PLAYER)
			{
				//decrypts(0)
				g_Eventlog.AddEventElement(color, "[ ");
				g_Eventlog.AddEventElement(color, hitgroup_name);
				g_Eventlog.AddEventElement(color, XorStr(" -> "));
				g_Eventlog.AddEventElement(0x04, get_hitgroup_name(_ShotResult.m_iHitgroupHit));
				g_Eventlog.AddEventElement(color, " ]");

				//encrypts(0)

				if (_ShotResult.m_iDamageGiven > 0)
				{
					//decrypts(0)
					g_Eventlog.AddEventElement(0x4, XorStr(" for "));
					g_Eventlog.AddEventElement(0x4, std::to_string(_ShotResult.m_iDamageGiven));
					g_Eventlog.AddEventElement(0x4, XorStr(" dmg "));
					//encrypts(0)
				}
				else
				{
#if _DEBUG
					logger::add(LERROR, "damage given: %d | miss flags: %s", _ShotResult.m_iDamageGiven, _ShotResult.get_miss_reason(fake_fired).data());
#endif
				}
				
				g_Eventlog.AddEventElement(0x4, "( ");
				g_Eventlog.AddEventElement(0x4, _Entity ? std::to_string(_Entity->GetHealth()) : "?");
				//decrypts(0)
				g_Eventlog.AddEventElement(0x4, XorStr(" remaining )"));
				//encrypts(0)
			}

			// via [resolve name]

#if defined _DEBUG || defined INTERNAL_DEBUG
			if (!resolve_name.empty())
#else
			if(unresolved && !resolve_name.empty())
#endif
			{
				//decrypts(0)
				g_Eventlog.AddEventElement(color, XorStr(" via "));
				//encrypts(0)

				//if (resolve_type.empty())
				//{
				//	DebugBreak();
				//}

				g_Eventlog.AddEventElement(color, resolve_name);
			}

#if defined DEBUG_SHOTS || defined _DEBUG || defined INTERNAL_DEBUG
			g_Eventlog.AddEventElement(0x01, " | ");
			g_Eventlog.AddEventElement(0x01, _ShotResult.get_miss_reason(fake_fired).data());
#endif

			g_Eventlog.OutputEvent(variable::get().visuals.i_shot_logs);
		}
	}

	LocalPlayer.m_ShotResults.clear();
	LocalPlayer.ShotMutex.Unlock();
}

void AirAccelerate(CBaseEntity* player, QAngle &angle, float fmove, float smove) {
	Vector fwd, right, wishvel, wishdir;
	float  maxspeed, wishspd, wishspeed, currentspeed, addspeed, accelspeed;

	// determine movement angles.
	AngleVectors(angle, &fwd, &right, nullptr);

	// zero out z components of movement vectors.
	fwd.z = 0.f;
	right.z = 0.f;

	// normalize remainder of vectors.
	VectorNormalizeFast(fwd);
	VectorNormalizeFast(right);

	// determine x and y parts of velocity.
	for (int i{}; i < 2; ++i)
		wishvel[i] = (fwd[i] * fmove) + (right[i] * smove);

	// zero out z part of velocity.
	wishvel.z = 0.f;

	// determine maginitude of speed of move.
	wishdir = wishvel;
	Vector wishdir2 = wishdir;
	wishspeed = wishdir2.NormalizeInPlace();

	// get maxspeed.
	// TODO; maybe global this or whatever its 260 anyway always.
	maxspeed = player->GetMaxSpeed();

	// clamp to server defined max speed.
	if (wishspeed != 0.f && wishspeed > maxspeed)
		wishspeed = maxspeed;

	// make copy to preserve original variable.
	wishspd = wishspeed;

	// cap speed.
	if (wishspd > 30.f)
		wishspd = 30.f;

	// determine veer amount.
	currentspeed = player->GetVelocity().Dot(wishdir);

	// see how much to add.
	addspeed = wishspd - currentspeed;

	// if not adding any, done.
	if (addspeed <= 0.f)
		return;

	// Determine acceleration speed after acceleration
	accelspeed = Interfaces::Cvar->FindVar("sv_airaccelerate")->GetFloat() * wishspeed * TICK_INTERVAL;

	// cap it.
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	// add accel.
	player->SetVelocity(player->GetVelocity() + (wishdir * accelspeed));
}

void PlayerMove(CBaseEntity* player) {
	Vector                start, end, normal;
	CGameTrace            trace;
	CTraceFilterWorldOnly filter;

	// define trace start.
	start = player->GetNetworkOrigin();

	Vector pVelocity = player->GetVelocity();

	// move trace end one tick into the future using predicted velocity.
	end = start + (pVelocity * TICK_INTERVAL);

	Ray_t ray;
	ray.Init(start, end, player->GetMins(), player->GetMaxs());
	// trace.
	Interfaces::EngineTrace->TraceRay(ray, CONTENTS_SOLID, &filter, &trace);

	// we hit shit
	// we need to fix hit.
	if (trace.fraction != 1.f) {

		// fix sliding on planes.
		for (int i{}; i < 2; ++i) {
			pVelocity -= trace.plane.normal * pVelocity.Dot(trace.plane.normal);

			float adjust = pVelocity.Dot(trace.plane.normal);
			if (adjust < 0.f)
				pVelocity -= (trace.plane.normal * adjust);

			start = trace.endpos;
			end = start + (pVelocity * (TICK_INTERVAL * (1.f - trace.fraction)));

			Ray_t ray2;
			ray2.Init(start, end, player->GetMins(), player->GetMaxs());
			Interfaces::EngineTrace->TraceRay(ray2, CONTENTS_SOLID, &filter, &trace);
			if (trace.fraction == 1.f)
				break;
		}
	}

	// set new final origin.
	start = end = trace.endpos;
	player->SetNetworkOrigin(trace.endpos);

	// move endpos 2 units down.
	// this way we can check if we are in/on the ground.
	end.z -= 2.f;

	// trace.
	Ray_t ray3;
	ray3.Init(start, end, player->GetMins(), player->GetMaxs());
	Interfaces::EngineTrace->TraceRay(ray3, CONTENTS_SOLID, &filter, &trace);

	// strip onground flag.
	player->RemoveFlag(FL_ONGROUND);

	// add back onground flag if we are onground.
	if (trace.fraction != 1.f && trace.plane.normal.z > 0.7f)
		player->AddFlag(FL_ONGROUND);

	player->SetVelocity(pVelocity);
}

CBaseEntity* CRagebot::ForwardTrack(CPlayerrecord* pCPlayer, bool ForceForwardTrack, bool* AlreadyForwardTracked)
{
	auto& var = variable::get();

	if (!var.ragebot.b_forwardtrack.get() || pCPlayer->m_bIsUsingFarESP || pCPlayer->m_bIsUsingServerSide)
		return nullptr;

	if (*AlreadyForwardTracked)
	{
		return nullptr;
	}

	*AlreadyForwardTracked = true;

	if (ForceForwardTrack || pCPlayer->m_bTeleporting || (pCPlayer->m_ValidRecordCount == 0 && pCPlayer->m_flTotalInvalidRecordTime > 0.5f))
	{
		//if (/*!g_Convars.Ragebot.ragebot_prediction->GetBool() ||*/ pCPlayer->m_bTickbaseShiftedBackwards_Rendering)
		//	return nullptr;	

#ifdef _DEBUG
#if 0
	//prevent drawing tons of hitboxes
		static float nagger = FLT_MIN;
		if (QPCTime() - nagger < 0.5f)
			return nullptr;
		nagger = QPCTime();
#endif
#endif
		std::deque<CTickrecord*> m_PredictedTicks;
		std::deque<CTickrecord*> m_NewTicks;
		CBaseEntity* pEntity = pCPlayer->m_pEntity;
		INetChannelInfo* nci = Interfaces::EngineClient->GetNetChannelInfo();

		//First restore to current record in case the player has been placed somewhere else. This ensures we have a pristine animation state and position for predicting future ticks
		CTickrecord* CurrentRecord = pCPlayer->GetCurrentRecord();
		CTickrecord* PreviousRecord = pCPlayer->GetPreviousRecord();
		CTickrecord* PrePreviousRecord = pCPlayer->GetPrePreviousRecord();

		if (!CurrentRecord)
		{
			return nullptr;
		}

		if (!PreviousRecord || PreviousRecord->m_Dormant)
		{
			return nullptr;
		}

		pCPlayer->CM_BacktrackPlayer(CurrentRecord);

		//If standing still and not changing duck stance, just return
		if (pCPlayer->m_ValidRecordCount > 0 &&
			(CurrentRecord->m_AbsVelocity.Length() < MINIMUM_PLAYERSPEED_CONSIDERED_MOVING
			&& (CurrentRecord->m_DuckAmount == 0.0f
				|| CurrentRecord->m_DuckAmount == 1.0f)))
		{
				return nullptr;
		}

		bool ducking = pCPlayer->m_bIsHoldingDuck;
		if (!ducking)
		{
			CTickrecord* PreviousRecord = pCPlayer->m_Tickrecords.size() > 1 ? pCPlayer->m_Tickrecords[1] : nullptr;
			if (PreviousRecord) //check for fakeduck
				if (CurrentRecord->m_DuckAmount > 0.0f && CurrentRecord->m_DuckAmount == PreviousRecord->m_DuckAmount)
					ducking = true;
		}

		int forwardtrackticks = TIME_TO_TICKS(g_ClientState->m_pNetChannel->GetLatency(FLOW_OUTGOING));
		int arrivaltick = g_ClientState->m_ClockDriftMgr.m_nServerTick + 1 + forwardtrackticks;
		bool jumping = !(pEntity->GetFlags() & FL_ONGROUND) || !(PreviousRecord->m_Flags & FL_ONGROUND);
		//int simulationtick = TIME_TO_TICKS(CurrentRecord->m_flFirstCmdTickbaseTime) + CurrentRecord->m_iTicksChoked;
		int simtick = TIME_TO_TICKS(CurrentRecord->m_SimulationTime);

		//prioritize choked ticks that aren't 0
		bool found_choked_count = false;
		int average_choked = 0;
		int total_choked_count = 0;
		for (int i = 0; i < min(pCPlayer->m_Tickrecords.size(), 10); ++i)
		{
			auto& tick = pCPlayer->m_Tickrecords[i];
			if (!tick->m_bIsDuplicateTickbase && tick->m_iTicksChoked > 0)
			{
				average_choked += tick->m_iTicksChoked;
				++total_choked_count;
			}
		}
		if (total_choked_count)
		{
			average_choked /= total_choked_count;
		}
		else
		{
			average_choked = CurrentRecord->m_iTicksChoked;
		}
		int lerpticks = TIME_TO_TICKS(g_LagCompensation.GetLerpTime());
		int deltaticks = average_choked + 1;
		int tickbase = CurrentRecord->m_iTickcount;
		int cmd_tickcount = tickbase + lerpticks;
		int desired_cmd_tickcount = tickbase + lerpticks;
		int max_future_tick;// g_ClientState->m_ClockDriftMgr.m_nServerTick + 1 + TIME_TO_TICKS(g_ClientState->m_pNetChannel->GetLatency(FLOW_OUTGOING)) + sv_max_usercmd_future_ticks.GetVar()->GetInt();
		//hard clamp it since latency causes us to fake fire a lot
		//max_future_tick -= 10;
		max_future_tick = Interfaces::Globals->tickcount + sv_max_usercmd_future_ticks.GetVar()->GetInt() + 1;//max(max_future_tick, g_ClientState->m_ClockDriftMgr.m_nServerTick + 1);
		// get the delta in ticks between the last server net update
		// and the net update on which we created this record.
		int updatedelta = g_ClientState->m_ClockDriftMgr.m_nServerTick - CurrentRecord->m_iServerTick;

		PlayerBackup_t backup(pEntity);

		// if the lag delta that is remaining is less than the current netlag
		// that means that we can shoot now and when our shot will get processed
		// the origin will still be valid, therefore we do not have to predict.
		if (forwardtrackticks <= deltaticks - updatedelta)
		{
			// don't allow fake fires
			if (g_LagCompensation.IsTickValid(CurrentRecord))
			{
				pCPlayer->m_bCountResolverStatsEvenWhenForwardtracking = true;
				goto ShootAnyway;
			}
	
			return nullptr;
		}

		// the next update will come in, wait for it.
		//FIXME: don't do this when forwardtracking
		int next = CurrentRecord->m_iServerTick + 1;
		if (next + deltaticks >= arrivaltick)
			return nullptr;

		desired_cmd_tickcount = desired_cmd_tickcount + deltaticks;
		cmd_tickcount = Interfaces::Globals->tickcount + 1 + lerpticks;//desired_cmd_tickcount;

		// don't allow fake fires
		if (desired_cmd_tickcount > max_future_tick)
			cmd_tickcount = max_future_tick;

		// if the enemy is moving and on ground, check around them to see if we can hit there, so we don't simulate and scan tons of crap
		if (CurrentRecord->m_Flags & FL_ONGROUND && CurrentRecord->m_Velocity.Length() >= MINIMUM_PLAYERSPEED_CONSIDERED_MOVING)
		{
			Autowall_Output_t output;
			Vector testposition = pEntity->GetAbsOriginDirect() + Vector(0.0f, 0.0f, 64.0f);
			QAngle angletoenemy = CalcAngle(LocalPlayer.ShootPosition, pEntity->GetAbsOriginDirect() + Vector(0.0f, 0.0f, 64.0f));
			Vector vRight;
			AngleVectors(angletoenemy, nullptr, &vRight, nullptr);

#ifdef _DEBUG
			Interfaces::DebugOverlay->AddBoxOverlay(testposition + vRight * 35.0f, Vector(-4, -4, -4), Vector(4, 4, 4), angletoenemy, 0, 0, 255, 255, TICKS_TO_TIME(2));
#endif
			Autowall(LocalPlayer.ShootPosition, testposition + vRight * 35.0f, output, false, true, pEntity, HITBOX_BODY);

			if (!output.entity_hit)
			{
#ifdef _DEBUG
				Interfaces::DebugOverlay->AddBoxOverlay(testposition - vRight * 35.0f, Vector(-4, -4, -4), Vector(4, 4, 4), angletoenemy, 0, 0, 255, 255, TICKS_TO_TIME(2));
#endif
				Autowall(LocalPlayer.ShootPosition, testposition - vRight * 35.0f, output, false, true, pEntity, HITBOX_BODY);
				if (!output.entity_hit)
				{
					return nullptr;
				}
			}
		}


		float change = 0.f, dir = 0.f;

		// get the direction of the current velocity.
		if (CurrentRecord->m_Velocity.y != 0.f || CurrentRecord->m_Velocity.x != 0.f)
			dir = RAD2DEG(atan2(CurrentRecord->m_Velocity.y, CurrentRecord->m_Velocity.x));

		//QAngle angelz;
		//VectorAngles(CurrentRecord->m_Velocity, angelz);

		// we have more than one update
		// we can compute the direction.
		// get the delta time between the 2 most recent records.
		float dt = TICKS_TO_TIME(CurrentRecord->m_iServerTick - PreviousRecord->m_iServerTick);

		// init to 0.
		float prevdir = 0.f;

		// get the direction of the prevoius velocity.
		if (PreviousRecord->m_AbsVelocity.y != 0.f || PreviousRecord->m_AbsVelocity.x != 0.f)
			prevdir = RAD2DEG(std::atan2(PreviousRecord->m_AbsVelocity.y, PreviousRecord->m_AbsVelocity.x));

		// compute the direction change per tick.
		change = (AngleNormalize(dir - prevdir) / dt) * TICK_INTERVAL;

		if (std::abs(change) > 6.f)
			change = 0.f;

		//int nextsendpackettick = simulationtick + deltaticks;

		//C_AnimationLayer layers[MAX_OVERLAYS];
		//C_CSGOPlayerAnimState state;
		//float poseparameters[MAX_CSGO_POSE_PARAMS];
		float choked_yaw = !var.ragebot.b_resolver ? CurrentRecord->m_EyeAngles.y : AngleNormalize(CurrentRecord->m_EyeAngles.y + CPlayerrecord::GetYawFromSide(CurrentRecord->m_iResolveSide));
		bool USE_ONE_TAP_METHOD = true;
		if (USE_ONE_TAP_METHOD)
			choked_yaw = CurrentRecord->m_EyeAngles.y;
		float _desyncAmount = 0.0f;
		float _desyncEyeYawAmount = 0.0f;
		auto* _BodyResolveInfo = pCPlayer->GetBodyHitResolveInfo(CurrentRecord);
		bool _useBodyResolver = false;
		if (_BodyResolveInfo && PrePreviousRecord)
		{
			if (pCPlayer->GetBodyHitDesyncAmount(&_desyncAmount, &_desyncEyeYawAmount, CurrentRecord, PreviousRecord, PrePreviousRecord, _BodyResolveInfo))
			{
				choked_yaw = AngleNormalize(CurrentRecord->m_EyeAngles.y + _desyncEyeYawAmount);
				_useBodyResolver = _desyncAmount != 0.0f;
			}
		}
		float curtime = Interfaces::Globals->curtime;

		float latency = nci->GetAvgLatency(FLOW_INCOMING) + nci->GetAvgLatency(FLOW_OUTGOING);
		float TimeItWillBeWhenPacketReachesServer = TICKS_TO_TIME(Interfaces::Globals->tickcount + TIME_TO_TICKS(latency) + 1);
		float deltaTime = fminf(TimeItWillBeWhenPacketReachesServer - CurrentRecord->m_SimulationTime, 1.0f);
		int deltaTicks = TIME_TO_TICKS(deltaTime);
		int ticks = CurrentRecord->m_iTicksChoked;
		int ChokedTicks = clamp(ticks + 1, 1, 15);
		float desyncamount = 0.0f;

		if (_useBodyResolver)
		{
			desyncamount = _BodyResolveInfo->m_flDesyncDelta;
		}
		else
		{
			switch (CurrentRecord->m_iResolveSide)
			{
			case ResolveSides::NEGATIVE_60:
				desyncamount = -60.0f;
				break;
			case ResolveSides::POSITIVE_60:
				desyncamount = 60.0f;
				break;
			case ResolveSides::NEGATIVE_35:
				desyncamount = -35.0f;
				break;
			case ResolveSides::POSITIVE_35:
				desyncamount = 35.0f;
				break;
			};
		}

		float choked_goalfeetyaw = CurrentRecord->m_EyeAngles.y;
		float final_yaw = CurrentRecord->m_EyeAngles.y;
		float final_pitch = CurrentRecord->m_EyeAngles.x;
		if (CurrentRecord->m_bFiredBullet)
		{
			final_yaw = AngleNormalize(PreviousRecord->m_EyeAngles.y);
			final_pitch = AngleNormalize(PreviousRecord->m_EyeAngles.x);
			choked_goalfeetyaw = AngleNormalizePositive(PreviousRecord->m_EyeAngles.y + desyncamount);
		}
		else
		{
			choked_goalfeetyaw = AngleNormalizePositive(CurrentRecord->m_EyeAngles.y + desyncamount);
		}
		CCSGOPlayerAnimState* animstate = pCPlayer->m_pAnimStateServer[CurrentRecord->m_iResolveSide];
		for (auto& tick : pCPlayer->m_Tickrecords)
		{
			m_PredictedTicks.push_back(tick);
		}

		//polakware
#if 0
		{
			int v20 = deltaTicks - ChokedTicks;
			while (v20 >= 0)
			{
				for (int i = 0; i < ChokedTicks; i++)
				{
					++tickbase;

					dir = AngleNormalize(dir + change);
					float yaw;
					if (i + 1 >= deltaticks)
						yaw = CurrentRecord->m_EyeAngles.y; //sendpacket tick
					else
						yaw = choked_yaw;

					pCPlayer->SimulatePlayer(CurrentRecord, PreviousRecord, CurrentRecord->m_iServerTick + (i + 1), false, true, true, CurrentRecord->m_EyeAngles.x, yaw);
					//EnginePredictPlayer(yaw, CurrentRecord->m_EyeAngles.x, pEntity, CurrentRecord->m_iServerTick + (sim + 1), true, jumping, ducking, true, QAngle(0.0f, dir, 0.0f), pCPlayer);
				}
				v20 -= ChokedTicks;
			}

		}
#endif
		// nitro
		for (;;)
		{
			// can the player shoot within his lag delta.
			/*if( shot && shot >= simulation && shot < simulation + lag ) {

				// if so his new lag will be the time until he shot again.
				lag = shot - simulation;
				math::clamp( lag, 3, 15 );

				// only predict a shot once.
				shot = 0;
			}*/

			// see if by predicting this amount of lag
			// we do not break stuff.
			next += deltaticks;
			if (next >= arrivaltick)
				break;

			CTickrecord *newprevioustickrecord = nullptr;
			bool is_final_tick = false;

			// predict lag.
			for (int sim = 0; sim < deltaticks; ++sim)
			{
				++tickbase;

				// predict movement direction by adding the direction change per tick to the previous direction.
				// make sure to normalize it, in case we go over the -180/180 turning point.
				dir = AngleNormalize(dir + change);

				float yaw;

				if (sim + 1 >= deltaticks)
				{
					yaw = final_yaw; //sendpacket tick
					is_final_tick = true;
				}
				else
				{
					//yaw = choked_yaw;
					yaw = final_yaw;

					// animate the player and all their choked ticks to the desired desync amount
					if (CurrentRecord->m_iResolveSide != ResolveSides::NONE && animstate)
					{
						animstate->m_flGoalFeetYaw = choked_goalfeetyaw;
					}
				}

				pCPlayer->m_pEntity->ToPlayerRecord()->m_bAllowAnimationEvents = true;
				pCPlayer->SimulatePlayer(CurrentRecord, PreviousRecord, tickbase, false, true, true, sim + 1 >= deltaticks, CurrentRecord->m_iResolveSide, CurrentRecord->m_EyeAngles.x, yaw, &dir, &change);
				pCPlayer->m_pEntity->SetPoseParameter(12, clamp(final_pitch, -90.0f, 90.0f));
				pCPlayer->m_pEntity->ToPlayerRecord()->m_bAllowAnimationEvents = false;

				/*
				if (_useBodyResolver && sim == (deltaticks - 2))
				{
					//second to last tick, generate new record
					newprevioustickrecord = new CTickrecord(*CurrentRecord);
					newprevioustickrecord->m_animstate_server = new CCSGOPlayerAnimState(*pCPlayer->m_animstate_server);
					newprevioustickrecord->m_animstate_server_negative35 = new CCSGOPlayerAnimState(*pCPlayer->m_animstate_server_negative35);
					newprevioustickrecord->m_animstate_server_negative60 = new CCSGOPlayerAnimState(*pCPlayer->m_animstate_server_negative60);
					newprevioustickrecord->m_animstate_server_positive35 = new CCSGOPlayerAnimState(*pCPlayer->m_animstate_server_positive35);
					newprevioustickrecord->m_animstate_server_positive60 = new CCSGOPlayerAnimState(*pCPlayer->m_animstate_server_positive60);
					newprevioustickrecord->m_AbsAngles = pEntity->GetAbsAnglesDirect();
					newprevioustickrecord->m_AbsOrigin = pEntity->GetAbsOriginDirect();
					newprevioustickrecord->m_Velocity = pEntity->GetVelocity();
					newprevioustickrecord->m_AbsVelocity = pEntity->GetAbsVelocityDirect();
					newprevioustickrecord->m_NetOrigin = pEntity->GetNetworkOrigin();
					newprevioustickrecord->m_Origin = pEntity->GetLocalOriginDirect();
					newprevioustickrecord->m_bStrafing = pEntity->IsStrafing();
					newprevioustickrecord->m_vecLadderNormal = pEntity->GetVecLadderNormal();
					newprevioustickrecord->m_flLowerBodyYaw = pEntity->GetLowerBodyYaw();
					newprevioustickrecord->m_EyeAngles = *pEntity->EyeAngles();
					pEntity->CopyAnimLayers(newprevioustickrecord->m_AnimLayer);
				}
				*/
				//EnginePredictPlayer(yaw, CurrentRecord->m_EyeAngles.x, pEntity, CurrentRecord->m_iServerTick + (sim + 1), true, jumping, ducking, true, QAngle(0.0f, dir, 0.0f), pCPlayer);
			}

			CTickrecord *newtick = new CTickrecord(pEntity);
			newtick->m_PlayerBackup.Get(pEntity);
			m_PredictedTicks.push_front(newtick);
			m_PredictedTicks[0]->m_SimulationTime = TICKS_TO_TIME(tickbase);
			m_NewTicks.push_front(newtick);

			/*
			if (_useBodyResolver)
			{
				CTickrecord *newcurrenttickrecord = new CTickrecord(*CurrentRecord);
				newcurrenttickrecord->m_animstate_server = pCPlayer->m_animstate_server;
				newcurrenttickrecord->m_animstate_server_negative35 = pCPlayer->m_animstate_server_negative35;
				newcurrenttickrecord->m_animstate_server_negative60 = pCPlayer->m_animstate_server_negative60;
				newcurrenttickrecord->m_animstate_server_positive35 = pCPlayer->m_animstate_server_positive35;
				newcurrenttickrecord->m_animstate_server_positive60 = pCPlayer->m_animstate_server_positive60;
				newcurrenttickrecord->m_AbsAngles = pEntity->GetAbsAnglesDirect();
				newcurrenttickrecord->m_AbsOrigin = pEntity->GetAbsOriginDirect();
				newcurrenttickrecord->m_Velocity = pEntity->GetVelocity();
				newcurrenttickrecord->m_AbsVelocity = pEntity->GetAbsVelocityDirect();
				newcurrenttickrecord->m_NetOrigin = pEntity->GetNetworkOrigin();
				newcurrenttickrecord->m_Origin = pEntity->GetLocalOriginDirect();
				newcurrenttickrecord->m_bStrafing = pEntity->IsStrafing();
				newcurrenttickrecord->m_vecLadderNormal = pEntity->GetVecLadderNormal();
				newcurrenttickrecord->m_flLowerBodyYaw = pEntity->GetLowerBodyYaw(); // d
				newcurrenttickrecord->m_EyeAngles = *pEntity->EyeAngles();
				pEntity->CopyAnimLayers(newcurrenttickrecord->m_AnimLayer);

				pCPlayer->ApplyBodyHitResolver(_BodyResolveInfo, desync_amount, pCPlayer->m_animstate_server_bodyhit, newcurrenttickrecord, newprevioustickrecord ? newprevioustickrecord : PreviousRecord);

				if (newprevioustickrecord)
					delete newprevioustickrecord;

				newcurrenttickrecord->m_animstate_server = nullptr;
				newcurrenttickrecord->m_animstate_server_negative35 = nullptr;
				newcurrenttickrecord->m_animstate_server_negative60 = nullptr;
				newcurrenttickrecord->m_animstate_server_positive35 = nullptr;
				newcurrenttickrecord->m_animstate_server_positive60 = nullptr;
				delete newcurrenttickrecord;
			}
			*/
		}

		//server lag compensation code
		if (!pCPlayer->m_bTeleporting)
		{
			Vector org, mins, maxs;
			QAngle ang;
			CTickrecord* prevRecord = nullptr;
			CTickrecord* record = nullptr;
			float flTargetTime = TICKS_TO_TIME(cmd_tickcount) - g_LagCompensation.GetLerpTime();

			// Walk context looking for any invalidating event
			for (auto& tick: m_PredictedTicks)
			{
				// remember last record
				prevRecord = record;

				// get next record
				record = tick;

				if (record->m_Health <= 0)
				{
					// entity must be alive, lost track
					break;
				}

				//Vector delta = record->m_vecOrigin - prevOrg;
				//if (delta.LengthSqr() > LAG_COMPENSATION_TELEPORTED_DISTANCE_SQR)
				//{
					// lost track, too much difference
				//	return false;
				//}

				// did we find a context smaller than target time ?
				if (record->m_SimulationTime <= flTargetTime)
					break; // hurra, stop

				//prevOrg = record->m_vecOrigin;

				// go one step back
				//curr = track->Next(curr);
			}

			if (record && prevRecord)
			{
				//restore the 'client' to that record
				record->m_PlayerBackup.RestoreData(true, false);

				float frac = 0.0f;
				if (prevRecord &&
					(record->m_SimulationTime < flTargetTime) &&
					(record->m_SimulationTime < prevRecord->m_SimulationTime))
				{
					// we didn't find the exact time but have a valid previous record
					// so interpolate between these two records;

					// calc fraction between both records
					frac = (flTargetTime - record->m_SimulationTime) /
						(prevRecord->m_SimulationTime - record->m_SimulationTime);

					Assert(frac > 0 && frac < 1); // should never extrapolate

					ang = Lerp(frac, record->m_AbsAngles, prevRecord->m_AbsAngles);
					org = Lerp(frac, record->m_AbsOrigin, prevRecord->m_AbsOrigin);
					mins = Lerp(frac, record->m_Mins, prevRecord->m_Mins);
					maxs = Lerp(frac, record->m_Maxs, prevRecord->m_Maxs);
				}
				else
				{
					// we found the exact record or no other record to interpolate with
					// just copy these values since they are the best we have
					ang = record->m_AbsAngles;
					org = record->m_AbsOrigin;
					mins = record->m_Mins;
					maxs = record->m_Maxs;
				}

				// See if this represents a change for the entity
				int flags = 0;

				QAngle angdiff = m_PredictedTicks[0]->m_AbsAngles - ang;
				Vector orgdiff = m_PredictedTicks[0]->m_AbsOrigin - org;

#define LAG_COMPENSATION_EPS_SQR ( 0.1f * 0.1f )
				// Allow 4 units of error ( about 1 / 8 bbox width )
#define LAG_COMPENSATION_ERROR_EPS_SQR ( 4.0f * 4.0f )

#define LC_NONE				0
#define LC_ALIVE			(1<<0)

#define LC_ORIGIN_CHANGED	(1<<8)
#define LC_ANGLES_CHANGED	(1<<9)
#define LC_SIZE_CHANGED		(1<<10)
#define LC_ANIMATION_CHANGED (1<<11)

				if (angdiff.LengthSqr() > LAG_COMPENSATION_EPS_SQR)
				{
					flags |= LC_ANGLES_CHANGED;
					pEntity->SetAbsAngles(ang);
				}

				// Use absolute equality here
				if ((mins != m_PredictedTicks[0]->m_Mins/*pEntity->WorldAlignMins()*/) ||
					(maxs != m_PredictedTicks[0]->m_Maxs/*entity->WorldAlignMaxs()*/))
				{
					flags |= LC_SIZE_CHANGED;
					//entity->SetSize(mins, maxs);
					pEntity->SetMins(mins);
					pEntity->SetMaxs(maxs);
				}

				// Note, do origin at end since it causes a relink into the k/d tree
				if (orgdiff.LengthSqr() > LAG_COMPENSATION_EPS_SQR)
				{
					flags |= LC_ORIGIN_CHANGED;
					pEntity->SetAbsOrigin(org);
				}

				////////////////////////
				// Now do all the layers
				//

				for (int layerIndex = 0; layerIndex < pEntity->GetNumAnimOverlays(); ++layerIndex)
				{
					C_AnimationLayer *currentLayer = pEntity->GetAnimOverlay(layerIndex);
					if (currentLayer)
					{
						bool interpolated = false;
						if ((frac > 0.0f))
						{
							auto &recordsLayerRecord = record->m_AnimLayer[layerIndex];
							auto &prevRecordsLayerRecord = prevRecord->m_AnimLayer[layerIndex];
							if ((recordsLayerRecord.m_nOrder == prevRecordsLayerRecord.m_nOrder)
								&& (recordsLayerRecord._m_nSequence == prevRecordsLayerRecord._m_nSequence)
								)
							{
								// We can't interpolate across a sequence or order change
								interpolated = true;
								if (recordsLayerRecord._m_flCycle > prevRecordsLayerRecord._m_flCycle)
								{
									// the older record is higher in frame than the newer, it must have wrapped around from 1 back to 0
									// add one to the newer so it is lerping from .9 to 1.1 instead of .9 to .1, for example.
									float newCycle = Lerp(frac, recordsLayerRecord._m_flCycle, prevRecordsLayerRecord._m_flCycle + 1);
									currentLayer->_m_flCycle = newCycle < 1 ? newCycle : newCycle - 1;// and make sure .9 to 1.2 does not end up 1.05
								}
								else
								{
									currentLayer->_m_flCycle = Lerp(frac, recordsLayerRecord._m_flCycle, prevRecordsLayerRecord._m_flCycle);
								}
								//currentLayer->m_nOrder = recordsLayerRecord.m_order;
								//currentLayer->m_nSequence = recordsLayerRecord.m_sequence;
								currentLayer->m_flWeight = Lerp(frac, recordsLayerRecord.m_flWeight, prevRecordsLayerRecord.m_flWeight);
							}
						}
						if (!interpolated)
						{
							//Either no interp, or interp failed.  Just use record.
							//currentLayer->_m_flCycle = record->m_layerRecords[layerIndex].m_cycle;
							//currentLayer->m_nOrder = record->m_layerRecords[layerIndex].m_order;
							//currentLayer->m_nSequence = record->m_layerRecords[layerIndex].m_sequence;
							//currentLayer->m_flWeight = record->m_layerRecords[layerIndex].m_weight;
						}
					}
				}

				//now do pose parameters
				for (int a = 0; a < MAX_CSGO_POSE_PARAMS; ++a)
				{
					pEntity->SetPoseParameterScaled(a, record->m_flPoseParams[a]);
				}
			}
		}

		for (auto tick = m_NewTicks.begin(); tick != m_NewTicks.end();)
		{
			delete *tick;
			tick = m_NewTicks.erase(tick);
		}

	ShootAnyway:

		pEntity->InvalidateBoneCache();
		pEntity->SetLastOcclusionCheckFlags(0);
		pEntity->SetLastOcclusionCheckFrameCount(Interfaces::Globals->framecount);
		pEntity->ReevaluateAnimLOD();
		pEntity->SetupBones_Server(BONE_USED_BY_HITBOX, Interfaces::Globals->curtime);

		CTickrecord* backup_record = pCPlayer->m_TargetRecord;
		pCPlayer->m_TargetRecord = nullptr;
		auto accessor = pEntity->GetBoneAccessor();

		struct HitboxTest
		{
			int hitbox;
			Vector bonepos;
			CBaseEntity* entityhit;
			Vector hitpos;
			int hitgrouphit;
			int hitboxhit;
			float dmghit;
			bool penetrated = false;
			HitboxTest(int box) { hitbox = box; }
		};

		std::vector<HitboxTest> hitboxes;
		if (var.ragebot.hitscan_head.b_enabled)
		{
			hitboxes.push_back(HITBOX_HEAD);
			//hitboxes.push_back(HITBOX_LOWER_NECK);
		}
		if (var.ragebot.hitscan_chest.b_enabled)
		{
			//hitboxes.push_back(HITBOX_THORAX);
			hitboxes.push_back(HITBOX_CHEST);
			//hitboxes.push_back(HITBOX_UPPER_CHEST);
		}
		if (var.ragebot.hitscan_stomach.b_enabled)
		{
			hitboxes.push_back(HITBOX_PELVIS);
			//hitboxes.push_back(HITBOX_BODY);
		}
		if (var.ragebot.hitscan_legs.b_enabled)
		{
			hitboxes.push_back(HITBOX_LEFT_THIGH);
			hitboxes.push_back(HITBOX_RIGHT_THIGH);
			//hitboxes.push_back(HITBOX_LEFT_CALF);
			//hitboxes.push_back(HITBOX_RIGHT_CALF);
		}
		if (var.ragebot.hitscan_feet.b_enabled)
		{
			hitboxes.push_back(HITBOX_LEFT_FOOT);
			hitboxes.push_back(HITBOX_RIGHT_FOOT);
		}
		if (var.ragebot.hitscan_arms.b_enabled)
		{
			//hitboxes.push_back(HITBOX_LEFT_HAND);
			//hitboxes.push_back(HITBOX_RIGHT_HAND);
			//hitboxes.push_back(HITBOX_LEFT_UPPER_ARM);
			hitboxes.push_back(HITBOX_LEFT_FOREARM);
			//hitboxes.push_back(HITBOX_RIGHT_UPPER_ARM);
			hitboxes.push_back(HITBOX_RIGHT_FOREARM);
		}


		HitboxTest* besthitbox = nullptr;
		int i = 0;
		for (; i < hitboxes.size(); ++i)
		{
			HitboxTest* box = &hitboxes[i];
			Autowall_Output_t output;
			bool scan_through_teammates = var.ragebot.b_scan_through_teammates;
			box->bonepos = pEntity->GetBonePositionCachedOnly(box->hitbox, accessor->GetBoneArrayForWrite());
			box->entityhit = Autowall(LocalPlayer.ShootPosition, box->bonepos, output, !scan_through_teammates, false, pEntity);
			box->penetrated = output.penetrated_wall;
			box->dmghit = output.damage_dealt;
			box->hitboxhit = output.hitbox_hit;
			box->hitgrouphit = output.hitgroup_hit;
			box->hitpos = output.position_hit;

			// todo: nit; handle baim logic here too, but fuck that rn
			if (box->entityhit == pEntity)
			{
				float mindmg = box->penetrated ? static_cast<float>(var.ragebot.i_mindmg_aw) : static_cast<float>(var.ragebot.i_mindmg);
				if (box->dmghit >= mindmg)
				{
					if (!besthitbox || box->dmghit > besthitbox->dmghit)
						besthitbox = box;
				}
			}

#if 0
			if (box->entityhit == pEntity)
				Interfaces::DebugOverlay->AddTextOverlayRGB(box->bonepos, 0, 10.f, 0, 255, 0, 255, "%2.1f", box->dmghit);
			else
				Interfaces::DebugOverlay->AddTextOverlayRGB(box->bonepos, 0, 10.f, 255, 255, 255, 255, "%2.1f", box->dmghit);
#endif
		}

		if (besthitbox)
		{
			bool _hitchanced = false;
			float calcedHitchance = 0.f;

			QAngle destAngle = CalcAngle(LocalPlayer.ShootPosition, besthitbox->bonepos);
			g_Removals.DoNoRecoil(destAngle);

			// run hitchance
			if (weapon_accuracy_nospread.GetVar()->GetInt() < 1)
			{
				if (LocalPlayer.WeaponWillFireBurstShotThisTick || LocalPlayer.WeaponVars.IsBurstableWeapon && LocalPlayer.CurrentWeapon->GetBurstShotsRemaining() > 0)
					_hitchanced = true;
				else
					_hitchanced = Hitchance(pEntity, destAngle, var.ragebot.f_hitchance, HITGROUP_GENERIC, &calcedHitchance);
			}
			else
			{
				_hitchanced = true;
			}

#if 0
			if (_hitchanced)
				Interfaces::DebugOverlay->AddTextOverlayRGB(besthitbox->bonepos, 0, 10.f, 0, 255, 0, 255, "FWD T");
			else
				Interfaces::DebugOverlay->AddTextOverlayRGB(besthitbox->bonepos, 0, 10.f, 255, 0, 0, 255, "FWD T");
#endif
			if (_hitchanced)
			{
				// save final hitbox
				auto& SavedHitbox = CurrentRecord->m_BestHitbox_ForwardTrack;
				SavedHitbox.bsaved = true;
				SavedHitbox.forwardtracked = true;
				SavedHitbox.penetrated = besthitbox->penetrated;
				SavedHitbox.origin = besthitbox->bonepos;
				SavedHitbox.bismultipoint = false;
				SavedHitbox.damage = besthitbox->dmghit;
				SavedHitbox.actual_hitbox_due_to_penetration = besthitbox->hitboxhit;
				SavedHitbox.actual_hitgroup_due_to_penetration = besthitbox->hitgrouphit;
				SavedHitbox.localplayer_origin = LocalPlayer.Entity->GetAbsOriginDirect();
				SavedHitbox.localplayer_shootpos = LocalPlayer.ShootPosition;
				SavedHitbox.hitchance = calcedHitchance;
				SavedHitbox.index = besthitbox->hitbox;
				QAngle EyeAnglesToHitbox, EyeAnglesToSend, ServerFireBulletAngles;
				g_AutoBone.GetAngleToHitboxInfo(besthitbox->bonepos, EyeAnglesToHitbox, EyeAnglesToSend, ServerFireBulletAngles);
				SavedHitbox.localplayer_eyeangles = EyeAnglesToHitbox;
				g_AutoBone.m_pBestHitbox = &SavedHitbox;
				g_AutoBone.m_iBestHitbox = besthitbox->hitbox;

				//restore original values
				pCPlayer->m_TargetRecord = backup_record;
				pCPlayer->m_iTickcount_ForwardTrack = cmd_tickcount - lerpticks;// -1;
#ifdef _DEBUG
				if (g_WeaponController.WeaponDidFire(CurrentUserCmd.cmd->buttons | IN_ATTACK) == 1)
				{
					printf("fwdtrack: %.1f %.1f %.1f\n", pEntity->GetAbsOriginDirect().x, pEntity->GetAbsOriginDirect().y, pEntity->GetAbsOriginDirect().z);
					printf("predicted %i ticks\n", deltaticks);
					pEntity->DrawHitboxesFromCache(ColorRGBA(158, 158, 100, 255), 3.0f, pEntity->GetBoneAccessor()->GetBoneArrayForWrite());
					//pEntity->DrawHitboxesFromCache(ColorRGBA(255, 0, 0, 255), 5.0f, CurrentRecord->m_CachedBoneMatrix);
				}
#endif
				pCPlayer->m_TargetRecord = backup_record;
				backup.RestoreData();
				return pEntity;
			}
		}

		//restore original values
		pCPlayer->m_TargetRecord = backup_record;
		backup.RestoreData();
	}

	return nullptr;
}

#if 0
void CLegitbot_imi::Update()
{
	// find weapon recoil scale convar
	if (!weapon_recoil_scale)
	{
		//decrypts(0)
		weapon_recoil_scale = Interfaces::Cvar->FindVar(XorStr("weapon_recoil_scale"));
		//encrypts(0)
	}

	// reset standalone rcs
	m_bAimbotNeedsRCS = false;

	// reset aimangles
	m_angAimAngles.Init();

	// reset target
	m_CurrentTarget.Reset();

	// set aimtime
	m_flAimTime = g_Convars.Legitbot.legitbot_smoothtime->GetFloat();

	// increase current aimtime
	m_flCurAimTime += Interfaces::Globals->interval_per_tick;
}

bool CLegitbot_imi::IsInHitboxRadius(CBaseEntity* entity, int hitbox, float scale)
{
	// init trace parameters
	CGameTrace trace;
	Ray_t ray;
	QAngle direction = LocalPlayer.CurrentEyeAngles + LocalPlayer.Entity->GetPunch() * (weapon_recoil_scale ? weapon_recoil_scale->GetFloat() : 2.f);
	Vector endPos;

	// get endPos
	AngleVectors(direction, &endPos);
	endPos = endPos * LocalPlayer.CurrentWeapon->GetCSWpnData()->flRange + LocalPlayer.ShootPosition;

	// init ray
	ray.Init(LocalPlayer.ShootPosition, endPos);

	// get playerrecord for hitboxes
	auto _playerRecord = g_LagCompensation.GetPlayerrecord(entity);

	// trace if we have hitboxes
	if (_playerRecord)
		TRACE_HITBOX_SCALE(entity, ray, trace, _playerRecord->CapsuleOBB.m_cSpheres, _playerRecord->CapsuleOBB.m_cOBBs, nullptr, scale);

	// hit what we wanted to hit?
	return (trace.fraction < 1.f && trace.m_pEnt && trace.m_pEnt == entity);
}

void CLegitbot_imi::SortTargettableEntityList()
{
	// clear entities
	m_PossibleTargets.clear();

	// loop through all streamed entities
	for (int i = 0; i < NumStreamedPlayers; i++)
	{
		// get player record
		CPlayerrecord* _playerRecord = m_StreamedPlayers[i];

		// invalid playerrecord
		if (!_playerRecord)
			continue;

		// get entity
		auto _Entity = _playerRecord->m_pInflictedEntity;

		// invalid/dead entity
		if (!_Entity || !_Entity->GetAlive())
			continue;

		// not a valid target
		if (!_playerRecord->IsValidTarget())
			continue;

		auto _HeadPos = _Entity->GetBonePosition(HITBOX_HEAD);

		Target_s _Target	= Target_s();
		_Target.m_iEntIndex = _Entity->index;
		_Target.m_flFOV		= GetFov(LocalPlayer.ShootPosition, _HeadPos, LocalPlayer.CurrentEyeAngles);

		// add target
		m_PossibleTargets.emplace_back(_Target);
	}

	// sort vector
	std::sort(m_PossibleTargets.begin(), m_PossibleTargets.end(), [](Target_s& lhs, Target_s& rhs) { return lhs.m_flFOV < rhs.m_flFOV; }); //Presort Entities
}

bool CLegitbot_imi::TargetEntity(Target_s* Target)
{
	// reset target
	m_CurrentTarget.Reset();

	// get target entity
	CBaseEntity* _Entity = Interfaces::ClientEntList->GetBaseEntity(Target->m_iEntIndex);

	// skip invalid entities
	if (!_Entity)
		return false;

	// get player record
	CPlayerrecord* _playerRecord = g_LagCompensation.GetPlayerrecord(_Entity);

	// skip invalid entities
	if (!_playerRecord || !_playerRecord->IsValidTarget())
		return false;

	// init locals
	Vector _HitboxClosest = Vector(0.f, 0.f, 0.f);
	float _BestFOV		  = -1.f;

	// loop through all hitboxes
	for (int j = HITBOX_HEAD; j < HITBOX_MAX; j++)
	{
		// not hitbox closest and not our hitbox, skip
		if (g_Convars.Legitbot.legitbot_hitbox->GetInt() != -1 && g_Convars.Legitbot.legitbot_hitbox->GetInt() != j)
			continue;

		// get current hitbox pos
		const Vector _CurrentHitbox = _Entity->GetBonePosition(j);

		// hitbox visible
		if (isVisible(_Entity, LocalPlayer.Entity, LocalPlayer.ShootPosition, LocalPlayer.CurrentEyeAngles, _CurrentHitbox) && !(g_Assistance.LineGoesThroughSmoke(LocalPlayer.ShootPosition, _CurrentHitbox) && g_Convars.Legitbot.legitbot_smokecheck->GetBool()))
		{
			// init fov
			float _FOV = 0.f;

			// rcs enabled
#if 0
			if (g_Convars.Legitbot.legitbot_rcs->GetBool())
			{
				// get weapon_recoil_scale
				float _wrs = weapon_recoil_scale ? weapon_recoil_scale->GetFloat() : 2.f;

				// get punch angle
				QAngle _Punch = LocalPlayer.Entity->GetPunch();

				// apply pitch and yaw rcs
				_Punch.x *= _wrs * (g_Convars.Legitbot.legitbot_rcs_y->GetFloat() / 100.f);
				_Punch.y *= _wrs * (g_Convars.Legitbot.legitbot_rcs_x->GetFloat() / 100.f);

				// get fov
				_FOV = GetFov(LocalPlayer.ShootPosition, _CurrentHitbox, LocalPlayer.CurrentEyeAngles + _Punch);
			}
			// rcs disabled, get fov from "norecoil"
			else
#endif
			_FOV = GetFov(LocalPlayer.ShootPosition, _CurrentHitbox, LocalPlayer.CurrentEyeAngles + LocalPlayer.Entity->GetPunch() * (weapon_recoil_scale ? weapon_recoil_scale->GetFloat() : 2.f));

			// in fov
			if (g_Convars.Legitbot.legitbot_fovtype->GetInt() == 0 && _FOV <= g_Convars.Legitbot.legitbot_fovptp->GetFloat() || g_Convars.Legitbot.legitbot_fovtype->GetInt() == 1 && IsInHitboxRadius(_Entity, j, g_Convars.Legitbot.legitbot_fovscale->GetFloat()))
			{
				// smaller than current best
				if (_FOV < _BestFOV || _BestFOV == -1)
				{
					// set new best
					_BestFOV = _FOV;

					// save hitbox pos
					_HitboxClosest = _CurrentHitbox;

					// not hitbox closest and we found our hitbox, break
					if (g_Convars.Legitbot.legitbot_hitbox->GetInt() != -1)
						break;
				}
			}
		}
	}

	// if target changed
	if (m_LastTarget.m_iEntIndex != Target->m_iEntIndex)
	{
		// reset aim time
		m_flCurAimTime = 0.f;

		// reset aim angles
		m_StartAimAngles.Init();

		// we had a previous target but don't want to snap
		if (m_LastTarget.m_iEntIndex > -1 && g_Convars.Legitbot.legitbot_nosnap->GetBool())
			return false;
	}

	// clamp aim time
	m_flCurAimTime = clamp(m_flCurAimTime, 0.f, m_flAimTime);

	// no target
	if (_HitboxClosest.IsZero())
		return false;

	// set aim angles
	if (m_StartAimAngles.IsZero())
		m_StartAimAngles = LocalPlayer.CurrentEyeAngles;

	// set aimbot target
	m_CurrentTarget = *Target;

	if (g_Convars.Legitbot.legitbot_nonsticky->GetBool() && FinishedAim(_Entity))
		return false;

	// get final aim angles
	Vector _delta = _HitboxClosest - LocalPlayer.ShootPosition;
	VectorAngles(_delta, m_angAimAngles);

	// get current punch
	QAngle _punch = LocalPlayer.Entity->GetPunch() * (weapon_recoil_scale ? weapon_recoil_scale->GetFloat() : 2.f);

	// rcs enabled
#if 0
	if (g_Convars.Legitbot.legitbot_rcs->GetBool())
	{
		// get weapon_recoil_scale
		float _wrs = weapon_recoil_scale ? weapon_recoil_scale->GetFloat() : 2.f;

		// get punch angle
		QAngle _Punch = LocalPlayer.Entity->GetPunch();

		// apply pitch and yaw rcs
		_Punch.x *= _wrs * (g_Convars.Legitbot.legitbot_rcs_y->GetFloat() / 100.f);
		_Punch.y *= _wrs * (g_Convars.Legitbot.legitbot_rcs_x->GetFloat() / 100.f);

		// apply rcs to aim angle
		m_angAimAngles -= _Punch;
	}
	// rcs disabled
	else if (LocalPlayer.Entity->GetShotsFired() > 2)
#endif
	m_angAimAngles -= _punch;

	// block standalone rcs
	m_bAimbotNeedsRCS = g_Convars.Legitbot.legitbot_rcs->GetBool();

	// normalize aim angles
	NormalizeAngles(m_angAimAngles);

	// backup last aim angles
	m_LastAimAngles = m_angAimAngles;

	// backup last target
	m_LastTarget = m_CurrentTarget;

	return true;
}
void CLegitbot_imi::TargetEntities()
{
	for (int i = 0; i < m_PossibleTargets.size(); i++)
	{
		if (TargetEntity(&m_PossibleTargets[i]))
			break;
	}
}

bool CLegitbot_imi::FinishedAim(CBaseEntity* _Entity)
{
	// init locals
	Ray_t ray;
	CGameTrace trace;

	// init tracefilter
	CTraceFilterSimple filter;
	filter.SetPassEntity((IHandleEntity*)LocalPlayer.Entity);

	// init trace parameters
	auto weaponRange = LocalPlayer.CurrentWeapon->GetCSWpnData()->flRange;
	auto traceFilter = (ITraceFilter*)&filter;
	QAngle angles = LocalPlayer.CurrentEyeAngles + LocalPlayer.Entity->GetPunch() * (weapon_recoil_scale ? weapon_recoil_scale->GetFloat() : 2.f);
	Vector endPos, direction;

	// get endPos
	AngleVectors(angles, &direction);
	VectorNormalizeFast(direction);
	endPos = LocalPlayer.ShootPosition + direction * weaponRange;

	// init ray
	ray.Init(LocalPlayer.ShootPosition, endPos);

	// run trace
	Interfaces::EngineTrace->TraceRay(ray, MASK_SHOT, traceFilter, &trace);

	// did we hit anything
	return (_Entity == trace.m_pEnt);
}

void CLegitbot_imi::ApplyRCS()
{
	// current punch angles * weapon recoil scale
	const QAngle _currentPunchAngles = LocalPlayer.Entity->GetPunch() * (weapon_recoil_scale ? weapon_recoil_scale->GetFloat() : 2.f);

	// target angles after rcs
	QAngle _targetAngles;

	// punch angle delta
	QAngle _punchDelta = m_LastPunchAngles - _currentPunchAngles;

	// backup last punch angles
	m_LastPunchAngles = _currentPunchAngles;

	// aimbot needs rcs, use target angles without punch delta
	if (m_bAimbotNeedsRCS)
		_targetAngles = m_angAimAngles - _punchDelta;
	// standalone rcs, use eye angles
	else
		_targetAngles = LocalPlayer.CurrentEyeAngles;

	// strip punch delta according to rcs values
	_punchDelta.x *= (g_Convars.Legitbot.legitbot_rcs_y->GetFloat() / 100.f);
	_punchDelta.y *= (g_Convars.Legitbot.legitbot_rcs_x->GetFloat() / 100.f);

	// add punch delta after strenght calc
	_targetAngles += _punchDelta;

	// get angle delta
	QAngle _angleDelta = LocalPlayer.CurrentEyeAngles - _targetAngles;
	NormalizeAngles(_angleDelta);

	// smooth factor
	if (g_Convars.Legitbot.legitbot_smoothtype->GetInt() != 2)
	{
		// smooth if wanted
		if (g_Convars.Legitbot.legitbot_smoothtype->GetInt() == 1)
			_angleDelta /= g_Convars.Legitbot.legitbot_smoothfactor->GetFloat();

		// apply rcs to aimangles
		_targetAngles = LocalPlayer.CurrentEyeAngles - _angleDelta;
	}
	// smooth time
	else if (m_flAimTime > 0.f)
	{
		// apply rcs to aimangles
		_targetAngles = LocalPlayer.CurrentEyeAngles - _angleDelta;

		// smooth relative to aimtime
		SmoothAngle(m_StartAimAngles, _targetAngles, m_flCurAimTime / m_flAimTime);
	}

	// save new aim angles
	if (m_bAimbotNeedsRCS)
		m_angAimAngles = _targetAngles;
	// save new standalone rcs delta
	else if (g_Convars.Legitbot.legitbot_rcs_standalone->GetBool())
		m_RCSDelta = LocalPlayer.CurrentEyeAngles - _targetAngles;
}

bool CLegitbot_imi::Finalize()
{
	// reset RCS Angle
	m_RCSDelta = QAngle(0.f, 0.f, 0.f);

	// init locals
	bool _aimcondition;

	// if on key, check keystate
	if (g_Convars.Legitbot.legitbot_mode->GetInt() == 2)
		_aimcondition = g_Input.IsKeyPressed(g_Convars.Legitbot.legitbot_key->GetInt());
	// else check IsAttacking
	else
		_aimcondition = CurrentUserCmd.IsAttacking() || LocalPlayer.CurrentWeapon->GetItemDefinitionIndex() == WEAPON_REVOLVER && CurrentUserCmd.IsSecondaryAttacking();

	// should we stop to aim
	if (!_aimcondition || !LocalPlayer.CurrentWeapon->GetClipOne())
	{
		// reset target
		if (!_aimcondition)
			m_LastTarget.Reset();

		// reset last rcs delta
		m_RCSDelta.Init();

		// update punch angles no matter what
		m_LastPunchAngles = LocalPlayer.Entity->GetPunch() * (weapon_recoil_scale ? weapon_recoil_scale->GetFloat() : 2.f);

		// reset aim angles
		m_StartAimAngles.Init();

		// abort
		return false;
	}

	// ...
	ApplyRCS();

	// can't shoot
	if (g_WeaponController.WeaponDidFire(CurrentUserCmd.cmd->buttons | IN_ATTACK) != 1)
		return false;

	// no target
	if (m_CurrentTarget.m_iEntIndex == INVALID_PLAYER)
		return false;

	// go go go
	return true;
}

void CLegitbot_imi::Run()
{
	Update();

	if (!TargetEntity(&m_LastTarget))
	{
		ClearAimbotTarget();

		SortTargettableEntityList();

		TargetEntities();
	}

	m_bHasTarget = Finalize();
}
#endif
