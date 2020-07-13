#include "nade_prediction.hpp"
#include "adr_math.hpp"
#include "renderer.hpp"
#include "adr_util.hpp"

#include "../LocalPlayer.h"

nade_prediction::nade_prediction()
{
}

nade_prediction::~nade_prediction()
{
}

void nade_prediction::create_move(CUserCmd* p_cmd)
{
	if (!variable::get().visuals.nade_prediction.b_enabled)
		return;

	if (!variable::get().visuals.b_enabled)
		return;

	const bool in_attack = p_cmd->buttons & IN_ATTACK;
	const bool in_attack2 = p_cmd->buttons & IN_ATTACK2;

    i_act = (in_attack && in_attack2) ? ACT_LOB : (in_attack2) ? ACT_DROP : (in_attack) ? ACT_THROW : ACT_NONE;
}

void nade_prediction::override_view(CViewSetup* setup)
{
	if (!variable::get().visuals.nade_prediction.b_enabled)
		return;

	if (!variable::get().visuals.b_enabled)
		return;

	if (!LocalPlayer.Entity)
		return;

	if (!LocalPlayer.Entity->GetAlive())
		return;

	auto p_weapon = LocalPlayer.Entity->GetActiveCSWeapon();
	if (!p_weapon)
		return;

	if (p_weapon->IsGrenade() && i_act != ACT_NONE)
	{
		s_type = p_weapon->GetItemDefinitionIndex();
		simulate(setup->angles);
	}
	else
	{
		s_type = 0;
	}
}

void nade_prediction::render()
{
	auto get_armour_health = [](float fl_damage, int armor_value) -> float
	{
		const auto fl_cur_damage = fl_damage;
		if (fl_cur_damage == 0.0f || armor_value == 0)
			return fl_cur_damage;

		const auto fl_armor_ratio = 0.5f;
		const auto fl_armor_bonus = 0.5f;

		auto fl_new = fl_cur_damage * fl_armor_ratio;
		auto fl_armor = (fl_cur_damage - fl_new) * fl_armor_bonus;

		if (fl_armor > armor_value)
		{
			fl_armor = armor_value * (1.0f / fl_armor_bonus);
			fl_new = fl_cur_damage - fl_armor;
		}

		return fl_new;
	};

	if (!variable::get().visuals.nade_prediction.b_enabled)
		return;

	if (!variable::get().visuals.b_enabled)
		return;

	if (!LocalPlayer.Entity)
		return;

	if (!LocalPlayer.Entity->GetAlive())
		return;

    if (s_type && path.size() > 1)
    {
        Vector nade_start, nade_end;
	    auto prev = path[0];

		for (auto it = path.begin(), end = path.end(); it != end; ++it)
		{
			trace_t tr;
			Ray_t ray;

			ray.Init(std::get<0>(*it), LocalPlayer.Entity->GetEyePosition());
			UTIL_TraceRay(ray, MASK_SHOT, LocalPlayer.Entity, COLLISION_GROUP_NONE, &tr);

			auto col = variable::get().visuals.nade_prediction.col_visible.color();
			if (tr.DidHit())
				col = variable::get().visuals.nade_prediction.col.color();

			if (adr_math::world_to_screen(std::get<0>(prev), nade_start) && adr_math::world_to_screen(std::get<0>(*it), nade_end))
				render::get().add_line(ImVec2(nade_start.x, nade_start.y), ImVec2(nade_end.x, nade_end.y), col.ToImGUI());

			if (std::get<1>(*it))
			{
				QAngle angles;

				adr_math::vector_angles(std::get<0>(prev), angles);
				render::get().add_cube(std::get<0>(*it), 1.f, angles, variable::get().visuals.nade_prediction.col_hit.color().ToImGUI());
			}

			prev = *it;
		}

		auto max_dmg = 0.f;
		std::vector<int> vec_players;

		if (s_type == WEAPON_HEGRENADE || s_type == WEAPON_MOLOTOV || s_type == WEAPON_INCGRENADE)
		{
			for (auto i = 0; i < Interfaces::EngineClient->GetMaxClients(); i++)
			{
				auto p_entity = Interfaces::ClientEntList->GetBaseEntity(i);
				if (!p_entity)
					continue;

				if (!p_entity->GetAlive() || p_entity->GetDormant() || p_entity->GetImmune())
					continue;

				auto vec_origin = p_entity->GetAbsOriginDirect();
				auto dist = vec_origin.Dist(std::get<0>(prev));
				if (dist >= 335.f)
					continue;

				trace_t tr;
				Ray_t ray;

				ray.Init(std::get<0>(prev), vec_origin);
				UTIL_TraceRay(ray, MASK_SHOT, p_entity, COLLISION_GROUP_NONE, &tr);

				if (tr.DidHit())
					continue;

				auto d = (dist - 25.f) / 140.f;
				auto fl_damage = 105.f * exp(-d * d);
				auto dmg = max(static_cast<int>(ceilf(get_armour_health(fl_damage, p_entity->GetArmor()))), 0);

				if (dmg > 1.f)
					vec_players.push_back(i);

				if (dmg > max_dmg)
					max_dmg = dmg;
			}
		}

		QAngle angles;
		adr_math::vector_angles(std::get<0>(prev), angles);
		render::get().add_cube(std::get<0>(prev), 2.5f, angles, variable::get().visuals.nade_prediction.col_hit.color().ToImGUI());
		
    	if (adr_math::world_to_screen(std::get<0>(prev), nade_end))
		{
			if (max_dmg > 0.f)
			{
				//decrypts(0)
				if (s_type == WEAPON_HEGRENADE)
					render::get().add_text(ImVec2(nade_end.x + 5, nade_end.y + 5), adr_util::get_health_color(max_dmg).ToImGUI(), CENTERED_X | DROP_SHADOW, BAKSHEESH_12, XorStr("-%0.f hp"), max_dmg);
				else if ((s_type == WEAPON_MOLOTOV || s_type == WEAPON_INCGRENADE) && !b_fire && !vec_players.empty())
					render::get().add_text(ImVec2(nade_end.x + 5, nade_end.y + 5), Color::Red().ToImGUI(), CENTERED_X | DROP_SHADOW, BAKSHEESH_12, XorStr("%d player(s) will burn."), static_cast<int>(vec_players.size()));
				//encrypts(0)
			}
		}
	}
}

