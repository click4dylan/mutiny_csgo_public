#pragma once
#include "misc.h"
#include "BaseEntity.h"
#include "model_types.h"
#include "CSGO_HX.h"

enum TraceType_t
{
	TRACE_EVERYTHING = 0,
	TRACE_WORLD_ONLY, // NOTE: This does *not* test static props!!!
	TRACE_ENTITIES_ONLY, // NOTE: This version will *not* test static props
	TRACE_EVERYTHING_FILTER_PROPS, // NOTE: This version will pass the IHandleEntity for props through the filter, unlike all other filters
};

struct Ray_t
{
	Ray_t() {}
	::Ray_t(Vector& start, Vector& end)
	{
		Init(start, end);
	}

	VectorAligned m_Start;
	VectorAligned m_Delta;
	VectorAligned m_StartOffset;
	VectorAligned m_Extents;
	const matrix3x4_t* m_pWorldAxisTransform;
	bool m_IsRay;
	bool m_IsSwept;

	void Init(Vector vecStart, Vector vecEnd)
	{
		m_Delta   = VectorAligned(vecEnd - vecStart);
		m_IsSwept = (m_Delta.LengthSqr() != 0);
		m_Extents.Zero();
		m_pWorldAxisTransform = NULL;
		m_IsRay				  = true;
		m_StartOffset.Zero();
		m_Start = vecStart;
	}

	void Init(Vector const& start, Vector const& end, Vector const& mins, Vector const& maxs)
	{
		m_pWorldAxisTransform = NULL;

		VectorSubtract(end, start, m_Delta);

		m_IsSwept = (m_Delta.LengthSqr() != 0);

		VectorSubtract(maxs, mins, m_Extents);
		m_Extents *= 0.5f;
		m_IsRay = (m_Extents.LengthSqr() < 1e-6);

		// Offset m_Start to be in the center of the box...
		VectorAdd(mins, maxs, m_StartOffset);
		m_StartOffset *= 0.5f;
		VectorAdd(start, m_StartOffset, m_Start);
		m_StartOffset *= -1.0f;
	}
};

bool StandardFilterRules(IHandleEntity* pHandleEntity, int fContentsMask, int collisionGroup);

bool PassServerEntityFilter(const IHandleEntity* pTouch, const IHandleEntity* pPass);

bool BaseShouldHitEntity(IHandleEntity* pSkip, IHandleEntity* pHandleEntity, int m_collisionGroup, int contentsMask);

class ITraceFilter
{
public:
	virtual bool ShouldHitEntity(IHandleEntity* pEntityHandle, int contentsMask)
	{
		return BaseShouldHitEntity(pSkip, pEntityHandle, m_icollisionGroup, contentsMask);
	}
	virtual TraceType_t GetTraceType()
	{
		return TRACE_EVERYTHING;
	}
	IHandleEntity* pSkip;
	int m_icollisionGroup;
};

class CTraceFilter : public ITraceFilter
{
public:
	virtual TraceType_t GetTraceType() const
	{
		return TRACE_EVERYTHING;
	}
};

class CTraceFilterEntitiesOnly : public ITraceFilter
{
public:
	virtual TraceType_t GetTraceType() const
	{
		return TRACE_ENTITIES_ONLY;
	}
};

class CTraceFilterWorldOnly : public ITraceFilter
{
public:
	bool ShouldHitEntity(IHandleEntity* pServerEntity, int contentsMask)
	{
		return false;
	}
	virtual TraceType_t GetTraceType() const
	{
		return TRACE_WORLD_ONLY;
	}
};

class CTraceFilterWorldAndPropsOnly : public ITraceFilter
{
public:
	bool ShouldHitEntity(IHandleEntity* pServerEntity, int contentsMask)
	{
		return false;
	}
	virtual TraceType_t GetTraceType() const
	{
		return TRACE_EVERYTHING;
	}
};

class CTraceFilterTargetSpecificEntity : public ITraceFilter
{
public:
	bool ShouldHitEntity(IHandleEntity* pServerEntity, int contentsMask)
	{
		if ((CBaseEntity*)pServerEntity == pTarget)
			return true;
		return false;
	}
	virtual TraceType_t GetTraceType() const
	{
		return TRACE_ENTITIES_ONLY;
	}
	CBaseEntity* pTarget;
};

