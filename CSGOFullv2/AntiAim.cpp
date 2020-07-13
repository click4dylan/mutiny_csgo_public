#include "precompiled.h"
#include "AntiAim.h"
#include "Aimbot_imi.h"
#include "LocalPlayer.h"
#include "WeaponController.h"
#include <cstdio>
#include <ppltasks.h>
#include "LocalPlayer.h"

#include "Adriel/stdafx.hpp"
#include "Adriel/input.hpp"

CAntiAim g_AntiAim;

float CAntiAim::WallThickness(Vector from, Vector to, CBaseEntity* skip, CBaseEntity* skip2, Vector& endpos)
{
	Vector endpos1, endpos2;

	Ray_t ray;
	ray.Init(from, to);

	CTraceFilterSkipTwoEntities_CSGO filter;
	filter.SetPassEntity((IHandleEntity*)skip);
	filter.SetPassEntity2((IHandleEntity*)skip2);

	//Interfaces::DebugOverlay->AddLineOverlay(from, to, 255, 0, 0, false, TICKS_TO_TIME(2));

	trace_t trace1, trace2;
	Interfaces::EngineTrace->TraceRay(ray, MASK_SHOT_BRUSHONLY, (ITraceFilter*)&filter, &trace1);

	if (trace1.DidHit())
		endpos1 = endpos = trace1.endpos;
	else
		return -1.f;

	ray.Init(to, from);
	Interfaces::EngineTrace->TraceRay(ray, MASK_SHOT_BRUSHONLY, (ITraceFilter*)&filter, &trace2);

	//Interfaces::DebugOverlay->AddLineOverlay(to, from, 0, 0, 255, false, TICKS_TO_TIME(2));

	if (trace2.DidHit())
		endpos2 = trace2.endpos;

	return endpos1.Dist(endpos2);
}

void CAntiAim::FreeStanding(float* _FreeStandingYaw, float* _FreeStandingFakeYaw)
{
	// init locals
	CBaseEntity* _finalEntity = nullptr;
	std::vector< std::pair< float, CBaseEntity* > > _Entities;

	// loop through all entities
	for (int i = 1; i <= MAX_PLAYERS; ++i)
	{
		auto &_Record = m_PlayerRecords[i];

		auto _Entity = _Record.m_pEntity;

		// not valid
		if (!_Entity || !_Entity->GetAlive() || !_Record.IsValidTarget())
			continue;

		// get fov
		float _fov = GetFov(LocalPlayer.ShootPosition, _Entity->GetBonePosition(HITBOX_HEAD), LocalPlayer.CurrentEyeAngles);

		//Add to vector to sort later
		_Entities.emplace_back(std::make_pair(_fov, _Entity));
	}

	// sort vector
	std::sort(_Entities.begin(), _Entities.end());

	if (!_Entities.empty())
		_finalEntity = _Entities[0].second;

	// backwards if we have no target
	if (!_finalEntity)
	{
		*_FreeStandingYaw += 180.f;
	}
	else
	{
		Vector _backwardsVector = LocalPlayer.Entity->GetAbsOriginDirect() - _finalEntity->GetAbsOriginDirect();
		QAngle _backwardsAngle;

		VectorAngles(_backwardsVector, _backwardsAngle);

		NormalizeAngles(_backwardsAngle);

		float best_rotation			 = 0.f;
		float worst_rotation		 = 0.f;
		const Vector head_position   = LocalPlayer.Entity->GetBonePosition(HITBOX_HEAD);
		const auto local_eyeposition = LocalPlayer.ShootPosition;
		const float step			 = float(2 * M_PI) / 8.f;
		const float radius			 = fabs(Vector(head_position - LocalPlayer.Entity->GetNetworkOrigin()).Length2D());
		float thickest				 = -1.f;
		float thinest				 = 8192.f;

		for (float rotation = 0; rotation < (M_PI * 2.0); rotation += step)
		{
			Vector newhead(radius * cos(rotation) + local_eyeposition.x, radius * sin(rotation) + local_eyeposition.y, local_eyeposition.z);

			Vector endpos;
			float thickness = WallThickness(_finalEntity->GetAbsOriginDirect() + Vector(0, 0, _finalEntity->IsDucked() ? 48.f : 64.f), newhead, _finalEntity, LocalPlayer.Entity, endpos);

			if (thickness > thickest)
			{
				thickest	  = thickness;
				best_rotation = rotation;
			}

			if (thickness < thinest)
			{
				thinest		   = thickness;
				worst_rotation = rotation;
			}
		}

		*_FreeStandingYaw	 = RAD2DEG(best_rotation);
		*_FreeStandingFakeYaw = RAD2DEG(worst_rotation);
	}
}

