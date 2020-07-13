#include "precompiled.h"
#include "Eventlog.h"
#include "HUD.h"

#include "Adriel/stdafx.hpp"

CEventLog g_Eventlog;

std::map <int, std::string> ChatMap = {
	{ 0x01, "\x01" },
	{ 0x02, "\x02" },
	{ 0x03, "\x03" },
	{ 0x04, "\x04" },
	{ 0x05, "\x05" },
	{ 0x06, "\x06" },
	{ 0x07, "\x07" },
	{ 0x08, "\x08" },
	{ 0x09, "\x09" },
	{ 0x0A, "\x0A" },
	{ 0x0B, "\x0B" },
	{ 0x0C, "\x0C" },
	{ 0x0D, "\x0D" },
	{ 0x0E, "\x0E" },
	{ 0x0F, "\x0F" },
	{ 0x10, "\x10" }
};

std::map <int, Color> ConsoleMap = {
	{ 0x01, Color(255, 255, 255) },
	{ 0x02, Color(255, 0, 0) },
	{ 0x03, Color(185, 130, 240) },
	{ 0x04, Color(0, 255, 0) },
	{ 0x05, Color(190, 255, 140) },
	{ 0x06, Color(165, 255, 80) },
	{ 0x07, Color(255, 60, 60) },
	{ 0x08, Color(200, 200, 200) },
	{ 0x09, Color(225, 210, 125) },
	{ 0x0A, Color(175, 195, 215) },
	{ 0x0B, Color(95, 150, 216) },
	{ 0x0C, Color(75, 105, 255) },
	{ 0x0D, Color(175, 195, 215) },
	{ 0x0E, Color(210, 40, 230) },
	{ 0x0F, Color(235, 75, 75) },
	{ 0x10, Color(225, 175, 50) }
};

void CEventLog::PreConsoleOutput()
{
#ifdef IMI_MENU
	PrintToConsole(Color::White(), "[");
	PrintToConsole(Color(75, 105, 255, 255), "ev0");
	PrintToConsole(Color::White(), "lve] ");
#else
	//decrypts(0)
	PrintToConsole(Color::White(), XorStr("[mutiny] | "));
	//encrypts(0)
#endif
}

void CEventLog::PrintToConsole(Color color, const char* pszText, ...)
{
	va_list va_alist;
	char *szBuffer = new char[4096];

	if (szBuffer)
	{
		va_start(va_alist, pszText);
		auto len = vsprintf_s(szBuffer, 4096, pszText, va_alist);
		va_end(va_alist);

		//decrypts(0)
		auto ColorMsg = GetProcAddress((HMODULE)Tier0Handle, XorStr("?ConColorMsg@@YAXABVColor@@PBDZZ"));
		//encrypts(0)

		if (ColorMsg)
		{

			//decrypts(0)
			ASSIGNVARANDIFNZERODO(_ColorMsg, reinterpret_cast<void(*)(const Color&, const char*, ...)>(ColorMsg))
				_ColorMsg(color, XorStr("%s"), szBuffer);
			//encrypts(0)
		}
		delete[] szBuffer;
	}
}

void CEventLog::OutputEvent(int logflag)
{
	if (!logflag)
	{
		if (!m_szEvent.empty())
			m_szEvent.clear();

		return;
	}

	std::string chat_msg = XorStrCT("\x01""[mutiny] | ");

	// 1 - console
	// 2 - chat
	// 3 - chat & console
	const bool use_console = logflag & 1;
	const bool use_chat = logflag & 2;

	if (use_console)
	{
		PreConsoleOutput();
	}

	for (size_t i = 0; i < m_szEvent.size(); i++)
	{
		auto evt = m_szEvent[i];

		if (use_console)
		{
			PrintToConsole(ConsoleMap[evt.m_iColor], evt.m_szText.data());
		}

		if (use_chat)
		{
			chat_msg.append(ChatMap.at(evt.m_iColor) + evt.m_szText);
		}
	}

	if (use_console)
	{
		PrintToConsole(Color::White(), "\n");
	}

	if (use_chat)
	{
		Interfaces::ClientMode->m_pChatElement->ChatPrintf(0, 0, chat_msg.data());
	}

	m_szEvent.clear();
}