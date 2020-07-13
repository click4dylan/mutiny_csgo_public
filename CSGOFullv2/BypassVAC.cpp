#include "precompiled.h"
#include "Interfaces.h"
#include "BaseEntity.h"
#include <unordered_map>
#include "LocalPlayer.h"


IMAGE_DOS_HEADER* GetPEHeader(uint32_t Module)
{
	auto dosHd = reinterpret_cast<IMAGE_DOS_HEADER*>(Module);

	if (dosHd->e_magic != IMAGE_DOS_SIGNATURE)
		return nullptr;

	return dosHd;
}

PIMAGE_NT_HEADERS32 GetNTHeader(uint32_t Module)
{
	auto DOSHeader = GetPEHeader(Module);

	if (!DOSHeader)
		return nullptr;

	auto NTHeader = reinterpret_cast<PIMAGE_NT_HEADERS32>(Module + DOSHeader->e_lfanew);

	if (NTHeader->Signature != IMAGE_NT_SIGNATURE)
		return nullptr;

	if (NTHeader->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
		return nullptr;

	return NTHeader;
}


VoiceStatusFn oVoiceStatus;
FireGameEventFn oFireEvent;
LevelShutdownPreEntityFn oGameStats_LevelShutdownPreEntity;
LevelInitPreEntityFn oGameStats_LevelInitPreEntity;
GameStatsInitFn oGameStatsInit;
ModelRenderGlowFn oModelRenderGlow;
DWORD ModelRenderGlowVMT = NULL; //This is actually ClientMode
DWORD LevelShutdownPreEntityVMT = NULL;
DWORD LevelInitPreEntityVMT = NULL;
DWORD pLevelShutdownVMTWhenWeHooked = NULL;
DWORD pModelRenderGlowVMTWhenWeHooked = NULL;
VTHook *HLevelShutdownPreEntity = nullptr;
VTHook *HModelRenderGlow = nullptr;
extern std::unordered_map<int, HookedEntity*> HookedNonPlayerEntities;
bool HookedModelRenderGlow = false;

DWORD *CHLClientTrap;
DWORD *CHLClientTrap2;
DWORD *LocalPlayerEntityTrap;
DWORD *EngineClientTrap;
DWORD *EngineClientTrap2;
DWORD *ModelRenderTrap;
DWORD *ModelRenderTrap2;

class vac_vtable_scan_s
{
public:
	char pad_0000[8]; //0x0000
	DWORD m_pCHLClient; //0x0008			m_pCHLClient |= CHLCLIENT ^ 0xA4D8BDDA;
	DWORD m_pCHLClient2; //0x000C			m_pCHLClient2 = CHLCLIENT ^ 0x46C2E52C;
	char pad_0010[12]; //0x0010
	DWORD m_pEngineClient; //0x001C			m_pEngineClient |= *(_DWORD *)engineclient ^ 0xC7CD9E05;
	DWORD m_pEngineClient2; //0x0020		m_pEngineClient2 = *(_DWORD *)engineclient ^ 0xE9882500;
	char pad_0024[8]; //0x0024
	DWORD m_pC_CSPlayer; //0x002C			m_pC_CSPlayer |= *(_DWORD *)v64 ^ 0x39142DD4;
	char pad_0030[4]; //0x0030
	DWORD m_pDebugOverlay; //0x0034			why would you hook that
	DWORD m_pDebugOverlay2; //0x0038
	DWORD m_pEngineVGUI; //0x003C			no hook
	DWORD m_pEngineVGUI2; //0x0040
	DWORD m_pRenderView; //0x0044			no hook
	DWORD m_pRenderView2; //0x0048
	DWORD m_pMaterialSystem; //0x004C		no hook
	DWORD m_pMaterialSystem2; //0x0050
	DWORD m_pGameUIFuncs; //0x0054			who the fuck hooks dat
	DWORD m_pGameUIFuncs2; //0x0058
	DWORD m_pInputSystem; //0x005C			i dont hook dat
	DWORD m_pInputSystem2; //0x0060
	DWORD m_pModelRender; //0x0064			#define VENGINE_HUDMODEL_INTERFACE_VERSION	"VEngineModel016" not hooked
	DWORD m_pModelRender2; //0x0068
	DWORD m_pEngineEffects; //0x006C		i dont hook dat
	DWORD m_pEngineEffects2; //0x0070
	char pad_0074[140]; //0x0074
}; //Size: 0x0100

void RestoreVACCedPointers()
{
	if (g_bIsExiting)
		return;
#ifdef DYLAN_VAC
	if (Client)
		*CHLClientTrap = (DWORD)Client->GetOldVT() ^ 0xA4D8BDDA;

	if (Engine)
		*EngineClientTrap = (DWORD)Engine->GetOldVT() ^ 0xC7CD9E05;

	if (ModelRenderMo)
	{
		*ModelRenderTrap = (DWORD)ModelRenderMo->GetOldVT() ^ 0x8F514AC;
		*ModelRenderTrap2 = (DWORD)ModelRenderMo->GetOldVT() ^ 0xBCCD9981;
	}

	if (HRenderView)
	{
		static DWORD* trap1 = StaticOffsets.GetOffsetValueByType<DWORD*>(_RenderViewTrap1);
		static DWORD* trap2 = StaticOffsets.GetOffsetValueByType<DWORD*>(_RenderViewTrap2);
		*trap1 = (DWORD)HRenderView->GetOldVT() ^ 0xAE500D33;
		*trap2 = (DWORD)HRenderView->GetOldVT() ^ 0x301A2315;
	}

	if (Interfaces::EngineClient && Interfaces::ClientEntList)
	{
		auto LocalPlayer = Interfaces::EngineClient->GetLocalPlayer();
		CBaseEntity* pLocalEntity = Interfaces::ClientEntList->GetBaseEntity(LocalPlayer);
		if (pLocalEntity)
		{
			auto record = g_LagCompensation.GetPlayerrecord(pLocalEntity);
			if (record && record->Hooks.m_bHookedBaseEntity)
			{
				*LocalPlayerEntityTrap = (DWORD)record->Hooks.BaseEntity->GetHook()->GetOldVT() ^ 0x39142DD4;
			}
		}
	}
#else

	// NOTE: bypass the 7600 VAC3 module

	// NOTE: panorama version fix since it doesn't have the xor start & end markers
	static uint32_t dataSection = 0;

	if (!dataSection)
	{
		auto ntHdr = GetNTHeader(reinterpret_cast<uint32_t>(ClientHandle));

		auto sections = IMAGE_FIRST_SECTION(ntHdr);

		for (int i = 0; i < ntHdr->FileHeader.NumberOfSections && !g_bIsExiting; ++i)
		{
			if (!memcmp((char*)sections[i].Name, (char*)".data", 5))
			{
				dataSection = reinterpret_cast<uint32_t>(ClientHandle) + sections[i].VirtualAddress;
				break;
			}
		}
	}
	else
		memset((void*)dataSection, 0, 0x100);
#endif
}

void __fastcall Hooks::VoiceStatus(void* ECX, void* EDX, int entindex, int iSsSlot, bool bTalking)
{
	oVoiceStatus(ECX, entindex, iSsSlot, bTalking);
	
#ifdef DYLAN_VAC
	*CHLClientTrap = (DWORD)Client->GetOldVT() ^ 0xA4D8BDDA;
	*EngineClientTrap = (DWORD)Engine->GetOldVT() ^ 0xC7CD9E05;
#endif
}

void __fastcall Hooks::FireEvent(void* ECX, void* EDX, CGameEvent*event, bool bServerOnly)
{
	oFireEvent(ECX, event, bServerOnly);

#ifdef DYLAN_VAC
	if (Interfaces::EngineClient->IsConnected())
	{
		auto LocalPlayer = Interfaces::EngineClient->GetLocalPlayer();
		CBaseEntity* pLocalEntity = Interfaces::ClientEntList->GetBaseEntity(LocalPlayer);
		if (pLocalEntity)
		{
			auto record = g_LagCompensation.GetPlayerrecord(pLocalEntity);
			if (record && record->Hooks.m_bHookedBaseEntity)
			{
				*LocalPlayerEntityTrap = (DWORD)record->Hooks.BaseEntity->GetHook()->GetOldVT() ^ 0x39142DD4;
			}
		}
	}
#endif
}

unsigned char __fastcall Molotov_Simulate(CBaseEntity* me)
{
	auto hook = HookedNonPlayerEntities.find(me->index);
	unsigned char result = 1;
	if (hook != HookedNonPlayerEntities.end())
		result = ((unsigned char (__thiscall*)(CBaseEntity*))hook->second->GetOriginalHookedSub1())(me);

#ifdef DYLAN_VAC
	auto LocalPlayer = Interfaces::EngineClient->GetLocalPlayer();
	CBaseEntity* pLocalEntity = Interfaces::ClientEntList->GetBaseEntity(LocalPlayer);
	if (pLocalEntity)
	{
		auto record = g_LagCompensation.GetPlayerrecord(pLocalEntity);
		if (record && record->Hooks.m_bHookedBaseEntity)
		{
			*LocalPlayerEntityTrap = (DWORD)record->Hooks.BaseEntity->GetHook()->GetOldVT() ^ 0x39142DD4;
		}
	}
#endif

	return result;
}

void __fastcall Hooks::GameStats_LevelShutdownPreEntity(void* ECX)
{
	oGameStats_LevelShutdownPreEntity(ECX);

#ifdef DYLAN_VAC
	*CHLClientTrap = (DWORD)Client->GetOldVT() ^ 0xA4D8BDDA;
	*EngineClientTrap = (DWORD)Engine->GetOldVT() ^ 0xC7CD9E05;
#endif
}

void __fastcall Hooks::GameStats_LevelInitPreEntity(void* ECX)
{
	oGameStats_LevelInitPreEntity(ECX);

#ifdef DYLAN_VAC
	*CHLClientTrap2 = (DWORD)Client->GetOldVT() ^ 0x46C2E52C;
	*EngineClientTrap2 = (DWORD)Engine->GetOldVT() ^ 0xE9882500;
#endif
}

void __fastcall Hooks::GameStats_Init(void* ECX)
{
	oGameStatsInit(ECX);

#ifdef DYLAN_VAC
	static DWORD* trap1 = StaticOffsets.GetOffsetValueByType<DWORD*>(_RenderViewTrap1);
	static DWORD* trap2 = StaticOffsets.GetOffsetValueByType<DWORD*>(_RenderViewTrap2);
	*trap1 = (DWORD)HRenderView->GetOldVT() ^ 0xAE500D33;
	*trap2 = (DWORD)HRenderView->GetOldVT() ^ 0x301A2315;
#endif
}

void Hooks::HookLevelShutdownAndInitPreEntity()
{
	DWORD pGameStats_LevelShutdownPreEntityVMT = *(DWORD*)((50 * 4) + *(DWORD*)LevelShutdownPreEntityVMT);

	if (!pGameStats_LevelShutdownPreEntityVMT || pLevelShutdownVMTWhenWeHooked != pGameStats_LevelShutdownPreEntityVMT)
	{
		if (HLevelShutdownPreEntity)
		{
			if (!pGameStats_LevelShutdownPreEntityVMT || GetVT((DWORD**)pGameStats_LevelShutdownPreEntityVMT) != HLevelShutdownPreEntity->GetNewVT())
			{
				HLevelShutdownPreEntity->ClearClassBase();
				delete HLevelShutdownPreEntity;
				HLevelShutdownPreEntity = nullptr;
			}
		}
		pLevelShutdownVMTWhenWeHooked = pGameStats_LevelShutdownPreEntityVMT;
	}

	if (pGameStats_LevelShutdownPreEntityVMT && (!HLevelShutdownPreEntity || HLevelShutdownPreEntity->GetCurrentVT() != HLevelShutdownPreEntity->GetNewVT()))
	{
		if (HLevelShutdownPreEntity)
		{
			HLevelShutdownPreEntity->ClearClassBase();
			delete HLevelShutdownPreEntity;
			HLevelShutdownPreEntity = nullptr;
		}

		HLevelShutdownPreEntity = new VTHook((DWORD**)pGameStats_LevelShutdownPreEntityVMT, hook_types::_HookLevelShutdownAndInitPreEntity);

		oGameStats_LevelShutdownPreEntity = (LevelShutdownPreEntityFn)HLevelShutdownPreEntity->HookFunction((DWORD)&Hooks::GameStats_LevelShutdownPreEntity, 6);
		oGameStats_LevelInitPreEntity = (LevelInitPreEntityFn)HLevelShutdownPreEntity->HookFunction((DWORD)&Hooks::GameStats_LevelInitPreEntity, 4);
		oGameStatsInit = (GameStatsInitFn)HLevelShutdownPreEntity->HookFunction((DWORD)&Hooks::GameStats_Init, 1);
	}
}

void __fastcall Hooks::ModelRenderGlow(void* ECX, void* EDX, DWORD unknown)
{
	oModelRenderGlow(ECX, unknown);

#ifdef DYLAN_VAC
	*ModelRenderTrap = (DWORD)ModelRenderMo->GetOldVT() ^ 0x8F514AC;
	*ModelRenderTrap2 = (DWORD)ModelRenderMo->GetOldVT() ^ 0xBCCD9981;
#endif
}

void Hooks::HookModelRenderGlow()
{
	if (ClientMode && !HookedModelRenderGlow)
	{
#if 0
		DWORD pModelRenderGlowVMT = *(DWORD*)ModelRenderGlowVMT;// *(DWORD*)ModelRenderGlowVMT;

		if (!pModelRenderGlowVMT || pModelRenderGlowVMTWhenWeHooked != pModelRenderGlowVMT)
		{
			if (HModelRenderGlow)
			{
				if (!pModelRenderGlowVMT || GetVT((DWORD**)pModelRenderGlowVMT) != HModelRenderGlow->GetNewVT())
				{
					HModelRenderGlow->ClearClassBase();
					delete HModelRenderGlow;
					HModelRenderGlow = nullptr;
				}
			}
			pModelRenderGlowVMTWhenWeHooked = pModelRenderGlowVMT;
		}

		if (pModelRenderGlowVMT && (!HModelRenderGlow || HModelRenderGlow->GetCurrentVT() != HModelRenderGlow->GetNewVT()))
		{
			if (HModelRenderGlow)
			{
				HModelRenderGlow->ClearClassBase();
				delete HModelRenderGlow;
				HModelRenderGlow = nullptr;
			}

			HModelRenderGlow = new VTHook((DWORD**)pModelRenderGlowVMT);
			oModelRenderGlow = (ModelRenderGlowFn)HModelRenderGlow->HookFunction((DWORD)&Hooks::ModelRenderGlow, (0x0B0 / 4));
		}
#endif
		oModelRenderGlow = (ModelRenderGlowFn)ClientMode->HookFunction((DWORD)&Hooks::ModelRenderGlow, (0x0B0 / 4));
		HookedModelRenderGlow = true;
	}
}