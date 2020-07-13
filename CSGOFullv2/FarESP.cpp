#ifdef USE_FAR_ESP

#include <ws2tcpip.h>
#include <Windows.h> //NOTE: DO NOT MOVE EITHER OF THESE OR FACE THE WRATH OF C++ COMPILER

#include "precompiled.h"
#include "FarESP.h"
#include <iostream>
#include <streambuf>
#include <fstream>
#include "LocalPlayer.h"
#include "raw_buffer.h"
#include "compression.h"

#include "VTHook.h"
#include "Netchan.h"
//Other libs
//#include "Ws2tcpip.h"
//#pragma comment(lib,"ws2_32.lib") //Winsock Library
#include <process.h>
#include "Reporting.h"

#include "Adriel/stdafx.hpp"

FarESP g_FarESP;
extern HANDLE CreateThreadSafe(const LPTHREAD_START_ROUTINE func, const LPVOID lParam);
std::atomic<bool> StartRetardThread = false;

DWORD __stdcall DoFarESPReceiving(void* arg);

HMODULE GetWS2DLL()
{
	//decrypts(0)
	static HANDLE winsockmodule = GetModuleHandleA(XorStr("Ws2_32.dll"));
	//encrypts(0)
	return (HMODULE)winsockmodule;
}

int GetAddressInfo(PCSTR pNodeName, PCSTR pServiceName, const ADDRINFOA * pHints, PADDRINFOA* ppResult)
{
#ifndef PRODUCTION
	return getaddrinfo(pNodeName, pServiceName, pHints, ppResult);
#else
	auto handle = GetWS2DLL();
	//decrypts(0)
	static auto getaddrinfoFn = GetProcAddress((HMODULE)handle, XorStr("getaddrinfo"));
	//encrypts(0)
	return ((int(__stdcall*)(PCSTR, PCSTR, const ADDRINFOA *, PADDRINFOA*))getaddrinfoFn)(pNodeName, pServiceName, pHints, ppResult);
#endif
}

void FreeAddressInfo(PADDRINFOA pAddrInfo)
{
#ifndef PRODUCTION
	freeaddrinfo(pAddrInfo);
#else
	auto handle = GetWS2DLL();
	//decrypts(0)
	static auto freeaddrinfofn = GetProcAddress((HMODULE)handle, XorStr("freeaddrinfo"));
	//encrypts(0)
	return ((void(__stdcall*)(PADDRINFOA))freeaddrinfofn)(pAddrInfo);
#endif
}

int WSAStart(WORD wVersionRequired, LPWSADATA lpWSAData)
{
#ifndef PRODUCTION
	return WSAStartup(wVersionRequired, lpWSAData);
#else
	auto handle = GetWS2DLL();
	//decrypts(0)
	static auto wsastartupfn = GetProcAddress((HMODULE)handle, XorStr("WSAStartup"));
	//encrypts(0)
	return ((int(__stdcall*)(WORD, LPWSADATA))wsastartupfn)(wVersionRequired, lpWSAData);
#endif
}

int WSAClean()
{
#ifndef PRODUCTION
	return WSACleanup();
#else
	auto handle = GetWS2DLL();
	//decrypts(0)
	static auto wsacleanupfn = GetProcAddress((HMODULE)handle, XorStr("WSACleanup"));
	//encrypts(0)
	return ((int(*)())wsacleanupfn)();
#endif
}

SOCKET CreateWSASocket(int af, int type, int protocol)
{
#ifndef PRODUCTION
	return socket(af, type, protocol);
#else
	auto handle = GetWS2DLL();
	//decrypts(0)
	static auto socketfn = GetProcAddress((HMODULE)handle, XorStr("socket"));
	//encrypts(0)
	return ((int(__stdcall*)(int, int, int))socketfn)(af, type, protocol);
#endif
}

int CloseWSASocket(SOCKET s)
{
#ifndef PRODUCTION
	return closesocket(s);
#else
	auto handle = GetWS2DLL();
	//decrypts(0)
	static auto closesocketfn = GetProcAddress((HMODULE)handle, XorStr("closesocket"));
	//encrypts(0)
	return ((int(__stdcall*)(SOCKET))closesocketfn)(s);
#endif
}

int SetSockOpt(SOCKET s, int level, int optname, const char *optval, int optlen)
{
#ifndef PRODUCTION
	return setsockopt(s, level, optname, optval, optlen);
#else
	auto handle = GetWS2DLL();
	//decrypts(0)
	static auto setsockoptfn = GetProcAddress((HMODULE)handle, XorStr("setsockopt"));
	//encrypts(0)
	return ((int(__stdcall*)(SOCKET, int, int, const char*, int))setsockoptfn)(s, level, optname, optval, optlen);
#endif
}

int Recv(SOCKET s, char *buf, int len, int flags)
{
#ifndef PRODUCTION
	return recv(s, buf, len, flags);
#else
	auto handle = GetWS2DLL();
	//decrypts(0)
	static auto recvfn = GetProcAddress((HMODULE)handle, XorStr("recv"));
	//encrypts(0)
	return ((int(__stdcall*)(SOCKET, char*, int, int))recvfn)(s, buf, len, flags);
#endif
}

int Send(SOCKET s, const char* buf, int len, int flags)
{
#ifndef PRODUCTION
	return send(s, buf, len, flags);
#else
	auto handle = GetWS2DLL();
	//decrypts(0)
	static auto sendfn = GetProcAddress((HMODULE)handle, XorStr("send"));
	//encrypts(0)
	return ((int(__stdcall*)(SOCKET, const char*, int, int))sendfn)(s, buf, len, flags);
#endif
}

int Connect(SOCKET s, const sockaddr *name, int namelen)
{
#ifndef PRODUCTION
	return connect(s, name, namelen);
#else
	auto handle = GetWS2DLL();
	//decrypts(0)
	static auto connectfn = GetProcAddress((HMODULE)handle, XorStr("connect"));
	//encrypts(0)
	return ((int(__stdcall*)(SOCKET, const sockaddr *, int))connectfn)(s, name, namelen);
#endif
}

int Shutdown(SOCKET s, int how)
{
#ifndef PRODUCTION
	return shutdown(s, how);
#else
	auto handle = GetWS2DLL();
	//decrypts(0)
	static auto shutdownfn = GetProcAddress((HMODULE)handle, XorStr("shutdown"));
	//encrypts(0)
	return ((int(__stdcall*)(SOCKET,int))shutdownfn)(s, how);
#endif
}