class CTraceFilterHitAll : public CTraceFilter
{
public:
	virtual bool ShouldHitEntity(IHandleEntity* pServerEntity, int contentsMask)
	{
		return true;
	}
};

class CTraceFilterNoPlayers : public CTraceFilter
{
public:
	virtual bool ShouldHitEntity(IHandleEntity* pEntityHandle, int contentsMask);
};

class CTraceFilterInterited_DisablePlayers : public CTraceFilter
{
public:
	::CTraceFilterInterited_DisablePlayers(ITraceFilter& pInheritedFilter)
	{
		m_pInheritedTraceFilter = &pInheritedFilter;
		m_icollisionGroup = pInheritedFilter.m_icollisionGroup;
	}
	::CTraceFilterInterited_DisablePlayers(ITraceFilter& pInheritedFilter, int collisionGroup)
	{
		m_pInheritedTraceFilter = &pInheritedFilter;
		m_icollisionGroup = collisionGroup;
	}
	virtual bool ShouldHitEntity(IHandleEntity* pEntityHandle, int contentsMask);
	virtual TraceType_t GetTraceType() const
	{
		return m_pInheritedTraceFilter->GetTraceType();
	}
	ITraceFilter* m_pInheritedTraceFilter;
};

class CTraceFilterPlayersOnlyNoWorld : public CTraceFilter
{
public:
	virtual bool ShouldHitEntity(IHandleEntity* pEntityHandle, int contentsMask);
	virtual TraceType_t GetTraceType()
	{
		return TRACE_ENTITIES_ONLY;
	}
	bool AllowTeammates;
};
#if 0 //fixme eventually
class CTraceFilterEntity : public CTraceFilter
{

public:
	CTraceFilterEntity(IHandleEntity *pEntity, int nCollisionGroup)
		: CTraceFilter(pEntity, nCollisionGroup)
	{
		m_pRootParent = pEntity->GetRootMoveParent();
		m_pInflictedEntity = pEntity;
		m_checkHash = g_EntityCollisionHash->IsObjectInHash(pEntity);
	}

	bool ShouldHitEntity(IHandleEntity *pHandleEntity, int contentsMask)
	{
		CBaseEntity *pEntity = (CBaseEntity*)pHandleEntity;
		if (!pEntity)
			return false;

		// Check parents against each other
		// NOTE: Don't let siblings/parents collide.
		if (UTIL_EntityHasMatchingRootParent(m_pRootParent, pEntity))
			return false;

		if (m_checkHash)
		{
			if (g_EntityCollisionHash->IsObjectPairInHash(m_pInflictedEntity, pEntity))
				return false;
		}

		return BaseShouldHitEntity(pHandleEntity, contentsMask, m_icollisionGroup, contents);
	}

private:

	IHandleEntity *m_pRootParent;
	IHandleEntity *m_pInflictedEntity;
	bool		m_checkHash;
};

class CTraceFilterEntityIgnoreOther : public CTraceFilterEntity
{
	DECLARE_CLASS(CTraceFilterEntityIgnoreOther, CTraceFilterEntity);
public:
	CTraceFilterEntityIgnoreOther(CBaseEntity *pEntity, const IHandleEntity *pIgnore, int nCollisionGroup) :
		CTraceFilterEntity(pEntity, nCollisionGroup), m_pIgnoreOther(pIgnore)
	{
	}

	bool ShouldHitEntity(IHandleEntity *pHandleEntity, int contentsMask)
	{
		if (pHandleEntity == m_pIgnoreOther)
			return false;

		return BaseClass::ShouldHitEntity(pHandleEntity, contentsMask);
	}

private:
	const IHandleEntity *m_pIgnoreOther;
};

#endif

class CTraceFilterPlayersOnly : public CTraceFilter
{
public:
	virtual bool ShouldHitEntity(IHandleEntity* pEntityHandle, int contentsMask);
	bool AllowTeammates;
};

class CTraceFilterIgnoreWorld : public CTraceFilter
{
public:
	virtual bool ShouldHitEntity(IHandleEntity* pEntityHandle, int contentsMask)
	{
		if (!pEntityHandle)
			return false;
		return true;
	}
	virtual TraceType_t GetTraceType()
	{
		return TRACE_ENTITIES_ONLY;
	}
};

