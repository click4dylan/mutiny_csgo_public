#include "precompiled.h"
#include "CreateMove.h"
#include "CSGO_HX.h"
#include "Overlay.h"
#include "BaseCombatWeapon.h"
#include "Targetting.h"
#include "AutoWall.h"
#include "ThirdPerson.h"
#include "LocalPlayer.h"
#include "VMProtectDefs.h"
#include "NetVars.h"
#include "Draw.h"
#include "ESP.h"
#include "FarESP.h"
#include "ServerSide.h"
#include "C_CSGameRules.h"
#include "CPlayerrecord.h"
#include "Removals.h"
#include "Events.h"
#include "GlobalInfo.h"
#include "ICollidable.h"
#include "IClientNetworkable.h"
#include "misc.h"
#include "CWeatherController.h"
#include "ConCommand.h"

#include "Adriel/asuswall.hpp"
#include "Adriel/clantag_changer.hpp"

FrameStageNotifyFn oFrameStageNotify;
ClientFrameStage_t LastFramestageNotifyStage;
bool AllowCustomCode = false;

#include <ctime>

extern void RestoreVACCedPointers();
extern void UpdateCustomNetchannels();
extern void HookGamerules();
extern bool ServerCrashing;
extern bool FuckingServer;

void OutputSequencesAndAnimations(CBaseEntity* Entity)
{
	std::ofstream output_animations("G:\\sequence_names.txt", std::ios::trunc);
	if (output_animations.is_open())
	{
		output_animations << "enum SequenceNames : int\n{\n";
		for (int i = 0; ; i++)
		{
			const char* name = GetSequenceName(Entity, i);
			output_animations << name;
			if (!strcmp(name, "Unknown"))
			{
				output_animations << "\n";
				break;
			}
			output_animations << " = " << i << ",\n";
		}
		output_animations << "};" << std::endl;
	}
	std::ofstream output_sequence_activities("G:\\sequence_activities.txt", std::ios::trunc);
	if (output_sequence_activities.is_open())
	{
		output_sequence_activities << "Sequence Activities:\n";
		for (int i = 0; i < 1024; i++)
		{
			output_sequence_activities << i << " = " << GetSequenceActivity(Entity, i) << "\n";
		}
	}
	std::ofstream output_sequence_activity_names("G:\\sequence_activity_names.txt", std::ios::trunc);
	if (output_sequence_activity_names.is_open())
	{
		output_sequence_activity_names << "Sequence Activity Names:\n";
		for (int i = 0; i < 1024; i++)
		{
			const char* name = GetSequenceActivityNameForModel(Entity->GetModelPtr(), i);
			if (name)
			{
				output_sequence_activity_names << i << " = " << name << "\n";
			}
			else
			{
				output_sequence_activity_names << i << " = " << "Unknown" << "\n";
			}
		}
		output_sequence_activity_names.close();
	}
	std::ofstream activity_list("G:\\activity_list.txt", std::ios::trunc);
	if (activity_list.is_open())
	{
		activity_list << "enum Activities : int\n{\nACT_INVALID = -1,\n";
		for (int i = 0; ; i++)
		{
			const char* name = ActivityList_NameForIndex(i);
			if (!name)
			{
				activity_list << "ACTIVITY_LIST_MAX\n";
				break;
			}
			activity_list << name << " = " << i << ",\n";
		}
		activity_list << "};" << std::endl;
	}
}

#if 0
	if (precip && !created_rain && precip->m_pCreateFn)
	{
		RecvProp* prop = nullptr;
		NetVarManager->Get_Prop(precip->m_pRecvTable, "m_nPrecipType", &prop);

		if (prop)
		{
			IClientNetworkable* rainNetworkable = ((IClientNetworkable*(*)(int, int))precip->m_pCreateFn)(MAX_EDICTS - 1, 0);
			if (rainNetworkable)
			{
				CBaseEntity* rainEntity = rainNetworkable->GetIClientUnknown()->GetBaseEntity();

				rainNetworkable->PreDataUpdate(DATA_UPDATE_CREATED);
				rainNetworkable->OnPreDataChanged(DATA_UPDATE_CREATED);

				*(int*)((uintptr_t)rainEntity + prop->GetOffset()) = 3;
				rainEntity->SetMins(Vector(-32768, -32768, -32768));
				rainEntity->SetMaxs(Vector(32768, 32768, 32768));

				auto collision = rainEntity->GetCollideable();
				if (collision)
				{
					(Vector&)collision->OBBMins() = rainEntity->GetMins();
					(Vector&)collision->OBBMaxs() = rainEntity->GetMaxs();
				}

				rainEntity->OnDataChanged(DATA_UPDATE_CREATED);
				rainEntity->PostDataUpdate(DATA_UPDATE_CREATED);
			}
			created_rain = true;
		}
	}
