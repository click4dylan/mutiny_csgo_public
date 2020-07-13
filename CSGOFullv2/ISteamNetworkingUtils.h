#pragma once

#include <Windows.h>
#include <inttypes.h>

struct SteamDatagramRelayAuthTicket;
struct SteamRelayNetworkStatus_t;
class SteamNetworkingMessage_t;
enum ESteamNetworkingSocketsDebugOutputType;
enum ESteamNetworkingConfigValue;
enum ESteamNetworkingConfigScope;
enum ESteamNetworkingConfigDataType;
class SteamNetworkingIPAddr;
class SteamNetworkingIdentity;
enum ESteamNetworkingConfigValue;
enum ESteamNetworkingGetConfigValueResult;
typedef __int64 SteamNetworkingMicroseconds;
typedef void(*FSteamNetworkingSocketsDebugOutput)(ESteamNetworkingSocketsDebugOutputType nType, const char *pszMsg);

class ISteamNetworkingUtils
{
public:
	virtual void(csgospecific1)();
	virtual void(csgospecific2)();
	virtual void(csgospecific3)();
	virtual void(csgospecific4)();
	virtual void(csgospecific5)();
	virtual void(csgospecific6)();
	virtual void(csgospecific7)();
	virtual void(csgospecific8)();
	virtual void(csgospecific9)();
	virtual void(csgospecific10)();
	virtual void(csgospecific11)();

	virtual SteamNetworkingMicroseconds GetLocalTimestamp();


	virtual void SetDebugOutputFunction(ESteamNetworkingSocketsDebugOutputType eDetailLevel, FSteamNetworkingSocketsDebugOutput pfnFunc);


	virtual bool SetConfigValue(ESteamNetworkingConfigValue eValue, ESteamNetworkingConfigScope eScopeType, intptr_t scopeObj,
		ESteamNetworkingConfigDataType eDataType, const void *pArg);


	virtual ESteamNetworkingGetConfigValueResult GetConfigValue(ESteamNetworkingConfigValue eValue, ESteamNetworkingConfigScope eScopeType, intptr_t scopeObj,
		ESteamNetworkingConfigDataType *pOutDataType, void *pResult, size_t *cbResult);


	virtual bool GetConfigValueInfo(ESteamNetworkingConfigValue eValue, const char **pOutName, ESteamNetworkingConfigDataType *pOutDataType, ESteamNetworkingConfigScope *pOutScope, ESteamNetworkingConfigValue *pOutNextValue);


	virtual ESteamNetworkingConfigValue GetFirstConfigValue();


	virtual void SteamNetworkingIPAddr_ToString(const SteamNetworkingIPAddr &addr, char *buf, size_t cbBuf, bool bWithPort);
	virtual bool SteamNetworkingIPAddr_ParseString(SteamNetworkingIPAddr *pAddr, const char *pszStr);
	virtual void SteamNetworkingIdentity_ToString(const SteamNetworkingIdentity &identity, char *buf, size_t cbBuf);
	virtual bool SteamNetworkingIdentity_ParseString(SteamNetworkingIdentity *pIdentity, const char *pszStr);

protected:
	~ISteamNetworkingUtils(); // Silence some warnings
};

extern ISteamNetworkingUtils* SteamNetworkingUtils();

using NET_SendToFn = int(__fastcall*)(void* netchan, unsigned char* buffer, int length, int unknown);
extern NET_SendToFn NET_SendTo;

// Use this to pick apart the network stream, must be packed
#pragma pack(1)
typedef struct
{
	int		netID;
	int		sequenceNumber;
	int		packetID : 16;
	int		nSplitSize : 16;
} SPLITPACKET;
#pragma pack(pop)

#define LittleShort( val )			( val )
#define LittleLong( val )			( val )
#define NET_HEADER_FLAG_SPLITPACKET				-2
#define NET_HEADER_FLAG_COMPRESSEDPACKET		-3
#define CONNECTIONLESS_HEADER 0xffffffff