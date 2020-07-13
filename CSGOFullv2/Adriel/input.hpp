#pragma once
#include "stdafx.hpp"

enum class key_state
{
	NONE = 1,
	DOWN,
	UP,
	PRESSED /*Down and then up*/
};
DEFINE_ENUM_FLAG_OPERATORS(key_state)

class input	: public singleton<input>
{
private:
	static LRESULT WINAPI wnd_proc(HWND h_wnd, UINT msg, WPARAM w_param, LPARAM l_param);
	bool process_message(UINT u_msg, WPARAM w_param, LPARAM l_param);
	bool process_mouse_message(UINT u_msg, WPARAM w_param, LPARAM l_param);
	bool process_keybd_message(UINT u_msg, WPARAM w_param, LPARAM l_param);

	HWND m_hTargetWindow{};
	LONG_PTR m_ulOldWndProc{};
	key_state m_iKeyMap[255]{};
	std::function<void(void)> m_Hotkeys[255];

public:
	input();
	~input();

	void remove();

	void initialize();
	HWND get_main_window() const;

	key_state get_key_state(uint32_t vk);

	void reset_key_map();

	bool is_key_down(uint32_t vk);
	bool was_key_pressed(uint32_t vk);

	void register_hotkey(uint32_t vk, std::function<void(void)> f);
	void remove_hotkey(uint32_t vk);
};