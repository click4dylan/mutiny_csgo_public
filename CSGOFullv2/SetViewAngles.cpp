#include "VTHook.h"
#include "Interfaces.h"
#include <intrin.h> //VS2017 requires this for _ReturnAddress
#if 0
//Credits: https://github.com/Leystryku/Nikyuria/blob/master/src/hooks.cpp

SetViewAnglesFn oSetViewAngles;

struct stack
{
	stack *next;
	char  *ret;

	template<typename T> inline T arg(unsigned int i)
	{
		return *(T *)((void **)this + i + 2);
	}
};

#define getebp() stack *_bp; __asm mov _bp, ebp;

void __stdcall Hooks::SetViewAngles(QAngle &va)
{
	getebp();

	CUserCmd* pCmd;
	__asm mov pCmd, esi

	unsigned long sebp;
	__asm mov sebp, ebp

	int *sequence_number = *(int**)(sebp)+0x2;

	static void* right_SetViewAngles = 0;
	if (!right_SetViewAngles)
	{
		if (/*pCmd->command_number != *sequence_number || pCmd->command_number < 0x12C || pCmd->command_number > 0xF0000 ||*/ !CurrentUserCmd || CurrentUserCmd.cmd->command_number < 0x12C || CurrentUserCmd.cmd->command_number > 0xF0000 || !Interfaces::Engine->IsInGame())
		{
			oSetViewAngles(va);
			return;
		}

		right_SetViewAngles = _ReturnAddress();
		oSetViewAngles(va);
		return;
	}

	if (_ReturnAddress() != right_SetViewAngles)
	{
		HookAllBaseEntityNonLocalPlayers();

#ifndef _CSGO
		//if (pCmd->hasbeenpredicted && menu->wep_norecoil)
		//	return;
#endif

		oSetViewAngles(va);
		return;
	}

	oSetViewAngles(va);

}
#endif