#pragma once
#ifdef USE_FAR_ESP
//#include <ws2tcpip.h>
#include "misc.h"
#include <deque>
#include "netadr.h"
#include <mutex>
#include "custompackettypes.h"
#include "PacketStructs.h"
#include <unordered_map>

enum SERVER_TYPE : unsigned char {
	COMMUNITY = 0,
	COMPETITIVE
};

struct hitboxinfo;
class KeepAlivePacket;
struct FarESPPlayer;

struct AsyncPacket
{
	AsyncPacket::AsyncPacket(char* b, uint32_t s) { buffer = b; targetpacketsize = s; }
	char* buffer;
	uint32_t targetpacketsize;
};

class FarESP
{
public:
	FarESP::FarESP()
	{
		Socket = INVALID_SOCKET;
		SendThreadIsDone = false;
		ReceiveThreadIsDone = false;
		SocketCreated = false;
		CreatingSocket = false;
		Exit = false;
		numplayersinpack = 0;
		unknownplayerhpiarmoristr = new char[30] { 47, 20, 17, 20, 21, 13, 20, 90, 42, 22, 27, 3, 31, 8, 90, 50, 42, 90, 95, 19, 90, 59, 8, 23, 21, 8, 90, 95, 19, 0 }; /*Unknown Player HP %i Armor %i*/
	}
	void OnCreateMove();
	void ClearClientToServerData();
	void OnNetUpdateEnd();
	void CreateFarESPThread();
	bool CreateSocket();
	void ShutdownSocket();
	bool PacketSizeIsKnown(uint32_t& numbytesinbuffer, char* buffer, uint32_t &targetpacketsize);
	bool InspectReceivedPacket(uint32_t& numbytesinbuffer, char* buffer, uint32_t &targetpacketsize);
	bool SendPacket(char *buffer, uint32_t totalsizeofpacket);
	SOCKET Socket;
	struct sockaddr socketaddress;
	netadr_t gamenetadr;
	uint64_t gamematchid;
	ULONGLONG LastSendAckTime;
	ULONGLONG LastReceiveAckTime;
	bool CheckIfReceiveThreadIsDone()
	{
		bool val;
		receivemutex.lock();
		val = ReceiveThreadIsDone;
		receivemutex.unlock();
		return val;
	}
	bool CheckIfSendThreadIsDone()
	{
		bool val;
		sendmutex.lock();
		val = SendThreadIsDone;
		sendmutex.unlock();
		return val;
	}
	void SetReceiveThreadIsDone(bool done)
	{
		receivemutex.lock();
		ReceiveThreadIsDone = done;
		receivemutex.unlock();
	}
	void SetSendThreadIsDone(bool done)
	{
		sendmutex.lock();
		SendThreadIsDone = done;
		sendmutex.unlock();
	}
	void SetShouldExit(bool exit)
	{
		exitmutex.lock();
		Exit = exit;
		exitmutex.unlock();
	}
	bool ShouldExit()
	{
		bool ret;
		exitmutex.lock();
		ret = Exit;
		exitmutex.unlock();
		return ret;
	}
	bool IsCreatingSocket()
	{
		bool ret;
		creatingsocketmutex.lock();
		ret = CreatingSocket;
		creatingsocketmutex.unlock();
		return ret;
	}
	void SetIsCreatingSocket(bool creating)
	{
		creatingsocketmutex.lock();
		CreatingSocket = creating;
		creatingsocketmutex.unlock();
	}
	bool IsSocketCreated()
	{
		bool ret;
		socketmutex.lock();
		ret = SocketCreated;
		socketmutex.unlock();
		return ret;
	}
	void SetSocketCreated(bool created)
	{
		socketmutex.lock();
		SocketCreated = created;
		socketmutex.unlock();
	}

	int numplayersinpack;
	FarESPPlayer packofplayers[MAX_PLAYERS];

	std::mutex packofplayersmutex;
	std::mutex farespmutex;
	std::unordered_map<uint32_t, FarESPPlayer>farespplayers;
	std::unordered_map<uint32_t, float>playerssenttoserver;
private:
	std::mutex sendmutex;
	std::mutex receivemutex;
	std::mutex exitmutex;
	std::mutex creatingsocketmutex;
	std::mutex socketmutex;
	bool SocketCreated;
	bool CreatingSocket;
	bool Exit;
	bool SendThreadIsDone;
	bool ReceiveThreadIsDone;
	char *unknownplayerhpiarmoristr;
};

extern FarESP g_FarESP;
extern std::atomic<bool>StartRetardThread;
extern int RetardThreadReturnValue;
#endif