bool CAntiAim::ApplyFreestanding(float & base_yaw, CBaseEntity* local_entity, CBaseEntity* closest_enemy)
{
	const int damage_tolerance = 1;
	std::vector<CBaseEntity*> enemies;

	//QAngle va;
	//Interfaces::EngineClient->GetViewAngles(va);

	float edge_offset = 0.f;

	float lowest_fov = 360.f;

	bool find_target = false;

	// grab our lowest fov target from the ragebot if available for optimization
	if (closest_enemy == nullptr && g_Ragebot.m_LowestFOVTarget.m_iEntIndex >= 0)
	{
		closest_enemy = Interfaces::ClientEntList->GetBaseEntity(g_Ragebot.m_LowestFOVTarget.m_iEntIndex);

		if (!closest_enemy || (!variable::get().ragebot.b_freestand_dormant && closest_enemy->GetDormant()) || !closest_enemy->IsEnemy(LocalPlayer.Entity))
			find_target = true;
	}
	// find the closest player via fov
	if (find_target)
	{
		for (int i = 1; i <= MAX_PLAYERS; ++i)
		{
			auto ent = &m_PlayerRecords[i];
			if (!ent->m_pEntity || ent->m_pEntity->IsLocalPlayer() || !ent->IsValidTarget() || !ent->m_pEntity->GetAlive())
				continue;

			if (ent->m_bDormant && !variable::get().ragebot.b_freestand_dormant)
				continue;

			float fov = GetFov(LocalPlayer.ShootPosition, ent->m_pEntity->GetBonePosition(HITBOX_HEAD), LocalPlayer.CurrentEyeAngles);
			if (fov < lowest_fov)
			{
				lowest_fov = fov;
				closest_enemy = ent->m_pEntity;
			}

			enemies.push_back(ent->m_pEntity);
		}
	}

	// if we have no enemy, don't freestand
	if (!closest_enemy || !closest_enemy->GetAlive())
	{
		m_iFreestandType = FREESTAND_NONE;
		return false;
	}

	// Calculate the angle to us
	QAngle angle_to_us;
	VectorAngles(local_entity->GetAbsOriginDirect() - closest_enemy->GetAbsOriginDirect(), angle_to_us);
	NormalizeAngles(angle_to_us);

	float best_rotation = 0.f;
	float worst_rotation = 0.f;
	//const Vector head_position = local_entity->GetBonePosition(HITBOX_HEAD);
	const auto local_eyeposition = local_entity->GetEyePosition(); //LocalPlayer.ShootPosition;
	const float step = (2.f * M_PI) / 8.f;
	const float radius = 20.f;//fabs(Vector(head_position - LocalPlayer.Entity->GetNetworkOrigin()).Length2D());

	float thickest = -1.f;
	//float thinest = 8192.f;
	Vector best_head, best_endpos;
	for (float rotation = 0; rotation < (2.f * M_PI); rotation += step)
	{
		Vector newhead = Vector(radius * cos(rotation) + local_eyeposition.x, radius * sin(rotation) + local_eyeposition.y, local_eyeposition.z);

		Vector endpos;
		float thickness = WallThickness(closest_enemy->GetAbsOriginDirect() + Vector(0, 0, closest_enemy->IsDucked() ? 48.f : 64.f), newhead, closest_enemy, local_entity, endpos);

		if (thickness > thickest)
		{
			thickest = thickness;
			best_rotation = rotation;
			best_head = Vector(radius * cos(best_rotation) + local_eyeposition.x, radius * sin(best_rotation) + local_eyeposition.y, local_eyeposition.z);
			best_endpos = endpos;
		}

		//if (thickness < thinest)
		//{
		//	thinest = thickness;
		//	worst_rotation = rotation;
		//}
	}

	//Interfaces::DebugOverlay->AddBoxOverlay(best_endpos, { -1.f, -1.f, -1.f }, { 1.f,1.f,1.f }, angZero, 255, 0, 0, 255, TICKS_TO_TIME(2));

	if (thickest != -1.f /*&& wall_dist <= variable::get().ragebot.f_freestanding_walldist*/)
	{
		base_yaw = AngleNormalize(RAD2DEG(best_rotation));

		//Interfaces::DebugOverlay->AddLineOverlay(closest_enemy->GetAbsOriginDirect() + Vector(0, 0, closest_enemy->IsDucked() ? 48.f : 64.f), best_head, 0, 255, 0, false, TICKS_TO_TIME(2));

		//printf("wall dist: %f | best thickness: %f | base yaw: %f\n", wall_dist, thickest, base_yaw);
		return true;
	}

	return false;
}

Vector rotate_and_extend(const Vector &base_pos, const float yaw, const float distance) 
{
	Vector dir;
	AngleVectors(QAngle(0, yaw, 0.f), &dir);

	return base_pos + (dir * distance);
}

float get_rotated_lby(float base_lby, float yaw)
{
	float delta = yaw - base_lby;
	NormalizeAngle(delta);

	if (fabs(delta) < 25.f)
		return base_lby;

	if (delta > 0.f)
		return yaw + 25.f;

	return yaw;
}

float rotate_lby_yaw(int right_dist, int left_dist, float right_yaw, float left_yaw, float delta, float &base_yaw, float &lby, bool perfect, bool symmetric)
{
	const bool prefer_right = (right_dist < left_dist);

	base_yaw = (prefer_right ? right_yaw : left_yaw);
	if (symmetric)
		delta *= (prefer_right ? -1.f : 1.f);

	if (perfect) {
		if (LocalPlayer.Entity->GetVelocity().Length2D() > 0.01f)
			base_yaw = get_rotated_lby(base_yaw + delta, base_yaw);
		else
			base_yaw = get_rotated_lby(LocalPlayer.Entity->GetLowerBodyYaw(), base_yaw);
	}

	lby = base_yaw + delta;

	float tmp = LocalPlayer.Entity->GetLowerBodyYaw() - lby;

	if (fabs(tmp) < 35.f) {
		base_yaw = LocalPlayer.Entity->GetLowerBodyYaw() - delta;
		lby = base_yaw + delta;
	}

	return true;
};

bool moving_via_cmd(CUserCmd* cmd)
{
	if (!cmd)
		return false;

	return (cmd->buttons & IN_FORWARD || cmd->buttons & IN_BACK || cmd->buttons & IN_LEFT ||
		cmd->buttons & IN_RIGHT || cmd->buttons & IN_MOVELEFT || cmd->buttons & IN_MOVERIGHT);
}

