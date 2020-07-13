#include "precompiled.h"
#include "VTHook.h"
#include "Visuals_imi.h"
#include "LocalPlayer.h"
#include "Spycam.h"

ViewRenderFn oViewRender;

void __fastcall Hooks::View_Render(void* ecx, void* edx, vrect_t* rect)
{
	// draw glow
	//g_Visuals.DrawGlow();

	// sanity check
#ifdef IMI_MENU
	if (**reinterpret_cast<void***>(StaticOffsets.GetOffsetValue(_ViewRender)))
	{
		// get current view
		CViewSetup* view = (CViewSetup*)((DWORD_PTR)**reinterpret_cast<void***>(StaticOffsets.GetOffsetValue(_ViewRender)) + 0x180);

		// do spycam
		g_Spycam.DoSpyCam(ecx, view);
	}
#endif

	// call original
	oViewRender(ecx, edx, rect);
}
