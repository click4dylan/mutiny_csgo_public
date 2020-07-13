#include "renderer.hpp"

#include "ImGui/imgui_internal.h"
#include "ImGui/font_compressed.h"

#include "imgui_extra.hpp"
//#include "ui.hpp"
#include "adr_util.hpp"
#include "adr_math.hpp"
#include "ui.hpp"

render::render() = default;
render::~render() = default;

void render::shutdown()
{
	invalidate_objects();
}

void render::initialize(IDirect3DDevice9* device)
{
	if (b_hide)
		return;

	_device = device;
	_draw_list = nullptr;
	_draw_list_act = nullptr;
	_draw_list_rendering = nullptr;

	create_objects();
}

D3DVIEWPORT9 render::get_viewport() const
{
	return _viewport;
}

void render::create_objects()
{
	if (b_hide)
		return;

	auto _data = ImGui::GetDrawListSharedData();
	_device->GetViewport(&_viewport);

	_draw_list = new ImDrawList(_data);
	_draw_list_act = new ImDrawList(_data);
	_draw_list_rendering = new ImDrawList(_data);

	const auto f_scale = ui::get().get_scale();
	static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
	static const ImWchar weapons_ranges[] = { 0xE000, 0xE20C, 0 };

	ImFontConfig cfg;
	ImFontConfig symbol_cfg;
	symbol_cfg.MergeMode = true;

	ImFontConfig icons_cfg;
	icons_cfg.MergeMode = true;
	icons_cfg.PixelSnapH = true;

	ImFontConfig csgo_cfg;
	csgo_cfg.MergeMode = true;
	csgo_cfg.PixelSnapH = true;

	smallest_pixel_10 = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(smallest_pixel_compressed_data_base85, (font_size - 2) /** f_scale*/, &cfg, ImGui::GetIO().Fonts->GetOnlyGlyphBasicRanges());
	smallest_pixel_10 = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(ImGuiEx::CopyFont(font_awesome_data, font_awesome_size), font_awesome_size, font_size /** f_scale*/, &icons_cfg, icons_ranges);
	smallest_pixel_10 = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(csgo_icons_compressed_data_base85, font_size /** f_scale*/, &csgo_cfg, weapons_ranges);
	smallest_pixel_10 = ImGui::GetIO().Fonts->AddFontFromFileTTF(std::string(adr_util::get_disk() + "Windows\\Fonts\\l_10646.ttf").c_str(), (font_size + 1)/** f_scale*/, &symbol_cfg, ImGui::GetIO().Fonts->GetOnlyGlyphRangesCyrillic());

	smallest_pixel_10_csgo_16 = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(smallest_pixel_compressed_data_base85, (font_size - 2) /** f_scale*/, &cfg, ImGui::GetIO().Fonts->GetOnlyGlyphBasicRanges());
	smallest_pixel_10_csgo_16 = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(ImGuiEx::CopyFont(font_awesome_data, font_awesome_size), font_awesome_size, (font_size) /** f_scale*/, &icons_cfg, icons_ranges);
	smallest_pixel_10_csgo_16 = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(csgo_icons_compressed_data_base85, (font_size + 4) /** f_scale*/, &csgo_cfg, weapons_ranges);
	smallest_pixel_10_csgo_16 = ImGui::GetIO().Fonts->AddFontFromFileTTF(std::string(adr_util::get_disk() + "Windows\\Fonts\\l_10646.ttf").c_str(), (font_size + 1)/** f_scale*/, &symbol_cfg, ImGui::GetIO().Fonts->GetOnlyGlyphRangesCyrillic());

	tahoma_14 = ImGui::GetIO().Fonts->AddFontFromFileTTF(std::string(adr_util::get_disk() + "Windows\\Fonts\\tahomabd.ttf").c_str(), (font_size + 2) /** f_scale*/, &cfg, ImGui::GetIO().Fonts->GetOnlyGlyphBasicRanges());
	tahoma_14 = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(ImGuiEx::CopyFont(font_awesome_data, font_awesome_size), font_awesome_size, font_size /** f_scale*/, &icons_cfg, icons_ranges);
	tahoma_14 = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(csgo_icons_compressed_data_base85, font_size /** f_scale*/, &csgo_cfg, weapons_ranges);
	tahoma_14 = ImGui::GetIO().Fonts->AddFontFromFileTTF(std::string(adr_util::get_disk() + "Windows\\Fonts\\l_10646.ttf").c_str(), (font_size + 1)/** f_scale*/, &symbol_cfg, ImGui::GetIO().Fonts->GetOnlyGlyphRangesCyrillic());

	tahoma_18 = ImGui::GetIO().Fonts->AddFontFromFileTTF(std::string(adr_util::get_disk() + "Windows\\Fonts\\tahomabd.ttf").c_str(), (font_size + 6) /** f_scale*/, &cfg, ImGui::GetIO().Fonts->GetOnlyGlyphBasicRanges());
	tahoma_18 = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(ImGuiEx::CopyFont(font_awesome_data, font_awesome_size), font_awesome_size, (font_size + 6) /** f_scale*/, &icons_cfg, icons_ranges);
	tahoma_18 = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(csgo_icons_compressed_data_base85, (font_size + 6) /** f_scale*/, &csgo_cfg, weapons_ranges);
	tahoma_18 = ImGui::GetIO().Fonts->AddFontFromFileTTF(std::string(adr_util::get_disk() + "Windows\\Fonts\\l_10646.ttf").c_str(), (font_size + 5) /** f_scale*/, &symbol_cfg, ImGui::GetIO().Fonts->GetOnlyGlyphRangesCyrillic());

	bakshees_14 = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(baksheesh_bold_compressed_data_base85, (font_size + 2) /** f_scale*/, &cfg, ImGui::GetIO().Fonts->GetOnlyGlyphBasicRanges());
	bakshees_14 = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(ImGuiEx::CopyFont(font_awesome_data, font_awesome_size), font_awesome_size, font_size + 2 /** f_scale*/, &icons_cfg, icons_ranges);
	bakshees_14 = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(csgo_icons_compressed_data_base85, font_size + 2 /** f_scale*/, &csgo_cfg, weapons_ranges);
	bakshees_14 = ImGui::GetIO().Fonts->AddFontFromFileTTF(std::string(adr_util::get_disk() + "Windows\\Fonts\\l_10646.ttf").c_str(), (font_size + 1) /** f_scale*/, &symbol_cfg, ImGui::GetIO().Fonts->GetOnlyGlyphRangesCyrillic());

	bakshees_18 = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(baksheesh_bold_compressed_data_base85, (font_size + 6) /** f_scale*/, &cfg, ImGui::GetIO().Fonts->GetOnlyGlyphBasicRanges());
	bakshees_18 = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(ImGuiEx::CopyFont(font_awesome_data, font_awesome_size), font_awesome_size, (font_size + 6) /** f_scale*/, &icons_cfg, icons_ranges);
	bakshees_18 = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(csgo_icons_compressed_data_base85, (font_size + 6) /** f_scale*/, &csgo_cfg, weapons_ranges);
	bakshees_18 = ImGui::GetIO().Fonts->AddFontFromFileTTF(std::string(adr_util::get_disk() + "Windows\\Fonts\\l_10646.ttf").c_str(), (font_size + 5) /** f_scale*/, &symbol_cfg, ImGui::GetIO().Fonts->GetOnlyGlyphRangesCyrillic());

	bakshees_12 = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(baksheesh_bold_compressed_data_base85, (font_size) /** f_scale*/, &cfg, ImGui::GetIO().Fonts->GetOnlyGlyphBasicRanges());
	bakshees_12 = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(ImGuiEx::CopyFont(font_awesome_data, font_awesome_size), font_awesome_size, (font_size) /** f_scale*/, &icons_cfg, icons_ranges);
	bakshees_12 = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(csgo_icons_compressed_data_base85, (font_size) /** f_scale*/, &csgo_cfg, weapons_ranges);
	bakshees_12 = ImGui::GetIO().Fonts->AddFontFromFileTTF(std::string(adr_util::get_disk() + "Windows\\Fonts\\l_10646.ttf").c_str(), (font_size + 1) /** f_scale*/, &symbol_cfg, ImGui::GetIO().Fonts->GetOnlyGlyphRangesCyrillic());

	bakshees_8 = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(baksheesh_bold_compressed_data_base85, (font_size - 4.f) /** f_scale*/, &cfg, ImGui::GetIO().Fonts->GetOnlyGlyphBasicRanges());
	bakshees_8 = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(ImGuiEx::CopyFont(font_awesome_data, font_awesome_size), font_awesome_size, (font_size - 4.f) /** f_scale*/, &icons_cfg, icons_ranges);
	bakshees_8 = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(csgo_icons_compressed_data_base85, (font_size - 4.f) /** f_scale*/, &csgo_cfg, weapons_ranges);
	bakshees_8 = ImGui::GetIO().Fonts->AddFontFromFileTTF(std::string(adr_util::get_disk() + "Windows\\Fonts\\l_10646.ttf").c_str(), (font_size + 1) /** f_scale*/, &symbol_cfg, ImGui::GetIO().Fonts->GetOnlyGlyphRangesCyrillic());
}

