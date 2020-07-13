#pragma once
#include <Windows.h>

class ISteamClient;
typedef HANDLE HSteamUser;
typedef HANDLE HSteamPipe;
class ISteamGameCoordinator;

extern ISteamClient* SteamClient();
extern HSteamUser GetSteamUser();
extern HSteamPipe GetSteamPipe();
extern ISteamGameCoordinator* GetSteamGameCoordinator();