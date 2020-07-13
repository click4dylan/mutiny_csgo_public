#include "precompiled.h"
#include "VTHook.h"
#include "threadtools.h"

void ISurface::DrawSetColor(int r, int g, int b, int a)
{
	typedef void(__thiscall* OriginalFn)(void*, int, int, int, int);
	GetVFunc<OriginalFn>(this, 15)(this, r, g, b, a);
}

void ISurface::DrawSetColor(Color col)
{
	typedef void(__thiscall* OriginalFn)(void*, Color);
	GetVFunc<OriginalFn>(this, 14)(this, col);
}
void ISurface::DrawRect(int x0, int y0, int x1, int y1)
{
	typedef void(__thiscall* OriginalFn)(void*, int, int, int, int);
	GetVFunc<OriginalFn>(this, 16)(this, x0, y0, x1, y1);
}

void ISurface::DrawFilledRect(int x0, int y0, int x1, int y1)
{
	typedef void(__thiscall* OriginalFn)(void*, int, int, int, int);
	GetVFunc<OriginalFn>(this, 16)(this, x0, y0, x1, y1);
}

void ISurface::DrawOutlinedRect(int x0, int y0, int x1, int y1)
{
	typedef void(__thiscall* OriginalFn)(void*, int, int, int, int);
	GetVFunc<OriginalFn>(this, 18)(this, x0, y0, x1, y1);
}

void ISurface::DrawLine(int x0, int y0, int x1, int y1)
{
	typedef void(__thiscall* OriginalFn)(void*, int, int, int, int);
	GetVFunc<OriginalFn>(this, 19)(this, x0, y0, x1, y1);
}

void ISurface::DrawPolyLine(int *px, int *py, int numPoints)
{
	typedef void(__thiscall* OriginalFn)(void*, int*, int*, int);
	GetVFunc<OriginalFn>(this, 20)(this, px, py, numPoints);
}

void ISurface::DrawSetTextFont(HFont font)
{
	typedef void(__thiscall* OriginalFn)(void*, HFont);
	GetVFunc<OriginalFn>(this, 23)(this, font);
}

void ISurface::DrawSetTextColor(int r, int g, int b, int a)
{
	typedef void(__thiscall* OriginalFn)(void*, int, int, int, int);
	GetVFunc<OriginalFn>(this, 25)(this, r, g, b, a);
}

void ISurface::DrawSetTextColor(Color col)
{
	typedef void(__thiscall* OriginalFn)(void*, Color);
	GetVFunc<OriginalFn>(this, 24)(this, col);
}

void ISurface::DrawSetTextPos(int x, int y)
{
	typedef void(__thiscall* OriginalFn)(void*, int, int);
	GetVFunc<OriginalFn>(this, 26)(this, x, y);
}

void ISurface::DrawPrintText(const wchar_t *text, int textLen, FontDrawType_t drawType)
{
	typedef void(__thiscall* OriginalFn)(void*, const wchar_t*, int, FontDrawType_t);
	GetVFunc<OriginalFn>(this, 28)(this, text, textLen, drawType);
}

void ISurface::DrawSetTextureRGBA(int id, const unsigned char *rgba, int wide, int tall)
{
	typedef void(__thiscall* OriginalFn)(void*, int, const unsigned char*, int, int);
	GetVFunc<OriginalFn>(this, 37)(this, id, rgba, wide, tall);
}

void ISurface::DrawSetTexture(int id)
{
	typedef void(__thiscall* OriginalFn)(void*, int);
	GetVFunc<OriginalFn>(this, 38)(this, id);
}

int	ISurface::CreateNewTextureID(bool procedural)
{
	typedef int(__thiscall* OriginalFn)(void*, bool);
	int ret = GetVFunc<OriginalFn>(this, 43)(this, procedural);
	return ret;
}

HFont ISurface::Create_Font()
{
	typedef HFont(__thiscall* OriginalFn)(void*);
	auto font =  GetVFunc<OriginalFn>(this, 71)(this);
	return font;
}

bool ISurface::SetFontGlyphSet(HFont font, const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags, int nRangeMin, int nRangeMax)
{
	typedef bool(__thiscall* OriginalFn)(void*, HFont, const char*, int, int, int, int, int, int, int);
	auto ret = GetVFunc<OriginalFn>(this, 72)(this, font, windowsFontName, tall, weight, blur, scanlines, flags, nRangeMin, nRangeMax);
	return ret;
}

void ISurface::GetTextSize(HFont font, const wchar_t *text, int &wide, int &tall)
{
	typedef void(__thiscall* OriginalFn)(void*, HFont, const wchar_t*, int&, int&);
	GetVFunc<OriginalFn>(this, 79)(this, font, text, wide, tall);
}

void ISurface::DrawOutlinedCircle(int x, int y, int radius, int segments)
{
	typedef void(__thiscall* OriginalFn)(void*, int, int, int, int);
	GetVFunc<OriginalFn>(this, 103)(this, x, y, radius, segments);
}

void ISurface::DrawTexturedPolygon(int n, Vertex_t *pVertice, bool bClipVertices)
{
	typedef void(__thiscall* OriginalFn)(void*, int, Vertex_t*, bool);
	GetVFunc<OriginalFn>(this, 106)(this, n, pVertice, bClipVertices);
}

int ISurface::GetFontTall(HFont font)
{
	typedef int(__thiscall* OriginalFn)(void*, HFont);
	auto ret = GetVFunc<OriginalFn>(this, 74)(this, font);
	return ret;
}

void ISurface::Play_Sound(const char* pSample)
{
	typedef void(__thiscall* PlaySound_t)(void*, const char*);
	GetVFunc<PlaySound_t>(this, 82)(this, pSample);
}

