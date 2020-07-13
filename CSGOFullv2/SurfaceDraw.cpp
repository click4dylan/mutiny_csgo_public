#include "precompiled.h"
#include "SurfaceDraw.h"
#include "LocalPlayer.h"
#include <stdio.h>

CSurfaceDraw g_Draw;

void TextW(CSurfaceDraw::TextAlign align, HFont font, int x, int y, int r, int g, int b, int a, wchar_t* pszString)
{
	Interfaces::Surface->DrawSetTextFont(font);
	Interfaces::Surface->DrawSetTextColor(r, g, b, a);

	if (align == CSurfaceDraw::TextAlign::CENTER) {
		int wide, tall;
		Interfaces::Surface->GetTextSize(font, pszString, wide, tall);
		x -= wide / 2;
		y -= tall / 2;
	}
	else if (align == CSurfaceDraw::TextAlign::RIGHT) {
		int wide, tall;
		Interfaces::Surface->GetTextSize(font, pszString, wide, tall);
		x -= wide;
	}

	Interfaces::Surface->DrawSetTextPos(x, y);
	Interfaces::Surface->DrawPrintText(pszString, (int)wcslen(pszString));
}

Vertex_t RotateVertex(const Vector2D& o, const Vertex_t& v, float angle)
{
	float t = DEG2RAD(angle), c = cos(t), s = sin(t);

	return Vertex_t{
		{
			o.x + (v.m_Position.x - o.x) * c - (v.m_Position.y - o.y) * s,
			o.y + (v.m_Position.x - o.x) * s + (v.m_Position.y - o.y) * c
		}
	};
}

void CSurfaceDraw::Update()
{
	sScreenSize CurrentScreenSize;
	Interfaces::EngineClient->GetScreenSize(CurrentScreenSize.Width, CurrentScreenSize.Height);

	if (CurrentScreenSize.Width != g_Info.ScreenSize.Width
		|| CurrentScreenSize.Height != g_Info.ScreenSize.Height)
	{
		BuildFonts();
		g_Info.ScreenSize.Width = CurrentScreenSize.Width;
		g_Info.ScreenSize.Height = CurrentScreenSize.Height;
	}
}

void CSurfaceDraw::BuildFonts()
{
	Fonts::Watermark = Interfaces::Surface->Create_Font();
	//decrypts(0)
	Interfaces::Surface->SetFontGlyphSet(Fonts::Watermark, XorStr("Tahoma"), 16, 800, 0, 0, FONTFLAG_OUTLINE);

	Fonts::Main = Interfaces::Surface->Create_Font();
	Interfaces::Surface->SetFontGlyphSet(Fonts::Main, XorStr("Tahoma"), 12, 0, 0, 0, FONTFLAG_OUTLINE);

	Fonts::Shadow = Interfaces::Surface->Create_Font();
	Interfaces::Surface->SetFontGlyphSet(Fonts::Shadow, XorStr("Tahoma"), 12, 800, 0, 0, FONTFLAG_DROPSHADOW);

	Fonts::MenuElements = Interfaces::Surface->Create_Font();
	Interfaces::Surface->SetFontGlyphSet(Fonts::MenuElements, XorStr("Tahoma"), 12, 0, 0, 0, FONTFLAG_NONE);

	Fonts::Bold = Interfaces::Surface->Create_Font();
	Interfaces::Surface->SetFontGlyphSet(Fonts::Bold, XorStr("Tahoma"), 12, 800, 0, 0, FONTFLAG_OUTLINE);

	Fonts::LBY = Interfaces::Surface->Create_Font();
	Interfaces::Surface->SetFontGlyphSet(Fonts::LBY, XorStr("Tahoma"), 24, 800, 0, 0, FONTFLAG_DROPSHADOW);
	//encrypts(0)
}

void CSurfaceDraw::GradientVertical(int x, int y, int w, int h, int from_r, int from_g, int from_b, int from_a, int to_r, int to_g, int to_b, int to_a) {
	float r = 1.f * (to_r - from_r) / h;
	float g = 1.f * (to_g - from_g) / h;
	float b = 1.f * (to_b - from_b) / h;
	float a = 1.f * (to_a - from_a) / h;

	for (int i = 0; i < h; i++) {
		int R = from_r + r * i;
		int G = from_g + g * i;
		int B = from_b + b * i;
		int A = from_a + a * i;

		FillRGBA(x, y + i, w, 1, R, G, B, A);
	}
}

void CSurfaceDraw::GradientHorizontal(int x, int y, int w, int h, int from_r, int from_g, int from_b, int from_a, int to_r, int to_g, int to_b, int to_a) {
	float r = 1.f * (to_r - from_r) / w;
	float g = 1.f * (to_g - from_g) / w;
	float b = 1.f * (to_b - from_b) / w;
	float a = 1.f * (to_a - from_a) / w;

	for (float i = 0.f; i < w; i++) {
		int R = from_r + ceil(r * i);
		int G = from_g + ceil(g * i);
		int B = from_b + ceil(b * i);
		int A = from_a + ceil(a * i);

		FillRGBA(x + i, y, 1, h, R, G, B, A);
	}
}

