#include "precompiled.h"
#include "LocalPlayer.h"
#include "VTHook.h"
#include "Netchan.h"
#include "VMProtectDefs.h"
#include "ProcessPacket.h"
#include "netmessages.h"
#include "soundinfo.h"
#include "CPlayerResource.h"
#include "Keys.h"
//#include "Convars.h"
#include "CClientState.h"
#include "CLC_Move.h"
#include "Adriel/input.hpp"
#include "utlbuffer.h"
#include "bitvec.h"
//#include <netmessages.pb.h>
//#include <cstrike15_usermessages.pb.h>

SendDatagramFn oSendDatagram = NULL;
CanPacketFn oCanPacket = NULL;

float FAKE_LATENCY_AMOUNT = 0.0f;
extern bool NextPacketShouldBeRealPing;
bool manuallystoppedcommunications = false;
bool stopfreezing = false;
float totaltimefrozen = 0.0f;
float totaltimeunfrozen = 0.0f;
bool corrupt_server = false;
int packet_index = 0;

#include "Secret.h"
#include "CreateMove.h"
#include "ISteamNetworkingUtils.h"
#include "ConCommand.h"
#include "UsedConvars.h"
#include "inetmsghandler.h"

#ifdef SERVER_CRASHER
bool PING_IS_STILL_SPIKED = false;
int SIMULATE_CMDS_TWICE = 0;
#endif

static int number = 0;

#pragma optimize("", off)

bool __fastcall Hooks::CanPacket(void* netchan)
{
//#ifdef SERVER_CRASHER
//	return true;
//#else
	return oCanPacket(netchan);
//#endif
}

enum NetworkSystemAddressType_t
{
	NSAT_NETADR,
	NSAT_P2P,
	NSAT_PROXIED_GAMESERVER,	// Client proxied through Steam Datagram Transport
	NSAT_PROXIED_CLIENT,		// Client proxied through Steam Datagram Transport
};

enum PeerToPeerAddressType_t
{
	P2P_STEAMID,
};

class CPeerToPeerAddress
{
public:
	uint64 m_steamID; //CSteamID
	int m_steamChannel; // SteamID channel (like a port number to disambiguate multiple connections)

	PeerToPeerAddressType_t m_AddrType;
};

struct ns_address
{
	netadr_t m_adr; // ip:port and network type (NULL/IP/BROADCAST/etc).
	CPeerToPeerAddress m_steamID; // SteamID destination
	NetworkSystemAddressType_t m_AddrType;
};

typedef struct netpacket2_s
{
	ns_address		from;		// sender address
	int				source;		// received source 
	double			received;	// received time
	unsigned char	*data;		// pointer to raw packet data
	bf_read			message;	// easy bitbuf data access
	int				size;		// size in bytes
	int				wiresize;   // size in bytes before decompression
	bool			stream;		// was send as stream
	struct netpacket2_s *pNext;	// for internal use, should be NULL in public
} netpacket2_t;

//USED BY TEST CRASHER
//char *namespam11str = new char[151]{ 119, 119, 119, 119, 119, 119, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, 0 };
//char *namespam22str = new char[148]{ 119, 119, 119, 119, 119, 119, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 31, -1, 0 };

#ifdef _DEBUG
void inline Cmd_ForwardToServer(char *str)
{
	typedef void(__thiscall* SendStringCmdFn)(DWORD, char*);
	static bool found = false;
	static DWORD forwardtoserveroffset2;
	static SendStringCmdFn SendStringCmd;
	if (!found)
	{
		char *forwardtoserversig = "A1  ??  ??  ??  ??  B9  ??  ??  ??  ??  FF  50  14  8B  0C  85  ??  ??  ??  ??  8D  85";
		auto adr = FindMemoryPattern(EngineHandle, forwardtoserversig, strlen(forwardtoserversig));
		if (adr)
			forwardtoserveroffset2 = *(DWORD*)(adr + 1);

		char *sendstringcmdsig = "55  8B  EC  83  EC  34  57  8B  F9  83  BF  94  00  00  00  00";
		adr = FindMemoryPattern(EngineHandle, sendstringcmdsig, strlen(sendstringcmdsig));

		if (adr)
			SendStringCmd = (SendStringCmdFn)adr;
		else
			forwardtoserveroffset2 = NULL;

		found = true;
	}

	if (forwardtoserveroffset2)
	{
		typedef DWORD(__thiscall* OriginalFn)(DWORD);
		DWORD ret = GetVFunc<OriginalFn>((void*)forwardtoserveroffset2, 0x14 / 4)(forwardtoserveroffset2);

		DWORD thisptr = *(DWORD*)(ret * 4 + (forwardtoserveroffset2 + 4));
		thisptr += 8;

		SendStringCmd(thisptr, str);
	}
}

//Does not seem to do anything
void inline SayText(char *str)
{
	typedef void(__thiscall* SendStringCmdFn)(DWORD, char*);
	static bool found = false;
	static DWORD sendstringcmdoffset;
	if (!found)
	{
		char *forwardtoserversig = "8B  0D  ??  ??  ??  ??  85  C9  74  14  8B  01  8D  95  ??  ??  ??  ??";
		auto adr = FindMemoryPattern(EngineHandle, forwardtoserversig, strlen(forwardtoserversig));
		if (adr)
			sendstringcmdoffset = *(DWORD*)(adr + 2);
		else
			sendstringcmdoffset = NULL;

		found = true;
	}

	if (sendstringcmdoffset)
	{
		DWORD ret = *(DWORD*)sendstringcmdoffset;
		if (ret)
		{
			GetVFunc<SendStringCmdFn>((void*)ret, 0x1D4 / 4)(ret, str);
		}
	}
}
#endif

bool ServerCrashing = false;
bool FuckingServer = false;

