#include "precompiled.h"
#include "VTHook.h"

AllowThirdPersonFn oAllowThirdPerson;

BOOLEAN __fastcall Hooks::AllowThirdPerson(void* gamerulespointer)
{
	return true;
}