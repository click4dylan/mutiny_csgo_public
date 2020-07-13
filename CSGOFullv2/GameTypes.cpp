#include "precompiled.h"
#include "Gametypes.h"
GameTypes **gametypes;

bool IsPlayingGuardian()
{
	if ((*gametypes)->GetCurrentGameType() != 4 || (*gametypes)->GetCurrentGameMode() != 1)
		return false;
	return true;
}