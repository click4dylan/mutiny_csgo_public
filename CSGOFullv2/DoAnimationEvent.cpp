#include "VTHook.h"
#include "BaseEntity.h"
#include "IClientEntityList.h"
#include "CPlayerrecord.h"
#include "LocalPlayer.h"

DWORD oDoAnimationEvent = 0; //TODO: fixme: get pBaseEntity from this animstate then ToPlayerRecord()->oDoAnimationEvent

void __fastcall Hooks::Hooked_DoAnimationEvent(void* animstate, void* edx, PlayerAnimEvent_t event, int data)
{
	CBaseEntity* pBaseEntity = (CBaseEntity*)(*(DWORD*)((DWORD)animstate + 28));
	if (pBaseEntity)
	{
		CPlayerrecord *_record = pBaseEntity->ToPlayerRecord();
		if (_record)
		{
			if (_record->m_pAnimStateServer[ResolveSides::NONE] && _record->m_bAllowAnimationEvents)
			{
				if (pBaseEntity->IsLocalPlayer() && LocalPlayer.bInPrediction_Start)
				{
					//queue the animation event when we are in CreateMove engine prediction
					//this is because the animations will be overwritten again in PredictLowerBodyYaw
					LocalPlayer.m_PredictionAnimationEventQueue.push_back(event);
				}
				else
				{
					_record->m_pAnimStateServer[ResolveSides::NONE]->DoAnimationEvent(event);
				}
			}
			return;
		}
	}
	((void(__thiscall*)(void*, PlayerAnimEvent_t, int))oDoAnimationEvent)(animstate, event, data);
}