void render::invalidate_objects()
{
	_draw_list->Clear();
	if (_draw_list)
		delete _draw_list;

	_draw_list = nullptr;

	//render_mutex.lock();

	_draw_list_rendering->Clear();
	if (_draw_list_rendering)
		delete _draw_list_rendering;

	_draw_list_rendering = nullptr;

	_draw_list_act->Clear();
	if (_draw_list_act)
		delete _draw_list_act;

	_draw_list_act = nullptr;

	//render_mutex.unlock();
}

void render::new_frame()
{
	if (b_hide)
		return;

	_draw_list->Clear();
	_draw_list->PushClipRectFullScreen();
}

void render::swap_data()
{
	if (b_hide)
		return;

	render_mutex.lock();
	*_draw_list_act = *_draw_list;
	render_mutex.unlock();
}

void render::hide()
{
	b_hide = !b_hide;
}

ImDrawList* render::get_draw_data()
{
	if (b_hide)
		return nullptr;

	if (render_mutex.try_lock())
	{
		*_draw_list_rendering = *_draw_list_act;
		render_mutex.unlock();
	}

	return _draw_list_rendering;
}

void render::add_text(ImVec2 point, ImU32 color, text_flags flags, font_flags font_flag, const char* format, ...)
{
	if (b_hide)
		return;

	// default in case someone doesn't choose and set the auto var, no need to declare this bullshit.
	auto font = smallest_pixel_10;

	if (font_flag & SMALLEST_PIXEL_10_WITH_CSGO_16)
		font = smallest_pixel_10_csgo_16;
	else if (font_flag & TAHOMA_14)
		font = tahoma_14;
	else if (font_flag & TAHOMA_18)
		font = tahoma_18;
	else if (font_flag & BAKSHEESH_14)
		font = bakshees_14;
	else if (font_flag & BAKSHEESH_18)
		font = bakshees_18;
	else if (font_flag & BAKSHEESH_12)
		font = bakshees_12;
	else if (font_flag & BAKSHEESH_8)
		font = bakshees_8;

	// hey imgui is cancer when multi threaded like this, so we should check as they don't have security checks for that.
	if (!font || !font->IsLoaded())
		return;

	_draw_list->PushTextureID(font->ContainerAtlas->TexID);

	va_list va_alist;
	char buffer[2048];
	va_start(va_alist, format);
	_vsnprintf_s(buffer, sizeof(buffer), format, va_alist);
	va_end(va_alist);

	const auto text_size = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, buffer);

	if (flags & RIGHT_ALIGN)
	{
		point.x -= text_size.x;
	}

	if (flags & CENTERED_X || flags & CENTERED_Y)
	{
		if (flags & CENTERED_X)
			point.x -= text_size.x / 2;
		if (flags & CENTERED_Y)
			point.y -= text_size.y / 2;
	}

	if (flags & DROP_SHADOW)
	{
		const auto vec_4 = ImGui::ColorConvertU32ToFloat4(color);
		const auto u32 = ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, vec_4.w));
		_draw_list->AddText(font, font->FontSize, ImVec2{ point.x + 1, point.y + 1 }, u32, buffer);
	}
	else if (flags & OUTLINE)
	{
		const auto vec_4 = ImGui::ColorConvertU32ToFloat4(color);
		const auto u32 = ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, vec_4.w));
		_draw_list->AddText(font, font->FontSize, ImVec2{ point.x - 1, point.y - 1 }, u32, buffer);
		_draw_list->AddText(font, font->FontSize, ImVec2{ point.x + 1, point.y }, u32, buffer);
		_draw_list->AddText(font, font->FontSize, ImVec2{ point.x    , point.y + 1 }, u32, buffer);
		_draw_list->AddText(font, font->FontSize, ImVec2{ point.x - 1, point.y }, u32, buffer);
	}

	_draw_list->AddText(font, font->FontSize, point, color, buffer);
	_draw_list->PopTextureID();
}

