#include "adr_math.hpp"
#include "../Interfaces.h"

void adr_math::sin_cos(float radians, float* sine, float* cosine)
{
	*sine = sin(radians);
	*cosine = cos(radians);
}

void adr_math::vector_angles(const Vector& forward, QAngle& angles)
{
	if (forward[1] == 0.0f && forward[0] == 0.0f)
	{
		angles[0] = (forward[2] > 0.0f) ? 270.0f : 90.0f;
		angles[1] = 0.0f;
	}
	else
	{
		angles[0] = atan2(-forward[2], forward.Length2D()) * -180 / M_PI;
		angles[1] = atan2(forward[1], forward[0]) * 180 / M_PI;

		if (angles[1] > 90) angles[1] -= 180;
		else if (angles[1] < 90) angles[1] += 180;
		else if (angles[1] == 90) angles[1] = 0;
	}

	angles[2] = 0.0f;
}

void adr_math::angle_matrix(const QAngle &angles, matrix3x4_t &matrix)
{
	float sr, sp, sy, cr, cp, cy;

	sin_cos(DEG2RAD(angles[1]), &sy, &cy);
	sin_cos(DEG2RAD(angles[0]), &sp, &cp);
	sin_cos(DEG2RAD(angles[2]), &sr, &cr);

	// matrix = (YAW * PITCH) * ROLL
	matrix[0][0] = cp * cy;
	matrix[1][0] = cp * sy;
	matrix[2][0] = -sp;

	const auto crcy = cr * cy;
	const auto crsy = cr * sy;
	const auto srcy = sr * cy;
	const auto srsy = sr * sy;
	matrix[0][1] = sp * srcy - crsy;
	matrix[1][1] = sp * srsy + crcy;
	matrix[2][1] = sr * cp;

	matrix[0][2] = (sp*crcy + srsy);
	matrix[1][2] = (sp*crsy - srcy);
	matrix[2][2] = cr * cp;

	matrix[0][3] = 0.0f;
	matrix[1][3] = 0.0f;
	matrix[2][3] = 0.0f;
}

void adr_math::matrix_set_column(const Vector &in, int column, matrix3x4_t &out)
{
	out[0][column] = in.x;
	out[1][column] = in.y;
	out[2][column] = in.z;
}

void adr_math::angle_matrix(const QAngle &angles, const Vector &position, matrix3x4_t &matrix)
{
	angle_matrix(angles, matrix);
	matrix_set_column(position, 3, matrix);
}

float __fastcall adr_math::angle_diff(float a1, float a2)
{
	auto val = fmodf(a1 - a2, 360.0);
	while (val < -180.0f) val += 360.0f;
	while (val >  180.0f) val -= 360.0f;

	return val;
}

void adr_math::vector_angles(const Vector& forward, Vector& up, QAngle& angles)
{
	auto left = up.Cross(forward);
	left.NormalizeInPlace();

	const auto forward_dist = forward.Length2D();
	if (forward_dist > 0.001f)
	{
		angles[0] = atan2f(-forward.z, forward_dist) * 180 / M_PI;
		angles[1] = atan2f(forward.y, forward.x) * 180 / M_PI;

		const auto up_z = (left.y * forward.x) - (left.x * forward.y);
		angles[2] = atan2f(left.z, up_z) * 180 / M_PI;
	}
	else
	{
		angles[0] = atan2f(-forward.z, forward_dist) * 180 / M_PI;
		angles[1] = atan2f(-left.x, left.y) * 180 / M_PI;
		angles[2] = 0;
	}
}

void adr_math::vector_transform(const Vector& in1, const matrix3x4_t& in2, Vector& out)
{
	out[0] = in1.Dot(in2[0]) + in2[0][3];
	out[1] = in1.Dot(in2[1]) + in2[1][3];
	out[2] = in1.Dot(in2[2]) + in2[2][3];
}

void adr_math::angle_vector(const QAngle& angles, Vector& forward)
{
	float sp, sy, cp, cy;

	sin_cos(DEG2RAD(angles[1]), &sy, &cy);
	sin_cos(DEG2RAD(angles[0]), &sp, &cp);

	forward.x = cp * cy;
	forward.y = cp * sy;
	forward.z = -sp;
}

