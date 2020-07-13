#include "precompiled.h"
#include "LocalPlayer.h"
#include "Interfaces.h"
#include "VMProtectDefs.h"
#include "HitboxDefines.h"
#include "Targetting.h"
#include "INetchannelInfo.h"
#include "SetClanTag.h"
#include "Keys.h"
#include "WeaponController.h"
#include "Aimbot_imi.h"
#include "AntiAim.h"
#include "Animation.h"
#include "ServerSide.h"
#include "TickbaseExploits.h"
#include "Fakelag.h"

#include "UsedConvars.h"
#include "Adriel/stdafx.hpp"
#include "Adriel/input.hpp"

CThreadMutex RENDER_MUTEX;

BackupMatrixStruct::BackupMatrixStruct(float simulationtime, int ticksallowedforprocessing, matrix3x4_t matrix[MAXSTUDIOBONES], Vector& origin, QAngle& angles)
{
	m_SimulationTime = simulationtime;
	m_TicksAllowedForProcessing = ticksallowedforprocessing;
	memcpy(m_Matrix, matrix, sizeof(matrix3x4_t) * MAXSTUDIOBONES);
	m_Origin = origin;
	m_Angles = angles;
}

MyPlayer LocalPlayer;

void MyPlayer::OnInvalid()
{
	m_UserCommandMutex.Lock();
	m_vecUserCommands.clear();
	m_UserCommandMutex.Unlock();
}

void MyPlayer::CopyShootPositionVars(Vector& Dest_EyePosition_Forward, Vector& Dest_HeadPosition_Forward)
{
	Dest_EyePosition_Forward = EyePosition_Forward;
	Dest_HeadPosition_Forward = HeadPosition_Forward;
}

void MyPlayer::WriteShootPositionVars(Vector& Source_EyePosition_Forward, Vector& Source_HeadPosition_Forward)
{
	EyePosition_Forward = Source_EyePosition_Forward;
	HeadPosition_Forward = Source_HeadPosition_Forward;
}

void MyPlayer::FixShootPosition(QAngle &desteyeangles, bool already_animated)
{
	PlayerBackup_t *localplayer = new PlayerBackup_t(Entity);

	if (!already_animated)
	{
		*Entity->EyeAngles() = desteyeangles;
		Entity->UpdateServerSideAnimation(ResolveSides::NONE, Interfaces::Globals->curtime);
	}

	Entity->SetPoseParameter(12, clamp(desteyeangles.x, -90.0f, 90.0f));

	AllowSetupBonesToUpdateAttachments = false;
	Entity->InvalidateBoneCache();
	Entity->SetLastOcclusionCheckFlags(0);
	Entity->SetLastOcclusionCheckFrameCount(Interfaces::Globals->framecount);
	Entity->SetupBones(nullptr, -1, BONE_USED_BY_HITBOX, Interfaces::Globals->curtime);

	EyePosition_Forward = Entity->EyePosition();

	//decrypts(0)
	int bone = Entity->LookupBone((char*)XorStr("head_0"));
	//encrypts(0)
	if (bone != -1)
	{
		QAngle vecHeadAngles;
		Entity->GetBonePositionRebuilt(bone, HeadPosition_Forward, vecHeadAngles);
	}
	else
	{
		HeadPosition_Forward = Entity->WorldSpaceCenter();
	}

	localplayer->RestoreData(true, true);
	delete localplayer;
}

void MyPlayer::BeginEnginePrediction(CUserCmd* cmd, bool backup_player_state, int custom_start_tick)
{
	if (backup_player_state)
		playerstate.Get(Entity); //Save player state before predicting

	if (!Interfaces::ClientEntList->EntityExists(Entity))
		return;

	g_Info.m_PredictionMutex.lock();

	int random_seed = MD5_PseudoRandom(cmd->command_number) & 0x7fffffff;
	cmd->random_seed = random_seed;
	bInPrediction = true;
	bInPrediction_Start = true;

	CBaseEntity** pPredictionPlayer = StaticOffsets.GetOffsetValueByType<CBaseEntity**>(_predictionplayer);
	originalpredictionplayer = *pPredictionPlayer;
#ifdef _DEBUG
	memcpy(&originalplayercommand, Entity->m_PlayerCommand(), sizeof(CUserCmd));
#else
	originalplayercommand = *Entity->m_PlayerCommand();
#endif
	originalrandomseed = *m_pPredictionRandomSeed;
	originalflags = Entity->GetFlags();
	originalframetime = Interfaces::Globals->frametime;
	originalcurtime = Interfaces::Globals->curtime;
	originalcurrentcommand = *Entity->m_pCurrentCommand();
	Vector originalorigin = Entity->GetNetworkOrigin();
	CBaseCombatWeapon *weapon = Entity->GetWeapon();
	if (weapon)
	{
		originalaccuracypenalty = weapon->GetInaccuracy();
		originalrecoilindex = weapon->GetRecoilIndex();
	}

	//Entity->RunPreThink();
	//Entity->RunThink();

	*pPredictionPlayer = Entity;
	Interfaces::Prediction->m_bInPrediction = true;
	(*Interfaces::MoveHelperClient)->SetHost((CBasePlayer*)Entity);
	*m_pPredictionRandomSeed = random_seed;
	Interfaces::Globals->frametime = Interfaces::Prediction->m_bEnginePaused ? 0.0f : Interfaces::Globals->interval_per_tick;
	Interfaces::Globals->curtime = (Entity->GetTickBase() + custom_start_tick) * Interfaces::Globals->interval_per_tick;
	*Entity->m_pCurrentCommand() = cmd;
	*Entity->m_PlayerCommand() = *cmd;

	memset(&NewMoveData, 0, sizeof(CMoveData));

	//#define USE_REBUILT_GAME_MOVEMENT

	#ifdef USE_REBUILT_GAME_MOVEMENT
		g_pGameMovement->StartTrackPredictionErrors(Entity);
		Entity->UpdateButtonState(cmd->buttons);
		Interfaces::Prediction->SetupMove((C_BasePlayer*)Entity, cmd, (*Interfaces::MoveHelperClient), &NewMoveData);
		g_pGameMovement->SetupMovementBounds(&NewMoveData); //Required because SetupMove is setting up the game's g_pGameMovement and not ours
		g_pGameMovement->ProcessMovement(Entity, &NewMoveData);
		Interfaces::Prediction->FinishMove((C_BasePlayer*)Entity, cmd, &NewMoveData);
		if (weapon && !weapon->IsGrenade(0) && !weapon->IsKnife(0))
			weapon->UpdateAccuracyPenalty();
		g_pGameMovement->FinishTrackPredictionErrors(Entity);
		(*Interfaces::MoveHelperClient)->SetHost(nullptr);
	#else

		Interfaces::GameMovement->StartTrackPredictionErrors((CBasePlayer*)Entity);
		Entity->UpdateButtonState(cmd->buttons);
		Interfaces::Prediction->SetupMove((C_BasePlayer*)Entity, cmd, (*Interfaces::MoveHelperClient), &NewMoveData);
		Interfaces::GameMovement->ProcessMovement((CBasePlayer*)Entity, &NewMoveData);
		Interfaces::Prediction->FinishMove((C_BasePlayer*)Entity, cmd, &NewMoveData);
		Vector vel = NewMoveData.m_vecVelocity_;//(Entity->GetNetworkOrigin() - originalorigin) / TICK_INTERVAL;
		Vector origin = Entity->GetNetworkOrigin();
		//vel = Entity->GetVelocity();
		Entity->SetAbsVelocityDirect(vel);
		Entity->SetAbsOriginDirect(origin);
		Entity->SetEFlags(Entity->GetEFlags() & ~(EFL_DIRTY_ABSTRANSFORM | EFL_DIRTY_ABSVELOCITY));
		//(*Interfaces::MoveHelperClient)->ProcessImpacts();
		Interfaces::GameMovement->FinishTrackPredictionErrors((CBasePlayer*)Entity);
		(*Interfaces::MoveHelperClient)->SetHost(nullptr);

		if (weapon)
			weapon->UpdateAccuracyPenalty();
	#endif

	//finished the gamemovement
	bInPrediction_Start = false;
	g_Info.m_PredictionMutex.unlock();
}

void MyPlayer::FinishEnginePrediction(CUserCmd* cmd, bool backup_player_state)
{
	if (bInPrediction)
	{
		//store the abs angles so that we can restore the correct one after RunCommand gets called
		Interfaces::Prediction->m_bInPrediction = false;
		Interfaces::Globals->curtime = originalcurtime;
		Interfaces::Globals->frametime = originalframetime;
		//Entity->SetFlags(originalflags);
		*StaticOffsets.GetOffsetValueByType<CBaseEntity**>(_predictionplayer) = originalpredictionplayer;
		*m_pPredictionRandomSeed = originalrandomseed;
		bInPrediction = false;

		//Restore player state after predicting.
		//Do not restore animations because PredictLowerBodyYaw is run before FinishEnginePrediction, which updates the animstate
		//Do not restore bones so that the bones are not invalid in third person when not using chams
		if (backup_player_state)
		{
			QAngle _CurrentAngles = Entity->GetAbsAnglesDirect();
			m_cmdAbsAngles[cmd->command_number % 150] = _CurrentAngles;
			*Entity->m_PlayerCommand() = originalplayercommand;
			*Entity->m_pCurrentCommand() = originalcurrentcommand;

			playerstate.RestoreData(false, false);

			Entity->SetAbsAnglesDirect(_CurrentAngles);
			Entity->SetLocalAnglesDirect(_CurrentAngles);

			Entity->SetTickBase(m_nOldTickbase);
			Interfaces::Globals->curtime = m_flOldCurtime;
		}

		//UpdateAccuracyPenalty updates the penalty and recoil index if it is time, but does not set the next time, so restore the original
		//If you do not restore these values, the game will update the penalty again which will result in the wrong penalty and recoil index later on
		CBaseCombatWeapon *weapon = Entity->GetWeapon();
		if (weapon)
		{
			weapon->SetAccuracyPenalty(originalaccuracypenalty);
			weapon->SetRecoilIndex(originalrecoilindex);
		}
		cmd->hasbeenpredicted = false;
		cmd->random_seed = 0;
	}
}

void MyPlayer::CorrectAutostop(CUserCmd *cmd)
{
	if (!variable::get().ragebot.b_autostop || !LocalPlayer.Entity || !LocalPlayer.Entity->IsOnGround() || (!LocalPlayer.WeaponVars.IsGun && !LocalPlayer.WeaponVars.IsTaser) || !LocalPlayer.CurrentWeapon || LocalPlayer.CurrentWeapon->GetClipOne() == 0 || !g_WeaponController.WeaponDidFire(cmd->buttons | IN_ATTACK))
		return;

	auto velocity = LocalPlayer.Entity->GetVelocity();
	velocity.z = 0.f;

	float speed = velocity.Length2D();

	//if (speed < 1.f)
	//{
	//	cmd->forwardmove = cmd->sidemove = 0.f;
	//	return;
	//}

	float acceleration = sv_accelerate.GetVar()->GetFloat();
	float maxspeed = sv_maxspeed.GetVar()->GetFloat();
	float surface_friction = LocalPlayer.Entity->GetSurfaceFriction();
	float max_accel_speed = acceleration * Interfaces::Globals->interval_per_tick * maxspeed * surface_friction;

	float wishspeed = 0.f;

	if (speed - max_accel_speed <= -1.f)
	{
		wishspeed = max_accel_speed / (speed / (acceleration * Interfaces::Globals->interval_per_tick));
	}
	else
	{
		wishspeed = max_accel_speed;
	}

	QAngle resistance{};
	VectorAngles((velocity * -1.f), resistance);
	resistance.y = cmd->viewangles.y - resistance.y;

	Vector resistance_vec{};
	AngleVectors(resistance, &resistance_vec);

	//printf("autostop | speed: %f | wishspeed: %f\n", speed, wishspeed);

	cmd->forwardmove = clamp(resistance_vec.x * wishspeed, 0.0f, 450.0f);
	cmd->sidemove = clamp(resistance_vec.y * wishspeed, 0.0f, 450.0f);
}

