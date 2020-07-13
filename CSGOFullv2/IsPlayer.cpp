#include "precompiled.h"
#include "BaseEntity.h"
#include "GameMemory.h"
#include "LocalPlayer.h"
#include <intrin.h> //VS2017 requires this for _ReturnAddress
#include "Aimbot_imi.h"

extern bool AllowShouldSkipAnimationFrame;

BOOLEAN __fastcall HookedEntityIsPlayer(CBaseEntity* _Entity)
{
	// world or not a player
	if (_Entity->index <= 0 || _Entity->index > 64)
		return false;

	// skip animation frame
	if (!AllowShouldSkipAnimationFrame && _ReturnAddress() == (void*)ShouldSkipAnimationFrameIsPlayerReturnAdr)
		return false;

	// is localplayer
	if (_Entity == LocalPlayer.Entity)
		return true;

	// call original
	auto &player = m_PlayerRecords[_Entity->index];
	if (player.Hooks.m_bHookedBaseEntity)
	{
		auto func = player.Hooks.BaseEntity->GetOriginalHookedSub1();
		if (func)
			return ((BOOLEAN(__thiscall*)(CBaseEntity*))func)(_Entity);
	}
	return true;
}