typedef bool (*ShouldHitFunc_t)(IHandleEntity* pHandleEntity, int contentsMask);

class CTraceFilterSimple : public CTraceFilter
{
public:
	// It does have a base, but we'll never network anything below here..

	CTraceFilterSimple()
	{
		m_pPassEnt		 = nullptr;
		m_collisionGroup = 0;
	}

	CTraceFilterSimple(IHandleEntity* passentity, int collisionGroup);

	virtual bool ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask)
	{
		return BaseShouldHitEntity((IHandleEntity*)m_pPassEnt, pHandleEntity, m_collisionGroup, contentsMask);
	}
	virtual void SetPassEntity(IHandleEntity* pPassEntity) { m_pPassEnt = pPassEntity; }
	virtual void SetCollisionGroup(int iCollisionGroup) { m_collisionGroup = iCollisionGroup; }

	const IHandleEntity* GetPassEntity(void) { return m_pPassEnt; }

private:
	const IHandleEntity* m_pPassEnt;
	int m_collisionGroup;
	//ShouldHitFunc_t m_pExtraShouldHitCheckFunction;
};

class CTraceFilterSkipTwoEntities_CSGO
{
public:
	virtual bool ShouldHitEntity(IHandleEntity* pEntityHandle, int contentsMask)
	{
		if (!PassServerEntityFilter(pEntityHandle, m_pPassEnt2))
			return false;
		return BaseShouldHitEntity(pEntityHandle, (IHandleEntity*)m_pPassEnt, m_collisionGroup, contentsMask);
	};
	virtual TraceType_t GetTraceType() { return TRACE_EVERYTHING; };
	virtual void SetPassEntity(const IHandleEntity* pPassEntity) { m_pPassEnt = pPassEntity; };
	virtual void SetCollisionGroup(int group) { m_collisionGroup = group; }
	virtual void SetPassEntity2(const IHandleEntity* pPassEntity2) { m_pPassEnt2 = pPassEntity2; }
	const IHandleEntity* m_pPassEnt;
	int m_collisionGroup;
	const IHandleEntity* m_pPassEnt2;
};

class CTraceFilterSkipTwoEntities : public CTraceFilter //Simple.... dylan changed but fucking compiler won't compile. complaining about no default constructor in CTraceFilterSimple..
{
public:
	CTraceFilterSkipTwoEntities(IHandleEntity* pPassEnt1, IHandleEntity* pPassEnt2, int collisionGroup);

	virtual bool ShouldHitEntity(IHandleEntity* pEntityHandle, int contentsMask)
	{
		if (!PassServerEntityFilter(pEntityHandle, m_pPassEnt2))
			return false;
		return BaseShouldHitEntity(pEntityHandle, pSkip, m_icollisionGroup, contentsMask);
	}

	virtual void SetPassEntity2(const IHandleEntity* pPassEntity2) { m_pPassEnt2 = pPassEntity2; }

public:
	const IHandleEntity* m_pPassEnt2;
};

class CTraceFilterSkipThreeEntities : public CTraceFilter //Simple.... dylan changed but fucking compiler won't compile. complaining about no default constructor in CTraceFilterSimple..
{
public:
	CTraceFilterSkipThreeEntities(IHandleEntity* pPassEnt1, IHandleEntity* pPassEnt2, IHandleEntity* pPassEnt3, int collisionGroup);

	virtual bool ShouldHitEntity(IHandleEntity* pEntityHandle, int contentsMask)
	{
		if (!PassServerEntityFilter(pEntityHandle, m_pPassEnt2))
			return false;
		if (!PassServerEntityFilter(pEntityHandle, m_pPassEnt3))
			return false;
		if (!BaseShouldHitEntity(pEntityHandle, pSkip, m_icollisionGroup, contentsMask))
			return false;
		return BaseShouldHitEntity(pEntityHandle, (IHandleEntity*)m_pPassEnt3, m_icollisionGroup, contentsMask);
	}

	virtual void SetPassEntity2(const IHandleEntity* pPassEntity2) { m_pPassEnt2 = pPassEntity2; }
	virtual void SetPassEntity3(const IHandleEntity* pPassEntity3) { m_pPassEnt3 = pPassEntity3; }

public:
	const IHandleEntity* m_pPassEnt2;
	const IHandleEntity* m_pPassEnt3;
};

