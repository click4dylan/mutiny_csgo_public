#include "VTHook.h"
#include "ConVar.h"
#include "Interfaces.h"
#include "FarESP.h"
#include "ServerSide.h"
#include "LocalPlayer.h"
#include "CPlayerrecord.h"
#include "Events.h"
#include "TickbaseExploits.h"
#include "GlobalInfo.h"
#include "WaypointSystem.h"
#include "CWeatherController.h"

#include "Adriel/stdafx.hpp"

LevelShutdownFn oLevelShutdown;

void __fastcall Hooks::LevelShutdown(void* pclient)
{
#if defined(USE_FAR_ESP) || defined(USE_SERVER_SIDE)
	//If not connected, shut down all far esp/server side connections
#ifdef USE_FAR_ESP
	if (g_FarESP.IsSocketCreated() && !g_FarESP.ShouldExit())
		g_FarESP.SetShouldExit(true);
	g_FarESP.ClearClientToServerData();
#endif
#ifdef USE_SERVER_SIDE
	if (pServerSide.IsSocketCreated() && !pServerSide.ShouldExit())
		pServerSide.SetShouldExit(true);
#endif
#endif

	ClearAllWaypoints();
	LocalPlayer.Entity = nullptr;
	g_Info.LevelisLoaded = false;
	ClearAllPlayers();

	g_WeatherController.OnLevelChanged();

	oLevelShutdown(pclient);

	g_QueuedImpactResolveEvents.clear();
	g_QueuedImpactEvents.clear();
	g_GameEvent_PlayerHurt_Queue.clear();
	g_GameEvent_Impact_Queue.clear();
	g_GameEvent_PlayerDeath_Queue.clear();
	g_Tickbase.Reset();
	LocalPlayer.m_UserCommandMutex.Lock();
	LocalPlayer.m_vecUserCommands.clear();
	LocalPlayer.m_UserCommandMutex.Unlock();
}