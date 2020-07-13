#pragma once
#include "stdafx.hpp"

enum text_flags
{
	NO_TFLAG = 0,
	CENTERED_X = 2,
	CENTERED_Y = 4,
	OUTLINE = 8,
	DROP_SHADOW = 16,
	RIGHT_ALIGN = 32,
};
DEFINE_ENUM_FLAG_OPERATORS( text_flags )

/* All of them contain the csgo icons + FontAwesome + all symbols from ARIAL */
enum font_flags
{
	SMALLEST_PIXEL_10 = 0,
	SMALLEST_PIXEL_10_WITH_CSGO_16 = 2,
	TAHOMA_14 = 4,
	TAHOMA_18 = 8,
	BAKSHEESH_14 = 16,
	BAKSHEESH_18 = 32,
	BAKSHEESH_12 = 64,
	BAKSHEESH_8 = 128,
};
DEFINE_ENUM_FLAG_OPERATORS( font_flags )

class render : public singleton<render>
{
public:
	ImFont* smallest_pixel_10 {};
	ImFont* smallest_pixel_10_csgo_16 {};

	ImFont* tahoma_14 {};
	ImFont* tahoma_18 {};

	ImFont* bakshees_8{};
	ImFont* bakshees_12{};
	ImFont* bakshees_14 {};
	ImFont* bakshees_18 {};

private:
	ImDrawList* _draw_list_act {};
	ImDrawList* _draw_list_rendering {};
	ImDrawList* _draw_list {};
	ImDrawData	_draw_data {};

private:
	float				font_size = 12.f;
	std::mutex			render_mutex {};
	bool				b_hide = false;
	D3DVIEWPORT9		_viewport {};
	IDirect3DDevice9*   _device {};

public:
	render();
	~render();

	void shutdown();
	void initialize(IDirect3DDevice9* device);

	D3DVIEWPORT9 get_viewport() const;

	void create_objects();
	void invalidate_objects();
	void new_frame();
	void swap_data();
	void hide();

	ImDrawList* get_draw_data();

	void add_custom_box(int type, bool filled, RECT bbox, Vector points[], Color color, Color fill, Color shadow) const;
	void add_circle_3d(Vector position, float points, float radius, ImU32 color, float thickness = 1.0f) const;
	void add_cube(Vector origin, float scale, QAngle angle, ImU32 color, float thickness = 1.0f) const;

	void add_text(ImVec2 point, ImU32 color, text_flags flags, font_flags font_flag, const char* format, ...);
	void add_rect(const ImVec2& a, float w, float h, ImU32 col, float rounding = 0.0f, int rounding_corners_flags = ~0, float thickness = 1.0f) const;
	void add_line(const ImVec2& a, const ImVec2& b, ImU32 col, float thickness = 1.0f) const;
	void add_rect(const ImVec2& a, const ImVec2& b, ImU32 col, float rounding = 0.0f, int rounding_corners_flags = ~0, float thickness = 1.0f) const;
	void add_rect_filled(const ImVec2& a, float w, float h, ImU32 col, float rounding = 0.0f, int rounding_corners_flags = ~0) const;
	void add_rect_filled(const ImVec2& a, const ImVec2& b, ImU32 col, float rounding = 0.0f, int rounding_corners_flags = ~0) const;
	void add_rect_filled_multicolor(const ImVec2& a, const ImVec2& b, ImU32 col_upr_left, ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left) const;
	void add_quad(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d, ImU32 col, float thickness = 1.0f) const;
	void add_quad_filled(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d, ImU32 col) const;
	void add_triangle(const ImVec2& a, const ImVec2& b, const ImVec2& c, ImU32 col, float thickness = 1.0f) const;
	void add_triangle_filled(const ImVec2& a, const ImVec2& b, const ImVec2& c, ImU32 col) const;
	void add_circle(const ImVec2& centre, float radius, ImU32 col, int num_segments = 12, float thickness = 1.0f) const;
	void add_circle_filled(const ImVec2& centre, float radius, ImU32 col, int num_segments = 12) const;
	void add_polyline(const ImVec2* points, const int num_points, ImU32 col, bool closed, float thickness) const;
	void add_convex_poly_filled(const ImVec2* points, const int num_points, ImU32 col) const;
	void add_bezier_curve(const ImVec2& pos0, const ImVec2& cp0, const ImVec2& cp1, const ImVec2& pos1, ImU32 col, float thickness, int num_segments = 0) const;
};