void CSurfaceDraw::DrawMouse(int x, int y) {

	float r = 1.f * (30 - 80) / 17;
	int R = 80 + r * 2;

	FillRGBA(x + 1, y + 2, 1, 1, R, R, R, 255); R = 50 + r * 3;
	FillRGBA(x + 1, y + 3, 2, 1, R, R, R, 255); R = 50 + r * 4;
	FillRGBA(x + 1, y + 4, 3, 1, R, R, R, 255); R = 50 + r * 5;
	FillRGBA(x + 1, y + 5, 4, 1, R, R, R, 255); R = 50 + r * 6;
	FillRGBA(x + 1, y + 6, 5, 1, R, R, R, 255); R = 50 + r * 7;
	FillRGBA(x + 1, y + 7, 6, 1, R, R, R, 255); R = 50 + r * 8;
	FillRGBA(x + 1, y + 8, 7, 1, R, R, R, 255); R = 50 + r * 9;
	FillRGBA(x + 1, y + 9, 8, 1, R, R, R, 255); R = 50 + r * 10;
	FillRGBA(x + 1, y + 10, 9, 1, R, R, R, 255); R = 50 + r * 11;
	FillRGBA(x + 1, y + 11, 10, 1, R, R, R, 255); R = 50 + r * 12;
	FillRGBA(x + 1, y + 12, 6, 1, R, R, R, 255); R = 50 + r * 13;
	FillRGBA(x + 1, y + 13, 6, 1, R, R, R, 255); R = 50 + r * 14;
	FillRGBA(x + 1, y + 14, 2, 1, R, R, R, 255);
	FillRGBA(x + 6, y + 14, 2, 1, R, R, R, 255); R = 50 + r * 15;
	FillRGBA(x + 1, y + 15, 1, 1, R, R, R, 255);
	FillRGBA(x + 6, y + 15, 2, 1, R, R, R, 255); R = 50 + r * 16;
	FillRGBA(x + 7, y + 16, 2, 1, R, R, R, 255); R = 50 + r * 17;
	FillRGBA(x + 7, y + 17, 2, 1, R, R, R, 255);

	FillRGBA(x, y, 1, 17, 0, 160, 255, 255);;
	FillRGBA(x, y + 16, 2, 1, 0, 160, 255, 255);;
	FillRGBA(x + 2, y + 15, 1, 1, 0, 160, 255, 255);;
	FillRGBA(x + 3, y + 14, 1, 1, 0, 160, 255, 255);;
	FillRGBA(x + 4, y + 13, 1, 1, 0, 160, 255, 255);;
	FillRGBA(x + 5, y + 14, 1, 2, 0, 160, 255, 255);;
	FillRGBA(x + 6, y + 16, 1, 2, 0, 160, 255, 255);;
	FillRGBA(x + 7, y + 18, 2, 1, 0, 160, 255, 255);;
	FillRGBA(x + 8, y + 14, 1, 2, 0, 160, 255, 255);;
	FillRGBA(x + 9, y + 16, 1, 2, 0, 160, 255, 255);;
	FillRGBA(x + 7, y + 12, 1, 2, 0, 160, 255, 255);;
	FillRGBA(x + 7, y + 12, 5, 1, 0, 160, 255, 255);;
	FillRGBA(x + 11, y + 11, 1, 1, 0, 160, 255, 255);;
	FillRGBA(x + 10, y + 10, 1, 1, 0, 160, 255, 255);;
	FillRGBA(x + 9, y + 9, 1, 1, 0, 160, 255, 255);;
	FillRGBA(x + 8, y + 8, 1, 1, 0, 160, 255, 255);;
	FillRGBA(x + 7, y + 7, 1, 1, 0, 160, 255, 255);;
	FillRGBA(x + 6, y + 6, 1, 1, 0, 160, 255, 255);;
	FillRGBA(x + 5, y + 5, 1, 1, 0, 160, 255, 255);;
	FillRGBA(x + 4, y + 4, 1, 1, 0, 160, 255, 255);;
	FillRGBA(x + 3, y + 3, 1, 1, 0, 160, 255, 255);;
	FillRGBA(x + 2, y + 2, 1, 1, 0, 160, 255, 255);;
	FillRGBA(x + 1, y + 1, 1, 1, 0, 160, 255, 255);;
}

void CSurfaceDraw::DrawMouse(POINT pos)
{
	DrawMouse(pos.x, pos.y);
}

void CSurfaceDraw::FillRGBA(int x, int y, int w, int h, int r, int g, int b, int a)
{
	Interfaces::Surface->DrawSetColor(r, g, b, a);
	Interfaces::Surface->DrawFilledRect(x, y, x + w, y + h);
}

void CSurfaceDraw::FillRGBA(int x, int y, int w, int h, Color color)
{
	Interfaces::Surface->DrawSetColor(color.r(), color.g(), color.b(), color.a());
	Interfaces::Surface->DrawFilledRect(x, y, x + w, y + h);
}

void CSurfaceDraw::FillRGBA(int x, int y, int w, int h, Color color, int a)
{
	Interfaces::Surface->DrawSetColor(color.r(), color.g(), color.b(), a);
	Interfaces::Surface->DrawFilledRect(x, y, x + w, y + h);
}

