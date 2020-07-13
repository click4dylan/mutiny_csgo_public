#include "precompiled.h"
#include "Draw.h"
#include "CustomFont.h"

// Font List
HFont ESPFONT;
HFont ESPNumberFont;
HFont WeaponIcon;

// Textures
IMaterial* visible_tex;
IMaterial* hidden_tex;
IMaterial* visible_flat;
IMaterial* hidden_flat;


void SetupTextures()
{

}
void SetupFonts()
{
	// TODO : ENCRYPT
	// Setup Fonts allowing custom fonts for text
	Interfaces::Surface->SetFontGlyphSet(ESPFONT = Interfaces::Surface->Create_Font(), "Tahoma", 12, FW_EXTRABOLD, NULL, NULL, FONTFLAG_OUTLINE);
	Interfaces::Surface->SetFontGlyphSet(ESPNumberFont = Interfaces::Surface->Create_Font(), "Tahoma", 13, FW_LIGHT, NULL, NULL, FONTFLAG_OUTLINE | FONTFLAG_DROPSHADOW);
	Interfaces::Surface->SetFontGlyphSet(WeaponIcon = Interfaces::Surface->Create_Font(), "Arial", 18, 300, 0, 0, 0x210);
}
void DrawFilledRect(int x, int y, int w, int h, Color col)
{
	int r = 255, g = 255, b = 255, a = 255;
	col.GetColor(r, g, b, a);
	Interfaces::Surface->DrawSetColor(r, g, b, a);
	Interfaces::Surface->DrawFilledRect(x, y, x + w, y + h);
}
void DrawLine(int x0, int y0, int x1, int y1, Color col)
{
	Interfaces::Surface->DrawSetColor(col);
	Interfaces::Surface->DrawLine(x0, y0, x1, y1);
}
void DrawRect(int x, int y, int w, int h, Color col)
{
	Interfaces::Surface->DrawSetColor(col);
	Interfaces::Surface->DrawRect(x, y, x + w, y + h);
}
void DrawOutlinedRect(int x, int y, int w, int h, Color col)
{
	Interfaces::Surface->DrawSetColor(col);
	Interfaces::Surface->DrawOutlinedRect(x, y, x + w, y + h);
}
void DrawPixel(int x, int y, Color col)
{
	Interfaces::Surface->DrawSetColor(col);
	Interfaces::Surface->DrawRect(x, y, x + 1, y + 1);
}
void GetTextSize(unsigned long& Font, int& w, int& h, const char* strText, ...) 
{
	char buf[1024];
	wchar_t wbuf[1024];

	va_list vlist;
	va_start(vlist, strText);
	vsprintf(buf, strText, vlist);
	va_end(vlist);

	MultiByteToWideChar(CP_UTF8, 0, buf, 256, wbuf, 256);
	Interfaces::Surface->GetTextSize(Font, wbuf, w, h);
}

void DrawStringOutlined(unsigned long& Font, Vector2D pos, Color c, unsigned int flags, const char* strText, ...)
{
	wchar_t formated[128] = { '\0' };
	wsprintfW(formated, L"%S", strText);
	char buf[1024];
	wchar_t wbuf[1024];

	va_list vlist;
	va_start(vlist, strText);
	vsprintf(buf, strText, vlist);

	MultiByteToWideChar(CP_UTF8, 0, buf, 256, wbuf, 256);

	int w, h;
	GetTextSize(Font, w, h, strText, vlist);
	va_end(vlist);

	if (flags & 1)
		pos.x -= w / 2.0f;
	if (flags & 2)
		pos.y -= h / 2.0f;

	Interfaces::Surface->DrawSetTextFont(Font);
	Interfaces::Surface->DrawSetTextColor(0, 0, 0, 255);

	Interfaces::Surface->DrawSetTextPos(pos.x + 1, pos.y + 1);
	Interfaces::Surface->DrawPrintText(wbuf, wcslen(wbuf));
	Interfaces::Surface->DrawSetTextPos(pos.x - 1, pos.y + 1);
	Interfaces::Surface->DrawPrintText(wbuf, wcslen(wbuf));
	Interfaces::Surface->DrawSetTextPos(pos.x + 1, pos.y - 1);
	Interfaces::Surface->DrawPrintText(wbuf, wcslen(wbuf));
	Interfaces::Surface->DrawSetTextPos(pos.x - 1, pos.y - 1);
	Interfaces::Surface->DrawPrintText(wbuf, wcslen(wbuf));

	Interfaces::Surface->DrawSetTextColor(c.r(), c.g(), c.b(), c.a());
	Interfaces::Surface->DrawSetTextPos(pos.x, pos.y);
	Interfaces::Surface->DrawPrintText(wbuf, wcslen(wbuf));
}
void FillRGBA(int x, int y, int w, int h, Color col)
{
	Interfaces::Surface->DrawSetColor(col);
	Interfaces::Surface->DrawFilledRect(x, y, x + w, y + h);
}
void DrawCircle(float x, float y, float r, float s, Color color)
{
	float Step = M_PI * 2.0 / s;
	for (float a = 0; a < (M_PI*2.0); a += Step)
	{
		float x1 = r * cos(a) + x;
		float y1 = r * sin(a) + y;
		float x2 = r * cos(a + Step) + x;
		float y2 = r * sin(a + Step) + y;

		DrawLine(x1, y1, x2, y2, color);
	}
}
void DrawString(HFont font, int x, int y, Color color, DWORD alignment, const char* msg, ...)
{
	va_list va_alist;
	char buf[1024];
	va_start(va_alist, msg);
	_vsnprintf(buf, sizeof(buf), msg, va_alist);
	va_end(va_alist);
	wchar_t wbuf[1024];
	MultiByteToWideChar(CP_UTF8, 0, buf, 256, wbuf, 256);

	int r = 255, g = 255, b = 255, a = 255;
	color.GetColor(r, g, b, a);

	int width, height;
	Interfaces::Surface->GetTextSize(font, wbuf, width, height);

	if (alignment & FONT_RIGHT)
		x -= width;
	if (alignment & FONT_CENTER)
		x -= width / 2;

	Interfaces::Surface->DrawSetTextFont(font);
	Interfaces::Surface->DrawSetTextColor(r, g, b, a);
	Interfaces::Surface->DrawSetTextPos(x, y - height / 2);
	Interfaces::Surface->DrawPrintText(wbuf, wcslen(wbuf));
}
