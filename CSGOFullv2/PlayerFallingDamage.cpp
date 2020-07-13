#include "precompiled.h"
#include "Includes.h"
#include "VTHook.h"
#include "LocalPlayer.h"

PlayerFallingDamageFn oPlayerFallingDamage;

void __fastcall Hooks::PlayerFallingDamage(void* ecx)
{
	oPlayerFallingDamage(ecx);
	LocalPlayer.CalledPlayerHurt = true;
}