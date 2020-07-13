#pragma once
#include "Includes.h"
#include <deque>
#include "PacketStructs.h"
#include "ResolveSides.h"

enum HeadReason : int {
	HEAD_REASON_NONE = 0,
	HEAD_REASON_BLOOD = 1,
	HEAD_REASON_MOVING = 2,
	HEAD_REASON_BETTER_DAMAGE = 3,
	HEAD_REASON_HITBOX_BETTER = 4,
	HEAD_REASON_MAX
};

enum BaimReason : int
{
	BAIM_REASON_HEAD_FIRED_BULLET = -2,
	BAIM_REASON_HEAD_RESOLVED = -1,
	BAIM_REASON_NONE = 0,
	BAIM_REASON_FORCE = 1,
	BAIM_REASON_LETHAL = 2,
	BAIM_REASON_AIRBORNE_TARGET = 3,
	BAIM_REASON_AIRBORNE_LOCAL = 4,
	BAIM_REASON_MOVING_TARGET = 5,
	BAIM_REASON_FORWARD_TICKBASE = 6,
	BAIM_REASON_CANT_RESOLVE = 7,
	BAIM_REASON_NOT_BODY_HIT_RESOLVED = 8,
	BAIM_REASON_TOO_MANY_MISSES = 9,
	BAIM_REASON_ZERO_TICKS = 10,
	BAIM_REASON_BAD_HITCHANCE = 11,
	BAIM_REASON_NOT_ENOUGH_DAMAGE = 12,
	BAIM_REASON_HEALTH = 13,
	BAIM_REASON_LOCAL_MULTITAP = 14,
	BAIM_REASON_MAX
};

enum ResolveModes
{
	RESOLVE_MODE_AUTOMATIC = 0,
	RESOLVE_MODE_BRUTE_FORCE,
	RESOLVE_MODE_NONE,
	RESOLVE_MODE_MANUAL,
	MAX_RESOLVE_MODES
};
#define RESOLVE_MODE_AUTOMATIC 0
#define RESOLVE_MODE_BRUTE_FORCE 1
#define RESOLVE_MODE_NONE 2
#define RESOLVE_MODE_MANUAL 3

#define MAX_RESOLVE_SIDES_HIT_COUNT 3 //maximum amount we can increase shots hit for a mode to
#define MINIMUM_PLAYERSPEED_CONSIDERED_MOVING 5.0f

class CPlayerrecord;

//Information stored for a specific case of body hit resolving
typedef struct
{
	bool m_bIsBodyHitResolved;
	bool m_bRealYawIsLeft;
	bool m_bIsNearMaxDesyncDelta;
	bool m_bResetSetGoalFeetYaw;
	float m_flLastResolveTime;
	float m_flDesyncDelta;
	Vector m_vecResolvedOrigin;
	float m_NetworkedEyeYaw;
	float m_NetworkedLBY;
	int m_iShotsMissed;

	void Reset()
	{
		m_bIsBodyHitResolved = false;
		m_bRealYawIsLeft = false;
		m_bIsNearMaxDesyncDelta = false;
		m_flDesyncDelta = 0.0f;
		m_flLastResolveTime = 0.0f;
		m_vecResolvedOrigin = { 0.0f,0.0f,0.0f };
		m_NetworkedEyeYaw = 0.0f;
		m_NetworkedLBY = 0.0f;
		m_iShotsMissed = 0;
	}
	bool IsPositive()
	{
		return m_flDesyncDelta > 0.0f;
	}
	bool IsNegative()
	{
		return m_flDesyncDelta < 0.0f;
	}
	ResolveSides GetSide()
	{
		if (m_flDesyncDelta >= 42.0f)
		{
			return ResolveSides::POSITIVE_60;
		}
		else if (m_flDesyncDelta > 10.0f)
		{
			return ResolveSides::POSITIVE_35;
		}
		else if (m_flDesyncDelta <= -42.0f)
		{
			return ResolveSides::NEGATIVE_60;
		}
		else if (m_flDesyncDelta < -10.0f)
		{
			return ResolveSides::NEGATIVE_35;
		}

		return ResolveSides::NONE;
	}
} s_Impact_Info;