bool CAntiAim::V4Freestanding(float& base_yaw, float& lby)
{
	// todo: nit; calculate max damage of all enemies, not just the main target
	std::vector<CBaseEntity *> enemies;

	QAngle va;
	Interfaces::EngineClient->GetViewAngles(va);

	float edge_lby_offset = 0.f;

	float lowest_fov = 360.f;
	CBaseEntity* closest_enemy = nullptr;

	bool find_target = variable::get().ragebot.b_freestand_dormant;

	// grab our lowest fov target from the ragebot if available for optimization
	if (!find_target)
	{
		if (g_Ragebot.m_LowestFOVTarget.m_iEntIndex >= 0)
		{
			closest_enemy = Interfaces::ClientEntList->GetBaseEntity(g_Ragebot.m_LowestFOVTarget.m_iEntIndex);

			if (!closest_enemy)
				find_target = true;
		}
	}
	// find the closest player via fov
	if (find_target)
	{
		for (int i = 1; i < 64; ++i)
		{
			auto ent = Interfaces::ClientEntList->GetBaseEntity(i);
			if (!ent || ent->IsLocalPlayer() || !ent->IsEnemy(LocalPlayer.Entity) || ent->GetImmune() || !ent->GetAlive())
				continue;

			if (ent->GetDormant() && !variable::get().ragebot.b_freestand_dormant)
				continue;

			float fov = GetFov(LocalPlayer.ShootPosition, ent->GetBonePosition(HITBOX_HEAD), LocalPlayer.CurrentEyeAngles);
			if (fov < lowest_fov)
			{
				lowest_fov = fov;
				closest_enemy = ent;
			}

			enemies.push_back(ent);
		}
	}

	// if we have no enemy or they don't have a weapon, don't freestand
	if (!closest_enemy || !closest_enemy->GetWeapon())
	{
		m_iFreestandType = FREESTAND_NONE;
		return false;
	}

	CBaseCombatWeapon* enemy_weap = closest_enemy->GetWeapon();
	CBaseCombatWeapon* weap = LocalPlayer.CurrentWeapon;
	const Vector enemy_pos = closest_enemy->GetBonePositionCachedOnly(HITBOX_HEAD, closest_enemy->GetBoneAccessor()->GetBoneArrayForWrite()); //closest_enemy->Weapon_ShootPosition();
	const Vector head_pos = LocalPlayer.ShootPosition;

	//Interfaces::DebugOverlay->AddTextOverlay(head_pos, TICKS_TO_TIME(2), "%f %f %f", head_pos.x, head_pos.y, head_pos.z);

	float damage_tolerance = 500.f;
	if (weap != nullptr && weap->GetCSWpnData() != nullptr)
	{
		damage_tolerance = static_cast<float>(weap->GetCSWpnData()->iDamage);
		ScaleDamage(HITGROUP_HEAD, LocalPlayer.Entity, weap->GetCSWpnData()->flArmorRatio, damage_tolerance);
	}


	// Calculate the to-angle towards our enemy
	QAngle ang;
	VectorAngles(closest_enemy->GetAbsOriginDirect() - LocalPlayer.Entity->GetAbsOriginDirect(), ang);

	const float at_target_yaw = ang.y;
	const float back_yaw = at_target_yaw + 180.f;
	const float left_yaw = at_target_yaw + 90.f;
	const float right_yaw = at_target_yaw - 90.f;

	Interfaces::DebugOverlay->AddBoxOverlay(head_pos, { -1.f, -1.f, -1.f }, { 1.f,1.f,1.f }, angZero, 0, 255, 0, 255, TICKS_TO_TIME(2));
	Interfaces::DebugOverlay->AddBoxOverlay(enemy_pos, { -1.f, -1.f, -1.f }, { 1.f,1.f,1.f }, angZero, 255, 0, 0, 255, TICKS_TO_TIME(2));

	Vector head_left_pos = rotate_and_extend(head_pos, left_yaw, 17.f);
	Vector head_right_pos = rotate_and_extend(head_pos, right_yaw, 17.f);
	Vector head_back_pos = rotate_and_extend(head_pos, back_yaw, 17.f);

	Interfaces::DebugOverlay->AddBoxOverlay(head_left_pos, { -1.f, -1.f, -1.f }, { 1.f,1.f,1.f }, angZero, 255, 255, 0, 255, TICKS_TO_TIME(2));
	Interfaces::DebugOverlay->AddBoxOverlay(head_right_pos, { -1.f, -1.f, -1.f }, { 1.f,1.f,1.f }, angZero, 0, 255, 255, 255, TICKS_TO_TIME(2));
	Interfaces::DebugOverlay->AddBoxOverlay(head_back_pos, { -1.f, -1.f, -1.f }, { 1.f,1.f,1.f }, angZero, 0, 0, 0, 255, TICKS_TO_TIME(2));

	// get the damage for each position
	float left_damage = 0.f, right_damage = 0.f, back_damage = 0.f;
	bool left_pen = false, right_pen = false, back_pen = false;

	Autowall_Output_t output;

	// get left head damage from the enemy's position
	Autowall(head_left_pos, enemy_pos, output, false, false, closest_enemy);
	left_damage = output.damage_dealt;//output.penetrated_wall ? output.damage_dealt : 0.f;
	left_pen = output.penetrated_wall;

	Interfaces::DebugOverlay->AddTextOverlayRGB(head_left_pos + Vector{ 0.f, -2.f, 0.f }, 0, TICKS_TO_TIME(2), 255, 255, 0, output.penetrated_wall ? 255 : 50, "%f", left_damage);
	Interfaces::DebugOverlay->AddLineOverlayAlpha(head_left_pos, enemy_pos, 255, 255, 0, output.penetrated_wall ? 255 : 50, false, TICKS_TO_TIME(2));

	// get right head damage
	Autowall(head_right_pos, enemy_pos, output, false, false, closest_enemy);
	right_damage = output.damage_dealt;//output.penetrated_wall ? output.damage_dealt : 0.f;
	right_pen = output.penetrated_wall;

	Interfaces::DebugOverlay->AddTextOverlayRGB(head_right_pos + Vector{ 0.f, -2.f, 0.f }, 0, TICKS_TO_TIME(2), 0, 255, 255, output.penetrated_wall ? 255 : 50, "%f", right_damage);
	Interfaces::DebugOverlay->AddLineOverlayAlpha(head_right_pos, enemy_pos, 0, 255, 255, output.penetrated_wall ? 255 : 50, false, TICKS_TO_TIME(2));

	Autowall(head_back_pos, enemy_pos, output, false, false, closest_enemy);
	back_damage = output.damage_dealt;// output.penetrated_wall ? output.damage_dealt : 0.f;
	back_pen = output.penetrated_wall;

	Interfaces::DebugOverlay->AddTextOverlayRGB(head_back_pos + Vector{ 0.f, -2.f, 0.f }, 0, TICKS_TO_TIME(2), 0, 0, 0, output.penetrated_wall ? 255 : 100, "%f", back_damage);
	Interfaces::DebugOverlay->AddLineOverlayAlpha(head_back_pos, enemy_pos, 0, 0, 0, output.penetrated_wall ? 255 : 50, false, TICKS_TO_TIME(2));

	// check to see if we have too much damage on both sides
	if (left_damage > damage_tolerance && right_damage > damage_tolerance) {

		// try to do backwards
		if (back_damage < damage_tolerance) {
			lby = base_yaw = back_yaw;
			m_iFreestandType = FREESTAND_BACK;
			return true;
		}

		m_iFreestandType = FREESTAND_NONE;
		return false;
	}

	const float auto_backwards_height = variable::get().ragebot.i_auto_backwards;

	bool moving, in_air;
	bool this_tick_is_lby_update = LocalPlayer.IsCurrentTickAnLBYUpdate(moving, in_air);

	// lets keep looking for a better angle for our head then
	if (left_damage == right_damage) 
	{
		if (variable::get().ragebot.b_auto_backwards) 
		{
			// check if we're on top of them.

			float height = ang.x;
			NormalizeAngle(height);
			
			// if so - do backwards freestanding
			if (height > auto_backwards_height) {
				lby = base_yaw = back_yaw;
				m_iFreestandType = FREESTAND_BACK;
				return true;
			}
		}

		// try to do some traces further out then ( instead of 17, we'll do 50 units )
		head_right_pos = rotate_and_extend(head_pos, right_yaw, 50.f);
		head_left_pos = rotate_and_extend(head_pos, left_yaw, 50.f);

		Autowall(head_right_pos, enemy_pos, output, false, false, closest_enemy);
		right_damage = output.damage_dealt;//output.penetrated_wall ? output.damage_dealt : 0.f;

		Interfaces::DebugOverlay->AddTextOverlayRGB(head_right_pos + Vector{ 0.f, -4.f, 0.f }, 0, TICKS_TO_TIME(2), 0, 255, 255, output.penetrated_wall ? 255 : 50, "%f", right_damage);
		Interfaces::DebugOverlay->AddLineOverlayAlpha(head_right_pos, enemy_pos, 0, 255, 255, output.penetrated_wall ? 255 : 50, false, TICKS_TO_TIME(2));

		Autowall(head_left_pos, enemy_pos, output, false, false, closest_enemy);
		left_damage = output.damage_dealt;//output.penetrated_wall ? output.damage_dealt : 0.f;

		Interfaces::DebugOverlay->AddTextOverlayRGB(head_left_pos + Vector{ 0.f, -4.f, 0.f }, 0, TICKS_TO_TIME(2), 255, 255, 0, output.penetrated_wall ? 255 : 50, "%f", left_damage);
		Interfaces::DebugOverlay->AddLineOverlayAlpha(head_left_pos, enemy_pos, 255, 255, 0, output.penetrated_wall ? 255 : 50, false, TICKS_TO_TIME(2));

		if (left_damage == right_damage) 
		{
			// todo: nit; only do this if we have 1 target so we don't have to call autowall
			// return the side that is closest to the wall
			CGameTrace trace;
			CTraceFilterWorldAndPropsOnly filter{};

			Ray_t ray;
			ray.Init(head_pos, rotate_and_extend(head_pos, right_yaw, 17.f));
			Interfaces::EngineTrace->TraceRay(ray, MASK_ALL, (ITraceFilter *)&filter, &trace);

			head_right_pos = trace.endpos;

			ray.Init(head_pos, rotate_and_extend(head_pos, left_yaw, 17.f));
			Interfaces::EngineTrace->TraceRay(ray, MASK_ALL, (ITraceFilter *)&filter, &trace);

			head_left_pos = trace.endpos;

			// right pos
			ray.Init(head_right_pos, enemy_pos);
			Interfaces::EngineTrace->TraceRay(ray, MASK_ALL, (ITraceFilter *)&filter, &trace);
			const float right_dist_from_wall = (head_right_pos - trace.endpos).Length();

			// left pos
			ray.Init(head_left_pos, enemy_pos);
			Interfaces::EngineTrace->TraceRay(ray, MASK_ALL, (ITraceFilter *)&filter, &trace);
			const float left_dist_from_wall = (head_left_pos - trace.endpos).Length();

			if (fabs(right_dist_from_wall - left_dist_from_wall) > 15.f) {
				rotate_lby_yaw(right_dist_from_wall, left_dist_from_wall, right_yaw, left_yaw, edge_lby_offset, base_yaw, lby, true, false);

				if ((moving && !moving_via_cmd(CurrentUserCmd.cmd)) || !this_tick_is_lby_update && !moving)
					base_yaw = lby + 60.f;

				m_iFreestandType = (right_dist_from_wall < left_dist_from_wall ? FREESTAND_RIGHT : FREESTAND_LEFT);
				return true;
			}

			// do backwards freestanding
			lby = base_yaw = back_yaw;
			m_iFreestandType = FREESTAND_BACK;
			return true;
		}
		else 
		{
			rotate_lby_yaw(right_damage, left_damage, right_yaw, left_yaw, edge_lby_offset, base_yaw, lby, true, false);

			if ((moving && !moving_via_cmd(CurrentUserCmd.cmd)) || !this_tick_is_lby_update && !moving)
				base_yaw = lby + 60.f;

			m_iFreestandType = FREESTAND_RIGHT;
			return true;
		}
	}
	// found an angle that does less damage than the other one
	else 
	{
		if (variable::get().ragebot.b_auto_backwards) {
			// check if we're on top of them.
			float height = ang.x;
			NormalizeAngle(height);

			// if so - do backwards freestanding
			if (height > auto_backwards_height) {
				lby = base_yaw = back_yaw;
				m_iFreestandType = FREESTAND_BACK;
				return true;
			}
		}

		// check to see if the left or right have penetrated a wall, if not, we're going to always default backwards
		if (!left_pen && !right_pen)
		{
			lby = base_yaw = back_yaw;
			m_iFreestandType = FREESTAND_BACK;
			return true;
		}

		const bool prefer_right = (right_damage < left_damage);
		base_yaw = (prefer_right) ? right_yaw : left_yaw;

		m_iFreestandType = (prefer_right ? FREESTAND_RIGHT : FREESTAND_LEFT);
		return true;
		//lby = base_yaw + edge_lby_offset;
		//
		//float tmp = LocalPlayer.Entity->GetLowerBodyYaw() - lby;
		//NormalizeAngle(tmp);
		//
		//if (fabs(tmp) < 35.f) {
		//	base_yaw = LocalPlayer.Entity->GetLowerBodyYaw() - lby;
		//	lby = base_yaw + edge_lby_offset;
		//}
		//
		////std::cout << "[freestanding-lby] run" << std::endl;
		//Vector lby_pos = rotate_and_extend(head_pos, lby, 18.f);
		//
		//Autowall(lby_pos, enemy_pos, output, false, false, closest_enemy);
		//float lby_damage = output.damage_dealt;// output.penetrated_wall ? output.damage_dealt : 0.f;
		//
		//Interfaces::DebugOverlay->AddTextOverlayRGB(lby_pos + Vector{ 0.f, -4.f, 0.f }, 0, TICKS_TO_TIME(2), 255, 0, 255, output.penetrated_wall ? 255 : 50, "%f", lby_damage);
		//Interfaces::DebugOverlay->AddLineOverlayAlpha(lby_pos, enemy_pos, 255, 0, 255, output.penetrated_wall ? 255 : 50, false, TICKS_TO_TIME(2));
		//
		//if (lby_damage > 0.f) {
		//	lby = base_yaw + (prefer_right ? -115.f : 115.f);
		//
		//	lby_pos = rotate_and_extend(head_pos, lby, 18.f);
		//
		//	Autowall(enemy_pos, lby_pos, output, false, false, closest_enemy);
		//	lby_damage = output.damage_dealt; //output.penetrated_wall ? output.damage_dealt : 0.f;
		//
		//	Interfaces::DebugOverlay->AddTextOverlayRGB(lby_pos + Vector{ 0.f, -4.f, 0.f }, 0, TICKS_TO_TIME(2), 255, 0, 255, output.penetrated_wall ? 255 : 50, "%f", lby_damage);
		//	Interfaces::DebugOverlay->AddLineOverlayAlpha(lby_pos, enemy_pos, 255, 0, 255, output.penetrated_wall ? 255 : 50, false, TICKS_TO_TIME(2));
		//
		//	if (lby_damage > 0)
		//		lby = base_yaw;
		//}
		//else 
		//{
		//
		//	if ((moving && !moving_via_cmd(CurrentUserCmd.cmd)) || !this_tick_is_lby_update && !moving)
		//		base_yaw = lby + 60.f;
		//
		//	m_iFreestandType = (prefer_right ? FREESTAND_RIGHT : FREESTAND_LEFT);
		//	return true;
		//}
	}

	m_iFreestandType = FREESTAND_NONE;
	return false;
}

