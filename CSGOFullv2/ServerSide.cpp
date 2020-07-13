#ifdef FRAME_DISABLE
#pragma comment(lib, "winmm.lib") //Pragma for the time unresolved external symbols
#endif

#include "precompiled.h"

#ifdef USE_SERVER_SIDE
#include "ServerSide.h"
#include "VTHook.h"
#include "Netchan.h"
//Other libs
//#include "Ws2tcpip.h"
//#pragma comment(lib,"ws2_32.lib") //Winsock Library
#include <process.h>
#include "LocalPlayer.h"

SOCKET tcps;
WSADATA w;
ServerSide pServerSide;
#define DEFAULT_BUFLEN 512

std::deque<CSGOPacket>serverpackets;
std::mutex packetmutex;

bool HandleNewPacket(size_t& numbytesinbuffer, char* buffer, uint32_t &targetpacketsize)
{
	//Beginning of new packet
	if (numbytesinbuffer < sizeof(targetpacketsize))
	{
		//Didn't receive enough bytes to get packet size yet, so continue receiving
		false;
	}
	targetpacketsize = *(uint32_t*)buffer;

	if (targetpacketsize == sizeof(KeepAlivePacket))
	{
#ifdef _DEBUG
		//printf("Client received beginning of new keepalive packet\n");
#endif
	}
	else if (targetpacketsize == sizeof(CSGOPacket))
	{
#ifdef _DEBUG
		//printf("Client received beginning of new CSGOPacket\n");
#endif
	}
	else
	{
#ifdef _DEBUG
		printf("Client received invalid packetsize (%u)\n", targetpacketsize);
#endif
		numbytesinbuffer = 0;
		false;
	}
	return true;
}

