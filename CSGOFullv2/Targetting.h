#pragma once
#include <Windows.h>
#include "misc.h"
#include "HitboxDefines.h"
#include "GameVersion.h"

class CBaseEntity;
class CBaseCombatWeapon;
struct SavedHitboxPos;

class Targetting
{
public:
	bool IsPlayerAValidTarget(CBaseEntity* Player);
	int HitboxToHitgroup(int Hitbox);
	int HitgroupToHitbox(int Hitgroup);
	bool HitboxIsArmOrHand(int Hitbox);
	bool HitboxIsLegOrFoot(int Hitbox);
};

extern Targetting MTargetting;