void MyPlayer::GetAverageFrameTime()
{
	static std::array<float, 30> m_FrameTimes{ Interfaces::Globals->absoluteframetime };
	m_FrameTimes[CurrentUserCmd.cmd->command_number % 30] = Interfaces::Globals->absoluteframetime;
	float avg_frametime = 0.0f;
	for (auto _time : m_FrameTimes)
		avg_frametime += _time;

	avg_frametime /= 30.0f;

	m_flAverageFrameTime = avg_frametime;
}

void MyPlayer::UpdateCurrentWeapon()
{
	CBaseCombatWeapon* OldWeapon = CurrentWeapon;
	CurrentWeapon = Entity->GetWeapon();
	int ItemDefinitionIndex = CurrentWeapon ? CurrentWeapon->GetItemDefinitionIndex() : -1;
	bool Changed = CurrentWeapon != OldWeapon || (CurrentWeapon && CurrentWeapon->GetItemDefinitionIndex() != CurrentWeaponItemDefinitionIndex);

	if (Changed)
	{
		CurrentWeaponItemDefinitionIndex = ItemDefinitionIndex;

		if (CurrentWeapon)
		{
			const int index = CurrentWeaponItemDefinitionIndex;

			WeaponVars.iItemDefinitionIndex = index;
			WeaponInfo_t *data = CurrentWeapon != nullptr ? CurrentWeapon->GetCSWpnData() : nullptr;
			WeaponVars.flRange = data ? data->flRange : 8192.0f;

			WeaponVars.IsGun = CurrentWeapon->IsGun();
			WeaponVars.IsShotgun = CurrentWeapon->IsShotgun(index);
			WeaponVars.IsPistol = CurrentWeapon->IsPistol(index);
			WeaponVars.IsSniper = CurrentWeapon->IsSniper(true, index);
			WeaponVars.IsKnife = CurrentWeapon->IsKnife(index);
			WeaponVars.IsGrenade = CurrentWeapon->IsGrenade(index);
			WeaponVars.IsC4 = index == WEAPON_C4;
			WeaponVars.IsTaser = index == WEAPON_TASER;
			WeaponVars.IsAutoSniper = index == WEAPON_G3SG1 || index == WEAPON_SCAR20;
			WeaponVars.IsRevolver = index == WEAPON_REVOLVER;

			WeaponVars.IsScopedWeapon = index == WEAPON_AUG || index == WEAPON_SG556 || CurrentWeapon->IsSniper(true, false); // we should just use aug, sg556 and check for snipers as previous zoom did not check aug anyway

			WeaponVars.IsBurstableWeapon = WeaponVars.IsGun && data ? data->bHasBurstMode : false;
			WeaponVars.IsFullAuto = data ? data->bFullAuto : false;
			//WeaponVars.CanLegitRCS = false;

			/*
			if (index == WEAPON_GLOCK || index == WEAPON_CZ75A || index == WEAPON_FISTS || index == WEAPON_HEALTHSHOT
				|| index == WEAPON_TABLET || index == WEAPON_MELEE
				|| index == WEAPON_FIREBOMB || index == WEAPON_BREACHCHARGE
				|| index == WEAPON_HAMMER)*/
			{
				WeaponVars.LastMode = CurrentWeapon->GetMode();
			}
		}
		else
		{
			//No CurrentWeapon
			WeaponVars.iItemDefinitionIndex = 0;
			WeaponVars.IsGun = false;
			WeaponVars.IsShotgun = false;
			WeaponVars.IsPistol = false;
			WeaponVars.IsSniper = false;
			WeaponVars.IsKnife = false;
			WeaponVars.IsGrenade = false;
			WeaponVars.IsC4 = false;
			WeaponVars.IsTaser = false;
			WeaponVars.IsAutoSniper = false;
			WeaponVars.IsRevolver = false;
			WeaponVars.IsScopedWeapon = false;
			WeaponVars.IsBurstableWeapon = false;
			WeaponVars.IsFullAuto = false;
			//WeaponVars.CanLegitRCS = false;
			//WeaponVars.CanLegitRCS = false;
			//WeaponVars.LegitRCSIndex = 0;
			//WeaponVars.dbNextRCSIndexChangeTime = 0.0;
			//WeaponVars.dbLastRCS = 0.0;
			//WeaponVars.totalrecoilsubtracted = angZero;
			//WeaponVars.totalrecoiladded = angZero;
			//WeaponVars.recoilslope = angZero;
		}
	}
	else
	{
		if (CurrentWeapon)
		{
			WeaponVars.LastMode = CurrentWeapon->GetMode();
		}
	}
}

void SetAnimationEyeInfo(CBaseEntity* entity, QAngle& eyeangles)
{
	NormalizeAngles(eyeangles);

	*entity->EyeAngles() = eyeangles;
	float EyeYaw = eyeangles.y;
	float EyePitch = eyeangles.x + entity->GetThirdPersonRecoil();
	NormalizeAngle(EyePitch);
	entity->SetLocalAngles(eyeangles);
	entity->SetAngleRotation(eyeangles);

	entity->GetAngleRotation().x = 0.0f;
	entity->GetLocalAnglesDirect().x = 0.0f;
	entity->GetAbsAnglesDirect().x = 0.0f;
	entity->SetAbsOrigin(entity->GetLocalOrigin());
}

void ForceSetupBones(CBaseEntity* entity, matrix3x4_t *dest)
{
	entity->InvalidateBoneCache();
	entity->SetLastOcclusionCheckFlags(0);
	entity->SetLastOcclusionCheckFrameCount(Interfaces::Globals->framecount);
	entity->SetupBones(dest, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, Interfaces::Globals->curtime);
	AllowSetupBonesToUpdateAttachments = false;
}

void RestoreServerAnimations(CBaseEntity* entity, CPlayerrecord* record)
{
	auto DataUpdateVars = &record->DataUpdateVars;
	auto animstate = entity->GetPlayerAnimState();
	entity->WriteAnimLayers(DataUpdateVars->m_PostAnimLayers);
	animstate->m_flFeetCycle = DataUpdateVars->m_flPostFeetCycle;
	animstate->m_flFeetWeight = DataUpdateVars->m_flPostFeetWeight;
}

void MyPlayer::UpdateAnimations(QAngle& predictionangles, bool& waiting_on_lby_update_from_server, float& last_real_lby)
{
	auto animstate = Entity->GetPlayerAnimState();
	float flTimeSinceLastAnimationUpdate = fmaxf(0.0f, Interfaces::Globals->curtime - m_flLastAnimationUpdateTime);

	m_flLastAnimationUpdateTime = Interfaces::Globals->curtime;

	float flLastDuckAmount = m_last_duckamount;
	float flNewDuckAmount = clamp(Entity->GetDuckAmount() + animstate->m_flHitGroundCycle, 0.0f, 1.0f);
	float flDuckSmooth = flTimeSinceLastAnimationUpdate * 6.0f;
	float flDuckDelta = flNewDuckAmount - flLastDuckAmount;

	if (flDuckDelta <= flDuckSmooth) {
		if (-flDuckSmooth > flDuckDelta)
			flNewDuckAmount = flLastDuckAmount - flDuckSmooth;
	}
	else {
		flNewDuckAmount = flDuckSmooth + flLastDuckAmount;
	}

	float newduckamount = clamp(flNewDuckAmount, 0.0f, 1.0f);
	m_last_duckamount = newduckamount;

	//End of Update and before SetupVelocity

	//SetupVelocity
	Vector absvel = *Entity->GetAbsVelocity();
	absvel.z = 0.0f;
	Vector newvelocity = g_LagCompensation.GetSmoothedVelocity(flTimeSinceLastAnimationUpdate * 2000.0f, absvel, m_lastvelocity);
	m_lastvelocity = newvelocity;

	float speed = fminf(newvelocity.Length(), 260.0f);

	// TODO: when C_CSGOPlayerAnimState::Update is fully reversed, replace the local player weapon with the animstate->pWeapon
	float flMaxMovementSpeed = 260.0f;
	if (Entity && Entity->GetWeapon())
		flMaxMovementSpeed = fmaxf(LocalPlayer.Entity->GetWeapon()->GetMaxSpeed(), 0.001f);

	float m_flRunningSpeed = speed / (flMaxMovementSpeed * 0.520f);
	float m_flDuckingSpeed = speed / (flMaxMovementSpeed * 0.340f);

	m_curfeetyaw = m_goalfeetyaw;
	m_goalfeetyaw = clamp(m_goalfeetyaw, -360.0f, 360.0f);
	float eye_feet_delta = AngleDiff(predictionangles.y, m_goalfeetyaw);

	float flRunningSpeed = clamp(m_flRunningSpeed, 0.0f, 1.0f);
	float flYawModifier = (((animstate->m_flGroundFraction * -0.3f) - 0.2f) * flRunningSpeed) + 1.0f; //(0.8f - (animstate->m_flGroundFraction * 0.3f) - 1.0f * flRunningSpeed) + 1.0f;

	if (newduckamount > 0.0f) {
		float flDuckingSpeed = clamp(m_flDuckingSpeed, 0.0f, 1.0f);
		flYawModifier += (newduckamount * flDuckingSpeed * (0.5f - flYawModifier));
	}
	float flMaxYawModifier = flYawModifier * animstate->m_flMaxYaw;
	float flMinYawModifier = flYawModifier * animstate->m_flMinYaw;

	if (eye_feet_delta <= flMaxYawModifier) {
		if (flMinYawModifier > eye_feet_delta)
			m_goalfeetyaw = fabs(flMinYawModifier) + predictionangles.y;
	}
	else {
		m_goalfeetyaw = predictionangles.y - fabsf(flMaxYawModifier);
	}

	NormalizeAngle(m_goalfeetyaw);

	//Check to see if the server sent us a new lby
	float original_lby = Entity->GetLowerBodyYaw();
	if (waiting_on_lby_update_from_server && last_real_lby != original_lby)
	{
		m_predicted_lowerbodyyaw = original_lby;
		m_predicted_lowerbodyyaw_lastsent = original_lby;
		m_last_lby_update_time = Interfaces::Globals->curtime;
		waiting_on_lby_update_from_server = false;
	}

	if (speed <= 0.1f && fabsf(Entity->GetAbsVelocity()->z) <= 100.0f) {
		float newgoalfeetyaw = ApproachAngle(
			m_predicted_lowerbodyyaw,
			m_goalfeetyaw,
			flTimeSinceLastAnimationUpdate * 100.0f);

		//printf("eye_feet_delta: %f", eye_feet_delta);

		m_goalfeetyaw = newgoalfeetyaw;

		//printf(" - m_curfeetyaw: %f - m_goalfeetyaw: %f", m_curfeetyaw, m_goalfeetyaw);

		float goalfeetyaw_eye_delta = AngleDiff(m_goalfeetyaw, predictionangles.y);

		//printf(" - goalfeetyaw_eye_delta: %f", goalfeetyaw_eye_delta);

		if (Interfaces::Globals->curtime > m_next_lby_update_time) {

			if (fabsf(goalfeetyaw_eye_delta) > 35.0f) {
				m_next_lby_update_time = Interfaces::Globals->curtime + 1.1f;
				/*if (!bFakeWalking)
					CurrentUserCmd.SetForceNextPacketToSend(true);*/

				if (m_predicted_lowerbodyyaw != predictionangles.y) {
					m_predicted_lowerbodyyaw = predictionangles.y;
					waiting_on_lby_update_from_server = true;
					last_real_lby = original_lby;
				}
			}
		}
		//printf("\n");
	}
	else {
		float newgoalfeetyaw = ApproachAngle(
			predictionangles.y,
			m_goalfeetyaw,
			((animstate->m_flGroundFraction * 20.0f) + 30.0f)
			* flTimeSinceLastAnimationUpdate);

		m_goalfeetyaw = newgoalfeetyaw;
		m_next_lby_update_time = Interfaces::Globals->curtime + 0.22f;

		if (m_predicted_lowerbodyyaw != predictionangles.y) {
			m_predicted_lowerbodyyaw = predictionangles.y;
			waiting_on_lby_update_from_server = true;
			last_real_lby = original_lby;
		}
	}
}

