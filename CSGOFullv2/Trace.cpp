#include "precompiled.h"
#include "IModelInfoClient.h"
#include "Trace.h"
#include "Interfaces.h"
#include "VTHook.h"
#include "C_CSGameRules.h"
#include "Targetting.h"
#include "IClientRenderable.h"

bool __fastcall PlayerFilterRules(IHandleEntity* pPass, CBaseEntity* pEntityHandle)
{
	return StaticOffsets.GetOffsetValueByType< bool(__fastcall*)(IHandleEntity*, CBaseEntity*) >(_PlayerFilterRules)(pPass, pEntityHandle);
}

int CGameTrace::GetEntityIndex() const
{
	if (m_pEnt)
		return m_pEnt->entindex();
	else
		return -1;
};

bool CTraceFilterInterited_DisablePlayers::ShouldHitEntity(IHandleEntity* pEntityHandle, int contentsMask)
{
	CBaseEntity* pEnt = EntityFromEntityHandle(pEntityHandle);
	if (pEnt && pEnt->IsPlayer())
		return false;

	return m_pInheritedTraceFilter->ShouldHitEntity(pEntityHandle, contentsMask);
};

bool CTraceFilterPlayersOnlyNoWorld::ShouldHitEntity(IHandleEntity* pEntityHandle, int contentsMask)
{
	if (!pEntityHandle || pEntityHandle == pSkip)
		return false;

	if (((CBaseEntity*)pEntityHandle)->IsPlayer() && (AllowTeammates || MTargetting.IsPlayerAValidTarget((CBaseEntity*)pEntityHandle)))
		return true;
	return false;
}

bool CTraceFilterPlayersOnly::ShouldHitEntity(IHandleEntity* pEntityHandle, int contentsMask)
{
	if (!pEntityHandle)
		return true;
	if (pEntityHandle != pSkip && (((CBaseEntity*)pEntityHandle)->IsPlayer() && (AllowTeammates || MTargetting.IsPlayerAValidTarget((CBaseEntity*)pEntityHandle))))
		return true;
	return false;
}

bool CTraceFilterNoPlayers::ShouldHitEntity(IHandleEntity* pEntityHandle, int contentsMask)
{
	if (!pEntityHandle)
		return true;
	if (((CBaseEntity*)pEntityHandle)->IsPlayer())
		return false;
	if (pSkip && pEntityHandle == pSkip)
		return false;
	return true;
}

bool CGameTrace::DidHitWorld() const
{
	return m_pEnt == Interfaces::ClientEntList->GetBaseEntity(0);
}

CTraceFilterSimple::CTraceFilterSimple(IHandleEntity* passentity, int collisionGroup)
{
	m_pPassEnt		 = passentity;
	m_collisionGroup = collisionGroup;
}

CTraceFilterSkipTwoEntities::CTraceFilterSkipTwoEntities(IHandleEntity* pPassEnt1, IHandleEntity* pPassEnt2, int collisionGroup)
{
	pSkip			  = pPassEnt1;
	m_pPassEnt2		  = pPassEnt2;
	m_icollisionGroup = collisionGroup;
}

CTraceFilterSkipThreeEntities::CTraceFilterSkipThreeEntities(IHandleEntity* pPassEnt1, IHandleEntity* pPassEnt2, IHandleEntity* pPassEnt3, int collisionGroup)
{
	pSkip			  = pPassEnt1;
	m_pPassEnt2		  = pPassEnt2;
	m_pPassEnt3		  = pPassEnt3;
	m_icollisionGroup = collisionGroup;
}

bool __fastcall StandardFilterRules(IHandleEntity* pHandleEntity, int fContentsMask)
{
	CBaseEntity* pCollide = EntityFromEntityHandle(pHandleEntity); //(*(CBaseEntity*(**)(IHandleEntity*))(*(DWORD *)pHandleEntity + 0x1C))(pHandleEntity);

	// Static prop case...
	if (!pCollide)
		return true;

	SolidType_t solid = pCollide->GetSolid();

	const model_t* pModel = pCollide->GetModel();

	if ((Interfaces::ModelInfoClient->GetModelType(pModel) != mod_brush) || (solid != SOLID_BSP && solid != SOLID_VPHYSICS))
	{
		if ((fContentsMask & CONTENTS_MONSTER) == 0)
			return false;
	}
	// This code is used to cull out tests against see-thru entities
	if (!(fContentsMask & CONTENTS_WINDOW))
	{
		//if (pCollide->IsTransparent())
		//return false;

		//easy to read format
		//register DWORD EAX = *(DWORD*)((DWORD)pCollide + 4);
		//register DWORD ECX = ((DWORD)pCollide + 4);
		//unsigned short v10 = *((unsigned short*(__thiscall *)(DWORD)) *(DWORD*)(EAX + 0x1C))(ECX);

		//unsigned short v10 = *((unsigned short*(__thiscall *)(DWORD)) *(DWORD*)(*(DWORD*)((DWORD)pCollide + 4) + 0x1C))(((DWORD)pCollide + 4));
		ClientRenderHandle_t renderhandle = pCollide->RenderHandle();

		if (renderhandle == INVALID_CLIENT_RENDER_HANDLE)
		{
			CBaseEntity* newent = ((CBaseEntity * (__thiscall*)(CBaseEntity*)) AdrOf_StandardFilterRulesCallOne)(pCollide);
			if (newent)
			{
				//easy to read format
				//ECX = *(DWORD*)(v9 + 4);
				//EAX = *(DWORD*)ECX;
				//v10 = *((unsigned short*(__thiscall *)(DWORD)) *(DWORD*)(EAX + 0x1C))(ECX);

				//DWORD ECX = *(DWORD*)(v9 + 4);
				//v10 = *((unsigned short*(__thiscall *)(DWORD)) *(DWORD*)(*(DWORD*)ECX + 0x1C))(ECX);
				renderhandle = newent->RenderHandle();
			}
		}
		//easy to read format
		//EAX = *(DWORD*)AdrOf_StandardFilterRulesMemoryOne;
		//BOOL transparent = ((bool(__thiscall *)(DWORD, DWORD)) *(DWORD*)(EAX + 0x80)) (AdrOf_StandardFilterRulesMemoryOne, (DWORD)v10);
		//BOOLEAN transparent = ((BOOLEAN(__thiscall *)(DWORD, DWORD)) *(DWORD*)(*(DWORD*)g_pClientLeafSystem + 0x80)) (g_pClientLeafSystem, (DWORD)renderhandle);
		RenderableTranslucencyType_t transparent = GetTranslucencyType(renderhandle);
		if (transparent == RENDERABLE_IS_TRANSLUCENT)
			return false;
	}

	// FIXME: this is to skip BSP models that are entities that can be
	// potentially moved/deleted, similar to a monster but doors don't seem to
	// be flagged as monsters
	// FIXME: the FL_WORLDBRUSH looked promising, but it needs to be set on
	// everything that's actually a worldbrush and it currently isn't
	if (!(fContentsMask & CONTENTS_MOVEABLE) && (pCollide->GetMoveType() == MOVETYPE_PUSH)) // !(touch->flags & FL_WORLDBRUSH) )
		return false;

	return true;
}