void CSurfaceDraw::Border(int x, int y, int w, int h, int line, int r, int g, int b, int a)
{
	FillRGBA(x, y, w, line, r, g, b, a);
	FillRGBA(x, y, line, h, r, g, b, a);
	FillRGBA(x + w - line, y, line, h, r, g, b, a);
	FillRGBA(x, y + h - line, w - line, line, r, g, b, a);
}

void CSurfaceDraw::ESPBox(int x, int y, int w, int h, int borderPx, int percentW, int percentH, Color clr)
{
	int _width = (w / 2) * ((1.f * percentW) / 100.f), _height = (h / 2) * ((1.f * percentH) / 100.f);
	if (borderPx > 0) {
		if (percentW < 100) {
			FillRGBA(x - borderPx, y - borderPx, _width + (borderPx * 2), 1 + borderPx * 2, Color::Black());						//Side: Top - Part: Left 
			FillRGBA(x - borderPx + w - (_width), y - borderPx, _width + (borderPx * 2), 1 + borderPx * 2, Color::Black());			//Side: Top - Part: Right
			FillRGBA(x - borderPx, y - borderPx + h, _width + (borderPx * 2), 1 + borderPx * 2, Color::Black());					//Side: Bottom - Part: Left 
			FillRGBA(x - borderPx + w - (_width), y - borderPx + h, _width + (borderPx * 2) + 1, 1 + borderPx * 2, Color::Black());	//Side: Bottom - Part: Right
		}
		else {
			FillRGBA(x - borderPx, y - borderPx, w + (borderPx * 2), 1 + borderPx * 2, Color::Black());								//Side: Top
			FillRGBA(x - borderPx, y - borderPx + h, w + (borderPx * 2) + 1, 1 + borderPx * 2, Color::Black());						//Side: Bottom
		}
		if (percentH < 100) {
			FillRGBA(x - borderPx, y - borderPx, 1 + borderPx * 2, _height + (borderPx * 2), Color::Black());						//Side: Left - Part: Top
			FillRGBA(x - borderPx, y - borderPx + h - (_height), 1 + borderPx * 2, _height + (borderPx * 2), Color::Black());		//Side: Left - Part: Bottom
			FillRGBA(x - borderPx + w, y - borderPx, 1 + borderPx * 2, _height + (borderPx * 2), Color::Black());					//Side: Right - Part: Top
			FillRGBA(x - borderPx + w, y - borderPx + h - (_height), 1 + borderPx * 2, _height + (borderPx * 2), Color::Black());	//Side: Right - Part: Bottom
		}
		else {
			FillRGBA(x - borderPx, y - borderPx, 1 + borderPx * 2, h + (borderPx * 2), Color::Black());								//Side: Left
			FillRGBA(x - borderPx + w, y - borderPx, 1 + borderPx * 2, h + (borderPx * 2), Color::Black());							//Side: Right
		}
	}

	if (percentW < 100) {
		FillRGBA(x, y, _width, 1, clr);							//Side: Top - Part: Left 
		FillRGBA(x + w - (_width), y, _width, 1, clr);			//Side: Top - Part: Right
		FillRGBA(x, y + h, _width, 1, clr);						//Side: Bottom - Part: Left 
		FillRGBA(x + w - (_width), y + h, _width + 1, 1, clr);	//Side: Bottom - Part: Right
	}
	else {
		FillRGBA(x, y, w, 1, clr);								//Side: Top
		FillRGBA(x, y + h, w + 1, 1, clr);						//Side: Bottom
	}

	if (percentH < 100) {
		FillRGBA(x, y, 1, _height, clr);						//Side: Left - Part: Top
		FillRGBA(x, y + h - (_height), 1, _height, clr);		//Side: Left - Part: Bottom
		FillRGBA(x + w, y, 1, _height, clr);					//Side: Right - Part: Top
		FillRGBA(x + w, y + h - (_height), 1, _height, clr);	//Side: Right - Part: Bottom
	}
	else {
		FillRGBA(x, y, 1, h, clr);								//Side: Left
		FillRGBA(x + w, y, 1, h, clr);							//Side: Right
	}
}

