#pragma once
#include "stdafx.hpp"

#include <tuple>

#include "../Interfaces.h"

enum ACT
{
    ACT_NONE,
    ACT_THROW,
    ACT_LOB,
    ACT_DROP,
};

class nade_prediction : public singleton<nade_prediction>
{
private:
	std::vector<std::tuple<Vector, bool, QAngle>> path;

	bool b_fire;
	short s_type;
	int i_act;

private:
	void setup(Vector& vec_src, Vector& vec_throw, QAngle viewangles) const;
	void simulate(QAngle viewangles);

	int step(Vector& vec_src, Vector& vec_throw, int tick, float interval);
	bool check_detonate(const Vector& vec_throw, const trace_t& tr, int tick, float interval);

	static void trace_hull(Vector& src, Vector& end, trace_t& tr);
	static void add_gravity_move(Vector& move, Vector& vel, float frametime, bool onground);
	static void push_entity(Vector& src, const Vector& move, trace_t& tr);
	static void resolve_fly_collision_custom(trace_t& tr, Vector& vec_velocity, float interval);
	static int physics_clip_velocity(const Vector& in, const Vector& normal, Vector& out, float overbounce);

public:
	nade_prediction();
	~nade_prediction();

    void create_move(CUserCmd* p_cmd);
    void override_view(CViewSetup* setup);
    void render();
};