class CTickrecord
{
protected:
	void Reset();

public:
	~CTickrecord();
	CTickrecord();
	CTickrecord(CBaseEntity* _Entity);
	
	void Initialize(CBaseEntity* _Entity);

	void GetLocalPlayerVars();
	void GetLocalPlayerVars_Hitscan();

	bool IsRealTick() const;

	bool ScanStillValid() const;

	bool IsVisible();
	bool IsValid(CBaseEntity* Entity);
	int GetMaxWeight() const;
	int GetWeight(CTickrecord* bestrecord = nullptr, int* bestweight = nullptr);

	bool HasMinDamage() const;
	void StoreOppositeResolveSide();

	struct s_LocalVars
	{
		Vector m_EyePos;
		Vector m_ShootPos;
		Vector m_NetOrigin;
		Vector m_Origin;
		Vector m_ViewOffset;
	} LocalVars, LocalVarsScan;

	CBaseEntity* m_pEntity;
	int m_Index;

	SavedHitboxPos m_BestHitbox[HITBOX_MAX];
	SavedHitboxPos m_BestHitbox_ForwardTrack;
	float m_flBestHitboxDamage;
	float m_flBestScaledMinDamage;

	Vector m_vecHeadPos;

	bool m_bMoving;
	bool m_bInAir;

	int m_iTicksChoked;

	bool m_bIsDuplicateTickbase;
	bool m_bTickbaseShiftedBackwards;
	bool m_bTickbaseShiftedForwards;
	bool m_bTeleporting;
	bool m_bStrafing;
	int m_Tickbase;
	float m_flCorrectedSimulationTime; //temp for testing
	float m_SimulationTime;
	float m_OldSimulationTime;
	float m_flFirstCmdTickbaseTime;
	float m_flLastShotTime;
	float m_next_lby_update_time;

	float m_DuckAmount;
	float m_DuckSpeed;

	float m_flPoseParams[MAX_CSGO_POSE_PARAMS];
	float m_flPredictedPoseParameters[MAX_RESOLVE_SIDES][MAX_CSGO_POSE_PARAMS];
	float m_flPredictedGoalFeetYaw[MAX_RESOLVE_SIDES];
	float m_flGoalFeetYaw;
	float m_flCurrentFeetYaw;
	float m_flMaxDesyncMultiplier;
	float m_flEstimatedDesyncMultiplier;
	float m_flDesyncMultiplierRateOfChange;
	float m_flAbsMaxDesyncDelta; //The desync delta calculated just before the final usercmd is animated fully
	float m_flMaxDesyncDelta;
	float m_flEstimatedMaxDesyncDelta;
	Vector m_vecLadderNormal;

	C_AnimationLayer m_AnimLayer[MAX_OVERLAYS];

	bool m_bCachedBones;
	matrix3x4_t m_EntityToWorldTransform;
	Vector m_HitboxWorldMins, m_HitboxWorldMaxs;
	bool m_bCachedFakeAngles;
	QAngle m_angAbsAngles_Fake;
	float m_flBodyYaw_Fake;

	bool m_bRanHitscan;
	bool m_bHeadIsVisible;
	bool m_bBodyIsVisible;
	bool m_bHasBodyPartNotBehindWall;
	bool m_bHasHeadNotBehindWall;
	bool m_bShotAndMissed;

	bool m_Dormant;
	int m_Health;
	int m_Armor;

	QAngle m_LocalAngles;
	QAngle m_AbsAngles;
	QAngle m_Angles;

	QAngle m_EyeAngles;
	QAngle m_ResolvedEyeAngles;
	QAngle m_OriginalEyeAngles;
	float m_EyeYaw;
	float m_flLowerBodyYaw;
	float m_flSpawnTime;
	EHANDLE m_GroundEntity;

	Vector m_Origin;
	Vector m_NetOrigin;
	Vector m_AbsOrigin;

	Vector m_Velocity;
	Vector m_AbsVelocity;
	Vector m_BaseVelocity;

	Vector m_ViewOffset;

	Vector m_Mins;
	Vector m_Maxs;

	int m_MoveType;
	int m_MoveState;

