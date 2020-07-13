#pragma once
#include "misc.h"
#include <string>
#include "NetVarManager.h"
#include "utlvectorsimple.h"
#include <vector>
#include "bone_accessor.h"
#include "Offsets.h"
#include "threadtools.h"
#include <unordered_map>
#include "ClientSideAnimationList.h"
#include "ClassIDS.h"
#include "Intersection.h"
#include "NetworkedVariables.h"
#include "CBaseHandle.h"
#include "IClientRenderable.h"
#include "UtlVector.hpp"
#include "UtlSortVector.h"
#include "utlsymbol.h"

class CGameTrace;
class C_BaseViewModel;
typedef CGameTrace trace_t;
class VarMapping_t;
class CParticleProperty;


#pragma pack(push, 1)
class CAttachmentData
{
public:
	matrix3x4_t m_AttachmentToWorld;
	QAngle m_angRotation; //0x30
	Vector m_vOriginVelocity; //3C
	int m_nLastFramecount;// : 31; //48
	unsigned char m_bAnglesComputed;// : 1; //4C
	char pad[3];
};
#pragma pack(pop)

enum eEntityType
{
	none = -1,
	player,
	weapon,
	item,
	projectile,
	c4,
	plantedc4,
	chicken
};

class CBaseHandle;
class INetChannelInfo;
class ICollideable;
class CBaseAnimating;
class CBaseCombatWeapon;
class C_AnimationLayer;
class C_AnimationLayerFromPacket;
struct StoredNetvars;
struct C_CSGOPlayerAnimState;
class CCSGOPlayerAnimState;
struct studiohdr_t;
class CStudioHdr;
enum PlayerAnimEvent_t;
class IPhysicsObject;
class CSphere;
class COBB;
class IClientVehicle;
class CPlayerrecord;
class CTickrecord;
struct datamap_t;
class CBaseEntity;
enum ParticleAttachment_t;


struct ParticleControlPoint_t
{
	ParticleControlPoint_t()
	{
		iControlPoint = 0;
		iAttachType = (ParticleAttachment_t)1/* PATTACH_ABSORIGIN_FOLLOW*/;
		iAttachmentPoint = 0;
		vecOriginOffset = { 0.0f,0.0f,0.0f };
	}

	int								iControlPoint;
	ParticleAttachment_t			iAttachType;
	int								iAttachmentPoint;
	Vector							vecOriginOffset;
	EHANDLE							hEntity;
};

class CParticleSimulateIterator;
struct Particle;
class CParticleRenderIterator;
class CParticleSubTexture;
class CParticleSubTextureGroup;
class CEffectMaterial;


struct Particle
{
	Particle *m_pPrev, *m_pNext;

	// Which sub texture this particle uses (so we can get at the tcoord mins and maxs).
	CParticleSubTexture *m_pSubTexture;

	// If m_Pos isn't used to store the world position, then implement IParticleEffect::GetParticlePosition()
	Vector m_Pos;			// Position of the particle in world space
};

#if 0
class IParticleEffect
{
public:

	virtual			~IParticleEffect() {}
	virtual void	Update(float fTimeDelta) {}
	virtual void	StartRender(VMatrix &effectMatrix) {}
	virtual bool	ShouldSimulate() const = 0;
	virtual void	SetShouldSimulate(bool bSim) = 0;
	virtual void	SimulateParticles(CParticleSimulateIterator *pIterator) = 0;
	virtual void	RenderParticles(CParticleRenderIterator *pIterator) = 0;
	virtual void	NotifyRemove() {}
	virtual void	NotifyDestroyParticle(Particle* pParticle) {}
	virtual const Vector &GetSortOrigin() = 0;
	virtual const Vector *GetParticlePosition(Particle *pParticle) { return &pParticle->m_Pos; }
	virtual const char *GetEffectName() { return "???"; }

};

class CParticleEffectBinding : public CDefaultClientRenderable
{
	friend class CParticleMgr;
	friend class CParticleSimulateIterator;
	friend class CNewParticleEffect;

public:
	virtual const Vector&			GetRenderOrigin(void);
	virtual const QAngle&			GetRenderAngles(void);
	virtual const matrix3x4_t &		RenderableToWorldTransform();
	virtual void					GetRenderBounds(Vector& mins, Vector& maxs);
	virtual bool					ShouldDraw(void);
	virtual int						DrawModel(int flags, const RenderableInstance_t &instance);

	//private:

	VMatrix m_LocalSpaceTransform;
	bool m_bLocalSpaceTransformIdentity;	// If this is true, then m_LocalSpaceTransform is assumed to be identity.

	// Bounding box. Stored in WORLD space.
	Vector							m_Min;
	Vector							m_Max;

	// paramter copies to detect changes
	Vector							m_LastMin;
	Vector							m_LastMax;

	// The particle cull size
	float							m_flParticleCullRadius;

	// Number of active particles.
	unsigned short					m_nActiveParticles;

	// See CParticleMgr::m_FrameCode.
	unsigned short					m_FrameCode;

	// For CParticleMgr's list index.
	unsigned short					m_ListIndex;

	IParticleEffect					*m_pSim;
	CParticleMgr					*m_pParticleMgr;

	// Combination of the CParticleEffectBinding::FLAGS_ flags.
	int								m_Flags;

	// Materials this effect is using.
	enum { EFFECT_MATERIAL_HASH_SIZE = 8 };
	CEffectMaterial *m_EffectMaterialHash[EFFECT_MATERIAL_HASH_SIZE];

	// For faster iteration.
	CUtlLinkedList<CEffectMaterial*, unsigned short> m_Materials;

	// auto updates the bbox after N frames
	unsigned short					m_UpdateBBoxCounter;

};

class CParticleEffect : public IParticleEffect
{
	DECLARE_CLASS_NOBASE(CParticleEffect);

	friend class CRefCountAccessor;

public:

	virtual void				SetParticleCullRadius(float radius);
	virtual void				NotifyRemove(void);
	virtual const Vector &		GetSortOrigin();
	virtual void				NotifyDestroyParticle(Particle* pParticle);
	virtual void				Update(float flTimeDelta);

	virtual bool				ShouldSimulate() const { return m_bSimulate; }
	virtual void				SetShouldSimulate(bool bSim) { m_bSimulate = bSim; }

protected:

	virtual						~CParticleEffect();

	enum
	{
		FLAG_ALLOCATED = (1 << 1),	// Most of the CParticleEffects are dynamically allocated but
								// some are member variables of a class. If they're member variables.
								FLAG_DONT_REMOVE = (1 << 2),
	};

	// Used to track down bugs.
	char const					*m_pDebugName;

	CParticleEffectBinding		m_ParticleEffect;
	Vector						m_vSortOrigin;

	int							m_Flags;		// Combination of CParticleEffect::FLAG_

	bool						m_bSimulate;
	int							m_nToolParticleEffectId;

private:

	int							m_RefCount;
};

class CNewParticleEffect : public IParticleEffect, public CParticleCollection, public CDefaultClientRenderable
{
public:
	DECLARE_CLASS_NOBASE(CNewParticleEffect);
	//DECLARE_REFERENCED_CLASS(CNewParticleEffect);

public:
	friend class CRefCountAccessor;
	friend class CParticleSystemQuery;

	// list management
	CNewParticleEffect *m_pNext;
	CNewParticleEffect *m_pPrev;

	virtual int GetRenderFlags(void);
	virtual bool	ShouldDrawForSplitScreenUser(int nSlot);
	virtual int DrawModel(int flags, const RenderableInstance_t &instance);
	virtual void	SimulateParticles(CParticleSimulateIterator *pIterator)
	{
	}
	virtual void	RenderParticles(CParticleRenderIterator *pIterator)
	{
	}
	virtual void				SetParticleCullRadius(float radius);
	virtual void				NotifyRemove(void);
	virtual const Vector &		GetSortOrigin(void);

	//	virtual void				NotifyDestroyParticle( Particle* pParticle );
	virtual void				Update(float flTimeDelta);
	virtual bool				ShouldSimulate() const { return m_bSimulate; }
	virtual void				SetShouldSimulate(bool bSim) { m_bSimulate = bSim; }

	virtual ~CNewParticleEffect();

protected:

	// Used to track down bugs.
	const char	*m_pDebugName;

	bool		m_bDontRemove : 1;
	bool		m_bRemove : 1;
	bool		m_bDrawn : 1;
	bool		m_bNeedsBBoxUpdate : 1;
	bool		m_bIsFirstFrame : 1;
	bool		m_bAutoUpdateBBox : 1;
	bool		m_bAllocated : 1;
	bool		m_bSimulate : 1;
	bool		m_bRecord : 1;
	bool		m_bShouldPerformCullCheck : 1;

	// if a particle system is created through the non-aggregation entry point, we can't aggregate into it
	bool        m_bDisableAggregation : 1;



	int			m_nToolParticleEffectId;
	Vector		m_vSortOrigin;
	EHANDLE		m_hOwner;
	EHANDLE     m_hControlPointOwners[MAX_PARTICLE_CONTROL_POINTS];

	// holds the min/max bounds used to manage this thing in the client leaf system
	Vector		m_LastMin;
	Vector		m_LastMax;

	int			m_nSplitScreenUser; // -1 means don't care
	Vector m_vecAggregationCenter;							// origin for aggregation if aggregation enabled

private:


	int			m_RefCount;		// When this goes to zero and the effect has no more active
								// particles, (and it's dynamically allocated), it will delete itself.

	matrix3x4a_t m_DrawModelMatrix;

};
#endif

#define MAX_PARTICLE_CONTROL_POINTS 64

class CNewParticleEffect;

class CParticleCollection
{
public:
	CNewParticleEffect* ToNewParticleEffect() { return (CNewParticleEffect*)((uintptr_t)this - 16); }
	CParticleCollection* GetChildren() { return (CParticleCollection*)*(uintptr_t*)((uintptr_t)this + 0x8C); }
	CParticleCollection* GetNext() { return (CParticleCollection*)*(uintptr_t*)((uintptr_t)this + 0x70); }
	Vector& GetPosition(int nWhichPoint) { return *(Vector*)(*(uintptr_t *)((uintptr_t)this + 0x68) + 96 * nWhichPoint); } //Located in CParticleCollection::SetControlPoint
	Vector& GetOrigin() { return *(Vector*)((uintptr_t)this + 0xB0); }
	int GetNumControlPointsAllocated() { return *(int*)((uintptr_t)this + 0x64); }
	int GetHighestControlPoint() { return *(int*)((uintptr_t)this + 0xBC); }
	void RenderChildren(const ColorRGBA& color);
};

class CNewParticleEffect
{
public:
	CParticleCollection* ToParticleCollection() { return (CParticleCollection*)((uintptr_t)this + 16); }
	EHANDLE& GetOwner() { return *(EHANDLE*)((uintptr_t)&GetSortOrigin() + sizeof(Vector)); }
	EHANDLE *GetControlPointOwners() { return (EHANDLE*)((uintptr_t)&GetOwner() + sizeof(EHANDLE)); }; //MAX_PARTICLE_CONTROL_POINTS
	Vector& GetSortOrigin() { return *(Vector*)((uintptr_t)this + 0x3B8); }; //m_vSortOrigin
	Vector& GetRenderOrigin() { return GetSortOrigin(); };
	Vector& GetLastMax() { return *(Vector*)((uintptr_t)this + 0x4D4); }
	Vector& GetLastMin() { return *(Vector*)((uintptr_t)this + 0x4C8); }
	Vector& GetMinBounds() { return *(Vector*)((uintptr_t)this + 0x0B4); }
	Vector& GetMaxBounds() { return *(Vector*)((uintptr_t)this + 0x0C0); }
	Vector& GetAggregationCenter() { return *(Vector*)((uintptr_t)this + 0x4E4); };
	matrix3x4_t& GetDrawModelMatrix() { return *(matrix3x4_t*)((uintptr_t)this + 0x4F8); }
	bool IsAggregationDisabled() { return *(bool*)((uintptr_t)this + 0x3AC); }; //note: game writes a dword to this
	bool IsBoundsValid() { return *(bool*)((uintptr_t)this + 0x0B0); }
	//const char* GetDebugName() { return (const char*)((uintptr_t)this + 0x3B1); } //AND BYTE PTR DS:[EDI+3B1],FB ?????
};

struct ParticleEffectList_t
{
	ParticleEffectList_t()
	{
		pParticleEffect = NULL;
	}

	CUtlVector<ParticleControlPoint_t>	pControlPoints;
	/*CSmartPtr<CNewParticleEffect>*/ CNewParticleEffect*	pParticleEffect;
};

class CParticleProperty
{
public:
	CBaseEntity *m_pOuter;
	CUtlVector<ParticleEffectList_t>	m_ParticleEffects;
	int			m_iDormancyChangedAtFrame;

	friend class CBaseEntity;
};

Vector MainViewOrigin(int nSlot);

extern bool* s_bEnableInvalidateBoneCache;
extern bool* bAllowBoneAccessForViewModels;
extern bool* bAllowBoneAccessForNormalModels;
extern unsigned long* g_iModelBoneCounter;
extern bool* g_bInThreadedBoneSetup;
extern bool (*ThreadInMainThread)(void);
extern DWORD AdrOf_SequenceDuration;

extern bool AllowSetupBonesToUpdateAttachments;
CBaseEntity* GetHudPlayer();
void __stdcall SurvivalCalcView(void* survivalgamerules, CBaseEntity* _Entity, Vector& eyeOrigin, QAngle& eyeAngles);
const CBaseEntity* EntityFromEntityHandle(const IHandleEntity* pConstHandleEntity);
CBaseEntity* EntityFromEntityHandle(IHandleEntity* pHandleEntity);
enum RenderableTranslucencyType_t;
extern RenderableTranslucencyType_t GetTranslucencyType(ClientRenderHandle_t handle);
extern DWORD oDoAnimationEvent;
extern DWORD oCSPlayerAnimState_Release;

class CEntIndexLessFunc
{
public:
	bool Less(CBaseEntity * const & lhs, CBaseEntity * const & rhs, void *pContext);
};

class CPredictableList
{
public:
	CBaseEntity	*GetPredictable(int slot);
	int				GetPredictableCount(void) const;

protected:
	void			AddToPredictableList(CBaseEntity *add);
	void			RemoveFromPredictablesList(CBaseEntity *remove);

private:
	CUtlSortVector< CBaseEntity *, CEntIndexLessFunc >	m_Predictables;

	friend class CBaseEntity;
};

__forceinline CBaseEntity *CPredictableList::GetPredictable(int slot)
{
	return m_Predictables[slot];
}

__forceinline int CPredictableList::GetPredictableCount(void) const
{
	return m_Predictables.Count();
}

extern CPredictableList *GetPredictables(int nSlot);

#define ENTCLIENTFLAG_DONTUSEIK 2
enum
{
	EFL_KILLME = (1 << 0),	// This entity is marked for death -- This allows the game to actually delete ents at a safe time
	EFL_DORMANT = (1 << 1),	// Entity is dormant, no updates to client
	EFL_NOCLIP_ACTIVE = (1 << 2),	// Lets us know when the noclip command is active.
	EFL_SETTING_UP_BONES = (1 << 3),	// Set while a model is setting up its bones.
	EFL_KEEP_ON_RECREATE_ENTITIES = (1 << 4), // This is a special entity that should not be deleted when we restart entities only

	EFL_HAS_PLAYER_CHILD = (1 << 4),	// One of the child entities is a player.

	EFL_DIRTY_SHADOWUPDATE = (1 << 5),	// Client only- need shadow manager to update the shadow...
	EFL_NOTIFY = (1 << 6),	// Another entity is watching events on this entity (used by teleport)

	// The default behavior in ShouldTransmit is to not send an entity if it doesn't
	// have a model. Certain entities want to be sent anyway because all the drawing logic
	// is in the client DLL. They can set this flag and the engine will transmit them even
	// if they don't have a model.
	EFL_FORCE_CHECK_TRANSMIT = (1 << 7),

	EFL_BOT_FROZEN = (1 << 8),	// This is set on bots that are frozen.
	EFL_SERVER_ONLY = (1 << 9),	// Non-networked entity.
	EFL_NO_AUTO_EDICT_ATTACH = (1 << 10), // Don't attach the edict; we're doing it explicitly

	// Some dirty bits with respect to abs computations
	EFL_DIRTY_ABSTRANSFORM = (1 << 11),
	EFL_DIRTY_ABSVELOCITY = (1 << 12),
	EFL_DIRTY_ABSANGVELOCITY = (1 << 13),
	EFL_DIRTY_SURROUNDING_COLLISION_BOUNDS = (1 << 14),
	EFL_DIRTY_SPATIAL_PARTITION = (1 << 15),
	//	UNUSED						= (1<<16),

	EFL_IN_SKYBOX = (1 << 17),	// This is set if the entity detects that it's in the skybox.
	// This forces it to pass the "in PVS" for transmission.
	EFL_USE_PARTITION_WHEN_NOT_SOLID = (1 << 18),	// Entities with this flag set show up in the partition even when not solid
	EFL_TOUCHING_FLUID = (1 << 19),	// Used to determine if an entity is floating

	// FIXME: Not really sure where I should add this...
	EFL_IS_BEING_LIFTED_BY_BARNACLE = (1 << 20),
	EFL_NO_ROTORWASH_PUSH = (1 << 21),		// I shouldn't be pushed by the rotorwash
	EFL_NO_THINK_FUNCTION = (1 << 22),
	EFL_NO_GAME_PHYSICS_SIMULATION = (1 << 23),

	EFL_CHECK_UNTOUCH = (1 << 24),
	EFL_DONTBLOCKLOS = (1 << 25),		// I shouldn't block NPC line-of-sight
	EFL_DONTWALKON = (1 << 26),		// NPC;s should not walk on this entity
	EFL_NO_DISSOLVE = (1 << 27),		// These guys shouldn't dissolve
	EFL_NO_MEGAPHYSCANNON_RAGDOLL = (1 << 28),	// Mega physcannon can't ragdoll these guys.
	EFL_NO_WATER_VELOCITY_CHANGE = (1 << 29),	// Don't adjust this entity's velocity when transitioning into water
	EFL_NO_PHYSCANNON_INTERACTION = (1 << 30),	// Physcannon can't pick these up or punt them
	EFL_NO_DAMAGE_FORCES = (1 << 31),	// Doesn't accept forces from physics damage
};

#define CNetworkQAngle(name) QAngle name;
#define CNetworkVector(name) Vector name;
#define CNetworkVar(type, name) type name;
#define CNetworkArray(type, name, count) type name[count];

struct localdata_t
{
	CNetworkArray(unsigned char, m_chAreaBits, MAX_AREA_STATE_BYTES); // Area visibility flags.
	CNetworkArray(unsigned char, m_chAreaPortalBits, MAX_AREA_PORTAL_STATE_BYTES); // Area portal visibility flags.

	// BEGIN PREDICTION DATA COMPACTION (these fields are together to allow for faster copying in prediction system)
	int m_nStepside;
	int m_nOldButtons;
	CNetworkVar(float, m_flFOVRate); // rate at which the FOV changes

	CNetworkVar(int, m_iHideHUD); // bitfields containing sections of the HUD to hide
	CNetworkVar(int, m_nDuckTimeMsecs_);
	CNetworkVar(int, m_nDuckJumpTimeMsecs_);
	CNetworkVar(int, m_nJumpTimeMsecs_);

	CNetworkVar(float, m_flFallVelocity_);
	float m_flOldFallVelocity;
	CNetworkVar(float, m_flStepSize_);

