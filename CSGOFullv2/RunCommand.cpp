#include "precompiled.h"
#include "RunCommand.h"
#include "LocalPlayer.h"
#include "ClientSideAnimationList.h"
#include "Netchan.h"
//#include "Convars.h"
#include "WeaponController.h"
#include "UsedConvars.h"
#include "TickbaseExploits.h"
RunCommandFn oRunCommand;


void __stdcall RunCmd(void* thisptr, CBaseEntity *pEntity, CUserCmd* pUserCmd, void* moveHelper)
{
	LocalPlayer.Get(&LocalPlayer);

	if (pEntity == LocalPlayer.Entity)
	{
		if (pUserCmd->tick_count > Interfaces::Globals->tickcount + sv_max_usercmd_future_ticks.GetVar()->GetInt())
		{
#if 0
			//Server runs the last processed command for dropped commands
			for (int i = pUserCmd->command_number - 1; i >= pUserCmd->command_number - 18; --i)
			{
				CUserCmd *cmd = GetUserCmd(0, i);
				if (cmd && cmd->tick_count <= ( (Interfaces::Globals->tickcount - (pUserCmd->command_number - cmd->command_number)) + sv_max_usercmd_future_ticks->GetInt()))
				{
					//oRunCommand(thisptr, pEntity, cmd, moveHelper);
					return;
				}
			}
#endif
			return;
		}

		CBaseCombatWeapon *weapon = nullptr;
		bool _IsRevolver = false;
		int _OldActivity = 0;
		int _WeaponWillFire = 0;
		int _Tickbase = 0;
		float _OldNextAttack = pEntity->GetNextAttack();
		float _OldNextSecondaryAttack;
		CBaseCombatWeapon* pWeapon = pEntity->GetWeapon();
		if (pWeapon) _OldNextSecondaryAttack = pWeapon->GetNextSecondaryAttack();

		if (LocalPlayer.InActualPrediction)
		{
			//Check to see if we are using a revolver and if it will fire
#ifdef FIX_REVOLVER
			if ((weapon = pEntity->GetActiveCSWeapon()) && weapon->GetItemDefinitionIndex() == WEAPON_REVOLVER)
			{
				_IsRevolver = true;
				_OldActivity = weapon->GetActivity();
				float backupcurtime = Interfaces::Globals->curtime;
				Interfaces::Globals->curtime = pEntity->GetTickBase() * TICK_INTERVAL;
				_WeaponWillFire = g_WeaponController.WeaponDidFire(pUserCmd->buttons);
				Interfaces::Globals->curtime = backupcurtime;
			}
#endif

		}

		//CPlayerrecord *_playerRecord = pEntity->ToPlayerRecord();

		//Allow DoAnimationEvent to run during game movement because the IsPlayerCheck was inlined into game movement by the compiler
		//bool _originalIsCustomPlayer = pEntity->IsCustomPlayer();
		//if (_playerRecord->m_bAllowAnimationEvents)
		//	pEntity->SetIsCustomPlayer(false);

		oRunCommand(thisptr, pEntity, pUserCmd, moveHelper);

		//Fix prediction errors. server does not have m_pPhysicsController
		pEntity->SetvphysicsCollisionState(0);

		//pEntity->SetIsCustomPlayer(_originalIsCustomPlayer);

		if (LocalPlayer.IsAllowedUntrusted())
		{
			CBaseCombatWeapon *pCurrentWeapon = pEntity->GetWeapon();
			if (pCurrentWeapon && pWeapon && pUserCmd->weaponselect != 0 && pWeapon != pCurrentWeapon && pEntity->GetNextAttack() != _OldNextAttack)
			{
				//Enforce a few ticks of delay in case we didn't predict the correct tickbase when tickbase manipulating
				//This should stop us from shooting early when we can't shoot on the server
				float _CurrentTime = TICKS_TO_TIME(pEntity->GetTickBase());
				float _DeployDelaySecs = pEntity->GetNextAttack() - _CurrentTime;
				if (LocalPlayer.IsAllowedUntrusted())
				{
					//pEntity->SetNextAttack(_CurrentTime + _DeployDelaySecs + TICKS_TO_TIME(2));
				}
			}

			if (pUserCmd->buttons & IN_ATTACK2)
			{
				int index;
				if (pCurrentWeapon && pWeapon && pCurrentWeapon == pWeapon && (index = pCurrentWeapon->GetItemDefinitionIndex(), pCurrentWeapon->IsSniper(index) || index == WEAPON_AUG || index == WEAPON_SG556))
				{
					//Fix sniper scopes when tickbase manipulating
					if (pCurrentWeapon->GetNextSecondaryAttack() != _OldNextSecondaryAttack)
					{
						//float _CurrentTime = TICKS_TO_TIME(pEntity->GetTickBase());
						//float _DeployDelaySecs = pCurrentWeapon->GetNextPrimaryAttack() - _CurrentTime;
						//g_Tickbase.m_flDelayTickbaseShiftUntilThisTime = Interfaces::Globals->realtime + 0.5f;
					}
				}
			}


			if (_Tickbase != 0)
				pEntity->SetTickBase(_Tickbase + 1);
		}

		//Fix m_flPostPoneFireReadyTime for the revolver
#ifdef FIX_REVOLVER
		if (LocalPlayer.InActualPrediction && weapon && _IsRevolver)
		{
			//Only do this if we are running high enough FPS
			if (LocalPlayer.m_flAverageFrameTime < Interfaces::Globals->interval_per_tick)
			{
				//Are we reloading?
				if (weapon->IsReloading())
					weapon->SetPostPoneFireReadyTime(FLT_MAX);

				//Did we start cocking this tick?
				if (weapon->GetActivity() == 208 && _OldActivity != 208)
					weapon->SetPostPoneFireReadyTime(TICKS_TO_TIME(pEntity->GetTickBase()) + 0.2f);

				//Did we shoot this tick?
				if (_WeaponWillFire == 1)
					weapon->SetPostPoneFireReadyTime(FLT_MAX);
			}
		}
#endif

		//Restore the angles for this command since animations are done in CreateMove
		if (LocalPlayer.InActualPrediction && pEntity->GetAlive())
		{
			QAngle& _CorrectedAngles = LocalPlayer.m_cmdAbsAngles[pUserCmd->command_number % 150];
			pEntity->SetAbsAngles(_CorrectedAngles);
			pEntity->SetLocalAngles(_CorrectedAngles);
		}

		LocalPlayer.SaveUncompressedNetvars();
	}
	else
	{
		oRunCommand(thisptr, pEntity, pUserCmd, moveHelper);
	}
}

void __fastcall Hooks::RunCommand(void* thisptr, void* edx, CBaseEntity *pEntity, CUserCmd* pUserCmd, void* moveHelper)
{
	RunCmd(thisptr, pEntity, pUserCmd, moveHelper);
}