void nade_prediction::setup(Vector& vec_src, Vector& vec_throw, QAngle viewangles) const
{
	auto ang_throw = viewangles;
	auto x = ang_throw.x;

	if (x <= 90.0f)
	{
		if (x<-90.0f)
			x += 360.0f;
	}
	else
		x -= 360.0f;

	const auto a = x - (90.0f - fabs(x)) * 10.0f / 90.0f;
	ang_throw.x = a;

	auto fl_vel = 750.0f * 0.9f;
	static const float power[] = { 1.0f, 1.0f, 0.5f, 0.0f };
	auto b = power[i_act];

	b = b * 0.7f;
	b = b + 0.3f;
	fl_vel *= b;

	Vector v_forward, v_right, v_up;
	adr_math::angle_vector(ang_throw, v_forward, v_right, v_up);

	vec_src = LocalPlayer.Entity->GetAbsOriginDirect();
	vec_src += LocalPlayer.Entity->GetViewOffset();

	const auto off = (power[i_act] * 12.0f) - 12.0f;
	vec_src.z += off;

	trace_t tr;
	auto vec_dest = vec_src;
	vec_dest += v_forward * 22.0f;
	trace_hull(vec_src, vec_dest, tr);

	auto vec_back = v_forward; vec_back *= 6.0f;
	vec_src = tr.endpos;
	vec_src -= vec_back;

	vec_throw = LocalPlayer.Entity->GetVelocity();
	vec_throw *= 1.25f;
	vec_throw += v_forward * fl_vel;
}

void nade_prediction::simulate(QAngle viewangles)
{
	Vector vec_src, vec_throw;
	setup(vec_src, vec_throw, viewangles);

	const auto interval = Interfaces::Globals->interval_per_tick;
	const auto logstep = static_cast<int>(0.04f / interval);
	auto logtimer = 0;
	static auto b_colide = false;

	path.clear();
	for (unsigned int i = 0; i < 128; ++i)
	{
		if (!logtimer)
		{
			QAngle angles;
			adr_math::vector_angles(vec_src, angles);

			path.push_back(std::make_tuple(vec_src, b_colide, angles));
			b_colide = false;
		}

		const auto s = step(vec_src, vec_throw, i, interval);
		if (s & 1)
			break;

		if (s & 2 || logtimer >= logstep)
			logtimer = 0;
		else
			++logtimer;

		if (s & 2)
			b_colide = true;
	}

	path.push_back(std::make_tuple(vec_src, false, QAngle()));
}

