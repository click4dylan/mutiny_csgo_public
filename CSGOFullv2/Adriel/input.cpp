#include "input.hpp"
#include "console.hpp"
#include "../string_encrypt_include.h"
#include "ui.hpp"
#include "../VMProtectDefs.h"

//#include "ui.hpp"

LRESULT ImGui_ImplDX9_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui::GetCurrentContext() == nullptr)
		return 0;

	ImGuiIO& io = ImGui::GetIO();
	switch (msg)
	{
	case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
	{
		int button = 0;
		if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) button = 0;
		if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK) button = 1;
		if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK) button = 2;
		if (!ImGui::IsAnyMouseDown() && ::GetCapture() == nullptr)
			::SetCapture(hwnd);

		io.MouseDown[button] = true;
		return 1;
	}
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	{
		int button = 0;
		if (msg == WM_LBUTTONUP) button = 0;
		if (msg == WM_RBUTTONUP) button = 1;
		if (msg == WM_MBUTTONUP) button = 2;
		io.MouseDown[button] = false;
		if (!ImGui::IsAnyMouseDown() && ::GetCapture() == hwnd)
			::ReleaseCapture();

		return 1;
	}
	case WM_MOUSEWHEEL:
		io.MouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0f : -1.0f;
		return 1;
	case WM_MOUSEHWHEEL:
		io.MouseWheelH += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0f : -1.0f;
		return 1;
	case WM_MOUSEMOVE:
		io.MousePos.x = (signed short)(lParam);
		io.MousePos.y = (signed short)(lParam >> 16);
		return 1;
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if (wParam < 256)
			io.KeysDown[wParam] = true;
		return 1;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		if (wParam < 256)
			io.KeysDown[wParam] = false;
		return 1;
	case WM_CHAR:
		// You can also use ToAscii()+GetKeyboardState() to retrieve characters.
		if (wParam > 0 && wParam < 0x10000)
			io.AddInputCharacter((unsigned short)wParam);
		return 1;
	}
	return 0;
}

input::input() = default;

input::~input()
{
	remove();
}

void input::remove()
{
	VMP_BEGINMUTILATION("input # remove");
	if (m_ulOldWndProc)
	{
		SetWindowLongPtr(m_hTargetWindow, GWLP_WNDPROC, m_ulOldWndProc);
		//logger::add(LWARN, "[ Input ] Restored Window Ptr.");
	}

	m_ulOldWndProc = 0;
	VMP_END;
}

void input::initialize()
{
	VMP_BEGINMUTILATION("input # initialize");
	//decrypts(0)
	while (!((m_hTargetWindow = FindWindowA(XorStr("Valve001"), nullptr))))
	{
		//decrypts(1)
		logger::add(LWARN, XorStr("[ Input ] Looking for Valve001 window."));
		//encrypts(1)
		std::this_thread::sleep_for(std::chrono::milliseconds(1500));
	}
	//encrypts(0)

	m_ulOldWndProc = SetWindowLongPtr(m_hTargetWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(wnd_proc));
	if (!m_ulOldWndProc)
	{
		//decrypts(0)
		logger::add(LCRITICAL, XorStr("[ Input ] SetWindowLongPtr failed!"));
		//encrypts(0)
	}

	for (auto i = 0; i < 255; i++)
		m_iKeyMap[i] = key_state::NONE;

	VMP_END;
}

HWND input::get_main_window() const
{
	return m_hTargetWindow;
}

LRESULT __stdcall input::wnd_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	get().process_message(msg, wParam, lParam);

	if (ui::get().is_visible() && ImGui_ImplDX9_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	return CallWindowProcW((WNDPROC)get().m_ulOldWndProc, hWnd, msg, wParam, lParam);
}

bool input::process_message(UINT u_msg, WPARAM w_param, LPARAM l_param)
{
	switch (u_msg)
	{
	case WM_MBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_LBUTTONDBLCLK:
	case WM_XBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
	case WM_XBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_LBUTTONUP:
	case WM_XBUTTONUP:
		return process_mouse_message(u_msg, w_param, l_param);
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		return process_keybd_message(u_msg, w_param, l_param);
	default:
		return false;
	}
}