void MyPlayer::PredictLowerBodyYaw(QAngle& viewangles)
{
	CPlayerrecord *player = Entity->ToPlayerRecord();
	CCSGOPlayerAnimState *realanimstate;
	QAngle *animeyeangles;

	float roundedtime = ROUND_TO_TICKS(Interfaces::Globals->realtime);
	float spawntime = Entity->GetSpawnTime();
	float original_lby = Entity->GetLowerBodyYaw();;
	float original_predicted_lby;
	static bool waiting_on_lby_update_from_server = true;
	static float last_real_lby = 0.0f;
	static float lastsimulationtime = Entity->GetSimulationTime();
	static float lastupdatetime = ROUND_TO_TICKS(Interfaces::Globals->realtime - 0.015625f);
	static float lastsimulationupdate = roundedtime;
	bool received_server_update;
	if (received_server_update = Entity->GetSimulationTime() != lastsimulationtime)
	{
		lastsimulationtime = Entity->GetSimulationTime();
		lastsimulationupdate = roundedtime;
	}

	if (player->m_pAnimStateServer[ResolveSides::NONE] && player->m_pAnimStateServer[ResolveSides::NONE]->pBaseEntity != Entity)
	{
		delete player->m_pAnimStateServer[ResolveSides::NONE];
		player->m_pAnimStateServer[ResolveSides::NONE] = nullptr;
	}

	if (!player->m_pAnimStateServer[ResolveSides::NONE])
	{
		if (!(player->m_pAnimStateServer[ResolveSides::NONE] = CreateCSGOPlayerAnimState(Entity)))
			return;
	}

	realanimstate = player->m_pAnimStateServer[ResolveSides::NONE];

	if (spawntime != m_spawntime)
	{
		realanimstate->Reset();
		m_spawntime = spawntime;
		last_real_lby = Entity->GetLowerBodyYaw();
		Entity->CopyAnimLayers(m_AnimLayers);
		oResetAnimState(LocalPlayer.Entity->GetPlayerAnimState());
		real_playerbackup.Get(Entity);
		real_playerbackup_lastsent = fake_playerbackup = real_playerbackup;
		m_predicted_lowerbodyyaw = original_lby;
		m_predicted_lowerbodyyaw_lastsent = original_lby;
		m_last_lby_update_time = 0.0f;
		lastupdatetime = ROUND_TO_TICKS(Interfaces::Globals->realtime - TICK_INTERVAL);
		waiting_on_lby_update_from_server = false;
	}

	animeyeangles = &viewangles;

	// did we shoot during any of the chokes? if so, our angles will always be the shot angles due to sv_maxusrcmdprocessticks_holdaim
	if (CreateMoveVars.LastShot.didshoot)
		animeyeangles = &CreateMoveVars.LastShot.viewangles;

	SetAnimationEyeInfo(Entity, *animeyeangles);

	original_predicted_lby = m_predicted_lowerbodyyaw;
	Entity->SetLowerBodyYaw(m_predicted_lowerbodyyaw);

	//check if server sent us an lby update and compare to our predicted value
	if (waiting_on_lby_update_from_server && last_real_lby != original_lby)
	{
		m_predicted_lowerbodyyaw = original_lby;
		m_predicted_lowerbodyyaw_lastsent = original_lby;
		m_last_lby_update_time = Interfaces::Globals->curtime;
		waiting_on_lby_update_from_server = false;
	}

	//restore our own layers 
	Entity->WriteAnimLayers(m_AnimLayers);

	//Run any game movement triggered animation events
	for (auto &event : m_PredictionAnimationEventQueue)
	{
		realanimstate->DoAnimationEvent(event);
	}
	m_PredictionAnimationEventQueue.clear();

	if (g_ClientState->chokedcommands == 0)
	{
		//Restore server layers for specific things
		Entity->WriteAnimLayer(&player->DataUpdateVars.m_PostAnimLayers[FLASHEDSEQUENCE_LAYER], Entity->GetAnimOverlay(FLASHEDSEQUENCE_LAYER));
		Entity->WriteAnimLayer(&player->DataUpdateVars.m_PostAnimLayers[FLINCHSEQUENCE_LAYER], Entity->GetAnimOverlay(FLINCHSEQUENCE_LAYER));
		Entity->WriteAnimLayer(&player->DataUpdateVars.m_PostAnimLayers[MAIN_IDLE_SEQUENCE_LAYER], Entity->GetAnimOverlay(MAIN_IDLE_SEQUENCE_LAYER));
		Entity->WriteAnimLayer(&player->DataUpdateVars.m_PostAnimLayers[AIMSEQUENCE_LAYER1], Entity->GetAnimOverlay(AIMSEQUENCE_LAYER1));
		Entity->WriteAnimLayer(&player->DataUpdateVars.m_PostAnimLayers[AIMSEQUENCE_LAYER2], Entity->GetAnimOverlay(AIMSEQUENCE_LAYER2));
		Entity->WriteAnimLayer(&player->DataUpdateVars.m_PostAnimLayers[SILENCERCHANGESEQUENCE_LAYER], Entity->GetAnimOverlay(SILENCERCHANGESEQUENCE_LAYER));
		Entity->WriteAnimLayer(&player->DataUpdateVars.m_PostAnimLayers[WHOLEBODYACTION_LAYER], Entity->GetAnimOverlay(WHOLEBODYACTION_LAYER));
		Entity->WriteAnimLayer(&player->DataUpdateVars.m_PostAnimLayers[IDLE_LAYER], Entity->GetAnimOverlay(IDLE_LAYER));
	}

	Entity->UpdateServerSideAnimation(ResolveSides::NONE, Interfaces::Globals->curtime);

	Entity->CopyAnimLayers(m_AnimLayers);

	//save updated lby
	if (Entity->GetLowerBodyYaw() != m_predicted_lowerbodyyaw)
	{
		waiting_on_lby_update_from_server = true;
		last_real_lby = original_lby;
		m_predicted_lowerbodyyaw = Entity->GetLowerBodyYaw();
	}

	if (CurrentUserCmd.bSendPacket)
	{
		AllowSetupBonesToUpdateAttachments = true;
		ForceSetupBones(Entity, RealAngleMatrix);
		memcpy(RealAngleMatrixFixed, RealAngleMatrix, sizeof(matrix3x4_t) * MAXSTUDIOBONES);
		RealAngleEntityToWorldTransform = Entity->EntityToWorldTransform();
		PositionMatrix(Entity->GetAbsOriginDirect(), RealAngleEntityToWorldTransform);
		LastAnimatedOrigin = Entity->GetAbsOriginDirect();
		LastAnimatedAngles = Entity->GetAbsAnglesDirect();
	}

	m_curfeetyaw = realanimstate->m_flCurrentFeetYaw;
	m_goalfeetyaw = realanimstate->m_flGoalFeetYaw;
	m_last_duckamount = realanimstate->m_fDuckAmount;
	m_flLastAnimationUpdateTime = realanimstate->m_flLastClientSideAnimationUpdateTime;
	m_next_lby_update_time = realanimstate->m_flNextLowerBodyYawUpdateTime;
	m_lastvelocity = realanimstate->m_vVelocity;
	m_wasonground = realanimstate->m_bOnGround;

	Vector Vel = (*Entity->GetAbsOrigin() - real_playerbackup.AbsOrigin) / TICK_INTERVAL;
	real_playerbackup.Get(Entity);
	real_playerbackup.TicksChoked = g_ClientState->chokedcommands;
	real_playerbackup.LastDuckAmount = real_playerbackup_lastsent.DuckAmount;
	real_playerbackup.Teleporting = (Entity->GetNetworkOrigin() - real_playerbackup_lastsent.NetworkOrigin).LengthSqr() > (64.0f * 64.0f);
	if (!CurrentUserCmd.bSendPacket)
		++real_playerbackup.TicksChoked;
	real_playerbackups[CurrentUserCmd.cmd->command_number % 150] = real_playerbackup;

	//Now do the fake animstate

	//This will restore the fake animstate since we are using the client animation code for the fake angle chams
	real_playerbackup_lastsent.RestoreData(true, false);
	fake_playerbackup.RestoreBoneData();

	//Find out how much time we have to animate. The client animates on frames and not ticks
	float curtime = Interfaces::Globals->curtime;
	if (fabsf(roundedtime - lastupdatetime) > 0.1f)
		lastupdatetime = roundedtime - 0.1f;

	const int nTicks = (roundedtime - lastupdatetime) / Interfaces::Globals->absoluteframetime + 1;

	//Animate all of the frames that the client ran since the last usercmd
	for (int iter = 1; iter <= nTicks; ++iter)
	{
		Interfaces::Globals->curtime = ROUND_TO_TICKS(lastupdatetime + (float)iter * Interfaces::Globals->absoluteframetime);

		if (iter == nTicks)
		{
			C_CSGOPlayerAnimState *fakeanimstate = Entity->GetPlayerAnimState();
			if (CurrentUserCmd.bSendPacket)
			{
				real_playerbackup_lastsent = real_playerbackup;
				real_playerbackup_lastsent.RestoreData(true, false);
				fakeanimstate->m_flFeetCycle = real_playerbackup.m_AnimLayers[FEET_LAYER]._m_flCycle;
				fakeanimstate->m_flFeetWeight = real_playerbackup.m_AnimLayers[FEET_LAYER].m_flWeight;
				m_predicted_lowerbodyyaw_lastsent = m_predicted_lowerbodyyaw;
			}
			else
			{
				Entity->WriteAnimLayers(real_playerbackup_lastsent.m_AnimLayers);
				fakeanimstate->m_flFeetCycle = real_playerbackup_lastsent.m_AnimLayers[FEET_LAYER]._m_flCycle;
				fakeanimstate->m_flFeetWeight = real_playerbackup_lastsent.m_AnimLayers[FEET_LAYER].m_flWeight;
			}
		}

		*animeyeangles = real_playerbackup_lastsent.EyeAngles;
		SetAnimationEyeInfo(Entity, *animeyeangles);
		Entity->SetLowerBodyYaw(m_predicted_lowerbodyyaw_lastsent);
		Entity->UpdateClientSideAnimation();
		Entity->Interpolate(Interfaces::Globals->curtime);
	}

	lastupdatetime = roundedtime;
	Interfaces::Globals->curtime = curtime;

	RunningFakeAngleBones = true;
	AllowSetupBonesToUpdateAttachments = false;
	ForceSetupBones(Entity, FakeAngleMatrix);
	RunningFakeAngleBones = false;

	FakeAngleEntityToWorldTransform = Entity->EntityToWorldTransform();
	PositionMatrix(Entity->GetAbsOriginDirect(), FakeAngleEntityToWorldTransform);
	LastAnimatedOrigin_Fake = Entity->GetAbsOriginDirect();
	LastAnimatedAngles_Fake = Entity->GetAbsAnglesDirect();

	fake_playerbackup.Get(Entity);
	real_playerbackup.m_BackupClientAnimState = real_playerbackup_lastsent.m_BackupClientAnimState = fake_playerbackup.m_BackupClientAnimState; //Update the client animstate in our real backup to the current client animstate

	//Finished, restore data and save desync state
	real_playerbackup.RestoreData();

	if (CurrentUserCmd.bSendPacket)
		m_predicted_lowerbodyyaw_lastsent = m_predicted_lowerbodyyaw;

	QAngle fakeangles = CalcAngle(fake_playerbackup.AbsOrigin, Entity->GetBonePositionCachedOnly(HITBOX_HEAD, FakeAngleMatrix));
	QAngle realangles = CalcAngle(real_playerbackup_lastsent.AbsOrigin, Entity->GetBonePositionCachedOnly(HITBOX_HEAD, RealAngleMatrix));
	m_flDesynced = fabs(AngleDiff(realangles.y, fakeangles.y));
	m_bDesynced = m_flDesynced > 5.0f;

#ifdef USE_SERVER_SIDE
	float simtime = TICKS_TO_TIME(LocalPlayer.Entity->GetTickBase());
	BackupMatrixStruct *str = m_RealMatrixBackups.back<BackupMatrixStruct>();
	if (str && str->m_SimulationTime == simtime)
		memcpy(str->m_Matrix, LocalPlayer.RealAngleMatrix, sizeof(matrix3x4_t) * MAXSTUDIOBONES);
	else
		m_RealMatrixBackups.write<BackupMatrixStruct>(BackupMatrixStruct(simtime, -999, LocalPlayer.RealAngleMatrix, LocalPlayer.real_playerbackup.AbsOrigin, LocalPlayer.real_playerbackup.AbsAngles));

	if (m_RealMatrixBackups.getcount<BackupMatrixStruct>() > 72)
		m_RealMatrixBackups.pop_front<BackupMatrixStruct>();

	str = m_FakeMatrixBackups.back<BackupMatrixStruct>();
	if (str && str->m_SimulationTime == simtime)
		memcpy(str->m_Matrix, LocalPlayer.FakeAngleMatrix, sizeof(matrix3x4_t) * MAXSTUDIOBONES);
	else
		m_FakeMatrixBackups.write<BackupMatrixStruct>(BackupMatrixStruct(simtime, -999, LocalPlayer.FakeAngleMatrix, LocalPlayer.fake_playerbackup.AbsOrigin, LocalPlayer.fake_playerbackup.AbsAngles));

	if (m_FakeMatrixBackups.getcount<BackupMatrixStruct>() > 72)
		m_FakeMatrixBackups.pop_front<BackupMatrixStruct>();
#endif

#if 0
	static bool waiting_on_lby_update_from_server = true;
	static float last_real_lby = 0.0f;
	const float SpawnTime = Entity->GetSpawnTime();
	bool valid = true;
	//Todo: if fake_animstate contains bad data then the game will crash in SetupBones->StandardBlendingRules
	//Find a better way to check if we need to clear it
	if (SpawnTime == 0.0f || fake_playerbackup.m_BackupClientAnimState.pBaseEntity != Entity || real_playerbackup.m_BackupClientAnimState.pBaseEntity != Entity || real_playerbackup_lastsent.m_BackupClientAnimState.pBaseEntity != Entity || real_playerbackup_lastsent_pristine.m_BackupClientAnimState.pBaseEntity != Entity)
	{
		fake_playerbackup.Get(Entity);
		real_playerbackup = fake_playerbackup;
		real_playerbackup_lastsent = fake_playerbackup;
		real_playerbackup_lastsent_pristine = fake_playerbackup;
		valid = false;
	}
	if (SpawnTime != m_spawntime) 
	{
		fake_playerbackup.Get(Entity);
		real_playerbackup = fake_playerbackup;
		real_playerbackup_lastsent = fake_playerbackup;
		real_playerbackup_lastsent_pristine = fake_playerbackup;
		m_spawntime = SpawnTime;
		m_curfeetyaw = viewangles.y;
		m_goalfeetyaw = viewangles.y;
		m_predicted_lowerbodyyaw = viewangles.y;
		m_predicted_lowerbodyyaw_lastsent = viewangles.y;
		m_last_duckamount = 0.0f;
		m_flLastAnimationUpdateTime = 0.0f;
		m_next_lby_update_time = 0.0f;
		m_lastvelocity.Init(0, 0, 0);
		m_wasonground = false;
		waiting_on_lby_update_from_server = true;
		last_real_lby = 0.0f;
		valid = false;
	}

	//real yaw


	CPlayerrecord* _record = g_LagCompensation.GetPlayerrecord(Entity);
	C_CSGOPlayerAnimState *m_pClientAnimState = Entity->GetPlayerAnimState();

	PlayerBackup_t *pristine_backup = new PlayerBackup_t(Entity);

	QAngle *srcangles = &viewangles;

	// did we shoot during any of the chokes? if so, our angles will always be the shot angles due to sv_maxusrcmdprocessticks_holdaim
	if (CreateMoveVars.LastShot.didshoot)
		srcangles = &CreateMoveVars.LastShot.viewangles;

	if (CurrentUserCmd.bSendPacket)
	{
		//backup the current state so we can re-animate when the server sends us updated animlayers
		real_playerbackup_lastsent_pristine = *pristine_backup;
		eyeangles_lastsent_pristine = *srcangles;
	}

	*m_pClientAnimState = real_playerbackup.m_BackupClientAnimState;
	//restore last animated layers
	//Entity->WriteAnimLayers(real_playerbackup.AnimLayer);
	//Entity->InvalidateAnimations();

	CCSGOPlayerAnimState *_serverAnimState = _record->m_animstate_server;
	if (_serverAnimState)
	{
		//Run any game movement triggered animation events
		for (auto &event : m_PredictionAnimationEventQueue)
		{
			_serverAnimState->DoAnimationEvent(event);
		}
		m_PredictionAnimationEventQueue.clear();
	}

	QAngle predictionangles = *srcangles;
	SetAnimationEyeInfo(Entity, predictionangles);

	float original_lby = Entity->GetLowerBodyYaw();
	
	// set lby to the predicted server lby
	Entity->SetLowerBodyYaw(m_predicted_lowerbodyyaw);

	//Run setupvelocity
	UpdateAnimations(predictionangles, waiting_on_lby_update_from_server, last_real_lby);

	// update anims for real angles
	Entity->UpdateClientSideAnimation();
	
	// set lby back to current value
	Entity->SetLowerBodyYaw(original_lby);

	m_goalfeetyaw = m_pClientAnimState->m_flGoalFeetYaw;
	m_curfeetyaw = m_pClientAnimState->m_flCurrentFeetYaw;
	m_last_duckamount = m_pClientAnimState->m_fDuckAmount;

	static QAngle angRealAngles = QAngle(0.f, 0.f, 0.f), angFakeAngles = QAngle(0.f, 0.f, 0.f);
	Vector dummy, vecOrigin;

#ifdef IMI_MENU
	bool bCanFakelag = g_Convars.HVH.hvh_fakelag->GetBool() && g_Convars.HVH.hvh_fakelag_value->GetInt() > 0 || g_Convars.AntiAim.antiaim_desync->GetBool();
#else
	bool bCanFakelag = LocalPlayer.Config_IsFakelagging() && g_Fakelag.get_choked_ticks() > 0 || LocalPlayer.Config_IsDesyncing();//(variable::get().ragebot.i_fakelag_mode > 0 && variable::get().ragebot.i_fakelag_ticks > 0) || LocalPlayer.Config_IsDesyncing();
#endif

	//Only allow rendering of the newest packet because all of the choked commands will never be seen by clients, only the server during simulation
	if (CurrentUserCmd.bSendPacket)
	{
		//just render server animations, we know they are old at the point of sending packet but it prevents our client fighting with the server values causing flicker and weird legs
		//RestoreServerAnimations(Entity, _record);
		//DO NOT RESTORE SERVER ANIMATIONS OR IT DESYNCS THE LEGS!
#ifdef USE_SERVER_SIDE
		bool ConnectedToServerSide = pServerSide.IsSocketCreated();
#else
		bool ConnectedToServerSide = false;
#endif

		C_AnimationLayer *current_layers = nullptr;
		if (real_playerbackup.Entity == Entity)
		{
			//restore server animation layers to prevent jittering
			current_layers = new C_AnimationLayer[15];
			Entity->CopyAnimLayers(current_layers);
			RestoreServerAnimations(Entity, _record);
		}

		//if (ConnectedToServerSide || !bCanFakelag || Entity->GetCachedBoneData()->count == 0)
		{
			AllowSetupBonesToUpdateAttachments = true;
			ForceSetupBones(Entity, RealAngleMatrix);
			memcpy(RealAngleMatrixFixed, RealAngleMatrix, sizeof(matrix3x4_t) * MAXSTUDIOBONES);
			RealAngleEntityToWorldTransform = Entity->EntityToWorldTransform();
			PositionMatrix(Entity->GetAbsOriginDirect(), RealAngleEntityToWorldTransform);
			LastAnimatedOrigin = Entity->GetAbsOriginDirect();
			LastAnimatedAngles = Entity->GetAbsAnglesDirect();
		}

		dummy = Entity->GetBonePositionCachedOnly(HITBOX_HEAD, RealAngleMatrix);
		vecOrigin = Entity->GetAbsOriginDirect();
		angRealAngles = CalcAngle(vecOrigin, dummy);

		if (current_layers)
		{
			Entity->WriteAnimLayers(current_layers);
			delete[] current_layers;
		}

		//save real state
		real_playerbackup.Get(Entity);
		real_playerbackup_lastsent = real_playerbackup;
		m_predicted_lowerbodyyaw_lastsent = m_predicted_lowerbodyyaw;
	}
	else
	{
		//save real state
		real_playerbackup.Get(Entity);

		//restore to last sent packet and render the latest animation layers received from the server

		//TODO: FIXME: restoring server animation layers is causing local player weapon they are holding to show the old layers, not the new layers
#if 0
		//re-animate the original tick we sent with the latest received server animations to get the correct local player aim matrix, etc

		real_playerbackup_lastsent_pristine.RestoreData();

		float backup_thirdpersonrecoil = Entity->GetThirdPersonRecoil();
		Entity->SetThirdPersonRecoil(thirdpersonrecoil_lastsent_pristine);

		RestoreServerAnimations(Entity, _record);

		QAngle oldpredictionangles = eyeangles_lastsent_pristine;
		SetAnimationEyeInfo(Entity, oldpredictionangles);

		Entity->InvalidateAnimations();
		Entity->UpdateClientSideAnimation();
		Entity->InvalidatePhysicsRecursive(ANGLES_CHANGED);

		Entity->SetThirdPersonRecoil(backup_thirdpersonrecoil);
#else
		real_playerbackup_lastsent.RestoreData(true, true);
#endif

		//restore server animation layers
		RestoreServerAnimations(Entity, _record);

		AllowSetupBonesToUpdateAttachments = true;
		ForceSetupBones(Entity, RealAngleMatrix);

		//FIX THE WEAPON YOU ARE HOLDING POSITION
		if (Entity->GetCachedBoneData()->count)
			memcpy(real_playerbackup.CachedBoneMatrices, Entity->GetCachedBoneData()->Base(), sizeof(matrix3x4_t) * Entity->GetCachedBoneData()->count);

		RealAngleEntityToWorldTransform = Entity->EntityToWorldTransform();
		PositionMatrix(Entity->GetAbsOriginDirect(), RealAngleEntityToWorldTransform);

#if 0
		int maxentities = Interfaces::ClientEntList->GetHighestEntityIndex();

		#pragma loop(hint_parallel(2))
		for (int i = 0; i < maxentities; i++)
		{
			C_BaseEntity *ent = Interfaces::ClientEntList->GetBaseEntity(i);
			if (ent && ((ent->GetEffects() & EF_BONEMERGE || ent->GetEffects() & EF_BONEMERGE_FASTCULL) && (ent->GetOwner() == Entity || ent->GetMoveParent() == Entity)))
			{
				AllowSetupBonesToUpdateAttachments = true;
				ent->SetupBones(nullptr, -1, BONE_USED_BY_ANYTHING, Interfaces::Globals->curtime);
			}
		}
		AllowSetupBonesToUpdateAttachments = false;
#endif

		LastAnimatedOrigin = Entity->GetAbsOriginDirect();
		LastAnimatedAngles = Entity->GetAbsAnglesDirect();

		memcpy(RealAngleMatrixFixed, RealAngleMatrix, sizeof(matrix3x4_t) * MAXSTUDIOBONES);  

		dummy = Entity->GetBonePositionCachedOnly(HITBOX_HEAD, RealAngleMatrix);
		vecOrigin = Entity->GetAbsOriginDirect();
		angRealAngles = CalcAngle(vecOrigin, dummy);
	}

	//now do the 'fake' yaw data
	if (CurrentUserCmd.bSendPacket)
	{
		//back up the real yaw state

		pristine_backup->RestoreData(true, true);

		Entity->RemoveEFlag(EFL_DIRTY_ABSTRANSFORM);
		Entity->RemoveEFlag(EFL_DIRTY_ABSVELOCITY);

		//restore animstate to last fake animstate
		*m_pClientAnimState = fake_playerbackup.m_BackupClientAnimState;
		
		//the netvar will not have changed yet if this tick is an lby update and we haven't sent it yet, so force it now
		Entity->SetLowerBodyYaw(m_predicted_lowerbodyyaw);

		predictionangles = *srcangles;
		SetAnimationEyeInfo(Entity, predictionangles);

		Entity->InvalidateAnimations();
		Entity->UpdateClientSideAnimation();
	
		RunningFakeAngleBones = true;
		AllowSetupBonesToUpdateAttachments = false;
		ForceSetupBones(Entity, FakeAngleMatrix);
		RunningFakeAngleBones = false;

		FakeAngleEntityToWorldTransform = Entity->EntityToWorldTransform();
		PositionMatrix(Entity->GetAbsOriginDirect(), FakeAngleEntityToWorldTransform);
		LastAnimatedOrigin_Fake = Entity->GetAbsOriginDirect();
		LastAnimatedAngles_Fake = Entity->GetAbsAnglesDirect();

		//backup the player state we are sending
		fake_playerbackup.Get(Entity);

		Entity->SetLowerBodyYaw(original_lby);

		dummy = Entity->GetBonePositionCachedOnly(HITBOX_HEAD, FakeAngleMatrix);
		vecOrigin = Entity->GetAbsOriginDirect();
		angFakeAngles = CalcAngle(vecOrigin, dummy);

		//now restore client emulated server animations
		real_playerbackup.RestoreData(true, true);
	}
	else
	{
		//choking
		//show our fake yaw how legit players will see it

		//restore to the packet we sent and the fake animations

		fake_playerbackup.RestoreData(true, true);

		//just in case so it doesn't get calculated again
		Entity->RemoveEFlag(EFL_DIRTY_ABSTRANSFORM);
		Entity->RemoveEFlag(EFL_DIRTY_ABSVELOCITY);

		Entity->InvalidateAnimations();
		Entity->SetLowerBodyYaw(m_predicted_lowerbodyyaw_lastsent);
		Entity->UpdateClientSideAnimation();

		RunningFakeAngleBones = true;
		AllowSetupBonesToUpdateAttachments = false;
		ForceSetupBones(Entity, FakeAngleMatrix);
		RunningFakeAngleBones = false;

		FakeAngleEntityToWorldTransform = Entity->EntityToWorldTransform();
		PositionMatrix(Entity->GetAbsOriginDirect(), FakeAngleEntityToWorldTransform);
		LastAnimatedOrigin_Fake = Entity->GetAbsOriginDirect();
		LastAnimatedAngles_Fake = Entity->GetAbsAnglesDirect();

		//save the updated fake animations
		fake_playerbackup.Get(Entity);

		QAngle absangles = Entity->GetAbsAngles();

		dummy = Entity->GetBonePositionCachedOnly(HITBOX_HEAD, FakeAngleMatrix);
		vecOrigin = Entity->GetAbsOriginDirect();
		angFakeAngles = CalcAngle(vecOrigin, dummy);

		//now restore the real yaw state
		real_playerbackup.RestoreData(true, true);
	}

	NormalizeAngles(angRealAngles);
	NormalizeAngles(angFakeAngles);

	m_flDesynced = abs(AngleDiff(angRealAngles.y, angFakeAngles.y));

	m_bDesynced = m_flDesynced > 5.f;

	delete pristine_backup;

#ifdef USE_SERVER_SIDE
	float simtime = TICKS_TO_TIME(LocalPlayer.Entity->GetTickBase());
	BackupMatrixStruct *str = m_RealMatrixBackups.back<BackupMatrixStruct>();
	if (str && str->m_SimulationTime == simtime)
		memcpy(str->m_Matrix, LocalPlayer.RealAngleMatrix, sizeof(matrix3x4_t) * MAXSTUDIOBONES);
	else
		m_RealMatrixBackups.write<BackupMatrixStruct>(BackupMatrixStruct(simtime, -999, LocalPlayer.RealAngleMatrix, LocalPlayer.real_playerbackup.OriginalAbsOrigin, LocalPlayer.real_playerbackup.OriginalAbsAngles));

	if (m_RealMatrixBackups.getcount<BackupMatrixStruct>() > 72)
		m_RealMatrixBackups.pop_front<BackupMatrixStruct>();

	str = m_FakeMatrixBackups.back<BackupMatrixStruct>();
	if (str && str->m_SimulationTime == simtime)
		memcpy(str->m_Matrix, LocalPlayer.FakeAngleMatrix, sizeof(matrix3x4_t) * MAXSTUDIOBONES);
	else
		m_FakeMatrixBackups.write<BackupMatrixStruct>(BackupMatrixStruct(simtime, -999, LocalPlayer.FakeAngleMatrix, LocalPlayer.fake_playerbackup.OriginalAbsOrigin, LocalPlayer.fake_playerbackup.OriginalAbsAngles));

	if (m_FakeMatrixBackups.getcount<BackupMatrixStruct>() > 72)
		m_FakeMatrixBackups.pop_front<BackupMatrixStruct>();
#endif

#endif
}

