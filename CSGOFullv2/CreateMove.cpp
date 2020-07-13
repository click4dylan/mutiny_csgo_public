#include "precompiled.h"
#include "CreateMove.h"
#include "CSGO_HX.h"
#include "Overlay.h"
#include "Targetting.h"
#include "AutoWall.h"
#include "ConVar.h"
#include "Netchan.h"
#include "VTHook.h"
#include "LocalPlayer.h"
#include "ESP.h"
#include <sstream>
#include "ServerSide.h"
#include "FarESP.h"
#include "Globals.h"
#include "Draw.h"
#include "Keys.h"
#include "Aimbot_imi.h"
#include "WeaponController.h"
#include "Removals.h"
#include "AntiAim.h"
#include "Assistance.h"
#include "Fakelag.h"
#include "CParallelProcessor.h"
#include "Reporting.h"
#include "UsedConvars.h"
#include "TickbaseExploits.h"
#include "AutoBone_imi.h"
#include "INetchannelInfo.h"
#include "Eventlog.h"
#include "WaypointSystem.h"

#include "Adriel/stdafx.hpp"
#include "Adriel/input.hpp"
#include "Adriel/nade_prediction.hpp"
#include "Adriel/name_changer.hpp"
#include "Adriel/clantag_changer.hpp"

extern int temp_fakelag_limit;

struct SortedPlayerNew
{
	SortedPlayerNew(CPlayerrecord *pcplayer, float dist)
	{
		pCPlayer = pcplayer, DistToCrosshair = dist;
	}
	CPlayerrecord *pCPlayer;
	float DistToCrosshair;
};

bool SortedPlayerIsCloserNew(SortedPlayerNew& a, SortedPlayerNew& b)
{
	return a.DistToCrosshair < b.DistToCrosshair;
}

void HookNetChannel();
CL_MoveFn CL_Move;
CreateMoveFn oCreateMove;
ModifiableUserCmd CurrentUserCmd;
CreateMoveVars_t CreateMoveVars;

//Attempt to prevent fall damage
void StaminaBug(CUserCmd* cmd)
{
	CBaseEntity* Entity = LocalPlayer.Entity;

	if (!(cmd->buttons & IN_DUCK))
		return;

	if (Entity->GetFlags() & FL_ONGROUND)
		return;
	
	const auto flVelocityZ = Entity->GetVelocity().z;
	if (flVelocityZ >= 0.f)
		return;

	bool oldonground = Entity->GetGroundEntity();

	/*
	Ray_t rRay;
	const auto vecOrigin = Entity->GetNetworkOrigin();
	rRay.Init(vecOrigin, vecOrigin + Vector(0, 0, -100.f));

	CTraceFilterTest tfFilter;
	tfFilter.pSkip = Entity;

	CGameTrace gtRay;
	Interfaces::EngineTrace->TraceRay(rRay, MASK_PLAYERSOLID, &tfFilter, &gtRay);

	std::cout << "v/s\t" << -flVelocityZ << "\tdis\t" << gtRay.fraction * 100.f << "\trls\t" << -(abs(flVelocityZ) * Interfaces::Globals->interval_per_tick + 9.0f) << std::endl;

	if (gtRay.fraction * 100.f < (abs(flVelocityZ) * Interfaces::Globals->interval_per_tick + 9.0f))
	{
		std::cout << "unducked" << std::endl;

		#if 0
		cmd->buttons &= ~IN_DUCK;

		LocalPlayer.BeginEnginePrediction(cmd);

		bool onground = Entity->GetGroundEntity();

		LocalPlayer.FinishEnginePrediction(cmd);

		cmd->buttons &= ~IN_DUCK;
#endif
	}
	*/

	LocalPlayer.CalledPlayerHurt = false;

	if (!oldonground)
	{
		LocalPlayer.BeginEnginePrediction(cmd);
		if (!Entity->GetGroundEntity())
		{
			//LocalPlayer.BeginEnginePrediction(cmd, false);
			
			//if (Entity->GetGroundEntity())
			//{
			//	cmd->buttons &= ~IN_DUCK;
			//	LocalPlayer.FinishEnginePrediction(cmd);
			//}
			//LocalPlayer.FinishEnginePrediction(cmd);
			cmd->buttons |= IN_DUCK;
		}
		else
		{
#ifdef _DEBUG
			AllocateConsole();
			printf("Before Unduck Hurt %i originZ %f\n", LocalPlayer.CalledPlayerHurt ? 1 : 0, Entity->GetNetworkOrigin().z);
			Interfaces::DebugOverlay->AddBoxOverlay(Entity->GetNetworkOrigin(), Vector(-0.2f, -0.2f, -0.2f), Vector(0.2f, 0.2f, 0.2f), angZero, 255, 0, 0, 255, 15.0f);
#endif
			LocalPlayer.FinishEnginePrediction(cmd);
			LocalPlayer.CalledPlayerHurt = false;
			cmd->buttons &= ~IN_DUCK;
			LocalPlayer.BeginEnginePrediction(cmd);
#ifdef _DEBUG
			printf("After Unduck Hurt %i originZ %f\n", LocalPlayer.CalledPlayerHurt ? 1 : 0, Entity->GetNetworkOrigin().z);
			Interfaces::DebugOverlay->AddBoxOverlay(Entity->GetNetworkOrigin(), Vector(-0.2f, -0.2f, -0.2f), Vector(0.2f, 0.2f, 0.2f), angZero, 0, 255, 0, 255, 15.0f);
#endif
			
		}
		LocalPlayer.FinishEnginePrediction(cmd);
	}
}

//Returns true if we should exit
bool PreCreateMove(void* ecx, float flInputSampleTime, CUserCmd* cmd, bool& returnvalue)
{
	START_PROFILING

	LocalPlayer.NetvarMutex.Lock();

	//printf("%#010x\n", (DWORD)g_ClientState->m_pNetChannel);

#ifdef _DEBUG
	if (input::get().is_key_down(VK_F1))
		Interfaces::DebugOverlay->ClearAllOverlays();

	//Dump class ID's in debug mode
	static ClassIDDumper* dumper = new ClassIDDumper;
	if (!dumper->HasDumpedClassIDs())
		dumper->DumpClassIDs();
#endif

	LocalPlayer.Get(&LocalPlayer);

	if (!LocalPlayer.Entity || !Interfaces::ClientEntList->PlayerExists(LocalPlayer.Entity))
	{
		LocalPlayer.OnInvalid();
		ClearAllPlayers();
		returnvalue = oCreateMove(ecx, flInputSampleTime, cmd); //true
		LocalPlayer.NetvarMutex.Unlock();
		END_PROFILING
		return true;
	}

	if (Interfaces::EngineClient->IsPlayingDemo())
	{
		LocalPlayer.OnInvalid();
		ClearAllPlayers();
		returnvalue = oCreateMove(ecx, flInputSampleTime, cmd); //true
		LocalPlayer.NetvarMutex.Unlock();
		END_PROFILING
		return true;
	}

	if (!Interfaces::EngineClient->IsInGame() || !Interfaces::EngineClient->IsConnected())
	{
		ClearAllPlayers();
		returnvalue = oCreateMove(ecx, flInputSampleTime, cmd); //false
		LocalPlayer.NetvarMutex.Unlock();
		END_PROFILING
		return true;
	}

	if (cmd->command_number == 0)
	{
		returnvalue = oCreateMove(ecx, flInputSampleTime, cmd); //true
		LocalPlayer.NetvarMutex.Unlock();
		END_PROFILING
		return true;
	}

	if (oCreateMove(ecx, flInputSampleTime, cmd))
		Interfaces::EngineClient->SetViewAngles(cmd->viewangles);

	auto gamerules = GetGamerules();
	LocalPlayer.IsInCompetitive = !gamerules || gamerules->IsValveServer();

	END_PROFILING
	return false;
}

