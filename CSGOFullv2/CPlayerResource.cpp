#include "CPlayerResource.h"
#include "NetworkedVariables.h"

CPlayerResource *g_PR = nullptr;

#define STRING( offset )	( ( offset ) ? reinterpret_cast<const char *>( offset ) : "" )

const char* CPlayerResource::GetClan(int id)
{
	DWORD resource = *(DWORD*)g_PR;
	if (!resource)
		return nullptr;
	return STRING((resource + g_NetworkedVariables.Offsets.m_szClan + (id * 16)));
}