void CSurfaceDraw::CornerBox(int x, int y, int w, int h, int borderPx, int r, int g, int b, int a)
{
	FillRGBA((x - (w / 2)) - 1, (y - h + borderPx) - 1, (w / 3) + 2, borderPx + 2, 0, 0, 0, 255); //top left
	FillRGBA((x - (w / 2) + w - w / 3) - 1, (y - h + borderPx) - 1, w / 3, borderPx + 2, 0, 0, 0, 255); //top right
	FillRGBA(x - (w / 2) - 1, (y - h + borderPx), borderPx + 2, (w / 3) + 1, 0, 0, 0, 255); //left top
	FillRGBA(x - (w / 2) - 1, ((y - h + borderPx) + h - w / 3) - 1, borderPx + 2, (w / 3) + 2, 0, 0, 0, 255); //left bottom
	FillRGBA(x - (w / 2), y - 1, (w / 3) + 1, borderPx + 2, 0, 0, 0, 255); //bottom left
	FillRGBA(x - (w / 2) + w - (w / 3 + 1), y - 1, (w / 3) + 2, borderPx + 2, 0, 0, 0, 255); //bottom right
	FillRGBA((x + w - borderPx) - (w / 2) - 1, (y - h + borderPx) - 1, borderPx + 2, w / 3 + 2, 0, 0, 0, 255); //right bottom
	FillRGBA((x + w - borderPx) - (w / 2) - 1, ((y - h + borderPx) + h - w / 3) - 1, borderPx + 2, (w / 3) + 2, 0, 0, 0, 255); //right up

	FillRGBA(x - (w / 2), (y - h + borderPx), w / 3, borderPx, r, g, b, a);
	FillRGBA(x - (w / 2) + w - w / 3, (y - h + borderPx), w / 3, borderPx, r, g, b, a);
	FillRGBA(x - (w / 2), (y - h + borderPx), borderPx, w / 3, r, g, b, a);
	FillRGBA(x - (w / 2), (y - h + borderPx) + h - w / 3, borderPx, w / 3, r, g, b, a);
	FillRGBA(x - (w / 2), y, w / 3, borderPx, r, g, b, a);
	FillRGBA(x - (w / 2) + w - w / 3, y, w / 3, borderPx, r, g, b, a);
	FillRGBA((x + w - borderPx) - (w / 2), (y - h + borderPx), borderPx, w / 3, r, g, b, a);
	FillRGBA((x + w - borderPx) - (w / 2), (y - h + borderPx) + h - w / 3, borderPx, w / 3, r, g, b, a);
}

void CSurfaceDraw::HealthbarHorizontal(int x, int y, int w, int h, int EntityHealth, bool text, int alpha)
{
	int hp = clamp(EntityHealth, 0, 100);
	float flBoxes = std::ceil(hp / 10.f);
	float flMultiplier = 12 / 360.f; flMultiplier *= flBoxes - 1;
	Color ColHealth = Color::FromHSB(flMultiplier, 1, 1);

	ColHealth = Color(ColHealth.r(), ColHealth.g(), ColHealth.b(), alpha);

	float HealthSize = clamp((float)w * ((float)hp / 100.0f), 0.f, (float)w);
	float flHeight = w / 10.f;

	FillRGBA(x, y, w, h, Color(0, 0, 0, clamp(alpha - 100, 0, 255)));
	FillRGBA(x, y, HealthSize, h, ColHealth);
	FillRGBA(x + HealthSize - 1, y, 1, h, Color(0, 0, 0, alpha));
	Border(x, y, w, h, 1, 0, 0, 0, alpha);
	if (text && EntityHealth < 100)
		Text(x + HealthSize, y + (h / 2), 255, 255, 255, alpha, CENTER, Fonts::Main, "%i", EntityHealth);
	//FillRGBA(x + 2, y - 2, w, h - (h * ((float)hp / 100.0f)), Color(0, 0, 0, 255));

	/*if (flHeight > 10) {
		for (int i = 0; i < flBoxes; i++)
			FillRGBA(x + w - i * flHeight, y + 1, 1, h - 1, Color(0, 0, 0, alpha));
	}*/
}

void CSurfaceDraw::Healthbar(int x, int y, int w, int h, int EntityHealth, bool text, int alpha)
{
	int hp = clamp(EntityHealth, 0, 100);
	float flBoxes = std::ceil(hp / 10.f);
	float flMultiplier = 12 / 360.f; flMultiplier *= flBoxes - 1;
	Color ColHealth = Color::FromHSB(flMultiplier, 1, 1);

	ColHealth = Color(ColHealth.r(), ColHealth.g(), ColHealth.b(), alpha);

	float HealthSize = clamp(h * ((float)hp / 100.0f), 0.f, (float)h);
	float flHeight = h / 10.f;

	FillRGBA(x, y, w, h, Color(0, 0, 0, clamp(alpha - 100, 0, 255)));
	FillRGBA(x, y + h - HealthSize, w, HealthSize, ColHealth);
	FillRGBA(x, y + h - HealthSize, w, 1, Color(0, 0, 0, alpha));
	Border(x, y, w, h, 1, 0, 0, 0, alpha);
	if (text && EntityHealth < 100)
	{
		//decrypts(0)
		Text(x + (w / 2), y + h - HealthSize, 255, 255, 255, alpha, CENTER, Fonts::Main, XorStr("%i"), EntityHealth);
		//encrypts(0)
	}
	//FillRGBA(x + 2, y - 2, w, h - (h * ((float)hp / 100.0f)), Color(0, 0, 0, 255));

	/*if (flHeight > 10) {
		for (int i = 0; i < flBoxes; i++)
			FillRGBA(x + 1, y + h - i * flHeight - 1, w - 1, 1, Color(0, 0, 0, alpha));
	}*/
}

