#include "precompiled.h"
#include "BaseEntity.h"
#include "GameMemory.h"
#include "LocalPlayer.h"
//#include "Convars.h"
#include "CPlayerrecord.h"

unsigned char __fastcall HookedPlayer_Simulate(CBaseEntity* _Entity)
{
	LocalPlayer.Get(&LocalPlayer);
	if (_Entity != LocalPlayer.Entity /*&& !g_Convars.Compatibility.disable_all->GetBool()*/)
		return true;

	CPlayerrecord *_playerRecord = &m_PlayerRecords[_Entity->index];
	using SimulateFn = unsigned char(__thiscall *)(CBaseEntity*);
	if (_playerRecord->Hooks.BaseEntity)
	{
		SimulateFn oSimulate = (SimulateFn)_playerRecord->Hooks.BaseEntity->GetOriginalHookedSub4();
		return oSimulate(_Entity);
	}
	return true;
}
