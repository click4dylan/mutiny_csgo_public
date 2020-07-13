#include "VTHook.h"
#include "CClientState.h"

void CClientState::SetReservationCookie(unsigned long long cookie)
{
	PVOID pThis = (PVOID)((DWORD)this + 0x8);
	typedef void(__thiscall *OriginalFn)(PVOID, unsigned long long);
	return GetVFunc<OriginalFn>(pThis, 63) (pThis, cookie);
}