void CSurfaceDraw::DrawBar(int x, int y, int w, int h, float _value, float _max, Color clr, bool _drawText, std::string _text, int alpha)
{
	int FillSize = clamp(h * ((1.f * _value) / _max), 0.f, (float)h);
	FillRGBA(x, y, w, h, Color(0, 0, 0, clamp(alpha - 100, 0, 255)));
	FillRGBA(x + 1, y + h - FillSize, w - 1, FillSize, clr);
	FillRGBA(x + 1, y + h - FillSize, w - 1, 1, Color(0, 0, 0, alpha));
	Border(x, y, w, h, 1, 0, 0, 0, alpha);
	if (_drawText) {
		//decrypts(0)
		if (_text.empty() && _value < _max)
			Text(x + (w / 2), y + h - FillSize, 255, 255, 2255, alpha, CENTER, Fonts::Main, XorStr("%i"), (int)_value);
		else
			Text(x + (w / 2), y + h - FillSize, 255, 255, 255, alpha, CENTER, Fonts::Main, XorStr("%s"), _text.c_str());
		//encrypts(0)
	}
}

void CSurfaceDraw::DrawBar(int x, int y, int w, int h, int _value, int _max, Color clr, bool _drawText, std::string _text, int alpha)
{
	int FillSize = clamp(h * ((1.f * _value) / _max), 0.f, (float)h);
	FillRGBA(x, y, w, h, Color(0, 0, 0, clamp(alpha - 100, 0, 255)));
	FillRGBA(x + 1, y + h - FillSize, w - 1, FillSize, clr);
	FillRGBA(x + 1, y + h - FillSize, w - 1, 1, Color(0, 0, 0, alpha));
	Border(x, y, w, h, 1, 0, 0, 0, alpha);
	if (_drawText) {
		//decrypts(0)
		if (_text.empty() && _value < _max)
			Text(x + (w / 2), y + h - FillSize, 255, 255, 255, alpha, CENTER, Fonts::Main, XorStr("%i"), _value);
		else
			Text(x + (w / 2), y + h - FillSize, 255, 255, 255, alpha, CENTER, Fonts::Main, XorStr("%s"), _text.c_str());
		//encrypts(0)
	}
}

void CSurfaceDraw::DrawBarHorizontal(int x, int y, int w, int h, float _value, float _max, Color clr, bool _drawText, std::string _text, int alpha)
{
	const int FillSize = clamp(w * (_value / _max), 0.f, (float)w);
	
	FillRGBA(x, y, w, h, Color(0, 0, 0, clamp(alpha - 100, 0, 255)));
	FillRGBA(x + 1, y + 1, FillSize, h - 1, clr);
	FillRGBA(x + FillSize, y + 1, 1, h - 1, Color(0, 0, 0, alpha));
	Border(x, y, w, h, 1, 0, 0, 0, alpha);
	if (_drawText) {
		//decrypts(0)
		if (_text.empty() && _value < _max)
			Text(x + FillSize, y + (h / 2), 255, 255, 255, alpha, CENTER, Fonts::Main, XorStr("%i"), (int)_value);
		else
			Text(x + FillSize, y + (h / 2), 255, 255, 255, alpha, CENTER, Fonts::Main, XorStr("%s"), _text.c_str());
		//encrypts(0)
	}
	
}

void CSurfaceDraw::DrawBarHorizontal(int x, int y, int w, int h, int _value, int _max, Color clr, bool _drawText, std::string _text, int alpha)
{
	int FillSize = clamp(w * ((float)_value / _max), 0.f, (float)w);
	
	FillRGBA(x, y, w, h, Color(0, 0, 0, clamp(alpha - 100, 0, 255)));
	FillRGBA(x + 1, y + 1, FillSize, h - 1, clr);
	FillRGBA(x + FillSize, y + 1, 1, h - 1, Color(0, 0, 0, alpha));
	Border(x, y, w, h, 1, 0, 0, 0, alpha);
	if (_drawText) {
		//decrypts(0)
		if (_text.empty() && _value < _max)
			Text(x + FillSize, y + (h / 2), 255, 255, 255, alpha, CENTER, Fonts::Main, XorStr("%i"), _value);
		else
			Text(x + FillSize, y + (h / 2), 255, 255, 255, alpha, CENTER, Fonts::Main, XorStr("%s"), _text.c_str());
		//encrypts(0)
	}
	
}

void CSurfaceDraw::Text(int x, int y, int r, int g, int b, int a, TextAlign bAlign, HFont font, const char *fmt, ...)
{
	if (!font)
		return;

	va_list va_alist;
	char szBuffer[1024];

	va_start(va_alist, fmt);
	auto len = vsprintf_s(szBuffer, fmt, va_alist);
	va_end(va_alist);

	auto strsize = MultiByteToWideChar(CP_UTF8, 0, szBuffer, strlen(szBuffer) + 1, nullptr, 0);
	auto pszStringWide = new wchar_t[strsize];
	MultiByteToWideChar(CP_UTF8, 0, szBuffer, strlen(szBuffer) + 1, pszStringWide, strsize);

	
	TextW(bAlign, font, x, y, r, g, b, a, pszStringWide);
	
	delete[] pszStringWide;
}

void CSurfaceDraw::Text(int x, int y, int r, int g, int b, int a, TextAlign bAlign, HFont font, const wchar_t *fmt, ...)
{
	if (!font)
		return;

	va_list va_alist;
	wchar_t wszBuffer[1024];

	va_start(va_alist, fmt);
	auto len = vswprintf_s(wszBuffer, fmt, va_alist);
	va_end(va_alist);

	
	TextW(bAlign, font, x, y, r, g, b, a, wszBuffer);
	
}

