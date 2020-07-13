#include "precompiled.h"
#include "misc.h"
#include "AutoWall.h"
#include "Overlay.h"
#include "TraceRay.h"
#include "HitboxDefines.h"
#include "Targetting.h"
#include "LocalPlayer.h"
#include "ConVar.h"
#include "Intersection.h"
#include "UsedConvars.h"

#if 0
void UTIL_ClipTraceToPlayer(const Vector& vecStartPos, const Vector& vecEndPos, unsigned int mask, trace_t* tr, CBaseEntity* Entity) {
	Vector WorldSpaceCenter = Entity->GetAbsOriginDirect() + ( (Entity->GetMins() + Entity->GetMaxs()) * 0.5f );
	Vector to = WorldSpaceCenter - vecStartPos;
	Vector vecDir = vecStartPos - vecEndPos;
	float length = vecDir.NormalizeInPlace();
	float rangeAlong = vecDir.Dot(to);
	float range;

	if (rangeAlong < 0.f)
	{
		// off start point.
		range = -(to).Length();
	}
	else if (rangeAlong > length)
	{
		// off end point
		range = -(WorldSpaceCenter - vecEndPos).Length();
	}
	else 
	{
		// within ray bounds
		Vector pointOnRay = vecStartPos + (vecDir * rangeAlong);
		range = (WorldSpaceCenter - pointOnRay).Length();
	}

	if ( range >= 0.0f && range <= 60.f) 
	{
		// we shortened the ray - save off the trace
		trace_t playerTrace;
		Ray_t ray;
		ray.Init(vecStartPos, vecEndPos);
		Interfaces::EngineTrace->ClipRayToEntity(ray, mask | CONTENTS_HITBOX, (IHandleEntity*)Entity, &playerTrace);

		if (tr->fraction > playerTrace.fraction)
			*tr = playerTrace;
	}
}
#endif
void UTIL_ClipTraceToPlayer(CPlayerrecord* playerrecord, Ray_t &ray, trace_t* tr)
{
	CTickrecord *_targetTick = playerrecord->m_TargetRecord;
	CTickrecord _tmpTick;
	if (!_targetTick)
	{
		playerrecord->m_pEntity->ComputeHitboxSurroundingBox(&_tmpTick.m_HitboxWorldMins, &_tmpTick.m_HitboxWorldMaxs, playerrecord->m_pEntity->GetBoneAccessor()->GetBoneArrayForWrite());
		_targetTick = &_tmpTick;
	}
	CBaseEntity *_ent = playerrecord->m_pEntity;
	bool _IsUsingNetworkedESP = playerrecord->m_bIsUsingFarESP || playerrecord->m_bIsUsingServerSide;
	if ((!playerrecord->m_bDormant || _IsUsingNetworkedESP))
	{
		trace_t new_tr;
		// can't trust bones on far esp targets
		//if (!_IsUsingNetworkedESP)
		//{
			//Do a trace to the surrounding bounding box to see if we hit it, for optimization
			if (!IntersectRayWithBox(ray.m_Start, ray.m_Delta, _targetTick->m_HitboxWorldMins, _targetTick->m_HitboxWorldMaxs, 0.0f, &new_tr))
				return;

			Interfaces::EngineTrace->ClipRayToEntity(ray, MASK_SHOT, (IHandleEntity*)_ent, &new_tr);
		//}
		//else
		//{
		//	TRACE_HITBOX(_ent, (Ray_t&)ray, new_tr, _targetTick->m_cSpheres, _targetTick->m_cOBBs);
		//}
		if (new_tr.m_pEnt == _ent)
			*tr = new_tr;
	}
}

void UTIL_ClipTraceToPlayer(CPlayerrecord* playerrecord, Vector& start, Vector& end, trace_t* tr)
{
	Ray_t ray;
	ray.Init(start, end);
	UTIL_ClipTraceToPlayer(playerrecord, ray, tr);
}

float inline GetHitgroupDamageMultiplier(int iHitGroup)
{
	switch (iHitGroup)
	{
	case HITGROUP_HEAD:
	{
		return 4.0f;
	}
	case HITGROUP_STOMACH:
	{
		return 1.25f;
	}
	case HITGROUP_LEFTLEG:
	case HITGROUP_RIGHTLEG:
	{
		return 0.75f;
	}
	}
	return 1.0f;
}

