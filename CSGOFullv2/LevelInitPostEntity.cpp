#include "precompiled.h"
#include "VTHook.h"
#include "Draw.h"
#include "OverrideView.h"
#include "Aimbot_imi.h"
#include "Removals.h"
#include "StatsTracker.h"
#include "WaypointSystem.h"
#include "CWeatherController.h"

LevelInitPostEntityFn oLevelInitPostEntity;

//ToDo: imi cvar
void __fastcall Hooks::LevelInitPostEntity(void* pclient)
{
    // grab the skyname when we load the map to save the default for later
    //PlayerVisuals->GrabDefaultSky();

    // apply night mode/modulation changes on map-change/load
	g_Visuals.GrabMaterials();
	g_Visuals.HandleMaterialModulation();

	// force no smoke settings
	g_Removals.DoNoSmoke(true);

	g_Ragebot.ClearAimbotTarget();
	g_Ragebot.ClearIgnorePlayerIndex();
	g_Ragebot.ClearPossibleTargets();
	LocalPlayer.IsInCompetitive = !GetGamerules() || GetGamerules()->IsValveServer();
	g_Info.m_iServerTickrate = int(1.f / Interfaces::Globals->interval_per_tick);

	//g_WeatherController.OnLevelChanged();

    //DoSkyChanger();

	g_StatsTracker.clear();

	ReadWaypoints();

    return oLevelInitPostEntity(pclient);
}