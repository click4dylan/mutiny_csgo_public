#include "VTHook.h"
#include "Interfaces.h"

SetReservationCookieFn oSetReservationCookie;
unsigned long long g_ServerReservationCookie = 0;

void __fastcall Hooks::SetReservationCookie(void* clientstate, void* edx, unsigned long long cookie)
{
	g_ServerReservationCookie = cookie;
	oSetReservationCookie(clientstate, cookie);
}