void CSurfaceDraw::DrawCircle(int x, int y, int radius, Color color, int a)
{
	if (a == -1)
		a = color.a();

	
	Interfaces::Surface->DrawSetColor(color.r(), color.g(), color.b(), a);
	for (int i = radius; i > 0; i--)
		Interfaces::Surface->DrawOutlinedCircle(x, y, i, 50);
	
}

void CSurfaceDraw::DrawLine(int x1, int y1, int x2, int y2, int r, int g, int b, int a) {
	
	Interfaces::Surface->DrawSetColor(r, g, b, a);
	Interfaces::Surface->DrawLine(x1, y1, x2, y2);
	
}

void CSurfaceDraw::DrawLine(int x1, int y1, int x2, int y2, Color color) {
	
	Interfaces::Surface->DrawSetColor(color.r(), color.g(), color.b(), color.a());
	Interfaces::Surface->DrawLine(x1, y1, x2, y2);
	
}

void  CSurfaceDraw::DrawLine(Vector p1, Vector p2, Color color)
{
	if (p1.z == -1337.f || p2.z == -1337.f) //failsafe
		return;

	
	Interfaces::Surface->DrawSetColor(color.r(), color.g(), color.b(), color.a());
	Interfaces::Surface->DrawLine(p1.x, p1.y, p2.x, p2.y);
	
}

void  CSurfaceDraw::DrawLine(Vector p1, Vector p2, int r, int g, int b, int a)
{
	if (p1.z == -1337.f || p2.z == -1337.f) //failsafe
		return;

	
	Interfaces::Surface->DrawSetColor(r, g, b, a);
	Interfaces::Surface->DrawLine(p1.x, p1.y, p2.x, p2.y);

}

void CSurfaceDraw::DrawArrow(int _x, int _y, int a) {

	//FillRGBA(_x - 4, _y - 2, 9, 1, 170, 170, 170, a);
	//FillRGBA(_x - 3, _y - 1, 7, 1, 170, 170, 170, a);
	FillRGBA(_x - 2, _y, 5, 1, 170, 170, 170, a);
	FillRGBA(_x - 1, _y + 1, 3, 1, 170, 170, 170, a);
	FillRGBA(_x, _y + 2, 1, 1, 170, 170, 170, a);

}

void CSurfaceDraw::Polygon(int x, int y, std::vector< Vertex_t > verts, float angle, int r, int g, int b, int a)
{
	if (!m_Texture)
		m_Texture = Interfaces::Surface->CreateNewTextureID(true);

	Color color;
	color.SetColor(255, 255, 255, 255);


	Interfaces::Surface->DrawSetTextureRGBA(m_Texture, color.base(), 1, 1);

	Interfaces::Surface->DrawSetColor(r, g, b, a);

	Interfaces::Surface->DrawSetTexture(m_Texture);

	for (auto& v : verts) {
		v.m_Position.x += x;
		v.m_Position.y += y;
	}

	for (auto& v : verts)
		v = RotateVertex(Vector2D(x, y), v, angle);

	Interfaces::Surface->DrawTexturedPolygon(verts.size(), verts.data());

}

void CSurfaceDraw::angle_direction_thirdperson()
{
	QAngle Fake = QAngle(0.f, g_Info.m_flChokedYaw, 0.f);
	Vector fake_dir;
	AngleVectors(Fake, &fake_dir);
	fake_dir *= 40.f;

	QAngle Real = QAngle(0.f, g_Info.m_flUnchokedYaw, 0.f);
	Vector real_dir;
	AngleVectors(Real, &real_dir);
	real_dir *= 40.f;

	QAngle Body = QAngle(0.f, LocalPlayer.LowerBodyYaw, 0.f);
	Vector body_dir;
	AngleVectors(Body, &body_dir);
	body_dir *= 40.f;

	Vector start = *LocalPlayer.Entity->GetAbsOrigin();

	Vector scr_start, scr_fake, scr_real, scr_body;
	if (!WorldToScreen(start, scr_start)
		|| !WorldToScreen(start + fake_dir, scr_fake)
		|| !WorldToScreen(start + real_dir, scr_real)
		|| !WorldToScreen(start + body_dir, scr_body))
	{
		return angle_direction_firstperson();
	}

	int w, h;
	Interfaces::EngineClient->GetScreenSize(w, h);

	auto is_out_of_bounds = [w, h](Vector const & in) -> bool
	{
		if (in.x > w || in.x < 0)
			return true;
		if (in.y > h || in.y < 0)
			return true;

		return false;
	};

	if (is_out_of_bounds(scr_start)
		|| is_out_of_bounds(scr_fake)
		|| is_out_of_bounds(scr_real)
		|| is_out_of_bounds(scr_body))
	{
		return angle_direction_firstperson();
	}

	
	DrawLine((int)scr_start.x, (int)scr_start.y, (int)scr_fake.x, (int)scr_fake.y, 0, 55, 255, 255);
	DrawLine((int)scr_start.x, (int)scr_start.y, (int)scr_real.x, (int)scr_real.y, 255, 20, 0, 255);
	//DrawLine((int)scr_start.x, (int)scr_start.y, (int)scr_body.x, (int)scr_body.y, 0, 55, 255, 255);
	
}

