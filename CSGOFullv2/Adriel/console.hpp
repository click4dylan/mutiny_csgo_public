#pragma once
#include "stdafx.hpp"

class console : public singleton<console>
{
private:
	bool b_visible;

public:
	console();
	~console();

	void initialize();

	static void set_lines_columns(int i_width, int i_height, int x, int y);
	static std::pair<int, int> get_lines_columns();

	void close();
	void hide();
	void show();
	bool is_visible() const;

	static void clear();
	static void freeconsole();
};

enum
{
	LEMPTY,
	LSUCCESS,
	LDEBUG,
	LWARN,
	LERROR,
	LCRITICAL,
	LMAX
};

namespace logger
{
	extern std::shared_timed_mutex m_mutex;
	template<typename ... arg> void add(const int type, const std::string& format, arg ... a)
	{
#if defined _DEBUG || defined CONSOLE
		auto get_time = []() -> std::string
		{
			time_t current_time;
			time(&current_time);

			struct tm time_info;
			localtime_s(&time_info, &current_time);

			std::ostringstream oss;
			oss << std::put_time(&time_info, "%d/%m/%Y %H:%M:%S");
			return oss.str();
		};

		std::unique_lock<decltype(m_mutex)> lock(m_mutex);

		const size_t size = std::snprintf(nullptr, 0, format.c_str(), a ...) + 1;
		std::unique_ptr<char[]> buf(new char[size]);
		std::snprintf(buf.get(), size, format.c_str(), a ...);
		const auto str_formated = std::string(buf.get(), buf.get() + size - 1);

		static std::vector<std::pair<WORD, std::string>> tag =
		{
			{
				0,
				""
			},
			{
				10,
				"SUCCESS"
			},
			{
				9,
				"DEBUG"
			},
			{
				14,
				"WARN"
			},
			{
				12,
				"ERROR"
			},
			{
				BACKGROUND_RED | BACKGROUND_INTENSITY,
				"CRITICAL"
			},
		};

		if (console::get().is_visible())
		{
			static auto h_console = GetStdHandle(STD_OUTPUT_HANDLE);
			SetConsoleTextAttribute(h_console, 15);
			std::cout << "[ ";
			SetConsoleTextAttribute(h_console, 11);
			std::cout << get_time();
			SetConsoleTextAttribute(h_console, 15);
			std::cout << " ] - ";
			if (type > LEMPTY && type < LMAX)
			{
				SetConsoleTextAttribute(h_console, 15);
				std::cout << "[ ";
				SetConsoleTextAttribute(h_console, tag[type].first);
				std::cout << tag[type].second.c_str();
				SetConsoleTextAttribute(h_console, 15);
				std::cout << " ] - ";
			}
			std::cout << str_formated << std::endl;
		}
#endif
	}
}