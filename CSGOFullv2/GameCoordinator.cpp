#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING

#include <Windows.h>
#include <tchar.h>

#include "VTHook.h"

#include <cstdint>
#include <cstdio>

#include "SteamAPI.h"
#include "isteamgamecoordinator.h"

#if _MSC_VER > 1900
#include "protobufs/cstrike15_gcmessages.pb.h"

#pragma comment(lib, "ws2_32.lib")

#ifndef _DEBUG
#pragma comment(lib, "libprotobuf.lib")
#else
#pragma comment(lib, "libprotobufd.lib")
#endif

#endif

SendMessageGameCoordinatorFn oSendMessageSteamGameCoordinator;
RetrieveMessageGameCoordinatorFn oRetrieveMessageSteamGameCoordinator;

#include <intrin.h>
#include <string>
#include "ConCommand.h"
#include "LocalPlayer.h"


std::vector<char*> GetDatacenterNearDatacenter(char* name)
{
	std::vector<char*> list;

	if (strstr(name, "gru"))
		list.push_back("scl");
	else if (strstr(name, "scl"))
		list.push_back("gru");

	return list;
}

EGCResults __fastcall Hooks::SendMessageGameCoordinator(void* ecx, DWORD edx, uint32_t unMsgType, const void *pubData, uint32_t cubData)
{
#if _MSC_VER > 1900
	using WarningFn = void(__stdcall*)(const char* pMsg, ...);
	static WarningFn Warning = (WarningFn)(GetProcAddress(GetModuleHandleA("tier0.dll"), "Warning"));
	using MsgFn = void(__stdcall*)(const char* pMsg, ...);
	static MsgFn Msg = (MsgFn)(GetProcAddress(GetModuleHandleA("tier0.dll"), "Msg"));
	const auto type = unMsgType & 0x7FFFFFFF;
	// aka 9103
	if (type == k_EMsgGCCStrike15_v2_MatchmakingClient2ServerPing) 
	{
		CMsgGCCStrike15_v2_MatchmakingClient2ServerPing message;
		// + 8 to skip the header
		if (message.ParseFromArray((void*)(((std::uintptr_t)pubData) + 8), cubData - 8))
		{
			for (auto i = 0; i < message.data_center_pings_size(); i++)
			{
				const auto datacenter = message.mutable_data_center_pings(i);
				
				uint32_t id = datacenter->data_center_id();
				int numchars = 0;
				char name[5];
				memset(name, 0, sizeof(name));
				char* pName = (char*)&id;

				for (auto m = 3; m >= 0; --m)
				{
					if (pName[m] != 0)
						name[numchars++] = pName[m];
				}

				name[4] = 0;

				if (datacenter->has_ping())
				{
					if (g_DesiredRelayCluster.length() <= 0)
					{
						Warning("Datacenter %s ping is %i\n", name, datacenter->ping());
					}
					else if (!strstr(name, g_DesiredRelayCluster.c_str()))
					{
						char *pStr = nullptr;
						auto list_of_close_datacenters = GetDatacenterNearDatacenter(name);
						if (!list_of_close_datacenters.empty())
						{
							for (auto& str : list_of_close_datacenters)
							{
								if (strstr(name, str))
								{
									pStr = str;
									break;
								}
							}
						}
					
						if (pStr)
						{
							Warning("Datacenter %s ping is now %i, was %i\n", name, 5, datacenter->ping());
							datacenter->set_ping(5);
						}
						else
						{
							Warning("Datacenter %s ping is %i\n", name, datacenter->ping());
						}
					}
					else
					{
						Msg("Datacenter %s ping is now %i, was %i\n", name, 5, datacenter->ping());
						datacenter->set_ping(5);
					}
				}
				else
				{
					Warning("Datacenter %s has no ping!\n", name);
				}
			}

			const auto size = message.ByteSizeLong() + 8;
			const auto buffer = new std::uint8_t[size];
			auto status = EGCResults{};

			// copy header
			std::memcpy(buffer, pubData, 8);
			// serialize message
			if (message.SerializeToArray(buffer + 8, size - 8))
				status = oSendMessageSteamGameCoordinator(ecx, unMsgType, buffer, size);
			else
				// let's not crash / not queue and send the original data
				status = oSendMessageSteamGameCoordinator(ecx, unMsgType, pubData, cubData);

			delete[] buffer;

			return status;
		}
		else
		{
			Warning("WARNING: Failed to parse datacenter pings!\n");
		}
	}
	else if (type == k_EMsgGCCStrike15_v2_ClientPlayerDecalSign)
	{
#if 0
		CMsgGCCStrike15_v2_ClientPlayerDecalSign message;

		bool did = message.ParseFromArray((void*)(((std::uintptr_t)pubData) + 8), cubData - 8);

		if (did)
		{
			Vector start = LocalPlayer.Entity->Weapon_ShootPosition();
			QAngle angles = LocalPlayer.Entity->GetEyeAngles();
			Vector vecForward, vecRight, vecUp;
			AngleVectors(angles, &vecForward, &vecRight, &vecUp);

			trace_t tr;
			CTraceFilter filter;
			filter.pSkip = (IHandleEntity*)LocalPlayer.Entity;
			filter.m_icollisionGroup = COLLISION_GROUP_NONE;
			Ray_t ray;
			ray.Init(start, start + vecForward * 60.0f);
			Interfaces::EngineTrace->TraceRay(ray, MASK_SHOT, &filter, &tr);

			Vector newnormal = tr.plane.normal;

			Vector endpos = { message.data().endpos()[0], message.data().endpos()[1], message.data().endpos()[2] };
			Vector startpos = { message.data().startpos()[0], message.data().startpos()[1], message.data().startpos()[2] };
			Vector normal = { message.data().normal()[0], message.data().normal()[1], message.data().normal()[2] };
			Vector right = { message.data().right()[0], message.data().right()[1], message.data().right()[2] };

			//if (tr.DidHit())
			{
				//if (!GetAsyncKeyState(VK_RETURN))
				//{
				//	return k_EGCResultOK;
				//}

#if 1
				//((PlayerDecalDigitalSignature&)message.data()).set_startpos(2, message.data().startpos()[2] + 0.001f);
				((PlayerDecalDigitalSignature&)message.data()).set_accountid(INT_MAX);
#else
				((PlayerDecalDigitalSignature&)message.data()).set_endpos(0, 389.911133f);
				((PlayerDecalDigitalSignature&)message.data()).set_endpos(1, 2185.03027f);
				((PlayerDecalDigitalSignature&)message.data()).set_endpos(2, -130.897903f);
				((PlayerDecalDigitalSignature&)message.data()).set_startpos(0, 389.924957f);
				((PlayerDecalDigitalSignature&)message.data()).set_startpos(1, 2185.04639f);
				((PlayerDecalDigitalSignature&)message.data()).set_startpos(2, -129.898132f);
				((PlayerDecalDigitalSignature&)message.data()).set_normal(0, 0.0138118248f);
				((PlayerDecalDigitalSignature&)message.data()).set_normal(1, 0.0160706788f);
				((PlayerDecalDigitalSignature&)message.data()).set_normal(2, 0.999775529f);
				((PlayerDecalDigitalSignature&)message.data()).set_right(0, -0.156776115f);
				((PlayerDecalDigitalSignature&)message.data()).set_right(1, 0.987634182f);
				((PlayerDecalDigitalSignature&)message.data()).set_right(2, -0.0f);
#endif

				const auto size = message.ByteSizeLong() + 8;
				const auto buffer = new std::uint8_t[size];
				auto status = EGCResults{};

				// copy header
				std::memcpy(buffer, pubData, 8);
				// serialize message
				if (message.SerializeToArray(buffer + 8, size - 8))
					status = oSendMessageSteamGameCoordinator(ecx, unMsgType, buffer, size);
				else
					// let's not crash / not queue and send the original data
					status = oSendMessageSteamGameCoordinator(ecx, unMsgType, pubData, cubData);

				delete[] buffer;

				return status;
			}
			{
				return k_EGCResultOK;
			}
		}
#endif
	}
#endif
	return oSendMessageSteamGameCoordinator(ecx, unMsgType, pubData, cubData);
}