bool EntityHasArmor(CBaseEntity* Entity, int hitgroup)
{
	if (Entity->GetArmor() > 0)
	{
		if ((hitgroup == HITGROUP_HEAD && Entity->HasHelmet()) || (hitgroup >= HITGROUP_CHEST && hitgroup <= HITGROUP_RIGHTARM))
			return true;
	}
	return false;
}

void ScaleDamage(int hitgroup, CBaseEntity *Entity, float flArmorRatio, float &current_damage)
{
	bool bHasHeavyArmor = false;// player->HasHeavyArmor();

	switch (hitgroup) 
	{
		case HITGROUP_HEAD:
			if (!bHasHeavyArmor)
				current_damage *= 4.0f;
			else
				current_damage = (current_damage * 4.0f) * 0.5f;
			break;
		case HITGROUP_STOMACH:
			current_damage *= 1.25f;
			break;
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
			current_damage *= 0.75f;
			break;
	}

	if (Entity && EntityHasArmor(Entity, hitgroup))
	{
		float flHeavyRatio = 1.0f;
		float flBonusRatio = 0.5f;
		float flRatio = flArmorRatio * 0.5f;
		float flNewDamage;

		if (!bHasHeavyArmor) 
		{
			flNewDamage = current_damage * flRatio;
		}
		else
		{
			flBonusRatio = 0.33f;
			flRatio = flArmorRatio * 0.25f;
			flHeavyRatio = 0.33f;
			flNewDamage = (current_damage * flRatio) * 0.85f;
		}

		int iArmor = Entity->GetArmor();

		if (((current_damage - flNewDamage) * (flHeavyRatio * flBonusRatio)) > iArmor)
			flNewDamage = current_damage - (iArmor / flBonusRatio);

		current_damage = flNewDamage;
	}

	current_damage = floorf(current_damage);
}

bool(__thiscall *HandleBulletPenetrationCSGO)(CBaseEntity *pEntityHit, float *flPenetration, int *SurfaceMaterial, char *IsSolid, trace_t *ray, Vector *vecDir,
	int unused, float flPenetrationModifier, float flDamageModifier, int unused2, int weaponmask, float flPenetration2, int *hitsleft,
	Vector *ResultPos, int unused3, int unused4, float *damage);

//void __usercall DebugPenetration(signed __int16 a1@<dx>, __int16 enter_surfaceprops@<cx>, double a3@<st0>, float length@<xmm2>, float bulletdamage@<xmm3>, Vector startpos, Vector endpos, float taken_first, float damage_taken)
#if 0
void DebugPenetration(signed __int16 a1 , __int16 enter_surfaceprops, float length, float bulletdamage, Vector startpos, Vector endpos, float taken_first, float damage_taken)
{
	Vector  dir = endpos - startpos;
	float endposstartposlength = dir.Length();

	if (damage_taken >= bulletdamage)
	{
		a1 = -100;
		float v22 = fmaxf((bulletdamage - taken_first) / (damage_taken - taken_first), 0.0f);
		VectorNormalizeFast(dir);
		dir = (dir * endposstartposlength) * v22;
		Vector v54 = startpos;
		Vector v59 = endpos;

		if (taken_first >= bulletdamage)
		{
			v54 = dir;
			v59 = dir;
			length = 0.0f;
		}
	}

}
#endif

