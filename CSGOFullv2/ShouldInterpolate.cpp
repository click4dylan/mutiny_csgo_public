#include "precompiled.h"
#include "BaseEntity.h"
#include "GameMemory.h"
#include "LocalPlayer.h"
#include "CPlayerrecord.h"

BOOLEAN __fastcall HookedEntityShouldInterpolate(CBaseEntity* _Entity)
{
	LocalPlayer.Get(&LocalPlayer);

	if (_Entity == LocalPlayer.Entity)
		return TRUE;

	if (_Entity->IsPlayer())
	{
		return FALSE;

		//CPlayerrecord* _playerRecord = &m_PlayerRecords[_Entity->index];
		//if (_playerRecord->Hooks.BaseEntity && _playerRecord->Hooks.BaseEntity->GetOriginalHookedSub1())
		//	return ((BOOLEAN(__thiscall*)(CBaseEntity*))_playerRecord->Hooks.BaseEntity->GetOriginalHookedSub1())(_Entity);
	}

	return FALSE;
}