float CAntiAim::GetMaxDesyncDelta()
{
	const auto animstate = LocalPlayer.Entity->GetPlayerAnimState();

	float flLastDuckAmount = animstate->m_fDuckAmount;
	float flNewDuckAmount = clamp(LocalPlayer.Entity->GetDuckAmount() + animstate->m_flHitGroundCycle, 0.0f, 1.0f);
	float flDuckSmooth = TICKS_TO_TIME(1) * 6.0f;
	float flDuckDelta = flNewDuckAmount - flLastDuckAmount;

	if (flDuckDelta <= flDuckSmooth) {
		if (-flDuckSmooth > flDuckDelta)
			flNewDuckAmount = flLastDuckAmount - flDuckSmooth;
	}
	else {
		flNewDuckAmount = flDuckSmooth + flLastDuckAmount;
	}

	float newduckamount = clamp(flNewDuckAmount, 0.0f, 1.0f);

	Vector velocity = *animstate->pBaseEntity->GetAbsVelocity();
	Vector newvelocity = g_LagCompensation.GetSmoothedVelocity(animstate->m_flLastClientSideAnimationUpdateTimeDelta * 2000.0f, velocity, animstate->m_vVelocity);

	float speed = std::fmin(newvelocity.Length(), 260.0f);

	CBaseCombatWeapon *weapon = LocalPlayer.Entity->GetWeapon();

	float flMaxMovementSpeed = 260.0f;
	if (weapon)
		flMaxMovementSpeed = std::fmax(weapon->GetMaxSpeed3(), 0.001f);

	const float m_flRunningSpeed = clamp(speed / (flMaxMovementSpeed * 0.520f), 0.0f, 1.0f);

	float flYawModifier = (((animstate->m_flGroundFraction * -0.3f) - 0.2f) * m_flRunningSpeed) + 1.0f;

	if (newduckamount > 0.f)
	{
		const float m_flDuckingSpeed = clamp(speed / (flMaxMovementSpeed * 0.340f), 0.0f, 1.0f);
		flYawModifier += (newduckamount * m_flDuckingSpeed) * (0.5f - flYawModifier);
	}

	return animstate->m_flMaxYaw * flYawModifier;
}