void MyPlayer::CorrectTickBase()
{
	//NetvarMutex.Lock();
	int delta_tick = g_ClientState->m_nDeltaTick;
	bool valid = delta_tick > 0;
	int last_command_acked = g_ClientState->last_command_ack;
	int last_outgoing_command = g_ClientState->lastoutgoingcommand;
	int choked_commands = g_ClientState->chokedcommands;

	Interfaces::Prediction->Update(delta_tick, valid, last_command_acked, last_outgoing_command + choked_commands);

	m_flOldCurtime = Interfaces::Globals->curtime;
	m_nOldTickbase = Entity->GetTickBase();

	if (Entity)
	{
		g_Tickbase.SetCurrentCommandTickbase();
		Interfaces::Globals->curtime = TICKS_TO_TIME(Entity->GetTickBase());
	}

	//if (LocalPlayer.ShouldShiftShot)
	//	flServerTime -= TICKS_TO_TIME(max(0, GetTickbaseSubtraction() - 1));

	//Entity->SetTickBase(TIME_TO_TICKS(flServerTime));
	//Interfaces::Globals->curtime = flServerTime;

	CUserCmd *cmd = CurrentUserCmd.cmd;

	//TODO: clean this code up below so there isn't two switches
	if (IsSwaying() || (variable::get().ragebot.b_antiaim && !variable::get().ragebot.standing.b_break_lby_by_flicking))
	{
		static bool sw = false;
		if (cmd->forwardmove == 0.0f && cmd->sidemove == 0.0f && !(cmd->buttons & IN_JUMP))
		{
			if (sw)
				cmd->forwardmove = 1.01f;
			else
				cmd->forwardmove = -1.01f;
			sw = !sw;
		}
	}
	else if (m_bPredictionError)
	{
		//Force our lby to update to fix the timer
		if (cmd->forwardmove == 0.0f && cmd->sidemove == 0.0f && !(cmd->buttons & IN_JUMP))
		{
			switch (m_iPredictionErrorMovementDir)
			{
			case 0:
			case 2:
				cmd->forwardmove = 1.01f;
				m_iPredictionErrorMovementDir = 1;
				break;
			case 1:
				cmd->forwardmove = -1.01f;
				m_iPredictionErrorMovementDir = 2;
				break;
			};
		}
		else
		{
			m_iPredictionErrorMovementDir = 0;
		}
	}
	else
	{
		if (cmd->forwardmove == 0.0f && cmd->sidemove == 0.0f && !(cmd->buttons & IN_JUMP))
		{
			switch (m_iPredictionErrorMovementDir)
			{
			case 2:
				cmd->forwardmove = 1.01f;
				break;
			case 1:
				cmd->forwardmove = -1.01f;
				break;
			};
		}
		m_iPredictionErrorMovementDir = 0;
	}

	//NetvarMutex.Unlock();
}

