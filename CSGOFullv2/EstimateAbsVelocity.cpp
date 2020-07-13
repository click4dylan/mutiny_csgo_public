#include "precompiled.h"
#include "BaseEntity.h"
#include "GameMemory.h"
#include "LocalPlayer.h"
#include "CPlayerrecord.h"

//This is called by C_BasePlayer::Simulate
void __fastcall HookedEstimateAbsVelocity(CBaseEntity* _Entity, DWORD EDX, Vector& vel)
{
	//If this is a player, NEVER estimate the velocity using interpolated data, use exact velocity
	if (_Entity->IsPlayer() && _Entity != LocalPlayer.Entity && !_Entity->GetImmune())
	{
		if (!_Entity->HasEFlag(EFL_DIRTY_ABSANGVELOCITY))
			vel = _Entity->GetAbsVelocityDirect();
		else
			_Entity->GetAbsVelocity(vel);

		return;
	}

	((void(__thiscall*)(CBaseEntity*, Vector&))g_LagCompensation.GetPlayerrecord(_Entity)->Hooks.BaseEntity->GetOriginalHookedSub6())(_Entity, vel);
}