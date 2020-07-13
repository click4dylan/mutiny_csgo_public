#include "CreateMove.h"
#include "CPlayerrecord.h"

DWORD oCSPlayerAnimState_Release = 0; //TODO: fixme: get pBaseEntity from this animstate then ToPlayerRecord()->oCSPlayerAnimState_Release

void __fastcall Hooks::CSPlayerAnimState_Release(void* baseanimstate)
{
	CBaseEntity* pBaseEntity = (CBaseEntity*)(*(DWORD*)((DWORD)baseanimstate + 28));
	((void(__thiscall*)(void*))oCSPlayerAnimState_Release)(baseanimstate);
	return;
	if (pBaseEntity)
	{
		CPlayerrecord *_record = pBaseEntity->ToPlayerRecord();
		if (_record && _record->Hooks.m_pBaseAnimStateHook)
		{
			_record->Hooks.m_pBaseAnimStateHook->ClearClassBase();
			delete _record->Hooks.m_pBaseAnimStateHook;
			_record->Hooks.m_pBaseAnimStateHook = nullptr;
			_record->Hooks.m_pBaseAnimState = nullptr;
		}
	}
}