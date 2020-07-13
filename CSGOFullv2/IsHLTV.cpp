#include "precompiled.h"
#include "VTHook.h"
#include <intrin.h> //VS2017 requires this for _ReturnAddress
IsHLTVFn oIsHLTV;

DWORD AdrOf_SetupVelocityReturnAddress;
bool bIsSettingUpBones = false;

bool __fastcall Hooks::IsHLTV(void* thisptr)
{
	//if (!g_Convars.Compatibility.disable_all->GetBool())
	{
		if (bIsSettingUpBones)
			return true;
		
#if 1
		if ((DWORD)_ReturnAddress() == AdrOf_SetupVelocityReturnAddress)
		{
			return true;
		}
#else
		if ((DWORD)_ReturnAddress() == AdrOf_SetupVelocityReturnAddress)
		{
			//ToDo-important: fix this
			if (gLagCompensation.pCPlayerCurrentlyProcessing && gLagCompensation.pCPlayerCurrentlyProcessing->BaseEntity)
			{
				CBaseEntity *Entity = gLagCompensation.pCPlayerCurrentlyProcessing->BaseEntity;
				Entity->SetAbsVelocityDirect(gLagCompensation.pCPlayerCurrentlyProcessing->vecNewestVelocity);
				Entity->RemoveEFlag(EFL_DIRTY_ABSVELOCITY);
				//gLagCompensation.pCPlayerCurrentlyProcessing->BaseEntity->SetAbsVelocity(gLagCompensation.pCPlayerCurrentlyProcessing->vecNewestVelocity);
				//Vector test = *(Vector*)((DWORD)gLagCompensation.pCPlayerCurrentlyProcessing->BaseEntity + 0x94);
				return true;
			}
		}
#endif
	}

	return oIsHLTV(thisptr);
}