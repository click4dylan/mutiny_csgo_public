#pragma once
#include "CUtlMemory.h"
#include "bfwrite.h"
#include "netadr.h"
#include "GOTV.h"
#include "inetmsghandler.h"

#define UDP_HEADER_SIZE 28


class Netmsgbinder;
class INetChannel;

#if 0
class INetChannelHandler
{
public:
	virtual	~INetChannelHandler(void) {};

	virtual void ConnectionStart(INetChannel *chan) = 0;	// called first time network channel is established

	virtual void ConnectionClosing(const char *reason) = 0; // network channel is being closed by remote site

	virtual void ConnectionCrashed(const char *reason) = 0; // network error occured

	virtual void PacketStart(int incoming_sequence, int outgoing_acknowledged) = 0;	// called each time a new packet arrived

	virtual void PacketEnd(void) = 0; // all messages has been parsed

	virtual void FileRequested(const char *fileName, unsigned int transferID) = 0; // other side request a file for download

	virtual void FileReceived(const char *fileName, unsigned int transferID) = 0; // we received a file

	virtual void FileDenied(const char *fileName, unsigned int transferID) = 0;	// a file request was denied by other side

	virtual void FileSent(const char *fileName, unsigned int transferID) = 0;	// we sent a file
};
#endif

#pragma pack(push, 1)
struct netframe_t_firstpart
{
	float time; //0
	int size; //4
	__int16 choked; //8
	bool valid; //10
	char pad; //11
	float latency; //12
};
#pragma pack(pop)

#pragma pack(push, 1)
struct netframe_t_secondpart
{
	int dropped; //16
	float avg_latency;
	float m_flInterpolationAmount;
	unsigned __int16 msggroups[16];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct netflow_t
{
	float nextcompute; //0
	float avgbytespersec; //4
	float avgpacketspersec; //8
	float avgloss; //12
	float avgchoke; //16
	float avglatency; //20
	float latency; //24
	int totalpackets; //28
	int totalbytes; //32
	int currentindex; //36
	netframe_t_firstpart frames[128]; //40
	netframe_t_secondpart frames2[128]; //2088
	netframe_t_firstpart *currentframe; //7720
};
#pragma pack(pop)

#define MAX_FLOWS 2
#define MAX_STREAMS 2
#define MAX_SUBCHANNELS		8		// we have 8 alternative send&wait bits
#define SUBCHANNEL_FREE		0	// subchannel is free to use
#define SUBCHANNEL_TOSEND	1	// subchannel has data, but not send yet
#define SUBCHANNEL_WAITING	2   // sbuchannel sent data, waiting for ACK
#define SUBCHANNEL_DIRTY	3	// subchannel is marked as dirty during changelevel

#define FRAGMENT_BITS		8
#define FRAGMENT_SIZE		(1<<FRAGMENT_BITS)
#define BYTES2FRAGMENTS(i) ((i+FRAGMENT_SIZE-1)/FRAGMENT_SIZE)
#define NET_MAX_PAYLOAD_BITS 19
#define MAX_FILE_SIZE_BITS 26
#define MAX_FILE_SIZE		((1<<MAX_FILE_SIZE_BITS)-1)	// maximum transferable size is	64MB

#define	FRAG_NORMAL_STREAM	0
#define FRAG_FILE_STREAM	1


typedef struct dataFragments_s
{
	DWORD			file;			// open file handle
	char			filename[MAX_OSPATH]; // filename
	char*			buffer;			// if NULL it's a file
	unsigned int	bytes;			// size in bytes
	unsigned int	bits;			// size in bits
	unsigned int	transferID;		// only for files
	bool			isCompressed;	// true if data is bzip compressed
	unsigned int	nUncompressedSize; // full size in bytes
	//bool			asTCP;			// send as TCP stream
	bool			isReplayDemo;	// if it's a file, is it a replay .dem file?
	int				numFragments;	// number of total fragments
	int				ackedFragments; // number of fragments send & acknowledged
	int				pendingFragments; // number of fragments send, but not acknowledged yet
} dataFragments_t;

struct subChannel_s
{
	int				startFraggment[MAX_STREAMS];
	int				numFragments[MAX_STREAMS];
	int				sendSeqNr;
	int				state; // 0 = free, 1 = scheduled to send, 2 = send & waiting, 3 = dirty
	int				index; // index in m_SubChannels[]

