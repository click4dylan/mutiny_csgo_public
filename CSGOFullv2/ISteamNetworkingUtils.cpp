#include "ISteamNetworkingUtils.h"
#include <Windows.h>

ISteamNetworkingUtils* SteamNetworkingUtils()
{
	static ISteamNetworkingUtils* SteamNetworkingSockets = ((ISteamNetworkingUtils*(*)())GetProcAddress(GetModuleHandleA("steamnetworkingsockets.dll"), "SteamNetworkingUtils_Lib"))(); //(DWORD*)*(DWORD*)(FindMemoryPattern(EngineHandle, "8B  0D  ??  ??  ??  ??  56  85  C9  75  12  68  ??  ??  ??  ??"));

	return (ISteamNetworkingUtils*)SteamNetworkingSockets;
}

NET_SendToFn NET_SendTo = nullptr;