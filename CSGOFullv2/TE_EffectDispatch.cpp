#include "precompiled.h"


/////// THIS FUNCTION IS NOT USED ANYMORE





#include "TE_EffectDispatch.h"
#include "VTHook.h"
#include "SpawnBlood.h"
#include "VMProtectDefs.h"
#include "Interfaces.h"

TE_EffectDispatch_PostDataUpdateFn oTE_EffectDispatch_PostDataUpdate;

#if 0
//dumpstringtables
Table EffectDispatch(changed on ticks 838, 0)
16 / 1024 items
0 : error
1 : csblood
2 : gunshotsplash
3 : CS_MuzzleFlash
4 : KnifeSlash
5 : ParticleEffect
6 : ParticleEffectStop
7 : GlassImpact
8 : Impact
9 : RagdollImpact
10 : TracerSound
11 : Tracer
12 : watersplash
13 : waterripple
14 : bloodimpact
15 : ParticleTracer
#endif

void __fastcall Hooks::TE_EffectDispatch_PostDataUpdate(C_TEEffectDispatch* thisptr, void* edx, DataUpdateType_t updateType)
{
	VMP_BEGINMUTILATION("EFDPDU")
	//if (!g_Convars.Compatibility.disable_all->GetBool())
	{ 
		//decimal
		//4 = csblood
		//34 = Impact

		/*
		CBaseEntity* pEntity = thisptr->m_EffectData.m_hEntity != -1 ? Interfaces::ClientEntList->GetClientEntityFromHandle(thisptr->m_EffectData.m_hEntity) : nullptr;
		if (pEntity)
		{
			if (thisptr->m_EffectData.m_vOrigin.Length() > 5.0f == 0)
			{
				Interfaces::DebugOverlay->AddBoxOverlay(thisptr->m_EffectData.m_vOrigin, Vector(-0.5f, -0.5f, -0.5f), Vector(0.5f, 0.5f, 0.5f), angZero, 0, 0, 255, 255, 4.5f);
			}
			else
			{
				Interfaces::DebugOverlay->AddBoxOverlay(thisptr->m_EffectData.m_vOrigin + pEntity->GetLocalOriginDirect(), Vector(-0.5f, -0.5f, -0.5f), Vector(0.5f, 0.5f, 0.5f), angZero, 255, 0, 0, 255, 4.5f);
			}
		}
		*/

		DWORD offset = (DWORD)&thisptr->m_EffectData.m_vNormal - (DWORD)&thisptr->m_EffectData;

		switch (thisptr->m_EffectData.m_iEffectName)
		{
			case 4:
			{
				OnSpawnBlood(thisptr, updateType);
				return;
			}
			case 8:
			{
				OnSpawnImpact(thisptr, updateType);
				return;
			}
			default:
			{
				oTE_EffectDispatch_PostDataUpdate(thisptr, updateType);
				return;
			}
		}
	}
	//else
	//{
	//	oTE_EffectDispatch_PostDataUpdate(thisptr, updateType);
	//}
	VMP_END
}