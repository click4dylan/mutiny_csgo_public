#include "precompiled.h"
#include "IClientEntityList.h"
#include "BaseEntity.h"
#include "VTHook.h"
#include "IClientUnknown.h"
#include "GameMemory.h"
#include "VMProtectDefs.h"
#include "CPlayerrecord.h"
#include "IClientRenderable.h"
#include "CWeatherController.h"

extern bool __fastcall HookedSetupBones(CBaseAnimating* pBaseAnimating, DWORD EDX, matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime);
extern void __fastcall HookedBuildTransformations(CBaseEntity* end, DWORD edx, CStudioHdr* hdr, Vector* pos, Quaternion* q, const matrix3x4_t& cameraTransform, int boneMask, byte* boneComputed);
extern void __fastcall HookedStandardBlendingRules(CBaseEntity* end, DWORD edx, CStudioHdr* hdr, Vector pos[], QuaternionAligned q[], float currentTime, int boneMask);
extern void __fastcall HookedAccumulateLayers(CBaseEntity* ent, DWORD edx, void* boneSetup, Vector pos[], Quaternion q[], float currentTime);
extern BOOLEAN __fastcall HookedEntityShouldInterpolate(CBaseEntity* me);
extern unsigned char __fastcall HookedPlayer_Simulate(CBaseEntity* me); //this is baseent->simulate
extern unsigned char __fastcall Molotov_Simulate(CBaseEntity* me); //this is baseent->simulate
extern BOOLEAN __fastcall HookedEntityIsPlayer(CBaseEntity* me);
extern void __fastcall HookedPlayFootstepSound(CBaseEntity* me, DWORD edx, Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force, bool unknown);
extern void __fastcall HookedPreDataUpdate(CBaseEntity* me, DWORD edx, DataUpdateType_t updateType);
extern void __fastcall HookedPostDataUpdate(CBaseEntity* me, DWORD edx, DataUpdateType_t updateType);
extern void __fastcall HookedDoExtraBoneProcessing(CBaseEntity* thisptr, DWORD edx, CStudioHdr *hdr, Vector *pos, Quaternion *q, matrix3x4_t* bonearray, byte* computed, DWORD m_pIK);
extern void __fastcall HookedEstimateAbsVelocity(CBaseEntity* me, DWORD edx, Vector& vel);
extern void __fastcall HookedPhysicsSimulate(CBaseEntity* me);
extern Vector& __fastcall HookedWeaponShootPosition(CBaseEntity* me, DWORD edx, Vector& dest);
extern void __fastcall HookedCalcView(CBaseEntity* me, DWORD edx, Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov);
extern bool __fastcall HookedTestHitboxes(CBaseEntity* me, DWORD edx, const Ray_t& ray, uint32_t mask, CGameTrace& trace);
extern std::unordered_map<int, HookedEntity*> HookedNonPlayerEntities;

OnCreateEntityFn oOnAddEntity;
OnDeleteEntityFn oOnRemoveEntity;

DWORD __fastcall HookedEntityDestructor(CBaseEntity* _Entity, DWORD EDX, DWORD unknown)
{
	CPlayerrecord* _playerRecord = &m_PlayerRecords[_Entity->index];
	if (_playerRecord->Hooks.BaseEntity)
	{
		_playerRecord->Hooks.BaseEntity->MarkForDeletion();
		if (_playerRecord->Hooks.ClientRenderable)
			_playerRecord->Hooks.ClientRenderable->MarkForDeletion(); //If the destructor can ever get hooked properly for this, then remove this line
		if (_playerRecord->Hooks.ClientNetworkable)
			_playerRecord->Hooks.ClientNetworkable->MarkForDeletion(); //If the destructor can ever get hooked properly for this, then remove this line															   

#ifdef _DEBUG
		auto adr = _playerRecord->Hooks.BaseEntity->GetOriginalHookedSub3();
		if (!adr)
			DebugBreak();
#endif
		return ((DWORD(__thiscall*)(CBaseEntity*, DWORD))_playerRecord->Hooks.BaseEntity->GetOriginalHookedSub3())(_Entity, unknown);
	}
	THROW_ERROR("Fatal Error in HED");
	exit(EXIT_SUCCESS);
	return NULL;
}