int nade_prediction::step(Vector& vec_src, Vector& vec_throw, int tick, float interval)
{
	Vector move;
	add_gravity_move(move, vec_throw, interval, false);

	trace_t tr;
	push_entity(vec_src, move, tr);

	auto result = 0;
	if (check_detonate(vec_throw, tr, tick, interval))
		result |= 1;

	if (tr.fraction != 1.0f)
	{
		result |= 2;
		resolve_fly_collision_custom(tr, vec_throw, interval);
	}

	vec_src = tr.endpos;
	return result;
}

bool nade_prediction::check_detonate(const Vector& vecThrow, const trace_t& tr, int tick, float interval)
{
	b_fire = false;
	switch (s_type)
	{
	case WEAPON_SMOKEGRENADE:
	case WEAPON_DECOY:
		if (vecThrow.Length2D() < 0.1f)
		{
			const auto det_tick_mod = static_cast<int>(0.2f / interval);
			return !(tick % det_tick_mod);
		}
		return false;
	case WEAPON_MOLOTOV:
	case WEAPON_INCGRENADE:
		if (tr.fraction != 1.0f && tr.plane.normal.z > 0.7f)
			return true;
	case WEAPON_FLASHBANG:
	case WEAPON_HEGRENADE:
		b_fire = static_cast<float>(tick) * interval > 1.5f && !(tick % static_cast<int>(0.2f / interval));
		return b_fire;
	case WEAPON_SNOWBALL:
		return false;
	default:
		assert(false);
		return false;
	}
}

void nade_prediction::trace_hull(Vector& src, Vector& end, trace_t& tr)
{
	Ray_t ray;
	ray.Init(src, end, Vector(-2.0f, -2.0f, -2.0f), Vector(2.0f, 2.0f, 2.0f));

	CTraceFilterWorldAndPropsOnly filter;
	Interfaces::EngineTrace->TraceRay(ray, 0x200400B, &filter, &tr);
}

void nade_prediction::add_gravity_move(Vector& move, Vector& vel, float frametime, bool onground)
{
	const Vector basevel(0.0f, 0.0f, 0.0f);

	move.x = (vel.x + basevel.x) * frametime;
	move.y = (vel.y + basevel.y) * frametime;

	if (onground)
		move.z = (vel.z + basevel.z) * frametime;
	else
	{
		const auto gravity = 800.0f * 0.4f;

		const auto new_z = vel.z - (gravity * frametime);
		move.z = ((vel.z + new_z) / 2.0f + basevel.z) * frametime;

		vel.z = new_z;
	}
}

void nade_prediction::push_entity(Vector& src, const Vector& move, trace_t& tr)
{
	auto vec_abs_end = src;
	vec_abs_end += move;

	trace_hull(src, vec_abs_end, tr);
}

void nade_prediction::resolve_fly_collision_custom(trace_t& tr, Vector& vecVelocity, float interval)
{
	const auto fl_surface_elasticity = 1.f;
	const auto fl_grenade_elasticity = 0.45f;

	auto fl_total_elasticity = fl_grenade_elasticity * fl_surface_elasticity;

	if (fl_total_elasticity > 0.9f)
		fl_total_elasticity = 0.9f;

	if (fl_total_elasticity < 0.0f)
		fl_total_elasticity = 0.0f;

	Vector vec_abs_velocity;
	physics_clip_velocity(vecVelocity, tr.plane.normal, vec_abs_velocity, 2.0f);
	vec_abs_velocity *= fl_total_elasticity;

	const auto fl_speed_sqr = vec_abs_velocity.LengthSqr();
	static const auto fl_min_speed_sqr = 20.0f * 20.0f;
	if (fl_speed_sqr < fl_min_speed_sqr)
		vec_abs_velocity.Zero();

	if (tr.plane.normal.z > 0.7f)
	{
		vecVelocity = vec_abs_velocity;
		vec_abs_velocity *= ((1.0f - tr.fraction) * interval); //vecAbsVelocity.Mult((1.0f - tr.fraction) * interval);
		push_entity(tr.endpos, vec_abs_velocity, tr);
	}
	else
		vecVelocity = vec_abs_velocity;
}

int nade_prediction::physics_clip_velocity(const Vector& in, const Vector& normal, Vector& out, float overbounce)
{
	static const auto STOP_EPSILON = 0.1f;
	auto blocked = 0;

	const auto angle = normal[2];
	if (angle > 0)
		blocked |= 1;        // floor
	if (!angle)
		blocked |= 2;        // step

	const auto backoff = in.Dot(normal) * overbounce;
	for (auto i = 0; i < 3; i++)
	{
		const auto change = normal[i] * backoff;
		out[i] = in[i] - change;
		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
			out[i] = 0;
	}

	return blocked;
}