bool handle_bullet_penetration(float& penetration_amount, int &enter_material, bool& entry_is_grate, trace_t& enter_trace, Vector& direction,
	surfacedata_t* surfdata_unused,
	float entry_penetration_modifier, float entry_surface_damage_modifier, 
	bool bDoEffects,
	int weaponmask, float penetration_amount_start, int& hitsleft, Vector& exitpos, 
	float flUnused1, float flUnused2,
	float& bullet_damage,
	int team_number
)
{
	int is_nodraw = (enter_trace.surface.flags) & SURF_NODRAW;
	bool cannotpenetrate = false;

	Vector end;
	trace_t exit_trace;

	if (!hitsleft)
	{
		if (!is_nodraw && !entry_is_grate)
		{
			if (enter_material != CHAR_TEX_GLASS)
				cannotpenetrate = enter_material != CHAR_TEX_GRATE;
		}
	}

#if defined AUTOWALL_DEBUG && defined _DEBUG
	Vector vecUp = { 0.0f, 0.0f, 5.5f };
#endif

	if (penetration_amount <= 0.0f || hitsleft <= 0)
		cannotpenetrate = true;

	if (!trace_to_exit(enter_trace.endpos, direction, end, &enter_trace, &exit_trace)
		&& !(Interfaces::EngineTrace->GetPointContents(enter_trace.endpos, MASK_SHOT_HULL, 0) & MASK_SHOT_HULL)
		|| cannotpenetrate)
	{
#if defined AUTOWALL_DEBUG && defined _DEBUG
		//NOTE: UNCOMMENTING THESE 4 FLOATS WILL CAUSE A RACE CONDITION DEADLOCK IN RELEASE MODE WITH PARALLEL PROCESSING
		float penetration_length = (end - enter_trace.endpos).Length();
		float modifier = fmaxf(1.0f / entry_penetration_modifier, 0.0f);
		float taken_first = (fmaxf(((3.0f / penetration_amount_start) * 1.18f), 0.0f) * (modifier * 2.8f) + (bullet_damage * 0.15f));
		float taken_damage = (((penetration_length * penetration_length) * modifier) / 24.0f) + taken_first;

		QAngle angs = CalcAngle(enter_trace.endpos, end);
		surfacedata_t *enterdata = Interfaces::Physprops->GetSurfaceData(enter_trace.surface.surfaceProps);
		surfacedata_t *exitdata = Interfaces::Physprops->GetSurfaceData(exit_trace.surface.surfaceProps);
		const char *entername = Interfaces::Physprops->GetPropName(enter_trace.surface.surfaceProps);
		const char *exitname = Interfaces::Physprops->GetPropName(exit_trace.surface.surfaceProps);
		if (!entername)
			entername = "";
		Interfaces::DebugOverlay->AddTextOverlay(enter_trace.endpos + vecUp, 6.0f, entername);
		Interfaces::DebugOverlay->AddBoxOverlay(enter_trace.endpos + vecUp, Vector(-1, -1, -1), Vector(1, 1, 1), angs, 255, 255, 185, 5, 6.0f);
		Interfaces::DebugOverlay->AddLineOverlay(enter_trace.endpos + vecUp, end  + vecUp, 125, 125, 0, 40, 6.0f);
		Interfaces::DebugOverlay->AddBoxOverlay(end + vecUp, Vector(-0.5f, -0.5f, -0.5f), Vector(.5f, .5f, .5f), angs, 0, 5, 200, 50, 6.0f);
#endif
		return true;
	}
	
	auto exit_surface_data = Interfaces::Physprops->GetSurfaceData(exit_trace.surface.surfaceProps);
	bool use_new_penetration_system = sv_penetration_type.GetVar() ? sv_penetration_type.GetVar()->GetBool() : true;

	float damage_modifier;
	float average_penetration_modifier;
	unsigned short exit_material = exit_surface_data->game.gamematerial;

	if (!use_new_penetration_system)
	{
		if (entry_is_grate || is_nodraw)
		{
			average_penetration_modifier = 1.0f;
			damage_modifier = 0.99f;
		}
		else
		{
			average_penetration_modifier = fminf(exit_surface_data->game.flPenetrationModifier, entry_penetration_modifier);
			damage_modifier = fminf(entry_surface_damage_modifier, exit_surface_data->game.flDamageModifier);
		}

		if (enter_material == exit_material && (exit_material == CHAR_TEX_METAL || exit_material == CHAR_TEX_WOOD))
			average_penetration_modifier += average_penetration_modifier;

		float thickness = (exit_trace.endpos - enter_trace.endpos).Length();

		if (sqrt(thickness) <= average_penetration_modifier * penetration_amount)
		{
			bullet_damage *= damage_modifier;
			//if (bDoEffects) //note: not on windows version, is it inlined into the function?
			//ImpactTrace(exit_trace, weaponmask);
			exitpos = exit_trace.endpos;
			--hitsleft;

			return false;
		}

		return true;
	}
	else
	{
		damage_modifier = 0.16f;
		if (!entry_is_grate && !is_nodraw)
		{
			if (enter_material == CHAR_TEX_GLASS)
				goto LABEL_51;

			if (enter_material != CHAR_TEX_GRATE)
			{
				if (enter_trace.m_pEnt)
				{
					if (enter_trace.m_pEnt->IsPlayer())
					{
						if (enter_trace.m_pEnt->IsPlayerGhost())
							goto LABEL_45;
					}
				}

				const float reduction_bullets = ff_damage_reduction_bullets.GetVar() != nullptr ? ff_damage_reduction_bullets.GetVar()->GetFloat() : 0.1f;
				const float damage_bullet_pen = ff_damage_bullet_penetration.GetVar() != nullptr ? ff_damage_bullet_penetration.GetVar()->GetFloat() : 0.f;

				if (enter_material != CHAR_TEX_FLESH
					|| reduction_bullets != 0.0f
					|| !enter_trace.m_pEnt
					|| !enter_trace.m_pEnt->IsPlayer()
					|| enter_trace.m_pEnt->IsEnemy(LocalPlayer.Entity))
				{
					average_penetration_modifier = (exit_surface_data->game.flPenetrationModifier + entry_penetration_modifier) * 0.5f;
					goto LABEL_46;
				}
				if (damage_bullet_pen == 0.0f)
					return true;

				entry_penetration_modifier = damage_bullet_pen;

			LABEL_45:
				average_penetration_modifier = entry_penetration_modifier;
			LABEL_46:
				damage_modifier = 0.16f;
				goto LABEL_52;
			}
		}

		if (enter_material != CHAR_TEX_GRATE && enter_material != CHAR_TEX_GLASS)
		{
			average_penetration_modifier = 1.0f;
			goto LABEL_52;
		}

	LABEL_51:
		damage_modifier = 0.05f;
		average_penetration_modifier = 3.0f;
	}
LABEL_52:

	if (enter_material == exit_material)
	{
		if (exit_material == CHAR_TEX_WOOD || exit_material == CHAR_TEX_CARDBOARD)
			average_penetration_modifier = 3.0f;
		else if (exit_material == CHAR_TEX_PLASTIC)
			average_penetration_modifier = 2.0f;
	}

	float penetration_length = (exit_trace.endpos - enter_trace.endpos).Length();
	float modifier = fmaxf(1.0f / average_penetration_modifier, 0.0f);
	float taken_first = (fmaxf(((3.0f / penetration_amount_start) * 1.25f), 0.0f) * (modifier * 3.0f) + (bullet_damage * damage_modifier));
	float taken_damage = (((penetration_length * penetration_length) * modifier) / 24.0f) + taken_first;

#if defined AUTOWALL_DEBUG && defined _DEBUG
	QAngle angs = CalcAngle(enter_trace.endpos, exit_trace.endpos);
	surfacedata_t *enterdata = Interfaces::Physprops->GetSurfaceData(enter_trace.surface.surfaceProps);
	surfacedata_t *exitdata = Interfaces::Physprops->GetSurfaceData(exit_trace.surface.surfaceProps);
	const char *entername = Interfaces::Physprops->GetPropName(enter_trace.surface.surfaceProps);
	const char *exitname = Interfaces::Physprops->GetPropName(exit_trace.surface.surfaceProps);
	if (!entername)
		entername = "";
	if (!exitname)
		exitname = "";

	Interfaces::DebugOverlay->AddTextOverlay(enter_trace.endpos + vecUp, 6.0f, entername);
	Interfaces::DebugOverlay->AddTextOverlay(exit_trace.endpos + vecUp, 6.0f, exitname);
	Interfaces::DebugOverlay->AddBoxOverlay(enter_trace.endpos + vecUp, Vector(-1, -1, -1), Vector(1, 1, 1), angs, 255, 255, 185, 5, 6.0f);
	Interfaces::DebugOverlay->AddLineOverlay(enter_trace.endpos + vecUp, exit_trace.endpos + vecUp, 125, 125, 0, 40, 6.0f);
	Interfaces::DebugOverlay->AddBoxOverlay(exit_trace.endpos + vecUp, Vector(-.5f, -.5f, -.5f), Vector(.5f, .5f, .5f), angs, 0, 5, 200, 50, 6.0f);
#endif

	bullet_damage -= fmaxf(0.0f, taken_damage);

	if (bullet_damage < 1.0f )
		return true;

	//if (bDoEffects) //note: not on windows version, is it inlined into the function?
	//ImpactTrace(exit_trace, weaponmask);

	exitpos = exit_trace.endpos;

	--hitsleft;

	return false;
}