extern bool DisableCLC_Move;

bool __fastcall Hooks::CreateMove(void* ecx, void* edx, float flInputSampleTime, CUserCmd* cmd)
{
	bool oValue;
	if (PreCreateMove(ecx, flInputSampleTime, cmd, oValue))
		return oValue;

	START_PROFILING

	AllocateConsole();
	//if (LocalPlayer.Entity)
	//	printf("%i\n", LocalPlayer.Entity->index);

	auto& var = variable::get();

#if defined _DEBUG || defined INTERNAL_DEBUG
	if (input::get().was_key_pressed('L'))
	{
		Interfaces::DebugOverlay->ClearAllOverlays();
		g_ClientState->m_nDeltaTick = -1;
	}
#endif

    //  make sure the first command in a choke cycle starts out at 0, it will get modified later if we shift the tickbase
	if (!g_Tickbase.m_iCustomTickbase[cmd->command_number % 150][1]) //if the second collumn is set to 1, that means we shifted tickbase on the last sendpacket, so we don't want to erase this
		g_Tickbase.m_iCustomTickbase[cmd->command_number % 150][0] = 0;

	if (cmd->weaponselect != 0)
		g_Tickbase.m_flDelayTickbaseShiftUntilThisTime = Interfaces::Globals->realtime + 1.0f;

	// handle manual antiaim
	g_AntiAim.PreCreateMove();

	CurrentUserCmd.cmd_backup  = *cmd;

	// set fast crouch
	if(LocalPlayer.IsAllowedUntrusted() && (var.misc.b_unlimited_duck || (input::get().is_key_down(var.misc.i_fakeduck_key) || LocalPlayer.IsFakeDucking)))
		cmd->buttons |= IN_BULLRUSH;

	// reset weapon vars and set basic values such as IsAlive, etc
	DWORD* fp;
	__asm mov fp, ebp 
	CurrentUserCmd.FramePointer = fp;
	CurrentUserCmd.Reset(cmd);
	LocalPlayer.GetAverageFrameTime();

#if 0
	HookNetChannel();

	if (g_ClientState->chokedcommands < variable::get().ragebot.i_fakelag_ticks)
	{
		bool *pbSendPacket = (bool*)(*fp - 0x1C);
		*pbSendPacket = false;
	}
	LocalPlayer.NetvarMutex.Unlock();
	Interfaces::MDLCache->EndLock();
	return oCreateMove(ecx, flInputSampleTime, cmd);
#endif


	g_Tickbase.OnCreateMove(); 

	bool _ForceNextPacketToSend = false;
	g_Info.SetShouldChoke(false);
	LocalPlayer.IsAlive = LocalPlayer.Entity && LocalPlayer.Entity->GetAlive();

	if (LocalPlayer.Entity)
	{
		// missed shot due to spread
		g_Ragebot.OutputMissedShots();

		LocalPlayer.OnCreateMove();

		// reset eye angles to the new cmd
		LocalPlayer.CurrentEyeAngles = LocalPlayer.FinalEyeAngles = g_Assistance.m_angStrafeAngle = g_Assistance.m_angPostStrafe = cmd->viewangles;

#ifdef USE_SERVER_SIDE
		pServerSide.OnCreateMove();
#endif
#ifdef USE_FAR_ESP
		if (var.visuals.pf_enemy.b_faresp)
			g_FarESP.OnCreateMove();
#endif

		//Backup animstate info that gamemovement changes
		LocalPlayer.m_bJumping = false;
		LocalPlayer.m_bDeploying = false;
		LocalPlayer.m_bPlantingBomb = false;
		if (CCSGOPlayerAnimState *animstate = LocalPlayer.Entity->ToPlayerRecord()->m_pAnimStateServer[ResolveSides::NONE])
		{
			LocalPlayer.m_bJumping = animstate->m_bJumping;
			LocalPlayer.m_bDeploying = animstate->m_bDeploying;
			LocalPlayer.m_bPlantingBomb = animstate->m_bPlantingBomb;
			if (cmd->buttons & (IN_ATTACK | IN_ATTACK2 | IN_ZOOM))
				LocalPlayer.UseDoubleTapHitchance = false;
		}

		// correct the local client time
		LocalPlayer.CorrectTickBase();

		LocalPlayer.WeaponWillFireBurstShotThisTick = LocalPlayer.CurrentWeapon && LocalPlayer.CurrentWeapon->GetItemDefinitionIndex() != WEAPON_C4 && !LocalPlayer.CurrentWeapon->IsKnife() && LocalPlayer.CurrentWeapon->WeaponHasBurst() && LocalPlayer.CurrentWeapon->GetBurstShotsRemaining() > 0 && TICKS_TO_TIME(LocalPlayer.Entity->GetTickBase()) >= LocalPlayer.CurrentWeapon->GetNextBurstShotTime();

		g_Tickbase.RunExploits(cmd);

		// cache current bone cache and/or lag compensate if needed
		g_LagCompensation.OnCreateMove();

		Waypoint_OnCreateMove();

		LocalPlayer.WeaponWillFireBurstShotThisTick = LocalPlayer.CurrentWeapon && LocalPlayer.CurrentWeapon->GetItemDefinitionIndex() != WEAPON_C4 && !LocalPlayer.CurrentWeapon->IsKnife() && LocalPlayer.CurrentWeapon->WeaponHasBurst() && LocalPlayer.CurrentWeapon->GetBurstShotsRemaining() > 0 && TICKS_TO_TIME(LocalPlayer.Entity->GetTickBase()) >= LocalPlayer.CurrentWeapon->GetNextBurstShotTime();

		nade_prediction::get().create_move(cmd);
		name_changer::get().create_move();

		// run bunnyhop and auto strafe
		if (LocalPlayer.IsAlive)
			g_Assistance.BunnyHop();

		CBaseCombatWeapon* _CurrentWeapon = LocalPlayer.Entity->GetWeapon();
		bool _AttackIsBlocked = g_Tickbase.m_bIncrementTicksAllowedForProcessing || LocalPlayer.Entity->GetWaitForNoAttack() || !_CurrentWeapon || (_CurrentWeapon->IsGun() && !_CurrentWeapon->GetClipOne());

		if (CurrentUserCmd.bShouldAutoScope && LocalPlayer.CurrentWeapon != nullptr)
		{
			if (var.ragebot.b_autoscope && LocalPlayer.WeaponVars.IsSniper && LocalPlayer.CurrentWeapon->GetZoomLevel() == 0)
			{
				CurrentUserCmd.cmd->buttons |= IN_ATTACK2;
				_AttackIsBlocked = true;
				//g_Tickbase.m_flDelayTickbaseShiftUntilThisTime = Interfaces::Globals->realtime + 0.5f;
			}
			CurrentUserCmd.bShouldAutoScope = false;
		}

		if (CurrentUserCmd.bShouldAutoStop)
		{
			if (var.ragebot.b_autostop && _CurrentWeapon && LocalPlayer.Entity->GetVelocity().Length2D() >= var.misc.f_minwalk_speed) //std::fmax(LocalPlayer.CurrentWeapon->GetMaxSpeed3() / 3.f, 0.001f))
				LocalPlayer.CorrectAutostop(CurrentUserCmd.cmd);

			CurrentUserCmd.bShouldAutoStop = false;
		}


		// start predicting local player
		if (LocalPlayer.IsAlive)
		{
			// setup weapon controller for tick
			g_WeaponController.OnCreateMove();

			//StaminaBug(cmd);

			g_Assistance.LagCrouch();

			g_Assistance.Teleport();

			// single scope
			if (_CurrentWeapon && _CurrentWeapon->GetNumZoomLevels() > 1)
			{
				if (var.misc.b_single_scope)
				{
					if (_CurrentWeapon->GetZoomLevel() > 1)
					{
						CurrentUserCmd.cmd->buttons |= IN_ATTACK2;
						//g_Tickbase.m_flDelayTickbaseShiftUntilThisTime = Interfaces::Globals->realtime + 0.5f;
					}
				}
			}

			int _backupButtons = cmd->buttons;

			// predict as if we shot
			if (!CurrentUserCmd.IsAttacking() && !_AttackIsBlocked)
				cmd->buttons |= IN_ATTACK;

			// slowwalk
			g_AntiAim.SlowWalk();

			LocalPlayer.Entity->ToPlayerRecord()->m_bAllowAnimationEvents = true;
			LocalPlayer.Entity->SetIsCustomPlayer(false);
			LocalPlayer.BeginEnginePrediction(cmd);
			LocalPlayer.Entity->ToPlayerRecord()->m_bAllowAnimationEvents = false;
			LocalPlayer.Entity->SetIsCustomPlayer(true);

			//printf("Velocity %f %f %f\nOrigin %f %f %f\n", LocalPlayer.Entity->GetVelocity().x, LocalPlayer.Entity->GetVelocity().y, LocalPlayer.Entity->GetVelocity().z,
			//	LocalPlayer.Entity->GetAbsOriginDirect().x, LocalPlayer.Entity->GetAbsOriginDirect().y, LocalPlayer.Entity->GetAbsOriginDirect().z);

			cmd->buttons = _backupButtons;

			// fix the animation shooting position for the local player
			QAngle eyepos = { 0.0f, cmd->viewangles.y, 0.0f };
			LocalPlayer.FixShootPosition(eyepos, false);
		}

		// set up various functions
		LocalPlayer.OnCreateMove();
		LocalPlayer.UpdateCurrentWeapon();

#ifdef INCLUDE_LEGIT
		if (!var.ragebot.b_enabled)
		{
			legitbot::get().create_move(cmd);
		}
		else
		{
			// run the ragebot here for the love of god clean this function from junk.
		}
#endif

#ifdef _DEBUG
		if (IsDebuggerPresent())
		{
			if (GetAsyncKeyState(VK_CAPITAL))
				Interfaces::DebugOverlay->ClearAllOverlays();
		}
#endif

		// we're alive
		if (LocalPlayer.IsAlive)
		{
			if (cmd->buttons & IN_ATTACK)
			{
				bool _WaitForTickBase = LocalPlayer.WaitForTickbaseBeforeFiring;

				if (!var.ragebot.exploits.b_hide_shots && !var.ragebot.exploits.b_hide_record && !var.ragebot.exploits.b_multi_tap.get() && !var.ragebot.exploits.b_nasa_walk.get())
					_WaitForTickBase = false;

				if (!_WaitForTickBase)
				{

					if (LocalPlayer.CanShiftShot && !LocalPlayer.IsFakeDucking && !LocalPlayer.IsTeleporting && !LocalPlayer.IsFakeLaggingOnPeek)
					{
						LocalPlayer.ApplyTickbaseShift = true;
						g_Info.SetForceSend(true);
						g_Info.SetShouldChoke(false);
						_ForceNextPacketToSend = true;
					}


					if (LocalPlayer.ApplyTickbaseShift)
						_ForceNextPacketToSend = true;
				}
			}
			// we have a valid weapon
			if (!_AttackIsBlocked)
			{
				// we have a gun
				if (LocalPlayer.WeaponVars.IsGun || LocalPlayer.WeaponVars.IsTaser)
				{
					g_Visuals.RunAutowallCrosshair();

					// ragebot is enabled

					if (var.ragebot.b_enabled)
					{
#ifdef _DEBUG
						bool _debugpresent = IsDebuggerPresent();
						if (_debugpresent)
						{
							m_RagebotState = g_Ragebot;
							m_AutoBoneState = g_AutoBone;
						}
					RestartRagebot:
#endif

						// run ragebot
						g_Ragebot.Run();

#ifdef _DEBUG
						if (_debugpresent)
						{

							CPlayerrecord* _pPlayerRecord;
							if (g_Ragebot.HasTarget() && (_pPlayerRecord = g_LagCompensation.GetPlayerrecord(g_Ragebot.GetTarget())))
							{
								if (!_pPlayerRecord->m_TargetRecord->m_bRanHitscan && _pPlayerRecord->m_iTickcount_ForwardTrack == 0)
								{
									if (!_pPlayerRecord->m_bIsUsingFarESP && !_pPlayerRecord->m_bIsUsingServerSide)
									{
										CTickrecord *_currentRecord = _pPlayerRecord->GetCurrentRecord();
										if (_pPlayerRecord->m_TargetRecord != _currentRecord)
										{
											//restore back to the current record as they were before we ran ragebot
											_pPlayerRecord->CM_BacktrackPlayer(_currentRecord);
										}
										g_Ragebot = m_RagebotState;
										g_AutoBone = m_AutoBoneState;
										goto RestartRagebot;
									}
									else
									{
										DebugBreak();
									}
								}
							}
						}
#endif

						if (g_Ragebot.m_bTracedEnemy)
						{
							if (var.ragebot.b_autoscope && LocalPlayer.WeaponVars.IsSniper && LocalPlayer.CurrentWeapon->GetZoomLevel() == 0)
								CurrentUserCmd.bShouldAutoScope = true; // set next tick to autoscope

							if (var.ragebot.b_autostop && LocalPlayer.Entity->GetVelocity().Length2D() >= variable::get().misc.f_minwalk_speed)//std::fmax(LocalPlayer.CurrentWeapon->GetMaxSpeed3() / 3.f, 0.001f))
								CurrentUserCmd.bShouldAutoStop = true; // set next tick to autostop
						}

						// we have a target
						if (g_Ragebot.HasTarget() && !CurrentUserCmd.IsSecondaryAttacking() && (g_WeaponController.WeaponDidFire(cmd->buttons | IN_ATTACK) == 1)
							/*&& GetAsyncKeyState(VK_RETURN) && g_ClientState->chokedcommands == 0  */ )
						{
							// shot doesn't need to be delayed
							if (!g_WeaponController.ShouldDelayShot() /*&& GetAsyncKeyState(VK_RETURN)*/)
							{
								bool _WaitForTickBase = LocalPlayer.WaitForTickbaseBeforeFiring;

								if (!var.ragebot.exploits.b_hide_shots && !var.ragebot.exploits.b_hide_record && !var.ragebot.exploits.b_multi_tap.get() && !var.ragebot.exploits.b_nasa_walk.get())
									_WaitForTickBase = false;

								if (!_WaitForTickBase)
								{
									// set aim angles
									LocalPlayer.FinalEyeAngles = g_Ragebot.GetAimAngles();
									LocalPlayer.FinalEyeAngles.Clamp();

									// we want to fire
									g_WeaponController.SetShouldFire(true);

#ifdef _DEBUG
									g_Eventlog.PrintToConsole(Color(255, 255, 255, 255), "curtime for shot %f\n", TICKS_TO_TIME(LocalPlayer.Entity->GetTickBase()));
									printf("curtime for shot %f\n", TICKS_TO_TIME(LocalPlayer.Entity->GetTickBase()));
#endif

									// set engine angles if needed
									if (!var.ragebot.b_silent)
									//if (GetAsyncKeyState(VK_RETURN))
										Interfaces::EngineClient->SetViewAngles(LocalPlayer.FinalEyeAngles);

									if (LocalPlayer.CanShiftShot && !LocalPlayer.IsFakeDucking && !LocalPlayer.IsTeleporting && !LocalPlayer.IsFakeLaggingOnPeek)
									{
										LocalPlayer.ApplyTickbaseShift = true;
										g_Info.SetForceSend(true);
										g_Info.SetShouldChoke(false);
										_ForceNextPacketToSend = true;
									}
#ifdef _DEBUG
									else if (var.ragebot.exploits.b_hide_shots && !var.ragebot.exploits.b_multi_tap.get())
									{
										g_Eventlog.PrintToConsole(Color(255, 255, 255, 255), "WARNING: Hide shots is on but we didn't hide the shot!\n");
									}
#endif

									if (!LocalPlayer.IsFakeDucking && !LocalPlayer.IsFakeLaggingOnPeek && !LocalPlayer.IsTeleporting)
										g_Info.SetForceSend(true);

									if (LocalPlayer.ApplyTickbaseShift)
										_ForceNextPacketToSend = true;

									LocalPlayer.LastShotWasShifted = LocalPlayer.ApplyTickbaseShift;
									LocalPlayer.UseDoubleTapHitchance = LocalPlayer.ApplyTickbaseShift && var.get().ragebot.exploits.b_multi_tap.get() && LocalPlayer.CurrentWeapon->GetClipOne() - 1 != 0 && LocalPlayer.CurrentWeapon->ShouldUseDoubleTapHitchance();

									// set the cmd->tickcount to the ideal value
									g_LagCompensation.AdjustCMD(m_PlayerRecords[g_Ragebot.GetTarget()].m_pEntity);
								}
							}
						}
						else
						{
							if (cmd->buttons & (IN_ATTACK | IN_ATTACK2))
								g_LagCompensation.AdjustCMD(nullptr);
						}

						if (!g_WeaponController.ShouldFire() && CurrentUserCmd.IsAttacking() && g_WeaponController.RevolverWillFire() && !(CurrentUserCmd.cmd_backup.buttons & IN_ATTACK))
						{
							// stop firing the revolver since we don't have a target
							cmd->buttons &= ~IN_ATTACK;
						}
					}
					else
					{
						// reset unneeded aimbot target
						g_Ragebot.ClearAimbotTarget();
						g_Ragebot.ClearIgnorePlayerIndex();
						g_Ragebot.ClearPossibleTargets();
						g_Info.UsingForwardTrack = false;
						if (cmd->buttons & (IN_ATTACK|IN_ATTACK2))
							g_LagCompensation.AdjustCMD(nullptr);
					}

					//remove tickbase manipulation if needed, or undo attack flags if we predicted them and aren't actually firing
					bool _RemoveAttackFlag = !CurrentUserCmd.IsAttacking();//!LocalPlayer.WeaponVars.IsRevolver && CurrentUserCmd.IsAttacking() && !g_WeaponController.ShouldFire() && !(CurrentUserCmd.cmd_backup.buttons & IN_ATTACK);
					bool _RestoreTickbase = !LocalPlayer.ApplyTickbaseShift && LocalPlayer.PredictionStateIsShifted;
					bool _ShootPositionAlreadyAnimated = true;

					if (_RestoreTickbase || _RemoveAttackFlag)
					{
						LocalPlayer.FinishEnginePrediction(cmd);
						if (_RestoreTickbase)
						{
							g_Tickbase.UndoShiftedPrediction();
						}

						Interfaces::Globals->curtime = LocalPlayer.m_flOldCurtime;
						g_Tickbase.SetCurrentCommandTickbase();
						if (_RemoveAttackFlag)
							cmd->buttons &= ~IN_ATTACK;
						LocalPlayer.Entity->ToPlayerRecord()->m_bAllowAnimationEvents = true;
						LocalPlayer.Entity->SetIsCustomPlayer(false);
						LocalPlayer.BeginEnginePrediction(cmd);
						LocalPlayer.Entity->ToPlayerRecord()->m_bAllowAnimationEvents = false;
						LocalPlayer.Entity->SetIsCustomPlayer(true);
						LocalPlayer.WeaponWillFireBurstShotThisTick = LocalPlayer.CurrentWeapon && LocalPlayer.CurrentWeapon->GetItemDefinitionIndex() != WEAPON_C4 && !LocalPlayer.CurrentWeapon->IsKnife() && LocalPlayer.CurrentWeapon->WeaponHasBurst() && LocalPlayer.CurrentWeapon->GetBurstShotsRemaining() > 0 && TICKS_TO_TIME(LocalPlayer.Entity->GetTickBase()) >= LocalPlayer.CurrentWeapon->GetNextBurstShotTime();

#if 0
						//we should do this if we want to have an accurate shoot position from this point forward, but i don't think we use shoot position for anything important if we aren't shooting
						QAngle eyepos = { 0.0f, LocalPlayer.CurrentEyeAngles.y, 0.0f };
						LocalPlayer.FixShootPosition(eyepos, false);
#else
						_ShootPositionAlreadyAnimated = false;
#endif
					}

					// do norecoil
					if (var.ragebot.b_enabled)
					{
						g_Removals.DoNoRecoil(LocalPlayer.FinalEyeAngles);
					}

					//Correct the shoot position with our final eye angles if we are attacking
					if (cmd->buttons & (IN_ATTACK | IN_ATTACK2))
					{
						LocalPlayer.FixShootPosition(LocalPlayer.FinalEyeAngles, _ShootPositionAlreadyAnimated);
						LocalPlayer.ShootPosition = LocalPlayer.Entity->Weapon_ShootPosition();
					}
				}
				else
				{
					//we have no gun
					g_Ragebot.SetLowestFovEntity();
				}
				//g_Fakelag.run used to be here but I can't remember if I meant to put it here or below
			}

			// perform fake lag on the local player
			g_Fakelag.run();

#if 0
			if (g_WeaponController.WeaponDidFire(cmd->buttons | IN_ATTACK) 
				&& !(cmd->buttons & IN_ATTACK) && (input::get().is_key_down(VK_XBUTTON1) || input::get().is_key_down(VK_XBUTTON2)))
			{
				// get current weapon
				auto weapon = LocalPlayer.CurrentWeapon;

				// no weapon, abort
				if (weapon)
				{
					// init locals
					Ray_t ray;
					CGameTrace trace;

					// init tracefilter
					CTraceFilterSimple filter;
					filter.SetPassEntity((IHandleEntity*)LocalPlayer.Entity);

					// init trace parameters
					auto weaponRange = weapon->GetCSWpnData()->flRange;
					auto traceFilter = (ITraceFilter*)&filter;
					float scale = weapon_recoil_scale.GetVar()->GetFloat();
					QAngle angles = LocalPlayer.FinalEyeAngles + LocalPlayer.Entity->GetPunch() * scale;
					Vector endPos, direction;

					// get endPos
					AngleVectors(angles, &direction);
					VectorNormalizeFast(direction);
					endPos = LocalPlayer.ShootPosition + direction * weaponRange;

					// init ray
					ray.Init(LocalPlayer.ShootPosition, endPos);

					// run trace
					Interfaces::EngineTrace->TraceRay(ray, MASK_SHOT, &filter, &trace);
					UTIL_ClipTraceToPlayers(LocalPlayer.ShootPosition, endPos + direction * 40.0f, MASK_SHOT, &filter, &trace);

					CTickrecord *backtrackrecord = nullptr;
					CPlayerrecord *backtrackplayer = nullptr;
					bool RanLegitBacktrack = false;

					if (!trace.m_pEnt
						|| !trace.m_pEnt->IsPlayer()
						|| !trace.m_pEnt->GetAlive()
						|| trace.m_pEnt->GetImmune()
						)
					{
						RanLegitBacktrack = true;

						std::vector<SortedPlayerNew>SortedPlayersCloseToCrosshair;

						for (int i = 1; i <= MAX_PLAYERS; ++i)
						{
							CPlayerrecord *pCPlayer = &m_PlayerRecords[i];
							CBaseEntity* Entity = pCPlayer->m_pEntity;
							if (!pCPlayer->m_bDormant && Entity->IsEnemy(LocalPlayer.Entity) && pCPlayer->m_iLifeState == LIFE_ALIVE)
							{
								QAngle AngleToEnemy = CalcAngle(LocalPlayer.ShootPosition, Entity->GetAbsOriginDirect() + ((Entity->GetMins() + Entity->GetMaxs()) * 0.5f));
								float FOV = GetFov(angles, AngleToEnemy);
								if (FOV <= 50.0f)
								{
									SortedPlayersCloseToCrosshair.emplace_back(SortedPlayerNew(pCPlayer, FOV));
								}
							}
						}
						if (SortedPlayersCloseToCrosshair.size())
							std::sort(SortedPlayersCloseToCrosshair.begin(), SortedPlayersCloseToCrosshair.end(), SortedPlayerIsCloserNew);

						for (auto& pl : SortedPlayersCloseToCrosshair)
						{
							if (pl.pCPlayer->m_bIsUsingFarESP)
								continue;
							static PlayerBackup_t backup;
							backup.Get(pl.pCPlayer->m_pEntity);
							for (auto& tick : pl.pCPlayer->m_Tickrecords)
							{
								if (tick->m_bCachedBones && g_LagCompensation.IsTickValid(tick))
								{
									pl.pCPlayer->CM_BacktrackPlayer(backtrackrecord);
									CTraceFilterSimple filter;
									filter.pSkip = (IHandleEntity*)LocalPlayer.Entity;
									filter.m_icollisionGroup = COLLISION_GROUP_NONE;
									Interfaces::EngineTrace->TraceRay(ray, MASK_SHOT, &filter, &trace);
									UTIL_ClipTraceToPlayers(LocalPlayer.ShootPosition, endPos + direction * 40.0f, MASK_SHOT, &filter, &trace);
	
									if (trace.m_pEnt == pl.pCPlayer->m_pEntity)
									{
										// can hit backtrack player
										backtrackrecord = tick;
										backtrackplayer = pl.pCPlayer;
										cmd->tick_count = TIME_TO_TICKS(tick->m_SimulationTime + g_LagCompensation.GetLerpTime());
										cmd->buttons |= IN_ATTACK;
										//do legit backtrack
										break;
									}
								}
							}
							backup.RestoreData();
							if (backtrackplayer)
								break;
						}
					}
					else
					{
						if (trace.m_pEnt && trace.m_pEnt->IsEnemy(LocalPlayer.Entity))
						{
							cmd->tick_count = TIME_TO_TICKS(trace.m_pEnt->GetSimulationTime() + g_LagCompensation.GetLerpTime());
							cmd->buttons |= IN_ATTACK;
						}
					}
				}
			}
#endif

			//remove tickbase manipulation if needed, or undo attack flags if we predicted them and aren't actually firing
			bool _RemoveAttackFlag = !CurrentUserCmd.IsAttacking();//!LocalPlayer.WeaponVars.IsRevolver && CurrentUserCmd.IsAttacking() && !g_WeaponController.ShouldFire() && !(CurrentUserCmd.cmd_backup.buttons & IN_ATTACK);
			bool _RestoreTickbase = !LocalPlayer.ApplyTickbaseShift && LocalPlayer.PredictionStateIsShifted;
			if (_RestoreTickbase || _RemoveAttackFlag)
			{
				LocalPlayer.FinishEnginePrediction(cmd);
				if (_RestoreTickbase)
				{
					g_Tickbase.UndoShiftedPrediction();
				}
				Interfaces::Globals->curtime = LocalPlayer.m_flOldCurtime;
				g_Tickbase.SetCurrentCommandTickbase();
				if (_RemoveAttackFlag)
					cmd->buttons &= ~IN_ATTACK;
				LocalPlayer.Entity->ToPlayerRecord()->m_bAllowAnimationEvents = true;
				LocalPlayer.Entity->SetIsCustomPlayer(false);
				LocalPlayer.BeginEnginePrediction(cmd);
				LocalPlayer.Entity->ToPlayerRecord()->m_bAllowAnimationEvents = false;
				LocalPlayer.Entity->SetIsCustomPlayer(true);
				LocalPlayer.WeaponWillFireBurstShotThisTick = LocalPlayer.CurrentWeapon && LocalPlayer.CurrentWeapon->GetItemDefinitionIndex() != WEAPON_C4 && !LocalPlayer.CurrentWeapon->IsKnife() && LocalPlayer.CurrentWeapon->WeaponHasBurst() && LocalPlayer.CurrentWeapon->GetBurstShotsRemaining() > 0 && TICKS_TO_TIME(LocalPlayer.Entity->GetTickBase()) >= LocalPlayer.CurrentWeapon->GetNextBurstShotTime();
			}

			// do anti aim
			if (!g_Tickbase.m_bIncrementTicksAllowedForProcessing)
			{
				g_AntiAim.Run();

				// we want to shoot
				if (g_WeaponController.ShouldFire())
					g_WeaponController.FireBullet();
			}
			else
				g_Info.SetShouldChoke(false);

			// handle packet choking
			g_Info.HandleChoke();

			// apply anti aim angle
			if (!CurrentUserCmd.bSendPacket)
				LocalPlayer.FinalEyeAngles.y = g_Info.m_flChokedYaw;
			else
				LocalPlayer.FinalEyeAngles.y = g_Info.m_flUnchokedYaw;
		}
		else
		{
			//Local player is not alive, clear stuff
			g_Ragebot.ClearAimbotTarget();
			g_Ragebot.ClearIgnorePlayerIndex();
			g_Ragebot.ClearPossibleTargets();
		}

		// clamp the new eye angles - ToDo: imi cvar - Anti Untrusted
		LocalPlayer.FinalEyeAngles.Clamp();

		// write the new eye angles to the cmd
		cmd->viewangles = LocalPlayer.FinalEyeAngles;

		if (!std::isfinite(cmd->viewangles.x) || !std::isfinite(cmd->viewangles.y) || !std::isfinite(cmd->viewangles.z))
		{
			cmd->viewangles = angZero;
			LocalPlayer.FinalEyeAngles = cmd->viewangles;
		}

		// movement fix
		if (LocalPlayer.IsAlive)
			g_Assistance.FixMovement();

		if (!std::isfinite(cmd->forwardmove) || !std::isfinite(cmd->sidemove) || !std::isfinite(cmd->upmove))
		{
			cmd->forwardmove = 0.0f;
			cmd->sidemove = 0.0f;
			cmd->upmove = 0.0f;
			cmd->buttons &= ~IN_FORWARD;
			cmd->buttons &= ~IN_BACK;
			cmd->buttons &= ~IN_MOVELEFT;
			cmd->buttons &= ~IN_MOVERIGHT;
			g_Assistance.m_angPostStrafe = angZero;
			g_Assistance.m_angStrafeAngle = angZero;
			g_Assistance.m_flOldYaw = 0.0f;
		}

		if (CurrentUserCmd.bSendPacket)
			LocalPlayer.LastSentEyeAngles = cmd->viewangles;

		HookNetChannel();

		// save shot record
		if (LocalPlayer.CurrentWeapon && LocalPlayer.WeaponVars.IsGun)
		{
			if (CurrentUserCmd.IsAttacking() || (CurrentUserCmd.IsSecondaryAttacking() && LocalPlayer.WeaponVars.IsRevolver))
				LocalPlayer.CreateShotRecord();
		}

		g_LagCompensation.CM_RestorePlayers();


		//float lby = LocalPlayer.Entity->GetAbsAnglesDirect().y;

		// predict localplayer lby update and fix animations
		if (LocalPlayer.IsAlive && !g_Tickbase.m_bIncrementTicksAllowedForProcessing && cmd->tick_count <= Interfaces::Globals->tickcount + sv_max_usercmd_future_ticks.GetVar()->GetInt())
		{
			// try and find enemies and fake lag if we can hit them in the future
			if (!CurrentUserCmd.IsAttacking() && !LocalPlayer.IsFakeDucking && variable::get().ragebot.b_fakelag_peek)
				g_Assistance.FakeLagOnPeek();

			//Fix buttons in SetupMovement
			int buttons = LocalPlayer.Entity->GetButtons();
			int movestate = LocalPlayer.Entity->GetMoveState();
			bool iswalking = LocalPlayer.Entity->GetIsWalking();
			LocalPlayer.Entity->SetButtons(cmd->buttons);
			LocalPlayer.Entity->SetMoveState(0);
			LocalPlayer.Entity->SetIsWalking(false);
			auto movingForward = cmd->buttons & IN_FORWARD;
			auto movingBackward = cmd->buttons & IN_BACK;
			auto movingRight = cmd->buttons & IN_MOVERIGHT;
			auto movingLeft = cmd->buttons &  IN_MOVELEFT;
			auto walking = cmd->buttons & IN_SPEED;
			bool walkstate = walking ? true : false;
			if (cmd->buttons & IN_DUCK || LocalPlayer.playerstate.LocalData.m_bDucking_ || LocalPlayer.playerstate.Flags & FL_DUCKING)
			{
				walkstate = false;
			}
			else if (walking)
			{
				float adjustedmaxspeed = LocalPlayer.playerstate.MaxSpeed * _CS_PLAYER_SPEED_WALK_MODIFIER;
				if (adjustedmaxspeed + 25.f > LocalPlayer.playerstate.Velocity.Length())
					LocalPlayer.Entity->SetIsWalking(true);
			}
			auto pressinganymovebuttons = cmd->buttons & (IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT | IN_RUN); //0x1618;

			bool pressingbothforwardandback;
			bool pressingbothsidebuttons;

			if (!movingForward)
				pressingbothforwardandback = false;
			else
				pressingbothforwardandback = movingBackward;

			if (!movingRight)
				pressingbothsidebuttons = false;
			else
				pressingbothsidebuttons = movingLeft;

			if (pressinganymovebuttons)
			{
				if (pressingbothforwardandback)
				{
					if (pressingbothsidebuttons)
					{
						//can't move if pressing all the move keys
						LocalPlayer.Entity->SetMoveState(0);
					}
					else if (movingRight || movingLeft)
					{
						LocalPlayer.Entity->SetMoveState(2);
					}
					else
					{
						LocalPlayer.Entity->SetMoveState(0);
					}
				}
				else
				{
					if (!pressingbothsidebuttons || movingForward)
					{
						LocalPlayer.Entity->SetMoveState(2);
					}
					else if (!movingBackward)
					{
						LocalPlayer.Entity->SetMoveState(0);
					}
					else
					{
						LocalPlayer.Entity->SetMoveState(2);
					}
				}
			}

			if (LocalPlayer.Entity->GetMoveState() == 2 && walkstate)
				LocalPlayer.Entity->SetMoveState(1);

			LocalPlayer.PredictLowerBodyYaw(LocalPlayer.FinalEyeAngles);
			LocalPlayer.Entity->SetButtons(buttons);
			LocalPlayer.Entity->SetMoveState(movestate);
			LocalPlayer.Entity->SetIsWalking(iswalking);
		}

		//float newgfy = LocalPlayer.Entity->GetAbsAnglesDirect().y;

		//LocalPlayer.DrawTextInFrontOfPlayer(TICKS_TO_TIME(2), 0.0f, "%f            %f       %f        %f", AngleDiff(newgfy, lby), AngleDiff(cmd->viewangles.y, newgfy), cmd->viewangles.y, LocalPlayer.Entity->GetLowerBodyYaw());

		//Set and backup bSendPacket
		CurrentUserCmd.OverrideSendPacket();

		g_Tickbase.FixEvents();

		g_Tickbase.m_iCalculatedTickbase[cmd->command_number % 150] = LocalPlayer.Entity->GetTickBase();

		//Finish engine prediction
		if (LocalPlayer.IsAlive)
		{
			LocalPlayer.FinishEnginePrediction(cmd);
			Interfaces::Prediction->Update(g_ClientState->m_nDeltaTick, g_ClientState->m_nDeltaTick > 0, g_ClientState->last_command_ack, g_ClientState->lastoutgoingcommand + g_ClientState->chokedcommands);
		}
		else
			LocalPlayer.m_cmdAbsAngles[cmd->command_number % 150] = angZero;

		Interfaces::Globals->curtime = LocalPlayer.m_flOldCurtime;
	}
	else
	{
		//no local player entity
		g_Tickbase.m_iCalculatedTickbase[cmd->command_number % 150] = LocalPlayer.Entity->GetTickBase();
		LocalPlayer.m_cmdAbsAngles[cmd->command_number % 150] = angZero;
	}

	clantag_changer::get().create_move();

	g_Tickbase.OnCreateMoveFinished();

	if (CurrentUserCmd.bSendPacket)
		g_Visuals.last_ticks_choked = g_ClientState->chokedcommands;

#ifdef _DEBUG
	if (!LocalPlayer.WeaponVars.IsRevolver)
	{
		if (cmd->buttons & IN_ATTACK && !CurrentUserCmd.bSendPacket)
		{
			g_Eventlog.PrintToConsole(Color(255, 255, 255, 255), "WARNING: Fired shot without sendpacket!\n");
		}
		if (GetUserCmd(0, cmd->command_number - 1) && GetUserCmd(0, cmd->command_number - 1)->buttons & IN_ATTACK)
		{
			if (!CurrentUserCmd.bSendPacket)
			{
				g_Eventlog.PrintToConsole(Color(255, 255, 255, 255), "WARNING: last cmd had attack but we are choking this cmd!\n");
			}
		}
	}
#endif

	g_Tickbase.m_iCustomTickbase[cmd->command_number % 150][1] = 0; //now we can erase the flag for not overwriting

	if (_ForceNextPacketToSend)
		g_Info.SetForceSend(true);

	if (g_ClientState->m_nDeltaTick == -1)
		g_Tickbase.Reset();

	LocalPlayer.m_bPredictionError = false;
	LocalPlayer.NetvarMutex.Unlock();

	if (0 && input::get().is_key_down(VK_UP))
	{
		static DWORD ConstructorFn = 0;
		static DWORD SetTextFn;
		static DWORD WriteToBufferFn;
		static DWORD DeconstructorFn;

		INetChannel* chan = (INetChannel*)g_ClientState->m_pNetChannel;
		if (chan && chan->m_Socket >= 0 && !*(DWORD *)((DWORD)chan + 0x108))
		{
			if (!ConstructorFn)
			{
				DWORD adr = FindMemoryPattern(EngineHandle, std::string("E8 ? ? ? ? 53 8D 4C 24 1C E8 ? ? ? ? 8D 46 54 50 8D 4C 24 18 E8 ? ? ? ? 8B 06 8B CE 6A 00 FF 90 ? ? ? ? 8D 4C 24 14 E8 ? ? ? ?"), false);

				uint32_t* RelativeAdr = (uint32_t*)(adr + 1);
				ConstructorFn = ((uint32_t)RelativeAdr + 4 + *RelativeAdr);
				RelativeAdr = (uint32_t*)(adr + 11);
				SetTextFn = ((uint32_t)RelativeAdr + 4 + *RelativeAdr);
				RelativeAdr = (uint32_t*)(adr + 24);
				WriteToBufferFn = ((uint32_t)RelativeAdr + 4 + *RelativeAdr);
				RelativeAdr = (uint32_t*)(adr + 45);
				DeconstructorFn = ((uint32_t)RelativeAdr + 4 + *RelativeAdr);
			}

			const char* newreason = variable::get().misc.disconnect_reason.str_text.c_str();
			if (newreason)
			{
				char v17[128]; //52
				reinterpret_cast<void(__thiscall*)(char*)>(ConstructorFn)(v17);
				reinterpret_cast<void(__thiscall*)(char*, const char*)>(SetTextFn)(&v17[4], newreason);
				reinterpret_cast<void(__thiscall*)(char*, void*)>(WriteToBufferFn)(v17, &chan->m_StreamUnreliable);
				chan->Transmit(false);
				reinterpret_cast<void(__thiscall*)(char*)>(DeconstructorFn)(v17);
			}
		}
	}

	END_PROFILING
	return false;
}