void render::add_rect(const ImVec2& a, float w, float h, ImU32 col, float rounding /*= 0.0f*/, int rounding_corners_flags /*= ~0*/, float thickness /*= 1.0f*/) const
{
	if (b_hide)
		return;

	_draw_list->AddRect(a, { a.x + w, a.y + h }, col, rounding, rounding_corners_flags, thickness);
}

void render::add_rect_filled(const ImVec2& a, float w, float h, ImU32 col, float rounding /*= 0.0f*/, int rounding_corners_flags /*= ~0*/) const
{
	if (b_hide)
		return;

	_draw_list->AddRectFilled(a, { a.x + w, a.y + h }, col, rounding, rounding_corners_flags);
}

void render::add_line(const ImVec2& a, const ImVec2& b, ImU32 col, float thickness /*= 1.0f*/) const
{
	if (b_hide)
		return;

	_draw_list->AddLine(a, b, col, thickness);
}

void render::add_rect(const ImVec2& a, const ImVec2& b, ImU32 col, float rounding /*= 0.0f*/, int rounding_corners_flags /*= ~0*/, float thickness /*= 1.0f*/) const
{
	if (b_hide)
		return;

	_draw_list->AddRect(a, b, col, rounding, rounding_corners_flags, thickness);
}

void render::add_rect_filled(const ImVec2& a, const ImVec2& b, ImU32 col, float rounding /*= 0.0f*/, int rounding_corners_flags /*= ~0*/) const
{
	if (b_hide)
		return;

	_draw_list->AddRectFilled(a, b, col, rounding, rounding_corners_flags);
}

