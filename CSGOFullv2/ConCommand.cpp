#include "utlvector.h"
#include "UtlString.hpp"
#include "GameMemory.h"
#include "ConCommand.h"
#include "ISteamNetworkingUtils.h"
#include "CClientState.h"
#include "VTHook.h"
#include "Interfaces.h"
#include "Adriel/variable.hpp"

std::string g_DesiredRelayCluster = "";

ConCommand::ConCommand(const char *pName, FnCommandCallbackVoid_t callback, const char *pHelpString, int flags, FnCommandCompletionCallback completionFunc)
{
	typedef void(__thiscall *ConCommandConstructorFn)(void*, const char*, FnCommandCallbackVoid_t, const char*, int flags, FnCommandCompletionCallback);

	static auto Constructor = (ConCommandConstructorFn)FindMemoryPattern(ClientHandle, "55  8B  EC  8B  45  0C  56  8B  F1  89");

	Constructor(this, pName, callback, pHelpString, flags, completionFunc);

	m_bCallbackType = TYPE_VERSION1_ARG; //TYPE_VERSION1;
}

ConCommand* RegisterConCommand(const char* name, void* oncalled, void* oncomplete)
{
	//static DWORD init = FindMemoryPattern(EngineHandle, "8B  D1  8B  0D  ??  ??  ??  ??  85  C9  74  05  8B  01  52  FF  10  C3");
	//static DWORD vtable = *(DWORD*)(FindMemoryPattern(EngineHandle, "C7  06  ??  ??  ??  ??  0A  D0") + 2);
	//static ConCommandBase** concommandbase = (ConCommandBase**)*(DWORD*)(FindMemoryPattern(EngineHandle, "89  35  ??  ??  ??  ??  74  05  E8  ??  ??  ??  ??  8B  C6  5E  5D  C2  14  00") + 2);
	//static IConCommandBaseAccessor** accessor = (IConCommandBaseAccessor**)*(DWORD*)(FindMemoryPattern(EngineHandle, "83  3D  ??  ??  ??  ??  00  88  56  20  C7  46  0C") + 2);

	return new ConCommand(name, (FnCommandCallbackVoid_t)oncalled);
}

using MsgFn = void(__stdcall*)(const char* pMsg, ...);
using WarningFn = void(__stdcall*)(const char* pMsg, ...);

void Command_SDR(const CCommand& cmd)
{
	static MsgFn Msg = (MsgFn)(GetProcAddress(GetModuleHandleA("tier0.dll"), "Msg"));
	static WarningFn Warning = (WarningFn)(GetProcAddress(GetModuleHandleA("tier0.dll"), "Warning"));

	ESteamNetworkingConfigValue v1; // edx
	ESteamNetworkingConfigValue v2; // esi
	int v3; // edi
	const char *v4; // edx
	char *v6; // eax
	char v7; // cl
	const char *v8; // [esp+5Ch] [ebp-418h]
	ESteamNetworkingConfigValue i; // [esp+60h] [ebp-414h]
	size_t v10; // [esp+64h] [ebp-410h]
	int v11; // [esp+68h] [ebp-40Ch]
	char buffer[512]; // [esp+6Ch] [ebp-408h]
	int a3; // [esp+274h] [ebp-200h]

	if (SteamNetworkingUtils())
	{
		if (cmd.m_nArgc == 1)
		{
			Msg("Usage: sdr <setting> [<value>]\n");
			Msg("Available settings:\n");
			v1 = SteamNetworkingUtils()->GetFirstConfigValue();
			for (i = v1; i; v1 = i)
			{
				v8 = 0;
				if (!SteamNetworkingUtils()->GetConfigValueInfo(v1, (const char **)&v8, 0, 0, (ESteamNetworkingConfigValue *)&i))
					break;
				if (!v8)
					break;
				Msg("\t%s\n", v8);
			}
		}
		else
		{
			v2 = SteamNetworkingUtils()->GetFirstConfigValue();
			while (1)
			{
				while (1)
				{
					if (!v2
						|| (v8 = 0,
							!SteamNetworkingUtils()->GetConfigValueInfo(v2, (const char **)&v8, 0, 0, (ESteamNetworkingConfigValue *)&i))
						|| !v8)
					{
						Msg("sdr config option not found\n");
						return;
					}
					v3 = cmd.m_nArgc;
					v4 = cmd[1];
					if (!V_strcasecmp(v8, v4))
						break;
					v2 = i;
				}
				if (v3 > 2)
					break;
				v10 = 512;
				if (SteamNetworkingUtils()->GetConfigValue(v2, (ESteamNetworkingConfigScope)1, 0, (ESteamNetworkingConfigDataType*)&v11, &a3, &v10) == 1)
				{
					switch (v11)
					{
					case 1:
						sprintf(buffer, "%d", a3);
						break;
					case 2:
						sprintf(buffer, "%lld", a3);
						break;
					case 3:
						sprintf(buffer, "%g", a3);
						break;
					case 4:
						v6 = buffer;
						do
						{
							v7 = v6[(char *)&a3 - buffer];
							if (!v7)
								break;
							*v6++ = v7;
						} while (v6 < &buffer[511]);
						*v6 = 0;
						break;
					}
					Msg("%s %s = %s\n", "sdr", v8, buffer);
					return;
				}
			}
			if (!SteamNetworkingUtils()->SetConfigValue(v2, (ESteamNetworkingConfigScope)1, 0, (ESteamNetworkingConfigDataType)4, cmd.m_ppArgv[2]))
				Warning("Failed to set SteamNetSockets option '%s'\n", v8);
		}
	}
	else
	{
		Warning("No ISteamNetworkingUtils");
	}
}

