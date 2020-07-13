#pragma once
#include "inetmsghandler.h"
#include "utlvector.h"
struct ClientClass;
class INetChannelInfo;

#define STEAM_KEYSIZE 2048 // max size needed to contain a steam authentication key (both server and client)

class CEventInfo
{
public:
	enum
	{
		EVENT_INDEX_BITS = 8,
		EVENT_DATA_LEN_BITS = 11,
		MAX_EVENT_DATA = 192
	};

	short classID;
	float fire_delay;
	const void *pSendTable;
	const ClientClass *pClientClass;
	int bits;
	unsigned char *pData;
	int flags;
	char pad[0x18];
	CEventInfo *pNextEvent;
};

class CNetworkStringTableContainer /* : public INetworkStringTableContainer*/
{
public:
	bool m_bAllowCreation; // creat guard Guard
	int m_nTickCount; // current tick
	bool m_bLocked; // currently locked?
	bool m_bEnableRollback; // enables rollback feature

	CUtlVector< class CNetworkStringTable * > m_Tables; // the string tables
};

class CClockDriftMgr
{
public:
	enum
	{
		// This controls how much it smoothes out the samples from the server.
		NUM_CLOCKDRIFT_SAMPLES = 16
	};

	// This holds how many ticks the client is ahead each time we get a server tick.
	// We average these together to get our estimate of how far ahead we are.
	float m_ClockOffsets[NUM_CLOCKDRIFT_SAMPLES]; //0x0128
	int m_iCurClockOffset; // 0x0168

	int m_nServerTick; // 0x016C		// Last-received tick from the server.
	int m_nClientTick; // 0x0170		// The client's own tick counter (specifically, for interpolation during rendering).
		// The server may be on a slightly different tick and the client will drift towards it.
}; //Size: 76

class CBaseClientState : public INetChannelHandler,
						 public IConnectionlessPacketHandler,
						 public IServerMessageHandler
{
public:
	void ForceFullUpdate() { m_nDeltaTick = -1; }
	IClientNetworkable *GetNetworkable()
	{
		return (IClientNetworkable *)((uint32)this + 8);
	}
	void FreeEntityBaselines()
	{
		StaticOffsets.GetOffsetValueByType< void(__thiscall *)(IClientNetworkable *) >(_FreeEntityBaselines)(GetNetworkable());
	}

	char pad_000C[140]; //0x0000
	int32 m_Socket; //0x0098
	INetChannel *m_pNetChannel; //0x009C
	uint32 m_nChallengeNr; //0x00A0
	char pad_00A0[4]; //0x00A4
	double m_flConnectTime; //0x00A8
	uint32 m_nRetryNumber; //0x00B0
	char pad_00B4[84]; //0x00B4
	int32 m_nSignonState; //0x0108
	char pad_010C[4]; //0x010C
	double m_flNextCmdTime; //0x0114
	int32 m_nServerCount; //0x0118
	int32 m_nCurrentSequence; //0x011C
	char pad_0120[8]; //0x0120
	CClockDriftMgr m_ClockDriftMgr; //0x0128
	int32 m_nDeltaTick; //0x0174
	char pad_0178[4]; //0x0178
	int32 m_nViewEntity; //0x017C
	int32 m_nPlayerSlot; //0x0180
	bool m_bPaused; //0x0184
	char pad_0185[3]; //0x0185
	char m_szLevelName[260]; //0x0188
	char m_szLevelNameShort[40]; //0x028C
	char pad_02B4[212]; //0x02B4
	uint32 m_nMaxClients; //0x0310
	char pad_0314[232]; //0x0314
	class PackedEntity *m_pEntityBaselines[2][MAX_EDICTS]; //0x03FC
	class C_ServerClassInfo *m_pServerClasses; //0x43FC
	int32 m_nServerClasses; //0x4400
	int32 m_nServerClassBits; //0x4404
	char m_szEncrytionKey[STEAM_KEYSIZE]; //0x4408
	uint32 m_iEncryptionKeySize; //0x4C08
	char pad_4C0C[148]; //0x4C0C
};

class CClientState : public CBaseClientState /*, public CClientFrameManager*/
{
	typedef struct CustomFile_s
	{
		unsigned int crc; //file CRC
		unsigned int reqID; // download request ID
	} CustomFile_t;

public:
	int oldtickcount; // previous tick
	float m_tickRemainder; // client copy of tick remainder
	float m_frameTime; // dt of the current frame

	int lastoutgoingcommand; // correct 0x4D24 last command sequence number acknowledged by server
	int chokedcommands; // correct 0x4D28 number of choked commands
	int last_command_ack; //0x4D2C Sequence number of last outgoing command
	int command_ack_servertickcount; //0x4D30 server tickcount when we last received command ack
	int command_ack; //0x4D34 // current command sequence acknowledged by server
	int m_nSoundSequence; //0x4D38 current processed reliable sound sequence number

	char pad_4CD0[4]; //0x4D3C

	//
	// information that is static for the entire time connected to a server
	//
	bool ishltv; // true if HLTV server/demo				//0x4D40

	char pad_4CC9[279]; 

	CEventInfo* events;
	//CUtlFixedLinkedList< CEventInfo > events; // list of received events

	//int demonum; // -1 = don't play demos
	//char demos[MAX_DEMOS][MAX_DEMONAME]; // when not playing

	void SetReservationCookie(unsigned long long cookie);
};
