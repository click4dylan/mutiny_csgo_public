#include "c_renderer.h"
#include <utility>
#include "ProFont.h"
#include "cx_fnv1.h"

c_renderer::c_renderer() = default;
c_renderer::~c_renderer() = default;

c_renderer::c_renderer(IDirect3DDevice9* dev)
{
	DWORD fnt;
	AddFontMemResourceEx(reinterpret_cast<void*>(profont), sizeof(profont), nullptr, &fnt);

	fonts[cx::fnv1a("pro12")] = c_font("Tahoma", 12, FW_REGULAR);
	fonts[cx::fnv1a("pro13")] = c_font("Tahoma", 13, FW_REGULAR);
	fonts[cx::fnv1a("pro17")] = c_font("Tahoma", 17, FW_REGULAR);

	init_device_objects(dev);
}

void c_renderer::line(const Vector2D from, const Vector2D to, const Color color) const
{
	const auto col = color.direct();

	vertex vert[2] =
	{
		{ from.x, from.y, 0.0f, 1.0f, col },
		{ to.x, to.y, 0.0f, 1.0f, col }
	};

	dev->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
	dev->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);

	dev->SetTexture(0, nullptr);
	dev->DrawPrimitiveUP(D3DPT_LINELIST, 1, &vert, sizeof(vertex));

	dev->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
	dev->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
}

void c_renderer::rect(const Vector2D from, const Vector2D size, const Color color) const
{
	const auto to = from + size;
	const auto col = color.direct();

	vertex vert[5] =
	{
		{ from.x, from.y, 0.0f, 1.0f, col },
		{ to.x, from.y, 0.0f, 1.0f, col },
		{ to.x, to.y, 0.0f, 1.0f, col },
		{ from.x, to.y, 0.0f, 1.0f, col },
		{ from.x, from.y, 0.0f, 1.0f, col }
	};

	dev->SetTexture(0, nullptr);
	dev->DrawPrimitiveUP(D3DPT_LINESTRIP, 4, &vert, sizeof(vertex));
}

void c_renderer::rect_linear_gradient(const Vector2D from, const Vector2D size, const Color color1, const Color color2, const bool horizontal) const
{
	const auto to = from + size;
	const auto col1 = color1.direct();
	const auto col2 = color2.direct();

	vertex vert[5] =
	{
		{ from.x, from.y, 0.0f, 1.0f, col2 },
		{ to.x, from.y, 0.0f, 1.0f, horizontal ? col2 : col1 },
		{ to.x, to.y, 0.0f, 1.0f, col2 },
		{ from.x, to.y, 0.0f, 1.0f, horizontal ? col1 : col2 },
		{ from.x, from.y, 0.0f, 1.0f, col1 }
	};

	dev->SetTexture(0, nullptr);
	dev->DrawPrimitiveUP(D3DPT_LINESTRIP, 4, &vert, sizeof(vertex));
}

void c_renderer::rect_full_linear_gradient(const Vector2D from, const Vector2D size, Color color1, Color color2, Color color3,
	Color color4) const
{
	const auto to = from + size;
	const auto col1 = color1.direct();
	const auto col2 = color2.direct();
	const auto col3 = color3.direct();
	const auto col4 = color4.direct();

	vertex vert[5] =
	{
		{ from.x, from.y, 0.0f, 1.0f, col1 },
		{ to.x, from.y, 0.0f, 1.0f, col2 },
		{ to.x, to.y, 0.0f, 1.0f, col3 },
		{ from.x, to.y, 0.0f, 1.0f, col4 },
		{ from.x, from.y, 0.0f, 1.0f, col1 }
	};

	dev->SetTexture(0, nullptr);
	dev->DrawPrimitiveUP(D3DPT_LINESTRIP, 4, &vert, sizeof(vertex));
}

void c_renderer::rect_filled(const Vector2D from, const Vector2D size, const Color color) const
{
	const auto to = from + size;
	const auto col = color.direct();

	vertex vert[4] =
	{
		{ from.x, from.y, 0.0f, 1.0f, col },
		{ to.x, from.y, 0.0f, 1.0f, col },
		{ from.x, to.y, 0.0f, 1.0f, col },
		{ to.x, to.y, 0.0f, 1.0f, col }
	};

	dev->SetTexture(0, nullptr);
	dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &vert, sizeof(vertex));
}

