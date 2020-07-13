#include "precompiled.h"
#include "BaseAnimating.h"
#include "GameMemory.h"

int CBaseAnimating::GetHitboxSet()
{
	return ReadInt((uintptr_t)this + g_NetworkedVariables.Offsets.m_nHitboxSet);
}