//AUTOWALL DIRECTIONS:
//DontTraceHitboxes assumes that the ray already collides with the target player and we only want to make sure that we can penetrate to target position
CBaseEntity* Autowall(const Vector &pStartingPos, const Vector &pEndPos, Autowall_Output_t& output, bool CollideWithTeammates, bool DontTraceHitboxes, CBaseEntity* pTargetPlayer, int TargetHitbox)
{
	float bulletdmg = 0.0f;
	int hitsleft = 4;
	CBaseCombatWeapon *weapon = LocalPlayer.CurrentWeapon;
	if (weapon)
	{
		WeaponInfo_t *wpn_data = weapon->GetCSWpnData();
		if (wpn_data)
		{
			CBaseEntity *lastEntityHit = nullptr;
			CPlayerrecord *_playerRecord = pTargetPlayer ? pTargetPlayer->ToPlayerRecord() : nullptr;
			trace_t enter_tr;
			Vector vecSrc = pStartingPos;
			Vector vecDir = pEndPos - pStartingPos;
			VectorNormalizeFast(vecDir);
			float flRangeAssumedAHit = vecDir.Length();
			float penetration_amount_start = wpn_data->flPenetration;
			float penetration_amount = penetration_amount_start;
			float flMaxRange = wpn_data->flRange;
			float flCurrentDistance = 0.0f;  //distance that the bullet has traveled so far
			bulletdmg = (float)wpn_data->iDamage;

			while (hitsleft > 0 && bulletdmg > 0.0f)
			{
				flMaxRange -= flCurrentDistance;
				//printf("hitsleft %i thread %i maxrange %f\n", hitsleft, GetCurrentThreadId(), flMaxRange);
				Vector vecEnd = vecSrc + vecDir * flMaxRange;
				CTraceFilterSkipTwoEntities inherited((IHandleEntity*)LocalPlayer.Entity, (IHandleEntity*)lastEntityHit, COLLISION_GROUP_NONE);
				CTraceFilterInterited_DisablePlayers filter(inherited);
				Ray_t ray(vecSrc, vecEnd);
				Interfaces::EngineTrace->TraceRay(ray, MASK_SHOT, &inherited, &enter_tr); //Run initial trace into a wall, ignoring all players
				Ray_t ray_to_enter_tr_endpos(vecSrc, enter_tr.endpos);
				float backup_fraction = enter_tr.fraction;

				//UTIL_ClipTraceToPlayers(vecSrc, enter_tr.endpos, MASK_SHOT, &filter, &enter_tr, 0.0f, (enter_tr.endpos - vecSrc).Length() / (vecSrc - enter_tr.endpos).Length());
				
#if 1
				//Now, attempt to find a player between the start and the wall
				if (pTargetPlayer)
				{
					if (!DontTraceHitboxes)
						UTIL_ClipTraceToPlayer(_playerRecord, ray_to_enter_tr_endpos, &enter_tr); //note: doing it this way will ignore other enemies that may be in front of us! this will disrupt the damage result but result in a big speed improvement
						
					if (CollideWithTeammates)
						UTIL_ClipTraceToPlayers_Fixed(ray_to_enter_tr_endpos, &enter_tr, LocalPlayer.Entity, true); //note: doing it this way will ALWAYS return a teammate even if enemy is in front of the teammate
				}
				else
				{
					UTIL_ClipTraceToPlayers_Fixed(ray_to_enter_tr_endpos, &enter_tr, LocalPlayer.Entity, false, !CollideWithTeammates);
				}

				enter_tr.fraction = backup_fraction;
				if (enter_tr.m_pEnt && enter_tr.m_pEnt->IsPlayer())
				{
					//correct the fraction to the original end position since we traced enter_tr.endpos
					float flMaxLength = (vecEnd - vecSrc).Length();
					float flLengthToHitbox = (enter_tr.endpos - vecSrc).Length();
					enter_tr.fraction = flLengthToHitbox / flMaxLength;
				}
#endif

				if (enter_tr.fraction >= 1.0f)
				{
					output.position_hit = enter_tr.endpos;
					break;
				}

				surfacedata_t *entersurf = Interfaces::Physprops->GetSurfaceData(enter_tr.surface.surfaceProps);
				surfacegameprops_t *props = &entersurf->game;
				bool is_grate = (enter_tr.contents & CONTENTS_GRATE);
				int entry_material = (int)entersurf->game.gamematerial;
				float flPenetrationModifier = props->flPenetrationModifier;

				if (CollideWithTeammates && enter_tr.m_pEnt && enter_tr.m_pEnt->IsPlayer() && !enter_tr.m_pEnt->IsEnemy(LocalPlayer.Entity))
				{
					//we collided with a teammate

					flCurrentDistance += flMaxRange * enter_tr.fraction;
					bulletdmg *= powf(wpn_data->flRangeModifier, (flCurrentDistance * 0.002f)); // dist / 500
					if (weapon->GetItemDefinitionIndex() == WEAPON_TASER && LocalPlayer.Entity->IsPlayerGhost())
						bulletdmg = 1.0f;
					ScaleDamage(weapon->GetItemDefinitionIndex() != WEAPON_TASER ? enter_tr.hitgroup : HITGROUP_GENERIC, enter_tr.m_pEnt, wpn_data->flArmorRatio, bulletdmg);

					output.entity_hit = enter_tr.m_pEnt;
					output.damage_dealt = std::ceilf(bulletdmg);
					output.hitbox_hit = enter_tr.hitbox;
					output.hitgroup_hit = enter_tr.hitgroup;
					output.position_hit = enter_tr.endpos;
					output.penetrated_wall = hitsleft != 4;
					output.walls_penetrated = 4 - hitsleft;
				
					return enter_tr.m_pEnt;
				}

				if (DontTraceHitboxes && pTargetPlayer && flCurrentDistance + flMaxRange * enter_tr.fraction >= flRangeAssumedAHit)
				{
					int TargetHitgroup = MTargetting.HitboxToHitgroup(TargetHitbox);
					float flMaxLength = (vecEnd - vecSrc).Length();
					float flLengthToHitbox = (pEndPos - vecSrc).Length();
					float fraction = flLengthToHitbox / flMaxLength;

					flCurrentDistance += flMaxRange * fraction;
					bulletdmg *= powf(wpn_data->flRangeModifier, (flCurrentDistance * 0.002f)); // dist / 500
					if (weapon->GetItemDefinitionIndex() == WEAPON_TASER && LocalPlayer.Entity->IsPlayerGhost())
						bulletdmg = 1.0f;
					ScaleDamage(weapon->GetItemDefinitionIndex() != WEAPON_TASER ? TargetHitgroup : HITGROUP_GENERIC, pTargetPlayer, wpn_data->flArmorRatio, bulletdmg);

					output.entity_hit = pTargetPlayer;
					output.damage_dealt = std::ceilf(bulletdmg);
					output.hitbox_hit = TargetHitbox;
					output.hitgroup_hit = TargetHitgroup;
					output.position_hit = enter_tr.endpos;
					output.penetrated_wall = hitsleft != 4;
					output.walls_penetrated = 4 - hitsleft;
				
					return pTargetPlayer;
				}

				flCurrentDistance += flMaxRange * enter_tr.fraction;
				bulletdmg *= powf(wpn_data->flRangeModifier, (flCurrentDistance * 0.002f)); // dist / 500

				if (weapon->GetItemDefinitionIndex() == WEAPON_TASER && LocalPlayer.Entity->IsPlayerGhost())
					bulletdmg = 1.0f;

				lastEntityHit = enter_tr.m_pEnt ? enter_tr.m_pEnt->IsPlayer() ? enter_tr.m_pEnt : nullptr : nullptr;
				//enter_tr.hitgroup = 0;

				if (enter_tr.hitgroup != 0)
				{
					if (enter_tr.m_pEnt && lastEntityHit->IsPlayer())
					{
						ScaleDamage(enter_tr.hitgroup, lastEntityHit, wpn_data->flArmorRatio, bulletdmg);

						output.entity_hit = enter_tr.m_pEnt;
						output.damage_dealt = std::ceilf(bulletdmg);
						output.hitbox_hit = enter_tr.hitbox;
						output.hitgroup_hit = enter_tr.hitgroup;
						output.position_hit = enter_tr.endpos;
						output.penetrated_wall = hitsleft != 4;
						output.walls_penetrated = 4 - hitsleft;
						
						return enter_tr.m_pEnt;
					}
				}

				output.position_hit = enter_tr.endpos;
				int weaponmask = 0x1002;
				if (weapon->GetItemDefinitionIndex() == WEAPON_TASER)
				{
					weaponmask = 0x1100;
					if (LocalPlayer.Entity->IsPlayerGhost())
						bulletdmg = 1.0f;
				}

				if (hitsleft > 0)
				{
					output.damage_per_wall[4 - hitsleft] = std::ceilf(bulletdmg);
				}

				if (flCurrentDistance > 3000.0f && penetration_amount > 0.0f || flPenetrationModifier < 0.1f)
				{
					// test
					//output.entity_hit = pTargetPlayer;
					//output.damage_dealt = std::ceilf(bulletdmg);
					//output.hitbox_hit = TargetHitbox;
					//output.hitgroup_hit = MTargetting.HitboxToHitgroup(TargetHitbox);
					//output.position_hit = enter_tr.endpos;
					//output.penetrated_wall = hitsleft != 4;
					//output.walls_penetrated = 4 - hitsleft;
					//END_PROFILING
					//return pTargetPlayer;
					break;
				}

				if (handle_bullet_penetration(penetration_amount, entry_material, is_grate, enter_tr, vecDir, nullptr, entersurf->game.flPenetrationModifier, entersurf->game.flDamageModifier, false, weaponmask, penetration_amount_start, hitsleft, vecSrc, 0.0f, 0.0f, bulletdmg, LocalPlayer.Entity->GetTeam()))
					break;
			}
		}
	}
	output.entity_hit = nullptr;
	output.damage_dealt = std::ceilf(bulletdmg);
	output.penetrated_wall = hitsleft != 4;
	output.walls_penetrated = 4 - hitsleft;
	
	return nullptr;
}

