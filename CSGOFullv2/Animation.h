#pragma once
#include "BaseEntity.h"
#include "misc.h"
#include "cx_strenc.h"

// This can be used to give all datatables access to protected and private members of the class.
#define ALLOW_DATATABLES_PRIVATE_ACCESS() \
	template <typename T> friend int ClientClassInit(T *);

#define MAX_OVERLAYS 15

enum AnimTag_t
{
	ANIMTAG_UNINITIALIZED,
	ANIMTAG_STARTCYCLE_N,
	ANIMTAG_STARTCYCLE_NE,
	ANIMTAG_STARTCYCLE_E,
	ANIMTAG_STARTCYCLE_SE,
	ANIMTAG_STARTCYCLE_S,
	ANIMTAG_STARTCYCLE_SW,
	ANIMTAG_STARTCYCLE_W,
	ANIMTAG_STARTCYCLE_NW,
	ANIMTAG_AIMLIMIT_YAWMIN_IDLE,
	ANIMTAG_AIMLIMIT_YAWMAX_IDLE,
	ANIMTAG_AIMLIMIT_YAWMIN_WALK,
	ANIMTAG_AIMLIMIT_YAWMAX_WALK,
	ANIMTAG_AIMLIMIT_YAWMIN_RUN,
	ANIMTAG_AIMLIMIT_YAWMAX_RUN,
	ANIMTAG_AIMLIMIT_YAWMIN_CROUCHIDLE,
	ANIMTAG_AIMLIMIT_YAWMAX_CROUCHIDLE,
	ANIMTAG_AIMLIMIT_YAWMIN_CROUCHWALK,
	ANIMTAG_AIMLIMIT_YAWMAX_CROUCHWALK,
	ANIMTAG_AIMLIMIT_PITCHMIN_IDLE,
	ANIMTAG_AIMLIMIT_PITCHMAX_IDLE,
	ANIMTAG_AIMLIMIT_PITCHMIN_WALKRUN,
	ANIMTAG_AIMLIMIT_PITCHMAX_WALKRUN,
	ANIMTAG_AIMLIMIT_PITCHMIN_CROUCH,
	ANIMTAG_AIMLIMIT_PITCHMAX_CROUCH,
	ANIMTAG_AIMLIMIT_PITCHMIN_CROUCHWALK,
	ANIMTAG_AIMLIMIT_PITCHMAX_CROUCHWALK,
	ANIMTAG_FLASHBANG_PASSABLE,
	ANIMTAG_WEAPON_POSTLAYER,
	ANIMTAG_MAX
};

//animstate pose parameter cache
enum ANIMSTATE_POSE_PARAMETERS
{
	lean_yaw_cache = 0, //0
	speed_cache, //1
	ladder_speed_cache, //2
	ladder_yaw_cache, //3
	move_yaw_cache, //4
	run_cache, //5, only used if animset version < 2
	body_yaw_cache, //6
	body_pitch_cache, //7
	death_yaw_cache, //8
	stand_cache, //9
	jump_fall_cache, //10
	aim_blend_stand_idle_cache, //11
	aim_blend_crouch_idle_cache, //12
	strafe_yaw_cache, //13
	aim_blend_stand_walk_cache, //14
	aim_blend_stand_run_cache, //15
	aim_blend_crouch_walk_cache, //16
	move_blend_walk_cache, //17
	move_blend_run_cache, //18
	move_blend_crouch_cache //19
};

//baseentity indexes
enum POSE_PARAMETERS
{
	strafe_yaw = 0,
	stand,
	lean_yaw,
	speed,
	ladder_yaw,
	ladder_speed,
	jump_fall,
	move_yaw,
	move_blend_crouch,
	move_blend_walk,
	move_blend_run,
	body_yaw,
	body_pitch,
	aim_blend_stand_idle,
	aim_blend_stand_walk,
	aim_blend_stand_run,
	aim_blend_crouch_idle,
	aim_blend_crouch_walk,
	death_yaw
};

#pragma pack(push, 1)
struct animtag_t
{
	int index;
	float first_cycle;
	int tag_string_offset_from_this;

	const char* GetName()
	{
		return (const char*)this + tag_string_offset_from_this;
	}
};
#pragma pack(pop)

struct animtag_desc_t
{
	int index;
	char* name;
};

extern animtag_desc_t g_AnimTags[ANIMTAG_MAX];

extern int GetTagIndexFromAnimTagName(const char *name);

//-----------------------------------------------------------------------------
// For invalidate physics recursive
//-----------------------------------------------------------------------------
enum InvalidatePhysicsBits_t
{
	POSITION_CHANGED = 0x1,
	ANGLES_CHANGED = 0x2,
	VELOCITY_CHANGED = 0x4,
	ANIMATION_CHANGED = 0x8,		// Means cycle has changed, or any other event which would cause render-to-texture shadows to need to be rerendeded
	BOUNDS_CHANGED = 0x10,		// Means render bounds have changed, so shadow decal projection is required, etc.
	SEQUENCE_CHANGED = 0x20,		// Means sequence has changed, only interesting when surrounding bounds depends on sequence																				
};