void HookPlayer(CBaseEntity* _Entity)
{
	CPlayerrecord* _playerRecord = &m_PlayerRecords[_Entity->index];
	
#ifdef _DEBUG
	if (_playerRecord->Hooks.BaseEntity)
	{
		DebugBreak();
	}
	if (_playerRecord->Hooks.ClientRenderable)
	{
		DebugBreak();
	}
	if (_playerRecord->Hooks.ClientNetworkable)
	{
		DebugBreak();
	}
#endif

	if (!_playerRecord->Hooks.m_bHookedBaseEntity)
	{
		//dest->HookedBaseEntity->OriginalHookedSub1 = dest->HookedBaseEntity->hook->HookFunction((DWORD)&HookedUpdateClientSideAnimation, OffsetOf_UpdateClientSideAnimation);
		VTHook* pHook = new VTHook((PDWORD*)_Entity, hook_types::_BasePlayer);
		DWORD HookedSub1 = pHook->HookFunction((DWORD)&HookedEntityIsPlayer, StaticOffsets.GetOffsetValue(_IsPlayer));
		DWORD HookedSub2 =  pHook->HookFunction((DWORD)&HookedEntityShouldInterpolate, StaticOffsets.GetOffsetValue(_ShouldInterpolate));
		DWORD HookedSub3 = pHook->HookFunction((DWORD)&HookedDoExtraBoneProcessing, StaticOffsets.GetOffsetValue(_DoExtraBoneProcessing));
		//DWORD HookedSub3 = pHook->HookFunction((DWORD)&HookedEntityDestructor, 0);
		DWORD HookedSub4 =  pHook->HookFunction((DWORD)&HookedPlayer_Simulate, StaticOffsets.GetOffsetValue(_Simulate));
		DWORD HookedSub5 =  pHook->HookFunction((DWORD)&HookedPlayFootstepSound, StaticOffsets.GetOffsetValue(_PlayFootstepSound));
		DWORD HookedSub6 =  pHook->HookFunction((DWORD)&HookedEstimateAbsVelocity, StaticOffsets.GetOffsetValue(_EstimateAbsVelocity));
		DWORD HookedSub7 = pHook->HookFunction((DWORD)&HookedPhysicsSimulate, StaticOffsets.GetOffsetValue(_PhysicsSimulate));
		DWORD HookedSub8 = pHook->HookFunction((DWORD)&HookedWeaponShootPosition, StaticOffsets.GetOffsetValue(_Weapon_ShootPosition));
		DWORD HookedSub9 = pHook->HookFunction((DWORD)&HookedCalcView, StaticOffsets.GetOffsetValue(_C_CSPlayer_CalcView));
		DWORD HookedSub10 = pHook->HookFunction((DWORD)&HookedUpdateClientSideAnimation, OffsetOf_UpdateClientSideAnimation);
		DWORD HookedSub11 = pHook->HookFunction((DWORD)&HookedBuildTransformations, StaticOffsets.GetOffsetValue(_BuildTransformations));
		DWORD HookedSub12 = pHook->HookFunction((DWORD)&HookedStandardBlendingRules, StaticOffsets.GetOffsetValue(_StandardBlendingRules));
		DWORD HookedSub13 = pHook->HookFunction((DWORD)&HookedAccumulateLayers, StaticOffsets.GetOffsetValue(_AccumulateLayersVMT));
		DWORD HookedSub14 = NULL;// pHook->HookFunction((DWORD)&HookedTestHitboxes, 52);

		_playerRecord->Hooks.BaseEntity = new HookedEntity(
			_Entity->index,
			_Entity,
			pHook,
			HookedSub1,
			HookedSub2,
			HookedSub3,
			HookedSub4,
			HookedSub5,
			HookedSub6,
			HookedSub7,
			HookedSub8,
			HookedSub9,
			HookedSub10,
			HookedSub11,
			HookedSub12,
			HookedSub13,
			HookedSub14,
			&_playerRecord->Hooks.m_bHookedBaseEntity,
			&_playerRecord->Hooks.BaseEntity,
			_Entity->GetRefEHandle().ToUnsignedLong()
		);
		_playerRecord->Hooks.m_bHookedBaseEntity = true;
	}

	if (!_playerRecord->Hooks.m_bHookedClientRenderable)
	{
		VTHook* pHook = new VTHook((PDWORD*)((DWORD)_Entity + 4), hook_types::_BaseRenderable);
		_playerRecord->Hooks.ClientRenderable = new HookedEntity(
			_Entity->index,
			_Entity,
			pHook,
			pHook->HookFunction((DWORD)&HookedSetupBones, 13),
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			&_playerRecord->Hooks.m_bHookedClientRenderable,
			&_playerRecord->Hooks.ClientRenderable,
			_Entity->GetRefEHandle().ToUnsignedLong()
		);
		_playerRecord->Hooks.m_bHookedClientRenderable = true;
	}

	if (!_playerRecord->Hooks.m_bHookedClientNetworkable)
	{
		VTHook* pHook = new VTHook((PDWORD*)((DWORD)_Entity + 8), hook_types::_BaseNetworkable);
		DWORD HookedSub1 = pHook->HookFunction((DWORD)&HookedPreDataUpdate, m_dwPreDataUpdate);
		DWORD HookedSub2 = pHook->HookFunction((DWORD)&HookedPostDataUpdate, m_dwPostDataUpdate);

		if (!oPreDataUpdate)
			oPreDataUpdate = (PreDataUpdateFn)HookedSub1;
		if (!oPostDataUpdate)
			oPostDataUpdate = (PostDataUpdateFn)HookedSub2;

		_playerRecord->Hooks.ClientNetworkable = new HookedEntity(
			_Entity->index,
			_Entity,
			pHook,
			HookedSub1,
			HookedSub2,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			&_playerRecord->Hooks.m_bHookedClientNetworkable,
			&_playerRecord->Hooks.ClientNetworkable,
			_Entity->GetRefEHandle().ToUnsignedLong()
		);
		_playerRecord->Hooks.m_bHookedClientNetworkable = true;
	}

	if (!_playerRecord->Hooks.m_pBaseAnimStateHook)
	{
		void* _baseAnimState = _Entity->GetBasePlayerAnimState();
		if (_baseAnimState)
		{
			_playerRecord->Hooks.m_pBaseAnimStateHook = new VTHook((PDWORD*)_baseAnimState, hook_types::_BasePlayerAnimState);
			//FIXME Todo: add this into CPlayerRecord
			auto original = _playerRecord->Hooks.m_pBaseAnimStateHook->HookFunction((DWORD)&Hooks::Hooked_DoAnimationEvent, (uint32_t)*StaticOffsets.GetOffsetValueByType< uint8_t* >(_DoAnimationEvent2) / 4);;
			if (!oDoAnimationEvent)
				oDoAnimationEvent = original;

			original = _playerRecord->Hooks.m_pBaseAnimStateHook->HookFunction((DWORD)&Hooks::CSPlayerAnimState_Release, 0);;
			if (!oCSPlayerAnimState_Release)
				oCSPlayerAnimState_Release = original;
		}
		_playerRecord->Hooks.m_pBaseAnimState = _baseAnimState;
	}
}

