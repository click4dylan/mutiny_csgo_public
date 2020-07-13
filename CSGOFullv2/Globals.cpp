#include "precompiled.h"
#include "Globals.h"

namespace Settings
{
	WNDPROC oWndProc = 0;
	HWND Window = 0;
	int Configs = 0;
	bool Init = false;
	bool MenuOpened = false;
	float NextTimeCanPlaySound;
	float Time = FLT_MIN;
}
