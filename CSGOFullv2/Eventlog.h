#pragma once

#include "Includes.h"

class CEventLog {
public:
	struct EventElement {
		/*
		constexpr const char *CHAT_COLOR_DEFAULT = "\x01";
		constexpr const char *CHAT_COLOR_RED = "\x02";
		constexpr const char *CHAT_COLOR_LIGHT_PURPLE = "\x03";
		constexpr const char *CHAT_COLOR_GREEN = "\x04";
		constexpr const char *CHAT_COLOR_LIME = "\x05";
		constexpr const char *CHAT_COLOR_LIGHT_GREEN = "\x06";
		constexpr const char *CHAT_COLOR_LIGHT_RED = "\x07";
		constexpr const char *CHAT_COLOR_GRAY = "\x08";
		constexpr const char *CHAT_COLOR_LIGHT_OLIVE = "\x09";
		constexpr const char *CHAT_COLOR_LIGHT_STEELBLUE = "\x0A";
		constexpr const char *CHAT_COLOR_LIGHT_BLUE = "\x0B";
		constexpr const char *CHAT_COLOR_BLUE = "\x0C";
		constexpr const char *CHAT_COLOR_PURPLE = "\x0D";
		constexpr const char *CHAT_COLOR_PINK = "\x0E";
		constexpr const char *CHAT_COLOR_RED_ORANGE = "\x0F";
		constexpr const char *CHAT_COLOR_OLIVE = "\x10";
		*/
		int m_iColor = 0x01;
		std::string m_szText = "";
		EventElement(int _i, std::string _sz) {
			m_iColor = _i;
			m_szText = _sz;
		}
	};

	CEventLog() {

	}

	void AddEventElement(int Color, std::string _text) {
		m_szEvent.push_back(EventElement(Color, _text));
	}
	void OutputEvent(int logflag = 0);
	void ClearEvent() { m_szEvent.clear(); }
	bool Empty() const { return m_szEvent.empty(); }
	int Size() const { return m_szEvent.size(); }
protected:
	std::vector<EventElement> m_szEvent;
	void PreConsoleOutput();
#ifdef _DEBUG
public:
#endif
	void PrintToConsole(Color color, const char* pszText, ...);
};

extern CEventLog g_Eventlog;