	int m_Flags;
	int m_iResolveMode;
	ResolveSides m_iResolveSide;
	ResolveSides m_iOppositeResolveSide;
	struct s_Impact
	{
		bool m_bIsBodyHitResolved;
		int m_iBodyHitResolveStance;

		//float m_flLastBodyHitResolveTime;
		//float m_flLastImpactProcessRealTime;
		//float m_flLastBloodProcessRealTime;
		//int m_iLastBloodProcessTickCount;

		s_Impact_Info m_ImpactInfo;

		//Vector m_vecDirBlood;
		//Vector m_vecBloodOrigin;

		float m_DesyncAmount;
		float m_DesyncEyeYawAmount;

		void Reset()
		{
			m_bIsBodyHitResolved = false;
			m_iBodyHitResolveStance = 0;
			m_ImpactInfo.Reset();
			m_DesyncAmount = 0.0f;
			m_DesyncEyeYawAmount = 0.0f;
		};
	} Impact;

	bool m_bUsedBodyHitResolveDelta; //Did the resolver use the body hit resolve delta for this record
	bool m_bLBYUpdated;
	bool m_bBalanceAdjust;
	bool m_bIsUsingBalanceAdjustResolver;
	bool m_bIsUsingFreestandResolver;
	bool m_bIsUsingMovingResolver;
	bool m_bIsUsingMovingLBYMeme;
	float m_flTimeSinceLastBalanceAdjust;
	bool m_bFlickedToLBY;
	bool m_bLegit;
	bool m_bForceNotLegit;
	bool m_bRealTick;
	bool m_bFiredBullet;
	unsigned m_FiringFlags;

	int m_iTickcount; //simulationtime + lerptime
	int m_iServerTick; //exact server tick from the clockdrift manager

	float m_flFeetCycle;
	float m_flFeetWeight;
	float m_flOldFeetCycle;
	float m_flOldFeetWeight;

	C_CSGOPlayerAnimState m_animstate;
	PlayerBackup_t m_PlayerBackup;
	CCSGOPlayerAnimState *m_pAnimStateServer[MAX_RESOLVE_SIDES];
};

class CShotrecord
{
public:
	CShotrecord(CBaseEntity* _InflictorEntity, CBaseEntity* _InflictedEntity, CTickrecord* _tickrecord);
	CShotrecord() {};

	CBaseEntity* m_pInflictedEntity;
	CBaseEntity* m_pInflictorEntity;

	CTickrecord* m_Tickrecord;

	matrix3x4_t m_BoneCache[MAXSTUDIOBONES];

	Vector m_vecLocalEyePos;
	Vector m_vecTargetEyePos;
	Vector m_vecLocalPlayerOrigin;
	QAngle m_angLocalEyeAngles;

	int m_iTargetHitgroup;
	int m_iTargetHitbox;

	int m_iActualHitgroup;
	int m_iActualHitbox;

	int m_iResolveMode;
	ResolveSides m_iResolveSide;
	bool m_bEnemyIsNotChoked;
	bool m_bEnemyFiredBullet;
	bool m_bLegit;
	bool m_bForceNotLegit;
	bool m_bDoesNotFoundTowardsStats;
	bool m_bForwardTracked;
	bool m_bUsedBodyHitResolveDelta;
	bool m_bShotAtBalanceAdjust;
	bool m_bShotAtFreestanding;
	bool m_bShotAtMovingResolver;
	bool m_bInflictorIsLocalPlayer;
	int m_iBodyHitResolveStance;

	int m_iTickCountWhenWeShot;

	float m_flCurtime;
	int m_iEnemySimulationTickCount;
	float m_flRealTime;
	float m_flRealTimeAck;
	float m_flLatency;
	QAngle m_ImpactAngles;

	bool m_bAck; //Was the shot record acked at all
	bool m_bMissed; //Did the shot miss a player
	bool m_bTEImpactEffectAcked; //DispatchEffect
	bool m_bTEBloodEffectAcked; //DispatchEffect
	bool m_bImpactEventAcked; //GameEvent
	bool m_bIsInflictor; //if this is true, then this shot record is actually made by an enemy and not the local player, and we are storing info about the inflictor (enemy here) instead of localplayer