	CNetworkQAngle(m_viewPunchAngle_);
	CNetworkQAngle(m_aimPunchAngle_); // auto-decaying view angle adjustment
	CNetworkQAngle(m_aimPunchAngleVel_); // velocity of auto-decaying view angle adjustment

	CNetworkVar(bool, m_bDucked_);
	CNetworkVar(bool, m_bDucking_);

	CNetworkVar(float, m_flLastDuckTime_);
	CNetworkVar(bool, m_bInDuckJump_);
	CNetworkVar(bool, m_bDrawViewmodel);
	CNetworkVar(bool, m_bWearingSuit);
	CNetworkVar(bool, m_bPoisoned);
	CNetworkVar(bool, m_bAllowAutoMovement_);
	// END PREDICTION DATA COMPACTION

	bool m_bInLanding;
	float m_flLandingTime;

	// Base velocity that was passed in to server physics so
	//  client can predict conveyors correctly.  Server zeroes it, so we need to store here, too.
	Vector m_vecClientBaseVelocity;
};

class CPlayerLocalData
{
public:
	virtual void NetworkStateChanged(void* pVar);
	virtual void NetworkStateChanged();

	localdata_t localdata;
};

typedef enum
{
	GROUND = 0,
	STUCK,
	LADDER,
	LADDER_WEDGE
} IntervalType_t;

enum
{
	WL_NotInWater = 0,
	WL_Feet,
	WL_Waist,
	WL_Eyes
};

// The various states the player can be in during the join game process.
enum CSPlayerState
{
	// Happily running around in the game.
	// You can't move though if CSGameRules()->IsFreezePeriod() returns true.
	// This state can jump to a bunch of other states like STATE_PICKINGCLASS or STATE_DEATH_ANIM.
	STATE_ACTIVE = 0,

	// This is the state you're in when you first enter the server.
	// It's switching between intro cameras every few seconds, and there's a level info
	// screen up.
	STATE_WELCOME, // Show the level intro screen.

	// During these states, you can either be a new player waiting to join, or
	// you can be a live player in the game who wants to change teams.
	// Either way, you can't move while choosing team or class (or while any menu is up).
	STATE_PICKINGTEAM, // Choosing team.
	STATE_PICKINGCLASS, // Choosing class.

	STATE_DEATH_ANIM, // Playing death anim, waiting for that to finish.
	STATE_DEATH_WAIT_FOR_KEY, // Done playing death anim. Waiting for keypress to go into observer mode.
	STATE_OBSERVER_MODE, // Noclipping around, watching players, etc.
	NUM_PLAYER_STATES
};

// Spectator Movement modes
enum
{
	OBS_MODE_NONE = 0, // not in spectator mode
	OBS_MODE_DEATHCAM, // special mode for death cam animation
	OBS_MODE_FREEZECAM, // zooms to a target, and freeze-frames on them
	OBS_MODE_FIXED, // view from a fixed camera position
	OBS_MODE_IN_EYE, // follow a player in first person view
	OBS_MODE_CHASE, // follow a player in third person view
	OBS_MODE_ROAMING, // free roaming

	NUM_OBSERVER_MODES,
};

// edict->solid values
// NOTE: Some movetypes will cause collisions independent of SOLID_NOT/SOLID_TRIGGER when the entity moves
// SOLID only effects OTHER entities colliding with this one when they move - UGH!

// Solid type basically describes how the bounding volume of the object is represented
// NOTE: SOLID_BBOX MUST BE 2, and SOLID_VPHYSICS MUST BE 6
// NOTE: These numerical values are used in the FGD by the prop code (see prop_dynamic)
enum SolidType_t
{
	SOLID_NONE	 = 0, // no solid model
	SOLID_BSP	  = 1, // a BSP tree
	SOLID_BBOX	 = 2, // an AABB
	SOLID_OBB	  = 3, // an OBB (not implemented yet)
	SOLID_OBB_YAW  = 4, // an OBB, constrained so that it can only yaw
	SOLID_CUSTOM   = 5, // Always call into the entity for tests
	SOLID_VPHYSICS = 6, // solid vphysics object, get vcollide from the model and collide with that
	SOLID_LAST,
};

enum SolidFlags_t
{
	FSOLID_CUSTOMRAYTEST = 0x0001,	// Ignore solid type + always call into the entity for ray tests
	FSOLID_CUSTOMBOXTEST = 0x0002,	// Ignore solid type + always call into the entity for swept box tests
	FSOLID_NOT_SOLID = 0x0004,	// Are we currently not solid?
	FSOLID_TRIGGER = 0x0008,	// This is something may be collideable but fires touch functions
	// even when it's not collideable (when the FSOLID_NOT_SOLID flag is set)
	FSOLID_NOT_STANDABLE = 0x0010,	// You can't stand on this
	FSOLID_VOLUME_CONTENTS = 0x0020,	// Contains volumetric contents (like water)
	FSOLID_FORCE_WORLD_ALIGNED = 0x0040,	// Forces the collision rep to be world-aligned even if it's SOLID_BSP or SOLID_VPHYSICS
	FSOLID_USE_TRIGGER_BOUNDS = 0x0080,	// Uses a special trigger bounds separate from the normal OBB
	FSOLID_ROOT_PARENT_ALIGNED = 0x0100,	// Collisions are defined in root parent's local coordinate space
	FSOLID_TRIGGER_TOUCH_DEBRIS = 0x0200,	// This trigger will touch debris objects
	FSOLID_TRIGGER_TOUCH_PLAYER = 0x0400,	// This trigger will touch only players
	FSOLID_NOT_MOVEABLE = 0x0800,	// Assume this object will not move

	FSOLID_MAX_BITS = 12
};

//
// Enumerations for setting player animation.
//
enum PLAYER_ANIM
{
	PLAYER_IDLE,
	PLAYER_WALK,
	PLAYER_JUMP,
	PLAYER_SUPERJUMP,
	PLAYER_DIE,
	PLAYER_ATTACK1,
	PLAYER_IN_VEHICLE,

	// TF Player animations
	PLAYER_RELOAD,
	PLAYER_START_AIMING,
	PLAYER_LEAVE_AIMING,
};

enum MoveType_t
{
	MOVETYPE_NONE = 0,
	MOVETYPE_ISOMETRIC,
	MOVETYPE_WALK,
	MOVETYPE_STEP,
	MOVETYPE_FLY,
	MOVETYPE_FLYGRAVITY,
	MOVETYPE_VPHYSICS,
	MOVETYPE_PUSH,
	MOVETYPE_NOCLIP,
	MOVETYPE_LADDER,
	MOVETYPE_OBSERVER,
	MOVETYPE_CUSTOM,
	MOVETYPE_LAST	 = MOVETYPE_CUSTOM,
	MOVETYPE_MAX_BITS = 4
};
enum SequenceNames : int
{
	default = 0,
	ragdoll = 1,
	strafe = 2,
	recrouch_generic = 3,
	reposition = 4,
	reposition_to_idle = 5,
	reposition_to_crouchidle = 6,
	lean = 7,
	additive_posebreaker = 8,
	additive_posebreaker_knife = 9,
	rom = 10,
	rom_skin = 11,
	s_ladder_crouch_delta = 12,
	ladder_climb = 13,
	fall = 14,
	jump = 15,
	jump_moving = 16,
	jump_crouch = 17,
	jump_moving_crouch = 18,
	land_light_crouch_moving = 19,
	land_light = 20,
	land_light_crouch = 21,
	land_light_moving = 22,
	land_heavy = 23,
	land_heavy_crouch = 24,
	move_c = 25,
	move = 26,
	move_knife_c = 27,
	move_knife = 28,
	move_grenade_c = 29,
	move_grenade = 30,
	move_c4_c = 31,
	move_c4 = 32,
	pistol_deploy = 33,
	pistol_deploy_crouch = 34,
	pistol_deploy_02 = 35,
	pistol_deploy_03 = 36,
	rifle_deploy = 37,
	rifle_deploy_crouch = 38,
	rifle_deploy_02 = 39,
	rifle_deploy_03 = 40,
	heavy_deploy = 41,
	heavy_deploy_crouch = 42,
	heavy_deploy_02 = 43,
	heavy_deploy_03 = 44,
	sniper_deploy = 45,
	sniper_deploy_crouch = 46,
	sniper_deploy_02 = 47,
	sniper_deploy_03 = 48,
	smg_deploy = 49,
	smg_deploy_crouch = 50,
	smg_deploy_02 = 51,
	smg_deploy_03 = 52,
	shotgun_deploy = 53,
	shotgun_deploy_crouch = 54,
	shotgun_deploy_02 = 55,
	shotgun_deploy_03 = 56,
	knife_deploy = 57,
	knife_deploy_crouch = 58,
	knife_deploy_02 = 59,
	knife_deploy_03 = 60,
	c4_deploy = 61,
	c4_deploy_crouch = 62,
	c4_deploy_02 = 63,
	c4_deploy_03 = 64,
	grenade_deploy = 65,
	grenade_deploy_crouch = 66,
	grenade_deploy_02 = 67,
	grenade_deploy_03 = 68,
	grenade_catch = 69,
	grenade_catch_crouch = 70,
	pistol_catch = 71,
	pistol_catch_crouch = 72,
	c4_catch = 73,
	c4_catch_crouch = 74,
	shotgun_catch = 75,
	shotgun_catch_crouch = 76,
	smg_catch = 77,
	smg_catch_crouch = 78,
	sniper_catch = 79,
	sniper_catch_crouch = 80,
	heavy_catch = 81,
	heavy_catch_crouch = 82,
	rifle_catch = 83,
	rifle_catch_crouch = 84,
	pistol_aim_idle = 85,
	pistol_aim_walk = 86,
	pistol_aim_run = 87,
	pistol_aim_crouch_idle = 88,
	pistol_aim_crouch_moving = 89,
	pistol_aim = 90,
	pistol_fire = 91,
	pistol_fire_crouch = 92,
	pistol_fire_alt = 93,
	pistol_fire_alt_crouch = 94,
	pistol_reload = 95,
	pistol_reload_moving = 96,
	pistol_reload_crouch = 97,
	pistol_reload_crouch_moving = 98,
	pistol_silencer_off = 99,
	pistol_silencer_on = 100,
	rifle_aim_idle = 101,
	rifle_aim_walk = 102,
	rifle_aim_run = 103,
	rifle_aim_crouch_idle = 104,
	rifle_aim_crouch_moving = 105,
	rifle_aim = 106,
	rifle_fire = 107,
	rifle_fire_crouch = 108,
	rifle_fire_alt = 109,
	rifle_fire_alt_crouch = 110,
	rifle_reload = 111,
	rifle_reload_moving = 112,
	rifle_reload_crouch = 113,
	rifle_reload_crouch_moving = 114,
	rifle_silencer_off = 115,
	rifle_silencer_on = 116,
	sniper_aim_idle = 117,
	sniper_aim_walk = 118,
	sniper_aim_run = 119,
	sniper_aim_crouch_idle = 120,
	sniper_aim_crouch_moving = 121,
	sniper_aim = 122,
	sniper_fire = 123,
	sniper_fire_crouch = 124,
	sniper_reload = 125,
	sniper_reload_moving = 126,
	sniper_reload_crouch = 127,
	sniper_reload_crouch_moving = 128,
	shotgun_aim_idle = 129,
	shotgun_aim_walk = 130,
	shotgun_aim_run = 131,
	shotgun_aim_crouch_idle = 132,
	shotgun_aim_crouch_moving = 133,
	shotgun_aim = 134,
	shotgun_fire = 135,
	shotgun_fire_crouch = 136,
	shotgun_reload_start = 137,
	shotgun_reload_loop = 138,
	shotgun_reload_end = 139,
	shotgun_reload_start_crouch = 140,
	shotgun_reload_loop_crouch = 141,
	shotgun_reload_end_crouch = 142,
	smg_aim_idle = 143,
	smg_aim_walk = 144,
	smg_aim_run = 145,
	smg_aim_crouch_idle = 146,
	smg_aim_crouch_moving = 147,
	smg_aim = 148,
	smg_fire = 149,
	smg_fire_crouch = 150,
	smg_reload = 151,
	smg_reload_moving = 152,
	smg_reload_crouch = 153,
	smg_reload_crouch_moving = 154,
	heavy_aim_idle = 155,
	heavy_aim_walk = 156,
	heavy_aim_run = 157,
	heavy_aim_crouch_idle = 158,
	heavy_aim_crouch_moving = 159,
	heavy_aim = 160,
	heavy_fire = 161,
	heavy_fire_crouch = 162,
	heavy_reload = 163,
	heavy_reload_moving = 164,
	heavy_reload_crouch = 165,
	heavy_reload_crouch_moving = 166,
	knife_aim_idle = 167,
	knife_aim_walk = 168,
	knife_aim_run = 169,
	knife_aim_crouch_idle = 170,
	knife_aim_crouch_moving = 171,
	knife_aim = 172,
	knife_fire_front = 173,
	knife_fire_front_moving = 174,
	knife_fire_front_crouch = 175,
	knife_fire_front_crouch_moving = 176,
	knife_fire_frontswing_a = 177,
	knife_fire_frontswing_a_moving = 178,
	knife_fire_frontswing_a_crouch = 179,
	knife_fire_frontswing_a_crouch_moving = 180,
	knife_fire_frontswing_b = 181,
	knife_fire_frontswing_b_moving = 182,
	knife_fire_frontswing_b_crouch = 183,
	knife_fire_frontswing_b_crouch_moving = 184,
	knife_fire_back = 185,
	knife_fire_back_moving = 186,
	knife_fire_back_crouch = 187,
	knife_fire_back_crouch_moving = 188,
	knife_fire_back_overhead = 189,
	knife_fire_back_overhead_moving = 190,
	knife_fire_back_overhead_crouch = 191,
	knife_fire_back_overhead_crouch_moving = 192,
	grenade_aim_idle = 193,
	grenade_aim_walk = 194,
	grenade_aim_run = 195,
	grenade_aim_crouch_idle = 196,
	grenade_aim_crouch_moving = 197,
	grenade_aim = 198,
	grenade_near_fire = 199,
	grenade_near_fire_moving = 200,
	grenade_near_fire_crouch = 201,
	grenade_near_fire_crouch_moving = 202,
	grenade_operate = 203,
	grenade_fire = 204,
	grenade_operate_moving = 205,
	grenade_fire_moving = 206,
	grenade_operate_crouch = 207,
	grenade_fire_crouch = 208,
	grenade_operate_crouch_moving = 209,
	grenade_fire_crouch_moving = 210,
	c4_aim_idle = 211,
	c4_aim_walk = 212,
	c4_aim_run = 213,
	c4_aim_crouch_idle = 214,
	c4_aim_crouch_moving = 215,
	c4_aim = 216,
	c4_defuse_aim_crouch = 217,
	c4_defuse_aim_stand = 218,
	s_defuse_crouch = 219,
	c4_defusal = 220,
	s_defuse_crouch_kit = 221,
	c4_defusal_with_kit = 222,
	c4_fire = 223,
	flashed = 224,
	flashed_crouch = 225,
	flashed_knife = 226,
	flashed_knife_crouch = 227,
	death_pose_head = 228,
	death_pose_head_crouch = 229,
	death_pose_body = 230,
	death_pose_body_crouch = 231,
	deathpose_lowviolence = 232,
	flinch_arm_left = 233,
	flinch_arm_right = 234,
	flinch_leg_left = 235,
	flinch_leg_right = 236,
	flinch_chest = 237,
	flinch_chest_rear = 238,
	flinch_chest_right = 239,
	flinch_chest_left = 240,
	flinch_head = 241,
	flinch_head_rear = 242,
	flinch_head_right = 243,
	flinch_head_left = 244,
	flinch_stomach = 245,
	flinch_stomach_rear = 246,
	flinch_stomach_right = 247,
	flinch_stomach_left = 248,
	flinch_molotov = 249,
	move_w = 250,
	move_r = 251,
	move_knife_w = 252,
	move_knife_r = 253,
	move_grenade_w = 254,
	move_grenade_r = 255,
	move_c4_w = 256,
	move_c4_r = 257,
	pistol_silencer_off_crouch = 258,
	pistol_silencer_on_crouch = 259,
	rifle_silencer_off_crouch = 260,
	rifle_silencer_on_crouch = 261,
	surrender = 262,
	parachute_dir = 263,
	parachute = 264,
	shield_deploy = 265,
	shield_aim_idle = 266,
	shield_aim_walk = 267,
	shield_aim_run = 268,
	shield_aim_crouch_idle = 269,
	shield_aim_crouch_moving = 270,
	shield_aim = 271,
	shield_fire = 272,
	shield_fire_crouch = 273
};