void adr_math::angle_vector(const QAngle& angles, Vector& forward, Vector& right, Vector& up)
{
	float sr, sp, sy, cr, cp, cy;

	sin_cos(DEG2RAD(angles[1]), &sy, &cy);
	sin_cos(DEG2RAD(angles[0]), &sp, &cp);
	sin_cos(DEG2RAD(angles[2]), &sr, &cr);

	forward.x = (cp * cy);
	forward.y = (cp * sy);
	forward.z = (-sp);
	right.x = (-1 * sr * sp * cy + -1 * cr * -sy);
	right.y = (-1 * sr * sp * sy + -1 * cr * cy);
	right.z = (-1 * sr * cp);
	up.x = (cr * sp * cy + -sr * -sy);
	up.y = (cr * sp * sy + -sr * cy);
	up.z = (cr * cp);
}

void adr_math::correct_movement(CUserCmd* cmd, QAngle &wish_angle)
{
	Vector view_fwd, view_right, view_up, cmd_fwd, cmd_right, cmd_up;
	auto viewangles = cmd->viewangles;
	viewangles.Normalize();

	angle_vector(wish_angle, view_fwd, view_right, view_up);
	angle_vector(viewangles, cmd_fwd, cmd_right, cmd_up);

	const auto v8 = sqrtf((view_fwd.x * view_fwd.x) + (view_fwd.y * view_fwd.y));
	const auto v10 = sqrtf((view_right.x * view_right.x) + (view_right.y * view_right.y));
	const auto v12 = sqrtf(view_up.z * view_up.z);

	const Vector norm_view_fwd((1.f / v8) * view_fwd.x, (1.f / v8) * view_fwd.y, 0.f);
	const Vector norm_view_right((1.f / v10) * view_right.x, (1.f / v10) * view_right.y, 0.f);
	const Vector norm_view_up(0.f, 0.f, (1.f / v12) * view_up.z);

	const auto v14 = sqrtf((cmd_fwd.x * cmd_fwd.x) + (cmd_fwd.y * cmd_fwd.y));
	const auto v16 = sqrtf((cmd_right.x * cmd_right.x) + (cmd_right.y * cmd_right.y));
	const auto v18 = sqrtf(cmd_up.z * cmd_up.z);

	const Vector norm_cmd_fwd((1.f / v14) * cmd_fwd.x, (1.f / v14) * cmd_fwd.y, 0.f);
	const Vector norm_cmd_right((1.f / v16) * cmd_right.x, (1.f / v16) * cmd_right.y, 0.f);
	const Vector norm_cmd_up(0.f, 0.f, (1.f / v18) * cmd_up.z);

	const auto v22 = norm_view_fwd.x * cmd->forwardmove;
	const auto v26 = norm_view_fwd.y * cmd->forwardmove;
	const auto v28 = norm_view_fwd.z * cmd->forwardmove;
	const auto v24 = norm_view_right.x * cmd->sidemove;
	const auto v23 = norm_view_right.y * cmd->sidemove;
	const auto v25 = norm_view_right.z * cmd->sidemove;
	const auto v30 = norm_view_up.x * cmd->upmove;
	const auto v27 = norm_view_up.z * cmd->upmove;
	const auto v29 = norm_view_up.y * cmd->upmove;

	cmd->forwardmove = norm_cmd_fwd.x * v24 + norm_cmd_fwd.y * v23 + norm_cmd_fwd.z * v25
		+ (norm_cmd_fwd.x * v22 + norm_cmd_fwd.y * v26 + norm_cmd_fwd.z * v28)
		+ (norm_cmd_fwd.y * v30 + norm_cmd_fwd.x * v29 + norm_cmd_fwd.z * v27);
	cmd->sidemove = norm_cmd_right.x * v24 + norm_cmd_right.y * v23 + norm_cmd_right.z * v25
		+ (norm_cmd_right.x * v22 + norm_cmd_right.y * v26 + norm_cmd_right.z * v28)
		+ (norm_cmd_right.x * v29 + norm_cmd_right.y * v30 + norm_cmd_right.z * v27);
	cmd->upmove = norm_cmd_up.x * v23 + norm_cmd_up.y * v24 + norm_cmd_up.z * v25
		+ (norm_cmd_up.x * v26 + norm_cmd_up.y * v22 + norm_cmd_up.z * v28)
		+ (norm_cmd_up.x * v30 + norm_cmd_up.y * v29 + norm_cmd_up.z * v27);

	cmd->forwardmove = clamp(cmd->forwardmove, -450.f, 450.f);
	cmd->sidemove = clamp(cmd->sidemove, -450.f, 450.f);
	cmd->upmove = clamp(cmd->upmove, -320.f, 320.f);
}