	std::vector<int> m_iTicksChokedList;
};

enum ResolverFlags : unsigned
{
	LBYUPDATED = (1 << 0),
	SHOT = (1 << 1),
};

#define DESYNC_RESULTS_FOUND (1 << 0)
#define DESYNC_RESULTS_POSITIVE (1 << 1)
#define DESYNC_RESULTS_NEGATIVE  (1 << 2)
#define DESYNC_RESULTS_DONOTRESOLVE (1 << 3)
#define DESYNC_RESULTS_MISSED (1 << 4)
#define DESYNC_RESULTS_FREESTAND (1 << 5)

//#define DESYNC_RESULTS_OVERWRITTEN (1 << 3)
//#define DESYNC_RESULTS_BRUTEFORCE (1 << 5)

#define BALANCE_ADJUST_FOUND (1 << 0)
#define BALANCE_ADJUST_NEGATIVE (1 << 1)
#define BALANCE_ADJUST_POSITIVE (1 << 2)
#define BALANCE_ADJUST_MISSED (1 << 3)

class CShotrecord;

class CPlayerrecord
{
protected:
	CTickrecord* CreateNewTickrecord(CTickrecord *_previousRecord = nullptr);

	bool IsLegit();
	void UpdateVelocity(CTickrecord *_oldRecord);

	void FSN_UpdateClientSideAnimation(CTickrecord *_currentRecord, CTickrecord *_previousRecord, CTickrecord *_prePreviousRecord);
	unsigned FindDesyncDirection(CTickrecord* _previousRecord, CTickrecord* _currentRecord, CTickrecord *_prePreviousRecord, bool *pDidFind);
	void FSN_AnimateTicks(CTickrecord* _previousRecord, CTickrecord* _currentRecord, CCSGOPlayerAnimState* animstate, float _desyncYawAmount, float _desyncAbsYawAmount, bool Resolve, int mode, bool _RestoreAnimstate = true, bool _RestoreServerAnimLayers = true, bool _RunOnlySetupVelocitySetupLean = false);
	void FSN_AnimatePlayer(CTickrecord* _currentRecord, CTickrecord* _previousRecord, CTickrecord *_prePreviousRecord);
	void FSN_UpdateUnresolvedAnimState();
	void GetDesyncAmountsToAnimate(float(*PredictionDeltasEyeAndDesync)[2], CTickrecord *_currentRecord, CTickrecord *_previousRecord, CTickrecord* _prePreviousRecord, s_Impact_Info* BodyResolveInfo);
	void PredictLowerBodyYaw(CTickrecord* _previousRecord, CTickrecord* _currentRecord);
	void RunStandingResolver(CTickrecord* _previousRecord, CTickrecord* _currentRecord, CTickrecord* _prePreviousRecord, bool& DetectedDesyncSide, s_Impact_Info* BodyResolveInfo);
	//float GetChokedYawFromYaw(float desired_yaw, float pitch, float* goalfeet_yaw = nullptr, float desired_delta = 180.0f);
	CShotrecord* MarkShot(CTickrecord* _record);
	void ResolveMovingPlayers(CTickrecord *_currentRecord, CTickrecord* _previousRecord, bool *PredictedSequenceMatches, float *PredictedCycles, float* PredictedWeights, float* PredictedPlaybackRates, float(*PredictedPoseParameters)[MAX_CSGO_POSE_PARAMS], CCSGOPlayerAnimState*&bestanimstate, float*&bestposes, bool& SearchForJitter);
	void DetectJitter(bool _foundSpecificSide, CTickrecord* _currentRecord, CTickrecord* _previousRecord, float (*PredictedPoseParameters)[MAX_CSGO_POSE_PARAMS], float* &bestposes, CCSGOPlayerAnimState*& bestanimstate);
	ResolveSides GetMovingResolveSide(ResolveSides sidetouse, CTickrecord* _currentRecord, CTickrecord* _previousRecord, s_Impact_Info* BodyResolveInfo);
	void FixJumpFallPoseParameter(CTickrecord *_previousRecord);
public:

	CPlayerrecord();
	void Reset(bool ResetAnimStatePointers = true);
	void StorePlayerState(CTickrecord* record);

