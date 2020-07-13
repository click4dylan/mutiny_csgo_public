#pragma once

#include "GameMemory.h"
#include "Interfaces.h"
#include "BaseEntity.h"
#include "BaseCombatWeapon.h"
#include "Netchan.h"
#include "Keys.h"
#include "CPlayerrecord.h"
#include <map>

extern CThreadMutex RENDER_MUTEX;

enum MissFlags_t : int
{
	MISS_FROM_SPREAD = (1 << 0),
	MISS_FROM_BAD_HITGROUP = (1 << 1),
	MISS_FROM_BADRESOLVE = (1 << 2),
	MISS_RAY_OFF_TARGET = (1 << 3),
	MISS_AUTOWALL = (1 << 4),
	MISS_SHOULDNT_HAVE_MISSED = (1 << 5),
	HIT_PLAYER = (1 << 7),
	HIT_IS_RESOLVED = (1 << 8)
};


class QAngle;

struct MyWeapon
{
	int iItemDefinitionIndex;
	float flRange;
	bool IsGun;
	bool IsShotgun;
	bool IsPistol;
	bool IsSniper;
	bool IsKnife;
	bool IsGrenade;
	bool IsC4;
	bool IsAutoSniper;
	bool IsRevolver;
	bool IsScopedWeapon;
	bool IsBurstableWeapon;
	bool IsFullAuto;
	bool IsTaser;
	bool CanLegitRCS;
	int LegitRCSIndex;
	int LegitRCSNumIndexes;
	double dbNextRCSIndexChangeTime;
	double dbLastRCS;
	char LastMode;
	QAngle totalrecoilsubtracted;
	QAngle totalrecoiladded;
	QAngle recoilslope;
};

struct UncompressedNetvars_t
{
	QAngle punchangle;
	Vector punchvelocity;
	Vector viewoffset;
	int tickbase;
};

struct BackupMatrixStruct
{
	float m_SimulationTime;
	int m_TicksAllowedForProcessing;
	matrix3x4_t m_Matrix[MAXSTUDIOBONES];
	Vector m_Origin;
	QAngle m_Angles;

	BackupMatrixStruct(float simulationtime, int ticksallowedforprocessing, matrix3x4_t matrix[MAXSTUDIOBONES], Vector& origin, QAngle& angles);
};

struct ShotResult
{
	CBaseEntity* m_pInflictorEntity = nullptr;
	CBaseEntity* m_pInflictedEntity = nullptr;
	float m_flTimeCreated = -1.f;
	unsigned m_MissFlags = 0;
	int m_ResolveMode = -1;
	ResolveSides m_ResolveSide = INVALID_RESOLVE_SIDE;
	int m_ResolveSideHits[MAX_RESOLVE_SIDES]{};
	int m_ResolveSidesToIgnore[MAX_RESOLVE_SIDES]{};
	int m_iHitgroupShotAt = -1;
	int m_iHitgroupHit = -1;
	int m_iDamageGiven = -1;
	bool m_bEnemyFiredBullet = false;
	bool m_bEnemyIsNotChoked = false;
	bool m_bCountedMiss = false;
	bool m_bCountedBodyHitMiss = false;
	bool m_bAcked = false;
	bool m_bAwaitingImpact = false;
	bool m_bLegit = false;
	bool m_bForceNotLegit = false;
	bool m_bDoesNotFoundTowardsStats = false;
	bool m_bUsedBodyHitResolveDelta = false;
	int m_iBodyHitResolveStance = 0;
	bool m_bIsInflictor = false;
	bool m_bInflictorIsLocalPlayer = true;
	bool m_bForwardTracked = false;
	bool m_bShotAtBalanceAdjust = false;
	bool m_bShotAtFreestanding = false;
	bool m_bShotAtMovingResolver = false;
	int m_iTickCountWhenWeShot = -1;
	int m_iEnemySimulationTickCount = -1;

	ShotResult() = default;

	std::string get_miss_reason(bool fake_fire) const;
};

