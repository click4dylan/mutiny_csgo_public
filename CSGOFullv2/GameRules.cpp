#include "precompiled.h"
#include "VTHook.h"
#include "C_CSGameRules.h"
#include "C_CSGameTypes.h"
#include "UsedConvars.h"

C_CSGameRules **g_pGameRules;
IGameTypes **g_pGameTypes;

bool C_CSGameRules::IsFreezePeriod() { return *(bool*)((DWORD)this + g_NetworkedVariables.Offsets.m_bFreezePeriod); }
void* C_CSGameRules::GetSurvivalGamerules() { return (void*)((DWORD)this + g_NetworkedVariables.Offsets.m_SurvivalRules); }
bool C_CSGameRules::IsValveServer() { return *(bool*)((DWORD)this + g_NetworkedVariables.Offsets.m_bIsValveDS); }
void* C_CSGameRules::GetCustomizedGameRules()
{
	auto gametypes = GetGameTypes();
	if ((gametypes->GetCurrentGameType() != 6 || gametypes->GetCurrentGameMode()) && !mp_coopmission_dz.GetVar()->GetBool())
		return nullptr;
	return GetSurvivalGamerules();
}

void HookGamerules()
{
	if (!*(DWORD*)(DWORD)g_pGameRules)
	{
		if (GameRules)
		{
			GameRules->ClearClassBase();
			delete GameRules;
			GameRules = nullptr;
		}
	}
	else
	{
		if (!GameRules || GetVT((DWORD)g_pGameRules) != GameRules->GetNewVT())
		{
			if (GameRules)
			{
				GameRules->ClearClassBase();
				delete GameRules;
			}
			GameRules = new VTHook((DWORD**)*(DWORD*)(DWORD)g_pGameRules, hook_types::_Gamerules);
			oAllowThirdPerson = (AllowThirdPersonFn)GameRules->HookFunction((DWORD)Hooks::AllowThirdPerson, AllowThirdPersonOffset);
		}
	}
}