#endif

void __fastcall Hooks::FrameStageNotify(void* ecx, void*, ClientFrameStage_t stage)
{
#ifdef _DEBUG
	if (GetAsyncKeyState(VK_PAUSE))
	{
		int leaks = _CrtDumpMemoryLeaks();
		printf("%i memory leaks \n", leaks);

#if 0
		m_MemoryAllocationMutex.lock();
		char tmp[256];
		for (auto _thread = m_MemoryAllocationTable.begin<MemoryInfo_t>(); _thread != m_MemoryAllocationTable.end<MemoryInfo_t>(); ++_thread)
		{
			sprintf(tmp, "leaked %llu bytes at 0x%08x from 0x%08x\n", _thread->size, _thread->ptr, _thread->returnaddress);
			OutputDebugStringA(tmp);
		}
		m_MemoryAllocationMutex.unlock();
#endif
	}
#endif

	if (sizeof(C_CSGOPlayerAnimState) != 836)
		DebugBreak(); //FIX ME IF THIS HAPPENS ASAP

#ifdef _DEBUG
	if (GetAsyncKeyState(VK_PAUSE))
	{
		int leaks = _CrtDumpMemoryLeaks();
		int f = 0;
	}
#endif

	LastFramestageNotifyStage = stage;

	bool _InGame = Interfaces::EngineClient->IsInGame() && LocalPlayer.Entity;

	//if (LocalPlayer.GetLocalPlayer() && LocalPlayer.GetLocalPlayer()->GetHealth() > 100)
	{
		//printf("test optimizer\n");
	}

	//Hook this asap
#ifdef DYLAN_VAC
	Hooks::HookLevelShutdownAndInitPreEntity();
	//Hooks::HookModelRenderGlow();
#endif

	//Unhook netchannel if it changed - ToDo: create function for this
	if (!g_ClientState->m_pNetChannel)
	{
		if (HNetchan)
		{
			HNetchan->ClearClassBase();
			delete HNetchan;
			HNetchan = nullptr;
		}
	}
	else
	{
		if (HNetchan)
		{
			//Update our rebuilt netchannels (client and server)
			UpdateCustomNetchannels();
		}
	}

	//Hook/rehook gamerules and thirdperson
	HookGamerules();

	/* I am sorry for not following the normal way */

	asuswall::get().fsn(stage);

	if (!_InGame)
		clantag_changer::get().clear();

	/* ******************************************* */

	// pre original
	switch (stage)
	{
	case FRAME_START:
	{
		*s_bEnableInvalidateBoneCache = false;
		static bool done = false;
		if (!done)
		{
			RegisterConCommand("sdr", &Command_SDR, nullptr);
			RegisterConCommand("setrelay", &Command_SetRelay, nullptr);
			RegisterConCommand("resetcookie", &Command_ResetCookie, nullptr);
			RegisterConCommand("retry2", &Command_RetryValve, nullptr);
			done = true;
		}
		break;
	}
	case FRAME_NET_UPDATE_START:
	{
		// lock mutex
		//Interfaces::MDLCache->BeginLock();
		LocalPlayer.NetvarMutex.Lock();

#if 0
		if (GetAsyncKeyState(VK_RETURN))
		{
			g_WeatherController.StopAllWeather();
			for (int i = 0; i <= Interfaces::ClientEntList->GetHighestEntityIndex(); ++i)
			{
				CBaseEntity* bleh = Interfaces::ClientEntList->GetBaseEntity(i);
				if (bleh && bleh->GetClientClass() && bleh->GetClientClass()->m_ClassID == ClassID::_CPrecipitation)
				{
					int modelindex = bleh->GetModelIndex();
					bleh->GetClientNetworkable()->Release();
				}
			}
		}
		else
		{
			int i = 1;
			if (!g_WeatherController.HasSpawnedWeatherOfType(CWeatherController::CClient_Precipitation::PRECIPITATION_TYPE_PARTICLESNOW))
			{
				if (GetAsyncKeyState(VK_F1))
					g_WeatherController.CreateWeather(CWeatherController::CClient_Precipitation::PRECIPITATION_TYPE_PARTICLESNOW);
			}
			if (!g_WeatherController.HasSpawnedWeatherOfType(CWeatherController::CClient_Precipitation::PRECIPITATION_TYPE_PARTICLERAIN))
			{
				if (GetAsyncKeyState(VK_F2))
					g_WeatherController.CreateWeather(CWeatherController::CClient_Precipitation::PRECIPITATION_TYPE_PARTICLERAIN);
			}
			g_WeatherController.UpdateAllWeather();
		}
#endif

		break;
	}
	case FRAME_NET_UPDATE_POSTDATAUPDATE_START:
		// update localplayer
		LocalPlayer.Get(&LocalPlayer);

		// various removals
		if (_InGame)
		{
			g_Removals.DoNoFlash();
			g_Removals.DoNoSmoke();
		}
		break;
	case FRAME_NET_UPDATE_POSTDATAUPDATE_END:
		break;
	case FRAME_NET_UPDATE_END:
		break;
	case FRAME_RENDER_START:
		// lock mutex
		//Interfaces::MDLCache->BeginLock();
		//LocalPlayer.NetvarMutex.Lock();

		// disable interpolation for entities when ingame
		//if (_InGame)
		//	g_LagCompensation.FSN_PreRenderStart();
		break;
	case FRAME_RENDER_END:
		// lock mutex
		//Interfaces::MDLCache->BeginLock();
		//LocalPlayer.NetvarMutex.Lock();
		break;
	default:
		break;
	}

	// call original function
	oFrameStageNotify(ecx, stage);

	RestoreVACCedPointers();

	// post original call
	switch (stage)
	{
	case FRAME_START:
	{
#if defined(USE_FAR_ESP) || defined(USE_SERVER_SIDE)
		//If not connected, shut down all far esp/server side connections
		if (!g_ClientState->m_pNetChannel)
		{
#ifdef USE_FAR_ESP
			if (g_FarESP.IsSocketCreated() && !g_FarESP.ShouldExit())
				g_FarESP.SetShouldExit(true);
			g_FarESP.ClearClientToServerData();
#endif
#ifdef USE_SERVER_SIDE
			if (pServerSide.IsSocketCreated() && !pServerSide.ShouldExit())
				pServerSide.SetShouldExit(true);
#endif
		}
#ifdef USE_FAR_ESP
		else if (!g_Info.LevelisLoaded || !_InGame)
		{
			g_FarESP.ClearClientToServerData();
		}
#endif
#endif
		break;
	}
	case FRAME_NET_UPDATE_START:
		break;
	case FRAME_NET_UPDATE_POSTDATAUPDATE_START:
		break;
	case FRAME_NET_UPDATE_POSTDATAUPDATE_END:
		break;
	case FRAME_NET_UPDATE_END:
	{
		LocalPlayer.Get(&LocalPlayer);
		LocalPlayer.ApplyUncompressedNetvars();
		CL_FireEvents();

		static bool done = false;
		if (!done)
		{
			if (LocalPlayer.Entity)
				OutputSequencesAndAnimations(LocalPlayer.Entity);
			done = true;
		}

		// update lagcompensation data for each player

		g_LagCompensation.FSN_UpdatePlayers();
#ifdef USE_FAR_ESP
		if (variable::get().visuals.pf_enemy.b_faresp)
		{
			g_FarESP.OnNetUpdateEnd();
		}
#endif

		//process all queued events after updating all entities
		for (const auto& ev : g_GameEvent_PlayerDeath_Queue)
		{
			GameEvent_PlayerDeath_ProcessQueuedEvent(ev);
		}
		for (const auto& ev : g_GameEvent_PlayerHurt_Queue)
		{
			GameEvent_PlayerHurt_PreProcessQueuedEvent(ev);
		}
		for (const auto& ev : g_GameEvent_Impact_Queue)
		{
			GameEvent_Impact_ProcessQueuedEvent(ev);
		}
		for (const auto& ev : g_GameEvent_PlayerHurt_Queue)
		{
			GameEvent_PlayerHurt_ProcessQueuedEvent(ev);
		}
		g_GameEvent_Impact_Queue.clear();
		g_GameEvent_PlayerHurt_Queue.clear();
		g_GameEvent_PlayerDeath_Queue.clear();


		// unlock mutex
		LocalPlayer.NetvarMutex.Unlock();
		break;
	}
	case FRAME_RENDER_START:
		// unlock mutex
		//LocalPlayer.NetvarMutex.Unlock();

		// re-enable interpolation for entities
		//g_LagCompensation.FSN_PostRenderStart();
		break;
	case FRAME_RENDER_END:
		// unlock mutex
		//LocalPlayer.NetvarMutex.Unlock();
		break;
	default:
		break;
	}
}