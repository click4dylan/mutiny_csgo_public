#include "precompiled.h"
#include "Includes.h"
#include "VTHook.h"
#include "LocalPlayer.h"
#include "CWeatherController.h"

vcollide_t* __fastcall Hooks::GetVCollide(void* ecx, DWORD edx, int index)
{
	vcollide_t* w = g_WeatherController.GetVCollideByModelIndex(index);
	return w ? w : oGetVCollide(ecx, index);
}