//char *netmsglimitsigstr = new char[21]{ 73, 56, 90, 74, 79, 90, 69, 69, 90, 69, 69, 90, 69, 69, 90, 69, 69, 90, 77, 63, 0 }; /*3B 05 ?? ?? ?? ?? 7E*/
//char *steamnetworkingsocketsstr = new char[27]{ 9, 14, 31, 27, 23, 20, 31, 14, 13, 21, 8, 17, 19, 20, 29, 9, 21, 25, 17, 31, 14, 9, 84, 30, 22, 22, 0 }; /*steamnetworkingsockets.dll*/
DWORD CNetMsg_Tick_Constructor = NULL;
DWORD CNetMsg_Tick_Destructor = NULL;
DWORD CNetMsg_Tick_t_VFTable1 = NULL;
DWORD CNetMsg_Tick_t_VFTable2 = NULL;
DWORD CNetMsg_Tick_t_Setup = NULL;
float *host_framestarttime_std_deviation; //xmm0
float *host_computationtime_std_deviation; //xmm3
float *host_computationtime; //xmm2

void __fastcall CNetMsg_Tick_Destruct(void* memory)
{
	((void(__thiscall*)(void*))CNetMsg_Tick_Destructor)(memory);
}

void __fastcall CNetMsg_Tick_Construct(void* memory, float host_computationtime /*@<xmm2>*/, float host_computationtime_std_deviation /*@<xmm3>*/, int m_nDeltaTick, float host_framestarttime_std_deviation /*@<xmm0>*/)
{
	DWORD pmemory; // edi
	unsigned int framestarttime_std_deviation; // esi
	unsigned int computationtime; // ecx
	unsigned int computationtime_std_deviation; // ecx
	double fl_framestarttime_std_deviation; // xmm0_8

	pmemory = (DWORD)memory;
	((void(__thiscall*)(void*))CNetMsg_Tick_t_Setup)(memory);
	*(DWORD *)pmemory = CNetMsg_Tick_t_VFTable1;// &CNETMsg_Tick_t::`vftable';
	*(DWORD *)(pmemory + 4) = CNetMsg_Tick_t_VFTable2;//&CNETMsg_Tick_t::`vftable';
	*(BYTE *)(pmemory + 40) = 0;
	*(DWORD *)(pmemory + 36) |= 1u;
	*(DWORD *)(pmemory + 12) = m_nDeltaTick;
	framestarttime_std_deviation = 1000000;
	computationtime = 1000000;
	if ((unsigned int)(host_computationtime * 1000000.0) < 1000000)
		computationtime = (unsigned int)(host_computationtime * 1000000.0);
	*(DWORD *)(pmemory + 36) |= 2u;
	*(DWORD *)(pmemory + 16) = computationtime;
	computationtime_std_deviation = 1000000;
	if ((unsigned int)(host_computationtime_std_deviation * 1000000.0) < 1000000)
		computationtime_std_deviation = (unsigned int)(host_computationtime_std_deviation * 1000000.0);
	*(DWORD *)(pmemory + 36) |= 4u;
	*(DWORD *)(pmemory + 20) = computationtime_std_deviation;
	fl_framestarttime_std_deviation = host_framestarttime_std_deviation * 1000000.0;
	if ((unsigned int)fl_framestarttime_std_deviation < 1000000)
		framestarttime_std_deviation = (unsigned int)fl_framestarttime_std_deviation;
	*(DWORD *)(pmemory + 36) |= 8u;
	*(DWORD *)(pmemory + 24) = framestarttime_std_deviation;
}

//Do not remove
#ifdef SERVER_CRASHER

#ifndef PUBLIC_CRASHER
char *constructsig = new char[39]{ 79, 79, 90, 90, 66, 56, 90, 90, 63, 57, 90, 90, 79, 76, 90, 90, 79, 77, 90, 90, 66, 56, 90, 90, 60, 67, 90, 90, 66, 62, 90, 90, 78, 60, 90, 90, 74, 78, 0 }; /*55  8B  EC  56  57  8B  F9  8D  4F  04*/
char *destructsig = new char[355]{ 79, 73, 90, 90, 79, 76, 90, 90, 79, 77, 90, 90, 66, 56, 90, 90, 60, 67, 90, 90, 66, 62, 90, 90, 77, 77, 90, 90, 69, 69, 90, 90, 57, 77, 90, 90, 74, 77, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 57, 77, 90, 90, 78, 77, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 66, 56, 90, 90, 78, 76, 90, 90, 69, 69, 90, 90, 66, 73, 90, 90, 60, 66, 90, 90, 69, 69, 90, 90, 77, 72, 90, 90, 69, 69, 90, 90, 76, 59, 90, 90, 69, 69, 90, 90, 78, 74, 90, 90, 79, 74, 90, 90, 60, 60, 90, 90, 73, 76, 90, 90, 63, 66, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 66, 73, 90, 90, 57, 78, 90, 90, 69, 69, 90, 90, 57, 77, 90, 90, 78, 76, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 66, 73, 90, 90, 77, 63, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 57, 77, 90, 90, 78, 76, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 77, 72, 90, 90, 69, 69, 90, 90, 66, 56, 90, 90, 73, 76, 90, 90, 66, 62, 90, 90, 78, 60, 90, 90, 69, 69, 90, 90, 57, 76, 90, 90, 74, 76, 90, 90, 69, 69, 90, 90, 63, 66, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 57, 77, 90, 90, 74, 77, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 79, 60, 90, 90, 79, 63, 90, 90, 79, 56, 90, 90, 57, 73, 90, 90, 66, 73, 90, 90, 63, 67, 90, 90, 69, 69, 0 }; /*53  56  57  8B  F9  8D  77  ??  C7  07  ??  ??  ??  ??  C7  47  ??  ??  ??  ??  ??  8B  46  ??  83  F8  ??  72  ??  6A  ??  40  50  FF  36  E8  ??  ??  ??  ??  83  C4  ??  C7  46  ??  ??  ??  ??  ??  83  7E  ??  ??  C7  46  ??  ??  ??  ??  ??  72  ??  8B  36  8D  4F  ??  C6  06  ??  E8  ??  ??  ??  ??  C7  07  ??  ??  ??  ??  5F  5E  5B  C3  83  E9  ??*/
char *clcmsg_listeneventsconstructorsig = "56  8B  F1  ??  ??  ??  ??  ??  ??  8D  4E  08  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  C7  46  0C  00  00  00  00  8B  C6  C7  46  10  00  00  00  00  C7  46  14  00  00  00  00  C7  46  18  00  00  00  00  C7  46  1C  00  00  00  00  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  C6  46  20  01";
char *clc_filecrccheckvtablesig = "C7  45  9C  ??  ??  ??  ??  C7  45  A0  ??  ??  ??  ??  66  90";
char *clc_filecrccheckinitsig = "C7  41  34  00  00  00  00  C7  41  0C  00  00  00  00  C7  41  08";
#endif