	void UpdateInfo(int _index);

	void GetSetupVelocityResults(QAngle& eyeangles, SetUpVelocityResults_t& dest, float time_since_last_animation_update, float curtime, Vector& absVelocity, float duckAmount, float lby);
	
	CTickrecord* GetBestRecord(bool PreferBacktrackUnlessNewestIsBest, std::vector<CTickrecord*>* _excluded = nullptr);
	CTickrecord* GetCurrentRecord();
	CTickrecord* GetPreviousRecord();
	CTickrecord* GetPrePreviousRecord();
	CTickrecord* GetOldestValidRecord();
	CTickrecord* GetNewestValidRecord();
	CTickrecord* FindTickRecord(CTickrecord* record); //returns the record if tickrecord exists

	int GetValidRecordCount();
	static float GetYawFromSide(ResolveSides _Side);
	static ResolveSides GetSideFromYaw(float _Yaw);

	bool CacheBones(float _time, bool _force, CTickrecord* _record) const;

	bool IsValidTarget() const;
	FarESPPlayer *GetFarESPPacket(int index = 0);
	FarESPPlayer* IsValidFarESPTarget();
	FarESPPlayer* IsValidFarESPAimbotTarget();
	CSGOPacket *GetServerSidePacket();
	CSGOPacket* IsValidServerSideTarget();
	CSGOPacket *IsValidServerSideAimbotTarget();

#ifdef _DEBUG
	void OutputAnimations();
	void WriteToAnimationFile(char* str);
	C_AnimationLayer lastoutput_layers[MAX_OVERLAYS];
#endif
	void ManualPredict();
	int GetButtons(FarESPPlayer* record, FarESPPlayer* previousrecord);
	void SimulatePlayer_FarESP(FarESPPlayer *_currentRecord, FarESPPlayer *_previousRecord, int time, bool SimulateInPlace, bool CalledfromCreateMove, bool useDirection, float &angDirection, float &angChange);
	void SimulatePlayer(CTickrecord *_currentRecord, CTickrecord *_previousRecord, int time, bool SimulateInPlace, bool CalledFromCreateMove, bool animate, bool isfinaltick, ResolveSides resolvemode, float pitch = 0.0f, float yaw = 0.0f, float* angDirection = nullptr, float *angChange = nullptr);
	void FSN_UpdatePlayer();

	void CM_RestoreAnimations(CTickrecord* _target) const;
	void CM_RestoreNetvars(CTickrecord* _target) const;
	bool CM_BacktrackPlayer(CTickrecord* _target, bool include_bones = true);
	bool FarESPPredict();
	void OnCreateMove();
	void CM_RestorePlayer();

	//SetupVelocity
	float m_next_lby_update_time;
	Vector m_lastvelocity;

	struct s_DataUpdateVars
	{
		float m_flPrePoseParams[MAX_CSGO_POSE_PARAMS];

		C_AnimationLayer m_PreAnimLayers[MAX_OVERLAYS];
		C_AnimationLayer m_PostAnimLayers[MAX_OVERLAYS];

		float m_flPreBoneControllers[MAXSTUDIOBONECTRLS];
		float m_flPostBoneControllers[MAXSTUDIOBONECTRLS];

		float m_flPreSimulationTime;

		Vector m_vecPreNetOrigin;
		Vector m_vecPostNetOrigin;

		QAngle m_angPreAngleRotation;
		QAngle m_angPostAngleRotation;

		QAngle m_angPreAbsAngles;
		QAngle m_angPostAbsAngles;

		QAngle m_angPreLocalAngles;
		QAngle m_angPostLocalAngles;

		QAngle m_angPreEyeAngles;
		QAngle m_angPostEyeAngles;

		float  m_flPreVelocityModifier;
		float  m_flPostVelocityModifier;

		float  m_flPreShotTime;
		float  m_flPostShotTime;

		float m_flPreFeetCycle;
		float m_flPostFeetCycle;

		float m_flPreFeetWeight;
		float m_flPostFeetWeight;

		C_AnimationLayer m_LastOutputAnimLayers[MAX_OVERLAYS];
	} DataUpdateVars;

