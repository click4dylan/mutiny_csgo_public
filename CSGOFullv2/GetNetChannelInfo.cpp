#include "precompiled.h"
#include "VTHook.h"

GetNetChannelInfoFn oGetNetChannelInfo;
DWORD GetNetChannelInfo_cvarcheck_retaddr;

INetChannelInfo* __fastcall Hooks::GetNetChannelInfo(void* eng)
{
	if ((DWORD)_ReturnAddress() == GetNetChannelInfo_cvarcheck_retaddr)
		return nullptr;

	return oGetNetChannelInfo(eng);
}