TraceToExitFn TraceToExitGame;

//IsEntityBreakable reversed by dylan

bool inline FClassnameIs(CBaseEntity *pEntity, const char* szClassname)
{
	return !strcmp(pEntity->GetClassname(), szClassname) ? true : false;
}

bool IsEntityBreakable(CBaseEntity* pEntity)
{
#if 0
	typedef bool(__thiscall* IsBreakbaleEntity_t)(CBaseEntity*);
	IsBreakbaleEntity_t IsBreakbaleEntityFn = (IsBreakbaleEntity_t)FindMemoryPattern(ClientHandle, "55  8B  EC  51  56  8B  F1  85  F6  74  68");
	if (!IsBreakbaleEntityFn)
		return false;

	return IsBreakbaleEntityFn(pEntity);
#else

	// skip null ents and the world ent.
	if (!pEntity || !pEntity->index)
		return false;

	int iHealth = pEntity->GetHealth();

	if ((iHealth >= 0 || pEntity->GetMaxHealth() <= 0))//(v2 = pEntity->GetUnknownInt(), result = 1, v2 <= 0)))
	{
		if (pEntity->GetTakeDamage() != DAMAGE_YES)
		{
#ifndef NO_CUSTOM_FIXES
			char *NetworkName = pEntity->GetClientClass()->m_pNetworkName;
			if (!(NetworkName[1] != 'F' || NetworkName[4] != 'c' || NetworkName[5] != 'B' || NetworkName[9] != 'h'))
				//if (*(unsigned*)NetworkName != 0x65724243)
#endif
				return false;
		}

		int CollisionGroup = pEntity->GetCollisionGroup();

		//Interfaces::Physprops->GetSurfaceIndex(pEntity->GetModelPtr()->_m_pStudioHdr->GetSurfaceProp())

		if (CollisionGroup != COLLISION_GROUP_PUSHAWAY && CollisionGroup != COLLISION_GROUP_BREAKABLE_GLASS && CollisionGroup != COLLISION_GROUP_NONE)
			return false;

		if (iHealth > 200)
			return false;

		DWORD pPhysicsInterface = ((DWORD(__cdecl *)(CBaseEntity*, DWORD, DWORD, DWORD, DWORD))AdrOf_IsEntityBreakableCall)(pEntity, 0, IsEntityBreakable_FirstCall_Arg1, IsEntityBreakable_FirstCall_Arg2, 0);
		if (pPhysicsInterface)
		{
			if (((bool(__thiscall*)(DWORD)) *(DWORD*)(*(DWORD*)pPhysicsInterface))(pPhysicsInterface) != PHYSICS_MULTIPLAYER_SOLID)
				return false;
		}
		else
		{
#ifdef NO_CUSTOM_FIXES
			if (FClassnameIs(pEntity, "func_breakable") || FClassnameIs(pEntity, "func_breakable_surf"))
			{
				if (FClassnameIs(pEntity, "func_breakable_surf"))
				{
					// don't try to break it if it has already been broken
					if (pEntity->IsBroken())
						return false;
				}
			}
#else
			const char *szClassname = pEntity->GetClassname();
			if (szClassname[0] == 'f' && szClassname[5] == 'b' && szClassname[13] == 'e')
			{
				if (szClassname[15] == 's')
				{
					// don't try to break it if it has already been broken
					if (pEntity->IsBroken())
						return false;
				}
			}
#endif
			else if (pEntity->PhysicsSolidMaskForEntity() & CONTENTS_PLAYERCLIP)
			{
				// hostages and players use CONTENTS_PLAYERCLIP, so we can use it to ignore them
				return false;
			}
		}

		DWORD pBreakableInterface = ((DWORD(__cdecl *)(CBaseEntity*, DWORD, DWORD, DWORD, DWORD))AdrOf_IsEntityBreakableCall)(pEntity, 0, IsEntityBreakable_SecondCall_Arg1, IsEntityBreakable_SecondCall_Arg2, 0);
		if (pBreakableInterface)
		{
			// Bullets don't damage it - ignore
			float DamageModBullet = ((float(__thiscall*)(DWORD)) *(DWORD*)(*(DWORD*)pPhysicsInterface + 0xC))(pBreakableInterface);
			if (DamageModBullet <= 0.0f)
				return false;
		}
	}

	return true;
#endif
}