EGCResults __fastcall Hooks::RetrieveMessageGameCoordinator(void* ecx, DWORD edx, uint32_t *punMsgType, void *pubDest, uint32_t cubDest, uint32_t *pcubMsgSize)
{
#if _MSC_VER > 1900
	using WarningFn = void(__stdcall*)(const char* pMsg, ...);
	static WarningFn Warning = (WarningFn)(GetProcAddress(GetModuleHandleA("tier0.dll"), "Warning"));
	const auto type = *punMsgType & 0x7FFFFFFF;

	if (type == k_EMsgGCCStrike15_v2_MatchmakingGC2ClientReserve)
	{
		CMsgGCCStrike15_v2_MatchmakingGC2ClientReserve message;
		// + 8 to skip the header
		message.ParsePartialFromArray((void*)((DWORD)pubDest + 8), *pcubMsgSize - 8);

		if (message.has_serverid() && message.has_reservationid() && message.has_reservation() && message.has_server_address())
		{
			Warning("SERVER ID %" PRIu64 "\nMATCH ID %" PRIu64 "\nRESERVATION ID %" PRIu64 "\nSERVER ADR %s\n", message.serverid(), message.reservation().match_id(), message.reservationid(), message.server_address().c_str());
		}
	}
#endif
	return oRetrieveMessageSteamGameCoordinator(ecx, punMsgType, pubDest, cubDest, pcubMsgSize);
}