#include "Adriel/adr_util.hpp"
#include "LocalPlayer.h"

void FarESP::OnCreateMove()
{
#ifdef FAR_ESP_LOGGING
	//decrypts(0)
	std::ofstream file(XorStr("faresp.txt"), std::ofstream::out | std::ofstream::app);
	file << XorStr("OnCreateMove\n");
	//encrypts(0)
	file.close();
#endif

	INetChannel *netchan = (INetChannel*)g_ClientState->m_pNetChannel;
	if (IsSocketCreated())
	{
		if (netchan)
		{
			if (!IsCreatingSocket() && !ShouldExit() && !netchan->remote_address.CompareAdr(gamenetadr) && !Helper_GetLastCompetitiveMatchId())
			{
				SetShouldExit(true);
				ClearClientToServerData();
			}
		}
		else if (!ShouldExit())
		{
			SetShouldExit(true);
			ClearClientToServerData();
		}
	}
	else if (netchan)
	{
		if (variable::get().visuals.pf_enemy.i_faresp > 0 && !IsCreatingSocket() && (Helper_GetLastCompetitiveMatchId() || (netchan->remote_address.IsValid() && !netchan->remote_address.IsLoopback() && !netchan->remote_address.IsReservedAdr())))
		{
			CreateFarESPThread();
		}
	}

	farespmutex.lock();
	if (!farespplayers.empty())
	{
		float flSimulationTime = LocalPlayer.Entity->GetSimulationTime();
		for (auto storedplayer = farespplayers.begin(); storedplayer != farespplayers.end(); )
		{
			bool found = false;
			//Always loop through the entire entity list because if a player disconnects and reconnects they could be in a different slot
			for (CPlayerrecord *pCPlayer = &m_PlayerRecords[1]; pCPlayer != &m_PlayerRecords[MAX_PLAYERS + 1]; ++pCPlayer)
			{
				if (pCPlayer->hashedsteamid == storedplayer->second.hashedsteamid)
				{
					if (pCPlayer->m_bConnected && pCPlayer->m_pEntity)
					{
						found = true;
						//std::string name = adr_util::sanitize_name((char*)pCPlayer->m_PlayerInfo.name);

						if (pCPlayer->m_pEntity != LocalPlayer.Entity)
						{
							pCPlayer->faresprecordmutex.lock();
							if (pCPlayer->FarESPPackets.empty() || pCPlayer->FarESPPackets.at(0)->simulationtime != storedplayer->second.simulationtime)
							{
								pCPlayer->FarESPPackets.push_front(new FarESPPlayer(storedplayer->second));
								pCPlayer->m_nLastFarESPPacketServerTickCount = g_ClientState->m_ClockDriftMgr.m_nServerTick;
							}
							while (pCPlayer->FarESPPackets.size() > 3)
							{
								delete pCPlayer->FarESPPackets.back();
								pCPlayer->FarESPPackets.pop_back();
							}
							pCPlayer->faresprecordmutex.unlock();
						}
					}
				}
			}
			if (!found)
			{
				//This player doesn't exist in our entity table yet
//#if _DEBUG
				DecStr(unknownplayerhpiarmoristr, 29);
				Interfaces::DebugOverlay->AddTextOverlay(storedplayer->second.absorigin, Interfaces::Globals->interval_per_tick * 2.0f, unknownplayerhpiarmoristr, storedplayer->second.health, storedplayer->second.armor);
				EncStr(unknownplayerhpiarmoristr, 29);
//#endif
			}
			//Erase outdated players
			if (fabsf(storedplayer->second.simulationtime - flSimulationTime) <= 1.0f)
				storedplayer++;
			else
				storedplayer = farespplayers.erase(storedplayer);
		}
	}
	farespmutex.unlock();
}

void FarESP::ShutdownSocket()
{
#ifdef FAR_ESP_LOGGING
	//decrypts(0)
	std::ofstream file(XorStr("faresp.txt"), std::ofstream::out | std::ofstream::app);
	file << XorStr("ShutdownSocket\n");
	//encrypts(0)
	file.close();
#endif

#ifdef _DEBUG
	printf("Client Closing Far ESP socket..\n");
#endif
	int iResult = Shutdown(Socket, 2); //SD_BOTH = 2, SD_SEND = 1, SD_RECEIVE = 0
	CloseWSASocket(Socket);
	WSAClean();
	Socket = INVALID_SOCKET;
	Sleep(1000);
	SetSocketCreated(false);
	SetShouldExit(false);
}

void FarESP::CreateFarESPThread()
{
#ifdef _DEBUG
	printf("FarESP::CreateFarESPThread\n");
#endif

#ifdef FAR_ESP_LOGGING
	//decrypts(0)
	std::ofstream file(XorStr("faresp.txt"), std::ofstream::out | std::ofstream::app);
	file << XorStr("CreateThread\n");
	//encrypts(0)
	file.close();
#endif

	SetIsCreatingSocket(true);

#if 0
	std::thread thr(DoFarESPReceiving, nullptr);
	thr.detach();
#else
	unsigned int threadid;
	//HANDLE thr = CreateThreadSafe(DoFarESPReceiving, nullptr);
	HANDLE thr = (HANDLE)_beginthreadex(nullptr, 0, (_beginthreadex_proc_type)&DoFarESPReceiving, nullptr, 0, &threadid);
	if (thr)
		CloseHandle(thr);
	else
	{
#ifdef FAR_ESP_LOGGING
		//decrypts(0)
		MessageBoxA(NULL, XorStr("Failed to create Far ESP Thread"), "", MB_OK);
		//encrypts(0)
#endif
		SetIsCreatingSocket(false);
		SetShouldExit(false);
	}
#endif
}

char *masterserverstr = new char[23]{ 28, 27, 8, 31, 9, 10, 84, 20, 19, 29, 18, 14, 28, 19, 8, 31, 10, 25, 84, 25, 21, 23, 0 }; /*faresp.nightfirepc.com*/
char *masterserverportstr = new char[5]{ 66, 74, 74, 66, 0 }; /*8008*/

