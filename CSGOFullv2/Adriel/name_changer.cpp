#include "name_changer.hpp"

#include "../LocalPlayer.h"
#include "adr_util.hpp"

name_changer::name_changer() = default;

name_changer::~name_changer() = default;

void name_changer::set_name(const char* name)
{
	//decrypts(0)
	static auto cvar_name = Interfaces::Cvar->FindVar(XorStr("name"));
	//encrypts(0)
	*reinterpret_cast<int*>(reinterpret_cast<DWORD>(&cvar_name->fnChangeCallback) + 0xC) = NULL;
	cvar_name->SetValue(name);
}

void name_changer::create_move()
{
	static auto i_changes = 0;
	static std::string str_old_name = "";

	if (!LocalPlayer.Entity)
		return;

	if (!variable::get().misc.namechanger.b_enabled)
	{
		if (str_old_name.empty())
		{
			player_info_t p_info {};
			LocalPlayer.Entity->GetPlayerInfo(&p_info);

			str_old_name = adr_util::sanitize_name(p_info.name);
		}

		if (i_changes >= 5)
			i_changes = 0;

		return;
	}

	const auto current_time_ms = adr_util::get_epoch_time();
	static auto time_stamp = current_time_ms;
	if (current_time_ms - time_stamp < 150)
		return;

	time_stamp = current_time_ms;
	++i_changes;

	if (i_changes >= 5)
	{
		if (variable::get().misc.namechanger.i_mode == 0)
			set_name(str_old_name.c_str());

		variable::get().misc.namechanger.b_enabled = false;
		return;
	}

	if (variable::get().misc.namechanger.i_mode == 0)
	{
		//decrypts(0)
		set_name(adr_util::string::pad_right(XorStr("-> mutiny.pw <-"), strlen(XorStr("-> mutiny.pw <-")) + i_changes).c_str());
		//encrypts(0)
	}
	else if(variable::get().misc.namechanger.i_mode == 1)
	{
		set_name(XorStrCT(" \xe2\x80\xa8")); // some p paragraph here, thx volvo for no fixerino
	}
	else
	{
		std::string name = XorStrCT(" \xE2\x80\xAE") + str_old_name;
		set_name(name.data());
	}
}