void CAntiAim::PreCreateMove()
{
	if (input::get().was_key_pressed(variable::get().ragebot.i_manual_aa_lean_dir))
		g_AntiAim.m_bManualSwapDesyncDir = !g_AntiAim.m_bManualSwapDesyncDir;

	if (input::get().was_key_pressed(variable::get().ragebot.i_manual_aa_left_key))
	{
		g_AntiAim.m_iManualType = g_AntiAim.m_iManualType != MANUAL_LEFT ? MANUAL_LEFT : MANUAL_NONE;
	}
	else if (input::get().was_key_pressed(variable::get().ragebot.i_manual_aa_right_key))
	{
		g_AntiAim.m_iManualType = g_AntiAim.m_iManualType != MANUAL_RIGHT ? MANUAL_RIGHT : MANUAL_NONE;
	}
	else if (input::get().was_key_pressed(variable::get().ragebot.i_manual_aa_back_key))
	{
		g_AntiAim.m_iManualType = g_AntiAim.m_iManualType != MANUAL_BACK ? MANUAL_BACK : MANUAL_NONE;
	}
}

bool CAntiAim::ShouldRun()
{
	// no weapon
	if (!LocalPlayer.CurrentWeapon)
		return true;

	// ladder or noclip
	if ((LocalPlayer.Entity->GetMoveType() == MOVETYPE_NOCLIP || LocalPlayer.Entity->GetMoveType() == MOVETYPE_LADDER) && (CurrentUserCmd.cmd->forwardmove != 0.0f || CurrentUserCmd.cmd->sidemove != 0.0f || CurrentUserCmd.cmd->upmove != 0.0f))
		return false;

	// in_use
	const bool planting_or_defusing = ((LocalPlayer.WeaponVars.IsC4 && LocalPlayer.CurrentWeapon && LocalPlayer.CurrentWeapon->StartedArming()) || LocalPlayer.Entity->IsDefusing());

	if (CurrentUserCmd.IsUsing() && !planting_or_defusing)
		return false;

	// burst fire
	//if (LocalPlayer.WeaponVars.IsBurstableWeapon && LocalPlayer.CurrentWeapon->GetBurstShotsRemaining() > 0)
	//	return false;

	if (!LocalPlayer.WeaponVars.IsC4 && LocalPlayer.WeaponWillFireBurstShotThisTick)
		return false;

	// will fire
	if(!LocalPlayer.WeaponVars.IsC4 && g_WeaponController.WeaponDidFire(CurrentUserCmd.cmd->buttons | IN_ATTACK) == 1 && g_Ragebot.HasTarget() && (g_WeaponController.ShouldFire() || LocalPlayer.IsManuallyFiring(LocalPlayer.CurrentWeapon)))
		return false;

	// in grenade throw
	if (LocalPlayer.WeaponVars.IsGrenade && LocalPlayer.CurrentWeapon->IsInThrow())
		return false;

	// primary fire
	if (g_WeaponController.WeaponDidFire(CurrentUserCmd.cmd->buttons | IN_ATTACK) == 1 && CurrentUserCmd.IsAttacking() && !LocalPlayer.WeaponVars.IsGrenade && !LocalPlayer.WeaponVars.IsC4)
		return false;

	// secondary fire for knife and revolver
	if ((LocalPlayer.WeaponVars.IsKnife || LocalPlayer.WeaponVars.IsRevolver) && g_WeaponController.WeaponDidFire(CurrentUserCmd.cmd->buttons | IN_ATTACK2) == 2 && CurrentUserCmd.IsSecondaryAttacking())
		return false;

	// run
	return true;
}

