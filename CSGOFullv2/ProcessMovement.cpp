#include "precompiled.h"
#include "Includes.h"
#include "VTHook.h"
#include "LocalPlayer.h"
#include "IGameMovement.h"

ProcessMovementFn oProcessMovement;

void __fastcall Hooks::ProcessMovement(void* ecx, DWORD edx, CBaseEntity* basePlayer, CMoveData* moveData)
{
	//Fix prediction errors when jumping
	moveData->m_bGameCodeMovedPlayer = false;

	oProcessMovement(ecx, basePlayer, moveData);
}