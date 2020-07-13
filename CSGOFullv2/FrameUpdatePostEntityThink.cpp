#include "precompiled.h"
#include "VTHook.h"
#include "LocalPlayer.h"
#include "FrameUpdatePostEntityThink.h"
#include "INetchannelInfo.h"

//Server lag compensation hook

#ifdef HOOK_LAG_COMPENSATION
FrameUpdatePostEntityThinkFn oLagCompFrameUpdatePostEntityThink;


std::deque<LagCompensationRecord*> gLagCompensationRecords[65];

void __fastcall Hooks::LagComp_FrameUpdatePostEntityThink(void* thisptr)
{
	oLagCompFrameUpdatePostEntityThink(thisptr);

	AllocateConsole();

	for (int i = 0; i < GetServerClientCount(); i++)
	{
		DWORD cl = GetServerClientEntity(i);
		if (cl && ((CBaseEntity*)cl)->IsActive() && ((CBaseEntity*)cl)->IsConnected())// && !((CBaseEntity*)cl)->IsFakeClient())
		{
			CBaseEntity *pEnt = ServerClientToEntity(cl);
			if (pEnt)
			{
				int index = pEnt->entindexServer();

				float flSimulationTime = pEnt->GetSimulationTimeServer();
				auto Records = &gLagCompensationRecords[index];
				LagCompensationRecord* currentrecord = Records->size() ? Records->front() : nullptr;

				if (!currentrecord || flSimulationTime > currentrecord->flSimulationTime)
				{
					//INetChannelInfo *nci = GetPlayerNetInfoServer(index);
					//if (nci)
					//	printf("Latency %f\n", nci->GetLatency(FLOW_OUTGOING));


					int tickschoked = currentrecord ? TIME_TO_TICKS(flSimulationTime - currentrecord->flSimulationTime) - 1 : 0;
					BOOLEAN bAlive = pEnt->GetAliveServer();
					QAngle absangles = pEnt->GetAbsAnglesServer();
					Vector absorigin = pEnt->GetAbsOriginServer();
					float body_yaw = pEnt->GetPoseParameterUnScaledServer(11);
					float body_pitch = pEnt->GetPoseParameterUnScaledServer(12);
					float goalfeetyaw = pEnt->GetGoalFeetYawServer();
					float curfeetyaw = pEnt->GetCurrentFeetYawServer();
					CUserCmd *lastusercmd = (CUserCmd*)((DWORD)pEnt + 0x0EF8); //0F54
					CUserCmd *curusercmd = (CUserCmd*)*(DWORD*)((DWORD)pEnt + 0x0F60);

					LagCompensationRecord* newrecord = new LagCompensationRecord;

					newrecord->flSimulationTime = flSimulationTime;
					newrecord->bAlive = bAlive;
					newrecord->absangles = absangles;
					newrecord->absorigin = absorigin;
					newrecord->body_yaw = body_yaw;
					newrecord->body_pitch = body_pitch;
					newrecord->goalfeetyaw = goalfeetyaw;
					newrecord->curfeetyaw = curfeetyaw;
					newrecord->tickschoked = tickschoked;
					newrecord->lastusercmd = *lastusercmd;

					if (curusercmd)
						newrecord->currentusercmd = *curusercmd;
					else
						newrecord->currentusercmd.command_number = -1;

					Records->push_front(newrecord);

					if (Records->size() > 72)
					{
						delete Records->back();
						Records->pop_back();
					}
					//printf("Choked: %i, Last tick: %i, Last eyeangles %f %f %f, Last commandnr %i, FM %f, SM %f\n", tickschoked, lastusercmd->tick_count, lastusercmd->viewangles.x, lastusercmd->viewangles.y, lastusercmd->viewangles.z, lastusercmd->command_number, lastusercmd->forwardmove, lastusercmd->sidemove);
					//printf("Server Index: %i\nAlive: %i\nTickCount: %i\nCommandNr: %i\nTicksChoked: %i\nSimulationTime: %f\nAbsAngles: %f %f %f\nAbsOrigin: %f %f %f\nBody_Yaw: %f\nBody_Pitch: %f\nGoalFeetYaw: %f\nCurFeetYaw: %f\n\n", index, (int)bAlive, lastusercmd->tick_count, lastusercmd->command_number, tickschoked, flSimulationTime, absangles.x, absangles.y, absangles.z, absorigin.x, absorigin.y, absorigin.z, body_yaw, body_pitch, goalfeetyaw, curfeetyaw);
#if 0
					if (Records->size() > 1)
					{
						LagCompensationRecord *prevrecord = Records->at(1);
						if (newrecord->absangles.y != prevrecord->absangles.y || newrecord->body_pitch != prevrecord->body_pitch || newrecord->body_yaw != prevrecord->body_yaw)
							printf("AbsAngles: %f %f %f\nBody_Yaw: %f\nBody_Pitch: %f\n\n", absangles.x, absangles.y, absangles.z, body_yaw, body_pitch);
					}
#endif
				}
			}
		}
	}
}

#endif
