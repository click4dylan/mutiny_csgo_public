#pragma once

#include "Includes.h"

class CSurfaceDraw
{
public:

	enum TextAlign {
		LEFT,
		CENTER,
		RIGHT
	};

	int m_Texture = 0;

	void Update();
	void BuildFonts();
	void GradientVertical(int x, int y, int w, int h, int from_r, int from_g, int from_b, int from_a, int to_r, int to_g, int to_b, int to_a);
	void GradientHorizontal(int x, int y, int w, int h, int from_r, int from_g, int from_b, int from_a, int to_r, int to_g, int to_b, int to_a);
	void DrawMouse(int x, int y);
	void DrawMouse(POINT pos);
	void FillRGBA(int x, int y, int w, int h, int r, int g, int b, int a);
	void FillRGBA(int x, int y, int w, int h, Color color);
	void FillRGBA(int x, int y, int w, int h, Color color, int a);
	void Border(int x, int y, int w, int h, int line, int r, int g, int b, int a);
	void ESPBox(int x, int y, int w, int h, int borderPx, int percentW, int percentH, Color clr);
	void CornerBox(int x, int y, int w, int h, int borderPx, int r, int g, int b, int a);
	void HealthbarHorizontal(int x, int y, int w, int h, int EntityHealth, bool text, int alpha = 255);
	void Healthbar(int x, int y, int w, int h, int EntityHealth, bool text, int alpha = 255);
	void DrawBar(int x, int y, int w, int h, float _value, float _max, Color clr, bool _drawText = false, std::string _text = "", int alpha = 255);
	void DrawBar(int x, int y, int w, int h, int _value, int _max, Color clr, bool _drawText = false, std::string _text = "", int alpha = 255);
	void DrawBarHorizontal(int x, int y, int w, int h, float _value, float _max, Color clr, bool _drawText = false, std::string _text = "", int alpha = 255);
	void DrawBarHorizontal(int x, int y, int w, int h, int _value, int _max, Color clr, bool _drawText = false, std::string _text = "", int alpha = 255);
	void Text(int x, int y, int r, int g, int b, int a, TextAlign bCenter, unsigned long font, const char *fmt, ...);
	void Text(int x, int y, int r, int g, int b, int a, TextAlign bCenter, unsigned long font, const wchar_t *fmt, ...);
	void DrawCircle(int x, int y, int radius, Color color, int a = -1);
	void DrawLine(int x1, int y1, int x2, int y2, int r, int g, int b, int a);
	void DrawLine(int x1, int y1, int x2, int y2, Color color);
	void DrawLine(Vector p1, Vector p2, Color color);
	void DrawLine(Vector p1, Vector p2, int r, int g, int b, int a);
	void DrawArrow(int x, int y, int a);
	void Polygon(int x, int y, std::vector< Vertex_t > verts, float angle, int r, int g, int b, int a);
	void angle_direction_firstperson();
	void angle_direction_thirdperson();
};

extern CSurfaceDraw g_Draw;