QAngle adr_math::calc_angle(Vector src, Vector dst)
{
	QAngle angles;
	auto delta = src - dst;

	delta.NormalizeInPlace();
	vector_angles(delta, angles);

	return angles;
}

float adr_math::fov(const QAngle& view_angle, const QAngle& aim_angle)
{
	Vector ang, aim;

	angle_vector(view_angle, aim);
	angle_vector(aim_angle, ang);

	return RAD2DEG(acos(aim.Dot(ang) / aim.LengthSqr()));
}

float adr_math::real_fov(float distance, QAngle view_angle, QAngle aim_angle)
{
	auto delta = aim_angle - view_angle;
	normalize_angles(delta);

	return sin(DEG2RAD(delta.Length()) / 2.f) * 180.f;
}

bool adr_math::screen_transform(const Vector& v_origin, Vector& v_out)
{
	/*static std::uintptr_t p_view_matrix;
	if (!p_view_matrix)
	{
		p_view_matrix = (uintptr_t)memory::pattern_scan_ida(xorstr("client_panorama.dll"), xorstr("0F 10 05 ? ? ? ? 8D 85 ? ? ? ? B9"));
		p_view_matrix += 3;
		p_view_matrix = *reinterpret_cast<std::uintptr_t*>(p_view_matrix);
		p_view_matrix += 176;
	}

	const auto& w2_s_matrix = *reinterpret_cast<VMatrix*>(p_view_matrix);*/

	static auto& w2_s_matrix = Interfaces::EngineClient->WorldToScreenMatrix();
	v_out.x = w2_s_matrix.m[0][0] * v_origin.x + w2_s_matrix.m[0][1] * v_origin.y + w2_s_matrix.m[0][2] * v_origin.z + w2_s_matrix.m[0][3];
	v_out.y = w2_s_matrix.m[1][0] * v_origin.x + w2_s_matrix.m[1][1] * v_origin.y + w2_s_matrix.m[1][2] * v_origin.z + w2_s_matrix.m[1][3];
	v_out.z = 0.0f;

	const auto w = w2_s_matrix.m[3][0] * v_origin.x + w2_s_matrix.m[3][1] * v_origin.y + w2_s_matrix.m[3][2] * v_origin.z + w2_s_matrix.m[3][3];
	if (w < 0.001f)
		return true;

	v_out.x /= w;
	v_out.y /= w;

	return false;
}

void adr_math::rotate_movement(CUserCmd* cmd, float yaw)
{
	QAngle viewangles;
	Interfaces::EngineClient->GetViewAngles(viewangles);

	const auto rotation = DEG2RAD(viewangles.y - yaw);
	const auto cos_rot = cos(rotation);
	const auto sin_rot = sin(rotation);
	const auto new_forwardmove = cos_rot * cmd->forwardmove - sin_rot * cmd->sidemove;
	const auto new_sidemove = sin_rot * cmd->forwardmove + cos_rot * cmd->sidemove;

	cmd->forwardmove = new_forwardmove;
	cmd->sidemove = new_sidemove;
}

float adr_math::clamp_yaw(float yaw)
{
	if (yaw > 180)
		yaw -= (round(yaw / 360) * 360.f);
	else if (yaw < -180)
		yaw += (round(yaw / 360) * -360.f);

	return yaw;
}

float adr_math::clamp_pitch(float pitch)
{
	while (pitch > 89.f)
		pitch -= 180.f;
	while (pitch < -89.f)
		pitch += 180.f;

	return pitch;
}

