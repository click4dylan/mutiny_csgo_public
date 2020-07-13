#include "precompiled.h"
#include "VTHook.h"
#include "Interfaces.h"

ProcessTempEntitiesFn oProcessTempEntities;

bool __fastcall Hooks::ProcessTempEntities(void* clientstate, void* edx, void* msg)
{
	const auto oldmaxclients = g_ClientState->m_nMaxClients;

	//Override max clients to 1 so that interp does not get added onto temp entities. This makes them activate immediately rather than interp ticks behind
	g_ClientState->m_nMaxClients = 1;

	//Call original function
	const bool ret = oProcessTempEntities(clientstate, msg);

	//Restore old max clients
	g_ClientState->m_nMaxClients = oldmaxclients;

	CL_FireEvents();

	return ret;
}