	struct s_EntityHooks
	{
		//0x0
		HookedEntity* BaseEntity;
		//0x4
		HookedEntity* ClientRenderable;
		//0x8
		HookedEntity* ClientNetworkable;

		bool m_bHookedBaseEntity;
		bool m_bHookedClientRenderable;
		bool m_bHookedClientNetworkable;
		void* m_pBaseAnimState;
		VTHook* m_pBaseAnimStateHook;

		void OnDLLInit()
		{
			m_bHookedBaseEntity = false;
			m_bHookedClientRenderable = false;
			m_bHookedClientNetworkable = false;
			BaseEntity = nullptr;
			ClientRenderable = nullptr;
			ClientNetworkable = nullptr;
			m_pBaseAnimState = nullptr;
			m_pBaseAnimStateHook = nullptr;
		}
	} Hooks;

	struct s_NetUpdateChanged
	{
		bool Origin;
		bool Animations;
		bool FeetCycle;
		bool FeetWeight;
		bool ShotTime;
		bool EyeAngles;
		bool SimulationTime;

		void Reset()
		{
			Origin = Animations = FeetCycle = FeetWeight = ShotTime = EyeAngles = SimulationTime = false;
		}
	} Changed;

	struct s_Shot
	{
		int m_iTickHitWall;
		int m_iTickHitPlayer;

		int m_iTargetHitbox;
		int m_iTargetHitgroup;
		QAngle m_angEyeAngles;
		Vector m_vecMainImpactPos;
		std::list<Vector>m_vecImpactPositions;
		bool m_bReceivedExactImpactPos; //The exact spot of the player, not a wall

		void Reset()
		{
			m_iTickHitWall = m_iTickHitPlayer = m_iTargetHitbox = m_iTargetHitgroup = 0;
			m_bReceivedExactImpactPos = false;
			m_angEyeAngles.Init();
		}
	};

	s_Shot LastShot;
	s_Shot LastShotAsEnemy;

	const enum ImpactResolveStances
	{
		Standing = 0,
		Running,
		Walking,
		MAX_IMPACT_RESOLVE_STANCES
	};

	struct s_Impact
	{
		float m_flLastBodyHitResolveTime;
		float m_flLastImpactProcessRealTime;
		float m_flLastBloodProcessRealTime;
		int m_iLastBloodProcessTickCount;

		s_Impact_Info ResolveStances[ImpactResolveStances::MAX_IMPACT_RESOLVE_STANCES];

		Vector m_vecDirBlood;
		Vector m_vecBloodOrigin;

		void Reset();
	} Impact;

	bool GetBodyHitDesyncAmount(float* _desyncAmount, float* _desyncEyeAmount, CTickrecord* _currentRecord, CTickrecord* _previousRecord, CTickrecord* _prePreviousRecord, s_Impact_Info* BodyResolveInfo);
	s_Impact_Info *GetBodyHitResolveInfo(CTickrecord* tickrecord = nullptr, bool is_saving_data = false);
	ImpactResolveStances GetBodyHitStance(CTickrecord* tickrecord = nullptr);
	void ApplyBodyHitResolver(s_Impact_Info* BodyResolveInfo, float _desyncAmount, CCSGOPlayerAnimState* animstate, CTickrecord* _currentRecord, CTickrecord* _previousRecord);
	void SetResolveSide(int side, const char* reason);

	std::string GetBaimReasonString();

	CTickrecord* m_TargetRecord;
	CTickrecord* m_RestoreTick = nullptr;
	bool m_RestoreTickIsDynamic = false;

	int m_iTickcount;
	int m_iTickcount_ForwardTrack;
	bool m_bCountResolverStatsEvenWhenForwardtracking;
	int m_ValidRecordCount;
	float m_flTotalInvalidRecordTime;

	CBaseEntity* m_pEntity;
	CBaseCombatWeapon* m_pWeapon;

	float m_flSimulationTime;
	float m_flFirstCmdTickbaseTime;
	int m_iFirstCmdTickbase;
	float m_flNewestSimulationTime; //set after updating everything
	int m_iNewestTickbase; //set after updating everything

	//death information
	CBaseEntity* m_pKilledByEntity;
	int m_iLastKilledTickcount;