typedef void*(__thiscall* NET_SignonStateConstructFn)(void*, int, int);
typedef void(__thiscall* NET_SignonStateDestructFn)(void*, void*);
typedef void*(__thiscall* CLCMsg_ListenEventsConstructFn)(void*);
typedef void*(__thiscall* CLC_FileCRCCheck_InitFn)(void*);

NET_SignonStateConstructFn NET_SignonStateConstructor = NULL;
NET_SignonStateDestructFn NET_SignonStateDestructor = NULL;
CLCMsg_ListenEventsConstructFn CLCMsg_ListenEvents_Constructor = NULL;
CLC_FileCRCCheck_InitFn CLC_FileCRCCheck_Init = NULL;
DWORD CLC_FileCRCCheck_VTable1 = NULL;
DWORD CLC_FileCRCCheck_VTable2 = NULL;

void CLC_FileCRCCheck_Constructor(void* memory)
{
	DWORD *v1; // edi

	v1 = (DWORD*)memory;
	*(DWORD*)(v1 + 2) = 0;
	CLC_FileCRCCheck_Init(v1 + 1);
	*((BYTE *)v1 + 64) = 1;
	v1[22] = 15;
	v1[21] = 0;
	*((BYTE *)v1 + 68) = 0;
	*v1 = CLC_FileCRCCheck_VTable1;
	v1[1] = CLC_FileCRCCheck_VTable2;
}
#endif

char *cl_sendmovesig = new char[87]{ 79, 79, 90, 90, 66, 56, 90, 90, 63, 57, 90, 90, 59, 75, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 66, 75, 90, 90, 63, 57, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 56, 67, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 79, 73, 90, 90, 66, 56, 90, 90, 67, 66, 0 }; /*55  8B  EC  A1  ??  ??  ??  ??  81  EC  ??  ??  ??  ??  B9  ??  ??  ??  ??  53  8B  98*/

#include "raw_buffer.h"
float MIN_INTERVAL_PER_FRAME = 0.0f;

inline bool ShouldRunFrame(float dt)
{
	return dt >= MIN_INTERVAL_PER_FRAME;
}

void RunFrame()
{
#if 0
#define TICKS_TO_SEND_IN_BATCH 1
#define MAX_TICKS_PER_FRAME 1
	CUserCmd *lastcmd = CreateMoveVars.LastUserCmd;
	INetChannel *chan = (INetChannel*)g_ClientState->m_pNetChannel;
	if (chan && lastcmd)
	{
		for (int repeat = 0; repeat < MAX_TICKS_PER_FRAME; repeat++)
		{
			int lastcommand = g_ClientState->lastoutgoingcommand;
			int chokedcount = g_ClientState->chokedcommands;
			int nextcommandnr = lastcommand + chokedcount + 1;

			for (int i = 0; i < TICKS_TO_SEND_IN_BATCH; i++)
			{
				CUserCmd *cmd = GetUserCmd(0, nextcommandnr, true);
				if (cmd)
				{
					if (!lastcmd)
						cmd->Reset();
					else
						*cmd = *lastcmd;

					cmd->forwardmove = 0.0f;// 450.0f;
					cmd->buttons = IN_ATTACK | IN_BULLRUSH;

					cmd->command_number = nextcommandnr++;
					cmd->tick_count = Interfaces::Globals->tickcount;

#if TICKS_TO_SEND_IN_BATCH > 1
					chokedcount++;
#endif
				}
			}

			static void* tmp = malloc(2048);
			CNetMsg_Tick_Construct(tmp, *host_computationtime, *host_computationtime_std_deviation, g_ClientState->m_nDeltaTick, *host_framestarttime_std_deviation);
			chan->SendNetMsg(tmp, false, false);

			g_ClientState->chokedcommands = chokedcount;
			chan->m_nChokedPackets = chokedcount;
			((void(*)())CL_SendMove)();
			//chan->m_nOutSequenceNr += 20;
			int resul = oSendDatagram(chan, 0);
			g_ClientState->lastoutgoingcommand = resul;
			g_ClientState->chokedcommands = 0;
		}
	}
#endif
}

void _stdcall MoveThread(void* args)
{
#define INTERVAL_PER_FRAME (1.0f / 768.0f)
	float realtime = 0.0f;
	float frametime;
	double curtime = QPCTime(), lasttime = QPCTime();
	float mainloop_frametime = 0, time_remainder = 0;
	for (;;)
	{
		curtime = QPCTime();
		mainloop_frametime += (float)(curtime - lasttime);
		lasttime = curtime;
		if (ShouldRunFrame(mainloop_frametime))
		{
			realtime += mainloop_frametime;
			time_remainder += mainloop_frametime;
			frametime = mainloop_frametime; //Set time between frames so we can calculate FPS
			int numframes = 0; //how many times we will run a frame this frame
			if (time_remainder >= INTERVAL_PER_FRAME)
			{
				numframes = (int)(time_remainder / INTERVAL_PER_FRAME);
				time_remainder -= numframes * INTERVAL_PER_FRAME;
			}
			MIN_INTERVAL_PER_FRAME = INTERVAL_PER_FRAME - time_remainder;
			for (int frame = 0; frame < numframes; frame++)
			{
				RunFrame();
			}
			mainloop_frametime = 0; //Reset for next frame
		}
		else
		{
			Sleep(1);
		}
	}
}

