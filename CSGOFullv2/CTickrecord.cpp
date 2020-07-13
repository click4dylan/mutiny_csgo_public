#include "CPlayerrecord.h"
#include "LocalPlayer.h"
#include "UsedConvars.h"
#include "VTHook.h"
#include "INetchannelInfo.h"

CTickrecord::~CTickrecord()
{
	for (CCSGOPlayerAnimState** i = &m_pAnimStateServer[0]; i != &m_pAnimStateServer[MAX_RESOLVE_SIDES]; ++i)
	{
		CCSGOPlayerAnimState* animstate = *i;
		if (animstate)
		{
			delete animstate;
			*i = nullptr;
		}
	}
}

CTickrecord::CTickrecord()
{
	for (CCSGOPlayerAnimState** i = &m_pAnimStateServer[0]; i != &m_pAnimStateServer[MAX_RESOLVE_SIDES]; ++i)
	{
		*i = nullptr;
	}
	Reset();
}

CTickrecord::CTickrecord(CBaseEntity* _Entity)
{
	for (CCSGOPlayerAnimState** i = &m_pAnimStateServer[0]; i != &m_pAnimStateServer[MAX_RESOLVE_SIDES]; ++i)
	{
		*i = nullptr;
	}
	Initialize(_Entity);
}

void CTickrecord::Reset()
{
	m_pEntity = nullptr;

	LocalVars.m_EyePos = LocalVars.m_ShootPos = LocalVars.m_NetOrigin = LocalVars.m_Origin = LocalVars.m_ViewOffset = Vector(0.f, 0.f, 0.f);
	LocalVarsScan.m_EyePos = LocalVarsScan.m_ShootPos = LocalVarsScan.m_NetOrigin = LocalVarsScan.m_Origin = LocalVarsScan.m_ViewOffset = Vector(0.f, 0.f, 0.f);

	m_bMoving = m_bCachedBones = m_bRanHitscan = m_bHeadIsVisible = m_bBodyIsVisible = m_Dormant = m_bLBYUpdated = m_bRealTick = m_bFiredBullet = m_bStrafing = m_bLegit = m_bForceNotLegit = m_bTickbaseShiftedBackwards = m_bTickbaseShiftedForwards = m_bIsUsingBalanceAdjustResolver = m_bHasBodyPartNotBehindWall = m_bHasHeadNotBehindWall = m_bIsUsingFreestandResolver = m_bIsUsingMovingResolver = m_bUsedBodyHitResolveDelta = m_bShotAndMissed = m_bIsUsingMovingLBYMeme = false;
	m_flBestHitboxDamage = m_SimulationTime = m_OldSimulationTime = m_flFirstCmdTickbaseTime = m_DuckAmount = m_DuckSpeed = m_flGoalFeetYaw = m_flCurrentFeetYaw = m_EyeYaw = m_flLowerBodyYaw = m_flFeetCycle = m_flFeetWeight = m_next_lby_update_time = m_flBestScaledMinDamage = m_flAbsMaxDesyncDelta = 0.f;
	m_iTicksChoked = m_Health = m_Armor = m_MoveType = m_Flags = m_iTickcount = 0;
	m_LocalAngles = m_AbsAngles = m_Angles = m_OriginalEyeAngles = m_EyeAngles = QAngle(0.f, 0.f, 0.f);

	m_iResolveMode = RESOLVE_MODE_NONE;
	m_iResolveSide = ResolveSides::NONE;
	m_iOppositeResolveSide = ResolveSides::NONE;

	Impact.Reset();

	for (CCSGOPlayerAnimState** i = &m_pAnimStateServer[0]; i != &m_pAnimStateServer[MAX_RESOLVE_SIDES]; ++i)
	{
		CCSGOPlayerAnimState* animstate = *i;
		if (animstate)
		{
			delete animstate;
			*i = nullptr;
		}
	}

	m_Origin = m_NetOrigin = m_AbsOrigin = m_Velocity = m_AbsVelocity = m_BaseVelocity = m_ViewOffset = m_Mins = m_Maxs = m_vecHeadPos = Vector(0.f, 0.f, 0.f);

	for (auto i = 0; i < MAXSTUDIOBONES; i++)
	{
		if (i < HITBOX_MAX)
			m_BestHitbox[i].bsaved = false;

		if (i < MAX_CSGO_POSE_PARAMS)
			m_flPoseParams[i] = 0.0f;

		if (i < MAX_OVERLAYS)
			memset(&m_AnimLayer[i], 0, sizeof(C_AnimationLayer));

		const Vector zVec = { 0,0,0 };
		m_PlayerBackup.CachedBoneMatrices[i].Init(zVec, zVec, zVec, zVec);
	}
}

