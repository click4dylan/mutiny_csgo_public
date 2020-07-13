#include "precompiled.h"
#include "Includes.h"
#include "CPlayerrecord.h"

void PlayerBackup_t::Get(CBaseEntity* pEntity)
{
	ReceivedGameServerAck = false;
	Teleporting = false;
	ServerTickWhenAcked = 0;
	LastDuckAmount = FLT_MAX;
	playerRecord = g_LagCompensation.GetPlayerrecord(pEntity);
	Entity = pEntity;
	m_pClientAnimState = Entity->GetPlayerAnimState();

	Entity->CopyPoseParameters(m_flPoseParameters);
	Entity->CopyAnimLayers(m_AnimLayers);

	if (m_pClientAnimState)
		m_BackupClientAnimState = *m_pClientAnimState;

	for (int i = 0; i < MAX_RESOLVE_SIDES; ++i)
	{
		m_bHasBackupServerAnimState[i] = false;
		if (playerRecord->m_pAnimStateServer[i])
		{
			m_BackupServerAnimState[i] = *playerRecord->m_pAnimStateServer[i];
			m_bHasBackupServerAnimState[i] = true;
		}
	}

	GetBoneData(pEntity);

	TicksChoked = playerRecord->m_TargetRecord ? playerRecord->m_TargetRecord->m_iTicksChoked : playerRecord->m_iTicksChoked;
	Dormant = Entity->GetDormant();
	LifeState = Entity->GetLifeState();
	DeadFlag = Entity->GetDeadFlag();
	Armor = Entity->GetArmor();
	Health = Entity->GetHealth();
	Helmet = Entity->HasHelmet();

	LowerBodyYaw = Entity->GetLowerBodyYaw();
	NetEyeAngles = Entity->GetEyeAngles();
	EyeAngles = *Entity->EyeAngles();
	Origin = Entity->GetLocalOriginDirect();
	NetworkOrigin = Entity->GetNetworkOrigin();
	OldOrigin = Entity->GetOldOrigin();
	AbsOrigin = Entity->GetAbsOriginDirect();
	//RenderAngles = Entity->GetRenderAngles();
	NetworkAngles = Entity->GetAngleRotation();
	AbsAngles = Entity->GetAbsAnglesDirect();
	LocalAngles = Entity->GetLocalAnglesDirect();
	Velocity = Entity->GetVelocity();
	AbsVelocity = Entity->GetAbsVelocityDirect();
	DuckAmount = Entity->GetDuckAmount();
	DuckSpeed = Entity->GetDuckSpeed();
	SimulationTime = Entity->GetSimulationTime();
	OldSimulationTime = Entity->GetOldSimulationTime();
	Flags = Entity->GetFlags();
	EFlags = Entity->GetEFlags();
	LocalData = Entity->GetLocalData()->localdata;
	FallVelocity = Entity->GetFallVelocity();
	MoveType = Entity->GetMoveType();
	MoveCollide = Entity->GetMoveCollide();
	VelocityModifier = Entity->GetVelocityModifier();
	ViewPunch = Entity->GetViewPunch();
	Punch = Entity->GetPunch();
	PunchVel = Entity->GetPunchVel();
	WaterJumpVel = Entity->GetWaterJumpVel();
	WaterJumpTime = Entity->GetWaterJumpTime();
	GroundAccelLinearFracLastTime = Entity->GetGroundAccelLinearFracLastTime();
	GameMovementOnGround = Entity->HasWalkMovedSinceLastJump();
	Stamina = Entity->GetStamina();
	BaseVelocity = Entity->GetBaseVelocity();
	SwimSoundTime = Entity->GetSwimSoundTime();
	//JumpTime = Entity->GetJumpTime();
	InDuckJump = Entity->IsInDuckJump();
	DuckUntilOnGround = Entity->GetDuckUntilOnGround();
	VecLadderNormal = Entity->GetVecLadderNormal();
	TimeNotOnLadder = Entity->GetTimeNotOnLadder();
	Gravity = Entity->GetGravity();
	WaterLevel = Entity->GetWaterLevel();
	WaterType = Entity->GetWaterTypeDirect();
	SurfaceFriction = Entity->GetSurfaceFriction();
	ViewOffset = Entity->GetViewOffset();
	IsWalking = Entity->GetIsWalking();
	MoveState = Entity->GetMoveState();
	//DuckTimeMsecs = Entity->GetDuckTimeMsecs();
	//DuckJumpTimeMsecs = Entity->GetDuckJumpTimeMsecs();
	//JumpTimeMsecs = Entity->GetJumpTimeMsecs();
	StepSoundTime = Entity->GetStepSoundTime();
	//Ducking = Entity->GetDucking();
	//Ducked = Entity->GetDucked();
	DuckOverride = Entity->GetDuckOverride();
	DuckingOrigin = Entity->GetDuckingOrigin();
	//LastDuckTime = Entity->GetLastDuckTime();
	StuckLast = Entity->GetStuckLast();
	SurfaceProps = Entity->GetSurfaceProps();
	SurfaceData = Entity->GetSurfaceData();
	TextureType = Entity->GetTextureType();
	Interfaces::MDLCache->BeginLock();
	GroundEntity = Entity->GetGroundEntityDirect();
	Interfaces::MDLCache->EndLock();
	MaxSpeed = Entity->GetMaxSpeed();
	RefEHandle = Entity->GetRefEHandleDirect();
	TickBase = Entity->GetTickBase();
	PostPoneFireReadyTime = Entity->GetWeapon() ? Entity->GetWeapon()->GetPostPoneFireReadyTime() : 0.0f;
	IsStrafing = Entity->IsStrafing();
	Mins = Entity->GetMins();
	Maxs = Entity->GetMaxs();
	LastHitgroup = Entity->GetLastHitgroup();
	RelativeDirectionOfLastInjury = Entity->GetRelativeDirectionOfLastInjury();
	FireCount = Entity->GetFireCount();
	TimeOfLastInjury = Entity->GetTimeOfLastInjury();
	vphysicsCollisionState = Entity->GetvphysicsCollisionState();
	WeaponItemDefinitionIndex = Entity->GetWeapon() ? Entity->GetWeapon()->GetItemDefinitionIndex() : WEAPON_NONE;
	WeaponClassID = Entity->GetWeapon() ? ((CBaseEntity*)Entity->GetWeapon())->GetClientClass()->m_ClassID : -1;
	WeaponHandle = Entity->GetWeaponHandle();
	Entity->BackupButtonState(ButtonState);
}

