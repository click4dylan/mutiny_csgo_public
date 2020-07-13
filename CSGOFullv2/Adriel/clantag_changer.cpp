#include "stdafx.hpp"
#include "clantag_changer.hpp"
#include "variable.hpp"
#include "adr_util.hpp"

#include "../LocalPlayer.h"
#include "../SetClanTag.h"

clantag_changer::clantag_changer() = default;

clantag_changer::~clantag_changer() = default;

text_animation clantag_changer::marquee(std::string text, int width = 15) const
{
	text.erase(std::remove(text.begin(), text.end(), '\0'), text.end());
	const auto crop_string = std::string(width, ' ') + text + std::string(width - 1, ' ');

	std::vector<std::string> frames;
	for (unsigned long i = 0; i < text.length() + width; i++)
		frames.push_back(crop_string.substr(i, width + i));

	return text_animation(frames);
}

text_animation clantag_changer::words(std::string text)
{
	auto words = adr_util::string::split(text, ' ');
	std::vector<std::string> frames;
	for (const auto& word : words)
		frames.push_back(word);

	return text_animation(frames);
}

text_animation clantag_changer::letters(std::string text)
{
	std::vector<std::string> frames;
	for (unsigned long i = 1; i <= text.length(); i++)
		frames.push_back(text.substr(0, i));

	for (unsigned long i = text.length() - 2; i > 0; i--)
		frames.push_back(frames[i]);

	return text_animation(frames);
}

void clantag_changer::set(const char* str)
{
	//static auto v_func = reinterpret_cast<void(__fastcall *)(const char*, const char*)>(memory::pattern_scan_ida("engine.dll", "53 56 57 8B DA 8B F9 FF 15"));
	//static auto str_gp = util::string::random(10).c_str();
	//v_func(str, str_gp);

	SetClanTag(str, adr_util::string::random(1).c_str());
}

void clantag_changer::create_move()
{
	if (!CurrentUserCmd.bSendPacket)
		return;

	if (!LocalPlayer.Entity)
		return;

	auto& var = variable::get();

	if (!var.misc.clantag.b_enabled.get())
	{
		if (b_was_on)
		{
			set("");

			//decrypts(0)
			str_old_text = XorStr("129037hfsjiadnf9812wedcazsaxc");
			//encrypts(0)
			i_old_anim = -1;
			b_set = false;

			b_was_on = false;
		}
		return;
	}
	b_was_on = true;

	std::string txt{};
#if defined _DEBUG || defined INTERNAL_DEBUG
	//decrypts(0)
	txt = XorStr("mutiny.pw dev");
	//encrypts(0)
#elif defined STAFF
	//decrypts(0)
	txt = XorStr("mutiny.pw staff");
	//encrypts(0)
#else
	//decrypts(0)
	txt = XorStr("mutiny.pw beta");
	//encrypts(0)
#endif

#if !defined(_DEBUG) && !defined(INTERNAL_DEBUG)
	if (var.misc.clantag.i_anim != 0)
	{
		//don't allow dev tag or staff tag for public users
#ifdef STAFF
		//decrypts(0)
		if (var.misc.clantag.str_text == XorStr("mutiny.pw dev"))
			var.misc.clantag.str_text = txt;
		//encrypts(0)
#else
		//decrypts(0)
		if (var.misc.clantag.str_text == XorStr("mutiny.pw dev"))
			var.misc.clantag.str_text = txt;
		else if (var.misc.clantag.str_text == XorStr("mutiny.pw staff"))
			var.misc.clantag.str_text = txt;
		//encrypts(0)
#endif
	}
#endif

	auto str_text = var.misc.clantag.i_anim == 0 ? txt : var.misc.clantag.str_text;
	if (str_text.empty())//|| str_text.size() < 2)
	{
		b_set = false;
		return;
	}

	if ((str_old_text != str_text || i_old_anim != variable::get().misc.clantag.i_anim) /*&& Interfaces::Globals->realtime > m_next_clantag_time*/)
	{
		str_old_text = str_text;
		i_old_anim = variable::get().misc.clantag.i_anim;
		b_set = true;
	}

	// if static or custom
	if (var.misc.clantag.i_anim == 0 || var.misc.clantag.i_anim == 1)
	{
		if (b_set)
		{
			set(str_text.c_str());
			b_set = false;
		}
	}
}

void clantag_changer::clear()
{
	if (b_was_on)
	{
		set("");

		//decrypts(0)
		str_old_text = XorStr("129037hfsjiadnf9812wedcazsaxc");
		//encrypts(0)
		i_old_anim = -1;
		m_next_clantag_time = FLT_MIN;
		b_set = false;
		b_was_on = false;
	}
}
