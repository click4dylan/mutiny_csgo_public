#include "precompiled.h"
#include "VTHook.h"
#include "LocalPlayer.h"
#include "Reporting.h"
#include "TickbaseExploits.h"
#include "INetchannelInfo.h"

PacketStartFn oPacketStart;
PacketEndFn oPacketEnd;

int nChokedTicks;
int ShouldGetChokedTicks = 0;

int __fastcall OnPacketStart(void* messagehandler, DWORD edx, int outgoing_acknowledged, int incoming_sequence)
{
	return 1;

#if 0
	LocalPlayer.Get(&LocalPlayer);
	if (!Interfaces::EngineClient->IsInGame() || !LocalPlayer.Entity || !LocalPlayer.Entity->GetAlive())
		return 1;

	if (Helper_GetLastCompetitiveMatchId() || !GetGamerules() || GetGamerules()->IsValveServer())
		return 1;

	LocalPlayer.m_UserCommandMutex.Lock();

	auto &cmds = LocalPlayer.m_vecUserCommands;
	for (auto cmd = cmds.begin(); cmd != cmds.end(); ++cmd)
	{
		if (*cmd == outgoing_acknowledged)
		{
			cmds.erase(cmds.begin(), cmd + 1);
			LocalPlayer.m_UserCommandMutex.Unlock();
			return 1;
		}
	}

	LocalPlayer.m_UserCommandMutex.Unlock();
#endif
	return 0;
}

__declspec(naked) void __fastcall Hooks::PacketStart(void* messagehandler, DWORD edx, int incoming_sequence, int outgoing_acknowledged)
{
	__asm
	{
		mov eax, ShouldGetChokedTicks
		test eax, eax
		jz justcall
		mov eax, dword ptr ss : [esp - 0x30]; //-0xC also works
		mov nChokedTicks, eax
		justcall:
		push ecx
		push [esp + 8]
		push [esp + 16]
		call OnPacketStart
		pop ecx
		test eax, eax
		jz dontpacketstart
		dopacketstart:
		jmp oPacketStart
		dontpacketstart:
		ret 8
	}
}

void __fastcall Hooks::PacketEnd(void* messagehandler)
{
	// Did we get any messages this tick (i.e., did we call PreEntityPacketReceived)?
	if (g_ClientState->m_ClockDriftMgr.m_nServerTick != g_ClientState->m_nDeltaTick)
	{
		oPacketEnd(messagehandler);
		return;
	}
	// How many commands total did we run this frame
	int commands_acknowledged = g_ClientState->command_ack - g_ClientState->last_command_ack; 

	// Highest command parsed from messages
	//last_command_ack = command_ack;

	// Let prediction copy off pristine data and report any errors, etc.
	//g_pClientSidePrediction->PostNetworkDataReceived(commands_acknowledged);


	g_Tickbase.OnPacketEnd(commands_acknowledged);

	oPacketEnd(messagehandler);
}