void CTickrecord::Initialize(CBaseEntity* _Entity)
{
	Reset();

	CPlayerrecord* _playerRecord = g_LagCompensation.GetPlayerrecord(_Entity);

	if (!_Entity || !_playerRecord)
		return;

	m_pEntity = _Entity;
	m_Index = _Entity->index;

	m_bMoving = m_bCachedBones = m_bHeadIsVisible = m_bBodyIsVisible = m_bRealTick = m_bLBYUpdated = m_bFiredBullet = m_bCachedFakeAngles = false;

	m_bStrafing = _Entity->IsStrafing();
	m_Health = _Entity->GetHealth();
	m_Armor = _Entity->GetArmor();
	m_MoveType = _Entity->GetMoveType();
	m_MoveState = _Entity->GetMoveState();
	m_Dormant = false;

#ifdef PRINT_ANGLE_CHANGES
	printf("Storing tick yaw %f %f %f\n", _playerRecord->m_angPristineEyeAngles.x, _playerRecord->m_angPristineEyeAngles.y, _playerRecord->m_angPristineEyeAngles.z);
#endif
	m_EyeAngles = _playerRecord->m_angPristineEyeAngles;
	m_OriginalEyeAngles = m_EyeAngles;

	m_EyeYaw = m_EyeAngles.y; //ToDo: do we need this?
	m_LocalAngles = _Entity->GetLocalAnglesDirect();
	m_AbsAngles = _Entity->GetAbsAnglesDirect();

	m_NetOrigin = _Entity->GetNetworkOrigin();
	m_Origin = _Entity->GetLocalOriginDirect();
	m_AbsOrigin = _Entity->GetAbsOriginDirect();

	m_Velocity = _Entity->GetVelocity();
	m_AbsVelocity = _Entity->GetAbsVelocityDirect();
	m_BaseVelocity = _Entity->GetBaseVelocity();

	m_ViewOffset = _Entity->GetViewOffset();

	m_Mins = _Entity->GetMins();
	m_Maxs = _Entity->GetMaxs();

	m_flLowerBodyYaw = _Entity->GetLowerBodyYaw();

	m_SimulationTime = _Entity->GetSimulationTime();
	m_OldSimulationTime = _Entity->GetOldSimulationTime();

	m_DuckAmount = _Entity->GetDuckAmount();
	m_DuckSpeed = _Entity->GetDuckSpeed();

	m_Flags = _Entity->GetFlags();

	m_iTickcount = TIME_TO_TICKS(m_SimulationTime);
	m_iServerTick = g_ClientState->m_ClockDriftMgr.m_nServerTick;


	m_iTicksChoked = clamp(_playerRecord->m_iTicksChoked, 0, 16);

	m_flFirstCmdTickbaseTime = _playerRecord->m_flFirstCmdTickbaseTime;
	m_Tickbase = _playerRecord->m_nTickbase;
	m_bTickbaseShiftedBackwards = _playerRecord->m_bTickbaseShiftedBackwards;
	m_bTickbaseShiftedForwards = _playerRecord->m_bTickbaseShiftedForwards;
	m_bIsDuplicateTickbase = _playerRecord->m_bIsDuplicateTickbase;
	m_bLegit = _playerRecord->m_bLegit;
	m_bForceNotLegit = _playerRecord->m_bForceNotLegit;
	for (int i = 0; i < MAX_RESOLVE_SIDES; ++i)
	{
		if (_playerRecord->m_pAnimStateServer[i])
		{
			m_pAnimStateServer[i] = new CCSGOPlayerAnimState;
			if (m_pAnimStateServer[i])
				*m_pAnimStateServer[i] = *_playerRecord->m_pAnimStateServer[i];
		}
	}

	_Entity->CopyPoseParameters(m_flPoseParams);
	_Entity->CopyAnimLayers(m_AnimLayer);

	m_flGoalFeetYaw = _Entity->GetGoalFeetYaw();
	m_flCurrentFeetYaw = _Entity->GetCurrentFeetYaw();
	m_vecLadderNormal = _Entity->GetVecLadderNormal();

	m_FiringFlags = 0;

	GetLocalPlayerVars();
}

void CTickrecord::GetLocalPlayerVars()
{
	if (LocalPlayer.Entity)
	{
		LocalVars.m_EyePos = LocalPlayer.Entity->GetEyePosition();
		LocalVars.m_ShootPos = LocalPlayer.ShootPosition;//LocalPlayer.Entity->Weapon_ShootPosition();
		LocalVars.m_NetOrigin = LocalPlayer.Entity->GetNetworkOrigin();
		LocalVars.m_Origin = LocalPlayer.Entity->GetLocalOriginDirect();
		LocalVars.m_ViewOffset = LocalPlayer.Entity->GetViewOffset();
	}
}

void CTickrecord::GetLocalPlayerVars_Hitscan()
{
	if (LocalPlayer.Entity)
	{
		LocalVarsScan.m_EyePos = LocalPlayer.Entity->GetEyePosition();
		LocalVarsScan.m_ShootPos = LocalPlayer.ShootPosition;//LocalPlayer.Entity->Weapon_ShootPosition();
		LocalVarsScan.m_NetOrigin = LocalPlayer.Entity->GetNetworkOrigin();
		LocalVarsScan.m_Origin = LocalPlayer.Entity->GetLocalOriginDirect();
		LocalVarsScan.m_ViewOffset = LocalPlayer.Entity->GetViewOffset();
	}
}