enum Activities : int
{
	ACT_INVALID = -1,
	ACT_RESET = 0,
	ACT_IDLE = 1,
	ACT_TRANSITION = 2,
	ACT_COVER = 3,
	ACT_COVER_MED = 4,
	ACT_COVER_LOW = 5,
	ACT_WALK = 6,
	ACT_WALK_AIM = 7,
	ACT_WALK_CROUCH = 8,
	ACT_WALK_CROUCH_AIM = 9,
	ACT_RUN = 10,
	ACT_RUN_AIM = 11,
	ACT_RUN_CROUCH = 12,
	ACT_RUN_CROUCH_AIM = 13,
	ACT_RUN_PROTECTED = 14,
	ACT_SCRIPT_CUSTOM_MOVE = 15,
	ACT_RANGE_ATTACK1 = 16,
	ACT_RANGE_ATTACK2 = 17,
	ACT_RANGE_ATTACK1_LOW = 18,
	ACT_RANGE_ATTACK2_LOW = 19,
	ACT_DIESIMPLE = 20,
	ACT_DIEBACKWARD = 21,
	ACT_DIEFORWARD = 22,
	ACT_DIEVIOLENT = 23,
	ACT_DIERAGDOLL = 24,
	ACT_FLY = 25,
	ACT_HOVER = 26,
	ACT_GLIDE = 27,
	ACT_SWIM = 28,
	ACT_JUMP = 29,
	ACT_HOP = 30,
	ACT_LEAP = 31,
	ACT_LAND = 32,
	ACT_CLIMB_UP = 33,
	ACT_CLIMB_DOWN = 34,
	ACT_CLIMB_DISMOUNT = 35,
	ACT_SHIPLADDER_UP = 36,
	ACT_SHIPLADDER_DOWN = 37,
	ACT_STRAFE_LEFT = 38,
	ACT_STRAFE_RIGHT = 39,
	ACT_ROLL_LEFT = 40,
	ACT_ROLL_RIGHT = 41,
	ACT_TURN_LEFT = 42,
	ACT_TURN_RIGHT = 43,
	ACT_CROUCH = 44,
	ACT_CROUCHIDLE = 45,
	ACT_STAND = 46,
	ACT_USE = 47,
	ACT_ALIEN_BURROW_IDLE = 48,
	ACT_ALIEN_BURROW_OUT = 49,
	ACT_SIGNAL1 = 50,
	ACT_SIGNAL2 = 51,
	ACT_SIGNAL3 = 52,
	ACT_SIGNAL_ADVANCE = 53,
	ACT_SIGNAL_FORWARD = 54,
	ACT_SIGNAL_GROUP = 55,
	ACT_SIGNAL_HALT = 56,
	ACT_SIGNAL_LEFT = 57,
	ACT_SIGNAL_RIGHT = 58,
	ACT_SIGNAL_TAKECOVER = 59,
	ACT_LOOKBACK_RIGHT = 60,
	ACT_LOOKBACK_LEFT = 61,
	ACT_COWER = 62,
	ACT_SMALL_FLINCH = 63,
	ACT_BIG_FLINCH = 64,
	ACT_MELEE_ATTACK1 = 65,
	ACT_MELEE_ATTACK2 = 66,
	ACT_RELOAD = 67,
	ACT_RELOAD_START = 68,
	ACT_RELOAD_FINISH = 69,
	ACT_RELOAD_LOW = 70,
	ACT_ARM = 71,
	ACT_DISARM = 72,
	ACT_DROP_WEAPON = 73,
	ACT_DROP_WEAPON_SHOTGUN = 74,
	ACT_PICKUP_GROUND = 75,
	ACT_PICKUP_RACK = 76,
	ACT_IDLE_ANGRY = 77,
	ACT_IDLE_RELAXED = 78,
	ACT_IDLE_STIMULATED = 79,
	ACT_IDLE_AGITATED = 80,
	ACT_IDLE_STEALTH = 81,
	ACT_IDLE_HURT = 82,
	ACT_WALK_RELAXED = 83,
	ACT_WALK_STIMULATED = 84,
	ACT_WALK_AGITATED = 85,
	ACT_WALK_STEALTH = 86,
	ACT_RUN_RELAXED = 87,
	ACT_RUN_STIMULATED = 88,
	ACT_RUN_AGITATED = 89,
	ACT_RUN_STEALTH = 90,
	ACT_IDLE_AIM_RELAXED = 91,
	ACT_IDLE_AIM_STIMULATED = 92,
	ACT_IDLE_AIM_AGITATED = 93,
	ACT_IDLE_AIM_STEALTH = 94,
	ACT_WALK_AIM_RELAXED = 95,
	ACT_WALK_AIM_STIMULATED = 96,
	ACT_WALK_AIM_AGITATED = 97,
	ACT_WALK_AIM_STEALTH = 98,
	ACT_RUN_AIM_RELAXED = 99,
	ACT_RUN_AIM_STIMULATED = 100,
	ACT_RUN_AIM_AGITATED = 101,
	ACT_RUN_AIM_STEALTH = 102,
	ACT_CROUCHIDLE_STIMULATED = 103,
	ACT_CROUCHIDLE_AIM_STIMULATED = 104,
	ACT_CROUCHIDLE_AGITATED = 105,
	ACT_WALK_HURT = 106,
	ACT_RUN_HURT = 107,
	ACT_SPECIAL_ATTACK1 = 108,
	ACT_SPECIAL_ATTACK2 = 109,
	ACT_COMBAT_IDLE = 110,
	ACT_WALK_SCARED = 111,
	ACT_RUN_SCARED = 112,
	ACT_VICTORY_DANCE = 113,
	ACT_DIE_HEADSHOT = 114,
	ACT_DIE_CHESTSHOT = 115,
	ACT_DIE_GUTSHOT = 116,
	ACT_DIE_BACKSHOT = 117,
	ACT_FLINCH_HEAD = 118,
	ACT_FLINCH_CHEST = 119,
	ACT_FLINCH_STOMACH = 120,
	ACT_FLINCH_LEFTARM = 121,
	ACT_FLINCH_RIGHTARM = 122,
	ACT_FLINCH_LEFTLEG = 123,
	ACT_FLINCH_RIGHTLEG = 124,
	ACT_FLINCH_PHYSICS = 125,
	ACT_FLINCH_HEAD_BACK = 126,
	ACT_FLINCH_HEAD_LEFT = 127,
	ACT_FLINCH_HEAD_RIGHT = 128,
	ACT_FLINCH_CHEST_BACK = 129,
	ACT_FLINCH_STOMACH_BACK = 130,
	ACT_FLINCH_CROUCH_FRONT = 131,
	ACT_FLINCH_CROUCH_BACK = 132,
	ACT_FLINCH_CROUCH_LEFT = 133,
	ACT_FLINCH_CROUCH_RIGHT = 134,
	ACT_IDLE_ON_FIRE = 135,
	ACT_WALK_ON_FIRE = 136,
	ACT_RUN_ON_FIRE = 137,
	ACT_RAPPEL_LOOP = 138,
	ACT_180_LEFT = 139,
	ACT_180_RIGHT = 140,
	ACT_90_LEFT = 141,
	ACT_90_RIGHT = 142,
	ACT_STEP_LEFT = 143,
	ACT_STEP_RIGHT = 144,
	ACT_STEP_BACK = 145,
	ACT_STEP_FORE = 146,
	ACT_GESTURE_RANGE_ATTACK1 = 147,
	ACT_GESTURE_RANGE_ATTACK2 = 148,
	ACT_GESTURE_MELEE_ATTACK1 = 149,
	ACT_GESTURE_MELEE_ATTACK2 = 150,
	ACT_GESTURE_RANGE_ATTACK1_LOW = 151,
	ACT_GESTURE_RANGE_ATTACK2_LOW = 152,
	ACT_MELEE_ATTACK_SWING_GESTURE = 153,
	ACT_GESTURE_SMALL_FLINCH = 154,
	ACT_GESTURE_BIG_FLINCH = 155,
	ACT_GESTURE_FLINCH_BLAST = 156,
	ACT_GESTURE_FLINCH_BLAST_SHOTGUN = 157,
	ACT_GESTURE_FLINCH_BLAST_DAMAGED = 158,
	ACT_GESTURE_FLINCH_BLAST_DAMAGED_SHOTGUN = 159,
	ACT_GESTURE_FLINCH_HEAD = 160,
	ACT_GESTURE_FLINCH_CHEST = 161,
	ACT_GESTURE_FLINCH_STOMACH = 162,
	ACT_GESTURE_FLINCH_LEFTARM = 163,
	ACT_GESTURE_FLINCH_RIGHTARM = 164,
	ACT_GESTURE_FLINCH_LEFTLEG = 165,
	ACT_GESTURE_FLINCH_RIGHTLEG = 166,
	ACT_GESTURE_TURN_LEFT = 167,
	ACT_GESTURE_TURN_RIGHT = 168,
	ACT_GESTURE_TURN_LEFT45 = 169,
	ACT_GESTURE_TURN_RIGHT45 = 170,
	ACT_GESTURE_TURN_LEFT90 = 171,
	ACT_GESTURE_TURN_RIGHT90 = 172,
	ACT_GESTURE_TURN_LEFT45_FLAT = 173,
	ACT_GESTURE_TURN_RIGHT45_FLAT = 174,
	ACT_GESTURE_TURN_LEFT90_FLAT = 175,
	ACT_GESTURE_TURN_RIGHT90_FLAT = 176,
	ACT_BARNACLE_HIT = 177,
	ACT_BARNACLE_PULL = 178,
	ACT_BARNACLE_CHOMP = 179,
	ACT_BARNACLE_CHEW = 180,
	ACT_DO_NOT_DISTURB = 181,
	ACT_SPECIFIC_SEQUENCE = 182,
	ACT_VM_DRAW = 183,
	ACT_VM_HOLSTER = 184,
	ACT_VM_IDLE = 185,
	ACT_VM_FIDGET = 186,
	ACT_VM_PULLBACK = 187,
	ACT_VM_PULLBACK_HIGH = 188,
	ACT_VM_PULLBACK_LOW = 189,
	ACT_VM_THROW = 190,
	ACT_VM_PULLPIN = 191,
	ACT_VM_PRIMARYATTACK = 192,
	ACT_VM_SECONDARYATTACK = 193,
	ACT_VM_RELOAD = 194,
	ACT_VM_DRYFIRE = 195,
	ACT_VM_HITLEFT = 196,
	ACT_VM_HITLEFT2 = 197,
	ACT_VM_HITRIGHT = 198,
	ACT_VM_HITRIGHT2 = 199,
	ACT_VM_HITCENTER = 200,
	ACT_VM_HITCENTER2 = 201,
	ACT_VM_MISSLEFT = 202,
	ACT_VM_MISSLEFT2 = 203,
	ACT_VM_MISSRIGHT = 204,
	ACT_VM_MISSRIGHT2 = 205,
	ACT_VM_MISSCENTER = 206,
	ACT_VM_MISSCENTER2 = 207,
	ACT_VM_HAULBACK = 208,
	ACT_VM_SWINGHARD = 209,
	ACT_VM_SWINGMISS = 210,
	ACT_VM_SWINGHIT = 211,
	ACT_VM_IDLE_TO_LOWERED = 212,
	ACT_VM_IDLE_LOWERED = 213,
	ACT_VM_LOWERED_TO_IDLE = 214,
	ACT_VM_RECOIL1 = 215,
	ACT_VM_RECOIL2 = 216,
	ACT_VM_RECOIL3 = 217,
	ACT_VM_PICKUP = 218,
	ACT_VM_RELEASE = 219,
	ACT_VM_ATTACH_SILENCER = 220,
	ACT_VM_DETACH_SILENCER = 221,
	ACT_VM_EMPTY_FIRE = 222,
	ACT_VM_EMPTY_RELOAD = 223,
	ACT_VM_EMPTY_DRAW = 224,
	ACT_VM_EMPTY_IDLE = 225,
	ACT_SLAM_STICKWALL_IDLE = 226,
	ACT_SLAM_STICKWALL_ND_IDLE = 227,
	ACT_SLAM_STICKWALL_ATTACH = 228,
	ACT_SLAM_STICKWALL_ATTACH2 = 229,
	ACT_SLAM_STICKWALL_ND_ATTACH = 230,
	ACT_SLAM_STICKWALL_ND_ATTACH2 = 231,
	ACT_SLAM_STICKWALL_DETONATE = 232,
	ACT_SLAM_STICKWALL_DETONATOR_HOLSTER = 233,
	ACT_SLAM_STICKWALL_DRAW = 234,
	ACT_SLAM_STICKWALL_ND_DRAW = 235,
	ACT_SLAM_STICKWALL_TO_THROW = 236,
	ACT_SLAM_STICKWALL_TO_THROW_ND = 237,
	ACT_SLAM_STICKWALL_TO_TRIPMINE_ND = 238,
	ACT_SLAM_THROW_IDLE = 239,
	ACT_SLAM_THROW_ND_IDLE = 240,
	ACT_SLAM_THROW_THROW = 241,
	ACT_SLAM_THROW_THROW2 = 242,
	ACT_SLAM_THROW_THROW_ND = 243,
	ACT_SLAM_THROW_THROW_ND2 = 244,
	ACT_SLAM_THROW_DRAW = 245,
	ACT_SLAM_THROW_ND_DRAW = 246,
	ACT_SLAM_THROW_TO_STICKWALL = 247,
	ACT_SLAM_THROW_TO_STICKWALL_ND = 248,
	ACT_SLAM_THROW_DETONATE = 249,
	ACT_SLAM_THROW_DETONATOR_HOLSTER = 250,
	ACT_SLAM_THROW_TO_TRIPMINE_ND = 251,
	ACT_SLAM_TRIPMINE_IDLE = 252,
	ACT_SLAM_TRIPMINE_DRAW = 253,
	ACT_SLAM_TRIPMINE_ATTACH = 254,
	ACT_SLAM_TRIPMINE_ATTACH2 = 255,
	ACT_SLAM_TRIPMINE_TO_STICKWALL_ND = 256,
	ACT_SLAM_TRIPMINE_TO_THROW_ND = 257,
	ACT_SLAM_DETONATOR_IDLE = 258,
	ACT_SLAM_DETONATOR_DRAW = 259,
	ACT_SLAM_DETONATOR_DETONATE = 260,
	ACT_SLAM_DETONATOR_HOLSTER = 261,
	ACT_SLAM_DETONATOR_STICKWALL_DRAW = 262,
	ACT_SLAM_DETONATOR_THROW_DRAW = 263,
	ACT_SHOTGUN_RELOAD_START = 264,
	ACT_SHOTGUN_RELOAD_FINISH = 265,
	ACT_SHOTGUN_PUMP = 266,
	ACT_SMG2_IDLE2 = 267,
	ACT_SMG2_FIRE2 = 268,
	ACT_SMG2_DRAW2 = 269,
	ACT_SMG2_RELOAD2 = 270,
	ACT_SMG2_DRYFIRE2 = 271,
	ACT_SMG2_TOAUTO = 272,
	ACT_SMG2_TOBURST = 273,
	ACT_PHYSCANNON_UPGRADE = 274,
	ACT_RANGE_ATTACK_AR1 = 275,
	ACT_RANGE_ATTACK_AR2 = 276,
	ACT_RANGE_ATTACK_AR2_LOW = 277,
	ACT_RANGE_ATTACK_AR2_GRENADE = 278,
	ACT_RANGE_ATTACK_HMG1 = 279,
	ACT_RANGE_ATTACK_ML = 280,
	ACT_RANGE_ATTACK_SMG1 = 281,
	ACT_RANGE_ATTACK_SMG1_LOW = 282,
	ACT_RANGE_ATTACK_SMG2 = 283,
	ACT_RANGE_ATTACK_SHOTGUN = 284,
	ACT_RANGE_ATTACK_SHOTGUN_LOW = 285,
	ACT_RANGE_ATTACK_PISTOL = 286,
	ACT_RANGE_ATTACK_PISTOL_LOW = 287,
	ACT_RANGE_ATTACK_SLAM = 288,
	ACT_RANGE_ATTACK_TRIPWIRE = 289,
	ACT_RANGE_ATTACK_THROW = 290,
	ACT_RANGE_ATTACK_SNIPER_RIFLE = 291,
	ACT_RANGE_ATTACK_RPG = 292,
	ACT_MELEE_ATTACK_SWING = 293,
	ACT_RANGE_AIM_LOW = 294,
	ACT_RANGE_AIM_SMG1_LOW = 295,
	ACT_RANGE_AIM_PISTOL_LOW = 296,
	ACT_RANGE_AIM_AR2_LOW = 297,
	ACT_COVER_PISTOL_LOW = 298,
	ACT_COVER_SMG1_LOW = 299,
	ACT_GESTURE_RANGE_ATTACK_AR1 = 300,
	ACT_GESTURE_RANGE_ATTACK_AR2 = 301,
	ACT_GESTURE_RANGE_ATTACK_AR2_GRENADE = 302,
	ACT_GESTURE_RANGE_ATTACK_HMG1 = 303,
	ACT_GESTURE_RANGE_ATTACK_ML = 304,
	ACT_GESTURE_RANGE_ATTACK_SMG1 = 305,
	ACT_GESTURE_RANGE_ATTACK_SMG1_LOW = 306,
	ACT_GESTURE_RANGE_ATTACK_SMG2 = 307,
	ACT_GESTURE_RANGE_ATTACK_SHOTGUN = 308,
	ACT_GESTURE_RANGE_ATTACK_PISTOL = 309,
	ACT_GESTURE_RANGE_ATTACK_PISTOL_LOW = 310,
	ACT_GESTURE_RANGE_ATTACK_SLAM = 311,
	ACT_GESTURE_RANGE_ATTACK_TRIPWIRE = 312,
	ACT_GESTURE_RANGE_ATTACK_THROW = 313,
	ACT_GESTURE_RANGE_ATTACK_SNIPER_RIFLE = 314,
	ACT_GESTURE_MELEE_ATTACK_SWING = 315,
	ACT_IDLE_RIFLE = 316,
	ACT_IDLE_SMG1 = 317,
	ACT_IDLE_ANGRY_SMG1 = 318,
	ACT_IDLE_PISTOL = 319,
	ACT_IDLE_ANGRY_PISTOL = 320,
	ACT_IDLE_ANGRY_SHOTGUN = 321,
	ACT_IDLE_STEALTH_PISTOL = 322,
	ACT_IDLE_PACKAGE = 323,
	ACT_WALK_PACKAGE = 324,
	ACT_IDLE_SUITCASE = 325,
	ACT_WALK_SUITCASE = 326,
	ACT_IDLE_SMG1_RELAXED = 327,
	ACT_IDLE_SMG1_STIMULATED = 328,
	ACT_WALK_RIFLE_RELAXED = 329,
	ACT_RUN_RIFLE_RELAXED = 330,
	ACT_WALK_RIFLE_STIMULATED = 331,
	ACT_RUN_RIFLE_STIMULATED = 332,
	ACT_IDLE_AIM_RIFLE_STIMULATED = 333,
	ACT_WALK_AIM_RIFLE_STIMULATED = 334,
	ACT_RUN_AIM_RIFLE_STIMULATED = 335,
	ACT_IDLE_SHOTGUN_RELAXED = 336,
	ACT_IDLE_SHOTGUN_STIMULATED = 337,
	ACT_IDLE_SHOTGUN_AGITATED = 338,
	ACT_WALK_ANGRY = 339,
	ACT_POLICE_HARASS1 = 340,
	ACT_POLICE_HARASS2 = 341,
	ACT_IDLE_MANNEDGUN = 342,
	ACT_IDLE_MELEE = 343,
	ACT_IDLE_ANGRY_MELEE = 344,
	ACT_IDLE_RPG_RELAXED = 345,
	ACT_IDLE_RPG = 346,
	ACT_IDLE_ANGRY_RPG = 347,
	ACT_COVER_LOW_RPG = 348,
	ACT_WALK_RPG = 349,
	ACT_RUN_RPG = 350,
	ACT_WALK_CROUCH_RPG = 351,
	ACT_RUN_CROUCH_RPG = 352,
	ACT_WALK_RPG_RELAXED = 353,
	ACT_RUN_RPG_RELAXED = 354,
	ACT_WALK_RIFLE = 355,
	ACT_WALK_AIM_RIFLE = 356,
	ACT_WALK_CROUCH_RIFLE = 357,
	ACT_WALK_CROUCH_AIM_RIFLE = 358,
	ACT_RUN_RIFLE = 359,
	ACT_RUN_AIM_RIFLE = 360,
	ACT_RUN_CROUCH_RIFLE = 361,
	ACT_RUN_CROUCH_AIM_RIFLE = 362,
	ACT_RUN_STEALTH_PISTOL = 363,
	ACT_WALK_AIM_SHOTGUN = 364,
	ACT_RUN_AIM_SHOTGUN = 365,
	ACT_WALK_PISTOL = 366,
	ACT_RUN_PISTOL = 367,
	ACT_WALK_AIM_PISTOL = 368,
	ACT_RUN_AIM_PISTOL = 369,
	ACT_WALK_STEALTH_PISTOL = 370,
	ACT_WALK_AIM_STEALTH_PISTOL = 371,
	ACT_RUN_AIM_STEALTH_PISTOL = 372,
	ACT_RELOAD_PISTOL = 373,
	ACT_RELOAD_PISTOL_LOW = 374,
	ACT_RELOAD_SMG1 = 375,
	ACT_RELOAD_SMG1_LOW = 376,
	ACT_RELOAD_SHOTGUN = 377,
	ACT_RELOAD_SHOTGUN_LOW = 378,
	ACT_GESTURE_RELOAD = 379,
	ACT_GESTURE_RELOAD_PISTOL = 380,
	ACT_GESTURE_RELOAD_SMG1 = 381,
	ACT_GESTURE_RELOAD_SHOTGUN = 382,
	ACT_BUSY_LEAN_LEFT = 383,
	ACT_BUSY_LEAN_LEFT_ENTRY = 384,
	ACT_BUSY_LEAN_LEFT_EXIT = 385,
	ACT_BUSY_LEAN_BACK = 386,
	ACT_BUSY_LEAN_BACK_ENTRY = 387,
	ACT_BUSY_LEAN_BACK_EXIT = 388,
	ACT_BUSY_SIT_GROUND = 389,
	ACT_BUSY_SIT_GROUND_ENTRY = 390,
	ACT_BUSY_SIT_GROUND_EXIT = 391,
	ACT_BUSY_SIT_CHAIR = 392,
	ACT_BUSY_SIT_CHAIR_ENTRY = 393,
	ACT_BUSY_SIT_CHAIR_EXIT = 394,
	ACT_BUSY_STAND = 395,
	ACT_BUSY_QUEUE = 396,
	ACT_DUCK_DODGE = 397,
	ACT_DIE_BARNACLE_SWALLOW = 398,
	ACT_GESTURE_BARNACLE_STRANGLE = 399,
	ACT_PHYSCANNON_DETACH = 400,
	ACT_PHYSCANNON_ANIMATE = 401,
	ACT_PHYSCANNON_ANIMATE_PRE = 402,
	ACT_PHYSCANNON_ANIMATE_POST = 403,
	ACT_DIE_FRONTSIDE = 404,
	ACT_DIE_RIGHTSIDE = 405,
	ACT_DIE_BACKSIDE = 406,
	ACT_DIE_LEFTSIDE = 407,
	ACT_DIE_CROUCH_FRONTSIDE = 408,
	ACT_DIE_CROUCH_RIGHTSIDE = 409,
	ACT_DIE_CROUCH_BACKSIDE = 410,
	ACT_DIE_CROUCH_LEFTSIDE = 411,
	ACT_OPEN_DOOR = 412,
	ACT_DI_ALYX_ZOMBIE_MELEE = 413,
	ACT_DI_ALYX_ZOMBIE_TORSO_MELEE = 414,
	ACT_DI_ALYX_HEADCRAB_MELEE = 415,
	ACT_DI_ALYX_ANTLION = 416,
	ACT_DI_ALYX_ZOMBIE_SHOTGUN64 = 417,
	ACT_DI_ALYX_ZOMBIE_SHOTGUN26 = 418,
	ACT_READINESS_RELAXED_TO_STIMULATED = 419,
	ACT_READINESS_RELAXED_TO_STIMULATED_WALK = 420,
	ACT_READINESS_AGITATED_TO_STIMULATED = 421,
	ACT_READINESS_STIMULATED_TO_RELAXED = 422,
	ACT_READINESS_PISTOL_RELAXED_TO_STIMULATED = 423,
	ACT_READINESS_PISTOL_RELAXED_TO_STIMULATED_WALK = 424,
	ACT_READINESS_PISTOL_AGITATED_TO_STIMULATED = 425,
	ACT_READINESS_PISTOL_STIMULATED_TO_RELAXED = 426,
	ACT_IDLE_CARRY = 427,
	ACT_WALK_CARRY = 428,
	ACT_STARTDYING = 429,
	ACT_DYINGLOOP = 430,
	ACT_DYINGTODEAD = 431,
	ACT_RIDE_MANNED_GUN = 432,
	ACT_VM_SPRINT_ENTER = 433,
	ACT_VM_SPRINT_IDLE = 434,
	ACT_VM_SPRINT_LEAVE = 435,
	ACT_FIRE_START = 436,
	ACT_FIRE_LOOP = 437,
	ACT_FIRE_END = 438,
	ACT_CROUCHING_GRENADEIDLE = 439,
	ACT_CROUCHING_GRENADEREADY = 440,
	ACT_CROUCHING_PRIMARYATTACK = 441,
	ACT_OVERLAY_GRENADEIDLE = 442,
	ACT_OVERLAY_GRENADEREADY = 443,
	ACT_OVERLAY_PRIMARYATTACK = 444,
	ACT_OVERLAY_SHIELD_UP = 445,
	ACT_OVERLAY_SHIELD_DOWN = 446,
	ACT_OVERLAY_SHIELD_UP_IDLE = 447,
	ACT_OVERLAY_SHIELD_ATTACK = 448,
	ACT_OVERLAY_SHIELD_KNOCKBACK = 449,
	ACT_SHIELD_UP = 450,
	ACT_SHIELD_DOWN = 451,
	ACT_SHIELD_UP_IDLE = 452,
	ACT_SHIELD_ATTACK = 453,
	ACT_SHIELD_KNOCKBACK = 454,
	ACT_CROUCHING_SHIELD_UP = 455,
	ACT_CROUCHING_SHIELD_DOWN = 456,
	ACT_CROUCHING_SHIELD_UP_IDLE = 457,
	ACT_CROUCHING_SHIELD_ATTACK = 458,
	ACT_CROUCHING_SHIELD_KNOCKBACK = 459,
	ACT_TURNRIGHT45 = 460,
	ACT_TURNLEFT45 = 461,
	ACT_TURN = 462,
	ACT_OBJ_ASSEMBLING = 463,
	ACT_OBJ_DISMANTLING = 464,
	ACT_OBJ_STARTUP = 465,
	ACT_OBJ_RUNNING = 466,
	ACT_OBJ_IDLE = 467,
	ACT_OBJ_PLACING = 468,
	ACT_OBJ_DETERIORATING = 469,
	ACT_OBJ_UPGRADING = 470,
	ACT_DEPLOY = 471,
	ACT_DEPLOY_IDLE = 472,
	ACT_UNDEPLOY = 473,
	ACT_CROSSBOW_DRAW_UNLOADED = 474,
	ACT_GAUSS_SPINUP = 475,
	ACT_GAUSS_SPINCYCLE = 476,
	ACT_VM_PRIMARYATTACK_SILENCED = 477,
	ACT_VM_RELOAD_SILENCED = 478,
	ACT_VM_DRYFIRE_SILENCED = 479,
	ACT_VM_IDLE_SILENCED = 480,
	ACT_VM_DRAW_SILENCED = 481,
	ACT_VM_IDLE_EMPTY_LEFT = 482,
	ACT_VM_DRYFIRE_LEFT = 483,
	ACT_VM_IS_DRAW = 484,
	ACT_VM_IS_HOLSTER = 485,
	ACT_VM_IS_IDLE = 486,
	ACT_VM_IS_PRIMARYATTACK = 487,
	ACT_PLAYER_IDLE_FIRE = 488,
	ACT_PLAYER_CROUCH_FIRE = 489,
	ACT_PLAYER_CROUCH_WALK_FIRE = 490,
	ACT_PLAYER_WALK_FIRE = 491,
	ACT_PLAYER_RUN_FIRE = 492,
	ACT_IDLETORUN = 493,
	ACT_RUNTOIDLE = 494,
	ACT_VM_DRAW_DEPLOYED = 495,
	ACT_HL2MP_IDLE_MELEE = 496,
	ACT_HL2MP_RUN_MELEE = 497,
	ACT_HL2MP_IDLE_CROUCH_MELEE = 498,
	ACT_HL2MP_WALK_CROUCH_MELEE = 499,
	ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE = 500,
	ACT_HL2MP_GESTURE_RELOAD_MELEE = 501,
	ACT_HL2MP_JUMP_MELEE = 502,
	ACT_VM_FIZZLE = 503,
	ACT_MP_STAND_IDLE = 504,
	ACT_MP_CROUCH_IDLE = 505,
	ACT_MP_CROUCH_DEPLOYED_IDLE = 506,
	ACT_MP_CROUCH_DEPLOYED = 507,
	ACT_MP_DEPLOYED_IDLE = 508,
	ACT_MP_RUN = 509,
	ACT_MP_WALK = 510,
	ACT_MP_AIRWALK = 511,
	ACT_MP_CROUCHWALK = 512,
	ACT_MP_SPRINT = 513,
	ACT_MP_JUMP = 514,
	ACT_MP_JUMP_START = 515,
	ACT_MP_JUMP_FLOAT = 516,
	ACT_MP_JUMP_LAND = 517,
	ACT_MP_JUMP_IMPACT_N = 518,
	ACT_MP_JUMP_IMPACT_E = 519,
	ACT_MP_JUMP_IMPACT_W = 520,
	ACT_MP_JUMP_IMPACT_S = 521,
	ACT_MP_JUMP_IMPACT_TOP = 522,
	ACT_MP_DOUBLEJUMP = 523,
	ACT_MP_SWIM = 524,
	ACT_MP_DEPLOYED = 525,
	ACT_MP_SWIM_DEPLOYED = 526,
	ACT_MP_VCD = 527,
	ACT_MP_ATTACK_STAND_PRIMARYFIRE = 528,
	ACT_MP_ATTACK_STAND_PRIMARYFIRE_DEPLOYED = 529,
	ACT_MP_ATTACK_STAND_SECONDARYFIRE = 530,
	ACT_MP_ATTACK_STAND_GRENADE = 531,
	ACT_MP_ATTACK_CROUCH_PRIMARYFIRE = 532,
	ACT_MP_ATTACK_CROUCH_PRIMARYFIRE_DEPLOYED = 533,
	ACT_MP_ATTACK_CROUCH_SECONDARYFIRE = 534,
	ACT_MP_ATTACK_CROUCH_GRENADE = 535,
	ACT_MP_ATTACK_SWIM_PRIMARYFIRE = 536,
	ACT_MP_ATTACK_SWIM_SECONDARYFIRE = 537,
	ACT_MP_ATTACK_SWIM_GRENADE = 538,
	ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE = 539,
	ACT_MP_ATTACK_AIRWALK_SECONDARYFIRE = 540,
	ACT_MP_ATTACK_AIRWALK_GRENADE = 541,
	ACT_MP_RELOAD_STAND = 542,
	ACT_MP_RELOAD_STAND_LOOP = 543,
	ACT_MP_RELOAD_STAND_END = 544,
	ACT_MP_RELOAD_CROUCH = 545,
	ACT_MP_RELOAD_CROUCH_LOOP = 546,
	ACT_MP_RELOAD_CROUCH_END = 547,
	ACT_MP_RELOAD_SWIM = 548,
	ACT_MP_RELOAD_SWIM_LOOP = 549,
	ACT_MP_RELOAD_SWIM_END = 550,
	ACT_MP_RELOAD_AIRWALK = 551,
	ACT_MP_RELOAD_AIRWALK_LOOP = 552,
	ACT_MP_RELOAD_AIRWALK_END = 553,
	ACT_MP_ATTACK_STAND_PREFIRE = 554,
	ACT_MP_ATTACK_STAND_POSTFIRE = 555,
	ACT_MP_ATTACK_STAND_STARTFIRE = 556,
	ACT_MP_ATTACK_CROUCH_PREFIRE = 557,
	ACT_MP_ATTACK_CROUCH_POSTFIRE = 558,
	ACT_MP_ATTACK_SWIM_PREFIRE = 559,
	ACT_MP_ATTACK_SWIM_POSTFIRE = 560,
	ACT_MP_STAND_PRIMARY = 561,
	ACT_MP_CROUCH_PRIMARY = 562,
	ACT_MP_RUN_PRIMARY = 563,
	ACT_MP_WALK_PRIMARY = 564,
	ACT_MP_AIRWALK_PRIMARY = 565,
	ACT_MP_CROUCHWALK_PRIMARY = 566,
	ACT_MP_JUMP_PRIMARY = 567,
	ACT_MP_JUMP_START_PRIMARY = 568,
	ACT_MP_JUMP_FLOAT_PRIMARY = 569,
	ACT_MP_JUMP_LAND_PRIMARY = 570,
	ACT_MP_SWIM_PRIMARY = 571,
	ACT_MP_DEPLOYED_PRIMARY = 572,
	ACT_MP_SWIM_DEPLOYED_PRIMARY = 573,
	ACT_MP_ATTACK_STAND_PRIMARY = 574,
	ACT_MP_ATTACK_STAND_PRIMARY_DEPLOYED = 575,
	ACT_MP_ATTACK_CROUCH_PRIMARY = 576,
	ACT_MP_ATTACK_CROUCH_PRIMARY_DEPLOYED = 577,
	ACT_MP_ATTACK_SWIM_PRIMARY = 578,
	ACT_MP_ATTACK_AIRWALK_PRIMARY = 579,
	ACT_MP_RELOAD_STAND_PRIMARY = 580,
	ACT_MP_RELOAD_STAND_PRIMARY_LOOP = 581,
	ACT_MP_RELOAD_STAND_PRIMARY_END = 582,
	ACT_MP_RELOAD_CROUCH_PRIMARY = 583,
	ACT_MP_RELOAD_CROUCH_PRIMARY_LOOP = 584,
	ACT_MP_RELOAD_CROUCH_PRIMARY_END = 585,
	ACT_MP_RELOAD_SWIM_PRIMARY = 586,
	ACT_MP_RELOAD_SWIM_PRIMARY_LOOP = 587,
	ACT_MP_RELOAD_SWIM_PRIMARY_END = 588,
	ACT_MP_RELOAD_AIRWALK_PRIMARY = 589,
	ACT_MP_RELOAD_AIRWALK_PRIMARY_LOOP = 590,
	ACT_MP_RELOAD_AIRWALK_PRIMARY_END = 591,
	ACT_MP_ATTACK_STAND_GRENADE_PRIMARY = 592,
	ACT_MP_ATTACK_CROUCH_GRENADE_PRIMARY = 593,
	ACT_MP_ATTACK_SWIM_GRENADE_PRIMARY = 594,
	ACT_MP_ATTACK_AIRWALK_GRENADE_PRIMARY = 595,
	ACT_MP_STAND_SECONDARY = 596,
	ACT_MP_CROUCH_SECONDARY = 597,
	ACT_MP_RUN_SECONDARY = 598,
	ACT_MP_WALK_SECONDARY = 599,
	ACT_MP_AIRWALK_SECONDARY = 600,
	ACT_MP_CROUCHWALK_SECONDARY = 601,
	ACT_MP_JUMP_SECONDARY = 602,
	ACT_MP_JUMP_START_SECONDARY = 603,
	ACT_MP_JUMP_FLOAT_SECONDARY = 604,
	ACT_MP_JUMP_LAND_SECONDARY = 605,
	ACT_MP_SWIM_SECONDARY = 606,
	ACT_MP_ATTACK_STAND_SECONDARY = 607,
	ACT_MP_ATTACK_CROUCH_SECONDARY = 608,
	ACT_MP_ATTACK_SWIM_SECONDARY = 609,
	ACT_MP_ATTACK_AIRWALK_SECONDARY = 610,
	ACT_MP_RELOAD_STAND_SECONDARY = 611,
	ACT_MP_RELOAD_STAND_SECONDARY_LOOP = 612,
	ACT_MP_RELOAD_STAND_SECONDARY_END = 613,
	ACT_MP_RELOAD_CROUCH_SECONDARY = 614,
	ACT_MP_RELOAD_CROUCH_SECONDARY_LOOP = 615,
	ACT_MP_RELOAD_CROUCH_SECONDARY_END = 616,
	ACT_MP_RELOAD_SWIM_SECONDARY = 617,
	ACT_MP_RELOAD_SWIM_SECONDARY_LOOP = 618,
	ACT_MP_RELOAD_SWIM_SECONDARY_END = 619,
	ACT_MP_RELOAD_AIRWALK_SECONDARY = 620,
	ACT_MP_RELOAD_AIRWALK_SECONDARY_LOOP = 621,
	ACT_MP_RELOAD_AIRWALK_SECONDARY_END = 622,
	ACT_MP_ATTACK_STAND_GRENADE_SECONDARY = 623,
	ACT_MP_ATTACK_CROUCH_GRENADE_SECONDARY = 624,
	ACT_MP_ATTACK_SWIM_GRENADE_SECONDARY = 625,
	ACT_MP_ATTACK_AIRWALK_GRENADE_SECONDARY = 626,
	ACT_MP_STAND_MELEE = 627,
	ACT_MP_CROUCH_MELEE = 628,
	ACT_MP_RUN_MELEE = 629,
	ACT_MP_WALK_MELEE = 630,
	ACT_MP_AIRWALK_MELEE = 631,
	ACT_MP_CROUCHWALK_MELEE = 632,
	ACT_MP_JUMP_MELEE = 633,
	ACT_MP_JUMP_START_MELEE = 634,
	ACT_MP_JUMP_FLOAT_MELEE = 635,
	ACT_MP_JUMP_LAND_MELEE = 636,
	ACT_MP_SWIM_MELEE = 637,
	ACT_MP_ATTACK_STAND_MELEE = 638,
	ACT_MP_ATTACK_STAND_MELEE_SECONDARY = 639,
	ACT_MP_ATTACK_CROUCH_MELEE = 640,
	ACT_MP_ATTACK_CROUCH_MELEE_SECONDARY = 641,
	ACT_MP_ATTACK_SWIM_MELEE = 642,
	ACT_MP_ATTACK_AIRWALK_MELEE = 643,
	ACT_MP_ATTACK_STAND_GRENADE_MELEE = 644,
	ACT_MP_ATTACK_CROUCH_GRENADE_MELEE = 645,
	ACT_MP_ATTACK_SWIM_GRENADE_MELEE = 646,
	ACT_MP_ATTACK_AIRWALK_GRENADE_MELEE = 647,
	ACT_MP_STAND_ITEM1 = 648,
	ACT_MP_CROUCH_ITEM1 = 649,
	ACT_MP_RUN_ITEM1 = 650,
	ACT_MP_WALK_ITEM1 = 651,
	ACT_MP_AIRWALK_ITEM1 = 652,
	ACT_MP_CROUCHWALK_ITEM1 = 653,
	ACT_MP_JUMP_ITEM1 = 654,
	ACT_MP_JUMP_START_ITEM1 = 655,
	ACT_MP_JUMP_FLOAT_ITEM1 = 656,
	ACT_MP_JUMP_LAND_ITEM1 = 657,
	ACT_MP_SWIM_ITEM1 = 658,
	ACT_MP_ATTACK_STAND_ITEM1 = 659,
	ACT_MP_ATTACK_STAND_ITEM1_SECONDARY = 660,
	ACT_MP_ATTACK_CROUCH_ITEM1 = 661,
	ACT_MP_ATTACK_CROUCH_ITEM1_SECONDARY = 662,
	ACT_MP_ATTACK_SWIM_ITEM1 = 663,
	ACT_MP_ATTACK_AIRWALK_ITEM1 = 664,
	ACT_MP_STAND_ITEM2 = 665,
	ACT_MP_CROUCH_ITEM2 = 666,
	ACT_MP_RUN_ITEM2 = 667,
	ACT_MP_WALK_ITEM2 = 668,
	ACT_MP_AIRWALK_ITEM2 = 669,
	ACT_MP_CROUCHWALK_ITEM2 = 670,
	ACT_MP_JUMP_ITEM2 = 671,
	ACT_MP_JUMP_START_ITEM2 = 672,
	ACT_MP_JUMP_FLOAT_ITEM2 = 673,
	ACT_MP_JUMP_LAND_ITEM2 = 674,
	ACT_MP_SWIM_ITEM2 = 675,
	ACT_MP_ATTACK_STAND_ITEM2 = 676,
	ACT_MP_ATTACK_STAND_ITEM2_SECONDARY = 677,
	ACT_MP_ATTACK_CROUCH_ITEM2 = 678,
	ACT_MP_ATTACK_CROUCH_ITEM2_SECONDARY = 679,
	ACT_MP_ATTACK_SWIM_ITEM2 = 680,
	ACT_MP_ATTACK_AIRWALK_ITEM2 = 681,
	ACT_MP_GESTURE_FLINCH = 682,
	ACT_MP_GESTURE_FLINCH_PRIMARY = 683,
	ACT_MP_GESTURE_FLINCH_SECONDARY = 684,
	ACT_MP_GESTURE_FLINCH_MELEE = 685,
	ACT_MP_GESTURE_FLINCH_ITEM1 = 686,
	ACT_MP_GESTURE_FLINCH_ITEM2 = 687,
	ACT_MP_GESTURE_FLINCH_HEAD = 688,
	ACT_MP_GESTURE_FLINCH_CHEST = 689,
	ACT_MP_GESTURE_FLINCH_STOMACH = 690,
	ACT_MP_GESTURE_FLINCH_LEFTARM = 691,
	ACT_MP_GESTURE_FLINCH_RIGHTARM = 692,
	ACT_MP_GESTURE_FLINCH_LEFTLEG = 693,
	ACT_MP_GESTURE_FLINCH_RIGHTLEG = 694,
	ACT_MP_GRENADE1_DRAW = 695,
	ACT_MP_GRENADE1_IDLE = 696,
	ACT_MP_GRENADE1_ATTACK = 697,
	ACT_MP_GRENADE2_DRAW = 698,
	ACT_MP_GRENADE2_IDLE = 699,
	ACT_MP_GRENADE2_ATTACK = 700,
	ACT_MP_PRIMARY_GRENADE1_DRAW = 701,
	ACT_MP_PRIMARY_GRENADE1_IDLE = 702,
	ACT_MP_PRIMARY_GRENADE1_ATTACK = 703,
	ACT_MP_PRIMARY_GRENADE2_DRAW = 704,
	ACT_MP_PRIMARY_GRENADE2_IDLE = 705,
	ACT_MP_PRIMARY_GRENADE2_ATTACK = 706,
	ACT_MP_SECONDARY_GRENADE1_DRAW = 707,
	ACT_MP_SECONDARY_GRENADE1_IDLE = 708,
	ACT_MP_SECONDARY_GRENADE1_ATTACK = 709,
	ACT_MP_SECONDARY_GRENADE2_DRAW = 710,
	ACT_MP_SECONDARY_GRENADE2_IDLE = 711,
	ACT_MP_SECONDARY_GRENADE2_ATTACK = 712,
	ACT_MP_MELEE_GRENADE1_DRAW = 713,
	ACT_MP_MELEE_GRENADE1_IDLE = 714,
	ACT_MP_MELEE_GRENADE1_ATTACK = 715,
	ACT_MP_MELEE_GRENADE2_DRAW = 716,
	ACT_MP_MELEE_GRENADE2_IDLE = 717,
	ACT_MP_MELEE_GRENADE2_ATTACK = 718,
	ACT_MP_ITEM1_GRENADE1_DRAW = 719,
	ACT_MP_ITEM1_GRENADE1_IDLE = 720,
	ACT_MP_ITEM1_GRENADE1_ATTACK = 721,
	ACT_MP_ITEM1_GRENADE2_DRAW = 722,
	ACT_MP_ITEM1_GRENADE2_IDLE = 723,
	ACT_MP_ITEM1_GRENADE2_ATTACK = 724,
	ACT_MP_ITEM2_GRENADE1_DRAW = 725,
	ACT_MP_ITEM2_GRENADE1_IDLE = 726,
	ACT_MP_ITEM2_GRENADE1_ATTACK = 727,
	ACT_MP_ITEM2_GRENADE2_DRAW = 728,
	ACT_MP_ITEM2_GRENADE2_IDLE = 729,
	ACT_MP_ITEM2_GRENADE2_ATTACK = 730,
	ACT_MP_STAND_BUILDING = 731,
	ACT_MP_CROUCH_BUILDING = 732,
	ACT_MP_RUN_BUILDING = 733,
	ACT_MP_WALK_BUILDING = 734,
	ACT_MP_AIRWALK_BUILDING = 735,
	ACT_MP_CROUCHWALK_BUILDING = 736,
	ACT_MP_JUMP_BUILDING = 737,
	ACT_MP_JUMP_START_BUILDING = 738,
	ACT_MP_JUMP_FLOAT_BUILDING = 739,
	ACT_MP_JUMP_LAND_BUILDING = 740,
	ACT_MP_SWIM_BUILDING = 741,
	ACT_MP_ATTACK_STAND_BUILDING = 742,
	ACT_MP_ATTACK_CROUCH_BUILDING = 743,
	ACT_MP_ATTACK_SWIM_BUILDING = 744,
	ACT_MP_ATTACK_AIRWALK_BUILDING = 745,
	ACT_MP_ATTACK_STAND_GRENADE_BUILDING = 746,
	ACT_MP_ATTACK_CROUCH_GRENADE_BUILDING = 747,
	ACT_MP_ATTACK_SWIM_GRENADE_BUILDING = 748,
	ACT_MP_ATTACK_AIRWALK_GRENADE_BUILDING = 749,
	ACT_MP_STAND_PDA = 750,
	ACT_MP_CROUCH_PDA = 751,
	ACT_MP_RUN_PDA = 752,
	ACT_MP_WALK_PDA = 753,
	ACT_MP_AIRWALK_PDA = 754,
	ACT_MP_CROUCHWALK_PDA = 755,
	ACT_MP_JUMP_PDA = 756,
	ACT_MP_JUMP_START_PDA = 757,
	ACT_MP_JUMP_FLOAT_PDA = 758,
	ACT_MP_JUMP_LAND_PDA = 759,
	ACT_MP_SWIM_PDA = 760,
	ACT_MP_ATTACK_STAND_PDA = 761,
	ACT_MP_ATTACK_SWIM_PDA = 762,
	ACT_MP_GESTURE_VC_HANDMOUTH = 763,
	ACT_MP_GESTURE_VC_FINGERPOINT = 764,
	ACT_MP_GESTURE_VC_FISTPUMP = 765,
	ACT_MP_GESTURE_VC_THUMBSUP = 766,
	ACT_MP_GESTURE_VC_NODYES = 767,
	ACT_MP_GESTURE_VC_NODNO = 768,
	ACT_MP_GESTURE_VC_HANDMOUTH_PRIMARY = 769,
	ACT_MP_GESTURE_VC_FINGERPOINT_PRIMARY = 770,
	ACT_MP_GESTURE_VC_FISTPUMP_PRIMARY = 771,
	ACT_MP_GESTURE_VC_THUMBSUP_PRIMARY = 772,
	ACT_MP_GESTURE_VC_NODYES_PRIMARY = 773,
	ACT_MP_GESTURE_VC_NODNO_PRIMARY = 774,
	ACT_MP_GESTURE_VC_HANDMOUTH_SECONDARY = 775,
	ACT_MP_GESTURE_VC_FINGERPOINT_SECONDARY = 776,
	ACT_MP_GESTURE_VC_FISTPUMP_SECONDARY = 777,
	ACT_MP_GESTURE_VC_THUMBSUP_SECONDARY = 778,
	ACT_MP_GESTURE_VC_NODYES_SECONDARY = 779,
	ACT_MP_GESTURE_VC_NODNO_SECONDARY = 780,
	ACT_MP_GESTURE_VC_HANDMOUTH_MELEE = 781,
	ACT_MP_GESTURE_VC_FINGERPOINT_MELEE = 782,
	ACT_MP_GESTURE_VC_FISTPUMP_MELEE = 783,
	ACT_MP_GESTURE_VC_THUMBSUP_MELEE = 784,
	ACT_MP_GESTURE_VC_NODYES_MELEE = 785,
	ACT_MP_GESTURE_VC_NODNO_MELEE = 786,
	ACT_MP_GESTURE_VC_HANDMOUTH_ITEM1 = 787,
	ACT_MP_GESTURE_VC_FINGERPOINT_ITEM1 = 788,
	ACT_MP_GESTURE_VC_FISTPUMP_ITEM1 = 789,
	ACT_MP_GESTURE_VC_THUMBSUP_ITEM1 = 790,
	ACT_MP_GESTURE_VC_NODYES_ITEM1 = 791,
	ACT_MP_GESTURE_VC_NODNO_ITEM1 = 792,
	ACT_MP_GESTURE_VC_HANDMOUTH_ITEM2 = 793,
	ACT_MP_GESTURE_VC_FINGERPOINT_ITEM2 = 794,
	ACT_MP_GESTURE_VC_FISTPUMP_ITEM2 = 795,
	ACT_MP_GESTURE_VC_THUMBSUP_ITEM2 = 796,
	ACT_MP_GESTURE_VC_NODYES_ITEM2 = 797,
	ACT_MP_GESTURE_VC_NODNO_ITEM2 = 798,
	ACT_MP_GESTURE_VC_HANDMOUTH_BUILDING = 799,
	ACT_MP_GESTURE_VC_FINGERPOINT_BUILDING = 800,
	ACT_MP_GESTURE_VC_FISTPUMP_BUILDING = 801,
	ACT_MP_GESTURE_VC_THUMBSUP_BUILDING = 802,
	ACT_MP_GESTURE_VC_NODYES_BUILDING = 803,
	ACT_MP_GESTURE_VC_NODNO_BUILDING = 804,
	ACT_MP_GESTURE_VC_HANDMOUTH_PDA = 805,
	ACT_MP_GESTURE_VC_FINGERPOINT_PDA = 806,
	ACT_MP_GESTURE_VC_FISTPUMP_PDA = 807,
	ACT_MP_GESTURE_VC_THUMBSUP_PDA = 808,
	ACT_MP_GESTURE_VC_NODYES_PDA = 809,
	ACT_MP_GESTURE_VC_NODNO_PDA = 810,
	ACT_VM_UNUSABLE = 811,
	ACT_VM_UNUSABLE_TO_USABLE = 812,
	ACT_VM_USABLE_TO_UNUSABLE = 813,
	ACT_PRIMARY_VM_DRAW = 814,
	ACT_PRIMARY_VM_HOLSTER = 815,
	ACT_PRIMARY_VM_IDLE = 816,
	ACT_PRIMARY_VM_PULLBACK = 817,
	ACT_PRIMARY_VM_PRIMARYATTACK = 818,
	ACT_PRIMARY_VM_SECONDARYATTACK = 819,
	ACT_PRIMARY_VM_RELOAD = 820,
	ACT_PRIMARY_VM_DRYFIRE = 821,
	ACT_PRIMARY_VM_IDLE_TO_LOWERED = 822,
	ACT_PRIMARY_VM_IDLE_LOWERED = 823,
	ACT_PRIMARY_VM_LOWERED_TO_IDLE = 824,
	ACT_SECONDARY_VM_DRAW = 825,
	ACT_SECONDARY_VM_HOLSTER = 826,
	ACT_SECONDARY_VM_IDLE = 827,
	ACT_SECONDARY_VM_PULLBACK = 828,
	ACT_SECONDARY_VM_PRIMARYATTACK = 829,
	ACT_SECONDARY_VM_SECONDARYATTACK = 830,
	ACT_SECONDARY_VM_RELOAD = 831,
	ACT_SECONDARY_VM_DRYFIRE = 832,
	ACT_SECONDARY_VM_IDLE_TO_LOWERED = 833,
	ACT_SECONDARY_VM_IDLE_LOWERED = 834,
	ACT_SECONDARY_VM_LOWERED_TO_IDLE = 835,
	ACT_MELEE_VM_DRAW = 836,
	ACT_MELEE_VM_HOLSTER = 837,
	ACT_MELEE_VM_IDLE = 838,
	ACT_MELEE_VM_PULLBACK = 839,
	ACT_MELEE_VM_PRIMARYATTACK = 840,
	ACT_MELEE_VM_SECONDARYATTACK = 841,
	ACT_MELEE_VM_RELOAD = 842,
	ACT_MELEE_VM_DRYFIRE = 843,
	ACT_MELEE_VM_IDLE_TO_LOWERED = 844,
	ACT_MELEE_VM_IDLE_LOWERED = 845,
	ACT_MELEE_VM_LOWERED_TO_IDLE = 846,
	ACT_PDA_VM_DRAW = 847,
	ACT_PDA_VM_HOLSTER = 848,
	ACT_PDA_VM_IDLE = 849,
	ACT_PDA_VM_PULLBACK = 850,
	ACT_PDA_VM_PRIMARYATTACK = 851,
	ACT_PDA_VM_SECONDARYATTACK = 852,
	ACT_PDA_VM_RELOAD = 853,
	ACT_PDA_VM_DRYFIRE = 854,
	ACT_PDA_VM_IDLE_TO_LOWERED = 855,
	ACT_PDA_VM_IDLE_LOWERED = 856,
	ACT_PDA_VM_LOWERED_TO_IDLE = 857,
	ACT_ITEM1_VM_DRAW = 858,
	ACT_ITEM1_VM_HOLSTER = 859,
	ACT_ITEM1_VM_IDLE = 860,
	ACT_ITEM1_VM_PULLBACK = 861,
	ACT_ITEM1_VM_PRIMARYATTACK = 862,
	ACT_ITEM1_VM_SECONDARYATTACK = 863,
	ACT_ITEM1_VM_RELOAD = 864,
	ACT_ITEM1_VM_DRYFIRE = 865,
	ACT_ITEM1_VM_IDLE_TO_LOWERED = 866,
	ACT_ITEM1_VM_IDLE_LOWERED = 867,
	ACT_ITEM1_VM_LOWERED_TO_IDLE = 868,
	ACT_ITEM2_VM_DRAW = 869,
	ACT_ITEM2_VM_HOLSTER = 870,
	ACT_ITEM2_VM_IDLE = 871,
	ACT_ITEM2_VM_PULLBACK = 872,
	ACT_ITEM2_VM_PRIMARYATTACK = 873,
	ACT_ITEM2_VM_SECONDARYATTACK = 874,
	ACT_ITEM2_VM_RELOAD = 875,
	ACT_ITEM2_VM_DRYFIRE = 876,
	ACT_ITEM2_VM_IDLE_TO_LOWERED = 877,
	ACT_ITEM2_VM_IDLE_LOWERED = 878,
	ACT_ITEM2_VM_LOWERED_TO_IDLE = 879,
	ACT_RELOAD_SUCCEED = 880,
	ACT_RELOAD_FAIL = 881,
	ACT_WALK_AIM_AUTOGUN = 882,
	ACT_RUN_AIM_AUTOGUN = 883,
	ACT_IDLE_AUTOGUN = 884,
	ACT_IDLE_AIM_AUTOGUN = 885,
	ACT_RELOAD_AUTOGUN = 886,
	ACT_CROUCH_IDLE_AUTOGUN = 887,
	ACT_RANGE_ATTACK_AUTOGUN = 888,
	ACT_JUMP_AUTOGUN = 889,
	ACT_IDLE_AIM_PISTOL = 890,
	ACT_WALK_AIM_DUAL = 891,
	ACT_RUN_AIM_DUAL = 892,
	ACT_IDLE_DUAL = 893,
	ACT_IDLE_AIM_DUAL = 894,
	ACT_RELOAD_DUAL = 895,
	ACT_CROUCH_IDLE_DUAL = 896,
	ACT_RANGE_ATTACK_DUAL = 897,
	ACT_JUMP_DUAL = 898,
	ACT_IDLE_SHOTGUN = 899,
	ACT_IDLE_AIM_SHOTGUN = 900,
	ACT_CROUCH_IDLE_SHOTGUN = 901,
	ACT_JUMP_SHOTGUN = 902,
	ACT_IDLE_AIM_RIFLE = 903,
	ACT_RELOAD_RIFLE = 904,
	ACT_CROUCH_IDLE_RIFLE = 905,
	ACT_RANGE_ATTACK_RIFLE = 906,
	ACT_JUMP_RIFLE = 907,
	ACT_SLEEP = 908,
	ACT_WAKE = 909,
	ACT_FLICK_LEFT = 910,
	ACT_FLICK_LEFT_MIDDLE = 911,
	ACT_FLICK_RIGHT_MIDDLE = 912,
	ACT_FLICK_RIGHT = 913,
	ACT_SPINAROUND = 914,
	ACT_PREP_TO_FIRE = 915,
	ACT_FIRE = 916,
	ACT_FIRE_RECOVER = 917,
	ACT_SPRAY = 918,
	ACT_PREP_EXPLODE = 919,
	ACT_EXPLODE = 920,
	ACT_DOTA_IDLE = 921,
	ACT_DOTA_RUN = 922,
	ACT_DOTA_ATTACK = 923,
	ACT_DOTA_ATTACK_EVENT = 924,
	ACT_DOTA_DIE = 925,
	ACT_DOTA_FLINCH = 926,
	ACT_DOTA_DISABLED = 927,
	ACT_DOTA_CAST_ABILITY_1 = 928,
	ACT_DOTA_CAST_ABILITY_2 = 929,
	ACT_DOTA_CAST_ABILITY_3 = 930,
	ACT_DOTA_CAST_ABILITY_4 = 931,
	ACT_DOTA_OVERRIDE_ABILITY_1 = 932,
	ACT_DOTA_OVERRIDE_ABILITY_2 = 933,
	ACT_DOTA_OVERRIDE_ABILITY_3 = 934,
	ACT_DOTA_OVERRIDE_ABILITY_4 = 935,
	ACT_DOTA_CHANNEL_ABILITY_1 = 936,
	ACT_DOTA_CHANNEL_ABILITY_2 = 937,
	ACT_DOTA_CHANNEL_ABILITY_3 = 938,
	ACT_DOTA_CHANNEL_ABILITY_4 = 939,
	ACT_DOTA_CHANNEL_END_ABILITY_1 = 940,
	ACT_DOTA_CHANNEL_END_ABILITY_2 = 941,
	ACT_DOTA_CHANNEL_END_ABILITY_3 = 942,
	ACT_DOTA_CHANNEL_END_ABILITY_4 = 943,
	ACT_MP_RUN_SPEEDPAINT = 944,
	ACT_MP_LONG_FALL = 945,
	ACT_MP_TRACTORBEAM_FLOAT = 946,
	ACT_MP_DEATH_CRUSH = 947,
	ACT_MP_RUN_SPEEDPAINT_PRIMARY = 948,
	ACT_MP_DROWNING_PRIMARY = 949,
	ACT_MP_LONG_FALL_PRIMARY = 950,
	ACT_MP_TRACTORBEAM_FLOAT_PRIMARY = 951,
	ACT_MP_DEATH_CRUSH_PRIMARY = 952,
	ACT_DIE_STAND = 953,
	ACT_DIE_STAND_HEADSHOT = 954,
	ACT_DIE_CROUCH = 955,
	ACT_DIE_CROUCH_HEADSHOT = 956,
	ACT_CSGO_NULL = 957,
	ACT_CSGO_DEFUSE = 958,
	ACT_CSGO_DEFUSE_WITH_KIT = 959,
	ACT_CSGO_FLASHBANG_REACTION = 960,
	ACT_CSGO_FIRE_PRIMARY = 961,
	ACT_CSGO_FIRE_PRIMARY_OPT_1 = 962,
	ACT_CSGO_FIRE_PRIMARY_OPT_2 = 963,
	ACT_CSGO_FIRE_SECONDARY = 964,
	ACT_CSGO_FIRE_SECONDARY_OPT_1 = 965,
	ACT_CSGO_FIRE_SECONDARY_OPT_2 = 966,
	ACT_CSGO_RELOAD = 967,
	ACT_CSGO_RELOAD_START = 968,
	ACT_CSGO_RELOAD_LOOP = 969,
	ACT_CSGO_RELOAD_END = 970,
	ACT_CSGO_OPERATE = 971,
	ACT_CSGO_DEPLOY = 972,
	ACT_CSGO_CATCH = 973,
	ACT_CSGO_SILENCER_DETACH = 974,
	ACT_CSGO_SILENCER_ATTACH = 975,
	ACT_CSGO_TWITCH = 976,
	ACT_CSGO_TWITCH_BUYZONE = 977,
	ACT_CSGO_PLANT_BOMB = 978,
	ACT_CSGO_IDLE_TURN_BALANCEADJUST = 979,
	ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING = 980,
	ACT_CSGO_ALIVE_LOOP = 981,
	ACT_CSGO_FLINCH = 982,
	ACT_CSGO_FLINCH_HEAD = 983,
	ACT_CSGO_FLINCH_MOLOTOV = 984,
	ACT_CSGO_JUMP = 985,
	ACT_CSGO_FALL = 986,
	ACT_CSGO_CLIMB_LADDER = 987,
	ACT_CSGO_LAND_LIGHT = 988,
	ACT_CSGO_LAND_HEAVY = 989,
	ACT_CSGO_EXIT_LADDER_TOP = 990,
	ACT_CSGO_EXIT_LADDER_BOTTOM = 991,
	ACT_CSGO_PARACHUTE = 992,
	ACT_CSGO_UIPLAYER_IDLE = 993,
	ACT_CSGO_UIPLAYER_WALKUP = 994,
	ACT_CSGO_UIPLAYER_CELEBRATE = 995,
	ACT_CSGO_UIPLAYER_CONFIRM = 996,
	ACT_CSGO_UIPLAYER_BUYMENU = 997,
	ACT_CSGO_UIPLAYER_PATCH = 998,
	ACTIVITY_LIST_MAX
};


