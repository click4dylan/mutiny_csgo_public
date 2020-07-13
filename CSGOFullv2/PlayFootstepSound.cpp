#include "precompiled.h"
#include "BaseEntity.h"
#include "GameMemory.h"
#include "LocalPlayer.h"
//#include "Convars.h"
#include "CPlayerrecord.h"

void __fastcall HookedPlayFootstepSound(CBaseEntity* _Entity, DWORD edx, Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force, bool unknown)
{
	CBaseEntity* _LocalEntity = Interfaces::ClientEntList->GetBaseEntity(Interfaces::EngineClient->GetLocalPlayer());
	if (_Entity == _LocalEntity /*&& !g_Convars.Compatibility.disable_all->GetBool()*/)
	{
		if (LocalPlayer.bInPrediction)
			return;
	}
	((void(__thiscall*)(CBaseEntity*, Vector &, surfacedata_t *, float, bool, bool))m_PlayerRecords[_Entity->index].Hooks.BaseEntity->GetOriginalHookedSub5())(_Entity, vecOrigin, psurface, fvol, force, unknown);
}