void c_renderer::rect_filled_linear_gradient(const Vector2D from, const Vector2D size, const Color color1,
	const Color color2, const bool horizontal) const
{
	const auto to = from + size;
	const auto col1 = color1.direct();
	const auto col2 = color2.direct();

	vertex vert[4] =
	{
		{ from.x, from.y, 0.0f, 1.0f, col1 },
		{ to.x, from.y, 0.0f, 1.0f, horizontal ? col2 : col1 },
		{ from.x, to.y, 0.0f, 1.0f, horizontal ? col1 : col2 },
		{ to.x, to.y, 0.0f, 1.0f, col2 }
	};

	dev->SetTexture(0, nullptr);
	dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &vert, sizeof(vertex));
}

void c_renderer::rect_filled_radial_gradient(const Vector2D from, const Vector2D size, const Color color1,
	const Color color2)
{
	const auto center = from + size / 2.0f;
	const auto radius = (center - from).length();

	D3DVIEWPORT9 new_port;
	new_port.X = static_cast<uint32_t>(from.x);
	new_port.Y = static_cast<uint32_t>(from.y);
	new_port.Width = static_cast<uint32_t>(size.x);
	new_port.Height = static_cast<uint32_t>(size.y);

	dev->SetViewport(&new_port);
	circle_filled_radial_gradient(center, radius, color1, color2);
	dev->SetViewport(&port);
}

void c_renderer::rect_filled_diamond_gradient(const Vector2D from, const Vector2D size, const Color color1,
	const Color color2) const
{
	const auto to = from + size;
	const auto col1 = color1.direct();
	const auto col2 = color2.direct();

	vertex vert[6] =
	{
		{ (from.x + to.x) / 2.0f, (from.y + to.y) / 2.0f, 0.0f, 1.0f, col2 },
		{ from.x, from.y, 0.0f, 1.0f, col1 },
		{ to.x, from.y, 0.0f, 1.0f, col1 },
		{ to.x, to.y, 0.0f, 1.0f, col1 },
		{ from.x, to.y, 0.0f, 1.0f, col1 },
		{ from.x, from.y, 0.0f, 1.0f, col1 }
	};

	dev->SetTexture(0, nullptr);
	dev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 5, &vert, sizeof(vertex));
}

void c_renderer::parallelogram(const Vector2D from, const Vector2D size, Color color, const uint8_t side, const float radius) const
{
	const auto to = from + size;
	const auto col = color.direct();

	vertex vert[5] =
	{
		{ from.x + (side != 1 ? radius : 0.0f), from.y, 0.0f, 1.0f, col },
		{ to.x, from.y, 0.0f, 1.0f, col },
		{ to.x - (side != 2 ? radius : 0.0f), to.y, 0.0f, 1.0f, col },
		{ from.x, to.y, 0.0f, 1.0f, col },
		{ from.x + (side != 1 ? radius : 0.0f), from.y, 0.0f, 1.0f, col }
	};

	dev->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
	dev->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);

	dev->SetTexture(0, nullptr);
	dev->DrawPrimitiveUP(D3DPT_LINESTRIP, 4, &vert, sizeof(vertex));

	dev->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
	dev->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
}

void c_renderer::parallelogram_filled_linear_gradient(const Vector2D from, const Vector2D size, Color color1,
	Color color2, const uint8_t side, const bool horizontal, const float radius) const
{
	const auto to = from + size;
	const auto col1 = color1.direct();
	const auto col2 = color2.direct();

	vertex vert[4] =
	{
		{ from.x + (side != 1 ? radius : 0.0f), from.y, 0.0f, 1.0f, col1 },
		{ to.x, from.y, 0.0f, 1.0f, horizontal ? col2 : col1 },
		{ from.x, to.y, 0.0f, 1.0f, horizontal ? col1 : col2 },
		{ to.x - (side != 2 ? radius : 0.0f), to.y, 0.0f, 1.0f, col2 }
	};

	dev->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
	dev->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);

	dev->SetTexture(0, nullptr);
	dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &vert, sizeof(vertex));

	dev->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
	dev->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
}