#define NUM_AIMSEQUENCE_LAYERS 2 // Then it uses layers 2 and 3 to blend in the weapon run/walk/crouchwalk animation.

enum ANIMATION_LAYERS
{
	MAIN_IDLE_SEQUENCE_LAYER = 0, //0	 For 8-way blended models, this layer blends an idle on top of the run/walk animation to simulate a 9-way blend.
	//	 For 9-way blended models, we don't use this layer.
	AIMSEQUENCE_LAYER1, //1	 Aim sequence uses layers 0 and 1 for the weapon idle animation (needs 2 layers so it can blend).
	AIMSEQUENCE_LAYER2, //2
	LOWERBODY_LAYER, //3
	JUMPING_LAYER, //4
	LANDING_LAYER, //5
	FEET_LAYER, //6
	SILENCERCHANGESEQUENCE_LAYER, //7
	WHOLEBODYACTION_LAYER, //8
	FLASHEDSEQUENCE_LAYER, //9
	FLINCHSEQUENCE_LAYER, //10
	IDLE_LAYER, //11
	LEAN_LAYER, //12
	UNUSED1, //13
	UNUSED2, //14
};

class IClientUnknown;
class IClientNetworkable;
class IClientRenderable;
class CUtlSymbol;

INetChannelInfo* GetPlayerNetInfoServer(int entindex);
struct mstudioseqdesc_t;
enum DataUpdateType_t;