// Use this to disable range checks within a scope.
class CDisableRangeChecks
{
public:
	CDisableRangeChecks();
	~CDisableRangeChecks();
};


template< class T >
inline void RangeCheck(const T &value, int minValue, int maxValue)
{
#if 0
#ifdef _DEBUG
	extern bool g_bDoRangeChecks;
	if (ThreadInMainThread() && g_bDoRangeChecks)
	{
		// Ignore the min/max stuff for now.. just make sure it's not a NAN.
		//Assert(_finite(value));
	}
#endif
#endif
}

inline void RangeCheck(const Vector &value, int minValue, int maxValue)
{
#if 0
#ifdef _DEBUG
	RangeCheck(value.x, minValue, maxValue);
	RangeCheck(value.y, minValue, maxValue);
	RangeCheck(value.z, minValue, maxValue);
#endif
#endif
}


template< class T, int minValue, int maxValue, int startValue >
class CRangeCheckedVar
{
public:

	inline CRangeCheckedVar()
	{
		m_Val = startValue;
	}

	inline CRangeCheckedVar(const T &value)
	{
		*this = value;
	}

	T GetRaw() const
	{
		return m_Val;
	}

	// Clamp the value to its limits. Interpolation code uses this after interpolating.
	inline void Clamp()
	{
		if (m_Val < minValue)
			m_Val = minValue;
		else if (m_Val > maxValue)
			m_Val = maxValue;
	}

	inline operator const T&() const
	{
		return m_Val;
	}

	inline CRangeCheckedVar<T, minValue, maxValue, startValue>& operator=(const T &value)
	{
		RangeCheck(value, minValue, maxValue);
		m_Val = value;
		return *this;
	}

	inline CRangeCheckedVar<T, minValue, maxValue, startValue>& operator+=(const T &value)
	{
		return (*this = m_Val + value);
	}

	inline CRangeCheckedVar<T, minValue, maxValue, startValue>& operator-=(const T &value)
	{
		return (*this = m_Val - value);
	}

	inline CRangeCheckedVar<T, minValue, maxValue, startValue>& operator*=(const T &value)
	{
		return (*this = m_Val * value);
	}

	inline CRangeCheckedVar<T, minValue, maxValue, startValue>& operator/=(const T &value)
	{
		return (*this = m_Val / value);
	}

private:

	T m_Val;
};

//92 bytes on the server
//Size: 56 bytes
class C_AnimationLayer
{
public:
	// This allows the datatables to access private members.
	//ALLOW_DATATABLES_PRIVATE_ACCESS();

	//C_AnimationLayer();

	bool IsActive()
	{
		if (!m_pOwner || _m_flPlaybackRate <= 0.0f)
			return false;

		//idle layer is always looping
		if (m_pOwner->GetAnimOverlay(IDLE_LAYER) == this)
			return true;

		return _m_flCycle < 0.999f;
	}

#if 0
	virtual void Reset();

	virtual void SetOwner(/*C_BaseAnimatingOverlay*/CBaseEntity* *pOverlay);
	virtual /*C_BaseAnimatingOverlay*/CBaseEntity* *GetOwner() const;

	virtual void SetOrder(int order);
	virtual bool IsActive(void);
	virtual float GetFadeout(float flCurTime);

	virtual void SetSequence(int nSequence);
	virtual void SetCycle(float flCycle);
	virtual void SetPrevCycle(float flCycle);
	virtual void SetPlaybackRate(float flPlaybackRate);
	virtual void SetWeight(float flWeight);

	virtual int   GetOrder() const;
	virtual int   GetSequence() const;
	virtual float GetCycle() const;
	virtual float GetPrevCycle() const;
	virtual float GetPlaybackRate() const;
	virtual float GetWeight() const;
#endif

	float	m_flLayerAnimtime; //0
	float	m_flLayerFadeOuttime;
	int    _m_fFlags;
	int		m_iActivity;
	int		m_iPriority;
	int     m_nOrder; //20
	int _m_nSequence; //24
	float m_flPrevCycle;
	float m_flWeight; //32
	//CRangeCheckedVar<int, -1, 65535, 0>	m_nSequencef; //24
	//CRangeCheckedVar<float, -2, 2, 0>	m_flPrevCyclef;
	//CRangeCheckedVar<float, -5, 5, 0>	m_flWeightf; //32
	float m_flWeightDeltaRate;

	// used for automatic crossfades between sequence changes
	//CRangeCheckedVar<float, -50, 50, 1>		m_flPlaybackRatef;
	//CRangeCheckedVar<float, -2, 2, 0>		m_flCyclef; //44
	float _m_flPlaybackRate;
	float _m_flCycle; //44