void c_renderer::triangle(const Vector2D pos1, const Vector2D pos2, const Vector2D pos3, const Color color) const
{
	const auto col = color.direct();

	vertex vert[4] =
	{
		{ pos1.x, pos1.y, 0.0f, 1.0f, col },
		{ pos2.x, pos2.y, 0.0f, 1.0f, col },
		{ pos3.x, pos3.y, 0.0f, 1.0f, col },
		{ pos1.x, pos1.y, 0.0f, 1.0f, col }
	};

	dev->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
	dev->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);

	dev->SetTexture(0, nullptr);
	dev->DrawPrimitiveUP(D3DPT_LINESTRIP, 3, &vert, sizeof(vertex));

	dev->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
	dev->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
}

void c_renderer::triangle_linear_gradient(const Vector2D pos1, const Vector2D pos2, const Vector2D pos3, Color color1, Color color2, Color color3) const
{
	const auto col1 = color1.direct();
	const auto col2 = color2.direct();
	const auto col3 = color3.direct();

	vertex vert[4] =
	{
		{ pos1.x, pos1.y, 0.0f, 1.0f, col1 },
		{ pos2.x, pos2.y, 0.0f, 1.0f, col2 },
		{ pos3.x, pos3.y, 0.0f, 1.0f, col3 },
		{ pos1.x, pos1.y, 0.0f, 1.0f, col1 }
	};

	dev->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
	dev->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);

	dev->SetTexture(0, nullptr);
	dev->DrawPrimitiveUP(D3DPT_LINESTRIP, 3, &vert, sizeof(vertex));

	dev->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
	dev->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
}

void c_renderer::triangle_filled(const Vector2D pos1, const Vector2D pos2, const Vector2D pos3, const Color color) const
{
	const auto col = color.direct();

	vertex vert[4] =
	{
		{ pos1.x, pos1.y, 0.0f, 1.0f, col },
		{ pos2.x, pos2.y, 0.0f, 1.0f, col },
		{ pos3.x, pos3.y, 0.0f, 1.0f, col }
	};

	dev->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
	dev->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);

	dev->SetTexture(0, nullptr);
	dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 1, &vert, sizeof(vertex));

	dev->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
	dev->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
}

void c_renderer::triangle_filled_linear_gradient(const Vector2D pos1, const Vector2D pos2, const Vector2D pos3,
	const Color color1, const Color color2, const Color color3) const
{
	const auto col1 = color1.direct();
	const auto col2 = color2.direct();
	const auto col3 = color3.direct();

	vertex vert[4] =
	{
		{ pos1.x, pos1.y, 0.0f, 1.0f, col1 },
		{ pos2.x, pos2.y, 0.0f, 1.0f, col2 },
		{ pos3.x, pos3.y, 0.0f, 1.0f, col3 }
	};

	dev->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
	dev->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);

	dev->SetTexture(0, nullptr);
	dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 1, &vert, sizeof(vertex));

	dev->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
	dev->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
}

void c_renderer::circle(const Vector2D center, const float radius, const Color color)
{
	const auto col = color.direct();
	build_lookup_table();

	vertex vert[points + 1] = {};

	for (auto i = 0; i <= points; i++)
		vert[i] =
	{
		center.x + radius * lookup_table[i].x,
		center.y - radius * lookup_table[i].y,
		0.0f,
		1.0f,
		col
	};

	dev->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
	dev->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);

	dev->SetTexture(0, nullptr);
	dev->DrawPrimitiveUP(D3DPT_LINESTRIP, points, &vert, sizeof(vertex));

	dev->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
	dev->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
}

void c_renderer::circle_filled(const Vector2D center, const float radius, const Color color)
{
	const auto col = color.direct();
	build_lookup_table();

	vertex vert[points + 1] = {};

	for (auto i = 0; i <= points; i++)
		vert[i] =
	{
		center.x + radius * lookup_table[i].x,
		center.y - radius * lookup_table[i].y,
		0.0f,
		1.0f,
		col
	};

	dev->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
	dev->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);

	dev->SetTexture(0, nullptr);
	dev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, points, &vert, sizeof(vertex));

	dev->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
	dev->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
}