class CBaseEntity
{
public:
	char __pad[0x64];
	int index;

	IClientUnknown* GetClientUnknown() const;
	IClientNetworkable* GetClientNetworkable(); //+8
	IClientRenderable* GetClientRenderable(); //+4

	static void SetAbsQueriesValid(bool bValid);
	static bool IsAbsQueriesValid(void);

	// Enable/disable abs recomputations on a stack.
	static void PushEnableAbsRecomputations(bool bEnable);
	static void PopEnableAbsRecomputations();

	// This requires the abs recomputation stack to be empty and just sets the global state.
	// It should only be used at the scope of the frame loop.
	static void EnableAbsRecomputations(bool bEnable);
	static bool IsAbsRecomputationsEnabled(void);

	void PreDataUpdate(DataUpdateType_t updateType);
	void PostDataUpdate(DataUpdateType_t updateType);
	void OnDataChanged(DataUpdateType_t type);
	void OnPreDataChanged(DataUpdateType_t type);
	bool LockBones();
	void UnlockBones();
	int GetForceBone();
	void SetForceBone(int bone);
	QAngle GetAngleFromHitbox(int hitboxid);
	void GetDirectionFromHitbox(Vector* vForward, Vector* vRight, Vector* vUp, int hitboxid);
	CPlayerrecord* ToPlayerRecord();
	bool IsStrafing();
	void SetIsStrafing(bool strafing);
	int GetHealth();
	void SetHealth(int health);
	int GetTeam();
	bool IsSpectating();
	eEntityType GetEntityType();
	int GetFlags();
	void SetFlags(int flags);
	void RemoveFlag(int flag);
	void AddFlag(int flag);
	int HasFlag(int flag);
	int IsOnGround();
	int IsInAir();
	int HasEFlag(int flag);
	int GetEFlags();
	void SetEFlags(int flags);
	void AddEFlag(int flag);
	void RemoveEFlag(int flag);
	bool IsScoped();
	int GetTickBase();
	float GetThirdPersonRecoil();
	void SetThirdPersonRecoil(float recoil);
	void SetTickBase(int base);
	int GetMoney();
	void Simulate();
	int GetShotsFired();
	void SetMoveType(int type);
	int GetMoveType();
	int GetModelIndex();
	void SetModelIndex(int index);
	int GetHitboxSet();
	int GetHitboxSetServer();
	void SetHitboxSet(int set);
	int GetUserID();
	int GetArmor();
	void SetArmor(int armor);
	int GetCollisionGroup();
	void SetCollisionGroup(int group);
	unsigned int PhysicsSolidMaskForEntity();
	CBaseEntity* GetOwner();
	void SetOwnerHandle(EHANDLE handle);
	int GetGlowIndex();
	bool GetDeadFlag();
	void SetDeadFlag(bool flag);
	int GetLifeState();
	void SetLifeState(int state);
	BOOL GetAlive();
	BOOLEAN GetAliveServer();
	void CalcAbsolutePosition();
	void CalcAbsolutePositionServer();
	bool GetDormant();
	void SetDormant(bool dormant);
	void SetDormantVMT(bool dormant);
	int GetvphysicsCollisionState();
	void SetvphysicsCollisionState(int state);
	bool GetImmune();
	BOOLEAN IsBroken();
	BOOLEAN HasHelmet();
	void SetHasHelmet(BOOLEAN helmet);
	BOOLEAN HasDefuseKit();
	BOOLEAN IsDefusing();
	BOOLEAN IsFlashed();
	void SetMoveCollide(MoveCollide_t c);
	MoveCollide_t GetMoveCollide();
	float GetFlashDuration();
	void SetFlashDuration(float dur);
	void SetFlashMaxAlpha(float a);
	float GetBombTimer();
	bool GetBombDefused();
	QAngle* GetViewPunchAdr();
	QAngle GetViewPunch();
	void SetViewPunch(QAngle& punch);
	QAngle GetPunch();
	QAngle* GetPunchAdr();
	void GetPunchVMT(QAngle& dest);
	void SetPunchVMT(QAngle& punch);
	void SetPunch(QAngle& punch);
	Vector GetPunchVel();
	void SetPunchVel(Vector& vel);
	QAngle GetEyeAngles();
	QAngle GetEyeAnglesServer();
	QAngle* EyeAngles();
	void SetEyeAngles(QAngle& angles);
	Vector* GetLocalOriginVMT();
	QAngle* GetLocalAnglesVMT();
	void SetLocalOrigin(const Vector& origin);
	void SetLocalOriginDirect(const Vector& origin);
	Vector GetLocalOriginDirect();
	Vector GetLocalOrigin();
	void SetLocalAngles(const QAngle& angles);
	QAngle& GetLocalAnglesDirect();
	void SetLocalAnglesDirect(QAngle& angles);
	float GetSpawnTime();
	Vector GetNetworkOrigin();
	Vector GetOldOrigin();
	void SetOldOrigin(Vector& origin);
	matrix3x4_t* GetCoordinateFrame();
	void SetNetworkOrigin(Vector& origin);
	void SetAbsOrigin(const Vector& origin);
	QAngle GetAbsAngles();
	QAngle& GetAbsAnglesDirect();
	void SetAbsAnglesDirect(QAngle& angles);
	QAngle GetAbsAnglesServer();
	Vector WorldSpaceCenter();
	Vector GetEyePosition();
	Vector Weapon_ShootPosition();
	void Weapon_ShootPosition(Vector& dest);
	CBaseEntity* GetViewEntity();
	int GetSplitScreenPlayerSlot();
	void CacheVehicleView();
	Vector EyePosition();
	void EyePosition(Vector& dest);
	void Weapon_ShootPosition_Base(Vector& dest);
	static bool IsLocalPlayer(CBaseEntity* pOther);
	bool IsLocalPlayer();
	SolidType_t GetSolid();
	BOOLEAN ShouldCollide(int collisionGroup, int contentsMask);
	BOOLEAN IsTransparent();
	int LookupBone(char* name);
	void GetBoneTransform(int iBone, matrix3x4_t& pBoneToWorld);
	void GetBonePosition(int bone, Vector& destorigin);
	bool IsBoneCacheValid();
	const matrix3x4a_t& GetBone(int iBone);
	void GetCachedBoneMatrix(int boneIndex, matrix3x4_t& out);
	void GetBonePositionRebuilt(int iBone, Vector& origin, QAngle& angles);
	Vector GetBonePosition(int HitboxID);
	bool GetBoneTransformed(int HitboxID, matrix3x4_t *rotatedmatrix = nullptr, float* radius = nullptr, Vector* minsmaxs = nullptr, Vector* worldminsmaxs = nullptr);
	void GetBoneTransformed(matrix3x4_t* srcMatrix, mstudiobbox_t *hitbox, matrix3x4_t *rotatedmatrix = nullptr, float* radius = nullptr, Vector* minsmaxs = nullptr, Vector* worldminsmaxs = nullptr);
	Vector GetBonePositionCachedOnly(int HitboxID, matrix3x4_t* matrixes);
	void GetBonePosition(int iBone, Vector& origin, QAngle& angles);
	void GetVectors(Vector* pForward, Vector* pRight = nullptr, Vector* pUp = nullptr);
	bool IsViewModel();
	bool IsInFiringActivity();
	int GetLastSetupBonesFrameCount();
	void SetLastSetupBonesFrameCount(int framecount);
	bool IsBoneAccessAllowed();
	int GetPreviousBoneMask();
	void SetPreviousBoneMask(int mask);
	int GetAccumulatedBoneMask();
	void SetAccumulatedBoneMask(int mask);
	float LastBoneChangedTime();
	float GetLastBoneSetupTime();
	void SetLastBoneSetupTime(float time);
	DWORD GetIK();
	void SetIK(DWORD ik);
	unsigned char GetEntClientFlags();
	void SetEntClientFlags(unsigned char flags);
	void AddEntClientFlag(unsigned char flag);
	BOOLEAN IsToolRecording();
	BOOLEAN GetPredictable();
	CThreadFastMutex* GetBoneSetupLock();
	matrix3x4_t *GetBoneCache_Server(float curTime);
	void BuildMatricesWithBoneMerge(const CStudioHdr *pStudioHdr,
		const QAngle& angles,
		const Vector& origin,
		const Vector pos[MAXSTUDIOBONES],
		const Quaternion q[MAXSTUDIOBONES],
		matrix3x4_t bonetoworld[MAXSTUDIOBONES],
		CBaseEntity *pParent,
		matrix3x4_t *pParentCache);
	void GetSkeleton(CStudioHdr* studioHdr, Vector* pos, Quaternion* q, int boneMask);
	bool SetupBones_Server(int boneMask, float currentTime);
	bool SetupBonesRebuilt(matrix3x4_t* pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime);
	bool SetupBones(matrix3x4_t* pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime);
	int SelectWeightedSequenceFromModifiers(int activity, CUtlSymbol *pActivityModifiers, int iModifierCount);
	int SelectWeightedSequenceFromModifiers(int m_pActivityToSequence, CStudioHdr *pstudiohdr, int activity, CUtlSymbol *pActivityModifiers, int iModifierCount);
	CBaseHandle GetRefEHandle();
	unsigned long GetRefEHandleDirect();
	void SetRefEHandle(CBaseHandle handle);
	void SetRefEHandle(unsigned long handle);
	const Vector& GetVelocity();
	void EstimateAbsVelocity(const Vector& vel);
	void SetVelocity(const Vector &velocity);
	Vector* GetAbsVelocity();
	void GetAbsVelocity(Vector& vel);
	Vector GetAbsVelocityDirect();
	void SetAbsVelocityDirect(Vector& vel);
	void SetAbsVelocity(Vector& velocity);
	void CalcAbsoluteVelocity();
	Vector GetBaseVelocity();
	void SetBaseVelocity(Vector& velocity);
	ICollideable* GetCollideable();
	void GetPlayerInfo(player_info_t* dest);
	Vector GetVehicleViewOrigin();
	void LockStudioHdr();
	model_t* GetModel();
	CStudioHdr* GetModelPtr();
	CStudioHdr* GetStudioHdr();
	mstudioseqdesc_t* pSeqdesc(int seq);
	void SetModel(model_t* mod);
	std::string GetName();
	void GetSteamID(char* dest);
	CBaseCombatWeapon* GetWeapon();
	void SetWeaponHandle(EHANDLE handle);
	void Precache();
	void Spawn();
	EHANDLE GetWeaponHandle();
	ClientClass* GetClientClass();
	CBaseAnimating* GetBaseAnimating();
	void InvalidateBoneCache();
	HANDLE GetObserverTargetDirect();
	CBaseEntity* GetObserverTarget();
	BOOLEAN IsPlayer();
	CUserCmd* GetLastUserCommand();
	QAngle& GetAngleRotation();
	void SetAngleRotation(QAngle& rot);
	QAngle& GetLocalAngles();
	QAngle GetOldAngRotation();
	void SetOldAngRotation(QAngle& rot);
	void SetAbsAngles(const QAngle& rot);
	Vector GetMins();
	void SetMins(Vector& mins);
	Vector GetMaxs();
	Vector& GetPlayerMins();
	Vector& GetPlayerMaxs();
	unsigned int PlayerSolidMask(bool brushOnly = false, CBaseEntity* testPlayer = nullptr);
	void TracePlayerBBox(const Vector& rayStart, const Vector& rayEnd, int fMask, int collisionGroup, trace_t& tracePtr);
	void TracePlayerBBoxForGround(const Vector& start, const Vector& end, const Vector& minsSrc, const Vector& maxsSrc, IHandleEntity* player, unsigned int fMask, int collisionGroup, trace_t& pm);
	void SetMaxs(Vector& maxs);
	int GetSequence();
	void SetSequence(int seq);
	void SetSequenceVMT(int seq);
	float GetBoneController(int index);
	void SetBoneController(int index, float val);
	void GetBoneControllers(float controllers[MAXSTUDIOBONECTRLS]);
	void CopyBoneControllers(float controllers[MAXSTUDIOBONECTRLS]);
	void WriteBoneControllers(float controllers[MAXSTUDIOBONECTRLS]);
	float GetPoseParameter(int index);
	void GetPoseParameterRange(int index, float& flMin, float& flMax);
	float GetPoseParameterScaled(int index);
	float GetPoseParameterUnscaled(int index);
	float GetOldPoseParameterScaled(int index);
	float GetOldPoseParameterUnscaled(int index);
	int LookupPoseParameter(CStudioHdr* hdr, char* name);
	void SetPoseParameterGame(CStudioHdr* hdr, float flValue, int index);
	void SetPoseParameter(int index, float p);
	void SetPoseParameterScaled(int index, float p);
	void CopyPoseParameters(float* dest);
	int WritePoseParameters(float* src);
	void SetOldPoseParameter(int index, float p);
	void SetOldPoseParameterScaled(int index, float p);
	void CopyOldPoseParameters(float* dest);
	int WriteOldPoseParameters(float* src);
	unsigned char GetClientSideAnimation();
	void SetClientSideAnimation(unsigned char a);
	float GetCycle();
	void SetCycle(float cycle);
	float GetPlaybackRate();
	void SetPlaybackRate(float playbackrate);
	float GetNextAttack();
	void SetNextAttack(float att);
	bool IsDucked();
	bool GetDucked() { return IsDucked(); }
	void SetDucked(bool ducked);
	bool IsDucking();
	bool GetDucking() { return IsDucking(); };
	void SetDucking(bool ducking);
	bool IsInDuckJump();
	void SetInDuckJump(bool induckjump);
	int GetDuckJumpTimeMsecs();
	void SetDuckJumpTimeMsecs(int msecs);
	int GetDuckTimeMsecs();
	void SetDuckTimeMsecs(int msecs);
	int GetJumpTimeMsecs();
	void SetJumpTimeMsecs(int msecs);
	float GetFallVelocity();
	void SetFallVelocity(float vel);
	Vector GetViewOffset();
	Vector& GetViewOffsetVMT();
	void SetViewOffset(Vector& off);
	float GetDuckAmount();
	void SetDuckAmount(float duckamt);
	float GetDuckSpeed();
	void SetDuckSpeed(float spd);
	float GetVelocityModifier();
	void SetVelocityModifier(float vel);
	float GetSimulationTime();
	float GetOldSimulationTime();
	void SetOldSimulationTime(float time);
	void SetSimulationTime(float time);
	float GetLaggedMovement();
	void SetLaggedMovement(float mov);
	CBaseEntity* GetGroundEntity();
	void SetGroundEntityDirect(EHANDLE groundent);
	EHANDLE GetGroundEntityDirect();
	void SetGroundEntity(CBaseEntity* groundent);
	Vector GetVecLadderNormal();
	void SetVecLadderNormal(Vector& norm);
	float GetLowerBodyYaw();
	void SetLowerBodyYaw(float yaw);
	bool CanUpdateAnimations();
	void InvalidateAnimations();
	int DrawModel(int flags);
	C_CSGOPlayerAnimState* GetPlayerAnimState();
	void* GetBasePlayerAnimState();
	float GetMaxDesyncMultiplier(CTickrecord* sourcerecord = nullptr);
	float GetMaxDesyncDelta(CTickrecord* sourcerecord = nullptr);
	void EnableClientSideAnimation(clientanimating_t*& animating, int& out_originalflags);
	void DisableClientSideAnimation(clientanimating_t*& animating, int& out_originalflags);
	void RestoreClientSideAnimation(clientanimating_t*& animating, int& out_originalflags);
	bool IsPlayerGhost();
	bool IsWeapon();
	bool IsProjectile();
	bool IsGrenadeWeapon();
	bool IsKnifeWeapon();
	bool IsFlashGrenade();
	bool IsChicken();
	void DrawHitboxes(ColorRGBA color, float livetimesecs);
	void DrawHitboxesFromCache(ColorRGBA color, float livetimesecs, matrix3x4_t* matrix);
	bool IsCustomPlayer();
	void SetIsCustomPlayer(bool iscustom);
	bool WasKilledByTaser();
	int GetViewModelIndex();
	void HandleTaserAnimation();
	C_BaseViewModel* GetViewModel(int modelindex);
	void UpdateClientSideAnimation();
	void UpdateServerSideAnimation(int mode, float curtime, bool _RunOnlySetupVelocitySetupLean = false);
	void OnLatchInterpolatedVariables(int);
	void FrameAdvance(int);
	float GetLastClientSideAnimationUpdateTime();
	void SetLastClientSideAnimationUpdateTime(float time);
	int GetLastClientSideAnimationUpdateGlobalsFrameCount();
	void SetLastClientSideAnimationUpdateGlobalsFrameCount(int framecount);
	int GetEffects();
	void SetEffects(int effects);
	void AddEffect(int effect);
	void RemoveEffect(int effect);
	int GetObserverMode();
	int GetObserverModeVMT();
	void SetObserverMode(int mode);
	CUtlVectorSimple* GetAnimOverlayStruct() const;
	C_AnimationLayer* GetAnimOverlay(int i);
	C_AnimationLayer* GetAnimOverlayDirect(int i);
	int GetNumAnimOverlays() const;
	void CopyAnimLayers(C_AnimationLayer* dest);
	int WriteAnimLayers(C_AnimationLayer* src);
	int WriteAnimLayer(C_AnimationLayer* src, C_AnimationLayer* dest);
	int WriteAnimLayersFromPacket(C_AnimationLayerFromPacket* src);
	void InvalidatePhysicsRecursive(int nChangeFlags);
	Vector GetAbsOriginDirect();
	void SetAbsOriginDirect(Vector& origin);
	Vector* GetAbsOrigin();
	int entindex();
	float GetCurrentFeetYaw();
	void SetCurrentFeetYaw(float yaw);
	float GetGoalFeetYaw();
	void SetGoalFeetYaw(float yaw);
	float GetFriction();
	void SetFriction(float friction);
	float GetElasticity();
	void SetElasticity(float elasticity);
	float GetStepSize();
	void SetStepSize(float stepsize);
	float GetPlayerMaxSpeed();
	float GetMaxSpeed();
	void SetMaxSpeed(float maxspeed);
	bool IsParentChanging();
	void SetLocalVelocity(const Vector& vecVelocity);
	int GetTakeDamage(); //FIXME
	float* GetSurfaceFrictionAdr();
	float GetSurfaceFriction();
	void SetSurfaceFriction(float friction);
	char GetTextureType();
	void SetTextureType(char type);
	int GetFireCount();
	void SetFireCount(int count);
	int GetRelativeDirectionOfLastInjury();
	void SetRelativeDirectionOfLastInjury(int dir);
	int GetLastHitgroup();
	void SetLastHitgroup(int hitgroup);
	unsigned char GetWaterLevel();
	void SetWaterLevel(unsigned char level);
	float GetWaterJumpTime();
	void SetWaterJumpTime(float time);
	bool GetAllowAutoMovement();
	void SetAllowAutoMovement(bool allow);
	CBaseEntity* GetMoveParent();
	//FIXME, MOVEPARENT!
	//void SetMoveParent(CBaseEntity* parent) { *(DWORD*)((DWORD)this + moveparent) = (DWORD)parent; }
	const char* GetClassname();
	int GetMaxHealth();
	CBoneAccessor* GetBoneAccessor();
	void StandardBlendingRules(CStudioHdr* hdr, Vector pos[], QuaternionAligned q[], float currentTime, int boneMask);
	void BuildTransformations(CStudioHdr* hdr, Vector* pos, Quaternion* q, const matrix3x4_t& cameraTransform, int boneMask, byte* boneComputed);
	bool IsRagdoll();
	int LookupSequence(char*);
	CUtlVectorSimple* GetCachedBoneData();
	unsigned long GetMostRecentModelBoneCounter();
	void SetMostRecentModelBoneCounter(unsigned long counter);
	int GetLastOcclusionCheckFrameCount();
	void SetLastOcclusionCheckFrameCount(int count);
	int GetLastOcclusionCheckFlags();
	void SetLastOcclusionCheckFlags(int flags);
	void UpdateIKLocks(float currentTime);
	void CalculateIKLocks(float currentTime);
	void ControlMouth(CStudioHdr* pStudioHdr);
	void Wrap_SolveDependencies(DWORD m_pIk, Vector* pos, Quaternion* q, matrix3x4_t* bonearray, byte* computed);
	void Wrap_UpdateTargets(DWORD m_pIk, Vector* pos, Quaternion* q, matrix3x4_t* bonearray, byte* computed);
	CUtlVectorSimple& m_Attachments();
	bool PutAttachment(int number, const matrix3x4_t& attachmentToWorld);
	void FormatViewModelAttachment(int i, matrix3x4_t& world);
	void SetupBones_AttachmentHelper(CStudioHdr* hdr, matrix3x4a_t* custommatrix = nullptr);
	void Wrap_AttachmentHelper(CStudioHdr* hdr);
	void Wrap_AttachmentHelper();
	void ClearTargets(DWORD m_pIk);
	void Wrap_IKInit(DWORD m_pIk, CStudioHdr* hdrs, QAngle& angles, Vector& pos, float flTime, int iFramecounter, int boneMask);
	void DoExtraBoneProcessing(CStudioHdr* hdr, Vector* pos, Quaternion* q, matrix3x4_t* bonearray, byte* computed, DWORD m_pIK);
	void MarkForThreadedBoneSetup();
	bool SequencesAvailable();
	void ReevaluateAnimLOD();
	DWORD Wrap_CreateIK();
	void LockStudioHdrIfNoModel();
	float SequenceDuration(int iSequence);
	float GetSequenceCycleRate(int iSequence);
	float GetSequenceCycleRate_Server(int iSequence);
	float GetLayerSequenceCycleRate(C_AnimationLayer* layer, int sequence);
	float GetSequenceMoveDist(CStudioHdr *pStudioHdr, int iSequence);
	void GetSequenceLinearMotion(CStudioHdr *pStudioHdr, int iSequence, float* poseParameters, Vector* pVec);
	void GetSequenceLinearMotion(int iSequence, Vector *pVec);
	bool IsSequenceLooping(int iSequence);
	int GetButtons();
	void SetButtons(int buttons);
	class ButtonState_t
	{
	public:
		int nButtons;
		int afButtonLast;
		int afButtonPressed;
		int afButtonReleased;
	};
	void UpdateButtonState(int nUserCmdButtonMask);
	void BackupButtonState(ButtonState_t &state);
	void RestoreButtonState(ButtonState_t &state);
	int GetSequenceFlags(int sequence);
	int GetWaterType();
	void SetWaterType(int nType);
	unsigned char GetWaterTypeDirect();
	void SetWaterTypeDirect(unsigned char watertype);
	float GetFirstSequenceAnimTag(int sequence, int tag);
	float GetAnySequenceAnimTag(int sequence, int tag, float flLimit);
	float& GetUnknownAnimationFloat();
	float& GetUnknownSetupMovementFloat();
	void DoUnknownAnimationCode(DWORD offset, float val);
	void OnJump(float flUpVelocity);
	void OnLand(float flUpVelocity);
	bool HasWalkMovedSinceLastJump();
	void SetHasWalkMovedSinceLastJump(bool OnGround);
	bool GetSlowMovement();
	void PlayClientJumpSound();
	void PlayClientUnknownSound(Vector& origin, surfacedata_t* surf);
	bool UsesServerSideJumpAnimation();
	void DoAnimationEvent(PlayerAnimEvent_t event, int nData);
	bool GetDuckUntilOnGround();
	void SetDuckUntilOnGround(bool duckuntilonground);
	bool GetDuckOverride();
	void SetDuckOverride(bool over);
	Vector2D GetDuckingOrigin();
	void SetDuckingOrigin(Vector2D& origin);
	void SetDuckingOrigin(Vector& origin);
	int IsSpawnRappelling();
	float GetMaxFallVelocity();
	void SetMaxFallVelocity(float vel);
	float GetTimeNotOnLadder();
	void SetTimeNotOnLadder(float time);
	void SurpressLadderChecks(Vector* origin, Vector* normal);
	bool GetIsWalking();
	void SetIsWalking(bool walking);
	surfacedata_t* GetSurfaceData();
	void SetSurfaceData(surfacedata_t* data);
	int GetSurfaceProps();
	void SetSurfaceProps(int props);
	float GetStamina();
	void SetStamina(float stamina);
	int GetMoveState();
	void SetMoveState(int state);
	float GetSwimSoundTime();
	void SetSwimSoundTime(float time);
	CPlayerLocalData* GetLocalData();
	bool IsInAVehicle();
	int GetVehicleHandle();
	IClientVehicle* GetClientVehicle();
	IClientVehicle* GetVehicle();
	void ResetLatched();
	bool Interpolate(float curtime);
	VarMapping_t* GetVarMapping();
	float GetLastDuckTime();
	void SetLastDuckTime(float time);
	bool IsBot();
	int GetStuckLast();
	void SetStuckLast(int stucklast);
	void UpdateStepSound(surfacedata_t* psurface, const Vector& vecOrigin, const Vector& vecVelocity);
	IPhysicsObject* VPhysicsGetObject();
	Vector GetWaterJumpVel();
	void SetWaterJumpVel(Vector& vel);
	int GetPlayerState();
	void SetPlayerState(int state);
	bool IsObserver();
	bool HasHeavyArmor();
	void SetHasHeavyArmor(bool has);
	bool IsCarryingHostage(int unknown = 0);
	float GetGroundAccelLinearFracLastTime();
	void SetGroundAccelLinearFracLastTime(float time);
	CBaseCombatWeapon* GetActiveCSWeapon(); //DON'T USE, requires RTTI information
	CBaseCombatWeapon* GetCSWeapon(ClassID classname);
	float GetGravity();
	void SetGravity(float grav);
	float GetJumpTime();
	void SetJumpTime(float time);
	bool IsTaunting();
	bool IsInThirdPersonTaunt();
	float GetStepSoundTime();
	void SetStepSoundTime(float time);
	void PlayStepSound(Vector& vecOrigin, surfacedata_t* psurface, float fvol, bool force, bool unknown = false);
	void EyeVectors(Vector* pForward, Vector* pRight = NULL, Vector* pUp = NULL);
	CUserCmd* m_PlayerCommand();
	CUserCmd** m_pCurrentCommand();
	int CurrentCommandNumber();
	bool IsServer();
	bool GetCanMoveDuringFreezePeriod();
	bool IsGrabbingHostage();
	bool GetAliveVMT();
	bool GetWaitForNoAttack();
	void SetWaitForNoAttack(bool wait);
	float GetDefaultFOV();
	matrix3x4_t& EntityToWorldTransform();
	bool GetShouldUseAnimationEyeOffset();
	void SetShouldUseAnimationEyeOffset(bool use);
	int IsAutoMounting();
	void SetIsAutoMounting(int automounting);
	Vector& GetAutoMoveOrigin();
	void SetAutoMoveOrigin(Vector& origin);
	Vector& GetAutomoveTargetEnd();
	void SetAutomoveTargetEnd(Vector& end);
	float GetAutomoveStartTime();
	void SetAutomoveStartTime(float time);
	float GetAutomoveTargetTime();
	void SetAutomoveTargetTime(float time);
	//bool GetUnknownSurvivalBool();
	//void SetUnknownSurvivalBool(bool val);
	void ThirdPersonSwitch(bool thirdperson);
	int BlockingUseActionInProgress();
	void SetBlockingUseActionInProgress(int block);
	bool CanWaterJump();
	float GetEncumberance();
	float GetHealthShotBoostExpirationTime();
	void SetHealthShotBoostExpirationTime(float time);
	ClientRenderHandle_t& RenderHandle();
	float GetTimeOfLastInjury();
	void SetTimeOfLastInjury(float time);
	int GetSolidFlags();
	Vector GetAngularVelocity();
	void SetAngularVelocity(Vector& vel);
	bool IsStandable();
	bool IsBSPModel();
	surfacedata_t *GetGroundSurface();
	CBaseEntity* FindGroundEntity();
	void PhysicsCheckWaterTransition();
	void SetDesiredCollisionGroup(int group);
	int GetDesiredCollisionGroup();
	void SetNumAnimOverlays(int num);
	void Think();
	void PreThink();
	void PostThink();
	void RunPreThink();
	void RunThink();
	int GetThinkTick();
	void SetThinkTick(int tick);
	/*
	void ResolveFlyCollisionCustom(trace_t& trace, Vector& vecVelocity);
	void ResolveFlyCollisionBounce(trace_t& trace, Vector& vecVelocity);
	void ResolveFlyCollisionSlide(trace_t& pm, Vector& move);
	void PhysicsPushEntity(const Vector& push, trace_t* pTrace);
	*/
	CBaseEntity* GetUnknownEntity(int slot);
	bool ComputeHitboxSurroundingBox(Vector *pVecWorldMins, Vector *pVecWorldMaxs, matrix3x4_t* pSourceMatrix = nullptr, bool use_valve = false);
	static void Remove(IHandleEntity* pEnt);
	bool HasC4();
	float m_fMolotovDamageTime();
	float FlashbangTime();
	bool IsEnemy(CBaseEntity *pLocal);
	void UpdatePartition();
	bool CanUseFastPath();
	void SetCanUseFastPath(bool val);
	int GetExplodeEffectTickBegin() const;
	datamap_t* GetPredDescMap();
	void* GetPredictedFrame(int framenumber);
	inline bool IsEFlagSet(int nEFlagMask) { return (GetEFlags() & nEFlagMask) != 0; }
	inline void AddEFlags(int nEFlagMask) { SetEFlags(GetEFlags() | nEFlagMask); }
	inline void RemoveEFlags(int nEFlagMask) { SetEFlags(GetEFlags() & ~nEFlagMask); }
	void SetCheckUntouch(bool check);
	bool GetCheckUntouch() { return IsEFlagSet(EFL_CHECK_UNTOUCH); };
	int GetTouchStamp();
	void SetTouchStamp(int stamp);
	void DoLocalPlayerPrePrediction();
	void MoveToLastReceivedPosition(int i = 0);
	void PhysicsCheckForEntityUntouch();
	void PhysicsTouchTriggers(const Vector *pPrevAbsOrigin = NULL);
	bool GetUnknownEntityPredictionBool();
	void SetUnknownEntityPredictionBool(bool val);
	int GetFinalPredictedTick();
	void SetFinalPredictedTick(int tick);
	void* GetFirstPredictedFrame();
	void VPhysicsCompensateForPredictionErrors(void* frame);
	static void PushAllowBoneAccess(bool bAllowForNormalModels, bool bAllowForViewModels, char const *tagPush);
	static void PopBoneAccess(char const *tagPop);
	int SaveData(/*const char *context, */int slot, int type);
	bool DelayUnscope(float& fov);
	bool& IsFireBurning();
	void GetFireDelta(Vector& dest, int index);
	Vector& GetSurroundingMins();
	Vector& GetSurroundingMaxs();
	CParticleProperty* ParticleProp();