void __fastcall Hooks::OnAddEntity(void* thisptr, DWORD EDX, IHandleEntity* pEnt, CBaseHandle handle)
{
	CBaseEntity* pBaseEntity = ((IClientUnknown*)pEnt)->GetBaseEntity();
	if (pBaseEntity)
	{
		if (pBaseEntity->index > 0 && pBaseEntity->index <= MAX_PLAYERS)
		{
			HookPlayer(pBaseEntity);
		}
		else //not sure why this was deleted
		{
			auto clclass = pBaseEntity->GetClientClass();
			if (clclass)
			{
				if (clclass->m_ClassID == ClassID::_CInferno || clclass->m_ClassID == ClassID::_CFireSmoke || clclass->m_ClassID == ClassID::_CParticleFire)
				{
					g_Infernos.push_back(pBaseEntity);
				}
				else if ((clclass->m_ClassID == ClassID::_CMolotovGrenade || clclass->m_ClassID == ClassID::_CIncendiaryGrenade))
				{
					VTHook* pHook = new VTHook((PDWORD*)((DWORD)pBaseEntity), hook_types::_MolotovIncendiary);
					DWORD MolotovSimulate = pHook->HookFunction((DWORD)&Molotov_Simulate, m_dwSimulate);

					HookedNonPlayerEntities.emplace(pBaseEntity->index, new HookedEntity(pBaseEntity->index, pBaseEntity, pHook, MolotovSimulate, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, nullptr, nullptr, pBaseEntity->GetRefEHandle().ToUnsignedLong()));
				}
			}
		}
	}

	oOnAddEntity(thisptr, pEnt, handle);
}