void render::add_rect_filled_multicolor(const ImVec2& a, const ImVec2& b, ImU32 col_upr_left, ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left) const
{
	if (b_hide)
		return;

	_draw_list->AddRectFilledMultiColor(a, b, col_upr_left, col_upr_right, col_bot_right, col_bot_left);
}

void render::add_quad(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d, ImU32 col, float thickness /*= 1.0f*/) const
{
	if (b_hide)
		return;

	_draw_list->AddQuad(a, b, c, d, col, thickness);
}

void render::add_quad_filled(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d, ImU32 col) const
{
	if (b_hide)
		return;

	_draw_list->AddQuadFilled(a, b, c, d, col);
}

void render::add_triangle(const ImVec2& a, const ImVec2& b, const ImVec2& c, ImU32 col, float thickness /*= 1.0f*/) const
{
	_draw_list->AddTriangle(a, b, c, col, thickness);
}

void render::add_triangle_filled(const ImVec2& a, const ImVec2& b, const ImVec2& c, ImU32 col) const
{
	if (b_hide)
		return;

	_draw_list->AddTriangleFilled(a, b, c, col);
}

void render::add_circle(const ImVec2& centre, float radius, ImU32 col, int num_segments /*= 12*/, float thickness /*= 1.0f*/) const
{
	if (b_hide)
		return;

	_draw_list->AddCircle(centre, radius, col, num_segments, thickness);
}

void render::add_circle_filled(const ImVec2& centre, float radius, ImU32 col, int num_segments /*= 12*/) const
{
	if (b_hide)
		return;

	_draw_list->AddCircleFilled(centre, radius, col, num_segments);
}

void render::add_polyline(const ImVec2* points, const int num_points, ImU32 col, bool closed, float thickness) const
{
	if (b_hide)
		return;

	_draw_list->AddPolyline(points, num_points, col, closed, thickness);
}

void render::add_convex_poly_filled(const ImVec2* points, const int num_points, ImU32 col) const
{
	if (b_hide)
		return;

	_draw_list->AddConvexPolyFilled(points, num_points, col);
}

void render::add_bezier_curve(const ImVec2& pos0, const ImVec2& cp0, const ImVec2& cp1, const ImVec2& pos1, ImU32 col, float thickness, int num_segments /*= 0*/) const
{
	if (b_hide)
		return;

	_draw_list->AddBezierCurve(pos0, cp0, cp1, pos1, col, thickness, num_segments);
}