void PlayerBackup_t::GetBoneData(CBaseEntity* pEntity)
{
	playerRecord = g_LagCompensation.GetPlayerrecord(pEntity);
	Entity = pEntity;
	//animstate = Entity->GetPlayerAnimState();
	accessor = Entity->GetBoneAccessor();

	memcpy((void*)CachedBoneMatrices, Entity->GetCachedBoneData()->Base(), sizeof(matrix3x4_t) * Entity->GetCachedBoneData()->Count());

	OcclusionCheckFlags = Entity->GetLastOcclusionCheckFlags();
	OcclusionCheckFrameCount = Entity->GetLastOcclusionCheckFrameCount();
	ReadableBones = accessor->GetReadableBones();
	WritableBones = accessor->GetWritableBones();
	ModelBoneCounter = Entity->GetMostRecentModelBoneCounter();
	LastBoneSetupTime = Entity->GetLastBoneSetupTime();
	AccumulatedBoneMask = Entity->GetAccumulatedBoneMask();
	PreviousBoneMask = Entity->GetPreviousBoneMask();
	SetupBonesFrameCount = Entity->GetLastSetupBonesFrameCount();
	CachedBonesCount = Entity->GetCachedBoneData()->Count();
	giModelBoneCounter = *g_iModelBoneCounter;
}

