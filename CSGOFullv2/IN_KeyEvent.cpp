#include "precompiled.h"
#include "VTHook.h"

IN_KeyEventFn oIN_KeyEvent;

int __fastcall Hooks::new_IN_KeyEvent(void* ecx, void* edx, int eventcode, ButtonCode_t keynum, const char *pszCurrentBinding)
{
	// swallow the gamekey
	//if (eventcode && g_Menu.m_bMenuOpen)
	//	return 0;

	//ToDo: drop Antiaim;

	return oIN_KeyEvent(ecx, edx, eventcode, keynum, pszCurrentBinding);
}