void CSurfaceDraw::angle_direction_firstperson()
{

	auto angletoscreen = [](float yaw, float distance)
	{
		Vector result;
		result.x = sinf((yaw / 180.f)*M_PI_F) * distance;
		result.y = -cosf((yaw / 180.f)*M_PI_F) * distance;
		return result;
	};

	auto drawrotatedarrow = [&](int x, int y, float scaleX, float scaleY, float side_90_scale, float rotation, int r, int g, int b, int a) -> void
	{
		Vector base, side_90;
		base = angletoscreen(rotation, 1.f);
		side_90 = angletoscreen(rotation + 90.f, 1.f);

		side_90 *= side_90_scale;

		Vector point_left_start_down;
		point_left_start_down.x = x + ((base.x + side_90.x) * scaleX);
		point_left_start_down.y = y + ((base.y + side_90.y) * scaleY);

		Vector point_right_start_down;
		point_right_start_down.x = x + ((base.x - side_90.x) * scaleX);
		point_right_start_down.y = y + ((base.y - side_90.y) * scaleY);

		Vector point_right_start_up;
		point_right_start_up.x = x + ((base.x - side_90.x) + (base.x)) * scaleX;
		point_right_start_up.y = y + ((base.y - side_90.y) + (base.y)) * scaleY;

		Vector point_left_start_up;
		point_left_start_up.x = x + ((base.x + side_90.x) + (base.x)) * scaleX;
		point_left_start_up.y = y + ((base.y + side_90.y) + (base.y)) * scaleY;

		Vector point_right_end_up;
		point_right_end_up.x = point_right_start_up.x - (side_90.x*scaleX);
		point_right_end_up.y = point_right_start_up.y - (side_90.y*scaleY);

		Vector point_left_end_up;
		point_left_end_up.x = point_left_start_up.x + (side_90.x*scaleX);
		point_left_end_up.y = point_left_start_up.y + (side_90.y*scaleY);

		Vector endpoint;
		endpoint.x = x + ((base.x*scaleX)*2.6f);
		endpoint.y = y + ((base.y*scaleY)*2.6f);

	
		DrawLine((int)point_left_start_down.x, (int)point_left_start_down.y, (int)point_right_start_down.x, (int)point_right_start_down.y, r, g, b, a);
		DrawLine((int)point_left_start_down.x, (int)point_left_start_down.y, (int)point_left_start_up.x, (int)point_left_start_up.y, r, g, b, a);
		DrawLine((int)point_right_start_down.x, (int)point_right_start_down.y, (int)point_right_start_up.x, (int)point_right_start_up.y, r, g, b, a);
		DrawLine((int)point_right_start_up.x, (int)point_right_start_up.y, (int)point_right_end_up.x, (int)point_right_end_up.y, r, g, b, a);
		DrawLine((int)point_left_start_up.x, (int)point_left_start_up.y, (int)point_left_end_up.x, (int)point_left_end_up.y, r, g, b, a);
		DrawLine((int)endpoint.x, (int)endpoint.y, (int)point_left_end_up.x, (int)point_left_end_up.y, r, g, b, a);
		DrawLine((int)endpoint.x, (int)endpoint.y, (int)point_right_end_up.x, (int)point_right_end_up.y, r, g, b, a);
	
	};
	int w, h;
	Interfaces::EngineClient->GetScreenSize(w, h);
	w /= 2; h /= 2;
	QAngle localView;
	Interfaces::EngineClient->GetViewAngles(localView);

	drawrotatedarrow(w, h, 30.f, 30.f, 0.2f, localView.y - g_Info.m_flChokedYaw, 10, 55, 255, 255);
	drawrotatedarrow(w, h, 30.f, 30.f, 0.2f, localView.y - g_Info.m_flUnchokedYaw, 255, 20, 10, 255);
	
	//drawrotatedarrow(w, h, 30.f, 30.f, 0.2f, localView.y - LocalPlayer.LowerBodyYaw, 10, 55, 255, 255);

}