void PlayerBackup_t::RestoreData(bool bRestoreAnimations, bool bRestoreBones)
{
	//Restore  animations
	//memcpy(animstate, &backupanimstate, sizeof(C_CSGOPlayerAnimState));
	auto unknown = Entity->GetClientUnknown();
	if (!unknown || Entity->GetRefEHandle() != RefEHandle || Entity != playerRecord->m_pEntity /*|| !Interfaces::ClientEntList->EntityExists(Entity)*/)
		return;

	if (bRestoreAnimations)
	{
		if (m_pClientAnimState && m_pClientAnimState->pBaseEntity == Entity)
			*m_pClientAnimState = m_BackupClientAnimState;

		for (int i = 0; i < MAX_RESOLVE_SIDES; ++i)
		{
			if (m_bHasBackupServerAnimState[i] && playerRecord->m_pAnimStateServer[i] && m_BackupServerAnimState->pBaseEntity == Entity)
				*playerRecord->m_pAnimStateServer[i] = m_BackupServerAnimState[i];
		}
		//Flags for invalidating physics
		int flags = 0;
		flags |= Entity->WritePoseParameters(m_flPoseParameters);

		//Restore animations
		flags |= Entity->WriteAnimLayers(m_AnimLayers);

		Entity->InvalidatePhysicsRecursive(flags);
	}

	if (bRestoreBones && Entity->GetCachedBoneData()->Base())
	{
		Entity->SetLastOcclusionCheckFlags(OcclusionCheckFlags);
		Entity->SetLastOcclusionCheckFrameCount(OcclusionCheckFrameCount);
		accessor->SetReadableBones(ReadableBones);
		accessor->SetWritableBones(WritableBones);
		Entity->SetMostRecentModelBoneCounter(ModelBoneCounter);
		Entity->SetLastBoneSetupTime(LastBoneSetupTime);
		Entity->SetAccumulatedBoneMask(AccumulatedBoneMask);
		Entity->SetPreviousBoneMask(PreviousBoneMask);
		Entity->SetLastSetupBonesFrameCount(SetupBonesFrameCount);
		memcpy(Entity->GetCachedBoneData()->Base(), (void*)CachedBoneMatrices, sizeof(matrix3x4_t) * CachedBonesCount);
		Entity->GetCachedBoneData()->count = CachedBonesCount;
		*g_iModelBoneCounter = giModelBoneCounter;
	}

	Entity->SetLowerBodyYaw(LowerBodyYaw);
	*Entity->EyeAngles() = EyeAngles;
	Entity->SetEyeAngles(NetEyeAngles);
	Entity->SetLocalOriginDirect(Origin);
	Entity->SetNetworkOrigin(NetworkOrigin);
	Entity->SetOldOrigin(OldOrigin);
	Entity->SetAbsAnglesDirect(AbsAngles);
	Entity->SetLocalAnglesDirect(LocalAngles);
	//Entity->SetRenderAngles(RenderAngles);
	Entity->SetAngleRotation(NetworkAngles);
	Entity->SetAbsOriginDirect(AbsOrigin);
	Entity->SetVelocity(Velocity);
	Entity->SetAbsVelocityDirect(AbsVelocity);
	Entity->SetDuckAmount(DuckAmount);
	Entity->SetDuckSpeed(DuckSpeed);
	Entity->SetSimulationTime(SimulationTime);
	Entity->SetOldSimulationTime(OldSimulationTime);
	Entity->SetFlags(Flags);
	Entity->SetWaterJumpVel(WaterJumpVel);
	Entity->SetWaterJumpTime(WaterJumpTime);
	Entity->GetLocalData()->localdata = LocalData;
	//Entity->SetFallVelocity(FallVelocity);
	Entity->SetMoveType(MoveType);
	Entity->SetMoveCollide(MoveCollide);
	Entity->SetVelocityModifier(VelocityModifier);
	Entity->SetViewPunch(ViewPunch);
	Entity->SetPunch(Punch);
	Entity->SetPunchVel(PunchVel);
	Entity->SetGroundAccelLinearFracLastTime(GroundAccelLinearFracLastTime);
	Entity->SetHasWalkMovedSinceLastJump(GameMovementOnGround);
	Entity->SetStamina(Stamina);
	Entity->SetBaseVelocity(BaseVelocity);
	Entity->SetSwimSoundTime(SwimSoundTime);
	//Entity->SetJumpTime(JumpTime);
	//Entity->SetInDuckJump(InDuckJump);
	Entity->SetDuckUntilOnGround(DuckUntilOnGround);
	Entity->SetVecLadderNormal(VecLadderNormal);
	Entity->SetTimeNotOnLadder(TimeNotOnLadder);
	Entity->SetGravity(Gravity);
	Entity->SetWaterLevel(WaterLevel);
	Entity->SetWaterTypeDirect(WaterType);
	Entity->SetSurfaceFriction(SurfaceFriction);
	Entity->SetViewOffset(ViewOffset);
	Entity->SetIsWalking(IsWalking);
	Entity->SetMoveState(MoveState);
	//Entity->SetDuckTimeMsecs(DuckTimeMsecs);
	//Entity->SetDuckJumpTimeMsecs(DuckJumpTimeMsecs);
	//Entity->SetJumpTimeMsecs(JumpTimeMsecs);
	Entity->SetStepSoundTime(StepSoundTime);
	//Entity->SetDucking(Ducking);
	//Entity->SetDucked(Ducked);
	Entity->SetDuckOverride(DuckOverride);
	Entity->SetDuckingOrigin(DuckingOrigin);
	//Entity->SetLastDuckTime(LastDuckTime);
	Entity->SetMins(Mins);
	Entity->SetMaxs(Maxs);

	Entity->SetStuckLast(StuckLast);
	Entity->SetSurfaceProps(SurfaceProps);
	Entity->SetSurfaceData(SurfaceData);
	Entity->SetTextureType(TextureType);
	Interfaces::MDLCache->BeginLock();
	Entity->SetGroundEntityDirect(GroundEntity);
	Interfaces::MDLCache->EndLock();
	Entity->SetMaxSpeed(MaxSpeed);
	Entity->SetRefEHandle(RefEHandle);
	Entity->SetIsStrafing(IsStrafing);
	Entity->SetLastHitgroup(LastHitgroup);
	Entity->SetRelativeDirectionOfLastInjury(RelativeDirectionOfLastInjury);
	Entity->SetFireCount(FireCount);
	Entity->SetTimeOfLastInjury(TimeOfLastInjury);
	Entity->SetvphysicsCollisionState(vphysicsCollisionState);
	Entity->RestoreButtonState(ButtonState);

	//NOTE: RESTORING EFLAGS FIXES TRACERAY NOT RETURNING THAT WE HIT THE PLAYER AS WELL AS COLLISION, BUT IS A MAJOR PERFORMANCE DROP, USE UTIL_ClipTraceToPlayers_Fixed AND USE CTraceFilterNoPlayers for TraceRay
	//Entity->SetEFlags(EFlags);
	//auto eflags = EFlags;
	//eflags |= (EFL_DIRTY_SURROUNDING_COLLISION_BOUNDS|EFL_DIRTY_SPATIAL_PARTITION);
	//Entity->SetEFlags(eflags);
	//Entity->UpdatePartition();

	//if (Entity->GetWeapon())
	//	Entity->GetWeapon()->SetPostPoneFireReadyTime(PostPoneFireReadyTime);
}

void PlayerBackup_t::RestoreBoneData()
{
	if (Entity->GetCachedBoneData()->Base())
	{
		Entity->SetLastOcclusionCheckFlags(OcclusionCheckFlags);
		Entity->SetLastOcclusionCheckFrameCount(OcclusionCheckFrameCount);
		accessor->SetReadableBones(ReadableBones);
		accessor->SetWritableBones(WritableBones);
		Entity->SetMostRecentModelBoneCounter(ModelBoneCounter);
		Entity->SetLastBoneSetupTime(LastBoneSetupTime);
		Entity->SetAccumulatedBoneMask(AccumulatedBoneMask);
		Entity->SetPreviousBoneMask(PreviousBoneMask);
		Entity->SetLastSetupBonesFrameCount(SetupBonesFrameCount);
		memcpy(Entity->GetCachedBoneData()->Base(), (void*)CachedBoneMatrices, sizeof(matrix3x4_t) * CachedBonesCount);
		Entity->GetCachedBoneData()->count = CachedBonesCount;
		*g_iModelBoneCounter = giModelBoneCounter;
	}
}