void __fastcall Hooks::OnRemoveEntity(void* thisptr, DWORD EDX, IHandleEntity* pEnt, CBaseHandle handle)
{
	CBaseEntity* _Entity = ((IClientUnknown*)pEnt)->GetBaseEntity();
	if (_Entity)
	{
		if (_Entity->index > 0 && _Entity->index <= MAX_PLAYERS)
		{
			CPlayerrecord* _playerRecord = &m_PlayerRecords[_Entity->index];
			if (_playerRecord->Hooks.m_bHookedBaseEntity)
			{
				delete _playerRecord->Hooks.BaseEntity;
				_playerRecord->Hooks.m_bHookedBaseEntity = false;
				_playerRecord->Hooks.BaseEntity = nullptr;
			}
			if (_playerRecord->Hooks.m_bHookedClientRenderable)
			{
				delete _playerRecord->Hooks.ClientRenderable;
				_playerRecord->Hooks.m_bHookedClientRenderable = false;
				_playerRecord->Hooks.ClientRenderable = nullptr;
			}
			if (_playerRecord->Hooks.m_bHookedClientNetworkable)
			{
				delete _playerRecord->Hooks.ClientNetworkable;
				_playerRecord->Hooks.m_bHookedClientNetworkable = false;
				_playerRecord->Hooks.ClientNetworkable = nullptr;
			}
			if (_playerRecord->Hooks.m_pBaseAnimStateHook)
			{
				_playerRecord->Hooks.m_pBaseAnimStateHook->ClearClassBase();
				delete _playerRecord->Hooks.m_pBaseAnimStateHook;
				_playerRecord->Hooks.m_pBaseAnimStateHook = nullptr;
			}
			_playerRecord->Hooks.m_pBaseAnimState = nullptr;

			for (CCSGOPlayerAnimState** i = &_playerRecord->m_pAnimStateServer[0]; i != &_playerRecord->m_pAnimStateServer[MAX_RESOLVE_SIDES]; ++i)
			{
				CCSGOPlayerAnimState* animstate = *i;
				if (animstate)
				{
					delete animstate;
					*i = nullptr;
				}
			}
		}
		else
		{
			g_WeatherController.OnEntityDeleted(_Entity);

			auto clclass = _Entity->GetClientClass();
			if (clclass)
			{
				if (std::find(g_Infernos.begin(), g_Infernos.end(), _Entity) != g_Infernos.end())
				{
					g_Infernos.remove(_Entity);
				}
				else if ((clclass->m_ClassID == ClassID::_CMolotovGrenade || clclass->m_ClassID == ClassID::_CIncendiaryGrenade))
				{
					auto hookedmolotov = HookedNonPlayerEntities.find(_Entity->index);
					if (hookedmolotov != HookedNonPlayerEntities.end())
					{
						delete hookedmolotov->second;
						HookedNonPlayerEntities.erase(hookedmolotov);
					}
				}
			}
		}
	}
	oOnRemoveEntity(thisptr, pEnt, handle);
}