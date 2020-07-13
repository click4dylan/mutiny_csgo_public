#pragma once

class CBaseHudChat;

class IClientModeShared
{
public:
	char pad_0000[16]; //0x0000
	float				m_flReplayStartRecordTime; //0x0010
	float				m_flReplayStopRecordTime; //0x0014
	void*				m_pReplayReminderPanel; //0x0018
	CBaseHudChat*		m_pChatElement; //0x001C
	unsigned long		m_CursorNone; //0x0020
	void*				m_pWeaponSelection; //0x0024
	int					m_nRootSize[2]; //0x0028
}; //Size: 0x0030