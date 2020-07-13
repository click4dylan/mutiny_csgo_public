#include "VTHook.h"
#include "ConVar.h"
#include "Interfaces.h"
#include "LocalPlayer.h"
#include "Visuals_imi.h"
#include "Events.h"
#include "Assistance.h"
#include "CreateMove.h"
#include "WaypointSystem.h"

LevelInitPreEntityFn2 oLevelInitPreEntityHLClient;
bool ResetCLC_Move_Variables = false;

void __fastcall Hooks::LevelInitPreEntity(void* pclient, void* edx, const char* mapname)
{
	g_Info.LevelisLoaded = true;

	g_QueuedImpactResolveEvents.clear();
	g_QueuedImpactEvents.clear();
	g_GameEvent_PlayerHurt_Queue.clear();
	g_GameEvent_Impact_Queue.clear();
	g_GameEvent_PlayerDeath_Queue.clear();
	ResetCLC_Move_Variables = true;

	//decrypts(0)
	static ConVar *cl_interp_ratio = Interfaces::Cvar->FindVar(XorStr("cl_interp_ratio"));
	static ConVar *cl_interp = Interfaces::Cvar->FindVar(XorStr("cl_interp"));
	static ConVar *cl_updaterate = Interfaces::Cvar->FindVar(XorStr("cl_updaterate"));
	static ConVar* pMax = Interfaces::Cvar->FindVar(XorStr("sv_client_max_interp_ratio"));
	static ConVar* mp_forcecamera = Interfaces::Cvar->FindVar(XorStr("mp_forcecamera"));
	//encrypts(0)

	g_Visuals.last_forcecam = mp_forcecamera->GetInt();
	g_Assistance.m_angStrafeAngle = angZero;
	g_Assistance.m_angPostStrafe = angZero;
	g_Assistance.m_flOldYaw = 0.0f;

	if (cl_interp_ratio && cl_interp && cl_updaterate)
	{
		//if (cl_interp->GetFloat() != 0.0f && cl_interp->GetFloat() != cl_interp_ratio->GetFloat() / cl_updaterate->GetFloat())
		{
			cl_interp_ratio->SetValue(2.0f);
			cl_interp->SetValue(TICK_INTERVAL);
		}
	}

	ClearAllWaypoints();

	oLevelInitPreEntityHLClient(pclient, mapname);
}
