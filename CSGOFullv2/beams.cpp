#include "precompiled.h"
#include "beams.h"
#include "VTHook.h"

CBeams *beams;
UpdateTempEntBeamsFn oUpdateTempEntBeams;

//Beam updates are done in ProcessPacket hook now

void __fastcall Hooks::HookedUpdateTempEntBeams(void* beamsptr)
{
	//oUpdateTempEntBeams((CBeams*)beamsptr);
}