//Local Player
class MyPlayer
{
public:
	MyPlayer::MyPlayer();
	void ClearVariables();
	void OnInvalid();
	void CopyShootPositionVars(Vector& Dest_EyePosition_Forward, Vector& Dest_HeadPosition_Forward);
	void WriteShootPositionVars(Vector& Source_EyePosition_Forward, Vector& Source_HeadPosition_Forward);
	void FixShootPosition(QAngle& targeteyeangles, bool already_animated);
	void BeginEnginePrediction(CUserCmd *cmd, bool backup_player_state = true, int custom_start_tick = 0);
	void FinishEnginePrediction(CUserCmd *cmd, bool backup_player_state = true);
	void CorrectAutostop(CUserCmd* cmd);
	bool IsManuallyFiring(CBaseCombatWeapon *weapon) const;
	void UpdateCurrentWeapon();
	CBaseEntity* Get(MyPlayer* dest);
	CBaseEntity* GetLocalPlayer();
	void DrawTextInFrontOfPlayer(float flStayTime, float Z, char *text, ...);
	void ResetWeaponVars();
	void OnCreateMove();
	CBaseEntity* CreateShotRecord();
	void PredictLowerBodyYaw(QAngle& angle);
	void UpdateAnimations(QAngle& angle, bool& waiting_on_lby_update_from_server, float& last_real_lby);
	void CorrectTickBase();
	bool IsCurrentTickAnLBYUpdate(bool &ismoving, bool &inair);
	bool IsNextAnimUpdateAnLBYUpdate(bool *current_is_lby_update = nullptr);
	bool IsAnimUpdateAnLBYUpdate(float time, bool* current_is_lby_update = nullptr);
	void SaveUncompressedNetvars();
	void ApplyUncompressedNetvars();
	void SnapAttachmentsToCurrentPosition();
	bool GetDesyncStyle();
	bool ShouldDelaySway();
	float GetSwayDelay();
	bool IsSwaying();
	float GetSwaySpeed();

	void GetAverageFrameTime();

	// todo: nit; holy fucking shit do this better somehow by doing the same speed checks but return a different boolean or something
	// maybe even do it through variable?
	bool Config_IsDesyncing() const;
	bool Config_IsJitteringYaw() const;
	float Config_GetDesyncAmount() const;
	float Config_GetYawJitter() const;
	float Config_GetLBYDelta() const;
	int Config_GetPitch() const;
	int Config_GetYaw() const;
	float Config_GetYawAdd() const;
	float Config_GetYawSpin() const;
	float Config_GetCustomPitch() const;
	bool Config_IsAntiaiming() const;
	bool Config_IsFreestanding() const;
	bool Config_IsWallDetecting() const;
	bool Config_IsFakelagging(int* move_type = nullptr) const;
	int Config_GetFakelagMode() const;
	int Config_GetFakelagMin() const;
	int Config_GetFakelagMax() const;
	int Config_GetFakelagStatic() const;
	bool Config_IsDisruptingFakelag() const;
	float Config_GetFakelagDisruptChance() const;

	bool IsAllowedUntrusted() const;

	//Engine Prediction
	bool bInPrediction;
	bool bInPrediction_Start; // are we running BeginEnginePrediction and haven't finished running gamemovement yet

	private:

	//Netcode related
	UncompressedNetvars_t UncompressedNetvars[150];

	public:

	//Entity related
	CBaseEntity *Entity;

	//Camera pos in the world
	Vector LastCameraPosition = Vector(0,0,0);

	//Generic
	bool IsInCompetitive;

	//Angle related
	QAngle CurrentEyeAngles; //Current real eye angles without any antiaim
	QAngle FinalEyeAngles; //Eye angles to send to the server
	QAngle LastSentEyeAngles;
	int m_iLastAASwapTick;

	float m_flAverageFrameTime;

	//TODO: add this back from V3

	//Ghost camera variables
	//bool bIsInGhostCamera;
	//bool bSetGhostCameraForwardAngles;
	//float flMissCameraEndTime;
	//QAngle GhostCamForwardAngles;
	//Vector LastViewPos;

	//button press features
	bool bFakeWalking = false;
	bool IsFakeDucking = false;
	bool IsTeleporting = false;

	//tickbase manipulation
	bool ApplyTickbaseShift = false;
	bool WaitForTickbaseBeforeFiring = false;
	bool PredictionStateIsShifted = false;
	bool LastShotWasShifted = false;
	bool UseDoubleTapHitchance = false;
	bool CanShiftShot = false;
	bool InActualPrediction = false;

	bool RunningFakeAngleBones = false;

	//netcode related
	int Last_OutSequenceNrSent; //Last m_nOutSequenceNr sent to the server, only set if SendDatagram was hooked
	int Last_InSequenceNrSent; //Last m_nInSequenceNr sent to the server, only set if SendDatagram was hooked
	int Last_ChokeCountSent; //Last choked count sent to the server
	int Last_Server_InSequenceNrSent;
	int Last_Server_OutSequenceNrSent;
	int Last_Server_ChokeCountSent;
	int Last_Packet_Size;

	//netvars and related variables
	Vector Current_Origin;
	Vector Previous_Origin;
	float LowerBodyYaw = 0.0f;
	float flSimulationTime;
	float m_flOldCurtime;
	int m_nOldTickbase;
	int m_nTickBase_Pristine = 0;
	BOOL IsAlive;
	BOOL IsFakeLaggingOnPeek;