void render::add_custom_box(int type, bool filled, RECT bbox, Vector points[], Color color, Color fill, Color shadow) const
{
	if (b_hide)
		return;

	const auto length_horizontal = (bbox.right - bbox.left) * 0.2f;
	const auto length_vertical = (bbox.bottom - bbox.top) * 0.2f;
	switch (type)
	{
	default:
		break;
	case 1:
		if (filled)
			add_rect_filled(ImVec2(bbox.left, bbox.top), ImVec2(bbox.right, bbox.bottom), fill.ToImGUI());

		add_rect(ImVec2(bbox.left, bbox.top), ImVec2(bbox.right, bbox.bottom), color.ToImGUI());
		add_rect(ImVec2(bbox.left + 1.f, bbox.top + 1.f), ImVec2(bbox.right - 1.f, bbox.bottom - 1.f), shadow.ToImGUI());
		break;
	case 2:
		if (filled)
			add_rect_filled(ImVec2(bbox.left, bbox.top), ImVec2(bbox.right, bbox.bottom), fill.ToImGUI());

		add_line(ImVec2(bbox.left + 1.f, bbox.top + 1.f), ImVec2(bbox.left + length_horizontal - 1.f, bbox.top + 1.f), shadow.ToImGUI());
		add_line(ImVec2(bbox.right - length_horizontal, bbox.top + 1.f), ImVec2(bbox.right - 1.f, bbox.top + 1.f), shadow.ToImGUI());
		add_line(ImVec2(bbox.left + 1.f, bbox.bottom - 2.f), ImVec2(bbox.left + length_horizontal - 1.f, bbox.bottom - 2.f), shadow.ToImGUI());
		add_line(ImVec2(bbox.right - length_horizontal, bbox.bottom - 2.f), ImVec2(bbox.right - 1.f, bbox.bottom - 2.f), shadow.ToImGUI());
		add_line(ImVec2(bbox.left + 1.f, bbox.top), ImVec2(bbox.left + 1.f, bbox.top + length_vertical - 1.f), shadow.ToImGUI());
		add_line(ImVec2(bbox.right - 2.f, bbox.top), ImVec2(bbox.right - 2.f, bbox.top + length_vertical - 1.f), shadow.ToImGUI());
		add_line(ImVec2(bbox.left + 1.f, bbox.bottom - length_vertical), ImVec2(bbox.left + 1.f, bbox.bottom - 1.f), shadow.ToImGUI());
		add_line(ImVec2(bbox.right - 2.f, bbox.bottom - length_vertical), ImVec2(bbox.right - 2.f, bbox.bottom - 1.f), shadow.ToImGUI());
		add_line(ImVec2(bbox.left, bbox.top), ImVec2(bbox.left + length_horizontal - 1.f, bbox.top), color.ToImGUI());
		add_line(ImVec2(bbox.right - length_horizontal, bbox.top), ImVec2(bbox.right - 1.f, bbox.top), color.ToImGUI());
		add_line(ImVec2(bbox.left, bbox.bottom - 1.f), ImVec2(bbox.left + length_horizontal - 1.f, bbox.bottom - 1.f), color.ToImGUI());
		add_line(ImVec2(bbox.right - length_horizontal, bbox.bottom - 1.f), ImVec2(bbox.right - 1.f, bbox.bottom - 1.f), color.ToImGUI());
		add_line(ImVec2(bbox.left, bbox.top), ImVec2(bbox.left, bbox.top + length_vertical - 1.f), color.ToImGUI());
		add_line(ImVec2(bbox.right - 1.f, bbox.top), ImVec2(bbox.right - 1.f, bbox.top + length_vertical - 1.f), color.ToImGUI());
		add_line(ImVec2(bbox.left, bbox.bottom - length_vertical), ImVec2(bbox.left, bbox.bottom - 1.f), color.ToImGUI());
		add_line(ImVec2(bbox.right - 1.f, bbox.bottom - length_vertical), ImVec2(bbox.right - 1.f, bbox.bottom), color.ToImGUI());
		break;
	case 3:
		add_line(ImVec2(points[0].x, points[0].y), ImVec2(points[1].x, points[1].y), color.ToImGUI());
		add_line(ImVec2(points[0].x, points[0].y), ImVec2(points[6].x, points[6].y), color.ToImGUI());
		add_line(ImVec2(points[1].x, points[1].y), ImVec2(points[5].x, points[5].y), color.ToImGUI());
		add_line(ImVec2(points[6].x, points[6].y), ImVec2(points[5].x, points[5].y), color.ToImGUI());
		add_line(ImVec2(points[2].x, points[2].y), ImVec2(points[1].x, points[1].y), color.ToImGUI());
		add_line(ImVec2(points[4].x, points[4].y), ImVec2(points[5].x, points[5].y), color.ToImGUI());
		add_line(ImVec2(points[6].x, points[6].y), ImVec2(points[7].x, points[7].y), color.ToImGUI());
		add_line(ImVec2(points[3].x, points[3].y), ImVec2(points[0].x, points[0].y), color.ToImGUI());
		add_line(ImVec2(points[3].x, points[3].y), ImVec2(points[2].x, points[2].y), color.ToImGUI());
		add_line(ImVec2(points[2].x, points[2].y), ImVec2(points[4].x, points[4].y), color.ToImGUI());
		add_line(ImVec2(points[7].x, points[7].y), ImVec2(points[4].x, points[4].y), color.ToImGUI());
		add_line(ImVec2(points[7].x, points[7].y), ImVec2(points[3].x, points[3].y), color.ToImGUI());
		break;
	}
}

