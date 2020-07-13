#include "precompiled.h"
#include "LocalPlayer.h"
#include "VTHook.h"
#include "Netchan.h"
#include "VMProtectDefs.h"
#include "ProcessPacket.h"
#include "inetmessage.h"

//42

SendNetMsgFn oSendNetMsg;

extern bool bAllowNetTick;

bool __fastcall Hooks::SendNetMsg(void* netchan, void* edx, void* msg, bool bForceReliable, bool bVoice)
{
	//if (((INetMessage*)msg)->GetGroup() == 9) //group 9 is VoiceData
	//	bVoice = true;

	//if (((INetMessage*)msg)->GetType() == 9)
	{
		//bVoice = true;
	}

	//CCLCMsg_BaselineAck_t
	DWORD amsg = (DWORD)msg;
	if (((INetMessage*)msg)->GetType() == 4)
	{
		//return false;

		int type = ((INetMessage*)msg)->GetType();
		int group = ((INetMessage*)msg)->GetGroup();
		int test = 1;

		//if (Interfaces::Globals->tickcount % 2 == 0)
		//	*(int*)(amsg + 0xC) = g_ClientState->m_ClockDriftMgr.m_nServerTick + ((INetChannel*)netchan)->GetLatency(0) + 1;
	}

	
	//disable CRC32 responses to sv_pure servers
#ifdef SERVER_CRASHER
	if (((INetMessage*)msg)->GetType() == 14)
	{
		return false;
	}
#endif

#if 0
	int group = (((INetMessage*)msg)->GetType());
	if (group == 4 && !bAllowNetTick)
		return false;
#endif



	typedef bool(__thiscall* OriginalFn)(DWORD);
	//if (bForceReliable || GetVFunc<OriginalFn>(msg, 0x18 / 4)((DWORD)msg))
	//	LastNetMsgWasReliable = true;

	return oSendNetMsg(netchan, msg, bForceReliable, bVoice);
}