int random2 = 0;
int random1 = 0;
bool fuck = 0;
#include <process.h>
//char *unknownnetmsgcall = new char[67]{ 79, 79, 90, 90, 66, 56, 90, 90, 63, 57, 90, 90, 79, 73, 90, 90, 66, 56, 90, 90, 79, 62, 90, 90, 74, 66, 90, 90, 79, 76, 90, 90, 66, 56, 90, 90, 60, 75, 90, 90, 66, 79, 90, 90, 62, 56, 90, 90, 77, 78, 90, 90, 79, 77, 90, 90, 66, 56, 90, 90, 78, 63, 90, 90, 75, 78, 0 }; /*55  8B  EC  53  8B  5D  08  56  8B  F1  85  DB  74  57  8B  4E  14*/
//char *net_sendpacketsigstr = new char[79]{ 79, 79, 90, 90, 66, 56, 90, 90, 63, 57, 90, 90, 56, 66, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 74, 74, 90, 90, 74, 74, 90, 90, 63, 66, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 79, 73, 90, 90, 66, 56, 90, 90, 62, 67, 90, 90, 79, 76, 90, 90, 79, 77, 90, 90, 66, 56, 90, 90, 60, 59, 0 }; /*55  8B  EC  B8  ??  ??  00  00  E8  ??  ??  ??  ??  53  8B  D9  56  57  8B  FA*/
Net_SendPacketFn NET_SendPacket = NULL;

bool bAllowNetTick = false;

#if 0
#pragma pack(push, 1)
struct CLC_VoiceData2
{
	void* vtable; //0x21B0  0
	uint32_t unknown; //0x21AC  4
	void* vtable2; //0x21A8  8
	uint32_t unknown; //21A4  12
	uint32_t somememory; //21A0  16
	uint32_t unknown; //219C  20
	uint32_t someint; //2198  24
	uint32_t someint; //2194  28
	uint32_t unknownint; //2190  32
	uint32_t someint; //218C  36
	uint32_t someint; //2188  40
	uint32_t someint; //2184  44
	uint32_t someint; //2180  48
	uint32_t flags;   //217C  52
	uint32_t someint; //2178  56
	uint32_t someint; //2174  60
	uint32_t unknown; //2170  64
	uint32_t unknown; //216C  68
	uint32_t unknown; //2168  72
	uint32_t someint; //2164  76
	uint32_t someint; //2160  80

	CLC_VoiceData2::CLCVoiceData();
};
#pragma pack(pop)

CLC_VoiceData2::CLCVoiceData()
{
	flags |= 4;
	unknownint = 0;
	if (unk)
		unknownint = 1;

}
#endif

volatile bool EnableFucking = false;
volatile INetChannel *FuckingChannel = nullptr;
CL_SendMoveFn CL_SendMove = NULL;
extern bool DisableCLC_Move;
int maxroutable = 1260;

int __fastcall sv_maxroutable_GetInt(ConVar* cvar)
{
	return maxroutable;
}

int __fastcall net_maxroutable_GetInt(ConVar* cvar)
{
	return maxroutable;
}


unsigned int splitoffset;
unsigned short SplitPacketSize = 0;
BOOL ShouldSetSplitPacketSize = FALSE;
void* SetSplitPacketSizeJmpBack;
__declspec(naked) void SetSplitPacketSize()
{
	__asm
	{
		push eax
		mov eax, ShouldSetSplitPacketSize
		test eax, eax
		jz dontset

		movzx ax, SplitPacketSize
		push edx
		mov edx, splitoffset
		mov word ptr ss : [ebp + edx], ax
		pop edx
		pop eax
		jmp SetSplitPacketSizeJmpBack

		dontset :
		pop eax
			push ecx
			mov ecx, splitoffset
			mov word ptr ss : [ebp + ecx], ax
			pop ecx
			jmp SetSplitPacketSizeJmpBack
	}
}

unsigned int splitidoffset;
unsigned short SplitPacketID = -1;
BOOL ShouldSetSplitPacketID = FALSE;
void* SetSplitPacketIDJmpBack;
__declspec(naked) void SetSplitPacketID()
{
	__asm
	{
		push eax
		mov eax, ShouldSetSplitPacketID
		test eax, eax
		jz dontset2

		movzx ax, SplitPacketID
		push edx
		mov edx, splitidoffset
		mov word ptr ss : [ebp + edx], ax
		pop edx
		pop eax
		jmp SetSplitPacketIDJmpBack

		dontset2 :
		pop eax
			push ecx
			mov ecx, splitidoffset
			mov word ptr ss : [ebp + ecx], ax
			pop ecx
			jmp SetSplitPacketIDJmpBack
	}
}

VTHook *sv_maxroutable_hook = nullptr;
VTHook *net_maxroutable_hook = nullptr;

// Use this to pick apart the network stream, must be packed
#pragma pack(1)
typedef struct
{
	int		netID;
	int		sequenceNumber;
	char	packetID;
} SPLITPACKET_RECV;
#pragma pack()

#define MAX_ROUTABLE_PAYLOAD		1260
#define MIN_USER_MAXROUTABLE_SIZE	576  // ( X.25 Networks )
#define MAX_USER_MAXROUTABLE_SIZE	MAX_ROUTABLE_PAYLOAD
#define NET_MAX_MESSAGE 523956


#define MAX_SPLIT_SIZE	(MAX_USER_MAXROUTABLE_SIZE - sizeof( SPLITPACKET ))
#define MIN_SPLIT_SIZE	(MIN_USER_MAXROUTABLE_SIZE - sizeof( SPLITPACKET ))

#define MAX_SPLITPACKET_SPLITS ( NET_MAX_MESSAGE / MIN_SPLIT_SIZE )

int NET_SendLong(unsigned char* sendbuf, int sendlen, int nMaxRoutableSize = 576 /*576-1200*/)
{
}

int NET_SendLong_Fucked2(int buffersize, int numsplits /*engine limit says 929 max, 128 is actual limit*/, int nMaxRoutableSize = 576 /*576-1200*/)
{
}

int NET_SendLong_Fucked(int buffersize, int numsplits /*engine limit says 929 max, 128 is actual limit*/, int nMaxRoutableSize = 576 /*576-1200*/)
{

}

