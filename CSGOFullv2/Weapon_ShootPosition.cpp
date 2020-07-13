#include "precompiled.h"
#include "VTHook.h"
#include "CPlayerrecord.h"

extern void __fastcall ModifyEyePositionServer(CCSGOPlayerAnimState* thisptr, DWORD EDX, Vector& vecEyePos, bool FixEyePosition);

Vector& __fastcall HookedWeaponShootPosition(CBaseEntity* me, DWORD edx, Vector& dest)
{
	//((void(__thiscall*)(CBaseEntity*, Vector&))AllPlayers[me->index].PersistentData.HookedBaseEntity->GetOriginalHookedSub8())(me, dest);

	me->Weapon_ShootPosition_Base(dest);

	if (me->GetShouldUseAnimationEyeOffset())
	{
		CPlayerrecord *record = g_LagCompensation.GetPlayerrecord(me);
		if (record && record->m_pAnimStateServer[ResolveSides::NONE] && record->m_pAnimStateServer[ResolveSides::NONE]->pBaseEntity == me)
			ModifyEyePositionServer(record->m_pAnimStateServer[ResolveSides::NONE], edx,  dest, true);
	}

	return dest;
}