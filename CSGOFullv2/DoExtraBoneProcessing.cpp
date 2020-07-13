#include "precompiled.h"
#include "BaseEntity.h"
#include "GameMemory.h"
#include "CPlayerrecord.h"
#include "LocalPlayer.h"

void __fastcall HookedDoExtraBoneProcessing(CBaseEntity* thisptr, DWORD edx, CStudioHdr *hdr, Vector *pos, Quaternion *q, matrix3x4_t* bonearray, byte* computed, DWORD m_pIK)
{
	if (!LocalPlayer.Entity || !LocalPlayer.Entity->GetAlive())
	{
		using DoExtraBoneProcessingFn = void(__thiscall*)(CBaseEntity*, CStudioHdr*, Vector*, Quaternion*, matrix3x4_t*, byte*, DWORD);
		DoExtraBoneProcessingFn oDoExtraBoneProcessing = nullptr;
		auto record = g_LagCompensation.GetPlayerrecord(thisptr);
		if (record->Hooks.m_bHookedClientRenderable)
		{
			oDoExtraBoneProcessing = (DoExtraBoneProcessingFn)record->Hooks.BaseEntity->GetOriginalHookedSub3();
			oDoExtraBoneProcessing(thisptr, hdr, pos, q, bonearray, computed, m_pIK);
		}
	}

	//Set RunningFakeAngleBones to show foot plant animation
	bool RunningFakeAngleBones = LocalPlayer.RunningFakeAngleBones;

	if (!RunningFakeAngleBones && thisptr->GetEffects() & EF_NOINTERP)
		return;

	C_CSGOPlayerAnimState* _animstate = thisptr->GetPlayerAnimState();
	CBaseEntity* _pOwner = nullptr; 
	int _iBackupTickcount;

	if (!RunningFakeAngleBones && _animstate)
	{
		_pOwner = _animstate->pBaseEntity;
		//_animstate->pBaseEntity = nullptr;
		_iBackupTickcount = _animstate->m_iSomeTickcount; //state + 8
		_animstate->m_iSomeTickcount = 0;
	}

	using DoExtraBoneProcessingFn = void(__thiscall*)(CBaseEntity*, CStudioHdr*, Vector*, Quaternion*, matrix3x4_t*, byte*, DWORD);
	DoExtraBoneProcessingFn oDoExtraBoneProcessing = nullptr;
	auto record = g_LagCompensation.GetPlayerrecord(thisptr);
	if (record->Hooks.m_bHookedClientRenderable)
	{
		oDoExtraBoneProcessing = (DoExtraBoneProcessingFn)record->Hooks.BaseEntity->GetOriginalHookedSub3();
		oDoExtraBoneProcessing(thisptr, hdr, pos, q, bonearray, computed, m_pIK);
	}

	if (!RunningFakeAngleBones && _animstate)
		_animstate->m_iSomeTickcount = _iBackupTickcount;
		//_animstate->pBaseEntity = _pOwner;
}