float adr_math::get_yaw_delta(float yaw1, float yaw2)
{
	return fabs(clamp_yaw(yaw1 - yaw2));
}

float adr_math::get_lby_rotated_yaw(float lby, float yaw)
{
	const auto delta = clamp_yaw(yaw - lby);
	if (fabs(delta) < 25.f)
		return lby;

	if (delta > 0.f)
		return yaw + 25.f;

	return yaw;
}

bool adr_math::world_to_screen(const Vector& v_origin, Vector& v_out)
{
	if (!screen_transform(v_origin, v_out))
	{
		int i_screen_width, i_screen_height;
		Interfaces::EngineClient->GetScreenSize(i_screen_width, i_screen_height);

		v_out.x = (i_screen_width * 0.5f) + (v_out.x * i_screen_width) * 0.5f;
		v_out.y = (i_screen_height * 0.5f) - (v_out.y * i_screen_height) * 0.5f;

		return true;
	}
	return false;
}

void adr_math::normalize_angles(QAngle& angle)
{
	for (auto i = 0; i < 3; i++)
	{
		while (angle[i] < -180.0f) angle[i] += 360.0f;
		while (angle[i] >  180.0f) angle[i] -= 360.0f;
	}
}

void adr_math::normalize_vector(Vector& vec)
{
	for (auto i = 0; i < 3; i++)
	{
		while (vec[i] < -180.0f) vec[i] += 360.0f;
		while (vec[i] >  180.0f) vec[i] -= 360.0f;
	}
	vec[2] = 0.f;
}

void adr_math::clamp_angles(QAngle& angle)
{
	angle.x = my_super_clamp(angle.x, -89.0f, 89.0f);
	if (angle.x < -180.0f || angle.x > 180.0f)
	{
		const auto fl_revolutions = std::round(std::abs(angle.x / 360.0f));
		if (angle.x < 0.0f)
			angle.x = (angle.x + (360.0f * fl_revolutions));
		else
			angle.x = (angle.x - (360.0f * fl_revolutions));
	}
	angle.z = my_super_clamp(angle.z, -50.0f, 50.0f);
}

float adr_math::distance_to_ray(const Vector& pos, const Vector& ray_start, const Vector& ray_end, float* along, Vector* point_on_ray)
{
	/*const auto to = pos - ray_start;
	auto dir = ray_end - ray_start;
	const auto length = dir.Normalize();
	const auto range_along = dir.Dot(to);

	if (along)
		*along = range_along;

	float range;
	if (range_along < 0.0f)
	{
		range = -(pos - ray_start).Length();
		if (point_on_ray)
			*point_on_ray = ray_start;
	}
	else if (range_along > length)
	{
		range = -(pos - ray_end).Length();
		if (point_on_ray)
			*point_on_ray = ray_end;
	}
	else
	{
		const auto on_ray = ray_start + range_along * dir;
		range = (pos - on_ray).Length();
		if (point_on_ray)
			*point_on_ray = on_ray;
	}
	return range;*/

	return 0.f;
}

float adr_math::angle_distance(float firstangle, float secondangle)
{
	if (firstangle == secondangle)
		return 0.f;

	auto opposite_sides = false;
	if (firstangle > 0 && secondangle < 0)
		opposite_sides = true;
	else if (firstangle < 0 && secondangle > 0)
		opposite_sides = true;

	if (!opposite_sides)
		return fabs(firstangle - secondangle);
	
	if (firstangle > 90 && secondangle < -90)
	{
		firstangle -= (firstangle - 90);
		secondangle += (secondangle + 90);
	}
	else if (firstangle < -90 && secondangle > 90)
	{
		firstangle += (firstangle + 90);
		secondangle -= (secondangle - 90);
	}

	const auto one_two = fabs(firstangle - secondangle);
	return one_two;
}

float adr_math::distance_to_line(const Vector& point, const Vector& line_origin, const Vector& dir)
{
    auto point_dir = point - line_origin;
	const auto temp = point_dir.Dot(dir) / dir.LengthSqr();
   
	if (temp < 0.000001f)
        return FLT_MAX;
	
	const auto perp_point = line_origin + dir * temp;	
    return (point - perp_point).Length();
};