#include <Windows.h>
#include "SteamAPI.h"
#include "isteamclient.h"
#include "isteamgamecoordinator.h"

ISteamClient* SteamClient()
{
	static ISteamClient* adr = ((ISteamClient*(*)())GetProcAddress(GetModuleHandle("steam_api.dll"), "SteamClient"))();;
	return adr;
}

HSteamUser GetHSteamUser()
{
	static HSteamUser adr = ((HSteamUser(*)())GetProcAddress(GetModuleHandle("steam_api.dll"), "GetHSteamUser"))();;
	return adr;
}

HSteamPipe GetHSteamPipe()
{
	static HSteamPipe* adr = ((HSteamPipe*(*)())GetProcAddress(GetModuleHandle("steam_api.dll"), "GetHSteamPipe"))();;
	return adr;
}

ISteamGameCoordinator* GetSteamGameCoordinator()
{
	return (ISteamGameCoordinator*)SteamClient()
		->GetISteamGenericInterface(GetHSteamUser(),
			GetHSteamPipe(),
			STEAMGAMECOORDINATOR_INTERFACE_VERSION);
}