void HookNetChannel()
{
	if (!g_ClientState)
		return;

	if (!HNetchan)
	{
		if (g_ClientState->m_pNetChannel)
		{
			//Reset our netchannels
			our_netchan.m_nChokedPackets	= 0;
			our_netchan.m_PacketDrop		= 0;
			our_netchan.m_nInSequenceNr		= 0;
			our_netchan.m_nInReliableState  = 0;
			our_netchan.m_nOutReliableState = 0;
			our_netchan.m_nOutSequenceNr	= 1;
			our_netchan.m_nOutSequenceNrAck = 0;
			memset(our_netchan.m_DataFlow, 0, sizeof(our_netchan.m_DataFlow));
			memset(our_netchan.m_MsgStats, 0, sizeof(our_netchan.m_MsgStats));
			client_netchan.m_nChokedPackets	= 0;
			client_netchan.m_PacketDrop		   = 0;
			client_netchan.m_nInSequenceNr	 = 0;
			client_netchan.m_nInReliableState  = 0;
			client_netchan.m_nOutReliableState = 0;
			client_netchan.m_nOutSequenceNr	= 1;
			client_netchan.m_nOutSequenceNrAck = 0;
			memset(client_netchan.m_DataFlow, 0, sizeof(client_netchan.m_DataFlow));
			memset(client_netchan.m_MsgStats, 0, sizeof(client_netchan.m_MsgStats));
			server_netchan.m_nChokedPackets	= 0;
			server_netchan.m_PacketDrop		   = 0;
			server_netchan.m_nInSequenceNr	 = 0;
			server_netchan.m_nInReliableState  = 0;
			server_netchan.m_nOutReliableState = 0;
			server_netchan.m_nOutSequenceNr	= 1;
			server_netchan.m_nOutSequenceNrAck = 0;
			memset(server_netchan.m_DataFlow, 0, sizeof(server_netchan.m_DataFlow));
			memset(server_netchan.m_MsgStats, 0, sizeof(server_netchan.m_MsgStats));

			HNetchan	   = new VTHook((DWORD**)g_ClientState->m_pNetChannel, hook_types::_Netchan);
			oSendDatagram  = (SendDatagramFn)HNetchan->HookFunction((DWORD)Hooks::SendDatagram, 46); //48
			oCanPacket	 = (CanPacketFn)HNetchan->HookFunction((DWORD)Hooks::CanPacket, 56);
			oSendNetMsg	= (SendNetMsgFn)HNetchan->HookFunction((DWORD)Hooks::SendNetMsg, 40); //42
			oProcessPacket = (ProcessPacketFn)HNetchan->HookFunction((DWORD)Hooks::ProcessPacket, 39);
			//oShutdownNetchan = (ShutdownNetchanFn)HNetchan->HookFunction((DWORD)Hooks::ShutdownNetchan, 36);

#if 0
			DWORD off = (DWORD)&((INetChannel*)NetChannel)->m_DataFlow - (DWORD)NetChannel;
			off = (DWORD)&((INetChannel*)NetChannel)->m_DataFlow - (DWORD)HNetchan;
			off = (DWORD)&((INetChannel*)NetChannel)->last_received - (DWORD)HNetchan;
			off = (DWORD)&((INetChannel*)NetChannel)->last_received - (DWORD)HNetchan;
			off = (DWORD)&((INetChannel*)NetChannel)->last_received - (DWORD)HNetchan;
			off = (DWORD)&((INetChannel*)NetChannel)->last_received - (DWORD)HNetchan;
#endif

			if (!HMessageHandler || 
				!g_ClientState->m_pNetChannel->m_MessageHandler 
				|| HMessageHandler->GetNewVT() != GetVT((DWORD**)g_ClientState->m_pNetChannel->m_MessageHandler))
			{
				if (HMessageHandler)
				{
					HMessageHandler->ClearClassBase();
					delete HMessageHandler;
				}
				if (g_ClientState->m_pNetChannel->m_MessageHandler)
				{
					HMessageHandler = new VTHook((DWORD**)((INetChannel*)g_ClientState->m_pNetChannel)->m_MessageHandler, hook_types::_MessageHandler);
					oPacketStart = (PacketStartFn)HMessageHandler->HookFunction((DWORD)Hooks::PacketStart, 5);
					oPacketEnd = (PacketEndFn)HMessageHandler->HookFunction((DWORD)Hooks::PacketEnd, 6);
				}
			}
		}
	}
	else
	{
		if (g_ClientState->m_pNetChannel)
		{
			if (HMessageHandler &&
				(!g_ClientState->m_pNetChannel->m_MessageHandler ||
					*(DWORD*)((DWORD)g_ClientState->m_pNetChannel->m_MessageHandler) != (DWORD)HMessageHandler->GetNewVT()))
			{
				HMessageHandler->ClearClassBase();
				delete HMessageHandler;
				HMessageHandler = nullptr;
			}

			if (!HMessageHandler && g_ClientState->m_pNetChannel->m_MessageHandler)
			{
				HMessageHandler = new VTHook((DWORD**)((INetChannel*)g_ClientState->m_pNetChannel)->m_MessageHandler, hook_types::_MessageHandler);
				oPacketStart = (PacketStartFn)HMessageHandler->HookFunction((DWORD)Hooks::PacketStart, 5);
				oPacketEnd = (PacketEndFn)HMessageHandler->HookFunction((DWORD)Hooks::PacketEnd, 6);
			}

			if (GetVT((DWORD**)g_ClientState->m_pNetChannel) != HNetchan->GetNewVT() || HNetchan->GetCurrentVT() != HNetchan->GetNewVT())
			{
				HNetchan->ClearClassBase();
				delete HNetchan;
				HNetchan = nullptr;
			}

			//Custom netchan handling
			if (HNetchan && HMessageHandler)
			{
				//Pretend we are the server, send a packet to our 'client' if it's time
				if (!CurrentUserCmd.bFinalTick || !CurrentUserCmd.bOriginalSendPacket)
				{
					server_netchan.m_nChokedPackets++;
				}
				else
				{
					//Generate just a generic size, packet size isn't used for ping calculation
#if 0
					int nTotalSize = 50 + UDP_HEADER_SIZE;
					FlowNewPacket(&server_netchan, FLOW_OUTGOING, server_netchan.m_nOutSequenceNr, server_netchan.m_nInSequenceNr, server_netchan.m_nChokedPackets, 0, nTotalSize);
					FlowUpdate(&server_netchan, FLOW_OUTGOING, nTotalSize);
					LocalPlayer.Last_Server_ChokeCountSent = server_netchan.m_nChokedPackets;
					LocalPlayer.Last_Server_InSequenceNrSent = server_netchan.m_nInSequenceNr;
					LocalPlayer.Last_Server_OutSequenceNrSent = server_netchan.m_nOutSequenceNr;

					server_netchan.m_nChokedPackets = 0;
					server_netchan.m_nOutSequenceNr++;
#endif
				}
			}
		}
		else
		{
			HNetchan->ClearClassBase();
			delete HNetchan;
			HNetchan = nullptr;
		}
	}
}
