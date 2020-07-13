#include "custom_def.hpp"
#include "util.hpp"
#include "input.hpp"
#include "ui.hpp"

color_var::color_var()
{
	col_color = ImColor(0.f, 0.f, 0.f, 1.f);
	i_mode = 0;
	f_rainbow_speed = 0.f;
}

color_var::color_var(ImColor col, int mode, float speed)
{
	col_color = col;
	i_mode = mode;
	f_rainbow_speed = speed;
}

color_var::~color_var()
{
}

Color color_var::color() const
{
	Color col_result = { static_cast<int>(col_color.Value.x * 255), static_cast<int>(col_color.Value.y * 255), static_cast<int>(col_color.Value.z * 255), static_cast<int>(col_color.Value.w * 255) };
	
	if (i_mode == 1)
	{
		col_result = adr_util::get_rainbow_color(f_rainbow_speed);
		col_result.SetAlpha(static_cast<int>(col_color.Value.w * 255));
	}

	return col_result;
}

health_color_var::health_color_var()
{
	col_color = ImColor(0.f, 0.f, 0.f, 1.f);
	i_mode = 0;
	f_rainbow_speed = 0.f;
}

health_color_var::health_color_var(ImColor col, int mode, float speed)
{
	col_color = col;
	i_mode = mode;
	f_rainbow_speed = speed;
}

health_color_var::~health_color_var()
{
}

Color health_color_var::color(int hp) const
{
	Color col_result = { static_cast<int>(col_color.Value.x * 255), static_cast<int>(col_color.Value.y * 255), static_cast<int>(col_color.Value.z * 255), static_cast<int>(col_color.Value.w * 255) };
	
	if (i_mode == 2)
	{
		col_result = adr_util::get_rainbow_color(f_rainbow_speed);
		col_result.SetAlpha(static_cast<int>(col_color.Value.w * 255));
	}
	else if (i_mode == 1)
	{
		col_result = adr_util::get_health_color(hp);
		col_result.SetAlpha(static_cast<int>(col_color.Value.w * 255));
	}

	return col_result;
}

bool_sw::bool_sw()
{
}

bool_sw::bool_sw(bool state, int mode, int key)
{
	b_state = state;
	i_mode = mode;
	i_key = key;
}

bool_sw::~bool_sw()
{
}

bool bool_sw::get()
{
	if (ui::get().is_visible())
		return b_state;

	if (i_mode == 1)
	{
		if (input::get().is_key_down(i_key))
			b_state = true;
		else
			b_state = false;
	}
	else if (i_mode == 2)
	{
		if (input::get().was_key_pressed(i_key))
			b_state = !b_state;
	}

	return b_state;
}