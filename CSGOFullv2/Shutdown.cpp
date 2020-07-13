#include "VTHook.h"
#include "Interfaces.h"
#include "FarESP.h"
#include "ServerSide.h"
#include "GlobalInfo.h"
#include "WaypointSystem.h"

#include "Adriel/stdafx.hpp"

ShutdownFn oShutdown;

void __fastcall Hooks::ShutdownNetchan(void* netchan, void* edx, int code, const char* reason)
{
	delete HMessageHandler;
	delete HNetchan;
	HNetchan = nullptr;
	HMessageHandler = nullptr;

	static DWORD ConstructorFn = 0;
	static DWORD SetTextFn;
	static DWORD WriteToBufferFn;
	static DWORD DeconstructorFn;

	INetChannel* chan = (INetChannel*)netchan;
	if (0 && chan->m_Socket >= 0 && !*(DWORD *)((DWORD)netchan + 0x108))
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
		char v17[4]; // [esp+20h] [ebp-34h]
		char v18[48]; // [esp+24h] [ebp-30h]
		reinterpret_cast<void(__thiscall*)(void*)>(ConstructorFn)(v17);
		reinterpret_cast<void(__thiscall*)(void*, const char*)>(SetTextFn)(v18, reason);
		reinterpret_cast<void(__thiscall*)(void*, void*)>(WriteToBufferFn)(v17, &chan->m_StreamUnreliable);
		chan->Transmit(false);
		reinterpret_cast<void(__thiscall*)(void*)>(DeconstructorFn)(v17);
	}
	//const char* newreason = variable::get().misc.disconnect_reason.str_text.c_str();
	oShutdownNetchan(netchan, edx, code, reason);
}

void __fastcall Hooks::Shutdown(void* pclient)
{
	ClearAllWaypoints();
	g_bIsExiting = true;
	g_Info.LevelisLoaded = false;

#if defined(USE_FAR_ESP) || defined(USE_SERVER_SIDE)
	//If not connected, shut down all far esp/server side connections
#ifdef USE_FAR_ESP
	//if (variable::get().visuals.pf_enemy.b_faresp)
	{
		if (g_FarESP.IsSocketCreated() && !g_FarESP.ShouldExit())
			g_FarESP.SetShouldExit(true);
		g_FarESP.ClearClientToServerData();
	}
#endif
#ifdef USE_SERVER_SIDE
		if (pServerSide.IsSocketCreated() && !pServerSide.ShouldExit())
			pServerSide.SetShouldExit(true);
#endif
#endif
#if !(defined NO_MALLOC_OVERRIDE) && !(defined NO_MEMOVERRIDE_NEW_DELETE)
		__fastfail(FAST_FAIL_UNEXPECTED_CALL);
	//TerminateProcess(GetCurrentProcess(), EXIT_SUCCESS);
#endif

#ifdef _DEBUG
	int leaks = _CrtDumpMemoryLeaks();
	int f = 0;
#endif
	oShutdown(pclient);
}