void CAntiAim::Pitch()
{
#ifdef IMI_MENU
	switch (g_Convars.AntiAim.antiaim_pitch->GetInt())
#else
	switch(LocalPlayer.Config_GetPitch())
#endif
	{
#ifdef IMI_MENU
		case Pitch_e::PITCH_DOWN:
			LocalPlayer.FinalEyeAngles.x = 89.f;
			break; //down
		case Pitch_e::PITCH_UP:
			LocalPlayer.FinalEyeAngles.x = -89.f;
			break; //up
		case Pitch_e::PITCH_JITTER:
			LocalPlayer.FinalEyeAngles.x = (CurrentUserCmd.cmd->command_number % 2) ? 89.f : -89.f;
			break; //jitter
		case Pitch_e::PITCH_FAKEDOWN:
			LocalPlayer.FinalEyeAngles.x = -180.f;
			break;
		case Pitch_e::PITCH_FAKEUP:
			LocalPlayer.FinalEyeAngles.x = 180.f;
			break;
		case Pitch_e::PITCH_LISPDOWN:
			LocalPlayer.FinalEyeAngles.x = 1080.f;
			break;
		case Pitch_e::PITCH_LISPUP:
			LocalPlayer.FinalEyeAngles.x = -1080.f;
			break;
		case Pitch_e::PITCH_FAKEZERO:
			LocalPlayer.FinalEyeAngles.x = 0.f;
			break;
#else
		// zero
		case 1:
		{
			LocalPlayer.FinalEyeAngles.x = 0.f;
			break;
		}
		// down
		case 2:
		{
			LocalPlayer.FinalEyeAngles.x = 89.f;
			break;
		}
		// up
		case 3:
		{
			LocalPlayer.FinalEyeAngles.x = -89.f;
			break;
		}
		// minimal
		case 4:
		{
			LocalPlayer.FinalEyeAngles.x = LocalPlayer.Entity->GetPlayerAnimState() ? LocalPlayer.Entity->GetPlayerAnimState()->m_flMaxPitch : 89.f;
			break;
		}
		// random
		case 5:
		{
			LocalPlayer.FinalEyeAngles.x = RandomFloat(-89.f, 89.f);
			break;
		}
		// custom
		case 6:
		{
			LocalPlayer.FinalEyeAngles.x = LocalPlayer.Config_GetCustomPitch();
			break;
		}
#endif
		default:
			break;
	}
}

void CAntiAim::YawBase(float& _yaw)
{
	switch (LocalPlayer.Config_GetYaw())
	{
		// at target
		case BASE_ATTARGET:
		{
			if (g_Ragebot.HasTarget())
			{
				_yaw = g_Ragebot.GetAimAngles().y;
			}
			else if (g_Ragebot.m_LowestFOVTarget.m_iEntIndex >= 0)
			{
				_yaw = g_Ragebot.m_LowestFOVTarget.m_AtTargetAngle.y;
			}
			break;
		}
		// movement direction
		//case BASE_MOVEMENTDIR:
		//	// we're moving
		//	if (LocalPlayer.Entity->GetVelocity().Length() > 0.f)
		//	{
		//		// get movement direction
		//		QAngle _movementDirection;
		//		VectorAngles(LocalPlayer.Entity->GetVelocity(), _movementDirection);
		//
		//		// set yaw
		//		_yaw = _movementDirection.y;
		//	}
		//	break;
		//// free standing
		//case BASE_FREESTANDING:
		//	// run freestanding
		//	FreeStanding(&best_yaw, &worst_yaw);
		//
		//	// set yaw
		//	_yaw = best_yaw;
		//	break;
		// local view
		default:
			break;
	}
}

void CAntiAim::YawAdjust(float& _yaw)
{
	// if we're freestanding, we don't want to add any extra yaws.
	if (m_iFreestandType != FREESTAND_NONE)
		return;

	// do yaw anti aim
	if (m_iManualType == MANUAL_LEFT)
	{
		_yaw += 90.f;
	}
	else if (m_iManualType == MANUAL_RIGHT)
	{
		_yaw -= 90.f;
	}
	else if (m_iManualType == MANUAL_BACK)
	{
		_yaw += 180.f;
	}
	else
	{
		_yaw += LocalPlayer.Config_GetYawAdd();
	}

	static float LastAngle = _yaw;

	if (LocalPlayer.IsSwaying())
	{
		_yaw += 180.0f;
		NormalizeAngle(_yaw);
		static float LastSwapTime = 0.0f;
		static float TargetYDegrees = 91.0f;
		static float LastTargetYDegrees = 91.0f;
		static float TimeSpentAtTargetDegrees = 0.0f;
		static float TimeToWaitAtTargetDegrees = 0.01f;
		float SpinDegPerTick = LocalPlayer.GetSwaySpeed();
		float TargetYaw = _yaw;

		TargetYaw += TargetYDegrees;

		NormalizeAngle(TargetYaw);

		_yaw = LastAngle;
		NormalizeAngle(_yaw);
		float delta_angle = AngleDiff(TargetYaw, _yaw);

		bool is_close_to_fake = !LocalPlayer.m_bDesynced || fabsf(AngleDiff(LocalPlayer.real_playerbackup.AbsAngles.y, LocalPlayer.fake_playerbackup.AbsAngles.y)) < 25.0f; //fabsf(delta_angle - 90.0f) < 5.0f;

		if (is_close_to_fake)
		{
			if (Interfaces::Globals->realtime - LastSwapTime > 0.1f)
			{
				m_bSwayDesyncStyle = !m_bSwayDesyncStyle;
				LastSwapTime = Interfaces::Globals->realtime;
			}
		}

		if (delta_angle < -SpinDegPerTick)
		{
			TimeSpentAtTargetDegrees = 0.0f;
			//if (is_close_to_fake)
			//	SpinDegPerTick = 30.0f;
			//else
			SpinDegPerTick = fminf(SpinDegPerTick, fabsf(delta_angle));
			_yaw -= SpinDegPerTick;
		}
		else if (delta_angle > SpinDegPerTick)
		{
			TimeSpentAtTargetDegrees = 0.0f;
			//if (is_close_to_fake)
			//	SpinDegPerTick = 30.0f;
			//else
			SpinDegPerTick = fminf(SpinDegPerTick, fabsf(delta_angle));
			_yaw += SpinDegPerTick;
		}
		else if (delta_angle != 0.0f)
		{
			//if (is_close_to_fake)
			//	_yaw += delta_angle > 0.0f ? 30.0f : -30.0f;
			//else
			_yaw += delta_angle;
		}
		else
		{
			TimeSpentAtTargetDegrees += Interfaces::Globals->frametime;
			_yaw = TargetYaw;

			if (TimeSpentAtTargetDegrees >= TimeToWaitAtTargetDegrees)
			{
				float amt = 90.0f + 35.0f;
				if (LastTargetYDegrees == amt)
				{
					LastTargetYDegrees = TargetYDegrees;
					TargetYDegrees = -amt;
				}
				else
				{
					LastTargetYDegrees = TargetYDegrees;
					TargetYDegrees = amt;
				}
				TimeToWaitAtTargetDegrees = LocalPlayer.ShouldDelaySway() ? LocalPlayer.GetSwayDelay() : 0.0f;
				TimeSpentAtTargetDegrees = 0.0f;
			}
		}
	}

	NormalizeAngle(_yaw);
	LastAngle = _yaw;
}