	//Eye position
	Vector HeadPosition;
	Vector HeadPosition_Forward;
	Vector EyePosition_Forward;
	Vector EyePosition;
	Vector ShootPosition;

	//Third person matrixes
	matrix3x4_t RealAngleMatrixFixed[MAXSTUDIOBONES];
	matrix3x4_t RealAngleMatrix[MAXSTUDIOBONES];
	matrix3x4_t RealAngleEntityToWorldTransform;
	matrix3x4_t FakeAngleMatrix[MAXSTUDIOBONES];
	matrix3x4_t FakeAngleEntityToWorldTransform;
	Vector LastAnimatedOrigin;
	QAngle LastAnimatedAngles;
	Vector LastAnimatedOrigin_Fake;
	QAngle LastAnimatedAngles_Fake;

	//Weapon
	CBaseCombatWeapon *CurrentWeapon;
	CBaseCombatWeapon *LastWeapon;
	int CurrentWeaponItemDefinitionIndex;
	int LastWeaponItemDefinitionIndex;
	bool WeaponWillFireBurstShotThisTick;
	MyWeapon WeaponVars;

	//Input related
	//int MousePosX;
	//int MousePosY;
	//int LastMousePosX;
	//int LastMousePosY;

	bool CalledPlayerHurt;

	//animations
	bool m_bDesynced;
	float m_flDesynced;
	float m_flFlashedDuration = 0.0f;
	float m_curfeetyaw;
	float m_goalfeetyaw;
	float m_flLastAnimationUpdateTime;
	float m_next_lby_update_time;
	float m_predicted_lowerbodyyaw;
	float m_predicted_lowerbodyyaw_lastsent;
	float m_last_lby_update_time;
	float m_last_duckamount;
	Vector m_lastvelocity;
	float m_spawntime;
	bool m_wasonground;
	C_AnimationLayer m_AnimLayers[15];

	//int m_iFirstCommandTickbase;
	int m_flFirstCommandLastInjuryTime;
	QAngle m_cmdAbsAngles[150];

	//persistent data
	std::map<float, ShotResult> m_ShotResults;
	CShotrecord* ShotRecord;
	CThreadFastMutex ShotMutex;

	//Engine Prediction
	int originalrandomseed;
	int originalflags;
	float originalframetime;
	float originalcurtime;
	CMoveData NewMoveData;
	CUserCmd *originalcurrentcommand;
	CUserCmd originalplayercommand;
	CBaseEntity* originalpredictionplayer;
	float originalaccuracypenalty;
	float originalrecoilindex;
	PlayerBackup_t playerstate;
	bool m_bJumping;
	bool m_bDeploying;
	bool m_bPlantingBomb;
	std::atomic<bool> m_bPredictionError = false;
	int m_iPredictionErrorMovementDir = 0;

	PlayerBackup_t fake_playerbackup;
	PlayerBackup_t real_playerbackup;
	PlayerBackup_t real_playerbackup_lastsent;
	PlayerBackup_t real_playerbackup_lastsent_pristine;
	PlayerBackup_t real_playerbackups[150];
	QAngle eyeangles_lastsent_pristine;
#ifdef USE_SERVER_SIDE
	SmallForwardBuffer m_RealMatrixBackups;
	SmallForwardBuffer m_FakeMatrixBackups;
#endif

	CThreadMutex NetvarMutex;
	CThreadFastMutex PlayerMutex;

	CThreadFastMutex m_UserCommandMutex;
	std::vector<int> m_vecUserCommands;

	std::vector<int> m_PredictionAnimationEventQueue; //queue of animation events to run before animating the local player

	struct ManualFireShot
	{
		CBaseEntity *Entities[MAX_PLAYERS + 1];
		matrix3x4_t BoneMatrixes[MAXSTUDIOBONES][MAX_PLAYERS + 1];
		int m_iNumBones[MAX_PLAYERS + 1];
		matrix3x4_t m_EntityToWorldTransform[MAX_PLAYERS + 1];
		Vector m_NetOrigin[MAX_PLAYERS + 1];
		float m_flBodyYaw[MAX_PLAYERS + 1];
		float m_flAbsYaw[MAX_PLAYERS + 1];
		float m_flNetEyeYaw[MAX_PLAYERS + 1];
		float m_flSpeed[MAX_PLAYERS + 1];
		Vector m_vecLocalEyePosition;
		float m_flLastManualShotRealTime;
		//Ack
		CBaseEntity* m_pEntityHit;
		Vector m_vecLastImpactPos;
		float m_flLastBodyHitResolveTime;
		int m_iTickHitWall;
		int m_iTickHitPlayer;
		int m_iActualHitgroup;
		int m_iActualHitbox;
	} ManualFireShotInfo;

	int LastAmmo;
	bool FakeFired;
};
extern MyPlayer LocalPlayer;
