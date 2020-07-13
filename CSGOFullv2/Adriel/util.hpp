#pragma once

#include "stdafx.hpp"
#include "../misc.h"
#undef clamp

namespace adr_util
{
	std::string get_disk();
	std::pair<int, int> get_window_size(HWND h_wnd);
	time_t get_epoch_time();
	bool check_one_instance();

	std::string get_key_name_by_id(int id);
	std::string get_time();
	std::string date_to_string(tm date);

	int random_int(int i_start, int i_end);
	float random_float(float i_start, float i_end);

	Color get_rainbow_color(float speed);
	Color get_health_color(int hp);

	template <typename T>
	T clamp(const T& n, const T& lower, const T& upper)
	{
		return max(lower, min(n, upper));
	}

	class string : public std::string
	{
	public:
		static bool is_number(const std::string& s);
		static void split(const std::string& s, char delim, std::vector<std::string>& elems);
		static std::vector<std::string> split(const std::string& s, char delim);

		static bool contains(const std::string& word, const std::string& sentence);
		static std::string replace(std::string text, const std::string& find, const std::string& replace);
		static std::string pad_right(std::string text, size_t value);
		static std::string wstring_to_string(std::wstring wstr);
		static std::wstring string_to_wstring(std::string str);

		static std::string random(int size);
		static std::string to_lower(std::string str);
		static std::string to_upper(std::string str);

		template<typename ... args>
		static std::string format(const std::string& format, args ... arg)
		{
			const size_t size = std::snprintf(nullptr, 0, format.c_str(), arg ...) + 1;
			std::unique_ptr<char[]> buf(new char[size]);
			std::snprintf(buf.get(), size, format.c_str(), arg ...);
			return std::string(buf.get(), buf.get() + size - 1);
		}
	};
}
