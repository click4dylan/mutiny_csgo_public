#pragma once

#include "c_font.h"
#include "cx_fnv1.h"
#include "Adriel\stdafx.hpp"
#include "Adriel\singleton.hpp"
#include <unordered_map>

class c_renderer : public singleton<c_renderer>
{
	struct vertex {
		float x, y, z, rhw;
		uint32_t color;
	};

public:
	c_renderer();
	~c_renderer();

	explicit c_renderer(IDirect3DDevice9* dev);

	void line(Vector2D from, Vector2D to, Color color) const;

	void rect(Vector2D from, Vector2D size, Color color) const;
	void rect_linear_gradient(Vector2D from, Vector2D size, Color color1, Color color2, bool horizontal) const;
	void rect_full_linear_gradient(Vector2D from, Vector2D size, Color color1, Color color2, Color color3, Color color4) const;
	void rect_filled(Vector2D from, Vector2D size, Color color) const;
	void rect_filled_linear_gradient(Vector2D from, Vector2D size, Color color1, Color color2, bool horizontal = false) const;
	void rect_filled_radial_gradient(Vector2D from, Vector2D size, Color color1, Color color2);
	void rect_filled_diamond_gradient(Vector2D from, Vector2D size, Color color1, Color color2) const;

	void parallelogram(Vector2D from, Vector2D size, Color color, uint8_t side = 0, float radius = 8.f) const;
	void parallelogram_filled_linear_gradient(Vector2D from, Vector2D size, Color color1, Color color2,
		uint8_t side = 0, bool horizontal = false, float radius = 8.f) const;

	void triangle(Vector2D pos1, Vector2D pos2, Vector2D pos3, Color color) const;
	void triangle_linear_gradient(Vector2D pos1, Vector2D pos2, Vector2D pos3, Color color1,
		Color color2, Color color3) const;
	void triangle_filled(Vector2D pos1, Vector2D pos2, Vector2D pos3, Color color) const;
	void triangle_filled_linear_gradient(Vector2D pos1, Vector2D pos2, Vector2D pos3, Color color1,
		Color color2, Color color3) const;

	void circle(Vector2D center, float radius, Color color);
	void circle_filled(Vector2D center, float radius, Color color);
	void circle_filled_radial_gradient(Vector2D center, float radius, Color color1, Color color2);

	void filled_box(const int x, const int y, const int w, const int h, const Color color) const;
	void filled_box_outlined(const int x, const int y, const int w, const int h, const Color color, const Color outline, const int thickness = 1) const;
	void bordered_box(const int x, const int y, const int w, const int h, const Color color, const int thickness = 1) const;
	void bordered_box_outlined(const int x, const int y, const int w, const int h, const Color color, const Color outline, const int thickness = 1) const;

	void text(Vector2D pos, const char* str, Color color, uint32_t font = default_font, uint8_t flags = 0);
	Vector2D get_text_size(const char* str, uint32_t font);


	void create_texture(void* src, uint32_t size, LPDIRECT3DTEXTURE9* texture) const;
	void create_sprite(LPD3DXSPRITE* sprite) const;
	static void image(Vector2D position, LPDIRECT3DTEXTURE9 texture, LPD3DXSPRITE sprite, float scl = 1.f, float alpha = 1.f);

	inline void scale(Vector2D& vec) const;
	Vector2D get_center() const;

	VMatrix& world_to_screen_matrix();

	inline bool screen_transform(const Vector& in, Vector2D& out, VMatrix& matrix) const;
	inline bool is_on_screen(const Vector& in, float width, VMatrix& matrix) const;

	void limit(rectangle rect) const;
	rectangle get_limit() const;
	void reset_limit() const;

	void refresh_viewport();

	void init_device_objects(IDirect3DDevice9* dev);
	void invalidate_device_objects();
	void setup_render_state() const;
	void register_reset_handler(std::function<void()> handler);

	D3DVIEWPORT9 port{};


private:

	static constexpr auto points = 64;
	static constexpr auto points_sphere_latitude = 16;
	static constexpr auto points_sphere_longitude = 24;
	static constexpr auto default_font = cx::fnv1a("pro13");

	IDirect3DDevice9* dev{};
	std::vector<Vector2D> lookup_table;
	std::vector<Vector> lookup_sphere;
	std::unordered_map<uint32_t, c_font> fonts;
	std::vector<std::function<void()>> reset_handlers;

	void build_lookup_table();
};

void c_renderer::scale(Vector2D& vec) const
{
	vec.x = (vec.x + 1.f) * port.Width / 2.f;
	vec.y = (-vec.y + 1.f) * port.Height / 2.f;
}

bool c_renderer::screen_transform(const Vector& in, Vector2D& out, VMatrix& matrix) const
{
	out.x = matrix[0][0] * in.x + matrix[0][1] * in.y + matrix[0][2] * in.z + matrix[0][3];
	out.y = matrix[1][0] * in.x + matrix[1][1] * in.y + matrix[1][2] * in.z + matrix[1][3];

	const auto w = matrix[3][0] * in.x + matrix[3][1] * in.y + matrix[3][2] * in.z + matrix[3][3];

	if (w < 0.001f)
	{
		out.x *= 100000;
		out.y *= 100000;
		return false;
	}

	const auto invw = 1.0f / w;
	out.x *= invw;
	out.y *= invw;

	scale(out);
	return true;
}

bool c_renderer::is_on_screen(const Vector& in, const float width, VMatrix& matrix) const
{
	auto out = Vector2D(matrix[0][0] * in.x + matrix[0][1] * in.y + matrix[0][2] * in.z + matrix[0][3],
		matrix[1][0] * in.x + matrix[1][1] * in.y + matrix[1][2] * in.z + matrix[1][3]);

	const auto w = matrix[3][0] * in.x + matrix[3][1] * in.y + matrix[3][2] * in.z + matrix[3][3];

	if (w < 0.001f)
	{
		out.x *= 100000;
		out.y *= 100000;
		return false;
	}

	const auto invw = 1.0f / w;
	out.x *= invw;
	out.y *= invw;

	scale(out);
	return !(out.x - width > port.Width || out.x + width < 1.f);
}