//credits: nitro and sharklaser
bool trace_to_exit(const Vector& start, const Vector dir, Vector &out, trace_t* enter_trace, trace_t* exit_trace)
{
#if 0
	typedef bool(__fastcall* TraceToExitFn)(Vector&, trace_t*, float, float, float, float, float, float, trace_t*);
	static TraceToExitFn TraceToExit = (TraceToExitFn)FindMemoryPattern(ClientHandle, "55  8B  EC  83  EC  30  F3  0F  10  75");

	if (!TraceToExit)
		return false;

	return TraceToExit(out, enter_trace, start.x, start.y, start.z, dir.x, dir.y, dir.z, exit_trace);
#else
	Vector          new_end;
	float           dist = 0.0f;
	int				first_contents = 0;
	int             contents;

	while (dist <= 90.0f)
	{
		dist += 4.0f;
		out = start + (dir * dist);
		new_end = out - (dir * 4.0f);

		contents = Interfaces::EngineTrace->GetPointContents(out, MASK_SHOT, nullptr);

		if (!first_contents)
			first_contents = contents;

		if ((contents & MASK_SHOT_HULL) && (!(contents & CONTENTS_HITBOX) || (first_contents == contents)))
			continue;

		UTIL_TraceLine(out, new_end, MASK_SHOT, (CTraceFilter*)nullptr, exit_trace);

		//if (sv_clip_penetration_traces_to_players.GetVar() != nullptr && sv_clip_penetration_traces_to_players.GetVar()->GetInt())
			UTIL_ClipTraceToPlayers(out, new_end, MASK_SHOT, nullptr, exit_trace, -60.f);

		if (exit_trace->startsolid && (exit_trace->surface.flags & SURF_HITBOX) != 0)
		{
			// we hit an ent's hitbox, do another trace.
			UTIL_TraceLine(out, start, MASK_SHOT_HULL, exit_trace->m_pEnt, COLLISION_GROUP_NONE, exit_trace);

			if (exit_trace->DidHit() && !exit_trace->startsolid)
			{
				out = exit_trace->endpos;
				return true;
			}
		}
		else
		{
			if (!exit_trace->DidHit() || exit_trace->startsolid)
			{
				if (enter_trace->DidHitNonWorldEntity() && (IsEntityBreakable(enter_trace->m_pEnt) || enter_trace->m_pEnt->GetClientClass()->m_ClassID == _CBaseEntity))
				{
					*exit_trace = *enter_trace;
					exit_trace->endpos = start + dir;
					return true;
				}
			}
			else
			{
				if ((exit_trace->surface.flags & SURF_NODRAW))
				{
					// note; server doesnt seem to check CGameTrace::DidHitNonWorldEntity for both of these breakable checks?
					if (IsEntityBreakable(exit_trace->m_pEnt) && (IsEntityBreakable(enter_trace->m_pEnt) || enter_trace->m_pEnt->GetClientClass()->m_ClassID == _CBaseEntity))
					{
						out = exit_trace->endpos;
						return true;
					}

					if (!(enter_trace->surface.flags & SURF_NODRAW))
						continue;
				}

				if (exit_trace->plane.normal.Dot(dir) <= 1.0f)
				{
					float fr = exit_trace->fraction * 4.0f;
					out -= dir * fr;
					return true;
				}
			}
		}
	}

	return false;
#endif
}