	// Used for debugging. Will produce asserts if someone tries to setup bones or
	// attachments before it's allowed.
	// Use the "AutoAllowBoneAccess" class to auto push/pop bone access.
	// Use a distinct "tag" when pushing/popping - asserts when push/pop tags do not match.
	struct AutoAllowBoneAccess
	{
		AutoAllowBoneAccess(bool bAllowForNormalModels, bool bAllowForViewModels);
		~AutoAllowBoneAccess(void);
	};
};

typedef CBaseEntity CBasePlayer;
typedef CBaseEntity C_BaseEntity;
typedef CBaseEntity C_BasePlayer;
typedef CBaseEntity CCSPlayer;
typedef CBaseEntity C_CSPlayer;
typedef CBaseEntity C_BaseCombatCharacter;
typedef CBaseEntity CBaseCombatCharacter;

class VTHook;

class HookedEntity
{
public:
	HookedEntity(){};
	HookedEntity(int index, CBaseEntity* entity, VTHook* hook, DWORD OriginalHookedSub1, DWORD OriginalHookedSub2, DWORD OriginalHookedSub3, DWORD OriginalHookedSub4, DWORD OriginalHookedSub5, DWORD OriginalHookedSub6, DWORD OriginalHookedSub7, DWORD OriginalHookedSub8, DWORD OriginalHookedSub9, DWORD OriginalHookedSub10, DWORD OriginalHookedSub11, DWORD OriginalHookedSub12, DWORD OriginalHookedSub13, DWORD OriginalHookedSub14, bool* IsHookedStorage, HookedEntity** HookedEntityStorage, int RefEHandle)
	{
		bmark_for_deletion	= false;
		iindex				  = index;
		pentity				  = entity;
		phook				  = hook;
		dwOriginalHookedSub1  = OriginalHookedSub1;
		dwOriginalHookedSub2  = OriginalHookedSub2;
		dwOriginalHookedSub3  = OriginalHookedSub3;
		dwOriginalHookedSub4  = OriginalHookedSub4;
		dwOriginalHookedSub5  = OriginalHookedSub5;
		dwOriginalHookedSub6  = OriginalHookedSub6;
		dwOriginalHookedSub7  = OriginalHookedSub7;
		dwOriginalHookedSub8  = OriginalHookedSub8;
		dwOriginalHookedSub9  = OriginalHookedSub9;
		dwOriginalHookedSub10 = OriginalHookedSub10;
		dwOriginalHookedSub11 = OriginalHookedSub11;
		dwOriginalHookedSub12 = OriginalHookedSub12;
		dwOriginalHookedSub13 = OriginalHookedSub13;
		dwOriginalHookedSub14 = OriginalHookedSub14;
		pIsHooked			  = IsHookedStorage;
		ppHookedEntity		  = HookedEntityStorage;
		refehandle			  = RefEHandle;
	};
	~HookedEntity();
	bool IsMarkedForDeletion() const { return bmark_for_deletion; }
	void MarkForDeletion() { bmark_for_deletion = true; }
	int GetIndex() const { return iindex; }
	CBaseEntity* GetEntity() const { return pentity; }
	VTHook* GetHook() const { return phook; }
	DWORD GetOriginalHookedSub1() const { return dwOriginalHookedSub1; }
	DWORD GetOriginalHookedSub2() const { return dwOriginalHookedSub2; }
	DWORD GetOriginalHookedSub3() const { return dwOriginalHookedSub3; }
	DWORD GetOriginalHookedSub4() const { return dwOriginalHookedSub4; }
	DWORD GetOriginalHookedSub5() const { return dwOriginalHookedSub5; }
	DWORD GetOriginalHookedSub6() const { return dwOriginalHookedSub6; }
	DWORD GetOriginalHookedSub7() const { return dwOriginalHookedSub7; }
	DWORD GetOriginalHookedSub8() const { return dwOriginalHookedSub8; }
	DWORD GetOriginalHookedSub9() const { return dwOriginalHookedSub9; }
	DWORD GetOriginalHookedSub10() const { return dwOriginalHookedSub10; }
	DWORD GetOriginalHookedSub11() const { return dwOriginalHookedSub11; }
	DWORD GetOriginalHookedSub12() const { return dwOriginalHookedSub12; }
	DWORD GetOriginalHookedSub13() const { return dwOriginalHookedSub13; }
	DWORD GetOriginalHookedSub14() const { return dwOriginalHookedSub14; }
	bool* GetIsHookedPointer() const { return pIsHooked; }
	HookedEntity** GetHookedEntityPointer() const { return ppHookedEntity; }
	unsigned long GetRefEHandle() const { return refehandle; }

private:
	bool bmark_for_deletion;
	int iindex;
	CBaseEntity* pentity;
	VTHook* phook;
	DWORD dwOriginalHookedSub1;
	DWORD dwOriginalHookedSub2;
	DWORD dwOriginalHookedSub3;
	DWORD dwOriginalHookedSub4;
	DWORD dwOriginalHookedSub5;
	DWORD dwOriginalHookedSub6;
	DWORD dwOriginalHookedSub7;
	DWORD dwOriginalHookedSub8;
	DWORD dwOriginalHookedSub9;
	DWORD dwOriginalHookedSub10;
	DWORD dwOriginalHookedSub11;
	DWORD dwOriginalHookedSub12;
	DWORD dwOriginalHookedSub13;
	DWORD dwOriginalHookedSub14;
	bool* pIsHooked;
	HookedEntity** ppHookedEntity;
	unsigned long refehandle;
};