	/*C_BaseAnimatingOverlay*/ CBaseEntity	*m_pOwner; //48
	int	m_nInvalidatePhysicsBits;


	friend class C_BaseAnimatingOverlay;
	friend C_AnimationLayer LoopingLerp(float flPercent, C_AnimationLayer& from, C_AnimationLayer& to);
	friend C_AnimationLayer Lerp(float flPercent, const C_AnimationLayer& from, const C_AnimationLayer& to);
	friend C_AnimationLayer LoopingLerp_Hermite(const C_AnimationLayer& current, float flPercent, C_AnimationLayer& prev, C_AnimationLayer& from, C_AnimationLayer& to);
	friend C_AnimationLayer Lerp_Hermite(const C_AnimationLayer& current, float flPercent, const C_AnimationLayer& prev, const C_AnimationLayer& from, const C_AnimationLayer& to);
	friend void Lerp_Clamp(C_AnimationLayer &val);
	friend int CheckForSequenceBoxChanges(const C_AnimationLayer& newLayer, const C_AnimationLayer& oldLayer);


	inline void SetOwner(CBaseEntity *pOverlay) /*C_BaseAnimatingOverlay*/
	{
		m_pOwner = pOverlay;
	}

	inline CBaseEntity* GetOwner() /*C_BaseAnimatingOverlay*/ { return m_pOwner; }

	inline void Reset()
	{
		if (m_pOwner)
		{
			int nFlags = 0;
			if (_m_nSequence != 0 || m_flWeight != 0.0f)
			{
				nFlags |= BOUNDS_CHANGED;
			}
			if (_m_flCycle != 0.0f)
			{
				nFlags |= ANIMATION_CHANGED;
			}
			if (nFlags)
			{
				m_pOwner->InvalidatePhysicsRecursive(nFlags);
			}
		}

		_m_nSequence = 0;
		m_flPrevCycle = 0;
		m_flWeight = 0;
		_m_flPlaybackRate = 0;
		_m_flCycle = 0;
		m_flLayerAnimtime = 0;
		m_flLayerFadeOuttime = 0;
	}

	inline void SetSequence(int nSequence)
	{
		if (m_pOwner && _m_nSequence != nSequence)
		{
			m_pOwner->InvalidatePhysicsRecursive(BOUNDS_CHANGED);
		}
		_m_nSequence = nSequence;
	}

	inline void SetCycle(float flCycle)
	{
		if (m_pOwner && _m_flCycle != flCycle)
		{
			m_pOwner->InvalidatePhysicsRecursive(ANIMATION_CHANGED);
		}
		_m_flCycle = flCycle;
	}

	inline void SetOrder(int order)
	{
		if (m_pOwner && (m_nOrder != order))
		{
			if (m_nOrder == MAX_OVERLAYS || order == MAX_OVERLAYS)
			{
				m_pOwner->InvalidatePhysicsRecursive(BOUNDS_CHANGED);
			}
		}
		m_nOrder = order;
	}

	inline void SetWeight(float flWeight)
	{
		if (m_pOwner && m_flWeight != flWeight)
		{
			if (m_flWeight == 0.0f || flWeight == 0.0f)
			{
				m_pOwner->InvalidatePhysicsRecursive(BOUNDS_CHANGED);
			}
		}
		m_flWeight = flWeight;
	}

	inline const char* GetLayerName()
	{
		switch (m_nOrder)
		{
			case 0:
				return charenc("MAIN_IDLE_SEQUENCE_LAYER");
			case 1:
				return charenc("AIMSEQUENCE_LAYER1");
			case 2:
				return charenc("AIMSEQUENCE_LAYER2");
			case 3:
				return charenc("FIRESEQUENCE_LAYER");
			case 4:
				return charenc("JUMPLANDSEQUENCE_LAYER");
			case 5:
				return charenc("DEPLOYSEQUENCE_LAYER");
			case 6:
				return charenc("RELOADSEQUENCE_LAYER");
			case 7:
				return charenc("SILENCERCHANGESEQUENCE_LAYER");
			case 8:
				return charenc("GRENADESEQUENCE_LAYER");
			case 9:
				return charenc("FLASHEDSEQUENCE_LAYER");
			case 10:
				return charenc("FLINCHSEQUENCE_LAYER");
			case 11:
				return charenc("TAUNTSEQUENCE_LAYER");
			case 12:
				return charenc("FOOTPLANTSEQUENCE_LAYER");
			case 13:
				return charenc("UNUSED1");
			case 14:
				return charenc("UNUSED2");
		}
		return "";
	}
};

#pragma pack(push, 1)
class C_AnimationLayerFromPacket
{
public:
	int32_t _m_nSequence;
	float m_flWeight;
	int _m_nOrder;
	float _m_flPlaybackRate;
	float _m_flCycle;
};
#pragma pack(pop)