bool MyPlayer::IsCurrentTickAnLBYUpdate(bool &ismoving, bool &inair)
{
	ismoving = false;
	inair = false;

	if (!Entity || !IsAlive || Entity->GetSpawnTime() != m_spawntime)
		return true;

	//server uses previous anim update's onground value..
	bool on_ground = Entity->GetPlayerAnimState()->m_bOnGround; // kek...

	Vector absvelocity = *Entity->GetAbsVelocity();
	Vector newvelocity = g_LagCompensation.GetSmoothedVelocity((Interfaces::Globals->curtime - m_flLastAnimationUpdateTime) * 2000.0f, Vector(absvelocity.x, absvelocity.y, 0.0f), m_lastvelocity);
	float newspeed = fminf(newvelocity.Length(), 260.0f);

	if (!on_ground) {
		inair = true;
		//return false;
	}

	if (newspeed > 0.1f || fabsf(absvelocity.z) > 100.0f) {

		if (newspeed > .1f)
			ismoving = true;
		return true;
	}

	if (Interfaces::Globals->curtime > m_next_lby_update_time)
		return true;

	return false;
}

bool MyPlayer::IsNextAnimUpdateAnLBYUpdate(bool *current_is_lby_update) {	
	bool ismoving, inair;
	bool current_is_update = current_is_lby_update ? *current_is_lby_update : IsCurrentTickAnLBYUpdate(ismoving, inair);

	if (current_is_update)
		return false;

	float anim_update_delta = TICKS_TO_TIME(1); // TICKS_TO_TIME(CurrentUserCmd.m_iNumCommandsToChoke + 1);
	float next_animation_update_time = Interfaces::Globals->curtime + anim_update_delta;
	return next_animation_update_time > m_next_lby_update_time;
}

bool MyPlayer::IsAnimUpdateAnLBYUpdate(float time, bool *current_is_lby_update)
{
	bool ismoving, inair;
	bool is_an_lby_update = IsCurrentTickAnLBYUpdate(ismoving, inair);
	if (is_an_lby_update)
		return false;

	return time > m_next_lby_update_time;
}

void MyPlayer::SaveUncompressedNetvars()
{
	int tickbase = Entity->GetTickBase();
	UncompressedNetvars_t *vars = &UncompressedNetvars[tickbase % 150];
	vars->punchangle = Entity->GetPunch();
	vars->punchvelocity = Entity->GetPunchVel();
	vars->viewoffset = Entity->GetViewOffset();
	vars->tickbase = Entity->GetTickBase();
}

void MyPlayer::ApplyUncompressedNetvars()
{
	if (Entity && Entity->GetAlive())
	{
		int tickbase = Entity->GetTickBase();
		UncompressedNetvars_t *vars = &UncompressedNetvars[tickbase % 150];

		if (vars->tickbase == tickbase)
		{
			Vector viewdelta = Entity->GetViewOffset() - vars->viewoffset;
			Vector velocitydelta = Entity->GetPunchVel() - vars->punchvelocity;
			QAngle punchdelta = Entity->GetPunch() - vars->punchangle;

			if (fabsf(punchdelta.x) < 0.03125f &&
				fabsf(punchdelta.y) < 0.03125f &&
				fabsf(punchdelta.z) < 0.03125f)
				Entity->SetPunch(vars->punchangle);

			if (fabsf(velocitydelta.x) < 0.03125f &&
				fabsf(velocitydelta.y) < 0.03125f &&
				fabsf(velocitydelta.z) < 0.03125f)
				Entity->SetPunchVel(vars->punchvelocity);

			if (fabsf(viewdelta.x) < 0.03125f &&
				fabsf(viewdelta.y) < 0.03125f &&
				fabsf(viewdelta.z) < 0.03125f)
				Entity->SetViewOffset(vars->viewoffset);
		}
	}
}

