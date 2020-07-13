#pragma once

struct HC_Vars 
{
	int maxseeds;
	int seedshit;
	int seedsneeded;
	int maxallowedmiss;
	HANDLE DoWork_Event;
	HANDLE Finished_Event;
};

bool intialHeadScan(CBaseCombatWeapon* Weapon, Vector vMin, Vector vMax, int hitChance);
bool BulletWillHit(CBaseCombatWeapon* Weapon, CBasePlayer* pTargetEntity, QAngle viewangles, Vector *pStartPos, unsigned int TargetHitgroupFlags);