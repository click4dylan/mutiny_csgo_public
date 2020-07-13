#pragma once

typedef void CBaseHudChatInputLine;
typedef void CHudChatHistory;
typedef void CHudChatFilterButton;
typedef void CHudChatFilterPanel;
typedef void CBaseHudChatLine;

enum ChatFilters_t : int
{
	CHAT_FILTER_NONE = 0,
	CHAT_FILTER_JOINLEAVE = (1 << 0),
	CHAT_FILTER_NAMECHANGE = (1 << 1),
	CHAT_FILTER_PUBLICCHAT = (1 << 2),
	CHAT_FILTER_SERVERMSG = (1 << 3),
	CHAT_FILTER_TEAMCHANGE = (1 << 4),
	CHAT_FILTER_ACHIEVEMENT = (1 << 5),
};

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

class CBaseHudChat// : public CHudElement
{
public:
	template<typename... T>
	void ChatPrintf(int iPlayerIndex, int iFilter, const char* fmt, T... args)
	{
		return GetVFunc<void(__cdecl*)(void*, int, int, const char*, ...)>(this, 27)(this, iPlayerIndex, iFilter, fmt, args...);
	}
	template<typename... T>
	void ChatPrintfW(int iPlayerIndex, int iFilter, const wchar_t* fmt, T... args)
	{
		return GetVFunc<void(__cdecl*)(void*, int, int, const wchar_t*, ...)>(this, 28)(this, iPlayerIndex, iFilter, fmt, args...);
	}

	CBaseHudChatLine* FindUnusedChatLine(void)
	{
		return m_ChatLine;
	}

	char pad_0004[452]; //0x0000
	float						m_flHistoryFadeTime; //0x01C4
	float						m_flHistoryIdleTime; //0x01C8
	CBaseHudChatInputLine*		m_pChatInput; //0x01CC
	CBaseHudChatLine*			m_ChatLine; //0x01D0
	int32_t						m_iFontHeight; //0x01D4
	CHudChatHistory*			m_pChatHistory; //0x01D8
	CHudChatFilterButton*		m_pFiltersButton; //0x01DC
	CHudChatFilterPanel*		m_pFilterPanel; //0x01E0
	Color						m_ColorCustom; //0x01E4
	int32_t						m_nMessageMode; //0x01E8
	int32_t						m_nVisibleHeight; //0x01EC
	uint32_t					m_hChatFont; //0x01F0
	int32_t						m_iFilterFlags; //0x01F4
	bool						m_bEnteringVoice; //0x01F8
};

class CCSGO_HudChat //: public CHudElement
{
public:

	char pad_0x0000[0x4C];
	int m_times_opened;
	char pad_0x0050[0x8];
	bool m_is_open;
	char pad_0x0059[0x427];
};

class CCSGO_DeathNotice //: public CHudElement
{
public:
	char pad_0x0000[0x4];				//0x0000
	int debug_id;						//0x0004 
	char pad_0x0008[0x4];				//0x0008
	bool N0000018D;						//0x000C 
	bool is_active;						//0x000D 
	char pad_0x000E[0xA];				//0x000E
	bool N00000190;						//0x0018 
	char pad_0x0019[0x7];				//0x0019
	char* name;							//0x0020 
	char pad_0x0024[0x18];				//0x0024
	void *m_hud;						//0x003C 
	CCSGO_DeathNotice *m_next_notice;	//0x0040 
	char pad_0x0044[0x8];				//0x0044
	float lifetime;						//0x004C 
	float local_player_modifier;		//0x0050 
	float fade_out_time;				//0x0054 
	char pad_0x0058[0x28];				//0x0058
};

class CHud
{
public:
	void *FindElement(const char* name)
	{
		return StaticOffsets.GetOffsetValueByType<void*(__thiscall *)(CHud*, const char *)>(_FindElement)(this, name);
	}
};
