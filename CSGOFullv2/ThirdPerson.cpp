#include "precompiled.h"
#include "VTHook.h"
#include "LocalPlayer.h"
#include "ThirdPerson.h"

CAM_ToFirstPersonFn oCAM_ToFirstPerson;

void SetThirdPersonAngles()
{
	//LocalPlayer.Entity->AddEffect(EF_NOINTERP);
	//LocalPlayer.Entity->SetAbsAngles(LocalPlayer.angAbsRotationAfterUpdatingClientSideAnimation);
	//LocalPlayer.Entity->SetAbsAngles(LocalPlayer.angAbsRotationAfterUpdatingClientSideAnimation);
	//LocalPlayer.Entity->SetAngleRotation(LocalPlayer.angAbsRotationAfterUpdatingClientSideAnimation);
}

void __fastcall Hooks::Hooked_CAM_ToFirstPerson(void* ecx, void*)
{
	//if (g_Convars.Visuals.misc_thirdperson->GetBool() && LocalPlayer.Entity && LocalPlayer.IsAlive)
	//	return;
	return oCAM_ToFirstPerson(ecx);
}