void CAntiAim::Experimental()
{
	// init locals
	static bool _switch = false;
	bool _IsMoving = false, _InAir = false;
	bool _ThisTickIsLBYUpdate		 = LocalPlayer.IsCurrentTickAnLBYUpdate(_IsMoving, _InAir);
	const bool _NextTickIsLBYUpdate		 = LocalPlayer.IsNextAnimUpdateAnLBYUpdate(&_ThisTickIsLBYUpdate);
	const bool _AfterNextTickIsLBYUpdate	 = LocalPlayer.IsAnimUpdateAnLBYUpdate(Interfaces::Globals->curtime + TICKS_TO_TIME(2), &_ThisTickIsLBYUpdate) && !_NextTickIsLBYUpdate && !_ThisTickIsLBYUpdate;
	const bool _LBYWillUpdate		 = _NextTickIsLBYUpdate || _ThisTickIsLBYUpdate;

	m_bLBYWillUpdate = _LBYWillUpdate;
	m_bThisTickIsLBYUpdate = _ThisTickIsLBYUpdate;
	m_bNextTickIsLBYUpdate = _NextTickIsLBYUpdate;
	m_bAfterNextTickIsLBYUpdate = _AfterNextTickIsLBYUpdate;
	m_bIsMoving = _IsMoving;
	m_bInAir = _InAir;

	// init yaw
	float _DesiredYaw = LocalPlayer.FinalEyeAngles.y;
	float _DesiredLBY = LocalPlayer.Entity->GetLowerBodyYaw();

	// check if freestanding is enabled
#if defined DEBUG || defined INTERNAL_DEBUG
	if (LocalPlayer.Config_IsFreestanding())
	{
		V4Freestanding(_DesiredYaw, _DesiredLBY); //ApplyFreestanding(_DesiredYaw);
	}
	else
#endif
	{
		YawBase(_DesiredYaw);
	}

	static float _LastDesiredYaw = _DesiredYaw;
	if (fabsf(AngleDiff(_DesiredYaw, _LastDesiredYaw)) < 5.0f)
		_DesiredYaw = _LastDesiredYaw;
	_LastDesiredYaw = _DesiredYaw;

	// apply yaw adjustments
	YawAdjust(_DesiredYaw);

	// jitter > 0
	if(LocalPlayer.Config_IsJitteringYaw())
	{
		if (!g_Info.ShouldChoke() || g_Info.IsForcedToSend())
			_switch = !_switch;

		float _value = LocalPlayer.Config_GetYawJitter();
		static float _lastvalue = _value;

		if ((_ThisTickIsLBYUpdate || _NextTickIsLBYUpdate) && !_IsMoving)
		{
			_value = _lastvalue;
			_DesiredYaw += _value;
		}
		else
		{
			// +- jitter value
			_DesiredYaw += _switch ? -_value : _value;

			_lastvalue = _value;
		}
	}

	// normalize value
	NormalizeAngle(_DesiredYaw);

	if (!LocalPlayer.IsSwaying() && fabsf(AngleDiff(_DesiredYaw, _LastDesiredYaw)) < 5.0f)
		_DesiredYaw = _LastDesiredYaw;
	_LastDesiredYaw = _DesiredYaw;

	// get desync direction (true - right, false - left)
	float _Config_LBYDelta = LocalPlayer.Config_GetLBYDelta();
	float _LBYDelta = LocalPlayer.GetDesyncStyle() ? _Config_LBYDelta : -_Config_LBYDelta;

	// set desired yaw before desync
	g_Info.m_flUnchokedYaw = g_Info.m_flChokedYaw = _DesiredYaw;

	// we want desync, run checks
	if(!LocalPlayer.IsFakeDucking && !LocalPlayer.IsTeleporting && !LocalPlayer.ApplyTickbaseShift && !g_Info.IsForcedToSend() && LocalPlayer.Config_IsDesyncing())
	{
		// not slow walking
		if (!g_Info.m_bShouldSlide)
		{
			if (!input::get().is_key_down(variable::get().misc.i_minwalk_key))
			{
				// not moving at all
				if (!_IsMoving)
				{
					bool is_tick_base_ready = false;

					if (variable::get().ragebot.exploits.i_ticks_to_wait >= 10 && variable::get().ragebot.exploits.b_multi_tap.get() && !LocalPlayer.IsFakeDucking)
					{
						if (g_Ragebot.GetTarget() != INVALID_PLAYER)
						{
							CPlayerrecord* _playerRecord = &m_PlayerRecords[g_Ragebot.GetTarget()];

							if (_playerRecord && _playerRecord->m_pEntity &&  _playerRecord->m_pEntity->GetAlive())
								is_tick_base_ready = true;
						}
					}

					if (!is_tick_base_ready)
					{
						// make sure lby flicks get choked
						if (_AfterNextTickIsLBYUpdate)
							g_Info.SetShouldChoke(false); //FIXME: WHAT IS THIS

						// lby update soon, choke ticks
						else if (_LBYWillUpdate)
							g_Info.SetShouldChoke(true);
					}
				}
			}
		}
	}

	// apply angle
	ApplyCorrectAngle(_DesiredYaw, _LBYDelta);
}

void CAntiAim::ApplyCorrectAngle(float desired_yaw, float lby_delta)
{
	// reset

	if (!LocalPlayer.Config_IsDesyncing())
	{
		g_Info.m_flUnchokedYaw = g_Info.m_flChokedYaw = desired_yaw;
		m_b_lby_after_stopping_already_updated_once = false;
		return;
	}

	SetYawAndBreakLBY(desired_yaw, lby_delta);
}