bool CTickrecord::IsRealTick() const
{
	return m_bRealTick;// || /*m_bLBYUpdated ||*/ (m_bMoving || fabsf(m_AbsVelocity.z) > 100.f);
}

bool CTickrecord::ScanStillValid() const
{
	return LocalPlayer.Entity->GetLocalOriginDirect() == LocalVarsScan.m_Origin && LocalPlayer.ShootPosition == LocalVarsScan.m_ShootPos;
}

bool CTickrecord::IsVisible()
{
	CGameTrace tr;
	Ray_t ray;
	CTraceFilterWorldOnly filter;
	filter.pSkip = (IHandleEntity*)LocalPlayer.Entity;

	ray.Init(LocalPlayer.ShootPosition, m_vecHeadPos);
	Interfaces::EngineTrace->TraceRay(ray, MASK_SHOT, &filter, &tr);

	return (tr.fraction > 0.99f);
}

bool CTickrecord::IsValid(CBaseEntity* Entity)
{
	return m_pEntity == Entity && g_LagCompensation.IsTickValid(this);
}
int CTickrecord::GetMaxWeight() const
{
	return 90;
}
int CTickrecord::GetWeight(CTickrecord *bestrecord, int* bestweight)
{
	// you can't backtrack or even shoot at ticks that shifted backwards, they are useless
	// the server doesn't save them into lag compensation at all

	if (m_bTickbaseShiftedBackwards || m_bIsDuplicateTickbase)
		return -999;

#if defined(DEBUG) || defined(INTERNAL_DEBUG)
	if (m_bBalanceAdjust && GetAsyncKeyState(VK_END) & (1 << 16))
		return -999;

	if (!m_bBalanceAdjust && GetAsyncKeyState(VK_HOME) & (1 << 16))
		return -999;
#endif

	// init locals
	int _weight = 0;

	// we can't backtrack lag comp breakers
	if (m_bTeleporting)
		return -999;

	if (m_bShotAndMissed)
		return -999;

	//if (m_bBalanceAdjust)
	//	_weight += 5; // try that?

	if (Impact.m_bIsBodyHitResolved)
	{
		if (fabsf(AngleDiff(Impact.m_ImpactInfo.m_NetworkedEyeYaw, m_EyeAngles.y)) < 15.0f)
		{
			_weight += 5;
		}
	}

	//It is so much easier to resolve 35/-35
	if (m_bIsUsingMovingLBYMeme)
		_weight += 20;

	//if (m_bTickbaseShiftedForwards)
		//_weight -= 2;

	// prioritize real ticks
#if 0
	if (m_bRealTick && !m_bTickbaseShiftedForwards)
	{
		if (m_bLegit && !m_bForceNotLegit)
			_weight += 5;
		else
			++_weight;
	}
#endif

	// moving fast => minimal desync
	//if (m_AbsVelocity.Length() >= 200.0f)
	//	_weight += 3;

	// player shot
	//if (m_bFiredBullet)
	//	_weight += 10;

	// we already hitscanned
	if (m_bRanHitscan && m_bCachedBones && ScanStillValid())
	{
		// prioritize visible ticks
		if (m_bBodyIsVisible)
		{
			_weight += 20;

			if (m_bHasBodyPartNotBehindWall)
				_weight += 10;
		}

		// prioritize head visibilty
		if (m_bHeadIsVisible)
		{
			_weight += 20;

			if (m_bHasHeadNotBehindWall)
				_weight += 10;
		}

		// don't use this record if we can't hit anything
		if (!m_bHeadIsVisible && !m_bBodyIsVisible)
			_weight = -999;
	}

	return _weight;
}

bool CTickrecord::HasMinDamage() const
{
	//float _mindmg = (float)LocalPlayer.CurrentWeapon->GetCSWpnData()->iDamage;
	//_mindmg *= powf(LocalPlayer.CurrentWeapon->GetCSWpnData()->flRangeModifier, ((m_BestHitbox[m_iBestHitboxIndex].origin - m_BestHitbox[m_iBestHitboxIndex].localplayer_shootpos).Length() * 0.002f));
	//ScaleDamage(m_BestHitbox[m_iBestHitboxIndex].actual_hitgroup_due_to_penetration, m_pEntity, LocalPlayer.CurrentWeapon->GetCSWpnData()->flArmorRatio, _mindmg);
	//_mindmg -= 1.0f;
	//
	//_mindmg = fmaxf(0.0f, fminf(_mindmg, m_BestHitbox[m_iBestHitboxIndex].penetrated ? static_cast<float>(variable::get().ragebot.i_mindmg_aw) : static_cast<float>(variable::get().ragebot.i_mindmg)));

	return m_flBestHitboxDamage >= m_pEntity->GetHealth() || m_flBestHitboxDamage >= m_flBestScaledMinDamage;
}

void CTickrecord::StoreOppositeResolveSide()
{
	m_iOppositeResolveSide = g_LagCompensation.GetOppositeResolveSide(m_iResolveSide);
	if (m_iOppositeResolveSide == m_iResolveSide)
		m_iOppositeResolveSide = ResolveSides::INVALID_RESOLVE_SIDE;
}