#pragma once
#include "CreateMove.h"
#include "BaseEntity.h"
#include "BaseCombatWeapon.h"

//#define USE_REBUILT_HANDLE_BULLET_PENETRATION

enum
{
	CHAR_TEX_ANTLION = 'A',
	CHAR_TEX_BLOODYFLESH = 'B',
	CHAR_TEX_CONCRETE = 'C',
	CHAR_TEX_DIRT = 'D',
	CHAR_TEX_EGGSHELL = 'E',
	CHAR_TEX_FLESH = 'F',
	CHAR_TEX_GRATE = 'G',
	CHAR_TEX_ALIENFLESH = 'H',
	CHAR_TEX_CLIP = 'I',
	CHAR_TEX_PLASTIC = 'L',
	CHAR_TEX_METAL = 'M',
	CHAR_TEX_SAND = 'N',
	CHAR_TEX_FOLIAGE = 'O',
	CHAR_TEX_COMPUTER = 'P',
	CHAR_TEX_SLOSH = 'S',
	CHAR_TEX_TILE = 'T',
	CHAR_TEX_CARDBOARD = 'U',
	CHAR_TEX_VENT = 'V',
	CHAR_TEX_WOOD = 'W',
	CHAR_TEX_GLASS = 'Y',
	CHAR_TEX_WARPSHIELD = 'Z',
};

float GetHitgroupDamageMultiplier(int iHitGroup);
typedef bool(__fastcall* TraceToExitFn)(Vector&, trace_t&, float, float, float, float, float, float, trace_t*);
extern TraceToExitFn TraceToExitGame;
extern bool trace_to_exit(const Vector& start, const Vector dir, Vector &out, trace_t* enter_trace, trace_t* exit_trace);
extern bool handle_bullet_penetration(float& penetration_amount, int &enter_material, bool& entry_is_grate, trace_t& enter_trace, Vector& direction,
	surfacedata_t* surfdata_unused,
	float entry_penetration_modifier, float entry_surface_damage_modifier,
	bool bDoEffects,
	int weaponmask, float penetration_amount_start, int& hitsleft, Vector& exitpos,
	float flUnused1, float flUnused2,
	float& bullet_damage,
	int team_number
);

void ScaleDamage(int hitgroup, CBaseEntity* Entity, float flArmorRatio, float& current_damage);

struct Autowall_Output_t
{
	::Autowall_Output_t()
	{
		entity_hit = nullptr;
		damage_dealt = 0.f;
		damage_per_wall.fill(0.f);
		hitbox_hit = -1;
		hitgroup_hit = -1;
		position_hit.Zero();
		penetrated_wall = false;
		walls_penetrated = 0;
	}
	CBaseEntity* entity_hit;
	float damage_dealt;
	std::array<float, 4> damage_per_wall;
	int hitbox_hit;
	int hitgroup_hit;
	Vector position_hit;
	bool penetrated_wall;
	int walls_penetrated;
};

CBaseEntity* Autowall(const Vector &pStartingPos, const Vector &pEndPos, Autowall_Output_t& output, bool CollideWithTeammates, bool DontTraceHitboxes = false, CBaseEntity* pTargetPlayer = nullptr, int TargetHitbox = -1);

extern bool(__thiscall *HandleBulletPenetrationCSGO)(CBaseEntity *pEntityHit, float *flPenetration, int *SurfaceMaterial, char *IsSolid, trace_t *ray, Vector *vecDir,
	int unused, float flPenetrationModifier, float flDamageModifier, int unused2, int weaponmask, float flPenetration2, int *hitsleft,
	Vector *ResultPos, int unused3, int unused4, float *damage);