void CAntiAim::SetYawAndBreakLBY(float desired_yaw, float desired_lby_delta)
{
	if (LocalPlayer.Entity->GetSpawnTime() != LocalPlayer.m_spawntime)
	{
		LocalPlayer.m_flLastAnimationUpdateTime		= 0.0f;
		m_b_lby_after_stopping_already_updated_once = false;
		return;
	}

	bool sendpacket = !g_Info.ShouldChoke() || g_Info.IsForcedToSend();

	float &dest_yaw = !sendpacket ? g_Info.m_flChokedYaw : g_Info.m_flUnchokedYaw;

	Vector absvelocity = *LocalPlayer.Entity->GetAbsVelocity();
	absvelocity.z = 0.0f;
	Vector newvelocity = g_LagCompensation.GetSmoothedVelocity(/*(Interfaces::Globals->curtime - LocalPlayer.m_flLastAnimationUpdateTime)*/TICKS_TO_TIME(1) * 2000.0f, absvelocity, LocalPlayer.m_lastvelocity);
	float newspeed	 = fminf(newvelocity.Length(), 260.0f);

	NormalizeAngle(desired_yaw);
	float desired_unchoked_yaw = desired_yaw;
	float desync_amt = LocalPlayer.Config_GetDesyncAmount();
	float desired_delta = LocalPlayer.GetDesyncStyle() ? -desync_amt : desync_amt;

	float desired_choked_yaw = AngleNormalize(desired_yaw + desired_delta);//GetBestYawFromDesiredYaw(desired_yaw, LocalPlayer.FinalEyeAngles.x, 10.0f);
	static float last_desired_choked_yaw = desired_choked_yaw;
	if (fabsf(AngleDiff(desired_choked_yaw, last_desired_choked_yaw)) < 2.0f)
		desired_choked_yaw = last_desired_choked_yaw;
	last_desired_choked_yaw = desired_choked_yaw;

	if (!sendpacket)
	{
		desired_yaw = desired_choked_yaw;
	}
	else
	{
		//float dt = 122.01f;
		//if (desired_delta < 0.0f)
		//	desired_choked_yaw += 180.0f;
		//desired_yaw = AngleNormalize(desired_choked_yaw + dt);
	}

	QAngle desired_eyeangles = { LocalPlayer.FinalEyeAngles.x, desired_yaw, LocalPlayer.FinalEyeAngles.z };

	if (newspeed > 0.1f || fabsf(absvelocity.z) > 100.0f)
	{
		//this is causing slowwalk to throw our real straight into our fake
		//if ((g_Info.m_bShouldSlide || g_Convars.AntiAim.antiaim_fakewalk->GetBool()) && g_Convars.AntiAim.antiaim_desync_flick->GetBool() && abs(LocalPlayer.m_flDesynced) > 20.f && fabs(LocalPlayer.m_flDesynced) < 30.f && _switch)
		//	desired_yaw += desired_lby_delta;

		// moving, lby will update immediately
		dest_yaw = desired_yaw;
		m_b_lby_after_stopping_already_updated_once = false;

		// stop
		return;
	}

	// lby is ready to update on the server, flick to the desired delta to stop it from changing
	if (Interfaces::Globals->curtime > LocalPlayer.m_next_lby_update_time)
	{
		m_b_lby_after_stopping_already_updated_once = true;
		desired_eyeangles.y	= AngleNormalize(desired_yaw + desired_lby_delta);
		dest_yaw = desired_eyeangles.y;

		//check if lby timer won't change on server and force it to change by changing our lby
		//this happens if our pre-flick was not done correctly (which is because the desired yaw used during preflick is wrong, due to GetBestYawFromDesiredYaw)
#if 0
		SetUpVelocityResults_t results;
		LocalPlayer.GetSetupVelocityResults(desired_eyeangles, results);

		if (!results.m_lby_timer_updated)
		{
			float original_desired_lby = desired_eyeangles.y;
			float mindelta = -5.0f;
			while (!results.m_lby_timer_updated && mindelta < 180.0f)
			{
				results.ClearResults();
				mindelta += 5.0f;
				desired_eyeangles.y = AngleNormalize(original_desired_lby + mindelta);
				LocalPlayer.GetSetupVelocityResults(desired_eyeangles, results);
			}
			if (!results.m_lby_timer_updated)
				desired_eyeangles.y = original_desired_lby;

			dest_yaw = desired_eyeangles.y;
		}
#endif

		// stop
		return;
	}

	// if on the NEXT animation update the lby will update, force a pre-flick to a wide delta
	// without a pre-flick, you can't break lby to any delta you want
	if (m_b_lby_after_stopping_already_updated_once)
	{
		float anim_update_delta = TICKS_TO_TIME(1); //TICKS_TO_TIME(CurrentUserCmd.m_iNumCommandsToChoke + 1);
		float next_animation_update_time = Interfaces::Globals->curtime + anim_update_delta;

		if (next_animation_update_time > LocalPlayer.m_next_lby_update_time)
		{
			const float target_lby = AngleNormalize(desired_yaw + desired_lby_delta);
			const float delta	  = AngleDiff(target_lby, desired_unchoked_yaw);
			float prebreak = 170.0f;
			if (fabsf(delta) < prebreak)
			{
				desired_eyeangles.y			 = AngleNormalize(target_lby + prebreak);
				g_Info.m_flChokedYaw			 = desired_eyeangles.y;
				LocalPlayer.FinalEyeAngles.y = desired_eyeangles.y;
				return;
			}
		}
	}

	// lby won't update yet, set the angle to whatever we want
	dest_yaw = desired_yaw;
}

void CAntiAim::Run()
{
	// backup original (possible aimbot) angles
	g_Info.m_flUnchokedYaw = g_Info.m_flChokedYaw = LocalPlayer.FinalEyeAngles.y;

	// no anti aim wanted
#ifdef IMI_MENU
	if (!g_Convars.AntiAim.antiaim_enable->GetBool())
#else
	if(!variable::get().ragebot.b_antiaim)
#endif
		return;

	// no anti aim needed
	if (!ShouldRun())
		return;

	// do pitch
	Pitch();

	// do yaw
	Experimental();

	// backup sidemove for dyn sideways
	if (abs(CurrentUserCmd.cmd->sidemove) > 0 && LocalPlayer.Entity->GetFlags() & FL_ONGROUND)
		m_flSideMove = CurrentUserCmd.cmd->sidemove;
}

void CAntiAim::SlowWalk()
{
	// we don't want to fakewalk
	if (!g_Info.m_bShouldSlide)
	{
#ifdef IMI_MENU
		if (!g_Convars.AntiAim.antiaim_fakewalk->GetBool())
		{
#else 
		if (!input::get().is_key_down(variable::get().misc.i_minwalk_key))
		{
			LocalPlayer.bFakeWalking = false;
#endif
			return;
		}
	}
	else
		g_Info.m_bShouldSlide = false;

	auto velocity = LocalPlayer.Entity->GetVelocity();
	const float speed = velocity.Length();

	if (LocalPlayer.CurrentWeapon && LocalPlayer.CurrentWeapon->GetCSWpnData() && speed > 0.1f )
	{
		LocalPlayer.bFakeWalking = true;

		auto weapon_info = LocalPlayer.CurrentWeapon->GetCSWpnData();
		const float third_of_weapon_speed = variable::get().misc.f_minwalk_speed;  //(LocalPlayer.Entity->IsScoped() ? weapon_info->flMaxPlayerSpeedAlt / 3.f : weapon_info->flMaxPlayerSpeed / 3.f);

		//printf("third of weapon speed: %f\n", third_of_weapon_speed);

		QAngle dir{};
		VectorAngles(velocity, dir);

		dir.y = CurrentUserCmd.cmd->viewangles.y - dir.y;

		Vector forward_vec{};
		AngleVectors(dir, &forward_vec);

		Vector slowdown_movement = forward_vec * -speed;

		// if we're moving faster than our target speed
		if (speed >= third_of_weapon_speed)
		{
			// apply the slowdown
			CurrentUserCmd.cmd->forwardmove = slowdown_movement.x;
			CurrentUserCmd.cmd->sidemove = slowdown_movement.y;
		}
	}
	else
	{
		LocalPlayer.bFakeWalking = false;
	}
}