BOOL __stdcall OnSendPacket(void* netchan, unsigned char *data, size_t length, bf_write *pVoicePayload, bool bUseCompression)
{
	//Don't allow recursive calls
	static std::atomic<bool> IsAlreadySending = false;
	static float timeelapsed = 0.0f;

	if (IsAlreadySending || length == 0)
		return 1;

	static float lastcalltime = QPCTime();
	float time = QPCTime();

	IsAlreadySending = true;
	std::ifstream nag("g:\\stuff.txt");

	//Set to 0 to drop the packet
	BOOL ShouldKeepPacket = !input::get().is_key_down(0x31);
	BOOL ShouldFuckWithPacket = !ShouldKeepPacket;
	int crash = 0;

	if (nag.is_open())
	{
		char tmp[128];
		memset(tmp, 0, 128);
		if (nag.getline(tmp, 128) && strlen(tmp) > 0)
			crash = atoi(tmp);

		if (crash >= 1)
			ShouldFuckWithPacket = true;
	}

	int NewLength = max(length, 262160);

	unsigned char *newdata = new unsigned char[NewLength];
	if (newdata)
	{
		memset(newdata, 0, NewLength);
		memcpy(newdata, data, length);

		
		for (int i = 0; i < length; ++i)
			newdata[i] = rand() % 255 
			;

		//Should we dump the packet to a file
		bool ShouldDumpPacket = false;
		if (ShouldDumpPacket)
		{
			static std::ofstream file("g:\\packetdump.hex", std::ofstream::trunc | std::ofstream::binary);
			if (file.is_open())
			{
				file.seekp(0);
				file.write((const char*)data, length);
				file.flush();
			}
		}

		static ConVar* sv_maxrouteable = Interfaces::Cvar->FindVar("sv_maxroutable");
		static ConVar* net_maxroutable = Interfaces::Cvar->FindVar("net_maxroutable");
		static ConVar* cl_flushentitypacket = Interfaces::Cvar->FindVar("cl_flushentitypacket");
		static ConVar* net_compresspackets_minsize = Interfaces::Cvar->FindVar("net_compresspackets_minsize");
		static ConVar* net_compresspackets = Interfaces::Cvar->FindVar("net_compresspackets");
		static ConVar* net_threaded_socket_recovery_time = Interfaces::Cvar->FindVar("net_threaded_socket_recovery_time");
		static ConVar* net_threaded_socket_recovery_rate = Interfaces::Cvar->FindVar("net_threaded_socket_recovery_rate");
		static ConVar* net_threaded_socket_burst_cap = Interfaces::Cvar->FindVar("net_threaded_socket_burst_cap");
		net_threaded_socket_burst_cap->nFlags = 0;
		net_threaded_socket_recovery_rate->nFlags = 0;
		net_threaded_socket_recovery_time->nFlags = 0;
		net_threaded_socket_recovery_time->SetValue(2);
		net_threaded_socket_recovery_rate->SetValue(999999);
		net_threaded_socket_burst_cap->SetValue(999999);
		net_compresspackets->nFlags = 0;
		net_compresspackets->SetValue(true);
		net_compresspackets_minsize->nFlags = 0;
		net_compresspackets_minsize->SetValue(0);
		cl_flushentitypacket->nFlags = 0;


		sv_maxrouteable->nFlags = 0;
		net_maxroutable->nFlags = 0;

		static bool Held = false;
		if (ShouldFuckWithPacket)
		{
			//cl_flushentitypacket->SetValue(5);

			maxroutable = 1000; //13 worked sometimes
			sv_maxrouteable->fMinVal = 0.0f;
			sv_maxrouteable->fValue = *(DWORD*)&maxroutable ^ (DWORD)sv_maxrouteable;
			sv_maxrouteable->nValue = *(DWORD*)&maxroutable ^ (DWORD)sv_maxrouteable;
			net_maxroutable->fMinVal = 0.0f;
			net_maxroutable->fValue = *(DWORD*)&maxroutable ^ (DWORD)net_maxroutable;
			net_maxroutable->nValue = *(DWORD*)&maxroutable ^ (DWORD)net_maxroutable;

			ShouldSetSplitPacketSize = 0;
			ShouldSetSplitPacketID = 0;
			SplitPacketSize = 1;
			SplitPacketID = -1; //-1

			static float lagfor = 0.0f;
			static float dontlagfor = 0.0f;
			static bool lag = true;
			int loops = 2;
			int splits = 1000;
			int automatic = crash == 2;
			float autotimer = 1.0f;

			if (nag.is_open() && !input::get().is_key_down(0x31))
			{
				char tmp[128];
				memset(tmp, 0, 128);
				if (nag.getline(tmp, 128) && strlen(tmp) > 0)
					lagfor = (float)atof(tmp);
				if (nag.getline(tmp, 128) && strlen(tmp) > 0)
					dontlagfor = (float)atof(tmp);
				if (nag.getline(tmp, 128) && strlen(tmp) > 0)
					loops = atoi(tmp);
				if (nag.getline(tmp, 128) && strlen(tmp) > 0)
					splits = atoi(tmp);
				if (nag.getline(tmp, 128) && strlen(tmp) > 0)
					autotimer = atof(tmp);
			}

			float waittime = lag ? lagfor : dontlagfor;

			if (timeelapsed >= waittime)
			{
				lag = !lag;
				waittime = lag ? lagfor : dontlagfor;
				timeelapsed = 0.0f;
			}

			static bool autolagging = false;
			if (automatic)
			{
				//float flTimeout = chan->GetTimeoutSeconds();
				//float flRemainingTime = flTimeout - chan->GetTimeSinceLastReceived();

				INetChannel *chan = (INetChannel*)netchan;

				bool istimingout;
				if (chan->m_Timeout == -1.0f)
					istimingout = false;
				else
					istimingout = chan->last_received + autotimer < *net_time;

				float tm = QPCTime();

				DWORD lastreceived = (DWORD)&chan->last_received - (DWORD)chan;

				if (/*tm - chan->last_received > 3.0f*/ istimingout)
				{
					autolagging = false;
				}
				else
				{
					autolagging = true;
				}
				
			}
			else
			{
				autolagging = false;
			}
			

			if (autolagging || lag || input::get().is_key_down(0x31))
			{
				for (int i = 0; i < loops; ++i) //6000    //150 worked sometimes
				{
					//if (Held)
					//	break;


#if 0
					SPLITPACKET packets[150];

					for (int fucker = 0; fucker < ARRAYSIZE(packets); ++fucker)
					{
						CreateSplitPacket(&packets[fucker]);
					}
#endif

					//SPLITPACKET fuck;
					//fuck.netID = NET_HEADER_FLAG_SPLITPACKET;
					//fuck.nSplitSize = 1188;
					//if (rand() % 2 == 0)
						//fuck.packetID = INT_MAX;
					//else
						//fuck.packetID = 0;
					//fuck.sequenceNumber = 0;

					

					//char		msg_buffer[MAX_ROUTABLE_PAYLOAD];
					//bf_write	msg(msg_buffer, sizeof(msg_buffer));

					//msg.WriteLong(CONNECTIONLESS_HEADER);
					//msg.WriteByte('\0');

					NET_SendLong_Fucked(0, splits);

					//NET_SendLong(newdata, 1);

					//send split packet
				}
			}
			
			timeelapsed += min(time - lastcalltime, 5.0f);

			ShouldSetSplitPacketSize = 0;
			ShouldSetSplitPacketID = 0;

			//delete[] payload.m_pData;

			Held = true;
		}
		else
		{
			Held = false;
			timeelapsed = 0.0f;
		}

		maxroutable = 1260;
		sv_maxrouteable->fValue = *(DWORD*)&maxroutable ^ (DWORD)sv_maxrouteable;
		sv_maxrouteable->nValue = *(DWORD*)&maxroutable ^ (DWORD)sv_maxrouteable;
		net_maxroutable->fValue = *(DWORD*)&maxroutable ^ (DWORD)net_maxroutable;
		net_maxroutable->nValue = *(DWORD*)&maxroutable ^ (DWORD)net_maxroutable;

		delete[] newdata;
	}
	IsAlreadySending = false;
	lastcalltime = time;

	return ShouldKeepPacket;
}