void c_renderer::circle_filled_radial_gradient(const Vector2D center, const float radius, const Color color1,
	const Color color2)
{
	const auto col1 = color1.direct();
	const auto col2 = color2.direct();
	build_lookup_table();

	vertex vert[points + 2] = {};

	for (auto i = 1; i <= points; i++)
		vert[i] =
	{
		center.x + radius * lookup_table[i].x,
		center.y - radius * lookup_table[i].y,
		0.0f,
		1.0f,
		col1
	};

	vert[0] = { center.x, center.y, 0.0f, 1.0f, col2 };
	vert[points + 1] = vert[1];

	dev->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
	dev->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);

	dev->SetTexture(0, nullptr);
	dev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, points, &vert, sizeof(vertex));

	dev->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
	dev->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
}

void c_renderer::text(const Vector2D pos, const char* str, Color color, const uint32_t font, const uint8_t flags)
{
	fonts.at(font).draw_string(std::roundf(pos.x), std::roundf(pos.y), color.direct(), str, flags);
}

void c_renderer::filled_box(const int x, const int y, const int w, const int h, const Color color) const
{
	vertex vertices2[4] = { { x, y + h, 0.0f, 1.0f, color.direct() },{ x,  y, 0.0f, 1.0f, color.direct() },{ (x + w),  (y + h), 0.0f, 1.0f, color.direct() },{ (x + w), y, 0.0f, 1.0f, color.direct() } };

	dev->SetTexture(0, nullptr);
	dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices2, sizeof(vertex));
}

void c_renderer::filled_box_outlined(const int x, const int y, const int w, const int h, const Color color, const Color outline, const int thickness) const
{
	filled_box(x, y, w, h, color);
	bordered_box(x, y, w, h, outline, thickness);
}

void c_renderer::bordered_box(const int x, const int y, const int w, const int h, const Color color, const int thickness) const
{
	filled_box(x, y, w, thickness, color);
	filled_box(x, y, thickness, h, color);
	filled_box(x + w - thickness, y, thickness, h, color);
	filled_box(x, y + h - thickness, w, thickness, color);
}

void c_renderer::bordered_box_outlined(const int x, const int y, const int w, const int h, const Color color, const Color outline, const int thickness) const
{
	bordered_box(x, y, w, h, outline, thickness);
	bordered_box(x + thickness, y + thickness, w - (thickness * 2), h - (thickness * 2), color, thickness);
	bordered_box(x + (thickness * 2), y + (thickness * 2), w - (thickness * 4), h - (thickness * 4), outline, thickness);
}

Vector2D c_renderer::get_text_size(const char* str, const uint32_t font)
{
	SIZE size;
	fonts.at(font).get_text_extent(str, &size);
	return Vector2D(static_cast<float>(size.cx), static_cast<float>(size.cy));
}

void c_renderer::create_texture(void* src, const uint32_t size, LPDIRECT3DTEXTURE9* texture) const
{
	D3DXCreateTextureFromFileInMemory(dev, src, size, texture);
}

void c_renderer::create_sprite(LPD3DXSPRITE* sprite) const
{
	D3DXCreateSprite(dev, sprite);
}

void c_renderer::image(const Vector2D position, LPDIRECT3DTEXTURE9 texture, LPD3DXSPRITE sprite, const float scl, const float alpha)
{
	D3DXMATRIX world;
	D3DXMATRIX rotation;
	D3DXMATRIX scale;
	D3DXMATRIX translation;
	D3DXMatrixIdentity(&world);

	D3DXMatrixScaling(&scale, scl, scl, 1.f);
	D3DXMatrixRotationYawPitchRoll(&rotation, 0.f, 0.f, 0.f);
	D3DXMatrixTranslation(&translation, 0.f, 0.f, 0.f);
	world = rotation * scale * translation;

	D3DSURFACE_DESC img_info;
	texture->GetLevelDesc(0, &img_info);

	auto vec = D3DXVECTOR3(position.x, position.y, 0.f);
	sprite->SetTransform(&world);
	sprite->Begin(D3DXSPRITE_ALPHABLEND);
	sprite->Draw(texture, nullptr, nullptr, &vec, D3DCOLOR_RGBA(255, 255, 255, static_cast<int>(255 * alpha)));
	sprite->End();
}

Vector2D c_renderer::get_center() const
{
	return Vector2D(static_cast<float>(port.Width), static_cast<float>(port.Height)) / 2.f;
}

VMatrix& c_renderer::world_to_screen_matrix()
{
	static auto view_matrix = NULL;
	return *reinterpret_cast<VMatrix*>(view_matrix);
}

