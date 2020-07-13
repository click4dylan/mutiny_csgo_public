#pragma once

#include "../misc.h"

class color_var
{
public:
	ImColor col_color = ImColor(0.f, 0.f, 0.f, 1.f);
	int i_mode = 0;
	float f_rainbow_speed = 0.f;

	color_var();
	color_var(ImColor col, int mode = 0, float speed = 0.f);

	~color_var();
	Color color() const;
};

class health_color_var
{
public:
	ImColor col_color = ImColor(0.f, 0.f, 0.f, 1.f);
	int i_mode = 0;
	float f_rainbow_speed = 0.f;

	health_color_var();
	health_color_var(ImColor col, int mode = 0, float speed = 0.f);

	~health_color_var();
	Color color(int hp) const;
};

class bool_sw
{
public:
	bool b_state = false;
	int i_mode = 0;
	int i_key = 0;

	bool_sw();
	explicit bool_sw(bool state, int mode = 0, int key = 0);

	~bool_sw();
	bool get();
};