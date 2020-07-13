#include "precompiled.h"
#include "Removals.h"
#include "LocalPlayer.h"
#include "Adriel/stdafx.hpp"

CRemovals g_Removals;

RecvVarProxyFn OriginalSmokeProxy = 0;

NETVARPROXY(SmokeProxy)
{	
	if (variable::get().visuals.b_no_smoke)
		*(bool*)((DWORD)pOut + 0x1) = true;

	if (OriginalSmokeProxy)
		OriginalSmokeProxy(pData, pStruct, pOut);
}

void CRemovals::DoNoRecoil(QAngle& Angles) const
{
	if(variable::get().legitbot.aim.b_enabled.b_state && !variable::get().ragebot.b_enabled)
	{
		if (!LocalPlayer.WeaponVars.IsGun || !LocalPlayer.WeaponVars.IsFullAuto || LocalPlayer.WeaponVars.IsSniper || LocalPlayer.WeaponVars.IsShotgun)
			return;
	}

	//decrypts(0)
	static ConVar* weapon_recoil_scale = Interfaces::Cvar->FindVar(XorStr("weapon_recoil_scale"));
	//encrypts(0)
	Angles -= LocalPlayer.Entity->GetPunch() * (weapon_recoil_scale ? weapon_recoil_scale->GetFloat() : 2.f);
}

void CRemovals::AddRecoil(QAngle& Angles) const
{
	//decrypts(0)
	static ConVar* weapon_recoil_scale = Interfaces::Cvar->FindVar(XorStr("weapon_recoil_scale"));
	//encrypts(0)
	Angles += LocalPlayer.Entity->GetPunch() * (weapon_recoil_scale ? weapon_recoil_scale->GetFloat() : 2.f);
}

void CRemovals::DoNoFlash()
{
	if(variable::get().visuals.b_no_flash)
		LocalPlayer.Entity->SetFlashDuration(0.f);
}

void CRemovals::DoNoSmoke(bool force)
{
	static bool last_value = false;
	if (force || last_value != variable::get().visuals.b_no_smoke)
	{
		if(!force)
			last_value = variable::get().visuals.b_no_smoke;

		for (auto mat : g_Visuals.SmokeMaterials)
		{
			if (mat)
			{
				mat->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, force ? variable::get().visuals.b_no_smoke : last_value);
			}
		}
	}

	if (variable::get().visuals.b_no_smoke)
	{
		static auto addr = StaticOffsets.GetOffsetValue(_LineGoesThroughSmoke);
		if (addr)
		{
			int* SmokeCount = *(int**)((uintptr_t)addr + 8);
			*SmokeCount = 0;
		}
	}
}