void c_renderer::limit(const rectangle rect) const
{
	RECT rec;
	rec.left = static_cast<LONG>(rect.first.x);
	rec.top = static_cast<LONG>(rect.first.y);
	rec.right = static_cast<LONG>(rect.first.x + rect.second.x);
	rec.bottom = static_cast<LONG>(rect.first.y + rect.second.y);

	dev->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
	dev->SetScissorRect(&rec);
}

rectangle c_renderer::get_limit() const
{
	RECT rec;
	dev->GetScissorRect(&rec);

	return rectangle({
		static_cast<float>(rec.left),
		static_cast<float>(rec.top)
	}, {
		static_cast<float>(rec.right),
		static_cast<float>(rec.bottom)
	});
}

void c_renderer::reset_limit() const
{
	dev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
}

void c_renderer::refresh_viewport()
{
	this->dev->GetViewport(&port);
}

void c_renderer::init_device_objects(IDirect3DDevice9* dev)
{
	//decrypts(0)
	this->dev = dev;
	this->dev->GetViewport(&port);

	DWORD fnt;
	AddFontMemResourceEx(reinterpret_cast<void*>(profont), sizeof(profont), nullptr, &fnt);

	fonts[cx::fnv1a(XorStr("esp"))] = c_font("Tahoma", 12, FW_REGULAR, 0x80, 0, ANTIALIASED_QUALITY);
	fonts[cx::fnv1a(XorStr("esp_small"))] = c_font("Tahoma", 13, FW_REGULAR, 0x80, 0, ANTIALIASED_QUALITY);
	fonts[cx::fnv1a(XorStr("esp_name"))] = c_font("Tahoma", 12, FW_REGULAR, 0x80, 0, ANTIALIASED_QUALITY);
	fonts[cx::fnv1a(XorStr("indicator"))] = c_font("Verdana", 27, FW_REGULAR, 0x80, 0, ANTIALIASED_QUALITY);
	fonts[cx::fnv1a(XorStr("esp_flags"))] = c_font("Smallest Pixel-7", 11, FW_NORMAL, 0x80, 0, ANTIALIASED_QUALITY);

	for (auto& font : fonts)
	{
		font.second.init_device_objects(dev);
		font.second.restore_device_objects();
	}
	//encrypts(0)
}

void c_renderer::invalidate_device_objects()
{
	dev = nullptr;

	for (auto& font : fonts)
		font.second.invalidate_device_objects();

	for (auto& handler : reset_handlers)
		handler();
}

void c_renderer::setup_render_state() const
{
	if (!dev)
		return;

	dev->SetVertexShader(nullptr);
	dev->SetPixelShader(nullptr);
	dev->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE); // NOLINT
	dev->SetRenderState(D3DRS_LIGHTING, FALSE);
	dev->SetRenderState(D3DRS_FOGENABLE, FALSE);
	dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	dev->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

	dev->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
	dev->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
	dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	dev->SetRenderState(D3DRS_STENCILENABLE, FALSE);

	dev->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
	dev->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);

	dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	dev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
	dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	dev->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_INVDESTALPHA);
	dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	dev->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ONE);

	dev->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
	dev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN // NOLINT NOLINTNEXTLINE
		| D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);

	dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

	dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_NONE);
	dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_NONE);
}

void c_renderer::register_reset_handler(const std::function<void()> handler)
{
	reset_handlers.push_back(handler);
}

void c_renderer::build_lookup_table()
{
	if (!lookup_table.empty())
		return;

	for (auto i = 0; i <= points; i++)
		lookup_table.emplace_back(
			std::cos(2.f * D3DX_PI * (i / static_cast<float>(points))),
			std::sin(2.f * D3DX_PI * (i / static_cast<float>(points)))
		);

	for (auto lat = 0; lat < points_sphere_latitude; lat++)
	{
		const auto a1 = D3DX_PI * static_cast<float>(lat + 1) / (points_sphere_latitude + 1);
		const auto sin1 = sin(a1);
		const auto cos1 = cos(a1);

		for (auto lon = 0; lon <= points_sphere_longitude; lon++)
		{
			const auto a2 = 2 * D3DX_PI * static_cast<float>(lon) / points_sphere_longitude;
			const auto sin2 = sin(a2);
			const auto cos2 = cos(a2);

			lookup_sphere.emplace_back(sin1 * cos2, cos1, sin1 * sin2);
		}
	}
}