class CTraceFilterForPlayerHeadCollision
{
public:
	virtual bool ShouldHitEntity(IHandleEntity* pEntityHandle, int contentsMask);
	virtual TraceType_t GetTraceType() { return TRACE_EVERYTHING; }; 
	virtual void SetPassEntity(const IHandleEntity* pPassEntity) { m_pSkip = pPassEntity; };
	virtual void SetCollisionGroup(int group) { m_iCollisionGroup = group; }
	const IHandleEntity* m_pPassEnt1;
	int m_iCollisionGroup;
	int m_Unknown;
	const IHandleEntity* m_pSkip;
};

//-----------------------------------------------------------------------------
// Like UTIL_WaterLevel, but *way* less expensive.
// I didn't replace UTIL_WaterLevel everywhere to avoid breaking anything.
//-----------------------------------------------------------------------------

// im sorry shark, fixing what is missing, trying to figure out what is happenign with trace ray.
#define   DISPSURF_FLAG_SURFACE           (1<<0)
#define   DISPSURF_FLAG_WALKABLE          (1<<1)
#define   DISPSURF_FLAG_BUILDABLE         (1<<2)
#define   DISPSURF_FLAG_SURFPROP1         (1<<3)
#define   DISPSURF_FLAG_SURFPROP2         (1<<4)

class CWaterTraceFilter : public CTraceFilter
{
public:
	virtual bool ShouldHitEntity(IHandleEntity *pHandleEntity, int contentsMask);
};

class CBaseTrace
{
public:
	bool IsDispSurface(void) { return ((dispFlags & DISPSURF_FLAG_SURFACE) != 0); }
	bool IsDispSurfaceWalkable(void) { return ((dispFlags & DISPSURF_FLAG_WALKABLE) != 0); }
	bool IsDispSurfaceBuildable(void) { return ((dispFlags & DISPSURF_FLAG_BUILDABLE) != 0); }
	bool IsDispSurfaceProp1(void) { return ((dispFlags & DISPSURF_FLAG_SURFPROP1) != 0); }
	bool IsDispSurfaceProp2(void) { return ((dispFlags & DISPSURF_FLAG_SURFPROP2) != 0); }

public:
	Vector startpos;
	Vector endpos;
	cplane_t plane;

	float fraction; //40

	int contents; //44
	unsigned short dispFlags; //48

	bool allsolid; //50
	bool startsolid; //51

	CBaseTrace() {}
};

struct csurface_t
{
	const char* name; //56
	short surfaceProps; //60
	unsigned short flags; //62
};

class CGameTrace : public CBaseTrace
{
public:
	bool DidHitWorld() const;
	bool DidHitNonWorldEntity() const { return m_pEnt != nullptr && !DidHitWorld(); };
	int GetEntityIndex() const;
	bool DidHit() const { return fraction < 1.0f || allsolid || startsolid; };
	bool IsVisible() const { return fraction > 0.97f; };

public:
	float fractionleftsolid; //52
	csurface_t surface; //56
	int hitgroup; //64
	short physicsbone;
	unsigned short worldSurfaceIndex;
	CBaseEntity* m_pEnt;
	int hitbox;

	CGameTrace() { m_pEnt = nullptr; }

private:
	CGameTrace(const CGameTrace& other) :
										fractionleftsolid(other.fractionleftsolid),
										surface(other.surface),
										hitgroup(other.hitgroup),
										physicsbone(other.physicsbone),
										worldSurfaceIndex(other.worldSurfaceIndex),
										m_pEnt(other.m_pEnt),
										hitbox(other.hitbox)
	{
		startpos = other.startpos;
		endpos = other.endpos;
		plane = other.plane;
		fraction = other.fraction;
		contents = other.contents;
		dispFlags = other.dispFlags;
		allsolid = other.allsolid;
		startsolid = other.startsolid;
	}
};

typedef CGameTrace trace_t;

BOOLEAN GameRulesShouldCollide(int collisionGroup0, int collisionGroup1);

bool TraceIsOnGroundOrPlayer(trace_t* tr);