int sendpacketstacksize;
void* OnSendPacketHookJmpBack;

__declspec (naked) void OnSendPacketHook()
{
	__asm
	{
		push ebp
		mov ebp, esp
		push ecx
		push edx
		push[ebp + 16]
		push[ebp + 12]
		push[ebp + 8]
		push edx
		push ecx
		call OnSendPacket
		test eax, eax
		pop edx
		pop ecx
		pop ebp
		jz dontcall

		push ebp
		mov ebp, esp
		mov eax, sendpacketstacksize
		jmp OnSendPacketHookJmpBack

		dontcall :
		retn
	}
}

void PlaceJMP(BYTE *bt_DetourAddress, DWORD dw_FunctionAddress, DWORD dw_Size)
{
	DWORD dw_OldProtection, dw_Distance;
	VirtualProtect(bt_DetourAddress, dw_Size, PAGE_EXECUTE_READWRITE, &dw_OldProtection);
	dw_Distance = (DWORD)(dw_FunctionAddress - (DWORD)bt_DetourAddress) - 5;
	*bt_DetourAddress = 0xE9;
	*(DWORD*)(bt_DetourAddress + 0x1) = dw_Distance;
	for (int i = 0x5; i < dw_Size; i++) *(bt_DetourAddress + i) = 0x90;
	VirtualProtect(bt_DetourAddress, dw_Size, dw_OldProtection, NULL);
	return;
}

DWORD __stdcall FuckServer(void*)
{
	return 0;
}

#ifdef SERVER_CRASHER

static std::atomic<bool> stoppedcommunications = false;
static DWORD processpacketentitiesaddress = 0;

#endif

#include "inetmsghandler.h"
#include "netmessages.h"

#define clc_VoiceData			10      // Voicestream data from a client

class CLC_VoiceData : public CNetMessage
{
public:
	DECLARE_CLC_MESSAGE(VoiceData);

	int	GetGroup() const { return INetChannelInfo::VOICE; }

	CLC_VoiceData() { m_bReliable = false; };

	//IClientMessageHandler *m_pMessageHandler;
	int m_nLength;
	bf_read m_DataIn;
	bf_write m_DataOut;
	uint64 m_xuid;
};

static char s_text[1024];
#define NETMSG_TYPE_BITS	5	// must be 2^NETMSG_TYPE_BITS > SVC_LASTMSG

const char *CLC_VoiceData::ToString(void) const
{
	Q_snprintf(s_text, sizeof(s_text), "%s: %i bytes", GetName(), Bits2Bytes(m_nLength));
	return s_text;
}

bool CLC_VoiceData::WriteToBuffer(bf_write &buffer)
{
	buffer.WriteUBitLong(GetType(), NETMSG_TYPE_BITS);

	m_nLength = m_DataOut.GetNumBitsWritten();

	buffer.WriteWord(m_nLength);	// length in bits

	//Send this client's XUID (only needed on the 360)
#if defined ( _X360 )
	buffer.WriteLongLong(m_xuid);
#endif

	return buffer.WriteBits(m_DataOut.GetBasePointer(), m_nLength);
}

template<typename T> FORCEINLINE T CallAddress(DWORD loc)
{
	if (!loc)
		return NULL;

	return (T)(loc + *(DWORD*)(loc + 0x1) + 0x5);
}

bool CLC_VoiceData::ReadFromBuffer(bf_read &buffer)
{
	// VPROF("CLC_VoiceData::ReadFromBuffer");

	m_nLength = buffer.ReadWord();	// length in bits

#if defined ( _X360 )
	m_xuid = buffer.ReadLongLong();
#endif

	m_DataIn = buffer;

	return buffer.SeekRelative(m_nLength);
}

#define LOBYTE_IDA(x)   (*((BYTE*)&(x)))   // low byte

class CNETMsg_NOP_t
{
	DWORD values[16];

public:

