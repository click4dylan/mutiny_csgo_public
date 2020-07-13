#include "precompiled.h"
#include "VTHook.h"
#include "LocalPlayer.h"

SceneEndFn oSceneEnd;

void __fastcall Hooks::Hooked_SceneEnd(IVRenderView* ecx, void* edx)
{
	//LocalPlayer.NetvarMutex.Lock();

	//LocalPlayer.SnapAttachmentsToCurrentPosition();

	// draw bullettracers
	g_Visuals.DrawBulletTracers();
	
	// call original
	oSceneEnd(ecx, edx);

	//LocalPlayer.NetvarMutex.Unlock();
}
