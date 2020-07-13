#include "CPlayerrecord.h"
#include "LocalPlayer.h"
#include "UsedConvars.h"
#include "VTHook.h"
#include "INetchannelInfo.h"

CShotrecord::CShotrecord(CBaseEntity* _InflictorEntity, CBaseEntity* _InflictedEntity, CTickrecord* _tickrecord)
{
	CPlayerrecord* _playerRecord = g_LagCompensation.GetPlayerrecord(_InflictedEntity);

	if (!_playerRecord)
		return;

	m_pInflictedEntity = _InflictedEntity;
	m_pInflictorEntity = _InflictorEntity;
	m_bInflictorIsLocalPlayer = _InflictorEntity->IsLocalPlayer();

	m_Tickrecord = _tickrecord;
	memcpy(m_BoneCache, reinterpret_cast<matrix3x4_t*>(_InflictedEntity->GetCachedBoneData()->Base()), _InflictedEntity->GetCachedBoneData()->Count() * sizeof(matrix3x4_t));
	m_vecLocalEyePos = LocalPlayer.ShootPosition;
	m_vecTargetEyePos = _InflictedEntity->GetEyePosition();
	m_vecLocalPlayerOrigin = LocalPlayer.Entity->GetAbsOriginDirect();
	m_angLocalEyeAngles = CurrentUserCmd.cmd->viewangles;
	m_angLocalEyeAngles += LocalPlayer.Entity->GetPunch() * weapon_recoil_scale.GetVar()->GetFloat();
	m_ImpactAngles = angZero;

	m_iTargetHitgroup = _playerRecord->LastShot.m_iTargetHitgroup;
	m_iTargetHitbox = _playerRecord->LastShot.m_iTargetHitbox;
	m_iActualHitgroup = HITGROUP_GENERIC;
	m_iActualHitbox = HITBOX_MAX;

	m_iResolveMode = _tickrecord->m_iResolveMode;
	m_iResolveSide = _tickrecord->m_iResolveSide;
	m_bEnemyIsNotChoked = _tickrecord->m_iTicksChoked == 0;
	m_bEnemyFiredBullet = _tickrecord->m_bFiredBullet;
	m_bLegit = _tickrecord->m_bLegit;
	m_bForceNotLegit = _tickrecord->m_bForceNotLegit;
	m_bDoesNotFoundTowardsStats = _playerRecord->m_bIsUsingFarESP || _playerRecord->m_bIsUsingServerSide || (_playerRecord->m_iTickcount_ForwardTrack != 0 && !_playerRecord->m_bCountResolverStatsEvenWhenForwardtracking) || _tickrecord->Impact.m_bIsBodyHitResolved;
	m_bForwardTracked = _playerRecord->m_iTickcount_ForwardTrack != 0;
	m_bUsedBodyHitResolveDelta = _tickrecord->m_bUsedBodyHitResolveDelta;
	m_iBodyHitResolveStance = _tickrecord->Impact.m_iBodyHitResolveStance;
	m_bShotAtBalanceAdjust = _tickrecord->m_bIsUsingBalanceAdjustResolver;
	m_bShotAtFreestanding = _tickrecord->m_bIsUsingFreestandResolver;
	m_bShotAtMovingResolver = _tickrecord->m_bIsUsingMovingResolver;

	m_iTickCountWhenWeShot = g_ClientState->m_ClockDriftMgr.m_nServerTick;
	if (_playerRecord->m_iTickcount_ForwardTrack != 0)
	{
		m_iEnemySimulationTickCount = _playerRecord->m_iTickcount_ForwardTrack;
	}
	else
	{
		if (m_iTickCountWhenWeShot >= _tickrecord->m_iTickcount)
			m_iEnemySimulationTickCount = _tickrecord->m_iTickcount;
		else
		{
			//tickbase is a wonderful thing, prevent drawing forwardtrack when we didn't really forwardtrack
			m_iEnemySimulationTickCount = m_iTickCountWhenWeShot;
		}
	}
	m_flCurtime = Interfaces::Globals->curtime;
	m_flLatency = Interfaces::EngineClient->GetNetChannelInfo()->GetLatency(FLOW_OUTGOING);
	m_flRealTime = Interfaces::Globals->realtime;
	m_flRealTimeAck = 0;

	m_bAck = m_bMissed = m_bTEImpactEffectAcked = m_bTEBloodEffectAcked = m_bImpactEventAcked = m_bIsInflictor = false;
}