#include "precompiled.h"
#include "BaseEntity.h"
#include "CPlayerrecord.h"

void __fastcall HookedPhysicsSimulate(CBaseEntity* _Entity)
{
	//*(int*)((DWORD)_Entity + m_nSimulationTick) = 0; //Allow PhysicsSimulate to simulate more than 1 command per tick. Fixes fakelag and low fps
	auto &player = m_PlayerRecords[_Entity->index];
	if (player.Hooks.m_bHookedBaseEntity)
	{
		auto func = player.Hooks.BaseEntity->GetOriginalHookedSub7();
		if (func)
			((void(__thiscall*)(CBaseEntity*))func)(_Entity);
	}
}