CBaseEntity* MyPlayer::CreateShotRecord()
{
	CBaseEntity* _TargetEntity = nullptr;
	
	// we can shoot
	if (g_WeaponController.WeaponDidFire(CurrentUserCmd.cmd->buttons) != 0)
	{
		CreateMoveVars.LastShot.didshoot = true;
		CreateMoveVars.LastShot.viewangles = CurrentUserCmd.cmd->viewangles;

		// we have a target
		if (g_Ragebot.HasTarget())
			_TargetEntity = Interfaces::ClientEntList->GetBaseEntity(g_Ragebot.GetTarget());

		// no valid target
		if (!_TargetEntity)
		{
			if (IsManuallyFiring(CurrentWeapon))
			{
				CUserCmd* cmd = CurrentUserCmd.cmd;

				trace_t tr;
				Vector eyepos = ShootPosition;
				Vector vecDir;
				AngleVectors(cmd->viewangles, &vecDir);
				VectorNormalizeFast(vecDir);
				Vector endpos = eyepos + vecDir * WeaponVars.flRange;

				//FIXME
				CTraceFilterPlayersOnly filter;
				filter.pSkip = (IHandleEntity*)Entity;
				filter.m_icollisionGroup = COLLISION_GROUP_NONE;
				AngleVectors(cmd->viewangles, &vecDir);
				UTIL_TraceLine(eyepos, endpos, MASK_SHOT, &filter, &tr);
				UTIL_ClipTraceToPlayers(eyepos, endpos + vecDir * 40.0f, MASK_SHOT, &filter, &tr);

				if (tr.m_pEnt && tr.m_pEnt->IsPlayer())
				{
					_TargetEntity = tr.m_pEnt;

					// save hitgroup since weaponcontroller did not set it
					CPlayerrecord* _playerRecord = g_LagCompensation.GetPlayerrecord(tr.m_pEnt);
#if _DEBUG
					_TargetEntity->DrawHitboxesFromCache(ColorRGBA(0, 15, 0, 255), 3.0f, (matrix3x4_t*)_TargetEntity->GetCachedBoneData()->Base());
#endif
					_playerRecord->LastShot.m_iTargetHitgroup = tr.hitgroup;
					_playerRecord->LastShot.m_iTargetHitbox = tr.hitbox;
				}
				else
				{
					//Create a manual shot record
					CPlayerrecord *localrecord = &m_PlayerRecords[Entity->index];
					ShotMutex.Lock();
					for (int i = 1; i <= MAX_PLAYERS; i++)
					{
						auto _Entity = Interfaces::ClientEntList->GetBaseEntity(i);
						ManualFireShotInfo.Entities[i] = _Entity;
						if (_Entity && _Entity->IsPlayer() && _Entity->GetAlive())
						{
							ManualFireShotInfo.Entities[i] = _Entity;
							memcpy(&ManualFireShotInfo.BoneMatrixes[0][i], _Entity->GetCachedBoneData()->Base(), _Entity->GetCachedBoneData()->Count() * sizeof(matrix3x4_t));
							ManualFireShotInfo.m_iNumBones[i] = _Entity->GetCachedBoneData()->Count();
							ManualFireShotInfo.m_EntityToWorldTransform[i] = _Entity->EntityToWorldTransform();
							ManualFireShotInfo.m_NetOrigin[i] = _Entity->GetNetworkOrigin();
							ManualFireShotInfo.m_flBodyYaw[i] = _Entity->GetPoseParameterScaled(11);
							ManualFireShotInfo.m_flAbsYaw[i] = _Entity->GetAbsAnglesDirect().y;
							auto currecord = _Entity->ToPlayerRecord()->GetCurrentRecord();
							if (currecord)
								ManualFireShotInfo.m_flSpeed[i] = currecord->m_Velocity.Length();
							else
								ManualFireShotInfo.m_flSpeed[i] = _Entity->GetVelocity().Length();
							auto tickrecord = _Entity->ToPlayerRecord()->m_TargetRecord;
							if (tickrecord)
								ManualFireShotInfo.m_flNetEyeYaw[i] = tickrecord->m_EyeYaw;
							else
								ManualFireShotInfo.m_flNetEyeYaw[i] = _Entity->GetEyeAngles().y;

						}
						else
							ManualFireShotInfo.Entities[i] = nullptr;
					}
					FixShootPosition(CurrentUserCmd.cmd->viewangles, true);
					ManualFireShotInfo.m_vecLocalEyePosition = Entity->Weapon_ShootPosition();
					ManualFireShotInfo.m_flLastManualShotRealTime = Interfaces::Globals->realtime;
					ManualFireShotInfo.m_pEntityHit = nullptr;
					ManualFireShotInfo.m_iTickHitWall = 0;
					ManualFireShotInfo.m_iTickHitPlayer = 0;
					ManualFireShotInfo.m_iActualHitbox = HITBOX_MAX;
					ManualFireShotInfo.m_iActualHitgroup = HITGROUP_GENERIC;
					ShotMutex.Unlock();
				}
			}
		}

		// we have a valid target
		if (_TargetEntity)
		{
			//Create a shot record for this target
			CPlayerrecord* _playerRecord = g_LagCompensation.GetPlayerrecord(_TargetEntity);
	
			CTickrecord* _record = _playerRecord->m_TargetRecord ? _playerRecord->m_TargetRecord : _playerRecord->GetCurrentRecord();
			if (_record)
			{
#ifdef _DEBUG
				if (_playerRecord->m_TargetRecord == nullptr)
					DebugBreak();
				Interfaces::DebugOverlay->AddTextOverlay(_record->m_NetOrigin, 3.0f, "%f %f %f", _record->m_NetOrigin.x, _record->m_NetOrigin.y, _record->m_NetOrigin.z);
				_TargetEntity->DrawHitboxesFromCache(ColorRGBA(255, 0, 0, 255), 4.1f, (matrix3x4_t*)_record->m_PlayerBackup.CachedBoneMatrices);
				//printf("network origin: %f %f %f | simtime: %f | correctedsimtime: %f\n", _TargetEntity->GetNetworkOrigin().x, _TargetEntity->GetNetworkOrigin().y, _TargetEntity->GetNetworkOrigin().z, _TargetEntity->GetSimulationTime(), _record->m_flCorrectedSimulationTime);
#endif
				CShotrecord* _shotRecord = new CShotrecord(LocalPlayer.Entity, _TargetEntity, _record);
				_shotRecord->m_vecLocalEyePos = LocalPlayer.ShootPosition;
#ifdef _DEBUG
				Vector vecDir;
				QAngle angles = CurrentUserCmd.cmd->viewangles + LocalPlayer.Entity->GetPunch() * weapon_recoil_scale.GetVar()->GetFloat();
				AngleVectors(angles, &vecDir);
				//Interfaces::DebugOverlay->AddLineOverlay(_shotRecord->m_vecLocalEyePos, _shotRecord->m_vecLocalEyePos + vecDir * 8192.0f, 255, 0, 0, true, 4.0f);
				//Interfaces::DebugOverlay->AddBoxOverlay(_shotRecord->m_vecLocalEyePos, Vector(-0.5f, -0.5f, -0.5f), Vector(0.5f, 0.5f, 0.5f), angles, 255, 0, 0, 255, 4.0f);
#endif
				CPlayerrecord *localrecord = &m_PlayerRecords[Entity->index];
				ShotMutex.Lock();
				if (localrecord->ShotRecords.size() > 7)
				{
					//Delete excess records
					delete localrecord->ShotRecords.front();
					localrecord->ShotRecords.pop_front();
				}
				localrecord->ShotRecords.push_back(_shotRecord);
				ShotMutex.Unlock();

#if 0
				//This is here to test ray/obb intersection enter and exit points
				std::vector<CSphere>spheres;
				std::vector<COBB>obbs;
				_TargetEntity->SetupHitboxes(spheres, obbs, (matrix3x4_t*)_TargetEntity->GetCachedBoneData()->Base());
				Ray_t ray;
				Vector vecDir;
				AngleVectors(CurrentUserCmd.cmd->viewangles, &vecDir);
				VectorNormalizeFast(vecDir);
				ray.Init(_shotRecord->m_vecLocalEyePos, _shotRecord->m_vecLocalEyePos + vecDir * 4096.f);
				trace_t tr;
				Vector exitpoint;
				TRACE_HITBOX(_TargetEntity, ray, tr, spheres, obbs, &exitpoint);
				if (tr.m_pEnt)
				{
					Interfaces::DebugOverlay->AddBoxOverlay(tr.endpos, Vector(-0.5, -0.5, -0.5), Vector(0.5, 0.5, 0.5), angZero, 255, 255, 0, 255, 8.0f);
					Interfaces::DebugOverlay->AddBoxOverlay(exitpoint, Vector(-0.85, -0.85, -0.85), Vector(0.85, 0.85, 0.85), angZero, 0, 0, 255, 255, 8.0f);
				}
#endif
			}
		}
	}

	return _TargetEntity;
}

void MyPlayer::OnCreateMove()
{
	if (Entity)
	{
		IsAlive = Entity->GetAlive() && Entity->GetHealth() > 0.0f;
		//bIsInGhostCamera = false;//!DisableAllChk.Checked && (Interfaces::Globals->realtime < flMissCameraEndTime || KeyboardKey::Pressed[GhostCamKeyTxt.iValue]);
		//bFakeWalking = !DisableAllChk.Checked && IsAlive && !bIsInGhostCamera && FakeWalkKeyTxt.iValue > 0 && (KeyboardKey::Pressed[FakeWalkKeyTxt.iValue]) && Entity->GetGroundEntity() /*&& (cmd->forwardmove != 0.0f || cmd->sidemove != 0.0f)*/;
		EyePosition = Entity->GetEyePosition();
		ShootPosition = Entity->Weapon_ShootPosition();
		CurrentWeapon = Entity->GetWeapon();
	}
	else
	{
		IsAlive = false;
		WeaponWillFireBurstShotThisTick = false;
	}

	if (!IsAlive)
	{
		CreateMoveVars.bShouldFakeUnDuck = false;
		CreateMoveVars.bShouldFakeDuck = false;
		CreateMoveVars.iUnduckedChokedCount = 0;
		CreateMoveVars.iDuckedChokedCount = 0;
		ResetWeaponVars();
		WeaponWillFireBurstShotThisTick = false;
	}
}

MyPlayer::MyPlayer()
{
	Entity = nullptr;
	CurrentEyeAngles = QAngle(0, 0, 0);
	LowerBodyYaw = 0;
	bInPrediction = false;
	bInPrediction_Start = false;
	m_PredictionAnimationEventQueue.clear();
	m_UserCommandMutex.Lock();
	m_vecUserCommands.clear();
	m_UserCommandMutex.Unlock();
}

void MyPlayer::ClearVariables()
{
	memset(this, 0, (DWORD)&this->m_ShotResults - (DWORD)this);
}

bool MyPlayer::IsManuallyFiring(CBaseCombatWeapon *weapon) const
{
	return weapon && (CurrentUserCmd.cmd_backup.buttons & IN_ATTACK || (CurrentUserCmd.cmd_backup.buttons & IN_ATTACK2 && WeaponVars.IsRevolver));
}

CBaseEntity* MyPlayer::Get(MyPlayer* dest)
{
	PlayerMutex.Lock();
	auto LP = Interfaces::EngineClient->GetLocalPlayer();
	CBaseEntity* pNewPlayer = LP ? Interfaces::ClientEntList->GetBaseEntity(LP) : nullptr;
	if (pNewPlayer != dest->Entity)
	{
		OnInvalid();
		if (pNewPlayer)
		{
			dest->ClearVariables();
		}
		dest->Entity = pNewPlayer;
	}
	PlayerMutex.Unlock();
	return pNewPlayer;
}

CBaseEntity* MyPlayer::GetLocalPlayer()
{
	return Interfaces::ClientEntList->GetBaseEntity(Interfaces::EngineClient->GetLocalPlayer());
}

void MyPlayer::DrawTextInFrontOfPlayer(float flStayTime, float Z, char *text, ...)
{
	va_list args;
	va_start(args, text);

	char buffer[512];
	vsprintf(buffer, text, args);

	Vector vForward;
	QAngle realangs;
	Interfaces::EngineClient->GetViewAngles(realangs);
	AngleVectors(realangs, &vForward);
	Vector destorigin = Entity->GetLocalOrigin();
	destorigin.z += Z;
	destorigin += vForward * 250.0f;
	Interfaces::DebugOverlay->AddTextOverlay(destorigin, flStayTime, buffer);

	va_end(args);
}

void MyPlayer::ResetWeaponVars()
{
	WeaponVars.LegitRCSIndex = 0;
	WeaponVars.dbNextRCSIndexChangeTime = 0.0;
	WeaponVars.dbLastRCS = 0.0;
	WeaponVars.totalrecoilsubtracted = angZero;
	WeaponVars.totalrecoiladded = angZero;
	WeaponVars.recoilslope = angZero;
}

