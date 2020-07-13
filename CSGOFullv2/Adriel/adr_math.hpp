#pragma once
#include "stdafx.hpp"

namespace adr_math
{
	float distance_to_line(const Vector& point, const Vector& line_origin, const Vector& dir);
	void sin_cos(float radians, float* sine, float* cosine);
	void vector_angles(const Vector& forward, QAngle& angles);
	void angle_matrix(const QAngle& angles, matrix3x4_t& matrix);
	void matrix_set_column(const Vector& in, int column, matrix3x4_t& out);
	void angle_matrix(const QAngle& angles, const Vector& position, matrix3x4_t& matrix);
	void angle_vector(const QAngle& angles, Vector& forward);
	void angle_vector(const QAngle& angles, Vector& forward, Vector& right, Vector& up);
	void correct_movement(CUserCmd* cmd, QAngle& wish_angle);

	void vector_angles(const Vector& forward, Vector& up, QAngle& angles);
	void vector_transform(const Vector& in1, const matrix3x4_t& in2, Vector& out);
	void normalize_angles(QAngle& angle);
	void normalize_vector(Vector& vec);
	void clamp_angles(QAngle& angles);

	float __fastcall angle_diff(float a1, float a2);
	float distance_to_ray(const Vector& pos, const Vector& ray_start, const Vector& ray_end, float* along = nullptr, Vector* point_on_ray = nullptr);
	float angle_distance(float firstangle, float secondangle);
	QAngle calc_angle(Vector src, Vector dst);
	float fov(const QAngle& view_angle, const QAngle& aim_angle);
	bool world_to_screen(const Vector& v_origin, Vector& v_out);
	float real_fov(float distance, QAngle view_angle, QAngle aim_angle);
	bool screen_transform(const Vector& v_origin, Vector& v_out);
	float clamp_yaw(float yaw);
	float clamp_pitch(float pitch);
	float get_yaw_delta(float yaw1, float yaw2);
	float get_lby_rotated_yaw(float lby, float yaw);
	void rotate_movement(CUserCmd* cmd, float yaw);

	template <typename T>
	T my_super_clamp(const T& n, const T& lower, const T& upper)
	{
		return max(lower, min(n, upper));
	}
}