	void Free()
	{
		state = SUBCHANNEL_FREE;
		sendSeqNr = -1;
		for (int i = 0; i < MAX_STREAMS; i++)
		{
			numFragments[i] = 0;
			startFraggment[i] = -1;
		}
	}
};

#pragma pack(push, 1)
class INetChannel
{
public:
	virtual float GetTime();
	virtual float GetTimeConnected();
	virtual float GetTimeSinceLastReceived();
	virtual int GetDataRate();
	virtual int GetBufferSize();
	virtual bool IsLoopback();
	virtual bool IsNull();
	virtual bool IsTimingOut();
	virtual bool IsPlayback();
	virtual float GetLatency(int flow);
	virtual float GetAvgLatency(int flow);
	virtual float GetAvgLoss(int flow);
	virtual float GetAvgData(int flow);
	virtual float GetAvgChoke(int flow);
	virtual float GetAvgPackets(int flow);
	virtual int GetTotalData(int flow);
	virtual int GetSequenceNr(int flow);
	virtual bool IsValidPacket(int flow, int frame_number);
	virtual float GetPacketTime(int flow, int frame_number);
	virtual int	GetPacketBytes(int flow, int frame_number, int group);
	virtual bool GetStreamProgress(int flow, int *received, int *total);
	virtual float GetCommandInterpolationAmount(int flow, int frame_number);
	virtual void GetPacketResponseLatency(int flow, int frame_number, int *pnLatencyMsecs, int *pnChoke);
	virtual void GetRemoteFramerate(float *pflFrameTime, float *pflFrameTimeStdDeviation);
	virtual float GetTimeoutSeconds();
	virtual void Func25();
	virtual void Func26();
	virtual void Func27();
	virtual void Func28();
	virtual void Func29();
	virtual void Func30();
	virtual void Func31();
	virtual void Func32();
	virtual void Func33();
	virtual void Func34();
	virtual void Func35();
	virtual void Func36();
	virtual void Func37();
	virtual void Func38();
	//virtual void Func39();
	//virtual void Func40();
	virtual void ProcessPacket(netpacket_t* packet, bool bHasHeader);
	virtual void SendNetMsg(void* msg, bool bForceReliable, bool bVoice);
	virtual void Func41();
	virtual void Func42();
	virtual void Func43();
	virtual void Func44();
	virtual void Func45();
	virtual int SendDatagram(void* datagram);
	virtual bool Transmit(bool bOnlyReliable);
	virtual void Func48();
	virtual void Func49();
	virtual void Func50();
	virtual void Func51();
	virtual void Func52();
	virtual void Func53();
	virtual void Func54();
	virtual void Func55();
	virtual bool CanPacket();
#if 0
	virtual void Func57();
	virtual void Func58();
	virtual void Func59();
	virtual void Func60();
	virtual void Func61();
	virtual void Func62();
	virtual void Func63();
	virtual void Func64();
	virtual void Func65();
	virtual void Func66();
	virtual void Func67();
	virtual void Func68();
	virtual void Func69();
	virtual void Func70();
	virtual void Func71();
	virtual void Func72();
	virtual void Func73();
	virtual void Func74();
	virtual void Func75();
	virtual void Func76();
	virtual int QueuePacketFunc(char* packet, size_t size, int delay);
#endif