void MyPlayer::SnapAttachmentsToCurrentPosition()
{
	//Ugh, now we need to make the attachments of the player match the new position
	//Attachments are the items currently holstered

	//Do nothing, don't seem to need to do this right now anymore
	LocalPlayer.NetvarMutex.Lock();

	auto cache = Entity->GetCachedBoneData();
	matrix3x4_t * const original_matrix = (matrix3x4_t* const)RealAngleMatrix;
	matrix3x4_t OriginalMatrixInverted, EndMatrix;
	matrix3x4_t final_matrix[MAXSTUDIOBONES];
	int numbones = cache->Count();

	//Transform the old matrix to the current player position for aesthetic reasons
	MatrixInvert(RealAngleEntityToWorldTransform, OriginalMatrixInverted);
	MatrixCopy(RealAngleEntityToWorldTransform, EndMatrix);

	//Get a positional matrix from the current position
	PositionMatrix(Entity->GetAbsOriginDirect(), EndMatrix);

	//Interfaces::DebugOverlay->AddBoxOverlay(Entity->GetAbsOriginDirect(), Vector(0, 0, 0), Vector(5, 5, 5), angZero, 255, 0, 0, 255, 0.05f);

	//Get a relative transform
	matrix3x4_t TransformedMatrix;
	ConcatTransforms(EndMatrix, OriginalMatrixInverted, TransformedMatrix);

	for (int i = 0; i < numbones; i++)
	{
		//Now concat the original matrix with the rotated one
		ConcatTransforms(TransformedMatrix, original_matrix[i], final_matrix[i]);
											//old matrix		dest new matrix
	}

	//Entity->DrawHitboxesFromCache(ColorRGBA(255, 0, 0, 255), 0.05f, final_matrix);

	Entity->SetupBones_AttachmentHelper(Entity->GetModelPtr(), (matrix3x4a_t*)final_matrix);
	LocalPlayer.NetvarMutex.Unlock();
}

float MyPlayer::GetSwaySpeed()
{
	auto& var = variable::get();

	if (!var.ragebot.b_antiaim)
		return 0.0f;

	//When running full speed, this can provide a bigger head displacement
	//if (Entity->GetMaxDesyncMultiplier() == 0.5f)
	//	return false;

	const float speed = Entity->GetVelocity().Length();

	if (Entity->IsInAir())
	{
		if (!var.ragebot.in_air.b_apply_sway)
			return 0.0f;

		return var.ragebot.in_air.f_sway_speed;
	}
	else if (LocalPlayer.bFakeWalking)
	{
		if (var.ragebot.minwalking.b_desync)
		{
			if (!var.ragebot.minwalking.b_apply_sway)
				return 0.0f;

			return var.ragebot.minwalking.f_sway_speed;
		}
	}
	else if (speed < 1.2f)
	{
		if (var.ragebot.standing.b_desync)
		{
			if (!var.ragebot.standing.b_apply_sway)
				return 0.0f;

			return var.ragebot.standing.f_sway_speed;
		}
	}
	else if (var.ragebot.moving.b_desync)
	{
		if (!var.ragebot.moving.b_apply_sway)
			return 0.0f;

		return var.ragebot.moving.f_sway_speed;
	}

	return 0.0f;
}

bool MyPlayer::IsSwaying()
{
	auto& var = variable::get();

	if (!var.ragebot.b_antiaim)
		return false;

	//When running full speed, this can provide a bigger head displacement
	//if (Entity->GetMaxDesyncMultiplier() == 0.5f)
	//	return false;

	const float speed = Entity->GetVelocity().Length();

	if (Entity->IsInAir())
	{
		if (var.ragebot.in_air.b_desync)
		{
			return var.ragebot.in_air.b_apply_sway;
		}
	}
	else if (LocalPlayer.bFakeWalking)
	{
		if (var.ragebot.minwalking.b_desync)
		{
			return var.ragebot.minwalking.b_apply_sway;
		}
	}
	else if (speed < 1.2f)
	{
		if (var.ragebot.standing.b_desync)
		{
			return var.ragebot.standing.b_apply_sway;
		}
	}
	else if (var.ragebot.moving.b_desync)
	{
		return var.ragebot.moving.b_apply_sway;
	}

	return false;
}

bool MyPlayer::ShouldDelaySway()
{
	auto& var = variable::get();

	if (!var.ragebot.b_antiaim)
		return false;

	//When running full speed, this can provide a bigger head displacement
	//if (Entity->GetMaxDesyncMultiplier() == 0.5f)
	//	return false;

	const float speed = Entity->GetVelocity().Length();

	if (Entity->IsInAir())
	{
		if (var.ragebot.in_air.b_desync)
		{
			if (var.ragebot.in_air.b_apply_sway)
				return var.ragebot.in_air.b_sway_wait;
		}
	}
	else if (LocalPlayer.bFakeWalking)
	{
		if (var.ragebot.minwalking.b_desync)
		{
			if (var.ragebot.minwalking.b_apply_sway)
				return var.ragebot.minwalking.b_sway_wait;
		}
	}
	else if (speed < 1.2f)
	{
		if (var.ragebot.standing.b_desync)
		{
			if (var.ragebot.standing.b_apply_sway)
				return var.ragebot.standing.b_sway_wait;
		}
	}
	else if (var.ragebot.moving.b_desync)
	{
		if (var.ragebot.moving.b_apply_sway)
			return var.ragebot.moving.b_sway_wait;
	}

	return false;
}

float MyPlayer::GetSwayDelay()
{
	auto& var = variable::get();

	if (!var.ragebot.b_antiaim)
		return 0.0f;

	//When running full speed, this can provide a bigger head displacement
	//if (Entity->GetMaxDesyncMultiplier() == 0.5f)
	//	return false;

	const float speed = Entity->GetVelocity().Length();

	if (Entity->IsInAir())
	{
		if (var.ragebot.in_air.b_desync)
		{
			if (var.ragebot.in_air.b_apply_sway && var.ragebot.in_air.b_sway_wait)
				return SORandomFloat(var.ragebot.in_air.f_sway_min, var.ragebot.in_air.f_sway_max);
		}
	}
	else if (LocalPlayer.bFakeWalking)
	{
		if (var.ragebot.minwalking.b_desync)
		{
			if (var.ragebot.minwalking.b_apply_sway && var.ragebot.minwalking.b_sway_wait)
				return SORandomFloat(var.ragebot.minwalking.f_sway_min, var.ragebot.minwalking.f_sway_max);
		}
	}
	else if (speed < 1.2f)
	{
		if (var.ragebot.standing.b_desync)
		{
			if (var.ragebot.standing.b_apply_sway && var.ragebot.standing.b_sway_wait)
				return SORandomFloat(var.ragebot.standing.f_sway_min, var.ragebot.standing.f_sway_max);
		}
	}
	else if (var.ragebot.moving.b_desync)
	{
		if (var.ragebot.moving.b_apply_sway && var.ragebot.moving.b_sway_wait)
			return SORandomFloat(var.ragebot.moving.f_sway_min, var.ragebot.moving.f_sway_max);
	}

	return 0.0f;
}

// true = left desync | false = right desync
bool MyPlayer::GetDesyncStyle()
{
	auto& var = variable::get();

	if (!var.ragebot.b_antiaim)
		return false;

	//When running full speed, this can provide a bigger head displacement
	//if (Entity->GetMaxDesyncMultiplier() == 0.5f)
	//	return false;

	const float speed = Entity->GetVelocity().Length();

	if (Entity->IsInAir())
	{
		if (var.ragebot.in_air.b_desync)
		{
			if (var.ragebot.in_air.b_apply_sway)
				return g_AntiAim.m_bSwayDesyncStyle;

			if (g_AntiAim.m_bManualSwapDesyncDir)
				return var.ragebot.in_air.i_desync_style != 1;

			return var.ragebot.in_air.i_desync_style;
		}
	}
	else if (LocalPlayer.bFakeWalking)
	{
		if (var.ragebot.minwalking.b_desync)
		{
			if (var.ragebot.minwalking.b_apply_sway)
				return g_AntiAim.m_bSwayDesyncStyle;

			if (g_AntiAim.m_bManualSwapDesyncDir)
				return var.ragebot.minwalking.i_desync_style != 1;

			return var.ragebot.minwalking.i_desync_style;
		}
	}
	else if (speed < 1.2f)
	{
		if (var.ragebot.standing.b_desync)
		{
			if (var.ragebot.standing.b_apply_sway)
				return g_AntiAim.m_bSwayDesyncStyle;

			if (g_AntiAim.m_bManualSwapDesyncDir)
				return var.ragebot.standing.i_desync_style != 1;

			return var.ragebot.standing.i_desync_style;
		}
	}
	else if (var.ragebot.moving.b_desync)
	{
		if (var.ragebot.moving.b_apply_sway)
			return g_AntiAim.m_bSwayDesyncStyle;

		if (g_AntiAim.m_bManualSwapDesyncDir)
			return var.ragebot.moving.i_desync_style != 1;

		return var.ragebot.moving.i_desync_style;
	}

	return false;
}

bool MyPlayer::Config_IsDesyncing() const
{
	if (!variable::get().ragebot.b_antiaim || !Entity)
		return false;

	float speed = Entity->GetVelocity().Length();

	if (Entity->IsInAir())
		return variable::get().ragebot.in_air.b_desync;
	else if (LocalPlayer.bFakeWalking)
		return variable::get().ragebot.minwalking.b_desync;
	else if (speed < 1.2f)
		return variable::get().ragebot.standing.b_desync;

	return variable::get().ragebot.moving.b_desync;
}

bool MyPlayer::Config_IsJitteringYaw() const
{
	if (!variable::get().ragebot.b_antiaim || !Entity)
		return false;

	const float speed = Entity->GetVelocity().Length();

	if (Entity->IsInAir())
		return variable::get().ragebot.in_air.b_jitter;
	else if (LocalPlayer.bFakeWalking)
		return variable::get().ragebot.minwalking.b_jitter;
	else if (speed < 1.2f)
		return variable::get().ragebot.standing.b_jitter;

	return variable::get().ragebot.moving.b_jitter;
}

float MyPlayer::Config_GetDesyncAmount() const
{
	if (!variable::get().ragebot.b_antiaim || !Entity)
		return 0.0f;

	float speed = Entity->GetVelocity().Length();

	if (Entity->IsInAir())
	{
		if (variable::get().ragebot.in_air.b_desync)
		{
			//if (speed > 110.0f)
			//	return variable::get().ragebot.in_air.f_desync_amt_fast;

			float amt = variable::get().ragebot.in_air.f_desync_amt;

			if (variable::get().ragebot.in_air.b_apply_sway)
				amt = clamp(amt, -120.0f, 120.0f);

			return amt;
		}
	}
	else if (LocalPlayer.bFakeWalking)
	{
		if (variable::get().ragebot.minwalking.b_desync)
		{
			float amt = variable::get().ragebot.minwalking.f_desync_amt;

			if (variable::get().ragebot.minwalking.b_apply_sway)
				amt = clamp(amt, -120.0f, 120.0f);

			return amt;
		}
	}
	else if (speed < 1.2f)
	{
		if (variable::get().ragebot.standing.b_desync)
		{
			float amt = variable::get().ragebot.standing.f_desync_amt;

			if (variable::get().ragebot.standing.b_apply_sway)
				amt = clamp(amt, -120.0f, 120.0f);

			return amt;
		}
	}
	else if (variable::get().ragebot.moving.b_desync)
	{
		//if (speed > 110.0f)
		//	return variable::get().ragebot.moving.f_desync_amt_fast;

		float amt = variable::get().ragebot.moving.f_desync_amt;

		if (variable::get().ragebot.moving.b_apply_sway)
			amt = clamp(amt, -120.0f, 120.0f);

		return amt;
	}

	return 0.0f;
}

float MyPlayer::Config_GetYawJitter() const
{
	if (!variable::get().ragebot.b_antiaim || !Entity)
		return 0.0f;

	const float speed = Entity->GetVelocity().Length();

	if (Entity->IsInAir())
	{
		if (variable::get().ragebot.in_air.b_jitter)
		{
			//if (speed > 80.f)
			//	return variable::get().ragebot.in_air.f_jitter_fast;

			return variable::get().ragebot.in_air.f_jitter;
		}
	}
	else if (LocalPlayer.bFakeWalking)
	{
		if (variable::get().ragebot.minwalking.b_jitter)
		{
			return variable::get().ragebot.minwalking.f_jitter;
		}
	}
	else if (speed < 1.2f)
	{
		if (variable::get().ragebot.standing.b_jitter)
		{
			return variable::get().ragebot.standing.f_jitter;
		}
	}
	else if (variable::get().ragebot.moving.b_jitter)
	{
		//if (speed > 110.f)
		//	return variable::get().ragebot.moving.f_jitter_fast;

		return variable::get().ragebot.moving.f_jitter;
	}

	return 0.0f;
}