void __fastcall HookedUpdateClientSideAnimation(CBaseEntity* me);

#pragma pack(push, 1)
class animstate_pose_param_cache_t
{
public:
	bool m_bInitialized; //0x0000
	char pad_01[3]; //0x0001
	int32_t m_iPoseParameter; //0x0004
	char* m_szPoseParameter; //0x0008

	void SetValue(CBaseEntity* pPlayer, float flValue);
	float GetValue(CBaseEntity* pPlayer);
	bool Init(CBaseEntity* pPlayer, char* name);
}; //Size: 0x000C
#pragma pack(pop)

#pragma pack(push, 1)
struct AimLayer
{
	float m_flUnknown0;
	float m_flTotalTime;
	float m_flUnknown1;
	float m_flUnknown2;
	float m_flWeight;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct AimLayers
{
	AimLayer layers[3];
};
#pragma pack(pop)


#pragma pack(push, 1)
struct C_CSGOPlayerAnimState
{
	void* vtable;
	bool m_bIsReset;
	bool m_bUnknownClientBool;
	char pad[2];
	int m_iSomeTickcount;
	float m_flFlashedStartTime;
	float m_flFlashedEndTime;
	AimLayers m_AimLayers;
	int m_nModelIndex;
	int m_iUnknownClientArr[3];
	CBaseEntity* pBaseEntity;
	CBaseCombatWeapon* pActiveWeapon;
	CBaseCombatWeapon* pLastActiveWeapon;
	float m_flLastClientSideAnimationUpdateTime;
	int32_t m_iLastClientSideAnimationUpdateFramecount;
	float m_flLastClientSideAnimationUpdateTimeDelta;
	float m_flEyeYaw;
	float m_flPitch;
	float m_flGoalFeetYaw;
	float m_flCurrentFeetYaw;
	float m_flCurrentMoveDirGoalFeetDelta;
	float m_flGoalMoveDirGoalFeetDelta;
	float m_flFeetVelDirDelta;
	float pad_0094;
	float m_flFeetCycle;
	float m_flFeetWeight;
	float m_fUnknown2;
	float m_fDuckAmount;
	float m_flHitGroundCycle;
	float m_fUnknown3;
	Vector m_vOrigin;
	Vector m_vLastOrigin;
	Vector m_vVelocity;
	Vector m_vVelocityNormalized;
	Vector m_vecLastAcceleratingVelocity;
	float m_flSpeed;
	float m_flAbsVelocityZ;
	float m_flSpeedNormalized;
	float m_flRunningSpeed;
	float m_flDuckingSpeed;
	float m_flTimeSinceStartedMoving;
	float m_flTimeSinceStoppedMoving;
	bool m_bOnGround;
	bool m_bInHitGroundAnimation;
	char pad_010A[2];
	float m_flNextLowerBodyYawUpdateTime;
	float m_flTotalTimeInAir;
	float m_flStartJumpZOrigin;
	float m_flHitGroundWeight;
	float m_flGroundFraction;
	bool m_bJust_Landed;
	bool m_bJust_LeftGround;
	char pad_0120[2];
	float m_flDuckRate;
	bool m_bOnLadder;
	char pad_0128[3];
	float m_flLadderCycle;
	float m_flLadderWeight;
	bool m_bNotRunning;
	char pad_0135[3];
	bool m_bInBalanceAdjust;
	char pad_0141[3];
	CUtlVectorSimple m_Modifiers;
	int gap148[1];
	float m_flTimeOfLastInjury;
	float m_flLastSetupLeanCurtime;
	Vector m_vecLastSetupLeanVelocity;
	Vector m_vecSetupLeanVelocityDelta;
	Vector m_vecSetupLeanVelocityInterpolated;
	float m_flLeanWeight;
	int m_iUnknownIntArr2[2];
	bool m_bFlashed;
	char m_bFlashedPad[3];
	float m_flStrafeWeight;
	int m_iUnknownint3;
	float m_flStrafeCycle;
	int m_iStrafeSequence;
	bool m_bStrafing;
	char m_bStrafingPad[3];
	float m_flTotalStrafeTime;
	int m_iUnknownInt4;
	bool m_bUnknownBool__;
	bool m_bIsAccelerating;
	char pad_01AE[2];
	animstate_pose_param_cache_t m_arrPoseParameters[20];
	int m_iUnknownClientInt__;
	float m_flVelocityUnknown;
	int m_iMoveState;
	float m_flMovePlaybackRate;
	float m_flUnknownFL0;
	float m_flUnknownFL;
	float m_flUnknownFL1;
	float m_flMinYawServer;
	float m_flMaxYawServer;
	float m_flMaximumPitchServer;
	float m_flMinimumPitchServer;
	int m_iUnknownInt;
	char pad_02D0[84];
	float m_flEyePosZ;
	bool m_bIsDucked;
	char pad_0329[3];
	float m_flUnknownCap1;
	float m_flMinYaw;
	float m_flMaxYaw;
	float m_flMinPitch;
	float m_flMaxPitch;
	int m_iAnimsetVersion;

