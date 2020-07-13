#include "precompiled.h"
#include "GameMemory.h"
#include "HitboxDefines.h"
#include "Targetting.h"

#include "BaseEntity.h"
#include "LocalPlayer.h"
#include "AutoWall.h"
#include "VMProtectDefs.h"
//#include "Convars.h"

Targetting MTargetting;

bool Targetting::IsPlayerAValidTarget(CBaseEntity* Player)
{
	return Player && Player->IsPlayer() && !Player->GetImmune() && Player->IsEnemy(LocalPlayer.Entity);
}

int Targetting::HitboxToHitgroup(int Hitbox)
{
	switch (Hitbox)
	{
	case HITBOX_HEAD:
		return HITGROUP_HEAD;

	case HITBOX_LOWER_NECK:
		return HITGROUP_NECK;

	case HITBOX_PELVIS:
	case HITBOX_BODY:
		return HITGROUP_STOMACH;

	case HITBOX_THORAX:
	case HITBOX_CHEST:
	case HITBOX_UPPER_CHEST:
		return HITGROUP_CHEST;

	case HITBOX_LEFT_THIGH:
	case HITBOX_LEFT_CALF:
	case HITBOX_LEFT_FOOT:
		return HITGROUP_LEFTLEG;

	case HITBOX_RIGHT_THIGH:
	case HITBOX_RIGHT_CALF:
	case HITBOX_RIGHT_FOOT:
		return HITGROUP_RIGHTLEG;

	case HITBOX_LEFT_HAND:
	case HITBOX_LEFT_UPPER_ARM:
	case HITBOX_LEFT_FOREARM:
		return HITGROUP_LEFTARM;

	case HITBOX_RIGHT_HAND:
	case HITBOX_RIGHT_UPPER_ARM:
	case HITBOX_RIGHT_FOREARM:
		return HITGROUP_RIGHTARM;

	default:
		return HITGROUP_GENERIC;
	}
}

int Targetting::HitgroupToHitbox(int Hitgroup)
{
	switch (Hitgroup)
	{
	case HITGROUP_HEAD:
		return HITBOX_HEAD;
	case HITGROUP_CHEST:
		return HITBOX_CHEST;
	case HITGROUP_STOMACH:
		return HITBOX_PELVIS;
	case HITGROUP_LEFTARM:
		return HITBOX_LEFT_FOREARM;
	case HITGROUP_RIGHTARM:
		return HITBOX_RIGHT_FOREARM;
	case HITGROUP_LEFTLEG:
		return HITBOX_LEFT_CALF;
	case HITGROUP_RIGHTLEG:
		return HITBOX_RIGHT_CALF;
	case HITGROUP_NECK:
		return HITBOX_LOWER_NECK;
	default:
		return HITBOX_BODY;
	}
}

bool Targetting::HitboxIsArmOrHand(int Hitbox)
{
	switch (Hitbox)
	{
	case HITBOX_LEFT_FOREARM:
	case HITBOX_LEFT_HAND:
	case HITBOX_RIGHT_FOREARM:
	case HITBOX_RIGHT_HAND:
		return true;
	}
	return false;
}

bool Targetting::HitboxIsLegOrFoot(int Hitbox)
{
	switch (Hitbox)
	{
	case HITBOX_LEFT_CALF:
	case HITBOX_LEFT_FOOT:
	case HITBOX_RIGHT_CALF:
	case HITBOX_RIGHT_FOOT:
		return true;
	}
	return false;
}