bool FarESP::CreateSocket()
{
#ifdef FAR_ESP_LOGGING
	//decrypts(0)
	std::ofstream file(XorStr("faresp.txt"), std::ofstream::out | std::ofstream::app);
	file << XorStr("CreateSocket\n");
	//encrypts(0)
	file.close();
#endif

	INetChannel *netchan = (INetChannel*)g_ClientState->m_pNetChannel;

	Socket = INVALID_SOCKET;

	int iResult;

#ifdef _DEBUG
	printf("Client attempting connection to Far ESP server..\n");
#endif

	// Initialize Winsock
	WSADATA fw;
	iResult = WSAStart(MAKEWORD(2, 2), &fw);
	
	if (iResult != 0)
	{
		char tmp[128];
		//decrypts(0)
		sprintf(tmp, XorStr("CS %i"), iResult /*WSAGetLastError()*/);
		//encrypts(0)
		MessageBoxA(NULL, tmp, "", MB_OK);

		Socket = INVALID_SOCKET;
#ifdef _DEBUG
		printf("WSAStartup failed with error: %d\n", iResult);
#endif
		SetIsCreatingSocket(false);
		SetSocketCreated(false);
		return 0;
	}
	else
	{
#ifdef FAR_ESP_LOGGING
		//decrypts(0)
		MessageBoxA(NULL, XorStr("FarESP successfully called WSAStartup"), "", MB_OK);
		//encrypts(0)
#endif
	}

	struct addrinfo *result = NULL, *ptr = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	DecStr(masterserverstr, 22);
	DecStr(masterserverportstr, 4);

	iResult = GetAddressInfo(/*"faresp.nightfirepc.com"*/ masterserverstr, masterserverportstr, &hints, &result);
	
	EncStr(masterserverportstr, 4);
	EncStr(masterserverstr, 22);
	if (iResult != 0)
	{
#ifdef _DEBUG
		printf("getaddrinfo failed with error: %d\n", iResult);
#endif
#ifdef FAR_ESP_LOGGING
		//decrypts(0)
		MessageBoxA(NULL, XorStr("getaddrinfo failed"), "", MB_OK);
		//encrypts(0)
#endif
		WSAClean();
		SetIsCreatingSocket(false);
		SetSocketCreated(false);
		return 0;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{
		// Create a SOCKET for connecting to server
		Socket = CreateWSASocket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (Socket == INVALID_SOCKET) {
#ifdef _DEBUG
			printf("socket failed with error: %ld\n", WSAGetLastError());
#endif
#ifdef FAR_ESP_LOGGING
			//decrypts(0)
			MessageBoxA(NULL, XorStr("socket failed"), "", MB_OK);
			//encrypts(0)
#endif
			FreeAddressInfo(result);
			WSAClean();
			SetIsCreatingSocket(false);
			SetSocketCreated(false);
			return 0;
		}

		// Connect to server.
		iResult = Connect(Socket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
#ifdef FAR_ESP_LOGGING
			//decrypts(0)
			MessageBoxA(NULL, XorStr("connect failed"), "", MB_OK);
			//encrypts(0)
#endif
			CloseWSASocket(Socket);
			Socket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	FreeAddressInfo(result);


	if (Socket == INVALID_SOCKET) {
#ifdef _DEBUG
		printf("Unable to connect to Far ESP server!\n");
#endif
		WSAClean();
		SetIsCreatingSocket(false);
		SetSocketCreated(false);
		return 0;
	}

	socketaddress = *ptr->ai_addr;
	gamematchid = Helper_GetLastCompetitiveMatchId();
	gamenetadr = netchan->remote_address;

	DWORD timeoutmilliseconds = 2000;

	if (SetSockOpt(Socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeoutmilliseconds, sizeof(DWORD)) == SOCKET_ERROR)
	{
#ifdef _DEBUG
		printf("setsockopt failed\n");
#endif
#ifdef FAR_ESP_LOGGING
		//decrypts(0)
		MessageBoxA(NULL, XorStr("setsockopt failed 1"), "", MB_OK);
		//encrypts(0)
#endif
		Socket = INVALID_SOCKET;
		SetIsCreatingSocket(false);
		SetSocketCreated(false);
		return 0;
	}

	if (SetSockOpt(Socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeoutmilliseconds, sizeof(DWORD)) == SOCKET_ERROR)
	{
#ifdef _DEBUG
		printf("setsockopt failed\n");
#endif
#ifdef FAR_ESP_LOGGING
		//decrypts(0)
		MessageBoxA(NULL, XorStr("setsockopt failed 2"), "", MB_OK);
		//encrypts(0)
#endif
		Socket = INVALID_SOCKET;
		SetIsCreatingSocket(false);
		SetSocketCreated(false);
		return 0;
	}

#if 0
	int nodelay = 1;
	if (SetSockOpt(Socket, IPPROTO_TCP, TCP_NODELAY, (PCHAR)&nodelay, sizeof(nodelay)) == SOCKET_ERROR)
	{
#ifdef _DEBUG
		printf("setsockopt failed\n");
#endif
#ifdef FAR_ESP_LOGGING
		//decrypts(0)
		MessageBoxA(NULL, XorStr("setsockopt failed 3"), "", MB_OK);
		//encrypts(0)
#endif
		Socket = INVALID_SOCKET;
		SetIsCreatingSocket(false);
		SetSocketCreated(false);
		return 0;
	}
#endif

	// Send an initial buffer
	char FAR_ESP_VERSION_STR[11] = { 38, 12, 31, 8, 9, 19, 21, 20, 38, 66, 0 }; /*\version\8*/




	DecStr(FAR_ESP_VERSION_STR, 10);
	size_t lengthofstring = strlen(FAR_ESP_VERSION_STR) + 1;
	SmallForwardBuffer buf(lengthofstring + sizeof(SERVER_TYPE) + sizeof(netadr_t));
	buf.write((void*)FAR_ESP_VERSION_STR, lengthofstring);
	if (gamematchid != 0)
	{
		buf.write(COMPETITIVE); //type of server
		buf.write(gamematchid);
		buf.write((unsigned short)USHRT_MAX); //fills the packet to the same size as if we sent netadr_t so we don't have to do any special checks on the server
	}
	else
	{
		buf.write(COMMUNITY); //type of server
		buf.write(&gamenetadr, sizeof(netadr_t));
	}
	EncStr(FAR_ESP_VERSION_STR, 10);

	iResult = Send(Socket, (const char*)buf.getdata(), buf.getsize(), 0);

	if (iResult <= 0)
	{
#ifdef _DEBUG
		printf("send failed with error: %d\n", WSAGetLastError());
#endif
#ifdef FAR_ESP_LOGGING
		//decrypts(0)
		MessageBoxA(NULL, XorStr("send failed"), "", MB_OK);
		//encrypts(0)
#endif
		CloseWSASocket(Socket);
		WSAClean();
		Socket = INVALID_SOCKET;
		SetIsCreatingSocket(false);
		SetSocketCreated(false);
		return 0;
	}

#ifdef _DEBUG
	printf("Client connected to Far ESP server, bytes sent: %ld\n", iResult);
#endif
	SetSocketCreated(true);
	SetIsCreatingSocket(false);
	return 1;
}

void FarESP::ClearClientToServerData()
{
#ifdef FAR_ESP_LOGGING
	//decrypts(0)
	std::ofstream file(XorStr("faresp.txt"), std::ofstream::out | std::ofstream::app);
	file << XorStr("ClearClientToServerData\n");
	//encrypts(0)
	file.close();
#endif

	packofplayersmutex.lock();
	playerssenttoserver.clear();
	numplayersinpack = 0;
	packofplayersmutex.unlock();
}

//Generate a player packet to send to the far esp server
char* GenerateOutgoingPlayerPacket(uint32_t& totalpacketsize)
{
#ifdef FAR_ESP_LOGGING
	//decrypts(0)
	std::ofstream file(XorStr("faresp.txt"), std::ofstream::out | std::ofstream::app);
	file << XorStr("GenerateOutgoingPlayerPacket\n");
	//encrypts(0)
	file.close();
#endif

	g_FarESP.packofplayersmutex.lock();

	uint32_t numplayers = g_FarESP.numplayersinpack;
	size_t packsize = sizeof(FarESPPlayer) * numplayers;
	SmallForwardBuffer buffer(packsize);
	tcp_compressor compressedpack;
	uint32_t cpacksize;
	auto& playerssenttoserver = g_FarESP.playerssenttoserver;

	if (playerssenttoserver.empty() || numplayers <= 0)
	{
		//we haven't sent any players yet
		playerssenttoserver.clear();

		//tell the server this is a full player list
		buffer.write<uint8_t>(FARESP_PLAYERPACKET_TYPE::FULL);

		buffer.write<uint8_t>(numplayers);

		if (numplayers > 0)
			//now write the full packet of players
			buffer.write((char*)g_FarESP.packofplayers, packsize);

		//now compress the data
		compressedpack.compress((char*)buffer.getdata(), buffer.getsize());
		cpacksize = (uint32_t)compressedpack.get_data().size();

		if (numplayers > 0)
		{
			FarESPPlayer *endofarray = &g_FarESP.packofplayers[numplayers];
			for (FarESPPlayer* playerinpack = g_FarESP.packofplayers; playerinpack != endofarray; playerinpack++)
			{
				playerssenttoserver.emplace(playerinpack->hashedsteamid, playerinpack->simulationtime);
			}
		}
	}
	else
	{
		//remove dormant players from our sent list
		for (auto i = playerssenttoserver.begin(); i != playerssenttoserver.end(); )
		{
			uint32_t hashedsteamid = i->first;
			auto entry = std::find_if(std::begin(g_FarESP.packofplayers), &g_FarESP.packofplayers[numplayers], [hashedsteamid](const FarESPPlayer&  p) { return p.hashedsteamid == hashedsteamid; });
			if (entry != &g_FarESP.packofplayers[numplayers])
				++i; //player is still not dormant
			else
				i = playerssenttoserver.erase(i); //player is dormant
		}

		//tell the server this is a partial player list
		buffer.write<uint8_t>(FARESP_PLAYERPACKET_TYPE::PARTIAL);

		//tell the server the # of players we are sending
		buffer.write<uint8_t>(numplayers);

		for (FarESPPlayer* pPlayer = &g_FarESP.packofplayers[0]; pPlayer != &g_FarESP.packofplayers[numplayers]; ++pPlayer)
		{
			uint32_t hashedsteamid = pPlayer->hashedsteamid;
			auto previouslysentplayer = std::find_if(playerssenttoserver.begin(), playerssenttoserver.end(), [hashedsteamid](std::pair<uint32_t, float> p) { return p.first == hashedsteamid; });
			if (previouslysentplayer != playerssenttoserver.end())
			{
				//check to see if we need to send the far esp server an update about this player
				if (previouslysentplayer->second != pPlayer->simulationtime)
				{
					buffer.write<uint8_t>(1); //update
					buffer.write(pPlayer, sizeof(FarESPPlayer)); //player
					previouslysentplayer->second = pPlayer->simulationtime;
				}
				else
				{
					//we don't need to update the far esp server, just tell it that we can see it
					buffer.write<uint8_t>(0); //no update
					buffer.write<uint32_t>(hashedsteamid); //FIXME: we could send 3 less bytes per player if we change the protocol to use ent indexes instead of hashed steam ids!
				}
			}
			else
			{
				//we haven't told the far esp server about this player yet, do so now
				buffer.write<uint8_t>(1); //update
				buffer.write(pPlayer, sizeof(FarESPPlayer)); //player
				playerssenttoserver.emplace(hashedsteamid, pPlayer->simulationtime);
			}
		}

		//now, compress the data or return if there's nothing to send
		size_t uncompressedsize = buffer.getsize();
		if (uncompressedsize)
		{
			compressedpack.compress((char*)buffer.getdata(), uncompressedsize);
			cpacksize = (uint32_t)compressedpack.get_data().size();
		}
		else
		{
			//nothing to send
			g_FarESP.packofplayersmutex.unlock();
			return nullptr;
		}
	}

	g_FarESP.packofplayersmutex.unlock();

	FARESP_PACKET_TYPE packettype = CLIENTTOSERVER;
	
	totalpacketsize = sizeof(uint32_t) + sizeof(packettype) + sizeof(cpacksize) + cpacksize;

	SmallForwardBufferNoFreeOnExitScope finalbuffer(totalpacketsize);
	finalbuffer.write(totalpacketsize);
	finalbuffer.write(packettype);
	finalbuffer.write(cpacksize);
	if (cpacksize)
	{
		finalbuffer.write((void*)compressedpack.get_data().c_str(), cpacksize);
	}
	
	return (char*)finalbuffer.getdata();
}

//Net update was received from game server, update player cache that far esp sends to the server
char *botstr = new char[4]{ 56, 53, 46, 0 }; /*BOT*/
void FarESP::OnNetUpdateEnd()
{
#ifdef FAR_ESP_LOGGING
	//decrypts(0)
	std::ofstream file(XorStr("faresp.txt"), std::ofstream::out | std::ofstream::app);
	file << XorStr("OnNetUpdateEnd\n");
	//encrypts(0)
	file.close();
#endif

	LocalPlayer.Get(&LocalPlayer);

	static uint32_t bothash;
	static uint32_t gotvhash = HashString_MUTINY("GOTV");
	static bool hashedbotstring = false;
	if (!hashedbotstring)
	{
		DecStr(botstr, 3);
		bothash = HashString_MUTINY(botstr);
		EncStr(botstr, 3);
		hashedbotstring = true;
	}

	packofplayersmutex.lock();
	numplayersinpack = 0;
	const auto endofarray = &m_PlayerRecords[MAX_PLAYERS];
	for (CPlayerrecord *pStreamedPlayer = &m_PlayerRecords[1] ; pStreamedPlayer != endofarray; ++pStreamedPlayer)
	{
		CPlayerrecord *pCPlayer = pStreamedPlayer;
		if (pCPlayer && pCPlayer->m_bConnected && pCPlayer->m_pEntity && !pCPlayer->m_pEntity->GetDormant() && pCPlayer->m_pEntity != LocalPlayer.Entity && pCPlayer->hashedsteamid != bothash && pCPlayer->hashedsteamid != gotvhash
			&& (pCPlayer->m_pEntity->GetAlive() || pCPlayer->m_pEntity->IsPlayerGhost()))
		{
			CBaseEntity *Entity = pCPlayer->m_pEntity;
			FarESPPlayer *slice = &packofplayers[numplayersinpack];
			CTickrecord* lasttick = pCPlayer->GetPreviousRecord();
			localdata_t &localdata = Entity->GetLocalData()->localdata;
			QAngle eyeangles = Entity->GetEyeAngles();
			slice->islocalplayer = false;
			slice->iswalking = Entity->GetIsWalking();
			slice->teleporting = pCPlayer->m_bTeleporting;
			slice->helmet = Entity->HasHelmet();
			slice->ducking = pCPlayer->m_bIsHoldingDuck;
			slice->fakeducking = pCPlayer->m_bIsFakeDucking;
			slice->strafing = Entity->IsStrafing();
			slice->movestate = clamp(Entity->GetMoveState(), 0, 3);
			slice->hashedsteamid = pCPlayer->hashedsteamid;
			slice->flags = Entity->GetFlags();
			slice->tickschoked = clamp(pCPlayer->m_iTicksChoked, 0, 31);
			slice->armor = clamp(Entity->GetArmor(), 0, 100);
			slice->movetype = Entity->GetMoveType();
			slice->relativedirectionoflastinjury = Entity->GetRelativeDirectionOfLastInjury();
			slice->lasthitgroup = Entity->GetLastHitgroup();
			slice->weaponclassid = Entity->GetWeapon() ? ((CBaseEntity*)Entity->GetWeapon())->GetClientClass()->m_ClassID : -1;
			slice->weaponitemdefinitionindex = Entity->GetWeapon() ? Entity->GetWeapon()->GetItemDefinitionIndex() : 0;
			slice->health = Entity->GetHealth();
			slice->firecount = Entity->GetFireCount();
			slice->localdata.ducktimemsecs = localdata.m_nDuckTimeMsecs_;
			slice->localdata.duckjumptimemsecs = localdata.m_nDuckJumpTimeMsecs_;
			slice->localdata.jumptimemsecs = localdata.m_nJumpTimeMsecs_;
			slice->localdata.fallvelocity = localdata.m_flFallVelocity_;
			slice->localdata.lastducktime = localdata.m_flLastDuckTime_;
			slice->localdata.ducked = localdata.m_bDucked_;
			slice->localdata.ducking = localdata.m_bDucking_;
			slice->localdata.induckjump = localdata.m_bInDuckJump_;
			slice->absorigin = Entity->GetNetworkOrigin();
			slice->velocity = Entity->GetVelocity();
			slice->laddernormal = Entity->GetVecLadderNormal();
			slice->mins = Entity->GetMins();
			slice->maxs = Entity->GetMaxs();
			slice->animstate.totaltimeinair = pCPlayer->m_pAnimStateServer[ResolveSides::NONE]->m_flTotalTimeInAir;
			slice->animstate.flashedstarttime = pCPlayer->m_pAnimStateServer[ResolveSides::NONE]->m_flFlashedStartTime;
			slice->animstate.flashedendtime = pCPlayer->m_pAnimStateServer[ResolveSides::NONE]->m_flFlashedEndTime;
			slice->animstate.onground = pCPlayer->m_pAnimStateServer[ResolveSides::NONE]->m_bOnGround;
			slice->animstate.flashed = pCPlayer->m_pAnimStateServer[ResolveSides::NONE]->m_bFlashed;
			slice->duckamount = Entity->GetDuckAmount();
			slice->eyepitch = eyeangles.x;
			slice->eyeyaw = eyeangles.y;
			slice->simulationtime = Entity->GetSimulationTime();
			slice->absyaw = Entity->GetAbsAnglesDirect().y;
			slice->maxspeed = Entity->GetMaxSpeed();
			slice->stamina = Entity->GetStamina();
			slice->velocitymodifier = Entity->GetVelocityModifier();
			slice->timenotonladder = Entity->GetTimeNotOnLadder();
			slice->timeoflastinjury = Entity->GetTimeOfLastInjury();
			Entity->CopyPoseParameters(slice->poseparameters);
			for (int o = 0; o < min(MAX_CSGO_ANIM_LAYERS, Entity->GetNumAnimOverlays()); o++)
			{
				C_AnimationLayerFromPacket* recordlayer = &slice->animlayers[o];
				C_AnimationLayer *layer = Entity->GetAnimOverlay(o);
				recordlayer->_m_nSequence = layer->_m_nSequence;
				recordlayer->m_flWeight = layer->m_flWeight;
				recordlayer->_m_nOrder = layer->m_nOrder;
				recordlayer->_m_flPlaybackRate = layer->_m_flPlaybackRate;
				recordlayer->_m_flCycle = layer->_m_flCycle;
			}
			++numplayersinpack;
		}
	}

	//Now add the local player
	if (LocalPlayer.Entity && (LocalPlayer.Entity->GetAlive() || LocalPlayer.Entity->IsPlayerGhost()))
	{
		FarESPPlayer *slice = &packofplayers[numplayersinpack];
		CBaseEntity* Entity = LocalPlayer.Entity;
		player_info_t info;
		LocalPlayer.Entity->GetPlayerInfo(&info);
		auto& LastSentState = LocalPlayer.real_playerbackups[g_ClientState->last_command_ack % 150];
		localdata_t &localdata = LastSentState.LocalData;
		QAngle eyeangles = LastSentState.EyeAngles;

		slice->islocalplayer = true;
		slice->iswalking = Entity->GetIsWalking();
		slice->teleporting = LastSentState.Teleporting;
		slice->helmet = Entity->HasHelmet();
		slice->ducking = LastSentState.DuckAmount > 0.0f && LastSentState.DuckAmount >= LastSentState.LastDuckAmount;
		slice->fakeducking = LocalPlayer.IsFakeDucking;
		slice->strafing = LastSentState.IsStrafing;
		slice->movestate = clamp(LastSentState.MoveState, 0, 3);
		slice->movecollide = clamp((int)LastSentState.MoveCollide, 0, 7);
		slice->hashedsteamid = HashString_MUTINY(info.guid);
		slice->flags = LastSentState.Flags;
		slice->tickschoked = clamp(LastSentState.TicksChoked, 0, 31);
		slice->armor = clamp(Entity->GetArmor(), 0, 100);
		slice->movetype = LastSentState.MoveType;
		slice->relativedirectionoflastinjury = LastSentState.RelativeDirectionOfLastInjury;
		slice->lasthitgroup = LastSentState.LastHitgroup;
		slice->weaponclassid = LastSentState.WeaponClassID;
		slice->weaponitemdefinitionindex = LastSentState.WeaponItemDefinitionIndex;
		slice->health = Entity->GetHealth();
		slice->firecount = LastSentState.FireCount;
		slice->localdata.ducktimemsecs = localdata.m_nDuckTimeMsecs_;
		slice->localdata.duckjumptimemsecs = localdata.m_nDuckJumpTimeMsecs_;
		slice->localdata.jumptimemsecs = localdata.m_nJumpTimeMsecs_;
		slice->localdata.fallvelocity = localdata.m_flFallVelocity_;
		slice->localdata.lastducktime = localdata.m_flLastDuckTime_;
		slice->localdata.ducked = localdata.m_bDucked_;
		slice->localdata.ducking = localdata.m_bDucking_;
		slice->localdata.induckjump = localdata.m_bInDuckJump_;
		slice->absorigin = LastSentState.AbsOrigin;
		slice->velocity = LastSentState.AbsVelocity;
		slice->laddernormal = LastSentState.VecLadderNormal;
		slice->mins = LastSentState.Mins;
		slice->maxs = LastSentState.Maxs;
		slice->animstate.totaltimeinair = LastSentState.m_BackupServerAnimState[ResolveSides::NONE].m_flTotalTimeInAir;
		slice->animstate.flashedstarttime = LastSentState.m_BackupServerAnimState[ResolveSides::NONE].m_flFlashedStartTime;
		slice->animstate.flashedendtime = LastSentState.m_BackupServerAnimState[ResolveSides::NONE].m_flFlashedEndTime;
		slice->animstate.onground = LastSentState.m_BackupServerAnimState[ResolveSides::NONE].m_bOnGround;
		slice->animstate.flashed = LastSentState.m_BackupServerAnimState[ResolveSides::NONE].m_bFlashed;
		slice->duckamount = LastSentState.DuckAmount;
		slice->eyepitch = eyeangles.x;
		slice->eyeyaw = eyeangles.y;
		slice->simulationtime = TICKS_TO_TIME(LastSentState.TickBase);
		slice->absyaw = LastSentState.AbsAngles.y;
		slice->maxspeed = LastSentState.MaxSpeed;
		slice->stamina = LastSentState.Stamina;
		slice->velocitymodifier = LastSentState.VelocityModifier;
		slice->timenotonladder = LastSentState.TimeNotOnLadder;
		slice->timeoflastinjury = LastSentState.TimeOfLastInjury;
		for (int i = 0; i < MAX_CSGO_POSE_PARAMS; ++i)
			slice->poseparameters[i] = LastSentState.m_flPoseParameters[i];
		for (int i = 0; i < min(MAX_CSGO_ANIM_LAYERS, LocalPlayer.Entity->GetNumAnimOverlays()); ++i)
		{
			C_AnimationLayerFromPacket* recordlayer = &slice->animlayers[i];
			C_AnimationLayer *layer = LocalPlayer.Entity->GetAnimOverlay(i);
			recordlayer->_m_nSequence = layer->_m_nSequence;
			recordlayer->m_flWeight = layer->m_flWeight;
			recordlayer->_m_nOrder = layer->m_nOrder;
			recordlayer->_m_flPlaybackRate = layer->_m_flPlaybackRate;
			recordlayer->_m_flCycle = layer->_m_flCycle;
		}
		++numplayersinpack;
	}
	packofplayersmutex.unlock();
}

DWORD __stdcall AsyncProcessFarESPPacket(AsyncPacket* packet)
{
#ifdef FAR_ESP_LOGGING
	//decrypts(0)
	std::ofstream file(XorStr("faresp.txt"), std::ofstream::out | std::ofstream::app);
	file << XorStr("AsyncProcessFarESPPacket\n");
	//encrypts(0)
	file.close();
#endif
	char *buffer = packet->buffer;
	PacketHeader *gpkt = (PacketHeader*)buffer;
	KeepAlivePacket *keepalive = (KeepAlivePacket*)buffer;
	if ((packet->targetpacketsize != sizeof(KeepAlivePacket) || keepalive->verify != KEEPALIVE_VERIFY) && gpkt->typeofpacket == SERVERTOCLIENT)
	{
		CompressedESPPacket *cpkt = (CompressedESPPacket*)buffer;
		uint32_t compressedsize = cpkt->compressedsize;
		if (compressedsize && cpkt->compressedsize == (((DWORD)buffer + packet->targetpacketsize) - (DWORD)&cpkt->data))
		{
			tcp_decompressor decompressed;
			decompressed.decompress(&cpkt->data, cpkt->compressedsize);

			FarESPPlayer* players = (FarESPPlayer*)decompressed.get_data().c_str();
			int numplayers = decompressed.get_data().size() / sizeof(FarESPPlayer);
			g_FarESP.farespmutex.lock();
			auto& farespplayers = g_FarESP.farespplayers;
			float flNewestSimulationTime = 0.0f;
			const auto endofplayersarray = &players[numplayers];
			for (FarESPPlayer* player = &players[0]; player != endofplayersarray; ++player)
			{
				if (player->simulationtime > flNewestSimulationTime)
					flNewestSimulationTime = player->simulationtime;

				if (!farespplayers.empty())
				{
					auto storedplayer = farespplayers.find(player->hashedsteamid);
					if (storedplayer == farespplayers.end())
					{
						farespplayers.emplace(player->hashedsteamid, *player);
					}
					else
					{
						storedplayer->second = *player;
					}
				}
				else
				{
					farespplayers.emplace(player->hashedsteamid, *player);
				}
			}
			for (auto storedplayer = farespplayers.begin(); storedplayer != farespplayers.end();)
			{
				//erase player info that is too old
				if (flNewestSimulationTime - storedplayer->second.simulationtime > 5.0f)
					storedplayer = farespplayers.erase(storedplayer);
				else
					++storedplayer;
			}
			g_FarESP.farespmutex.unlock();
		}
	}
	delete[] packet->buffer;
	delete packet;
	return EXIT_SUCCESS;
}

bool FarESP::PacketSizeIsKnown(uint32_t& numbytesinbuffer, char* buffer, uint32_t &targetpacketsize)
{
	//Beginning of new packet
	if (numbytesinbuffer < sizeof(targetpacketsize))
	{
		//Didn't receive enough bytes to get packet size yet, so continue receiving
		return false;
	}
	targetpacketsize = *(uint32_t*)buffer;

	return true;
}

bool FarESP::InspectReceivedPacket(uint32_t& numbytesinbuffer, char* buffer, uint32_t &targetpacketsize)
{
#ifdef FAR_ESP_LOGGING
	//decrypts(0)
	std::ofstream file(XorStr("faresp.txt"), std::ofstream::out | std::ofstream::app);
	file << XorStr("InspectReceivedPacket\n");
	//encrypts(0)
	file.close();
#endif
	if (numbytesinbuffer >= targetpacketsize)
	{
		//Asynchronously process full packets
		char *newbuffer = new char[targetpacketsize];
		if (newbuffer)
		{
			memmove(newbuffer, buffer, targetpacketsize);
			AsyncPacket *async = new AsyncPacket(newbuffer, targetpacketsize);
			if (async)
			{
#if 1
				AsyncProcessFarESPPacket(async);
#else
#if 0
				std::thread thr(AsyncProcessFarESPPacket, async);
				thr.detach();
#else
				unsigned int threadid;
				//HANDLE thr = CreateThreadSafe((LPTHREAD_START_ROUTINE)AsyncProcessFarESPPacket, (LPVOID)async);
				HANDLE thr = (HANDLE)_beginthreadex(nullptr, 0, (_beginthreadex_proc_type)&AsyncProcessFarESPPacket, async, 0, &threadid);
				if (thr != 0)
					CloseHandle(thr);
				else
				{
					delete[] newbuffer;
					delete async;
				}
#endif
#endif
			}
			else
			{
				delete[] newbuffer;
			}
		}

		if (numbytesinbuffer == targetpacketsize)
		{
			//No more packets waiting to be processed
			targetpacketsize = 0;
			numbytesinbuffer = 0;
		}
		else
		{
			//More packets are waiting in the buffer
			//Move the rest of the data in the buffer to the beginning of the buffer and then treat it as a freshly received packet
			//Recursively process all the data in the packets until we no longer have a complete packet
			memmove(buffer, (void*)(buffer + targetpacketsize), numbytesinbuffer - targetpacketsize);
			numbytesinbuffer -= targetpacketsize;
			targetpacketsize = 0;
			if (PacketSizeIsKnown(numbytesinbuffer, buffer, targetpacketsize))
			{
				if (targetpacketsize == 0)
				{
#ifdef _DEBUG
					printf("WARNING FARESP (InspectReceivedPacket): targetpacketsize is empty!");
#endif
				}
				else if (!InspectReceivedPacket(numbytesinbuffer, buffer, targetpacketsize))
				{
					return false;
				}
			}
		}
	}
	
	return true;
}

bool FarESP::SendPacket(char *buffer, uint32_t totalsizeofpacket)
{
#ifdef FAR_ESP_LOGGING
	//decrypts(0)
	std::ofstream file(XorStr("faresp.txt"), std::ofstream::out | std::ofstream::app);
	file << XorStr("SendPacket\n");
	//encrypts(0)
	file.close();
#endif
	uint32_t bytessentsofar = 0;
	for (int i = 0; i < 10; i++)
	{
		if (CheckIfReceiveThreadIsDone())
			break;

		int iResult = Send(Socket, (const char*)(buffer + bytessentsofar), totalsizeofpacket - bytessentsofar, 0);
		if (iResult < 0)
		{
#ifdef _DEBUG
			printf("Client send failed with error: %d\n", WSAGetLastError());
#endif
			continue;
		}
		else
		{
			if (iResult == 0)
				continue;

			bytessentsofar += iResult;

			LastSendAckTime = GetTickCount64();

			if (bytessentsofar >= totalsizeofpacket)
				return true;

			i = 0;
		}
	}
	return false;
}

DWORD __stdcall DoSending(void* arg)
{
#ifdef FAR_ESP_LOGGING
	//decrypts(0)
	std::ofstream file(XorStr("faresp.txt"), std::ofstream::out | std::ofstream::app);
	file << XorStr("DoSending\n");
	//encrypts(0)
	file.close();
#endif
	// Receive until the peer closes the connection
	int iResult = 0;
	KeepAlivePacket keepalivepacket;
	for (;;)
	{
		if (g_FarESP.CheckIfReceiveThreadIsDone() || variable::get().visuals.pf_enemy.i_faresp <= 0)
			break;

		ULONGLONG totaltimetakentosendpacket = 0;
		ULONGLONG time = GetTickCount64();
		ULONGLONG timesincelastack = time - g_FarESP.LastSendAckTime;
		if (timesincelastack >= 10000)
			break;

		uint32_t totalsize;
		char *buffer = GenerateOutgoingPlayerPacket(totalsize);
		bool bufferiskeepalive = false;

		if (!buffer)
		{
			if (timesincelastack >= 300)
			{
				buffer = (char*)new KeepAlivePacket;
				totalsize = (uint32_t)sizeof(KeepAlivePacket);
				bufferiskeepalive = true;
			}
		}

		if (buffer)
		{
			bool success = g_FarESP.SendPacket(buffer, totalsize);
			ULONGLONG newtime = GetTickCount64();
			totaltimetakentosendpacket = newtime - time;
			free(buffer);
			if (!success)
				break;
			g_FarESP.LastSendAckTime = newtime;
		}
		if (variable::get().visuals.pf_enemy.i_faresp <= 0)
			break;
		//else if (/*FarESPCmdRateTxt.iValue*/48 > 64)
		//	48;//FarESPCmdRateTxt.iValue = 64;

		DWORD sleepmsecs = (!buffer || bufferiskeepalive) ? 1 : clamp((1000 / variable::get().visuals.pf_enemy.i_faresp) - (int)totaltimetakentosendpacket, 0, 1000);
		Sleep(sleepmsecs);
	}
	g_FarESP.SetSendThreadIsDone(true);
#ifdef _DEBUG
	printf("Client side far esp thread is closing..\n");
#endif
	return EXIT_SUCCESS;
}

DWORD __stdcall DoFarESPReceiving(void* arg)
{
#ifdef FAR_ESP_LOGGING
	//decrypts(0)
	std::ofstream file(XorStr("faresp.txt"), std::ofstream::out | std::ofstream::app);
	file << XorStr("DoFarESPReceiving\n");
	//encrypts(0)
	file.close();
#endif
	g_FarESP.farespmutex.lock();
	g_FarESP.farespplayers.clear();
	g_FarESP.farespmutex.unlock();
	g_FarESP.ClearClientToServerData();

	if (!g_FarESP.CreateSocket())
	{
		g_FarESP.SetShouldExit(false);
		return EXIT_SUCCESS;
	}

	g_FarESP.SetSendThreadIsDone(false);
	g_FarESP.SetReceiveThreadIsDone(false);
	
#if 0
	std::thread thr(DoSending, nullptr);
	thr.detach();
#else
	unsigned int threadid;
	HANDLE thr = (HANDLE)_beginthreadex(nullptr, 0, (_beginthreadex_proc_type)&DoSending, nullptr, 0, &threadid);
	//HANDLE thr = CreateThreadSafe(DoSending, nullptr);
	if (thr == 0)
	{
		g_FarESP.SetReceiveThreadIsDone(true);
		g_FarESP.ShutdownSocket();
	}
	CloseHandle(thr);
#endif

	// Receive until the peer closes the connection
	int iResult = 0;
	//char recvbuf[DEFAULT_BUFLEN];
	//int recvbuflen = DEFAULT_BUFLEN;
	const auto bufsize = sizeof(char) * (1000000 * 5);
	const auto recvpacketbuffersize = sizeof(char) * (1000000 * 1);
	char *buffer = new char[bufsize];
	if (buffer)
	{
		auto& LastReceiveAckTime = g_FarESP.LastReceiveAckTime;
		char *packet = new char[recvpacketbuffersize];
		if (packet)
		{
			uint32_t targetpacketsize = 0;
			int totalbytesreceived = 0;
			uint32_t numbytesinbuffer = 0;
			LastReceiveAckTime = g_FarESP.LastSendAckTime = GetTickCount64();

			for (;;)
			{
				ULONGLONG time = GetTickCount64();

				if (variable::get().visuals.pf_enemy.i_faresp <= 0 || time - LastReceiveAckTime >= 10000 || g_FarESP.CheckIfSendThreadIsDone() || g_FarESP.ShouldExit())
				{
					break;
				}

				Sleep(1);

				iResult = Recv(g_FarESP.Socket, packet, recvpacketbuffersize, 0);

				if (iResult < 0)
				{
	#ifdef _DEBUG
					//printf("Client recv failed with error: %d\n", WSAGetLastError());
	#endif
					continue;
				}
				else
				{
					LastReceiveAckTime = GetTickCount64();

					if (iResult > 0)
					{
						if (numbytesinbuffer + (uint32_t)iResult > bufsize)
						{
	#ifdef _DEBUG
							printf("ERROR: BUFFER OVERFLOW IN FARESP CLIENT\n");
							DebugBreak();
	#endif
							break;
						}

						memcpy(&buffer[numbytesinbuffer], packet, (size_t)iResult);
						numbytesinbuffer += (uint32_t)iResult;

						if (targetpacketsize == 0)
						{
							if (!g_FarESP.PacketSizeIsKnown(numbytesinbuffer, buffer, targetpacketsize))
								continue;
	#ifdef _DEBUG
							if (targetpacketsize == 0)
							{
								printf("WARNING FAR ESP: empty packet size!\n");
								DebugBreak();
							}
	#endif
						}

						if (!g_FarESP.InspectReceivedPacket(numbytesinbuffer, buffer, targetpacketsize))
						{
	#ifdef _DEBUG
							printf("ERROR FAR ESP: disconnecting due to recursively MALFORMED packets");
							DebugBreak();
	#endif
							break;
						}
					}
				}
			}
			delete[] packet;
		}
		delete[] buffer;
	}

	g_FarESP.SetReceiveThreadIsDone(true);

	while (!g_FarESP.CheckIfSendThreadIsDone())
	{
		Sleep(10);
	}

	g_FarESP.ShutdownSocket();

	return EXIT_SUCCESS;
}

#endif