#if 0
void CSurfaceDraw::angle_direction_thirdperson()
{
	QAngle Fake = QAngle(0.f, Global::AntiAim->GetFakeYaw(), 0.f);
	Vector fake_dir;
	AngleVectors(Fake, &fake_dir);
	fake_dir *= 40.f;

	QAngle Real = QAngle(0.f, Global::AntiAim->GetRealYaw(), 0.f);
	Vector real_dir;
	AngleVectors(Real, &real_dir);
	real_dir *= 40.f;

	QAngle Body = QAngle(0.f, LocalPlayer.LowerBodyYaw, 0.f);
	Vector body_dir;
	AngleVectors(Body, &body_dir);
	body_dir *= 40.f;

	Vector start = *LocalPlayer.Entity->GetAbsOrigin();

	Vector scr_start, scr_fake, scr_real, scr_body;
	if (!WorldToScreen(start, scr_start)
		|| !WorldToScreen(start + fake_dir, scr_fake)
		|| !WorldToScreen(start + real_dir, scr_real)
		|| !WorldToScreen(start + body_dir, scr_body))
	{
		return angle_direction_firstperson();
	}

	int w, h;
	Interfaces::EngineClient->GetScreenSize(w, h);

	auto is_out_of_bounds = [w, h](Vector const & in) -> bool
	{
		if (in.x > w || in.x < 0)
			return true;
		if (in.y > h || in.y < 0)
			return true;

		return false;
	};

	if (is_out_of_bounds(scr_start)
		|| is_out_of_bounds(scr_fake)
		|| is_out_of_bounds(scr_real)
		|| is_out_of_bounds(scr_body))
	{
		return angle_direction_firstperson();
	}

	DrawLine((int)scr_start.x, (int)scr_start.y, (int)scr_fake.x, (int)scr_fake.y, 0, 255, 20, 255);
	DrawLine((int)scr_start.x, (int)scr_start.y, (int)scr_real.x, (int)scr_real.y, 255, 20, 0, 255);
	DrawLine((int)scr_start.x, (int)scr_start.y, (int)scr_body.x, (int)scr_body.y, 0, 55, 255, 255);
}

void CSurfaceDraw::angle_direction_firstperson()
{

	auto angletoscreen = [](float yaw, float distance)
	{
		Vector result;
		result.x = sinf((yaw / 180.f)*M_PI_F) * distance;
		result.y = -cosf((yaw / 180.f)*M_PI_F) * distance;
		return result;
	};

	auto drawrotatedarrow = [&](int x, int y, float scaleX, float scaleY, float side_90_scale, float rotation, int r, int g, int b, int a) -> void
	{
		Vector base, side_90;
		base = angletoscreen(rotation, 1.f);
		side_90 = angletoscreen(rotation + 90.f, 1.f);

		side_90 *= side_90_scale;

		Vector point_left_start_down;
		point_left_start_down.x = x + ((base.x + side_90.x) * scaleX);
		point_left_start_down.y = y + ((base.y + side_90.y) * scaleY);

		Vector point_right_start_down;
		point_right_start_down.x = x + ((base.x - side_90.x) * scaleX);
		point_right_start_down.y = y + ((base.y - side_90.y) * scaleY);

		Vector point_right_start_up;
		point_right_start_up.x = x + ((base.x - side_90.x) + (base.x)) * scaleX;
		point_right_start_up.y = y + ((base.y - side_90.y) + (base.y)) * scaleY;

		Vector point_left_start_up;
		point_left_start_up.x = x + ((base.x + side_90.x) + (base.x)) * scaleX;
		point_left_start_up.y = y + ((base.y + side_90.y) + (base.y)) * scaleY;

		Vector point_right_end_up;
		point_right_end_up.x = point_right_start_up.x - (side_90.x*scaleX);
		point_right_end_up.y = point_right_start_up.y - (side_90.y*scaleY);

		Vector point_left_end_up;
		point_left_end_up.x = point_left_start_up.x + (side_90.x*scaleX);
		point_left_end_up.y = point_left_start_up.y + (side_90.y*scaleY);

		Vector endpoint;
		endpoint.x = x + ((base.x*scaleX)*2.6f);
		endpoint.y = y + ((base.y*scaleY)*2.6f);

		DrawLine((int)point_left_start_down.x, (int)point_left_start_down.y, (int)point_right_start_down.x, (int)point_right_start_down.y, r, g, b, a);
		DrawLine((int)point_left_start_down.x, (int)point_left_start_down.y, (int)point_left_start_up.x, (int)point_left_start_up.y, r, g, b, a);
		DrawLine((int)point_right_start_down.x, (int)point_right_start_down.y, (int)point_right_start_up.x, (int)point_right_start_up.y, r, g, b, a);
		DrawLine((int)point_right_start_up.x, (int)point_right_start_up.y, (int)point_right_end_up.x, (int)point_right_end_up.y, r, g, b, a);
		DrawLine((int)point_left_start_up.x, (int)point_left_start_up.y, (int)point_left_end_up.x, (int)point_left_end_up.y, r, g, b, a);
		DrawLine((int)endpoint.x, (int)endpoint.y, (int)point_left_end_up.x, (int)point_left_end_up.y, r, g, b, a);
		DrawLine((int)endpoint.x, (int)endpoint.y, (int)point_right_end_up.x, (int)point_right_end_up.y, r, g, b, a);
	};
	int w, h;
	Interfaces::EngineClient->GetScreenSize(w, h);
	w /= 2; h /= 2;
	QAngle localView;
	Interfaces::EngineClient->GetViewAngles(localView);

	drawrotatedarrow(w, h, 30.f, 30.f, 0.2f, localView.y - Global::AntiAim->GetFakeYaw(), 10, 255, 20, 255);
	drawrotatedarrow(w, h, 30.f, 30.f, 0.2f, localView.y - Global::AntiAim->GetRealYaw(), 255, 20, 10, 255);
	drawrotatedarrow(w, h, 30.f, 30.f, 0.2f, localView.y - LocalPlayer.LowerBodyYaw, 10, 55, 255, 255);
}
#endif