bool PassServerEntityFilter(const IHandleEntity* pTouch, const IHandleEntity* pPass)
{
	if (!pPass)
		return true;

	if (pTouch == pPass)
		return false;

	CBaseEntity* pEntTouch = EntityFromEntityHandle((IHandleEntity*)pTouch);
	CBaseEntity* pEntPass  = EntityFromEntityHandle((IHandleEntity*)pPass);
	if (!pEntTouch || !pEntPass)
		return true;

	// don't clip against own missiles
	if (pEntTouch->GetOwner() == pEntPass)
		return false;

	// don't clip against owner
	if (pEntPass->GetOwner() == pEntTouch)
		return false;

	return true;
}

bool BaseShouldHitEntity(IHandleEntity* pSkip, IHandleEntity* pHandleEntity, int m_collisionGroup, int contentsMask)
{
	if (!StandardFilterRules(pHandleEntity, contentsMask))
		return false;

	if (pSkip)
	{
		if (!PassServerEntityFilter(pHandleEntity, pSkip))
		{
			return false;
		}
	}

	// Don't test if the game code tells us we should ignore this collision...
	CBaseEntity* pEntity = EntityFromEntityHandle(pHandleEntity);
	if (!pEntity)
		return false;
	if (!pEntity->ShouldCollide(m_collisionGroup, contentsMask))
		return false;
	if (/*pEntity && */ !GameRulesShouldCollide(m_collisionGroup, pEntity->GetCollisionGroup()))
		return false;

	return true;
}

BOOLEAN GameRulesShouldCollide(int collisionGroup0, int collisionGroup1)
{
#if 1
	__asm {
		push collisionGroup1
		push collisionGroup0
		mov ecx, g_pGameRules
		mov ecx, [ecx]
		mov eax, [ecx]
		mov eax, [eax + 0x70]
		call eax
	}
#endif

	//return ((BOOLEAN(__stdcall*)(int, int))(ReadInt(ReadInt(ReadInt(g_pGameRules)) + GameRulesShouldCollideOffset)))(collisionGroup0, collisionGroup1);
}

bool CTraceFilterForPlayerHeadCollision::ShouldHitEntity(IHandleEntity* pEntityHandle, int contentsMask)
{
	if (contentsMask & 0x1800)
	{
		if (m_pSkip)
		{
			if (m_iCollisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT)
			{
				CBaseEntity* ent = EntityFromEntityHandle(pEntityHandle);
				if (ent)
				{
					if (ent->IsPlayer() && PlayerFilterRules((IHandleEntity*)m_pSkip, ent))
						contentsMask &= 0xFFFFE7FF;
				}
			}
		}
	}
	return BaseShouldHitEntity((IHandleEntity*)m_pSkip, pEntityHandle, m_iCollisionGroup, contentsMask);
}

bool CWaterTraceFilter::ShouldHitEntity(IHandleEntity *pHandleEntity, int contentsMask)
{
	CBaseEntity *pCollide = EntityFromEntityHandle(pHandleEntity);

	// Static prop case...
	if (!pCollide)
		return false;

	// Only impact water stuff...
	if (pCollide->GetSolidFlags() & FSOLID_VOLUME_CONTENTS)
		return true;

	return false;
}

bool TraceIsOnGroundOrPlayer(trace_t* tr)
{
	bool v2 = false;
	if ((tr->fraction < 1.f || tr->allsolid || tr->startsolid) && tr->m_pEnt)
	{
		// calls CBaseEntity + 0x260
		if (tr->m_pEnt->IsPlayer())
			v2 = true;
		else
			v2 = (tr->plane.normal.z >= 0.7f);
	}
	else
	{
		v2 = false;
	}

	return v2;
}