	CNETMsg_NOP_t()
	{
		static DWORD adr = FindMemoryPattern(EngineHandle, std::string("C7  44  24  ??  ??  ??  ??  ??  C7  44  24  ??  ??  ??  ??  ??  C6  44  24  ??  01  C7  84  24  ??  ??  ??  ??  0F  00  00  00  C7  44  24  ??  ??  ??  ??  00  C6  44  24  ??  00"));
		static DWORD vtable1 = *(DWORD*)(adr + 4);
		static DWORD vtable2 = *(DWORD*)(adr + 12);

		//values[0] = (int)&INetMessage::`vftable';
		//values[1] = (int)&CNETMsg_NOP::`vftable';
		values[2] = 0;
		values[3] = 0;
		values[4] = 0;
		values[0] = vtable1;// (int)&CNetMessagePB<0, CNETMsg_NOP, 0, 1>::`vftable';
		values[1] = vtable2;// (int)&CNetMessagePB<0, CNETMsg_NOP, 0, 1>::`vftable';
		LOBYTE_IDA(values[5]) = 1;
		values[11] = 15;
		values[10] = 0;
		LOBYTE_IDA(values[6]) = 0;
	}

	~CNETMsg_NOP_t()
	{
		static DWORD deconstruct = FindMemoryPattern(EngineHandle, std::string("53  56  57  8B  F9  8D  77  18"));
		reinterpret_cast<void(__thiscall*)(CNETMsg_NOP_t*)>(deconstruct)(this);
	}

	void WriteToBuffer(bf_write& buf)
	{
		static DWORD writetobuffer = FindMemoryPattern(EngineHandle, std::string("55  8B  EC  51  53  56  8B  F1  8B  4E  08"));
		reinterpret_cast<void(__thiscall*)(CNETMsg_NOP_t*, bf_write&)>(writetobuffer)(this, buf);
	}
};


