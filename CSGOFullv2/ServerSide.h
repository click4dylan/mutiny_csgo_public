#pragma once
#ifdef USE_SERVER_SIDE
#include "misc.h"
#include <deque>
#include "netadr.h"
#include <mutex>
#include "custompackettypes.h"

struct hitboxinfo;
class KeepAlivePacket;
class CSGOPacket;

extern std::deque<CSGOPacket>serverpackets;
extern std::mutex packetmutex;

class ServerSide
{
public:
	ServerSide::ServerSide()
	{
		Socket = INVALID_SOCKET;
		SendThreadIsDone = false;
		ReceiveThreadIsDone = false;
		SocketCreated = false;
		CreatingSocket = false;
		Exit = false;
	}
	void OnCreateMove();
	void CreateThread();
	bool CreateSocket();
	SOCKET Socket;
	struct sockaddr socketaddress;
	netadr_t gamenetadr;
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
};

extern ServerSide pServerSide;
#endif