#pragma once
#include "BaseEntity.h"
#include "IClientUnknown.h"

class CEntInfo
{
public:
	IHandleEntity	*m_pEntity;
	int				m_SerialNumber;
	CEntInfo		*m_pPrev;
	CEntInfo		*m_pNext;

	void			ClearLinks();
};

extern CEntInfo *m_EntPtrArray;
typedef CBaseHandle ClientEntityHandle_t;

class CClientEntityList
{
public:
	virtual void Function0() = 0;
	virtual void Function1() = 0;
	virtual void Function2() = 0;

	virtual CBaseEntity*	GetClientEntity_Virtual(int iIndex) = 0;
	virtual CBaseEntity*	GetClientEntityFromHandle_Virtual(DWORD hHandle) = 0;
	virtual int				NumberOfEntities(bool bIncludeNonNetworkable) = 0;
	virtual int				GetHighestEntityIndex() = 0;

	inline CBaseEntity*     EntityExists(CBaseEntity* Entity)
	{
		int _highestindex = GetHighestEntityIndex();
		for (auto info = &m_EntPtrArray[0]; info <= &m_EntPtrArray[_highestindex]; ++info)
		{
			auto ent = info->m_pEntity;
			if (ent && ((IClientUnknown*)ent)->GetBaseEntity() == Entity)
				return Entity;
		}
		return nullptr;
	}

	inline CBaseEntity*     PlayerExists(CBaseEntity* Entity)
	{
		for (auto info = &m_EntPtrArray[1]; info != &m_EntPtrArray[MAX_PLAYERS + 1]; ++info)
		{
			auto ent = info->m_pEntity;
			if (ent && ((IClientUnknown*)ent)->GetBaseEntity() == Entity)
				return Entity;
		}
		return nullptr;
	}

	inline int				GetNumEntitiesStartingFromIndex(int index)
	{
		int highestindex = GetHighestEntityIndex();
		if (index > highestindex)
			return 0;
		return (highestindex - index) + 1;
	}
	
	inline IHandleEntity*	LookupEntityByNetworkIndex(int edictIndex)
	{
		if (edictIndex >= 0)
			return m_EntPtrArray[edictIndex].m_pEntity;

		return NULL;
	}
	inline IHandleEntity* LookupEntity(const CBaseHandle &handle) const
	{
		if (handle.m_Index == INVALID_EHANDLE_INDEX)
			return NULL;

		const CEntInfo *pInfo = &m_EntPtrArray[handle.GetEntryIndex()];
		if (pInfo->m_SerialNumber == handle.GetSerialNumber())
			return (IHandleEntity*)pInfo->m_pEntity;
		else
			return NULL;
	}
	inline IClientUnknown*			GetListedEntity(int entnum) { return (IClientUnknown*)LookupEntityByNetworkIndex(entnum); }
	inline CEntInfo*				GetClientEntityArray() const { return m_EntPtrArray; }
	inline CEntInfo*				GetEntityInfo(int entnum) const { return &m_EntPtrArray[entnum]; }
	inline IClientEntity*			GetClientEntity(int entnum)
	{
		IClientUnknown *pEnt = GetListedEntity(entnum);
		return (pEnt ? pEnt->GetIClientEntity() : 0);
	}
	inline CBaseEntity*				GetBaseEntity(int entnum)
	{
		IClientUnknown *pEnt = GetListedEntity(entnum);
		return (pEnt ? pEnt->GetBaseEntity() : 0);
	}
	inline ICollideable*			GetCollideable(int entnum)
	{
		IClientUnknown *pEnt = GetListedEntity(entnum);
		return (pEnt ? pEnt->GetCollideable() : 0);
	}
	inline IClientUnknown* GetClientUnknownFromHandle(ClientEntityHandle_t hEnt)
	{
		return (IClientUnknown*)LookupEntity(hEnt);
	}
	inline C_BaseEntity* GetBaseEntityFromHandle(ClientEntityHandle_t hEnt)
	{
		IClientUnknown *pEnt = GetClientUnknownFromHandle(hEnt);
		return pEnt ? pEnt->GetBaseEntity() : 0;
	}
	inline IClientNetworkable* GetClientNetworkableFromHandle(ClientEntityHandle_t hEnt)
	{
		IClientUnknown *pEnt = GetClientUnknownFromHandle(hEnt);
		return pEnt ? pEnt->GetClientNetworkable() : 0;
	}
	inline IClientEntity* GetClientEntityFromHandle(ClientEntityHandle_t hEnt)
	{
		IClientUnknown *pEnt = GetClientUnknownFromHandle(hEnt);
		return pEnt ? pEnt->GetIClientEntity() : 0;
	}
	inline IClientRenderable* GetClientRenderableFromHandle(ClientEntityHandle_t hEnt)
	{
		IClientUnknown *pEnt = GetClientUnknownFromHandle(hEnt);
		return pEnt ? pEnt->GetClientRenderable() : 0;
	}
	inline ICollideable* GetCollideableFromHandle(ClientEntityHandle_t hEnt)
	{
		IClientUnknown *pEnt = GetClientUnknownFromHandle(hEnt);
		return pEnt ? pEnt->GetCollideable() : 0;
	}
	inline IClientThinkable* GetClientThinkableFromHandle(ClientEntityHandle_t hEnt)
	{
		IClientUnknown *pEnt = GetClientUnknownFromHandle(hEnt);
		return pEnt ? pEnt->GetClientThinkable() : 0;
	}

	inline IClientNetworkable* GetClientNetworkable(int entnum)
	{
		IClientUnknown *pEnt = GetListedEntity(entnum);
		return (pEnt ? pEnt->GetClientNetworkable() : 0);
		//return m_EntityCacheInfo[entnum].m_pNetworkable;
	}
};