bool input::process_mouse_message(UINT u_msg, WPARAM w_param, LPARAM l_param)
{
	auto key = VK_LBUTTON;
	auto state = key_state::NONE;
	switch (u_msg)
	{
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
		state = u_msg == WM_MBUTTONUP ? key_state::UP : key_state::DOWN;
		key = VK_MBUTTON;
		break;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
		state = u_msg == WM_RBUTTONUP ? key_state::UP : key_state::DOWN;
		key = VK_RBUTTON;
		break;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
		state = u_msg == WM_LBUTTONUP ? key_state::UP : key_state::DOWN;
		key = VK_LBUTTON;
		break;
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_XBUTTONDBLCLK:
		state = u_msg == WM_XBUTTONUP ? key_state::UP : key_state::DOWN;
		key = (HIWORD(w_param) == XBUTTON1 ? VK_XBUTTON1 : VK_XBUTTON2);
		break;
	default:
		return false;
	}

	if (state == key_state::UP && m_iKeyMap[key] == key_state::DOWN)
		m_iKeyMap[key] = key_state::PRESSED;
	else
		m_iKeyMap[key] = state;

	return true;
}

bool input::process_keybd_message(UINT u_msg, WPARAM w_param, LPARAM l_param)
{
	const auto key = w_param;

	auto state = key_state::NONE;
	switch (u_msg)
	{
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		state = key_state::DOWN;
		break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		state = key_state::UP;
		break;
	default:
		return false;
	}

	int vkey = int(key);

	if (vkey > 0 && vkey < ARRAYSIZE(m_iKeyMap))
	{
		if (state == key_state::UP && m_iKeyMap[vkey] == key_state::DOWN)
		{
			m_iKeyMap[int(key)] = key_state::PRESSED;
			auto& hotkey_callback = m_Hotkeys[key];
			if (hotkey_callback)
				hotkey_callback();
		}
		else
			m_iKeyMap[int(key)] = state;
	}

	return true;
}

key_state input::get_key_state(std::uint32_t vk)
{
	if (vk > 0 && vk < ARRAYSIZE(m_iKeyMap))
		return m_iKeyMap[vk];
#ifdef _DEBUG
	else
		printf("tried to get_key_state virtual key %i\n", vk);
#endif
	return key_state::NONE;
}

void input::reset_key_map()
{
	static auto b_did = false;
	if (GetForegroundWindow() == m_hTargetWindow)
	{
		b_did = false;
		return;
	}

	if (b_did)
		return;

	for (auto i = 0; i < 255; i++)
		m_iKeyMap[i] = key_state::NONE;

	b_did = true;
}

bool input::is_key_down(std::uint32_t vk)
{
	if (vk > 0 && vk < ARRAYSIZE(m_iKeyMap))
		return m_iKeyMap[vk] == key_state::DOWN;
#ifdef _DEBUG
	//else
	//	printf("tried to is_key_down virtual key %i\n", vk);
#endif
	return false;
}

bool input::was_key_pressed(std::uint32_t vk)
{
	if (vk > 0 && vk < ARRAYSIZE(m_iKeyMap))
	{
		if (m_iKeyMap[vk] == key_state::PRESSED)
		{
			m_iKeyMap[vk] = key_state::UP;
			return true;
		}
		return false;
	}
#ifdef _DEBUG
	else
		printf("tried to was_key_pressed virtual key %i\n", vk);
#endif
	return false;
}

void input::register_hotkey(std::uint32_t vk, std::function<void(void)> f)
{
	if (vk > 0 && vk < ARRAYSIZE(m_iKeyMap))
		m_Hotkeys[vk] = f;
#ifdef _DEBUG
	else
		printf("tried to register_hotkey virtual key %i\n", vk);
#endif
}

void input::remove_hotkey(std::uint32_t vk)
{
	m_Hotkeys[vk] = nullptr;
}
