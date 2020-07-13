#include "precompiled.h"
#include "C_TEMuzzleflash.h"
#include "Interfaces.h"

TE_MuzzleFlash_PostDataUpdateFn oTE_MuzzleFlash_PostDataUpdate;

extern void __fastcall Hooks::TE_MuzzleFlash_PostDataUpdate(C_TEMuzzleFlash* thisptr, void* edx, DataUpdateType_t updateType)
{


	oTE_MuzzleFlash_PostDataUpdate(thisptr, updateType);
}