void Command_ResetCookie(const CCommand& cmd)
{
	oSetReservationCookie((void*)((DWORD)g_ClientState + 8), 0);
}

extern void StartClientState_ProcessPacketEntities();
extern int SIMULATE_CMDS_TWICE;
extern bool manuallystoppedcommunications;
extern bool stopfreezing;
extern float totaltimefrozen;
extern float totaltimeunfrozen;

void Command_RetryValve(const CCommand& cmd)
{
	oSetReservationCookie((void*)((DWORD)g_ClientState + 8), g_ServerReservationCookie);
	Interfaces::EngineClient->ClientCmd_Unrestricted("retry", 0);

	StartClientState_ProcessPacketEntities();
	SIMULATE_CMDS_TWICE = 0;
	manuallystoppedcommunications = false;

	stopfreezing = false;
	totaltimefrozen = 0.0f;
	totaltimeunfrozen = 0.0f;
}

void Command_SetRelay(const CCommand& cmd)
{
	static MsgFn Msg = (MsgFn)(GetProcAddress(GetModuleHandleA("tier0.dll"), "Msg"));
	static WarningFn Warning = (WarningFn)(GetProcAddress(GetModuleHandleA("tier0.dll"), "Warning"));
	ESteamNetworkingConfigValue v1; // edx
	ESteamNetworkingConfigValue v2; // esi
	int v3; // edi
	const char *v4; // edx
	char *v6; // eax
	char v7; // cl
	const char *v8; // [esp+5Ch] [ebp-418h]
	ESteamNetworkingConfigValue i; // [esp+60h] [ebp-414h]
	size_t v10; // [esp+64h] [ebp-410h]
	int v11; // [esp+68h] [ebp-40Ch]
	char buffer[512]; // [esp+6Ch] [ebp-408h]
	int a3; // [esp+274h] [ebp-200h]

	if (SteamNetworkingUtils())
	{
		v2 = SteamNetworkingUtils()->GetFirstConfigValue();
		while (1)
		{
			while (1)
			{
				if (!v2
					|| (v8 = 0,
						!SteamNetworkingUtils()->GetConfigValueInfo(v2, (const char **)&v8, 0, 0, (ESteamNetworkingConfigValue *)&i))
					|| !v8)
				{
					Msg("sdr config option not found\n");
					return;
				}
				if (!V_strcasecmp(v8, "SDRClient_ForceRelayCluster"))
					break;
				v2 = i;
			}
			if (cmd.m_nArgc > 1)
				break;
			v10 = 512;
			if (SteamNetworkingUtils()->GetConfigValue(v2, (ESteamNetworkingConfigScope)1, 0, (ESteamNetworkingConfigDataType*)&v11, &a3, &v10) == 1)
			{
				switch (v11)
				{
				case 1:
					sprintf(buffer, "%d", a3);
					break;
				case 2:
					sprintf(buffer, "%lld", a3);
					break;
				case 3:
					sprintf(buffer, "%g", a3);
					break;
				case 4:
					v6 = buffer;
					do
					{
						v7 = v6[(char *)&a3 - buffer];
						if (!v7)
							break;
						*v6++ = v7;
					} while (v6 < &buffer[511]);
					*v6 = 0;
					break;
				}
				Msg("%s %s = %s\n", "sdr", v8, buffer);
				return;
			}
		}
		if (!SteamNetworkingUtils()->SetConfigValue(v2, (ESteamNetworkingConfigScope)1, 0, (ESteamNetworkingConfigDataType)4, cmd.m_ppArgv[1]))
			Warning("Failed to set SteamNetSockets option '%s'\n", v8);
		else
		{
			g_DesiredRelayCluster = cmd.m_ppArgv[1];
			Msg("Successfully forced relay cluster to '%s'\n", cmd.m_ppArgv[1]);
		}
	}
	else
	{
		Warning("No ISteamNetworkingUtils");
	}
}