int __fastcall Hooks::SendDatagram(void* netchan, void*, void *datagram)
{
	VMP_BEGINMUTILATION("SD")
		INetChannel* chan = (INetChannel*)netchan;
	bf_write* data = (bf_write*)datagram;
	FuckingChannel = chan;

	//*host_interval_per_tick = (1.0 / 768.0);
	//chan->m_nOutSequenceNr += 20;

	static int times = 0;
	static int tickstart = 0;
	static float nexttime = 0;
	static bool isplaying = false;

	corrupt_server = false;

	if (/*GetAsyncKeyState(VK_SHIFT) || */ input::get().is_key_down(VK_END))
	{
		corrupt_server = true;
		return chan->m_nOutSequenceNr;
	}
	else if (input::get().is_key_down(VK_HOME))
	{
	}

	if (!CL_SendMove)
	{
		CL_SendMove = StaticOffsets.GetOffsetValueByType<CL_SendMoveFn>(_CL_SendMove);
#ifdef SERVER_CRASHER

		CNetMsg_Tick_Constructor = StaticOffsets.GetOffsetValue(_CNetMsg_Tick_Constructor);//FindMemoryPattern(EngineHandle, XorStr("55  8B  EC  83  EC  08  56  57  F3  0F  11  5D  F8"), strlen(XorStr("55  8B  EC  83  EC  08  56  57  F3  0F  11  5D  F8")));
		CNetMsg_Tick_Destructor = StaticOffsets.GetOffsetValue(_CNetMsg_Tick_Destructor);//FindMemoryPattern(EngineHandle, XorStr("53  56  57  8B  F9  8D  77  2C  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  C7  46  14  0F  00  00  00  83  7E  14  10  C7  46  10  00  00  00  00  72  02  8B  36  C6  06  00  8D  4F  08  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  8D  4F  04  E8  ??  ??  ??  ??  C7  07  ??  ??  ??  ??  5F  5E  5B  C3  83  E9  04"), strlen(XorStr("53  56  57  8B  F9  8D  77  2C  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  C7  46  14  0F  00  00  00  83  7E  14  10  C7  46  10  00  00  00  00  72  02  8B  36  C6  06  00  8D  4F  08  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  8D  4F  04  E8  ??  ??  ??  ??  C7  07  ??  ??  ??  ??  5F  5E  5B  C3  83  E9  04")));
		CNetMsg_Tick_t_Setup = StaticOffsets.GetOffsetValue(_CNetMsg_Tick_Setup);//FindMemoryPattern(EngineHandle, XorStr("56  8B  F1  C7  06  ??  ??  ??  ??  8D  4E  08  C7  46  04  ??  ??  ??  ??  E8  ??  ??  ??  ??  C7  46  20  00  00  00  00  8B  C6  C7  46  0C  00  00  00  00  C7  46  10  00  00  00  00  C7  46  14  00  00  00  00  C7  46  18  00  00  00  00  C7  46  1C  00  00  00  00  C7  46  24  00  00  00  00  C7  06  ??  ??  ??  ??  C7  46  04  ??  ??  ??  ??  C6  46  28  01  C7  46  40  0F  00  00  00  C7  46  3C  00  00  00  00  C6  46  2C  00  5E  C3  55"), strlen(XorStr("56  8B  F1  C7  06  ??  ??  ??  ??  8D  4E  08  C7  46  04  ??  ??  ??  ??  E8  ??  ??  ??  ??  C7  46  20  00  00  00  00  8B  C6  C7  46  0C  00  00  00  00  C7  46  10  00  00  00  00  C7  46  14  00  00  00  00  C7  46  18  00  00  00  00  C7  46  1C  00  00  00  00  C7  46  24  00  00  00  00  C7  06  ??  ??  ??  ??  C7  46  04  ??  ??  ??  ??  C6  46  28  01  C7  46  40  0F  00  00  00  C7  46  3C  00  00  00  00  C6  46  2C  00  5E  C3  55")));

		CNetMsg_Tick_t_VFTable1 = *(DWORD*)(CNetMsg_Tick_Constructor + 0x26);
		CNetMsg_Tick_t_VFTable2 = *(DWORD*)(CNetMsg_Tick_Constructor + 0x2D);

		DWORD adr = StaticOffsets.GetOffsetValue(_CNetMsg_Tick_MiscAdr);//FindMemoryPattern(EngineHandle, XorStr("F3  0F  10  05  ??  ??  ??  ??  F3  0F  10  1D  ??  ??  ??  ??  F3  0F  10  15  ??  ??  ??  ??  51  F3  0F  11  04  24  8D  4D  A8"), strlen(XorStr("F3  0F  10  05  ??  ??  ??  ??  F3  0F  10  1D  ??  ??  ??  ??  F3  0F  10  15  ??  ??  ??  ??  51  F3  0F  11  04  24  8D  4D  A8")));
		host_framestarttime_std_deviation = (float*)*(DWORD*)(adr + 4);
		host_computationtime_std_deviation = (float*)*(DWORD*)(adr + 0xC);
		host_computationtime = (float*)*(DWORD*)(adr + 0x14);



		//remove clamping from 
		
		//remove assert spam


		//Hook 
		
		//Remove

		//CREATE THREAD
		unsigned int threadid;
		HANDLE thr = (HANDLE)_beginthreadex(nullptr, 0, (_beginthreadex_proc_type)&FuckServer, 0, 0, &threadid);
#endif
	}

	//Do not remove
	bool bBacktrackExploit = false;

	if (datagram)
		DisableCLC_Move = true;

	CL_SendMove_Rebuilt();

#ifdef SERVER_CRASHER

	float time = QPCTime();
	static float lasttimecalled = QPCTime();
	float deltatime = fmaxf(0.0f, fminf(time - lasttimecalled, 0.5f));
	std::ifstream dick("g:\\msg2.txt");
	int freeze = 0;
	int numpkts = 2;
	float freezeforsecs = 0.0f;
	float desiredping = 0.0f;
	float unfreezeforsecs = 0.0f;


	bool keydown = input::get().is_key_down(0x51);
	static ULONGLONG lasttime = 0;
	

	PING_IS_STILL_SPIKED = totaltimefrozen > 0.0f;

	if (totaltimefrozen > 0.0f && LocalPlayer.Entity)
	{
	}

	lasttimecalled = time;

#endif

	if ((!bBacktrackExploit /*&& !LocalPlayer.isastronaut && !LocalPlayer.isgabenewell*/) || datagram || DisableCLC_Move)
	{
		DisableCLC_Move = false;

#if 0
		bAllowNetTick = true;

		static void* tmp = malloc(2048);
		CNetMsg_Tick_Construct(tmp, *host_computationtime, *host_computationtime_std_deviation, g_ClientState->m_nDeltaTick, *host_framestarttime_std_deviation);
		chan->SendNetMsg(tmp, false, false);

		bAllowNetTick = false;
#endif


		static bool flip = false;

		//if (flip)
		//	g_ClientState->m_nDeltaTick = g_ClientState->m_ClockDriftMgr.m_nServerTick + chan->GetAvgLatency(FLOW_OUTGOING) + 1;

		flip = !flip;

		//Call original SendDatagram
		int ret = oSendDatagram(chan, data);

		return ret;
	}

	DisableCLC_Move = false;

	int backup_in_sequence_nr = chan->m_nInSequenceNr;
	LocalPlayer.Last_InSequenceNrSent = client_netchan.m_nInSequenceNr;

	if (bBacktrackExploit)
	{
		if (!NextPacketShouldBeRealPing)
		{
			int backtrackticks = TIME_TO_TICKS(FAKE_LATENCY_AMOUNT);
			if (backup_in_sequence_nr > backtrackticks)
				chan->m_nInSequenceNr -= backtrackticks;

			if (client_netchan.m_nInSequenceNr > backtrackticks)
			{
				LocalPlayer.Last_InSequenceNrSent = client_netchan.m_nInSequenceNr - backtrackticks;
			}
		}
	}

	NextPacketShouldBeRealPing = false;

	//if (LocalPlayer.isastronaut && NASAAlternativeChk.Checked)
	//	AlternativeNASAAntiaim(chan);

	//When airstucking, this causes a ridiculous exploit where you can teleport to full speed from 0 velocity
	//chan->m_nChokedPackets = 0;


	int out = chan->m_nOutSequenceNr;
	int ret;

	int originalchokedpackets = chan->m_nChokedPackets;
	double originalcleartime = chan->m_fClearTime;
	if (originalcleartime < *net_time)
		originalcleartime = *net_time;

	LocalPlayer.Last_OutSequenceNrSent = client_netchan.m_nOutSequenceNr;
	LocalPlayer.Last_ChokeCountSent = chan->m_nChokedPackets & 0xFF;

#if 0
	bAllowNetTick = true;

	static void* tmp = malloc(2048);
	CNetMsg_Tick_Construct(tmp, *host_computationtime, *host_computationtime_std_deviation, g_ClientState->m_nDeltaTick, *host_framestarttime_std_deviation);
	chan->SendNetMsg(tmp, false, false);

	bAllowNetTick = false;
#endif

	//Call original SendDatagram
	ret = oSendDatagram(chan, data);

	//rebuild packet size
	int nTotalSize = (int)((chan->m_fClearTime - originalcleartime) * chan->m_Rate);

	//Update our packet flows
	LocalPlayer.Last_Packet_Size = nTotalSize;

	FlowNewPacket(&our_netchan, FLOW_OUTGOING, ret, backup_in_sequence_nr, originalchokedpackets, 0, nTotalSize);
	FlowUpdate(&our_netchan, FLOW_OUTGOING, nTotalSize);

	FlowNewPacket(&client_netchan, FLOW_OUTGOING, client_netchan.m_nOutSequenceNr, LocalPlayer.Last_InSequenceNrSent, originalchokedpackets, 0, nTotalSize);
	FlowUpdate(&client_netchan, FLOW_OUTGOING, nTotalSize);

	client_netchan.m_nOutSequenceNr++;
	client_netchan.m_nChokedPackets = 0;

	chan->m_nInSequenceNr = backup_in_sequence_nr;

	return ret;
	VMP_END
}

#if 0
__declspec (naked) int __stdcall Hooks::SendDatagram(void* netchan, void *datagram)
{
	__asm
	{
		push dword ptr ss : [esp]
		push[esp + 8]
		push ecx
		call hSendDatagram
		retn 4
	}
}
#endif
