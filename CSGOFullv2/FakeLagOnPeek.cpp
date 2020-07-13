#include "UtlVector.hpp"
#include "Assistance.h"
#include "Aimbot_imi.h"
#include "LocalPlayer.h"
#include "CParallelProcessor.h"
#include "AutoWall.h"

#include "Adriel/stdafx.hpp"

std::atomic<bool> FoundTraceablePlayer = false;
const Vector offset = { 0.0f, 0.0f, 0.25f };
Vector start;

void __cdecl Parallel_CanTraceEnemy(CBaseEntity*& Entity)
{
	if (FoundTraceablePlayer)
		return;

	//Vector end = Entity->GetAbsOriginDirect();
	//Vector head = end + (Entity->GetMaxs() - offset);
	//Vector center = end + ((Entity->GetMaxs() - Entity->GetMins()) * 0.5f);
	//Vector vecforward = (head - start);
	//Vector vecright;
	//QAngle angle;
	//VectorAngles(vecforward, angle);
	//AngleVectors(angle, &vecforward, &vecright, nullptr);
	//Vector leftcenter = center + vecright * -0.6f;
	//Vector rightcenter = center + vecright * 0.6f;

	Vector head = Entity->GetBonePosition(HITBOX_HEAD);
	Vector center = Entity->GetBonePosition(HITBOX_THORAX);

	//Interfaces::DebugOverlay->AddLineOverlay(start, center, 255, 0, 0, false, TICKS_TO_TIME(2));

	Autowall_Output_t output;
	bool scan_through_teammates = variable::get().ragebot.b_scan_through_teammates;
	if (Autowall(start, head, output, false, false, Entity)	|| Autowall(start, center, output, !scan_through_teammates, false, Entity))
	{
		FoundTraceablePlayer = true;
	}
}

//If we the ragebot can't find a player, search around for one if we are moving up to max amount of ticks into the future
//If we found a future target, keep fake lagging 
void CAssistance::FakeLagOnPeek()
{
	bool _IsFakeLaggingOnPeek = false;

	if (variable::get().ragebot.b_fakelag_peek && CurrentUserCmd.bSendPacket && !variable::get().legitbot.aim.b_enabled.b_state && variable::get().ragebot.b_enabled && !g_Ragebot.FoundEnemy() && LocalPlayer.WeaponVars.IsGun)
	{
		//FIXME: we could do something with fake duck here as well
		if (CurrentUserCmd.cmd->forwardmove != 0.0f || CurrentUserCmd.cmd->sidemove != 0.0f 
			|| LocalPlayer.Entity->GetVelocity().Length() > 0.1f 
			|| (LocalPlayer.Entity->GetDuckAmount() > 0.0f && !(CurrentUserCmd.cmd->buttons & IN_DUCK)))
		{
			if (g_ClientState->chokedcommands + 1 <= MAX_USER_CMDS - 1)
			{
				CUserCmd *cmd = CurrentUserCmd.cmd;
				CBaseEntity *Entity = LocalPlayer.Entity;

				CBaseEntity **EntityList = new CBaseEntity*[MAX_PLAYERS + 1];
				int numents = 0;
				for (int pindex = 1; pindex <= MAX_PLAYERS; ++pindex)
				{
					CBaseEntity* Ent = Interfaces::ClientEntList->GetBaseEntity(pindex);
					if (Ent && Ent != Entity && !Ent->GetDormant() && Ent->GetAlive() && !Ent->GetImmune() && Ent->IsEnemy(LocalPlayer.Entity))
						EntityList[numents++] = Ent;
				}

				if (numents)
				{
					PlayerBackup_t *backup = new PlayerBackup_t(Entity);

					int maxchoke = (MAX_USER_CMDS - 1 - (g_ClientState->chokedcommands + 1));
					MyPlayer *clone = new MyPlayer;
					clone->Get(clone);
					for (int i = 1; i < maxchoke; ++i)
					{
						clone->BeginEnginePrediction(cmd, false, i);
						clone->FinishEnginePrediction(cmd, false);

						LocalPlayer.FixShootPosition(angZero, false);
						start = LocalPlayer.Entity->Weapon_ShootPosition();

						FoundTraceablePlayer = false;
						ParallelProcess(EntityList, numents, Parallel_CanTraceEnemy);

						if (FoundTraceablePlayer)
						{
#ifdef _DEBUG
							AllocateConsole();
							printf("fakelag on peek must choke %i more cmds\n", i);
#endif
							++g_Info.m_iNumCommandsToChoke;
							CurrentUserCmd.bSendPacket = false;
							g_Info.m_bLastShouldChoke = true;
							_IsFakeLaggingOnPeek = true;
							break;
						}
					}

					delete clone;
					backup->RestoreData();
					delete backup;
				}
				delete[] EntityList;
			}
		}
	}

	LocalPlayer.IsFakeLaggingOnPeek = _IsFakeLaggingOnPeek;
}