	C_AnimationLayer* GetAnimOverlay(int layer);
	void IncrementLayerCycle(int layer, bool dont_clamp_cycle);
	void IncrementLayerCycleGeneric(int layer);
	void LayerWeightAdvance(int layer);
	void SetFeetCycle(float cycle);
	int GetLayerActivity(int layer);
	void Update(float yaw, float pitch);
	void SetupFlinch();
	void SetupAliveloop();
	void SetupFlashedReaction();
	void SetupWholeBodyAction();
	void SetupMovement();
	void SetupLean();
	void SetupWeaponAction();
	void SetupAimMatrix();
	void SetupVelocity();
	bool CacheSequences();
	void SetLayerCycle(int layer, float cycle);
	void SetLayerWeight(int layer, float weight);
	void SetLayerSequence(int layer, int sequence);
	float GetLayerCycle(int layer);
	float GetLayerWeight(int layer);
	char* GetWeaponMoveAnimation();
	void UpdateAnimLayer(float playbackrate, int layer, int sequence, float weight, float cycle);
	void UpdateLayerOrderPreset(float weight, int layer, int sequence);
	float GetLayerIdealWeightFromSeqCycle(int layer);
	void UpdateAimLayer(AimLayer *layer, float timedelta, float multiplier, bool somebool);
};
#pragma pack(pop)

//server 0x2C4 bytes
#pragma pack(push, 1)
class CCSGOPlayerAnimState
{
public:
	int *m_pAnimLayerOrder;
	bool m_bIsReset;
	char firstpad[3];
	float m_flFlashedStartTime;
	float m_flFlashedEndTime;
	AimLayers m_AimLayers;
	int m_nModelIndex;
	CBaseEntity *pBaseEntity;
	CBaseCombatWeapon *pActiveWeapon;
	CBaseCombatWeapon *pLastActiveWeapon;
	float m_flLastClientSideAnimationUpdateTime;
	int32_t m_iLastClientSideAnimationUpdateFramecount;
	float m_flLastClientSideAnimationUpdateTimeDelta;
	float m_flEyeYaw;
	float m_flPitch;
	float m_flGoalFeetYaw;
	float m_flCurrentFeetYaw;
	float m_flCurrentMoveDirGoalFeetDelta;
	float m_flGoalMoveDirGoalFeetDelta;
	float m_flFeetVelDirDelta;
	float pad_0094;
	float m_flFeetCycle;
	float m_flFeetWeight;
	float m_fUnknown2;
	float m_fDuckAmount;
	float m_flHitGroundCycle;
	float m_fUnknown3;
	Vector m_vOrigin;
	Vector m_vLastOrigin;
	Vector m_vVelocity;
	Vector m_vVelocityNormalized;
	Vector m_vecLastAcceleratingVelocity;
	float m_flSpeed;
	float m_flAbsVelocityZ;
	float m_flSpeedNormalized;
	float m_flRunningSpeed;
	float m_flDuckingSpeed;
	float m_flTimeSinceStartedMoving;
	float m_flTimeSinceStoppedMoving;
	bool m_bOnGround;
	bool m_bJumping;
	char pad_010A[2];
	float m_flNextLowerBodyYawUpdateTime;
	bool m_bInParachute;
	bool m_bInHitGroundAnimation;
	char PAD_FUCKER[2];
	int m_iUnknownIntBlah;
	float m_flTotalTimeInAir;
	float m_flStartJumpZOrigin;
	float m_flHitGroundWeight;
	float m_flGroundFraction;
	bool m_bJust_Landed;
	bool m_bJust_LeftGround;
	char pad_0120[2];
	float m_flDuckRate;
	bool m_bOnLadder;
	char pad_0128[3];
	float m_flLadderCycle;
	float m_flLadderWeight;
	bool m_bNotRunning;
	bool m_bDefusing;
	bool m_bPlantingBomb;
	char pad_0135;
	bool m_bInBalanceAdjust;
	char pad_0141[3];
	CUtlVector<CUtlSymbol> m_ActivityModifiers;
	int gap148[1];
	float m_flTimeOfLastInjury;
	float m_flLastSetupLeanCurtime;
	Vector m_vecLastSetupLeanVelocity;
	Vector m_vecSetupLeanVelocityDelta;
	Vector m_vecSetupLeanVelocityInterpolated;
	float m_flLeanWeight;
	int m_iUnknownIntArr2[2];
	bool m_bFlashed;
	char m_bFlashedPad[3];
	float m_flStrafeWeight;
	int m_iUnknownint3;
	float m_flStrafeCycle;
	int m_iStrafeSequence;
	bool m_bStrafing;
	char m_bStrafingPad[3];
	float m_flTotalStrafeTime;
	int m_iUnknownInt4;
	bool m_bUnknownBool__;
	bool m_bIsAccelerating;
	char pad_01AE[2];
	animstate_pose_param_cache_t m_arrPoseParameters[20];
	bool m_bDeploying;
	char pad__[3];
	DWORD m_iUnknownInt__;
	float m_flGoalRunningSpeed;
	int m_iMoveState;
	float m_flMovePlaybackRate;
	float m_flUnknownFL0;
	float m_flMinYaw;
	float m_flMaxYaw;
	float m_flMinPitch;
	float m_flMaxPitch;
	int m_iAnimsetVersion;

	float m_flCurTime; //dylan added this so we can multithread the server side animation code instead of using gpGlobals->curtime

	CCSGOPlayerAnimState& operator=(const CCSGOPlayerAnimState &vOther);
	::CCSGOPlayerAnimState(CBaseEntity* ent);
	::CCSGOPlayerAnimState(const CCSGOPlayerAnimState &vOther);
	::CCSGOPlayerAnimState() {};
	void  Reset();
	bool  IsLayerSequenceFinished(int layer);
	void  SetLayerSequenceFromActivity(int layer, int activity);
	void  IncrementLayerCycle(int layer, bool is_looping);
	void  IncrementLayerCycleGeneric(int layer);
	void  IncrementLayerCycleWeightRateGeneric(int layer);
	void  LayerWeightAdvance(int layer);
	void  SetFeetCycle(float cycle);
	int   GetLayerActivity(int layer);
	int   GetWeightedSequenceFromActivity(int activity);
	void  Update(float yaw, float pitch);
	void  UpdateSetupVelocityAndSetupLean(float yaw, float pitch);
	void  SetupFlinch();
	void  SetupAliveloop();
	void  SetupFlashedReaction();
	void  SetupWholeBodyAction();
	void  SetupMovement();
	void  SetupLean();
	void  SetupWeaponAction();
	void  SetupAimMatrix();
	void  SetupVelocity();
	bool  CacheSequences();
	void  SetLayerPlaybackRate(int layer, float playbackrate);
	void  SetLayerCycle(int layer, float cycle);
	void  SetLayerCycleDirect(int layer, float cycle);
	void  SetLayerWeight(int layer, float weight);
	void  SetLayerWeightDeltaRate(int layer, float oldweight);
	void  SetLayerSequence(int layer, int sequence);
	float GetLayerCycle(int layer);
	float GetLayerWeight(int layer);
	char* GetWeaponPrefix();
	void  UpdateAnimLayer(float playbackrate, int layer, int sequence, float weight, float cycle);
	void  UpdateLayerOrderPreset(float weight, int layer, int sequence);
	float GetLayerIdealWeightFromSeqCycle(int layer);
	void  ModifyEyePosition(Vector& vecEyePos);
	void  UpdateAimLayer(AimLayer *layer, float timedelta, float multiplier, bool somebool);
	void  DoAnimationEvent(int event);
	void  ApplyLayerOrderPreset(int* order, bool force_apply);
	bool  LayerSequenceHasActMod(int layer, char* sequence);
	int   GetLayerSequence(int layer);
	C_AnimationLayer* GetAnimOverlay(int layer);
	void AddActivityModifier(const char *szName);
	void UpdateActivityModifiers();

	int BacktraceCycle(float startcycle, int layer, bool is_looping);
};
#pragma pack(pop)

extern CCSGOPlayerAnimState* CreateCSGOPlayerAnimState(CBaseEntity* ent);

extern CUtlSymbolTable *g_ActivityModifiersTable;

extern float Studio_SetPoseParameter(const CStudioHdr* pStudioHdr, int iParameter, float flValue, float& ctlValue);

extern bool IsPlayingOldDemo();

#if 0

class CBoneSetup;
class IPoseDebugger;
class IBoneSetup
{
public:
	IBoneSetup(const CStudioHdr *pStudioHdr, int boneMask, const float poseParameter[], IPoseDebugger *pPoseDebugger = NULL);
	~IBoneSetup(void);
	void InitPose(Vector pos[], QuaternionAligned q[]);
	void AccumulatePose(Vector pos[], Quaternion q[], int sequence, float cycle, float flWeight, float flTime, CIKContext *pIKContext);
	void CalcAutoplaySequences(Vector pos[], Quaternion q[], float flRealTime, CIKContext *pIKContext);
	void CalcBoneAdj(Vector pos[], Quaternion q[], const float controllers[]);

	CStudioHdr *GetStudioHdr();

private:
	CBoneSetup *m_pBoneSetup;
};

struct ikcontextikrule_t
{
	int			index;

	int			type;
	int			chain;

	int			bone;

	int			slot;	// iktarget slot.  Usually same as chain.
	float		height;
	float		radius;
	float		floor;
	Vector		pos;
	Quaternion	q;

	float		start;	// beginning of influence
	float		peak;	// start of full influence
	float		tail;	// end of full influence
	float		end;	// end of all influence

	float		top;
	float		drop;

	float		commit;		// frame footstep target should be committed
	float		release;	// frame ankle should end rotation from latched orientation

	float		flWeight;		// processed version of start-end cycle
	float		flRuleWeight;	// blending weight
	float		latched;		// does the IK rule use a latched value?
	char		*szLabel;

	Vector		kneeDir;
	Vector		kneePos;

	ikcontextikrule_t() {}

private:
	// No copy constructors allowed
	ikcontextikrule_t(const ikcontextikrule_t& vOther);
};

class CIKTarget
{
public:
	void SetOwner(int entindex, const Vector &pos, const QAngle &angles);
	void ClearOwner(void);
	int GetOwner(void);
	void UpdateOwner(int entindex, const Vector &pos, const QAngle &angles);
	void SetPos(const Vector &pos);
	void SetAngles(const QAngle &angles);
	void SetQuaternion(const Quaternion &q);
	void SetNormal(const Vector &normal);
	void SetPosWithNormalOffset(const Vector &pos, const Vector &normal);
	void SetOnWorld(bool bOnWorld = true);

	bool IsActive(void);
	void IKFailed(void);
	int chain;
	int type;
	void MoveReferenceFrame(Vector &deltaPos, QAngle &deltaAngles);
	// accumulated offset from ideal footplant location
public:
	struct x2 {
		char		*pAttachmentName;
		Vector		pos;
		Quaternion	q;
	} offset;
private:
	struct x3 {
		Vector		pos;
		Quaternion	q;
	} ideal;
public:
	struct x4 {
		float		latched;
		float		release;
		float		height;
		float		floor;
		float		radius;
		float		flTime;
		float		flWeight;
		Vector		pos;
		Quaternion	q;
		bool		onWorld;
	} est; // estimate contact position
	struct x5 {
		float		hipToFoot;	// distance from hip
		float		hipToKnee;	// distance from hip to knee
		float		kneeToFoot;	// distance from knee to foot
		Vector		hip;		// location of hip
		Vector		closest;	// closest valid location from hip to foot that the foot can move to
		Vector		knee;		// pre-ik location of knee
		Vector		farthest;	// farthest valid location from hip to foot that the foot can move to
		Vector		lowest;		// lowest position directly below hip that the foot can drop to
	} trace;
private:
	// internally latched footset, position
	struct x1 {
		// matrix3x4_t		worldTarget;
		bool		bNeedsLatch;
		bool		bHasLatch;
		float		influence;
		int			iFramecounter;
		int			owner;
		Vector		absOrigin;
		QAngle		absAngles;
		Vector		pos;
		Quaternion	q;
		Vector		deltaPos;	// acculated error
		Quaternion	deltaQ;
		Vector		debouncePos;
		Quaternion	debounceQ;
	} latched;
	struct x6 {
		float		flTime; // time last error was detected
		float		flErrorTime;
		float		ramp;
		bool		bInError;
	} error;

	friend class CIKContext;
};

class CIKContext
{
public:
	CIKContext();
	void Init(const CStudioHdr *pStudioHdr, const QAngle &angles, const Vector &pos, float flTime, int iFramecounter, int boneMask);
	void AddDependencies(mstudioseqdesc_t &seqdesc, int iSequence, float flCycle, const float poseParameters[], float flWeight = 1.0f);

	void ClearTargets(void);
	void UpdateTargets(Vector pos[], Quaternion q[], matrix3x4_t boneToWorld[], CBoneBitList &boneComputed);
	void AutoIKRelease(void);
	void SolveDependencies(Vector pos[], Quaternion q[], matrix3x4_t boneToWorld[], CBoneBitList &boneComputed);

	void AddAutoplayLocks(Vector pos[], Quaternion q[]);
	void SolveAutoplayLocks(Vector pos[], Quaternion q[]);

	void AddSequenceLocks(mstudioseqdesc_t &SeqDesc, Vector pos[], Quaternion q[]);
	void SolveSequenceLocks(mstudioseqdesc_t &SeqDesc, Vector pos[], Quaternion q[]);

	void AddAllLocks(Vector pos[], Quaternion q[]);
	void SolveAllLocks(Vector pos[], Quaternion q[]);

	void SolveLock(const mstudioiklock_t *plock, int i, Vector pos[], Quaternion q[], matrix3x4_t boneToWorld[], CBoneBitList &boneComputed);

	CUtlVectorFixed< CIKTarget, 12 >	m_target;

private:

	CStudioHdr const *m_pStudioHdr;

	bool Estimate(int iSequence, float flCycle, int iTarget, const float poseParameter[], float flWeight = 1.0f);
	void BuildBoneChain(const Vector pos[], const Quaternion q[], int iBone, matrix3x4_t *pBoneToWorld, CBoneBitList &boneComputed);

	// virtual IK rules, filtered and combined from each sequence
	CUtlVector< CUtlVector< ikcontextikrule_t > > m_ikChainRule;
	CUtlVector< ikcontextikrule_t > m_ikLock;
	matrix3x4_t m_rootxform;

	int m_iFramecounter;
	float m_flTime;
	int m_boneMask;
};
#endif