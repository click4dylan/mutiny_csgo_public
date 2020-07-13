#include "precompiled.h"
#include "VTHook.h"
#include "Draw.h"

OnScreenSizeChangedFn oOnScreenSizeChanged;

//ToDo: imi cvar
void __fastcall Hooks::hkOnScreenSizeChanged(void* _this, void* edx, int nOldWidth, int nOldHeight)
{
	if (oOnScreenSizeChanged)
		oOnScreenSizeChanged(_this, nOldWidth, nOldHeight);

	//Setup Fonts
	SetupFonts();

   // PlayerVisuals->HandleMaterialModulation();
}