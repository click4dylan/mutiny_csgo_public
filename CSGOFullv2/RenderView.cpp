#include "precompiled.h"
#include "CSGO_HX.h"
#include "CViewSetup.h"
#include "VTHook.h"
#include "Overlay.h"

RenderViewFn oRenderView;

void __fastcall Hooks::RenderView(void* thisptr, void* edx, CViewSetup &setup, CViewSetup &hudViewSetup, int nClearFlags, int whatToDraw)
{
	//ToDo: imi cvar
	//if (ViewModelFOVChangerTxt.flValue != 68.0f)
	{
	//	WriteFloat((uintptr_t)&setup.fovViewmodel, ViewModelFOVChangerTxt.flValue);
	}
	oRenderView(thisptr, setup, hudViewSetup, nClearFlags, whatToDraw);
}