	bool m_bNeedsRestoring;
	bool m_bIsUsingFarESP;
	bool m_bIsUsingServerSide;
	bool m_bIsSpectating;
	bool m_bMovingLBYMemeFlipState[MAX_RESOLVE_SIDES];
	Vector m_vecLastFarESPPredictedPosition = { 0,0,0 };
	Vector m_vecLastFarESPPredictedMins = { 0,0,0 };
	Vector m_vecLastFarESPPredictedMaxs = { 0,0,72.0f };
	QAngle m_vecLastFarESPPredictedAbsAngles = { 0,0,0 };

	bool m_bNetUpdate;
	bool m_bNetUpdateSilent;

	player_info_t m_PlayerInfo;
	WeaponInfo_t m_WeaponInfo;
	bool m_bHasWeaponInfo;

	bool m_bIsBot;
	bool m_bConnected;

	bool m_bAllowMovingResolver;
	bool m_bAllowJitterResolver;
	bool m_bAllowBodyHitResolver;

	bool m_bAlreadyDead;
	bool m_bAlreadySpectatingOrDormant;
	bool m_bAlreadyResetRecord;
	bool m_bIsDuplicateTickbase;
	bool m_bTickbaseShiftedBackwards;
	bool m_bTickbaseShiftedForwards;
	int m_iTicksChoked;
	int m_iTicksSinceLastServerUpdate;
	int m_nServerTickCount;
	int m_nTickbase;

	int m_iShotsMissed;
	int m_iShotsMissed_BalanceAdjust;
	int m_iShotsMissed_MovingResolver;
	int m_iResolveMode;
	int m_iOldResolveMode;
	int m_iBackupOldResolveMode;
	unsigned int m_BackupDesyncresults;
	unsigned int m_BackupBalanceAdjustResults;
	ResolveSides m_iOldResolveSide;
	ResolveSides m_iBackupOldResolveSide;
	ResolveSides m_iResolveSide;
	bool m_bOldForceNotLegit;
	bool m_bResolved;
	float m_flLastFlipSideTime;
	
	//resolve states
	unsigned m_DesyncResults;
	unsigned m_BalanceAdjustResults;

	bool m_bForceNotLegit;

	bool m_bSpectating;
	bool m_bImmune;
	bool m_bLocalPlayer;
	bool m_bAwaitingFlashedResult;
	int m_ivphysicsCollisionState;

	float m_flNextLegitCheck;
	bool m_bLegit;
	bool m_bMoving;
	bool m_bIsHoldingDuck;
	bool m_bIsFakeDucking;
	bool m_bTeleporting;

	bool m_bJitteringYaw;
	bool m_bJitteringYawIsPredictable;

	float m_flSpawnTime;

	QAngle m_angPristineEyeAngles;
	float m_flPristineVelocityModifier;
	QAngle m_angEyeAnglesNotFiring;
	QAngle m_angEyeAnglesNotFiring_Choked;
	float m_flLastShotSimtime;

	QAngle m_angLastShotAngle;

	float m_flLowerBodyYaw;
	float m_flLastLowerBodyYaw;
	float m_flLastLBYUpdateTime;
	float m_flLastBalanceAdjust;
	float m_flLastBalanceAdjustEyeYaw;
	float m_flLastDetectedEyeDelta;
	float m_flTimeSinceLastBalanceAdjust;
	bool m_bIsConsistentlyBalanceAdjusting;
	bool m_bWasConsistentlyBalanceAdjusting;
	bool m_bIsBreakingLBYWithLargeDelta;
	bool m_bPredictLBY;
	float m_flLastMovingResolverDelta;
	float m_flLastTimeResolvedFromMoving;
	float m_flLastTimeDetectedJitter;
	ResolveSides m_nLastMovingResolveSide;
	float m_flTimeSinceStartedPressingUseKey;
	
	//PlayerBackup_t fakeplayerbackup_tickbytick;
#ifdef USE_SERVER_SIDE
	float m_flLastServerSidePrintSimulationTime;
	float m_flLastServerAbsYawChangeTime;
	float m_flLastServerAbsYaw;
	float m_flLastServerLBYChangeTime;
#endif
	float m_fake_curfeetyaw;
	float m_fake_goalfeetyaw;
	float m_fake_last_animation_update_time;
	float m_fake_duckamount;
	Vector m_fake_velocity;

