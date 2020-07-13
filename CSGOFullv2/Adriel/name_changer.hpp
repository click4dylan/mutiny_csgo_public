#pragma once
#include "stdafx.hpp"

class name_changer : public singleton<name_changer>
{
private:
	static void set_name(const char* name);

public:
	name_changer();
	~name_changer();

	static void create_move();
};