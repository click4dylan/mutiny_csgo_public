#pragma once
#include "ResolveSides.h"

class CPlayerrecord;
class CBaseEntity;

class PlayerBackup_t
{
public:
	PlayerBackup_t() {};
	void Get(CBaseEntity* pEntity);
	void GetBoneData(CBaseEntity* pEntity);
	PlayerBackup_t::PlayerBackup_t(CBaseEntity* pEntity) { Get(pEntity); }
	void RestoreData(bool bRestoreAnimations = true, bool bRestoreBones = true);
	void RestoreBoneData();

	//Data

	CPlayerrecord* playerRecord;
	CBaseEntity *Entity;
	C_CSGOPlayerAnimState *m_pClientAnimState;
	C_CSGOPlayerAnimState m_BackupClientAnimState;
	bool m_bHasBackupServerAnimState[MAX_RESOLVE_SIDES];
	CCSGOPlayerAnimState m_BackupServerAnimState[MAX_RESOLVE_SIDES];
	CBoneAccessor *accessor;
	matrix3x4_t CachedBoneMatrices[MAXSTUDIOBONES];
	C_AnimationLayer m_AnimLayers[MAX_OVERLAYS];
	float m_flPoseParameters[MAX_CSGO_POSE_PARAMS];
	float LowerBodyYaw;
	QAngle NetEyeAngles;
	QAngle EyeAngles;
	Vector Origin;
	Vector NetworkOrigin;
	Vector OldOrigin;
	Vector AbsOrigin;
	//QAngle RenderAngles;
	QAngle NetworkAngles;
	QAngle AbsAngles;
	QAngle LocalAngles;
	Vector Velocity;
	Vector AbsVelocity;
	bool Dormant;
	int LifeState;
	bool DeadFlag;
	int TicksChoked;
	int Armor;
	int Health;
	bool Helmet;
	int TickBase;
	float PostPoneFireReadyTime;
	bool IsStrafing;
	bool Teleporting;
	bool ReceivedGameServerAck;
	int ServerTickWhenAcked;
	int WeaponItemDefinitionIndex;
	int WeaponClassID;
	EHANDLE WeaponHandle;

	int OcclusionCheckFlags;
	int OcclusionCheckFrameCount;
	int ReadableBones;
	int WritableBones;
	unsigned long ModelBoneCounter;
	float LastBoneSetupTime;
	int AccumulatedBoneMask;
	int PreviousBoneMask;
	int SetupBonesFrameCount;
	int CachedBonesCount;
	int giModelBoneCounter;

	float DuckAmount;
	float LastDuckAmount;
	float DuckSpeed;
	float SimulationTime;
	float OldSimulationTime;
	int Flags;
	int EFlags;
	localdata_t LocalData;
	float FallVelocity;
	int MoveType;
	MoveCollide_t MoveCollide;
	float VelocityModifier;
	QAngle ViewPunch;
	QAngle Punch;
	Vector PunchVel;
	Vector WaterJumpVel;
	float WaterJumpTime;
	float GroundAccelLinearFracLastTime;
	bool GameMovementOnGround;
	float Stamina;
	Vector BaseVelocity;
	float SwimSoundTime;
	//float JumpTime;
	bool InDuckJump;
	bool DuckUntilOnGround;
	Vector VecLadderNormal;
	float TimeNotOnLadder;
	float Gravity;
	unsigned char WaterLevel;
	char WaterType;
	float SurfaceFriction;
	Vector ViewOffset;
	bool IsWalking;
	int MoveState;
	//int DuckTimeMsecs;
	//int DuckJumpTimeMsecs;
	//int JumpTimeMsecs;
	float StepSoundTime;
	//bool Ducking;
	//bool Ducked;
	bool DuckOverride;
	Vector2D DuckingOrigin;
	//float LastDuckTime;
	float StuckLast;
	int SurfaceProps;
	surfacedata_t *SurfaceData;
	char TextureType;
	EHANDLE GroundEntity;
	float MaxSpeed;
	unsigned long RefEHandle;
	Vector Mins;
	Vector Maxs;
	int LastHitgroup;
	int RelativeDirectionOfLastInjury;
	int FireCount;
	float TimeOfLastInjury;
	int vphysicsCollisionState;
	CBaseEntity::ButtonState_t ButtonState;
};