void ProcessPacket(size_t& numbytesinbuffer, char* buffer, uint32_t &targetpacketsize)
{
	if ((uint32_t)numbytesinbuffer >= targetpacketsize)
	{
		if (targetpacketsize == sizeof(KeepAlivePacket))
		{
#ifdef _DEBUG
			//printf("Client finished receiving keepalive packet\n");
#endif
			KeepAlivePacket* keepalive = (KeepAlivePacket*)buffer;
			if (keepalive->verify != KEEPALIVE_VERIFY)
			{
#ifdef _DEBUG
				printf("Client failed to verify keepalive packet!\n");
#endif
			}
		}
		else
		{
#ifdef _DEBUG
			//printf("Client finished receiving CSGO Packet\n");
#endif
			CSGOPacket* csgopacket = (CSGOPacket*)buffer;
			if (csgopacket->verify != CSGOPACKET_VERIFY)
			{
#ifdef _DEBUG
				printf("Client failed to verify CSGO Packet!\n");
#endif
			}
			else
			{
				packetmutex.lock();
				if (serverpackets.size() > 300)
					serverpackets.pop_front();
				serverpackets.push_back(*csgopacket);
				packetmutex.unlock();
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
			if (HandleNewPacket(numbytesinbuffer, buffer, targetpacketsize))
			{
				ProcessPacket(numbytesinbuffer, buffer, targetpacketsize);
			}
		}
	}
}

DWORD __stdcall DoReceiving(void* arg)
{
	if (!pServerSide.CreateSocket())
	{
		Sleep(2000);
		pServerSide.SetShouldExit(false);
		return 0;
	}

	pServerSide.SetSendThreadIsDone(false);
	pServerSide.SetReceiveThreadIsDone(false);

	// Receive until the peer closes the connection
	int iResult = 0;
	//char recvbuf[DEFAULT_BUFLEN];
	//int recvbuflen = DEFAULT_BUFLEN;
	auto bufsize = sizeof(char) * (1000000 * 5);
	char *packet = new char[bufsize];
	uint32_t targetpacketsize = 0;
	ULONGLONG LastPacketReceiveTime = GetTickCount64();

	size_t numbytesinbuffer = 0;
	char *buffer = new char[bufsize];

	for (;;)
	{
		ULONGLONG time = GetTickCount64();
		if (time - LastPacketReceiveTime >= 10000 || pServerSide.ShouldExit())
			break; 
		
		int iResult = recv(pServerSide.Socket, packet, bufsize, 0);

		if (iResult < 0)
		{
#ifdef _DEBUG
			printf("Client recv failed with error: %d\n", WSAGetLastError());
#endif
			continue;
		}
		else
		{
			LastPacketReceiveTime = GetTickCount64();

			if (iResult > 0)
			{
				if (numbytesinbuffer + iResult > bufsize)
				{
#ifdef _DEBUG
					printf("ERROR: BUFFER OVERFLOW IN SERVERSIDE CLIENT\n");
					//DebugBreak();
#endif
					break;
				}

				memcpy(&buffer[numbytesinbuffer], packet, iResult);
				numbytesinbuffer += iResult;

				if (targetpacketsize == 0)
				{
					if (!HandleNewPacket(numbytesinbuffer, buffer, targetpacketsize))
						continue;
				}

				ProcessPacket(numbytesinbuffer, buffer, targetpacketsize);
			}
		}
	}


	pServerSide.SetReceiveThreadIsDone(true);

	//while (!pServerSide.CheckIfSendThreadIsDone())
	//{
	//	Sleep(10);
	//}

#ifdef _DEBUG
	printf("Client Closing Server Side socket..\n");
#endif
	iResult = shutdown(pServerSide.Socket, 2); //SD_BOTH = 2, SD_SEND = 1, SD_RECEIVE = 0
	closesocket(pServerSide.Socket);
	WSACleanup();
	Sleep(1000);
	pServerSide.Socket = INVALID_SOCKET;
	pServerSide.SetSocketCreated(false);
	pServerSide.SetShouldExit(false);

	return 0;
}

void ServerSide::OnCreateMove()
{
	if (pServerSide.IsSocketCreated())
	{
		if (g_ClientState->m_pNetChannel)
		{
#if 1 //set to 0 if on lan
			if (!pServerSide.IsCreatingSocket() && !pServerSide.ShouldExit() && !((INetChannel*)g_ClientState->m_pNetChannel)->remote_address.CompareAdr(pServerSide.gamenetadr))
			{
				pServerSide.SetShouldExit(true);
			}
#endif
		}
		else if (!pServerSide.ShouldExit())
		{
			pServerSide.SetShouldExit(true);
		}
	}
	else if (g_ClientState->m_pNetChannel)
	{
		if (!pServerSide.IsCreatingSocket())
		{
			pServerSide.CreateThread();
		}
	}

	if (packetmutex.try_lock())
	{
		if (!serverpackets.empty())
		{
			for (auto packetiterator = serverpackets.begin(); packetiterator != serverpackets.end(); packetiterator++)
			{
				CSGOPacket *packet = &*packetiterator;
				CPlayerrecord *pTargetPlayer = nullptr;
				for (int i = 1; i <= MAX_PLAYERS; i++)
				{
					CPlayerrecord *pCPlayer = g_LagCompensation.GetPlayerrecord(i);
					if (pCPlayer->hashedsteamid == packet->hashedsteamid)
					{
						if (pCPlayer->m_bConnected)
							pTargetPlayer = pCPlayer;
						break;
					}
				}
#if (defined SERVER_SIDE_ONLY) || (defined ALLOW_SERVER_SIDE_RESOLVER)
				for (int i = 1; i <= MAX_PLAYERS; i++)
				{
					CPlayerrecord *pCPlayer = g_LagCompensation.GetPlayerrecord(i);
					if (pCPlayer->m_bConnected && pCPlayer->hashedsteamid == packet->hashedsteamid)
					{
						pTargetPlayer = pCPlayer;
						break;
					}
				}
#endif
				if (packet->typeofpacket == BACKTRACK)
				{
					if (pTargetPlayer && pTargetPlayer->m_pEntity != LocalPlayer.Entity)
					{
#if 1
						if (pTargetPlayer->m_pEntity)
						{
							PlayerBackup_t *backup = new PlayerBackup_t(pTargetPlayer->m_pEntity);
							if (backup)
							{
								CBaseEntity* pEnt = pTargetPlayer->m_pEntity;
								pEnt->WriteAnimLayersFromPacket(packet->AnimLayer);
								pEnt->WritePoseParameters(packet->flPoseParameters);
								pEnt->SetNetworkOrigin(packet->absorigin);
								pEnt->SetAbsOrigin(packet->absorigin);
								pEnt->SetAbsAngles(packet->absangles);

								AllowSetupBonesToUpdateAttachments = false;
								pEnt->InvalidateBoneCache();
								pEnt->SetLastOcclusionCheckFlags(0);
								pEnt->SetLastOcclusionCheckFrameCount(Interfaces::Globals->framecount);
								pEnt->ReevaluateAnimLOD();
								pEnt->SetupBonesRebuilt(nullptr, -1, BONE_USED_BY_HITBOX, Interfaces::Globals->curtime);
								pEnt->DrawHitboxesFromCache(ColorRGBA(55, 196, 255, 255), 5.4f, (matrix3x4_t*)pEnt->GetCachedBoneData()->Base());

								backup->RestoreData();
								delete backup;
							}
						}
#else
						for (hitboxinfo* info = &packet->hitboxes[0]; info != &packet->hitboxes[20]; info++)
						{
							if (info->flRadius <= 0.0f)
								Interfaces::DebugOverlay->AddBoxOverlay(info->position, info->vMin, info->vMax, info->angles, 55, 196, 255, 0, 4.4f);
							else
								Interfaces::DebugOverlay->DrawPill(info->vMin, info->vMax, info->flRadius, 55, 196, 255, 255, 4.4f, 0, 1);
						}
#endif
					}
				}
				else if (packet->typeofpacket == FIREBULLET)
				{
					if (pTargetPlayer && pTargetPlayer->m_pEntity == LocalPlayer.Entity)
					{
						for (hitboxinfo* info = &packet->hitboxes[0]; info != &packet->hitboxes[20]; info++)
						{
							//if (info->flRadius <= 0.0f)
							//	Interfaces::DebugOverlay->AddBoxOverlay(info->position, info->vMin, info->vMax, info->angles, 0, 255, 0, 0, 3.0f);
							//else
							//	Interfaces::DebugOverlay->DrawPill(info->vMin, info->vMax, info->flRadius, 0, 255, 0, 255, 3.0f, 0, 1);
						}
						//Draw server shot position box
						Interfaces::DebugOverlay->AddBoxOverlay(packet->shoteyepos, Vector(-0.5, -0.5, -0.5), Vector(0.5, 0.5, 0.5), packet->shoteyeangles, 0, 255, 0, 255, 4.0f);
						//Draw the intended shot line on the server (ignoring spread)
						Vector vecDir;
						AngleVectors(packet->shoteyeangles, &vecDir);
						VectorNormalizeFast(vecDir);
						Interfaces::DebugOverlay->AddLineOverlay(packet->shoteyepos, packet->shoteyepos + vecDir * 8192.0f, 0, 255, 0, true, 10.f);
					}
				}
				else if (pTargetPlayer)
				{
					//uncomment to not show local player hitboxes
					//if (pTargetPlayer->m_pEntity != LocalPlayer.Entity)
					{
						pTargetPlayer->serversidemutex.lock();
						pTargetPlayer->LastServerSidePacket = *packet;
						pTargetPlayer->ServerSidePackets.push_front(*packet);
						if (pTargetPlayer->ServerSidePackets.size() > 32)
							pTargetPlayer->ServerSidePackets.pop_back();

//#ifdef _DEBUG
						if (pTargetPlayer->m_pEntity != LocalPlayer.Entity && pTargetPlayer->m_bConnected && pTargetPlayer->m_iLifeState == LIFE_ALIVE && pTargetPlayer->GetCurrentRecord())
						{
							//draw newest synced up deltas from the server vs client
							bool found = false;
							for (auto& record : pTargetPlayer->ServerSidePackets)
							{
								if (record.flSimulationTime > pTargetPlayer->m_flLastServerSidePrintSimulationTime)
								{
									CTickrecord *tickrecord = nullptr;
									CTickrecord *prevrecord = nullptr;
									bool found = false;
									for (auto& tick : pTargetPlayer->m_Tickrecords)
									{
										if (found)
										{
											prevrecord = tick;
											break;
										}
										if (tick->m_SimulationTime == record.flSimulationTime)
										{
											found = true;
											tickrecord = tick;
										}
									}
									AllocateConsole();
									//printf("yaw %.1f client absyaw %.1f delta %.1f eyeyaw %.1f lby %.1f maxdt %.3f mult %.3f estmult %f estabsyaw %.1f\n", packet->absangles.y, pTargetPlayer->fakeabsangles.y, AngleDiff(packet->absangles.y, pTargetPlayer->fakeabsangles.y), AngleNormalize(pTargetPlayer->GetCurrentRecord()->m_EyeAngles.y), pTargetPlayer->m_pEntity->GetLowerBodyYaw(), pTargetPlayer->m_pEntity->GetMaxDesyncDelta(tickrecord), pTargetPlayer->m_pEntity->GetMaxDesyncMultiplier(tickrecord), tickrecord->m_flEstimatedDesyncMultiplier, tickrecord->m_AbsAngles.y);
									if (prevrecord && tickrecord && tickrecord->m_flLowerBodyYaw != prevrecord->m_flLowerBodyYaw)
									{
										pTargetPlayer->m_flLastServerLBYChangeTime = tickrecord->m_SimulationTime;
										LocalPlayer.DrawTextInFrontOfPlayer(0.5f, 0.0f, "LBY Update");
									}
									if (tickrecord && fabsf(AngleDiff(packet->absangles.y, pTargetPlayer->m_flLastServerAbsYaw) > 15.0f))
									{
										char tmp[512];
										sprintf(tmp, "time since abs change %f since lbyupdate %f", tickrecord->m_SimulationTime - pTargetPlayer->m_flLastServerAbsYawChangeTime, tickrecord->m_SimulationTime - pTargetPlayer->m_flLastServerLBYChangeTime);
										//LocalPlayer.DrawTextInFrontOfPlayer(tmp, 0.5f);
										pTargetPlayer->m_flLastServerAbsYawChangeTime = tickrecord->m_SimulationTime;
									}
									pTargetPlayer->m_flLastServerAbsYaw = packet->absangles.y;


									pTargetPlayer->m_flLastServerSidePrintSimulationTime = record.flSimulationTime;
									break;
								}
							}
						}
//#endif

						pTargetPlayer->serversidemutex.unlock();
					}
				}
				else
				{
					//Player doesn't exist in our entity table, so just draw their hitbox. We can't aimbot someone that doesn't exist yet
					for (hitboxinfo* info = &packet->hitboxes[0]; info != &packet->hitboxes[20]; info++)
					{
						if (info->flRadius <= 0.0f)
							Interfaces::DebugOverlay->AddBoxOverlay(info->position, info->vMin, info->vMax, info->angles, 200, 191, 231, 0, Interfaces::Globals->interval_per_tick * 2.0f);
						else
							Interfaces::DebugOverlay->DrawPill(info->vMin, info->vMax, info->flRadius, 200, 191, 231, 255, Interfaces::Globals->interval_per_tick * 2.0f, 0, 1);
					}
				}
			}
			serverpackets.clear();
		}
		packetmutex.unlock();
	}

	//Now draw the last known server hitbox for each player
	for (int i = 1; i <= MAX_PLAYERS; i++)
	{
		CPlayerrecord *pCPlayer = g_LagCompensation.GetPlayerrecord(i);
		if (pCPlayer->m_bConnected && pCPlayer->m_pEntity)
		{
			bool render = true;
			int R, G, B;
			if (pCPlayer->m_pEntity->GetTeam() == LocalPlayer.Entity->GetTeam())
			{
				//if (ESPEnemyOnlyChk.Checked)
				//	render = false;
				//else
				R = 0.486f * 255.0f; G = 0.811f * 255.0f; B = 0.894f * 255.0f;
			}
			else
			{
				R = 0.988f * 255.0f; G = 0.623f * 255.0f; B = 0.262f * 255.0f;
			}
			if (render)
			{
				pCPlayer->serversidemutex.lock();
				CSGOPacket *lastknownpacket = &pCPlayer->LastServerSidePacket;
				if (!pCPlayer->m_pEntity->GetDormant() && pCPlayer->ServerSidePackets.size() > 1)
				{
					//if (lastknownpacket->flSimulationTime > pCPlayer->m_pEntity->GetSimulationTime())
					//	lastknownpacket = &pCPlayer->ServerSidePackets.at(1);
				}
				if (lastknownpacket->bAlive && fabsf(Interfaces::Globals->curtime - lastknownpacket->flSimulationTime) < 35.0f)
				{
					if (!pCPlayer->m_bDormant)
					{
						for (const auto& tick : pCPlayer->m_Tickrecords)
						{
							if (tick->m_SimulationTime == lastknownpacket->flSimulationTime)
							{
								Interfaces::DebugOverlay->AddTextOverlay(lastknownpacket->absorigin + Vector(0, 0, 150), TICKS_TO_TIME(2), "Server AbsYaw %f Client AbsYaw %f", AngleNormalizePositive(lastknownpacket->absangles.y), AngleNormalizePositive(tick->m_AbsAngles.y));
								break;
							}
						}
					}

					for (hitboxinfo* info = &lastknownpacket->hitboxes[0]; info != &lastknownpacket->hitboxes[20]; info++)
					{
						if (info->flRadius <= 0.0f)
							Interfaces::DebugOverlay->AddBoxOverlay(info->position, info->vMin, info->vMax, info->angles, R, G, B, 0, Interfaces::Globals->interval_per_tick * 2.0f);
						else
							Interfaces::DebugOverlay->DrawPill(info->vMin, info->vMax, info->flRadius, R, G, B, 255, Interfaces::Globals->interval_per_tick * 2.0f, 0, 1);
					}
				}
				pCPlayer->serversidemutex.unlock();
			}
		}
	}
}

void ServerSide::CreateThread()
{
#ifdef _DEBUG
	//printf("ServerSide::CreateThread\n");
#endif
	pServerSide.SetIsCreatingSocket(true);
	unsigned int threadid;
	HANDLE thr = (HANDLE)_beginthreadex(nullptr, 0, (_beginthreadex_proc_type)&DoReceiving, nullptr, 0, &threadid);
	CloseHandle(thr);
}

bool ServerSide::CreateSocket()
{
	INetChannel *netchan = (INetChannel*)g_ClientState->m_pNetChannel;
#if 0
	unsigned short port = 28016;
#else
	unsigned short port = netchan->remote_address.GetPort() + 1000;
#endif

	Socket = INVALID_SOCKET;
	ZeroMemory(&socketaddress, sizeof(sockaddr));

	int iResult;

#ifdef _DEBUG
	//printf("Client attempting connection to server side plugin..\n");
#endif

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &w);
	if (iResult != 0) 
	{
		Socket = INVALID_SOCKET;
#ifdef _DEBUG
		printf("WSAStartup failed with error: %d\n", iResult);
#endif
		SetIsCreatingSocket(false);
		SetSocketCreated(false);
		return 0;
	}

#if 0
	memset(&socketaddress, 0, sizeof(struct sockaddr));
	unsigned char ip[4];
	ip[0] = 192;
	ip[1] = 168;
	ip[2] = 1;
	ip[3] = 2;
	((struct sockaddr_in*)&socketaddress)->sin_family = AF_INET;
	((struct sockaddr_in*)&socketaddress)->sin_addr.s_addr = *(int *)&ip[0];
#else
	netchan->remote_address.ToSockadr(&socketaddress);
#endif
	gamenetadr = netchan->remote_address;
	((struct sockaddr_in*)&socketaddress)->sin_port = BigShort(port);

	// Create a SOCKET for connecting to server
	Socket = socket(socketaddress.sa_family, SOCK_STREAM, IPPROTO_TCP);

	if (Socket == INVALID_SOCKET) 
	{
#ifdef _DEBUG
		printf("socket failed with error: %ld\n", WSAGetLastError());
#endif
		WSACleanup();
		SetIsCreatingSocket(false);
		SetSocketCreated(false);
		return 0;
	}

	DWORD timeoutmilliseconds = 2000;

	if (setsockopt(Socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeoutmilliseconds, sizeof(DWORD)) < 0)
	{
#ifdef _DEBUG
		printf("setsockopt failed\n");
#endif
		Socket = INVALID_SOCKET;
		SetIsCreatingSocket(false);
		SetSocketCreated(false);
		return 0;
	}

	if (setsockopt(Socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeoutmilliseconds, sizeof(DWORD)) < 0)
	{
#ifdef _DEBUG
		printf("setsockopt failed\n");
#endif
		Socket = INVALID_SOCKET;
		return 0;
	}

	// Connect to server.
	iResult = connect(Socket, &socketaddress, sizeof(struct sockaddr));
	if (iResult == SOCKET_ERROR) 
	{
#ifdef _DEBUG
		//printf("Unable to connect to server side plugin!\n");
#endif
		closesocket(Socket);
		Socket = INVALID_SOCKET;
		WSACleanup();
		SetIsCreatingSocket(false);
		SetSocketCreated(false);
		return 0;
	}

	if (Socket == INVALID_SOCKET)
	{
#ifdef _DEBUG
		printf("Unable to connect to server side plugin!\n");
#endif
		WSACleanup();
		SetIsCreatingSocket(false);
		SetSocketCreated(false);
		return 0;
	}

	// Send an initial buffer
	const char *buffer = "\version\1";
	iResult = send(Socket, buffer, (int)strlen(buffer) + 1, 0);
	if (iResult <= 0) 
	{
#ifdef _DEBUG
		printf("send failed with error: %d\n", WSAGetLastError());
#endif
		closesocket(Socket);
		WSACleanup();
		Socket = INVALID_SOCKET;
		SetIsCreatingSocket(false);
		SetSocketCreated(false);
		return 0;
	}

#ifdef _DEBUG
	printf("Client connected to server side plugin, bytes sent: %ld\n", iResult);
#endif

	// shutdown the connection since no more data will be sent
#if 0
	iResult = shutdown(ConnectSocket, 2); //SD_BOTH = 2, SD_SEND = 1, SD_RECEIVE = 0
	if (iResult == SOCKET_ERROR)
	{
		//printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		Socket = INVALID_SOCKET;
		return 1;
	}
#endif
	SetSocketCreated(true);
	SetIsCreatingSocket(false);
	return 1;
/*
	// cleanup
	closesocket(Socket);
	WSACleanup();
	Socket = INVALID_SOCKET;

	return 0;
*/
}
#endif