	//__int32 vtable; //0x0000 
	Netmsgbinder* msgbinder1; //0x0004 
	Netmsgbinder* msgbinder2; //0x0008 
	Netmsgbinder* msgbinder3; //0x000C 
	Netmsgbinder* msgbinder4; //0x0010 
	unsigned char m_bProcessingMessages; //0x0014 
	unsigned char m_bShouldDelete; //0x0015 
	char pad_0x0016[0x2]; //0x0016
	__int32 m_nOutSequenceNr; //0x0018 
	__int32 m_nInSequenceNr; //0x001C 
	__int32 m_nOutSequenceNrAck; //0x0020 
	__int32 m_nOutReliableState; //0x0024 
	__int32 m_nInReliableState; //0x0028 
	__int32 m_nChokedPackets; //0x002C 

	bf_write m_StreamReliable; //0x0030 
	/*CUtlMemory*/ char m_ReliableDataBuffer[12]; //0x0048 
	bf_write m_StreamUnreliable; //0x0054 
	/*CUtlMemory*/ char m_UnreliableDataBuffer[12]; //0x006C 
	bf_write m_StreamVoice; //0x0078 
	/*CUtlMemory*/char m_VoiceDataBuffer[12]; //0x0090 
	__int32 m_Socket; //0x009C 
	//__int32 m_StreamSocket; //0x00A0 
	__int32 m_MaxReliablePayloadSize; //0x00A4  //0x00A0 2020
	char pad_0x00A8[0x4]; //0x00A8
	netadr_t remote_address; //0x00AC //0xA8 2020
	char dylanpadding[88]; //padding added by dylan
	float last_received; //2018 0x10C
	//float last_received_pad;
	//char pad_0x00BC[0x4]; //0x00BC
	double /*float*/ connect_time; //0x00C0 //dylan found 0x110
	//char pad_0x00C4[0x4]; //0x00C4
	__int32 m_Rate;
	__int32 m_RatePad;
	/*float*/double m_fClearTime; //0x128 not anymore
	CUtlVector<dataFragments_t*>	m_WaitingList[2]; //0x128 as of 2020
	char pad_blehch[0x260]; //0x150
	subChannel_s					m_SubChannels[MAX_SUBCHANNELS]; //0x3B0 as of 2020
	char pad_blech2[8]; //0x490 as of 2020

//#if ENGINE_BUILD_VERSION >= 13635
	char NEWPAD2018[4];
//#endif
	netflow_t m_DataFlow[MAX_FLOWS]; //new 2018 0x49C //0x5C0
	int	m_MsgStats[16];	// total bytes for each message group
	__int32 m_PacketDrop; //0x4220  //dylan found 0x4250 new 0x4258 newnew 0x425C
	//char m_UnkPad[4];
	char m_Name[32]; //0x4224 
	__int32 m_ChallengeNr; //0x4244 
	float m_Timeout; //0x4280
	INetChannelHandler* m_MessageHandler; //0x4284   0x4288
	/*CUtlVector*/char m_NetMessages[16]; //dylan found 0x4284
	__int32 dylanUnknown;
	void* m_pDemoRecorder; //0x429C
	//__int32 dylanUnknown923874;
	//__int32 m_nQueuedPackets; //0x4268  //dylan found 0x4298
	float m_flInterpolationAmount; //0x42A0
	double m_flRemoteFrameTime; //0x42A4
	float m_flRemoteFrameTimeStdDeviation; //0x42AC
	__int32 m_nMaxRoutablePayloadSize; //0x42B0
	__int32 m_nSplitPacketSequence; //dylan found 0x42b4
	char pad_0x4280[0x14]; //0x4280
};//Size=0x4294
#pragma pack(pop)

//Rebuilt functions

extern INetChannel our_netchan;
extern INetChannel server_netchan;
extern INetChannel client_netchan;

void FlowNewPacket(INetChannel *chan, int flow, int seqnr, int acknr, int nChoked, int nDropped, int nSize);
void FlowUpdate(INetChannel* chan, int flow, int addbytes);