void render::add_circle_3d(Vector position, float points, float radius, ImU32 color, float thickness) const
{
	if (b_hide)
		return;

	const auto step = static_cast<float>(M_PI) * 2.0f / points;
	for (float a = 0; a < (M_PI * 2.0f); a += step)
	{
		const Vector start(radius * cosf(a) + position.x, radius * sinf(a) + position.y, position.z);
		const Vector end(radius * cosf(a + step) + position.x, radius * sinf(a + step) + position.y, position.z);

		Vector start2d, end2d;
		if (!adr_math::world_to_screen(start, start2d) || !adr_math::world_to_screen(end, end2d))
			return;

		add_line(ImVec2(start2d.x, start2d.y), ImVec2(end2d.x, end2d.y), color, thickness);
	}
}

void render::add_cube(Vector origin, float scale, QAngle angle, ImU32 color, float thickness) const
{
	if (b_hide)
		return;

	Vector forward, right, up;
	adr_math::angle_vector(angle, forward, right, up);

	Vector points[8];
	points[0] = origin - (right * scale) + (up * scale) - (forward * scale); // BLT
	points[1] = origin + (right * scale) + (up * scale) - (forward * scale); // BRT
	points[2] = origin - (right * scale) - (up * scale) - (forward * scale); // BLB
	points[3] = origin + (right * scale) - (up * scale) - (forward * scale); // BRB
	points[4] = origin - (right * scale) + (up * scale) + (forward * scale); // FLT
	points[5] = origin + (right * scale) + (up * scale) + (forward * scale); // FRT
	points[6] = origin - (right * scale) - (up * scale) + (forward * scale); // FLB
	points[7] = origin + (right * scale) - (up * scale) + (forward * scale); // FRB

	Vector points_screen[8];
	for (auto i = 0; i < 8; i++)
		if (!adr_math::world_to_screen(points[i], points_screen[i]))
			return;

	// Back frame
	add_line(ImVec2(points_screen[0].x, points_screen[0].y), ImVec2(points_screen[1].x, points_screen[1].y), color, thickness);
	add_line(ImVec2(points_screen[0].x, points_screen[0].y), ImVec2(points_screen[2].x, points_screen[2].y), color, thickness);
	add_line(ImVec2(points_screen[3].x, points_screen[3].y), ImVec2(points_screen[1].x, points_screen[1].y), color, thickness);
	add_line(ImVec2(points_screen[3].x, points_screen[3].y), ImVec2(points_screen[2].x, points_screen[2].y), color, thickness);

	// Frame connector
	add_line(ImVec2(points_screen[0].x, points_screen[0].y), ImVec2(points_screen[4].x, points_screen[4].y), color, thickness);
	add_line(ImVec2(points_screen[1].x, points_screen[1].y), ImVec2(points_screen[5].x, points_screen[5].y), color, thickness);
	add_line(ImVec2(points_screen[2].x, points_screen[2].y), ImVec2(points_screen[6].x, points_screen[6].y), color, thickness);
	add_line(ImVec2(points_screen[3].x, points_screen[3].y), ImVec2(points_screen[7].x, points_screen[7].y), color, thickness);

	// Front frame
	add_line(ImVec2(points_screen[4].x, points_screen[4].y), ImVec2(points_screen[5].x, points_screen[5].y), color, thickness);
	add_line(ImVec2(points_screen[4].x, points_screen[4].y), ImVec2(points_screen[6].x, points_screen[6].y), color, thickness);
	add_line(ImVec2(points_screen[7].x, points_screen[7].y), ImVec2(points_screen[5].x, points_screen[5].y), color, thickness);
	add_line(ImVec2(points_screen[7].x, points_screen[7].y), ImVec2(points_screen[6].x, points_screen[6].y), color, thickness);
}