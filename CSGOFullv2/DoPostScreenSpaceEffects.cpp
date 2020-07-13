#include "precompiled.h"
#include "VTHook.h"
#include "Draw.h"

DoPostScreenSpaceEffectsFn oDoPostScreenSpaceEffects;

#pragma optimize("", off)
bool __fastcall Hooks::DoPostScreenSpaceEffects(void* ecx, void* edx, CViewSetup* setup)
{
	//ToDo: fix this
    //PlayerVisuals->GlowAndSpot();
#ifndef IMI_MENU
	g_Visuals.DrawGlow();
#endif

    bool ret = oDoPostScreenSpaceEffects(ecx, setup);

#ifdef DYLAN_VAC
	*ModelRenderTrap = (DWORD)ModelRenderMo->GetOldVT() ^ 0x8F514AC;
	*ModelRenderTrap2 = (DWORD)ModelRenderMo->GetOldVT() ^ 0xBCCD9981;
#endif

	return ret;
}
#pragma optimize("", on)