	QAngle fakeabsangles;
	float fakebodyyaw;

	Vector m_vecCurNetOrigin;
	Vector m_vecOBBMins;
	Vector m_vecOBBMaxs;

	bool m_bDormant;

	char steamid[33];
	uint32_t hashedsteamid;
	std::recursive_mutex faresprecordmutex;
	std::mutex serversidemutex;
	std::deque<FarESPPlayer*> FarESPPackets;
	int m_nLastFarESPPacketServerTickCount;
	std::deque<CSGOPacket>ServerSidePackets;
	CSGOPacket LastServerSidePacket;
	bool m_bIsValidFarESPAimbotTarget;
	bool m_bIsValidServerSideAimbotTarget;
	int m_iLifeState;
	CTickrecord m_GeneratedTickRecord;

	std::deque<CShotrecord*>ShotRecords;
	//CThreadFastMutex ShotMutex;
	std::deque<CTickrecord*> m_Tickrecords;
	CCSGOPlayerAnimState* m_pAnimStateServer[MAX_RESOLVE_SIDES];
	bool m_bAllowAnimationEvents;
	int m_iBaimReason;
	int m_iHeadReason;
	std::deque<int> m_iTicksChokedHistory;
};

class CLagCompensation_imi
{
public:
	/// returns true if the given tickcount is valid
	bool IsTickValid(int tick);

	/// returns true if the given tickrecord is valid
	bool IsTickValid(CTickrecord* record);

	/// creates a new tickrecord if needed
	void FSN_UpdatePlayers();

	/// backtracks all players to their best tick
	void OnCreateMove();

	/// restores all backtracked player back to their original tick
	void CM_RestorePlayers();

	void CM_PredictPlayer(CPlayerrecord *_player);

	/// returns the lerptime
	float GetLerpTime();

	/// returns a pointer to the Playerrecord
	CPlayerrecord* GetPlayerrecord(CBaseEntity* _Entity);
	CPlayerrecord* GetPlayerrecord(int i);

	/// sets cmd->tickcount accordingly
	void AdjustCMD(CBaseEntity* _Entity);

	/// gets the exact velocity the server calculates for players in setupvelocity and other anim functions
	Vector GetSmoothedVelocity(float delta, Vector& newvel, Vector& prevvel);

	ResolveSides GetOppositeResolveSide(ResolveSides side);
	ResolveSides GetOppositeResolveSideWithNewDelta(ResolveSides side);
	bool IsResolveSide35(ResolveSides side);
	bool IsResolveSide60(ResolveSides side);
	bool IsResolveSidePositive(ResolveSides side);
	bool IsResolveSideNegative(ResolveSides side);
};

extern CLagCompensation_imi g_LagCompensation;
extern CPlayerrecord m_PlayerRecords[MAX_PLAYERS + 1];
extern const int m_pParallelProcessIndexes[MAX_PLAYERS];

struct ImpactResolverQueue_t
{
	CBaseEntity* m_EntityHit;
	CPlayerrecord *pCPlayer;
	CShotrecord m_ShotRecord;
	::ImpactResolverQueue_t() { };
	::ImpactResolverQueue_t(CBaseEntity* entityhit, CPlayerrecord* record, CShotrecord& shotrecord)
	{
		m_EntityHit = entityhit;
		pCPlayer = record;
		m_ShotRecord = shotrecord;
	}
};

struct ImpactEventQueue_t
{
	CBaseEntity *m_EntityHit;
	float m_flRealTime;
	int m_iActualHitbox;
	::ImpactEventQueue_t() { };
	::ImpactEventQueue_t(CBaseEntity* entityhit, float realtime, int actualhitbox)
	{
		m_EntityHit = entityhit;
		m_flRealTime = realtime;
		m_iActualHitbox = actualhitbox;
	}
};

extern std::vector<ImpactResolverQueue_t> g_QueuedImpactResolveEvents;
extern std::deque<ImpactEventQueue_t> g_QueuedImpactEvents;
