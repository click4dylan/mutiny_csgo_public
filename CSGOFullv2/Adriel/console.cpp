#include "console.hpp"
#include "util.hpp"

console::console() : b_visible(false)
{
}

console::~console()
{
	close();
}

void console::initialize()
{
	auto str_title = adr_util::string::random(adr_util::random_int(15, 35));
	FILE *conin, *conout;
	AllocConsole();
	AttachConsole(GetCurrentProcessId());
	SetConsoleTitle(str_title.c_str());
	//decrypts(0)
	freopen_s(&conin, XorStr("conin$"), "r", stdin);
	freopen_s(&conout, XorStr("conout$"), "w", stdout);
	freopen_s(&conout, XorStr("conout$"), "w", stderr);
	//encrypts(0)
	b_visible = true;
}

void console::set_lines_columns(int i_width, int i_height, int x, int y)
{
	_COORD coord;
	coord.X = static_cast<SHORT>(i_width);
	coord.Y = static_cast<SHORT>(i_height);

	_SMALL_RECT rect;
	rect.Top = 0;
	rect.Left = 0;
	rect.Bottom = i_height - 1;
	rect.Right = i_width - 1;

	const auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleScreenBufferSize(handle, coord);
	SetConsoleWindowInfo(handle, TRUE, &rect);

	const auto h_wnd = GetConsoleWindow();
	RECT rc_scr, rc_wnd, rc_client;

	GetWindowRect(h_wnd, &rc_wnd);
	GetWindowRect(GetDesktopWindow(), &rc_scr);
	GetClientRect(h_wnd, &rc_client);

	MoveWindow(h_wnd, x, y, rc_wnd.right - rc_wnd.left, rc_wnd.bottom - rc_wnd.top, 1);
	SetWindowLong(h_wnd, GWL_STYLE, WS_POPUP);
	SetWindowRgn(h_wnd, CreateRectRgn(rc_client.left + 2, rc_client.top + 2, rc_client.right + 2, rc_client.bottom + 2), TRUE);
	ShowWindow(h_wnd, 1);
}

std::pair<int, int> console::get_lines_columns()
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	return std::pair<int, int>(csbi.srWindow.Right - csbi.srWindow.Left + 1, csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
}

void console::clear()
{
	const COORD top_left = { 0, 0 };
	const auto console = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO screen;
	DWORD written;

	GetConsoleScreenBufferInfo(console, &screen);
	FillConsoleOutputCharacterA(console, ' ', screen.dwSize.X * screen.dwSize.Y, top_left, &written);
	FillConsoleOutputAttribute(console, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE, screen.dwSize.X * screen.dwSize.Y, top_left, &written);
	SetConsoleCursorPosition(console, top_left);
}

void console::hide()
{
	//decrypts(0)
	const auto handle = FindWindowA(XorStr("ConsoleWindowClass"), nullptr);
	//encrypts(0)
	ShowWindow(handle, 0);
	b_visible = false;
}

void console::show()
{
	//decrypts(0)
	const auto handle = FindWindowA(XorStr("ConsoleWindowClass"), nullptr);
	//encrypts(0)
	ShowWindow(handle, 1);
	b_visible = true;
}

void console::freeconsole()
{
	FreeConsole();
}

void console::close()
{
	hide();
	freeconsole();
}

bool console::is_visible() const
{
	return b_visible;
}

std::shared_timed_mutex logger::m_mutex;