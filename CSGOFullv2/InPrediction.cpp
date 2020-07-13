#include "precompiled.h"
#include "VTHook.h"
#include "LocalPlayer.h"
#include "Adriel/stdafx.hpp"

bool __fastcall Hooks::new_InPrediction(void* ecx, void* edx)
{
	if ((DWORD)_ReturnAddress() == StaticOffsets.GetOffsetValue(_MaintainSequenceTransitionsReturnAddress))
		return true;

	//decrypts(0)
	static ConVar* weapon_recoil_scale = Interfaces::Cvar->FindVar(XorStr("weapon_recoil_scale"));
	static ConVar* view_recoil_tracking = Interfaces::Cvar->FindVar(XorStr("view_recoil_tracking"));
	//encrypts(0)

	// no vis recoil is wanted
	if (*reinterpret_cast<uint32_t*>(_ReturnAddress()) == 0x875c084 && variable::get().visuals.b_no_visual_recoil &&
		LocalPlayer.Entity)
	{
		const auto oldEBP = *reinterpret_cast<uint32_t*>((uint32_t)_AddressOfReturnAddress() - 4);
		float* LocalView = *reinterpret_cast<float**>(oldEBP + 0xC);

		const auto ViewPunch = LocalPlayer.Entity->GetViewPunch();
		const auto Punch = LocalPlayer.Entity->GetPunch();

		const auto RecoilScale = weapon_recoil_scale->GetFloat();
		const auto RecoilTracking = view_recoil_tracking->GetFloat();

		LocalView[0] -= ViewPunch.x + Punch.x * RecoilScale * RecoilTracking;
		LocalView[1] -= ViewPunch.y + Punch.y * RecoilScale * RecoilTracking;
		LocalView[2] -= ViewPunch.z + Punch.z * RecoilScale * RecoilTracking;

		return true;
	}

	// call original
	return oInPrediction(ecx, edx);
}
