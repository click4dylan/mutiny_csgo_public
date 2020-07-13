#include "precompiled.h"
#include "tempents.h"
#include "VTHook.h"

CTempEnts* tempents = nullptr;
TempEntsUpdateFn oTempEntsUpdate = nullptr;

void __fastcall Hooks::HookedTempEntsUpdate(void* tempentsptr)
{
	//oTempEntsUpdate((CTempEnts*)tempentsptr);
}