float MyPlayer::Config_GetLBYDelta() const
{
	if (!variable::get().ragebot.b_antiaim)
		return 0.0f;

	const float speed = Entity->GetVelocity().Length();

	if (Entity->IsInAir())
	{
		return variable::get().ragebot.in_air.f_lby_delta;
	}
	else if (LocalPlayer.bFakeWalking)
	{
		return variable::get().ragebot.minwalking.f_lby_delta;
	}
	else if (speed < 1.2f)
	{
		return variable::get().ragebot.standing.f_lby_delta;
	}

	return variable::get().ragebot.moving.f_lby_delta;
}

int MyPlayer::Config_GetPitch() const
{
	if (!variable::get().ragebot.b_antiaim)
		return 0;

	const float speed = Entity->GetVelocity().Length();

	if (Entity->IsInAir() && variable::get().ragebot.in_air.i_pitch > 0)
		return variable::get().ragebot.in_air.i_pitch;
	else if (LocalPlayer.bFakeWalking && variable::get().ragebot.minwalking.i_pitch > 0)
		return variable::get().ragebot.minwalking.i_pitch;
	else if (speed < 1.2f && variable::get().ragebot.standing.i_pitch > 0)
		return variable::get().ragebot.standing.i_pitch;
	else if (speed >= 1.2f && variable::get().ragebot.moving.i_pitch > 0)
		return variable::get().ragebot.moving.i_pitch;

	return 0;
}

int MyPlayer::Config_GetYaw() const
{
	if (!variable::get().ragebot.b_antiaim)
		return 0;

	const float speed = Entity->GetVelocity().Length();

	if (Entity->IsInAir() && variable::get().ragebot.in_air.i_yaw > 0)
		return variable::get().ragebot.in_air.i_yaw;
	else if (LocalPlayer.bFakeWalking && variable::get().ragebot.minwalking.i_yaw > 0)
		return variable::get().ragebot.minwalking.i_yaw;
	else if (speed < 1.2f && variable::get().ragebot.standing.i_yaw > 0)
		return variable::get().ragebot.standing.i_yaw;
	else if (speed >= 1.2f && variable::get().ragebot.moving.i_yaw > 0)
		return variable::get().ragebot.moving.i_yaw;

	return 0;
}

float MyPlayer::Config_GetYawAdd() const
{
	if (!variable::get().ragebot.b_antiaim)
		return 0.f;

	const float speed = Entity->GetVelocity().Length();

	if (Entity->IsInAir() && variable::get().ragebot.in_air.i_yaw > 0)
		return variable::get().ragebot.in_air.f_yaw_offset;
	else if (LocalPlayer.bFakeWalking && variable::get().ragebot.minwalking.i_yaw > 0)
		return variable::get().ragebot.minwalking.f_yaw_offset;
	else if (speed < 1.2f && variable::get().ragebot.standing.i_yaw > 0)
		return variable::get().ragebot.standing.f_yaw_offset;
	else if (speed >= 1.2f && variable::get().ragebot.moving.i_yaw > 0)
		return variable::get().ragebot.moving.f_yaw_offset;

	return 0.f;
}

float MyPlayer::Config_GetYawSpin() const
{
	const float speed = Entity->GetVelocity().Length();

	if (Entity->IsInAir() && variable::get().ragebot.in_air.i_yaw == 3)
		return variable::get().ragebot.in_air.f_spin_speed;
	else if (LocalPlayer.bFakeWalking && variable::get().ragebot.minwalking.i_yaw == 3)
		return variable::get().ragebot.minwalking.f_spin_speed;
	else if (speed < 1.2f && variable::get().ragebot.standing.i_yaw == 3)
		return variable::get().ragebot.standing.f_spin_speed;
	else if (speed >= 1.2f && variable::get().ragebot.moving.i_yaw == 3)
		return variable::get().ragebot.moving.f_spin_speed;

	return 0.f;
}

float MyPlayer::Config_GetCustomPitch() const
{
	const float speed = Entity->GetVelocity().Length();

	if (Entity->IsInAir() && variable::get().ragebot.in_air.i_pitch == 6)
		return variable::get().ragebot.in_air.f_custom_pitch;
	else if (LocalPlayer.bFakeWalking && variable::get().ragebot.minwalking.i_pitch == 6)
		return variable::get().ragebot.minwalking.f_custom_pitch;
	else if (speed < 1.2f && variable::get().ragebot.standing.i_pitch == 6)
		return variable::get().ragebot.standing.f_custom_pitch;
	else if (speed >= 1.2f && variable::get().ragebot.moving.i_pitch == 6)
		return variable::get().ragebot.moving.f_custom_pitch;

	return 0.f;
}

bool MyPlayer::Config_IsAntiaiming() const
{
	return variable::get().ragebot.b_antiaim && Config_IsDesyncing() || Config_GetPitch() > 0 || Config_GetYaw() > 0;
}

bool MyPlayer::Config_IsFreestanding() const
{
	if (!variable::get().ragebot.b_antiaim)
		return false;

	const float speed = Entity->GetVelocity().Length();

	if (Entity->IsInAir())
		return variable::get().ragebot.in_air.b_apply_freestanding;
	else if (LocalPlayer.bFakeWalking)
		return variable::get().ragebot.minwalking.b_apply_freestanding;
	else if (speed < 1.2f)
		return variable::get().ragebot.standing.b_apply_freestanding;
	else if (speed >= 1.2f)
		return variable::get().ragebot.moving.b_apply_freestanding;

	return false;
}

bool MyPlayer::Config_IsWallDetecting() const
{
	if (!variable::get().ragebot.b_antiaim)
		return false;

	const float speed = Entity->GetVelocity().Length();

	if (Entity->IsInAir())
		return variable::get().ragebot.in_air.b_apply_walldtc;
	else if (LocalPlayer.bFakeWalking)
		return variable::get().ragebot.minwalking.b_apply_walldtc;
	else if (speed < 1.2f)
		return variable::get().ragebot.standing.b_apply_walldtc;
	else if (speed >= 1.2f)
		return variable::get().ragebot.moving.b_apply_walldtc;

	return false;
}

bool MyPlayer::Config_IsFakelagging(int* move_type) const
{
	if (!variable::get().ragebot.b_antiaim)
		return false;

	const float speed = Entity->GetVelocity().Length();

	if (Entity->IsInAir())
	{
		if(move_type != nullptr)
			*move_type = 3;
		return variable::get().ragebot.in_air.fakelag.b_enabled;
	}
	else if (LocalPlayer.bFakeWalking)
	{
		if (move_type != nullptr)
			*move_type = 1;
		return variable::get().ragebot.minwalking.fakelag.b_enabled;
	}
	else if (speed < 1.2f)
	{
		if (move_type != nullptr)
			*move_type = 0;
		return variable::get().ragebot.standing.fakelag.b_enabled;
	}
	else if (speed >= 1.2f)
	{
		if (move_type != nullptr)
			*move_type = 2;
		return variable::get().ragebot.moving.fakelag.b_enabled;
	}

	if (move_type != nullptr)
		*move_type = -1;

	return false;
}
int MyPlayer::Config_GetFakelagMode() const
{
	if (!LocalPlayer.Config_IsFakelagging())
		return 0;

	const float speed = Entity->GetVelocity().Length();

	if (Entity->IsInAir())
		return variable::get().ragebot.in_air.fakelag.i_mode;
	else if (LocalPlayer.bFakeWalking)
		return variable::get().ragebot.minwalking.fakelag.i_mode;
	else if (speed < 1.2f)
		return variable::get().ragebot.standing.fakelag.i_mode;
	else if (speed >= 1.2f)
		return variable::get().ragebot.moving.fakelag.i_mode;

	return 0;
}

int MyPlayer::Config_GetFakelagMin() const
{
	if (!LocalPlayer.Config_IsFakelagging())
		return 1;

	const float speed = Entity->GetVelocity().Length();

	if (Entity->IsInAir())
		return variable::get().ragebot.in_air.fakelag.i_min_ticks;
	else if (LocalPlayer.bFakeWalking)
		return variable::get().ragebot.minwalking.fakelag.i_min_ticks;
	else if (speed < 1.2f)
		return variable::get().ragebot.standing.fakelag.i_min_ticks;
	else if (speed >= 1.2f)
		return variable::get().ragebot.moving.fakelag.i_min_ticks;

	return 1;
}

int MyPlayer::Config_GetFakelagMax() const
{
	if (!LocalPlayer.Config_IsFakelagging())
		return MAX_TICKS_TO_CHOKE;

	const float speed = Entity->GetVelocity().Length();

	if (Entity->IsInAir())
		return variable::get().ragebot.in_air.fakelag.i_max_ticks;
	else if (LocalPlayer.bFakeWalking)
		return variable::get().ragebot.minwalking.fakelag.i_max_ticks;
	else if (speed < 1.2f)
		return variable::get().ragebot.standing.fakelag.i_max_ticks;
	else if (speed >= 1.2f)
		return variable::get().ragebot.moving.fakelag.i_max_ticks;

	return MAX_TICKS_TO_CHOKE;
}

int MyPlayer::Config_GetFakelagStatic() const
{
	if (!LocalPlayer.Config_IsFakelagging())
		return 1;

	const float speed = Entity->GetVelocity().Length();

	if (Entity->IsInAir())
		return variable::get().ragebot.in_air.fakelag.i_static_ticks;
	else if (LocalPlayer.bFakeWalking)
		return variable::get().ragebot.minwalking.fakelag.i_static_ticks;
	else if (speed < 1.2f)
		return variable::get().ragebot.standing.fakelag.i_static_ticks;
	else if (speed >= 1.2f)
		return variable::get().ragebot.moving.fakelag.i_static_ticks;

	return 1;
}

bool MyPlayer::Config_IsDisruptingFakelag() const
{
	if (!LocalPlayer.Config_IsFakelagging())
		return false;

	const float speed = Entity->GetVelocity().Length();

	if (Entity->IsInAir())
		return variable::get().ragebot.in_air.fakelag.b_disrupt;
	else if (LocalPlayer.bFakeWalking)
		return variable::get().ragebot.minwalking.fakelag.b_disrupt;
	else if (speed < 1.2f)
		return variable::get().ragebot.standing.fakelag.b_disrupt;
	else if (speed >= 1.2f)
		return variable::get().ragebot.moving.fakelag.b_disrupt;

	return false;
}
float MyPlayer::Config_GetFakelagDisruptChance() const
{
	if (!LocalPlayer.Config_IsFakelagging())
		return 0.f;

	const float speed = Entity->GetVelocity().Length();

	if (Entity->IsInAir())
		return variable::get().ragebot.in_air.fakelag.f_disrupt_chance;
	else if (LocalPlayer.bFakeWalking)
		return variable::get().ragebot.minwalking.fakelag.f_disrupt_chance;
	else if (speed < 1.2f)
		return variable::get().ragebot.standing.fakelag.f_disrupt_chance;
	else if (speed >= 1.2f)
		return variable::get().ragebot.moving.fakelag.f_disrupt_chance;

	return 0.f;
}

bool MyPlayer::IsAllowedUntrusted() const
{
	if (IsInCompetitive)
	{
		if (variable::get().ui.b_allow_untrusted)
			return true;
	}
	else
	{
		return true;
	}

	return false;
}

std::string ShotResult::get_miss_reason(bool fake_fire) const
{
	std::string info = {};

	//decrypts(0)
	if (fake_fire)
	{
		info += XorStr(" fake fire ");
	}
	if (m_MissFlags & MISS_FROM_SPREAD)
	{
		info += XorStr(" spread ");
	}
	if (m_MissFlags & MISS_FROM_BAD_HITGROUP)
	{
		info += XorStr(" bad hitgroup ");
	}
	if (m_MissFlags & MISS_FROM_BADRESOLVE)
	{
		info += XorStr(" bad resolve ");
	}
	if (m_MissFlags & MISS_RAY_OFF_TARGET)
	{
		info += XorStr(" off target ");
	}
	if (m_MissFlags & MISS_AUTOWALL)
	{
		info += XorStr(" autowall ");
	}
	if (m_MissFlags & MISS_SHOULDNT_HAVE_MISSED)
	{
		info += XorStr(" shouldn't have missed ");
	}
	if (m_MissFlags & HIT_PLAYER)
	{
		info += XorStr(" hit player ");
	}
	if (m_MissFlags & HIT_IS_RESOLVED)
	{
		info += XorStr(" hit resolved ");
	}
	if (m_bCountedMiss)
	{
		info += XorStr(" count as miss ");
	}
	//encrypts(0)
	
	return info;
	
}
