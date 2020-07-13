#include "precompiled.h"
#include "CPlayerrecord.h"
#include "ThirdPerson.h"
#include "Targetting.h"
#include "INetchannelInfo.h"
#include "LocalPlayer.h"
#include "Aimbot_imi.h"
#include "Eventlog.h"
#include "ICollidable.h"
#include "CPlayerResource.h"
#include "UsedConvars.h"
#include "TickbaseExploits.h"
#include "AntiAim.h"
#include "CParallelProcessor.h"
#include "Autowall.h"
#include "Adriel/adr_util.hpp"
#include "Adriel/input.hpp"
#include "IClientNetworkable.h"

#include "Adriel/stdafx.hpp"
#include "VMProtectDefs.h"

//#define USE_EYE_ANGLES_FOR_SETUPVELOCITY

CLagCompensation_imi g_LagCompensation;
CPlayerrecord m_PlayerRecords[MAX_PLAYERS + 1];
std::vector<ImpactResolverQueue_t> g_QueuedImpactResolveEvents;
std::deque<ImpactEventQueue_t> g_QueuedImpactEvents;
#define MAX_MOVING_RESOLVER_SHOTS 1
extern int nChokedTicks;

CPlayerrecord::CPlayerrecord()
{
	for (int i = 0; i < MAX_RESOLVE_SIDES; ++i)
		m_pAnimStateServer[i] = nullptr;
	Hooks.m_pBaseAnimState = nullptr;
	Hooks.m_pBaseAnimStateHook = nullptr;
	m_RestoreTick = nullptr;
	m_RestoreTickIsDynamic = false;
	Reset(true);
}

void CPlayerrecord::s_Impact::Reset()
{
	m_flLastBodyHitResolveTime = 0.0f;
	m_flLastImpactProcessRealTime = 0.0f;
	m_flLastBloodProcessRealTime = 0.0f;
	m_iLastBloodProcessTickCount = 0;
	m_flLastBodyHitResolveTime = 0.0f;
	for (int j = 0; j < CPlayerrecord::ImpactResolveStances::MAX_IMPACT_RESOLVE_STANCES; ++j)
	{
		ResolveStances[j].Reset();
	}
	m_vecDirBlood.Init();
	//m_vecBloodOrigin.Init(); can't remember why this wasn't added here, FIXME: check to see if it's okay to do this and it won't break anything
}

s_Impact_Info *CPlayerrecord::GetBodyHitResolveInfo(CTickrecord* tickrecord, bool is_saving_info)
{
	if (!variable::get().ragebot.b_resolver_experimental || (m_bLegit && !m_bForceNotLegit) || !m_bAllowBodyHitResolver)
		return nullptr;

	auto& info = Impact.ResolveStances[GetBodyHitStance(tickrecord)];
	if (is_saving_info || info.m_bIsBodyHitResolved)
		return &info;

	return nullptr;
}

CPlayerrecord::ImpactResolveStances CPlayerrecord::GetBodyHitStance(CTickrecord* tickrecord)
{
#if 1
	//Just use one stance for now
	return CPlayerrecord::ImpactResolveStances::Standing;
#else
	Vector& vecVelocity = !tickrecord ? m_pEntity->GetAbsVelocityDirect() : tickrecord->m_AbsVelocity;
	float speed = vecVelocity.Length();

	if (speed <= MINIMUM_PLAYERSPEED_CONSIDERED_MOVING)
		return CPlayerrecord::ImpactResolveStances::Standing;

	return speed > 50.0f ? CPlayerrecord::ImpactResolveStances::Running : CPlayerrecord::ImpactResolveStances::Walking;
#endif
}

CTickrecord* CPlayerrecord::CreateNewTickrecord(CTickrecord *_previousRecord)
{
	// get current weapon
	CBaseCombatWeapon* _Weapon = m_pEntity->GetWeapon();

	// update players weapon
	m_pWeapon = _Weapon;


	bool _JustMaterialized = false;
	bool _CreateNewRecord = true;
	m_bTickbaseShiftedBackwards = false;
	m_bTickbaseShiftedForwards = false;
	m_bIsDuplicateTickbase = false;

	float _SimulationTime = m_pEntity->GetSimulationTime();
	int _TicksSinceLastUpdate = clamp(m_iTicksSinceLastServerUpdate + 1, 0, 18); //clamp((g_ClientState->m_ClockDriftMgr.m_nServerTick - m_nServerTickCount), 0, MAX_USER_CMDS);
	int _OldTickbase = TIME_TO_TICKS(DataUpdateVars.m_flPreSimulationTime) + 1;
	int _NewTickbase = TIME_TO_TICKS(_SimulationTime) + 1;
	int _TicksSinceLastSimulation = _NewTickbase - _OldTickbase;
	m_nTickbase = _NewTickbase;

	// we had no previous tick so assume 0
	if (!_previousRecord || _previousRecord->m_Dormant || m_flSpawnTime != m_pEntity->GetSpawnTime())
	{
		m_iTicksChoked = 0;
		m_iTicksSinceLastServerUpdate = 0;
		m_nServerTickCount = g_ClientState->m_ClockDriftMgr.m_nServerTick;
		m_flFirstCmdTickbaseTime = _SimulationTime;
		m_flNewestSimulationTime = _SimulationTime;
		m_iNewestTickbase = _NewTickbase;
	}
	else if (_NewTickbase != _OldTickbase || Changed.ShotTime || Changed.Origin || Changed.Animations)
	{
		_TicksSinceLastUpdate = clamp((g_ClientState->m_ClockDriftMgr.m_nServerTick - m_nServerTickCount), 0, 18);

		if (_TicksSinceLastSimulation < 0)
		{
			m_bTickbaseShiftedBackwards = true;
			m_iTicksChoked = _TicksSinceLastUpdate - 1;
			m_flFirstCmdTickbaseTime = TICKS_TO_TIME(_NewTickbase - _TicksSinceLastUpdate - 1);
		}
		else if (_TicksSinceLastSimulation == 0)
		{
			m_iTicksChoked = _TicksSinceLastUpdate - 1;
			m_flFirstCmdTickbaseTime = TICKS_TO_TIME(_NewTickbase - _TicksSinceLastUpdate - 1);
		}
		else if (abs(_TicksSinceLastSimulation - _TicksSinceLastUpdate) > 0 && _previousRecord->m_bTickbaseShiftedBackwards)
		{
			//did their tickbase shift forwards
			m_bTickbaseShiftedForwards = true; //m_bTickbaseShiftedBackwards
			m_iTicksChoked = _TicksSinceLastUpdate - 1;
			m_flFirstCmdTickbaseTime = TICKS_TO_TIME(_NewTickbase - _TicksSinceLastUpdate - 1);
		}
		else
		{
			m_iTicksChoked = _TicksSinceLastSimulation - 1;
			m_flFirstCmdTickbaseTime = TICKS_TO_TIME(_NewTickbase - _TicksSinceLastSimulation - 1);
		}

		m_nServerTickCount = g_ClientState->m_ClockDriftMgr.m_nServerTick;
		m_iTicksChoked = clamp(m_iTicksChoked, 0, 16);

		//now check for duplicates
		if (_SimulationTime <= m_flNewestSimulationTime)
			m_bIsDuplicateTickbase = true;
		else
		{
			for (auto& tick : m_Tickrecords)
			{
				if (tick->m_Tickbase == _NewTickbase)
				{
					//we can't backtrack this new record we received :'(
					m_bIsDuplicateTickbase = true;
					break;
				}
			}
		}

		if (m_iTicksChokedHistory.size() > 31) // limit to 32
			m_iTicksChokedHistory.pop_front();

		m_iTicksChokedHistory.push_back(m_iTicksChoked);

		m_iTicksSinceLastServerUpdate = 0;
	}
	// nothing changed so we can just take the last value
	else
	{
		m_iTicksChoked = _previousRecord->m_iTicksChoked;
		++m_iTicksSinceLastServerUpdate;
		_CreateNewRecord = false;
	}

	m_iTicksChoked = clamp(m_iTicksChoked, 0, 16);

	CTickrecord* _newRecord;

	// only create a new tickrecord if the simtime changed
	if (_CreateNewRecord)
	{
		// create tickrecord
		_newRecord = new CTickrecord(m_pEntity);

		Interfaces::MDLCache->BeginLock();

		// update move parent data
		CBaseEntity* _parent = m_pEntity->GetMoveParent();

		while (_parent)
		{
			const float _simTimeDelta = _parent->GetSimulationTime() - _parent->GetOldSimulationTime();

			// parent sim time changed
			if (_simTimeDelta > 0.f)
			{
				_parent->SetLocalVelocity((_parent->GetNetworkOrigin() - _parent->GetOldOrigin()) / _simTimeDelta);
				_parent->CalcAbsoluteVelocity();
			}

			// correct origin of parent
			_parent->SetLocalOrigin(_parent->GetNetworkOrigin());

			// next
			_parent = _parent->GetMoveParent();
		}

		// reset network origin in case something changed it
		m_pEntity->SetNetworkOrigin(m_vecCurNetOrigin);

		// update velocity
		UpdateVelocity(_JustMaterialized ? nullptr : _previousRecord);
		_newRecord->m_AbsVelocity = m_pEntity->GetAbsVelocityDirect();
		_newRecord->m_BaseVelocity = m_pEntity->GetBaseVelocity();

		// update origin
		_newRecord->m_NetOrigin = m_vecCurNetOrigin;
		m_pEntity->SetLocalOrigin(_newRecord->m_NetOrigin);
		m_pEntity->CalcAbsolutePosition();
		_newRecord->m_AbsOrigin = m_pEntity->GetAbsOriginDirect();

		// update spawntime
		_newRecord->m_flSpawnTime = m_pEntity->GetSpawnTime();

		// update groundent
		_newRecord->m_GroundEntity = m_pEntity->GetGroundEntityDirect();

		Interfaces::MDLCache->EndLock();

		// store server-side information
		_newRecord->m_flOldFeetCycle = DataUpdateVars.m_flPreFeetCycle;
		_newRecord->m_flOldFeetWeight = DataUpdateVars.m_flPreFeetWeight;
		_newRecord->m_flFeetCycle = DataUpdateVars.m_flPostFeetCycle;
		_newRecord->m_flFeetWeight = DataUpdateVars.m_flPostFeetWeight;

		// store whether or not they shot
		MarkShot(_newRecord);

		// store new tickrecord
		m_Tickrecords.emplace_front(_newRecord);
	}
	else
	{
		// update the latest tick we have
		_newRecord = _previousRecord;

		//store updated eye angles
		_newRecord->m_EyeAngles = m_angPristineEyeAngles;

		// we got a silent update
		if (m_bNetUpdateSilent)
		{
			// origin updated
			if (Changed.Origin)
			{
				Vector _newOrigin = m_vecCurNetOrigin;
				m_pEntity->SetNetworkOrigin(_newOrigin);
				m_pEntity->SetLocalOrigin(_newOrigin);
				m_pEntity->CalcAbsolutePosition();
				_newRecord->m_NetOrigin = _newOrigin;
				_newRecord->m_Origin = _newOrigin;
				_newRecord->m_AbsOrigin = m_pEntity->GetAbsOriginDirect();
			}

			//FIXME: What to do with velocity?

			_newRecord->m_BaseVelocity = m_pEntity->GetBaseVelocity();

			// update other stuff
			if (Changed.FeetCycle)
				_newRecord->m_flFeetCycle = DataUpdateVars.m_flPostFeetCycle;
			if (Changed.FeetWeight)
				_newRecord->m_flFeetWeight = DataUpdateVars.m_flPostFeetWeight;
		}
	}

	// mark special stuff in tickrecord
	if (!_previousRecord || _CreateNewRecord)
	{
		// player is moving
		_newRecord->m_bMoving = _newRecord->m_Velocity.Length() > .1f;

		// set playerrecord information
		m_bMoving = _newRecord->m_bMoving;

		if (m_bMoving)
			m_bIsBreakingLBYWithLargeDelta = false;

		// set inair
		_newRecord->m_bInAir = !(_newRecord->m_Flags & FL_ONGROUND);

		// assume that we didn't break lagcomp
		m_bTeleporting = false;
		_newRecord->m_bTeleporting = false;

		auto _animstate = m_pAnimStateServer[ResolveSides::NONE];
		_newRecord->m_flMaxDesyncMultiplier = m_pEntity->GetMaxDesyncMultiplier();
		float maxyaw = _animstate && _animstate->m_flMaxYaw != 0.0f ? _animstate->m_flMaxYaw : 58.0f;
		_newRecord->m_flMaxDesyncDelta = maxyaw * _newRecord->m_flMaxDesyncMultiplier;

		// we have a new tick
		if (_previousRecord)
		{
			//The actual max desync multiplier can go below 0.5f due to the way SetupVelocity works,
			//so we need to estimate what the new value is 
			if (fabsf(_previousRecord->m_flMaxDesyncMultiplier - 0.5f) < 0.009f)
			{
				float _newEstimatedDesyncMultiplier = _previousRecord->m_flEstimatedDesyncMultiplier + (_previousRecord->m_flDesyncMultiplierRateOfChange * (m_iTicksChoked + 1));

				//min desync delta can't be below 29 degrees
				if (120.0f * _newEstimatedDesyncMultiplier >= 29.0f)
					_newRecord->m_flEstimatedDesyncMultiplier = _newEstimatedDesyncMultiplier;
				else
					_newRecord->m_flEstimatedDesyncMultiplier = _previousRecord->m_flEstimatedDesyncMultiplier;

				_newRecord->m_flDesyncMultiplierRateOfChange = _previousRecord->m_flDesyncMultiplierRateOfChange;
			}
			else
			{
				_newRecord->m_flEstimatedDesyncMultiplier = _newRecord->m_flMaxDesyncMultiplier;
				_newRecord->m_flDesyncMultiplierRateOfChange = (_newRecord->m_flMaxDesyncMultiplier - _previousRecord->m_flMaxDesyncMultiplier) / (m_iTicksChoked + 1);
			}

			_newRecord->m_flEstimatedMaxDesyncDelta = _newRecord->m_flEstimatedDesyncMultiplier * maxyaw;

			if (!_previousRecord->m_Dormant && _newRecord->m_Flags & FL_ONGROUND)
			{
				// check to see if the enemy used the lby move in place meme
				if (fabsf(_newRecord->m_AnimLayer[6]._m_flCycle - _previousRecord->m_AnimLayer[6]._m_flCycle) > FLT_EPSILON)
				{
					float spd = _newRecord->m_AbsVelocity.Length();
					if (spd < 2.0f)
					{
						int _TotalNewCommands = m_iTicksChoked + 1;
						Vector _absVeloSlope = (_newRecord->m_AbsVelocity - _previousRecord->m_AbsVelocity) / _TotalNewCommands;
						if (_absVeloSlope.Length() <= 0.1f)
							_newRecord->m_bIsUsingMovingLBYMeme = true;
					}
				}
			}

			//printf("dist %f spd %f ticks %i time %f origin %.1f %.1f %.1f\n", (_newRecord->m_NetOrigin - _previousRecord->m_NetOrigin).LengthSqr(), _newRecord->m_AbsVelocity.Length(), _newRecord->m_iTicksChoked + 1, _SimulationTime, _newRecord->m_NetOrigin.x, _newRecord->m_NetOrigin.y, _newRecord->m_NetOrigin.z);

			//Interfaces::Cvar->FindVar("cl_simulationtimefix")->nFlags &= ~FCVAR_HIDDEN;
			//Interfaces::Cvar->FindVar("cl_simulationtimefix")->SetValue(1);

			//AllocateConsole();

			// is player breaking lagcomp?

			//Get the last record that was actually stored in the server's lag compensation records
			m_bTeleporting = false;
			if (m_Tickrecords.size() > 1)
			{
				for (auto& tick : m_Tickrecords)
				{
					if (tick != _newRecord)
					{
						if (!tick->m_bIsDuplicateTickbase && !tick->m_bTickbaseShiftedBackwards)
						{
							m_bTeleporting = !tick->m_Dormant && (_newRecord->m_NetOrigin - tick->m_NetOrigin).LengthSqr() > 4096.f;
							break;
						}
					}
				}
			}

			_newRecord->m_bTeleporting = m_bTeleporting;

			if (m_bTeleporting)
			{
				//invalidate all older records so that we don't try to backtrack them
				if (!m_Tickrecords.empty())
				{
					for (auto& _oldrecord : m_Tickrecords)
					{
						_oldrecord->m_bTeleporting = true;
					}
				}
			}

			// is player ducking?
			m_bIsHoldingDuck = _newRecord->m_DuckAmount > 0.009f && _newRecord->m_DuckAmount >= _previousRecord->m_DuckAmount;
			m_bIsFakeDucking = _newRecord->m_DuckAmount > 0.0f && _newRecord->m_DuckAmount < 1.0f && _newRecord->m_DuckAmount == _previousRecord->m_DuckAmount;

			float _ServerTime = GetServerTime();

			// get current lby
			const float _lby = m_pEntity->GetLowerBodyYaw();

			// lby updated
			if (_lby != m_flLowerBodyYaw)
			{
				// set current lby
				m_flLowerBodyYaw = _lby;

				// set last update time
				m_flLastLBYUpdateTime = _ServerTime;

				// mark as lby update
				_newRecord->m_bLBYUpdated = true;

				// see if the lby prediction is wrong before we animate
				if (_previousRecord && !_previousRecord->m_Dormant)
				{
					int _TotalNewCommands = m_iTicksChoked + 1;
					Vector _absVeloSlope = (_newRecord->m_AbsVelocity - _previousRecord->m_AbsVelocity) / _TotalNewCommands;
					Vector _absVelo = _previousRecord->m_AbsVelocity;
					Vector _lastvelocity = _previousRecord->m_pAnimStateServer[ResolveSides::NONE] ? _previousRecord->m_pAnimStateServer[ResolveSides::NONE]->m_vVelocity : _previousRecord->m_animstate.m_vVelocity;
					float _simTime = _newRecord->m_flFirstCmdTickbaseTime - TICKS_TO_TIME(1);
					bool lby_timer_updated = false;
					float tm_next_lby_update_time = m_next_lby_update_time;
					for (int i = 0; i < _TotalNewCommands; ++i)
					{
						_absVelo += _absVeloSlope;
						_simTime += TICKS_TO_TIME(1);
						Vector _absVelTmp = _absVelo;
						_absVelTmp.z = 0.0f;
						_lastvelocity = g_LagCompensation.GetSmoothedVelocity(TICKS_TO_TIME(1) * 2000.0f, _absVelTmp, _lastvelocity);
						float m_flSpeed = fminf(_lastvelocity.Length(), 260.0f);

						if (m_flSpeed > 0.1f || fabsf(_absVelo.z) > 100.0f)
						{
							tm_next_lby_update_time = _simTime + 0.22f;
							lby_timer_updated = true;
							break;
						}
						else if (_simTime > tm_next_lby_update_time)
						{
							tm_next_lby_update_time = _simTime + 1.1f;
							lby_timer_updated = true;
							break;
						}
					}

					if (!lby_timer_updated)
					{
						//force the lby timer to change, FIXME: what exact time should we estimate the lby changed?
						m_next_lby_update_time = _newRecord->m_flFirstCmdTickbaseTime + TICKS_TO_TIME(max(0, m_iTicksChoked - 1));
					}
				}
			}

			_newRecord->m_bBalanceAdjust = false;
			_newRecord->m_bFlickedToLBY = false;

			C_AnimationLayer &firesequence = *m_pEntity->GetAnimOverlayDirect(LOWERBODY_LAYER);
			C_AnimationLayer &previousfiresequence = _previousRecord->m_AnimLayer[LOWERBODY_LAYER];
			Activities act = GetSequenceActivity(m_pEntity, firesequence._m_nSequence);
			Activities previousact = GetSequenceActivity(m_pEntity, previousfiresequence._m_nSequence);
			bool _RestartedBalanceAdjust = false;

			m_bWasConsistentlyBalanceAdjusting = m_bIsConsistentlyBalanceAdjusting;

			if (/*!m_bMoving && */act == ACT_CSGO_IDLE_TURN_BALANCEADJUST || act == ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING)
			{
				if (previousact != act || firesequence._m_flCycle < previousfiresequence._m_flCycle)
				{
					_RestartedBalanceAdjust = true;
					_newRecord->m_bBalanceAdjust = true;
					m_flLastBalanceAdjustEyeYaw = m_angPristineEyeAngles.y;
					float _oldTimeSinceLastBalanceAdjust = m_flTimeSinceLastBalanceAdjust;
					m_flTimeSinceLastBalanceAdjust = fmin(_ServerTime - m_flLastBalanceAdjust, 200.0f);
					m_flLastBalanceAdjust = _ServerTime;
					if (m_flTimeSinceLastBalanceAdjust >= 0.9f && _oldTimeSinceLastBalanceAdjust >= 0.9f && _oldTimeSinceLastBalanceAdjust <= 2.5f && m_flTimeSinceLastBalanceAdjust <= 2.5f)
						m_bIsConsistentlyBalanceAdjusting = true;
				}
				//else if (firesequence._m_flCycle < 0.9f)
				//{
				//	_newRecord->m_bBalanceAdjust = true;
				//}
			}

			if (m_flTimeSinceLastBalanceAdjust <= 0.2f || m_flTimeSinceLastBalanceAdjust > 3.0f)
				m_bIsConsistentlyBalanceAdjusting = false;

			_newRecord->m_flTimeSinceLastBalanceAdjust = fmin(_ServerTime - m_flLastBalanceAdjust, 200.0f);

			if (_newRecord->m_SimulationTime > m_next_lby_update_time &&
				!_newRecord->m_bLBYUpdated && _RestartedBalanceAdjust)
			{
				//they are breaking lby with delta > 120 degrees
				_newRecord->m_bFlickedToLBY = true;
				m_bIsBreakingLBYWithLargeDelta = true;
			}

			if (_newRecord->m_flTimeSinceLastBalanceAdjust > 1.5f)
				m_bIsBreakingLBYWithLargeDelta = false;

			// is player legit?
			m_bLegit = m_bIsBot || (!m_bForceNotLegit && IsLegit());

			// reset jittering state
			m_bJitteringYaw = false;
			m_bJitteringYawIsPredictable = false;

			if (!m_bLegit)
			{
				//check to see if they are using a desync method that involves sending two real ticks
				if (m_Tickrecords.size() >= 2)
				{
					//check to see if they are jittering their eye angle yaw
					int _JitterCount = 0;
					float _LastDelta = -999.0f;

					for (size_t i = 0; i < min(m_Tickrecords.size(), 6); ++i)
					{
						CTickrecord* _thisRecord = m_Tickrecords[i];
						// sanity check
						if (i < m_Tickrecords.size() - 1)
						{
							// get record previous to current one
							CTickrecord* _thePreviousRecord = m_Tickrecords[i + 1];
							float _Delta = fabsf(AngleDiff(_thisRecord->m_EyeAngles.y, _thePreviousRecord->m_EyeAngles.y));
							if (_Delta > 35.0f)
							{
								if (_LastDelta != -999.0f && fabsf(AngleDiff(_Delta, _LastDelta) < 10.0f))
									m_bJitteringYawIsPredictable = true; //FIXME TODO: check all of the deltas
								_LastDelta = _Delta;
								++_JitterCount;
							}
						}
					}
					m_bJitteringYaw = _JitterCount >= 2;
				}
			}
		}
		else
		{
			_newRecord->m_flEstimatedDesyncMultiplier = _newRecord->m_flMaxDesyncMultiplier;
			_newRecord->m_flDesyncMultiplierRateOfChange = TICKS_TO_TIME(1);
			_newRecord->m_flEstimatedMaxDesyncDelta = _newRecord->m_flMaxDesyncDelta;
		}

		// legit player or no choked ticks
		if ((m_iTicksChoked == 0 || (m_bLegit && !m_bForceNotLegit)) && _previousRecord)
		{
			if (_previousRecord->m_iTicksChoked == 0 && !m_bTickbaseShiftedBackwards)
				_newRecord->m_bRealTick = true; //Only count as real tick if they send two unchoked ticks in a row
		}
	}

	return _newRecord;
}

bool CPlayerrecord::IsLegit()
{
	// bots are always legit
	if (m_bIsBot)
		return true;

	// init locals
	int _legitRecords = 0, _suspiciousRecords = 0, _suspiciousCount = 0, _numSuspiciousChokes = 0, _numStrafes = 0, _numWideBodyDelta = 0;

	const float _lbyEyeDelta = fabsf(AngleNormalize(m_pEntity->GetLowerBodyYaw() - AngleNormalize(m_angPristineEyeAngles.y)));

	// standing still
	if (m_Tickrecords.size() > 32 && m_pEntity->GetVelocity().Length() <= MINIMUM_PLAYERSPEED_CONSIDERED_MOVING) {
		// eyeangles too far from lby
		if (_lbyEyeDelta > 35.f) {
			if ((GetServerTime() - m_flLastLBYUpdateTime) >= 1.2f)
			{
				m_flNextLegitCheck = Interfaces::Globals->realtime + 1.0f; //keep them as not legit for a specific period of time
				return false; //this is a definite cheater
			}
		}
	}
	else {
		// todo: check layers to see if feet are sliding
	}

	if (m_bIsBreakingLBYWithLargeDelta)
		_suspiciousRecords++;

	// loop through all tickrecords
	for (size_t i = 0; i < m_Tickrecords.size(); i++)
	{
		// get current record
		CTickrecord* _currentRecord = m_Tickrecords[i];

		// check if they are strafing
		if (_currentRecord->m_bStrafing)
		{
			++_numStrafes;
			if (_currentRecord->m_Velocity.Length2D() > 30.0f)
			{
				m_flNextLegitCheck = Interfaces::Globals->realtime + 1.0f; //keep them as not legit for a specific period of time
				return false; //You can't strafe legitimately while moving fast..
			}
		}

		float _pitch = fabsf(AngleNormalize(_currentRecord->m_EyeAngles.x));

		// player didn't choke
		if (_currentRecord->m_iTicksChoked == 0 && !_currentRecord->m_bFiredBullet && !_currentRecord->m_bTickbaseShiftedBackwards && !_currentRecord->m_bTickbaseShiftedForwards && _pitch < 80.f)
			_legitRecords++;
		// player choked
		else
		{
			int _prevSuspiciousCount = _suspiciousCount;
			int _currentChokedCount = _currentRecord->m_iTicksChoked;

			if (_pitch > 80.0f)
				++_suspiciousCount;

			// sanity check
			if (i < m_Tickrecords.size() - 1)
			{
				// get record previous to current one
				CTickrecord* _previousRecord = m_Tickrecords[i + 1];

				// check if breaking lag comp
				if (_currentRecord->m_bTeleporting)
					++_suspiciousCount;

				// check if they are choking the same number of ticks as last record
				if (!_currentRecord->m_bFiredBullet && _currentChokedCount > 1)
				{
					if (_previousRecord->m_iTicksChoked == _currentChokedCount)
						_numSuspiciousChokes++;
				}

				// check if tickbase was shifted
				if (_currentRecord->m_bTickbaseShiftedBackwards || _currentRecord->m_bTickbaseShiftedForwards)
					++_suspiciousCount;

				// is their fake abs yaw far away from their eye yaw
				if (!_currentRecord->m_bMoving && _currentRecord->m_bCachedFakeAngles &&
					fabsf(AngleDiff(_currentRecord->m_angAbsAngles_Fake.y, _currentRecord->m_EyeAngles.y)) >= 57.0f
					)
					++_numWideBodyDelta;

				// calculate rate of angle change per tick
				QAngle _AngleDelta = (_currentRecord->m_EyeAngles - _previousRecord->m_EyeAngles) / (_currentRecord->m_iTicksChoked + 1);
				NormalizeAngles(_AngleDelta);

				float _PitchRateOfChange = fabsf(_AngleDelta.x);
				float _YawRateOfChange = fabsf(_AngleDelta.y);

				if (_currentRecord->m_bFiredBullet && !_previousRecord->m_bFiredBullet)
				{
					// when firing, check if they snapped their pitch a long distance
					if (AngleDiff(_currentRecord->m_EyeAngles.x, _previousRecord->m_EyeAngles.x) > 42.5f)
						++_suspiciousCount;
					else
					{
						// check if rate of change per tick was too much
						if (_PitchRateOfChange > 32.f || _YawRateOfChange > 32.f)
							++_suspiciousCount;
					}
				}
				else
				{
					if (!_currentRecord->m_bFiredBullet && _previousRecord->m_bFiredBullet)
					{
						//They snapped straight up or down after a shot, they can't be legit
						if (fabsf(AngleNormalize(_currentRecord->m_EyeAngles.x)) > 80.0f && fabsf(AngleNormalize(_previousRecord->m_EyeAngles.x)) < 20.0f)
						{
							m_flNextLegitCheck = Interfaces::Globals->realtime + 10.0f; //keep them as not legit for a specific period of time
							return false;
						}
					}

					// check if rate of change was suspicious (snapped)
					if (_PitchRateOfChange > 30.f || _YawRateOfChange > 32.f)
						++_suspiciousCount;
				}
			}

			if (_suspiciousCount > _prevSuspiciousCount)
				_suspiciousRecords++;
			else
				_legitRecords++;
		}

		// too many suspicious records
		if (_suspiciousRecords >= 5)
		{
			m_flNextLegitCheck = Interfaces::Globals->realtime + 1.0f; //keep them as not legit for a specific period of time
			return false;
		}

		// they are strafing an unreasonable amount of times
		if (_numStrafes >= 10)
		{
			m_flNextLegitCheck = Interfaces::Globals->realtime + 1.0f; //keep them as not legit for a specific period of time
			return false;
		}

		// they consistently choke the same number of ticks
		if (_numSuspiciousChokes >= 7)
		{
			m_flNextLegitCheck = Interfaces::Globals->realtime + 1.0f; //keep them as not legit for a specific period of time
			return false;
		}

		// they are desyncing
		if (_numWideBodyDelta >= 10)
		{
			m_flNextLegitCheck = Interfaces::Globals->realtime + 1.0f; //keep them as not legit for a specific period of time
			return false;
		}
	}

	return m_Tickrecords.size() < 6 || _legitRecords >= 6;
}

void CPlayerrecord::UpdateVelocity(CTickrecord *_oldRecord)
{
	// fix bug when players first spawn causing their head to look down and gradually move up
	if (m_pEntity->GetSpawnTime() != m_flSpawnTime)
	{
		m_pEntity->SetLocalVelocity(vecZero);
		m_pEntity->SetBaseVelocity(vecZero);
		m_pEntity->CalcAbsoluteVelocity();
		return;
	}

	// calculate simtime delta
	const float _simTimeDelta = TICKS_TO_TIME(m_iTicksChoked + 1);//m_pEntity->GetSimulationTime() - m_pEntity->GetOldSimulationTime();

	// stop if simtime didn't update
	if (_simTimeDelta == 0.f)
		return;

	// calculate velocity
	m_pEntity->SetLocalVelocity((m_pEntity->GetNetworkOrigin() - m_pEntity->GetOldOrigin()) / _simTimeDelta);
	m_pEntity->CalcAbsoluteVelocity();

	// server does not network the base velocity. set it here
	// it does not fix velocity added on by triggers, unfortunately.
	if (!m_pEntity->IsObserver() && _oldRecord)
	{
		CBaseEntity *groundEntity = m_pEntity->GetGroundEntity();
		if (!groundEntity)
			m_pEntity->SetBaseVelocity(vecZero);
		else
		{
			//Vector baseVelocity;
			//simulate in place to get base velocity
			//PlayerBackup_t *backup = new PlayerBackup_t;
			//backup->Get(m_pEntity);
			//SimulatePlayer(_oldRecord, _oldRecord, _oldRecord->m_iServerTick, true, false, false);
			//baseVelocity = m_pEntity->GetBaseVelocity();
			//backup->RestoreData(true, false);
			//keep the base velocity calculated from game movement
			//m_pEntity->SetBaseVelocity(baseVelocity);
			//delete backup;
			m_pEntity->SetBaseVelocity(groundEntity->GetAbsVelocityDirect());
		}
	}
}

void CPlayerrecord::Reset(bool FullClear)
{
	m_TargetRecord = nullptr;
	if (m_RestoreTick && m_RestoreTickIsDynamic)
		delete m_RestoreTick;
	m_RestoreTickIsDynamic = false;
	m_RestoreTick = nullptr;

	m_bNetUpdate = false;
	m_bNetUpdateSilent = false;

	m_pEntity = nullptr;
	m_pWeapon = nullptr;

	m_bIsBot = false;
	m_bConnected = false;

	m_iTicksChoked = 0;
	m_iTicksSinceLastServerUpdate = 0;

	m_flTotalInvalidRecordTime = 0.0f;
	m_flTimeSinceStartedPressingUseKey = 0.f;
	m_flLastFlipSideTime = 0.0f;

	m_bHasWeaponInfo = false;

	//remove balanceadjust when dormant to allow other detection
	//m_DesyncResults &= ~DESYNC_RESULTS_BALANCEADJUST;

	//Only reset resolved state if they disconnected
	if (FullClear)
	{
		hashedsteamid = 0;
		m_bAlreadyDead = false;
		m_bAlreadyResetRecord = false;
		m_bAlreadySpectatingOrDormant = false;

		m_bAllowMovingResolver = true;
		m_bAllowJitterResolver = true;
		m_bAllowBodyHitResolver = true;

		m_ivphysicsCollisionState = 0;
		m_iResolveMode = RESOLVE_MODE_NONE;
		m_iOldResolveMode = RESOLVE_MODE_NONE;
		m_iBackupOldResolveMode = RESOLVE_MODE_NONE;
		m_iOldResolveSide = ResolveSides::INVALID_RESOLVE_SIDE;
		m_iBackupOldResolveSide = ResolveSides::INVALID_RESOLVE_SIDE;

		m_flLastMovingResolverDelta = FLT_MAX;
		m_flLastTimeResolvedFromMoving = 0.0f;
		m_nLastMovingResolveSide = ResolveSides::INVALID_RESOLVE_SIDE;
		m_flLastDetectedEyeDelta = FLT_MAX;

		m_pKilledByEntity = nullptr;
		m_iLastKilledTickcount = 0;

		m_bResolved = false;
		m_bIsDuplicateTickbase = false;
		m_bTickbaseShiftedBackwards = false;
		m_bTickbaseShiftedForwards = false;
		m_DesyncResults = 0;
		m_BalanceAdjustResults = 0;
		m_bForceNotLegit = false;
		m_flNextLegitCheck = FLT_MIN;
		m_bLegit = true;
		m_nServerTickCount = 0;
		m_nTickbase = 0;
		m_vecOBBMins = vecZero;
		m_vecOBBMaxs = { 0.0f, 0.0f, 64.0f };
		m_vecCurNetOrigin = vecZero;
		m_iShotsMissed = 0;
		m_iShotsMissed_BalanceAdjust = 0;
		m_iShotsMissed_MovingResolver = 0;
		m_flSpawnTime = 0.f;
		m_flLastTimeDetectedJitter = 0.f;

		LocalPlayer.ShotMutex.Lock();
		for (auto &shot =  ShotRecords.begin(); shot != ShotRecords.end();)
		{
			delete *shot;
			shot = ShotRecords.erase(shot);
		}
		LocalPlayer.ShotMutex.Unlock();

		for (int i = 0; i < MAX_RESOLVE_SIDES; ++i)
		{
			m_bMovingLBYMemeFlipState[i] = false;
			CCSGOPlayerAnimState* animstate = m_pAnimStateServer[i];
			if (animstate)
			{
				delete animstate;
				m_pAnimStateServer[i] = nullptr;
			}
		}
	}

	m_bSpectating = false;
	m_bImmune = false;
	m_bLocalPlayer = false;
	m_bMoving = false;
	m_bIsHoldingDuck = false;
	m_bTeleporting = false;

	m_angEyeAnglesNotFiring = QAngle(0.f, 0.f, 0.f);
	m_flLastShotSimtime = 0.f;

	m_angLastShotAngle = QAngle(0.f, 0.f, 0.f);

	m_flLowerBodyYaw = 0.f;
	m_flLastLowerBodyYaw = 0.f;
	m_flLastLBYUpdateTime = 0.f;
	m_flLastBalanceAdjust = 0.f;
	m_flSimulationTime = 0.f;
	m_flFirstCmdTickbaseTime = 0.f;
	m_iFirstCmdTickbase = 0;
	m_flFirstCmdTickbaseTime = 0.f;
	m_flNewestSimulationTime = 0.f;
	m_iNewestTickbase = 0;
	m_bIsBreakingLBYWithLargeDelta = false;
	m_bPredictLBY = false;
	m_bTickbaseShiftedBackwards = false;
	m_bIsDuplicateTickbase = false;

	m_bDormant = true;
	m_bIsSpectating = false;

	if (FullClear)
		memset(&DataUpdateVars, 0, sizeof(s_DataUpdateVars));

	Changed.Reset();
	if (FullClear)
	{
		LastShot.Reset();
		LastShotAsEnemy.Reset();
	}

	m_iLifeState = LIFE_DEAD;

	m_next_lby_update_time = 0.0f;
	m_lastvelocity.Init(0, 0, 0);
	m_flTimeSinceLastBalanceAdjust = 0.0f;
	m_bIsConsistentlyBalanceAdjusting = false;
	m_bWasConsistentlyBalanceAdjusting = false;
	m_bAllowAnimationEvents = false;
	m_bAwaitingFlashedResult = false;

	m_fake_curfeetyaw = 0.0f;
	m_fake_goalfeetyaw = 0.0f;
	m_fake_duckamount = 0.0f;
	m_fake_last_animation_update_time = 0.0f;
	m_fake_velocity.Init();

	if (FullClear)
	{
		Impact.Reset();
		LastShot.m_vecMainImpactPos.Init();
		LastShotAsEnemy.m_vecMainImpactPos.Init();

		if (!m_Tickrecords.empty())
		{
			for (auto i = m_Tickrecords.begin(); i != m_Tickrecords.end();)
			{
				delete *i;
				i = m_Tickrecords.erase(i);
			}
		}
	}
	else
	{
		if (!m_Tickrecords.empty())
		{
			if (!m_Tickrecords[0]->m_Dormant)
			{
				for (auto& tick : m_Tickrecords)
				{
					tick->m_Dormant = true;
				}
			}
		}
	}

	m_iBaimReason = BAIM_REASON_MAX;
	m_iTicksChokedHistory.clear();
}

void CPlayerrecord::StorePlayerState(CTickrecord *record)
{
	// save current values
	if (record)
	{
		if (m_RestoreTick && m_RestoreTickIsDynamic)
			delete m_RestoreTick;
		m_RestoreTickIsDynamic = false;
		m_RestoreTick = record;
		return;
	}

	m_RestoreTick = new CTickrecord(m_pEntity);
	m_RestoreTick->m_PlayerBackup.Get(m_pEntity);
	m_RestoreTickIsDynamic = true;
}

void CPlayerrecord::UpdateInfo(int _index)
{
	// get entity by index
	m_pEntity = Interfaces::ClientEntList->GetBaseEntity(_index);

	// valid entity, player and alive
	if (m_pEntity && m_pEntity->IsPlayer())
	{
		m_flSimulationTime = m_pEntity->GetSimulationTime();

		m_pEntity->GetPlayerInfo(&m_PlayerInfo);
		uint32_t hash = HashString_MUTINY(m_PlayerInfo.guid);
		if (hashedsteamid != hash)
		{
			memcpy(steamid, m_PlayerInfo.guid, 33);
			hashedsteamid = hash;
			EncStr(steamid, 32);
			faresprecordmutex.lock();
			for (auto i = FarESPPackets.begin(); i != FarESPPackets.end();)
			{
				delete (*i);
				i = FarESPPackets.erase(i);
			}
			faresprecordmutex.unlock();
			memset(&LastServerSidePacket, 0, sizeof(CSGOPacket));
		}

		m_bIsBot = m_pEntity->IsBot();

#if 0
		const char *_ClanTag = g_PR->GetClan(_index);
		if (_ClanTag && m_iCheatID == CheatNames::UNKNOWN)
		{
			//decrypts(0)
			if (!stricmp(_ClanTag, XorStr("fatality.win")))
				m_iCheatID = CheatNames::FATALITY;
			else if (!stricmp(_ClanTag, XorStr("ev0lve.xyz")))
				m_iCheatID = CheatNames::EVOLVE;
			//encrypts(0)
		}
#endif

		if (!m_pEntity->GetAlive())
		{
			if (!m_bAlreadyDead)
			{
				CBaseEntity* _backupEntity = m_pEntity;
				Reset(false);
				m_bConnected = true;
				m_pEntity = _backupEntity;
				m_iLifeState = LIFE_DEAD;
				m_bDormant = m_pEntity->GetDormant();
				if (m_bIsSpectating = m_pEntity->IsObserver())
					g_Info.m_SpectatorList.push_back(m_pEntity->index);

				// create the server animstate
				for (CCSGOPlayerAnimState** i = &m_pAnimStateServer[0]; i != &m_pAnimStateServer[MAX_RESOLVE_SIDES]; ++i)
				{
					if (*i && (*i)->pBaseEntity != m_pEntity)
					{
						delete *i;
						*i = nullptr;
					}

					if (!(*i))
						*i = CreateCSGOPlayerAnimState(m_pEntity);
				}

				// mark old records as dormant
				for (auto& tick : m_Tickrecords)
				{
					tick->m_Dormant = true;
				}
				m_bAlreadyDead = true;
				m_bAlreadySpectatingOrDormant = false;
			}
		}
		else
		{
			m_bAlreadyDead = false;

			m_iLifeState = m_pEntity->GetLifeState();
			m_bDormant = m_pEntity->GetDormant();

			// connected
			m_bConnected = true;

			// localplayer?
			m_bLocalPlayer = m_pEntity == LocalPlayer.Entity;

			// spectating?
			if (m_bIsSpectating = m_pEntity->IsObserver())
				g_Info.m_SpectatorList.push_back(m_pEntity->index);

			// spawn protected
			m_bImmune = m_pEntity->GetImmune();

			// we're done here
			if (m_bSpectating || m_bDormant)
			{
				if (!m_bAlreadySpectatingOrDormant)
				{
					// mark old records as dormant
					for (auto& tick : m_Tickrecords)
					{
						tick->m_Dormant = true;
					}
					m_bAlreadySpectatingOrDormant = true;
				}
				return;
			}

			m_bAlreadySpectatingOrDormant = false;
			m_bAlreadyResetRecord = false;
			
			// create the server animstate
			for (CCSGOPlayerAnimState** i = &m_pAnimStateServer[0]; i != &m_pAnimStateServer[MAX_RESOLVE_SIDES]; ++i)
			{
				if (*i && (*i)->pBaseEntity != m_pEntity)
				{
					delete *i;
					*i = nullptr;
				}

				if (!(*i))
					*i = CreateCSGOPlayerAnimState(m_pEntity);
			}

			if (m_bAwaitingFlashedResult)
			{
				const float _simulationTime = m_pEntity->GetSimulationTime();
				const float _startTime = _simulationTime;
				const float _flashDuration = m_pEntity->IsLocalPlayer() ? LocalPlayer.m_flFlashedDuration : m_pEntity->GetFlashDuration();
				const float _endTime = _simulationTime + _flashDuration;
				for (CCSGOPlayerAnimState** i = &m_pAnimStateServer[0]; i != &m_pAnimStateServer[MAX_RESOLVE_SIDES]; ++i)
				{
					CCSGOPlayerAnimState* animstate = *i;
					if (animstate)
					{
						animstate->m_flFlashedStartTime = _startTime;
						animstate->m_flFlashedEndTime = _endTime;
					}
				}
				m_bAwaitingFlashedResult = false;
			}
		}
	}
	else
	{
		// invalid, reset
		if (!m_bAlreadyResetRecord)
		{
			Reset(true);
			m_bAlreadyResetRecord = true;
		}
	}
}

CTickrecord* CPlayerrecord::GetBestRecord(bool PreferBacktrackUnlessNewestIsBest, std::vector<CTickrecord*>* _excluded)
{
	// we don't have any records
	if (m_Tickrecords.empty())
		return nullptr;

	int best_weight = -1;

	CTickrecord *best_record = nullptr;

	PreferBacktrackUnlessNewestIsBest = false;

	if (!PreferBacktrackUnlessNewestIsBest)
	{
		// loop through all ticks
		for (auto& _record : m_Tickrecords)
		{
			// skip invalid records
			if (!_record->IsValid(m_pEntity))
				continue;

			// skip excluded records
			if (_excluded && std::find(_excluded->begin(), _excluded->end(), _record) != _excluded->end())
				continue;

			auto weight = _record->GetWeight(best_record, &best_weight);

			// get the newest record that has the best weight
			if (weight > best_weight)
			{
				best_weight = weight;
				best_record = _record;
			}
		}
	}
	else
	{
		for (auto& i = m_Tickrecords.rbegin(); i != m_Tickrecords.rend(); ++i)
		{
			auto& _record = *i;

			// skip invalid records
			if (!_record->IsValid(m_pEntity))
				continue;

			// skip excluded records
			if (_excluded && std::find(_excluded->begin(), _excluded->end(), _record) != _excluded->end())
				continue;

			auto weight = _record->GetWeight(best_record, &best_weight);

			if (weight == _record->GetMaxWeight())
			{
				//This record is already the best it can get and it's not excluded, so just return it immediately
				best_weight = weight;
				best_record = _record;
				break;
			}

			// get the oldest record that has the best weight
			if (weight > best_weight)
			{
				best_weight = weight;
				best_record = _record;
			}
		}

		auto _currentRecord = GetCurrentRecord();

		//Check to see if the newest record weight matches the best record and it isn't excluded. If so, just return the current record
		if (_currentRecord && best_record && best_record != _currentRecord)
		{
			if (!_excluded || std::find(_excluded->begin(), _excluded->end(), _currentRecord) == _excluded->end())
			{
				if (_currentRecord->GetWeight() >= best_weight)
					best_record = _currentRecord;
			}
		}
	}

	return best_record;
}

CTickrecord* CPlayerrecord::GetCurrentRecord()
{
	return !m_Tickrecords.empty() ? m_Tickrecords.front() : nullptr;
}

CTickrecord* CPlayerrecord::GetPreviousRecord()
{
	return m_Tickrecords.size() >= 2 ? m_Tickrecords[1] : nullptr;
}

CTickrecord* CPlayerrecord::GetPrePreviousRecord()
{
	return m_Tickrecords.size() >= 3 ? m_Tickrecords[2] : nullptr;
}

CTickrecord* CPlayerrecord::GetNewestValidRecord()
{
	// we don't have any records
	if (m_Tickrecords.empty())
		return nullptr;

	// init return value
	CTickrecord* _best = nullptr;

	// skip invalid records
	if (!m_Tickrecords.front()->IsValid(m_pEntity))
		return nullptr;

	// set last record
	_best = m_Tickrecords.front();

	return _best;
}

CTickrecord* CPlayerrecord::GetOldestValidRecord()
{
	// we don't have any records
	if (m_Tickrecords.empty())
		return nullptr;

	// init return value
	CTickrecord* _last = nullptr;

	// loop through all ticks
	for (size_t i = 0; i < m_Tickrecords.size(); i++)
	{
		// skip invalid records
		if (!m_Tickrecords[i]->IsValid(m_pEntity))
			continue;

		// set last record
		_last = m_Tickrecords[i];
	}

	return _last;
}

CTickrecord* CPlayerrecord::FindTickRecord(CTickrecord* record)
{
	if (!record || m_Tickrecords.empty())
		return nullptr;

	for (auto rec : m_Tickrecords)
	{
		if (rec == record)
		{
			return rec;
		}
	}

	return nullptr;
}

int CPlayerrecord::GetValidRecordCount()
{
	return m_ValidRecordCount;
}

bool CPlayerrecord::CacheBones(float _time, bool _force, CTickrecord* _record) const
{
	if (_record)
	{
		CUtlVectorSimple* cache = m_pEntity->GetCachedBoneData();
		auto& state = _record->m_PlayerBackup;

		if (_force || !_record->m_bCachedBones || cache->Count() < state.CachedBonesCount)
		{
			//Update bone cache
			CBoneAccessor *accessor = m_pEntity->GetBoneAccessor();
			m_pEntity->InvalidateBoneCache();
			m_pEntity->SetLastOcclusionCheckFlags(0);
			m_pEntity->SetLastOcclusionCheckFrameCount(Interfaces::Globals->framecount);
			m_pEntity->SetupBones(state.CachedBoneMatrices, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, _time);
			state.CachedBonesCount = cache->count;
			state.ReadableBones = accessor->GetReadableBones();
			state.WritableBones = accessor->GetWritableBones();
			_record->m_bCachedBones = true;
			_record->m_EntityToWorldTransform = m_pEntity->EntityToWorldTransform();
			m_pEntity->ComputeHitboxSurroundingBox(&_record->m_HitboxWorldMins, &_record->m_HitboxWorldMaxs, state.CachedBoneMatrices);

			//draw box surrounding player
			//Vector& mins = _record->m_HitboxWorldMins;
			//Vector& maxs = _record->m_HitboxWorldMaxs;
			//Interfaces::DebugOverlay->AddLineOverlay(mins, maxs, 255, 0, 0, true, TICKS_TO_TIME(5));
			//Interfaces::DebugOverlay->AddBoxOverlay(_record->m_AbsOrigin, mins - _record->m_AbsOrigin, maxs - _record->m_AbsOrigin, angZero, 255, 0, 0, 255, TICKS_TO_TIME(5));
			
			return true;
		}

		//Use old bone cache
		CBoneAccessor *accessor = m_pEntity->GetBoneAccessor();
		accessor->SetReadableBones(state.ReadableBones);
		accessor->SetWritableBones(state.WritableBones);
		memcpy(cache->Base(), state.CachedBoneMatrices, sizeof(matrix3x4_t) * state.CachedBonesCount);
		if (accessor->GetBoneArrayForWrite() != cache->Base())
		{
#ifdef _DEBUG
			DebugBreak();
#endif
			accessor->SetBoneArrayForWrite((matrix3x4a_t*)cache->Base());
		}
		//prevent further bone setups unless mask is bad
		m_pEntity->SetLastBoneSetupTime(Interfaces::Prediction->InPrediction() ? Interfaces::Prediction->m_SavedVars.curtime : Interfaces::Globals->curtime);

		//update the attachment positions if wanted
		if (AllowSetupBonesToUpdateAttachments)
			m_pEntity->Wrap_AttachmentHelper();

		return true;
	}
	return false;
}

bool CPlayerrecord::IsValidTarget() const
{
	return !m_bImmune && !m_bSpectating && m_pEntity && m_pEntity->IsEnemy(LocalPlayer.Entity);
}

FarESPPlayer* CPlayerrecord::GetFarESPPacket(int index)
{
#ifndef USE_FAR_ESP
	return nullptr;
#else
	if (variable::get().visuals.pf_enemy.b_faresp)
	{
		faresprecordmutex.lock();
		if (index < FarESPPackets.size())
		{
			FarESPPlayer* lastpacket = FarESPPackets[0];
			if (fabsf(Interfaces::Globals->curtime - lastpacket->simulationtime) <= 1.0f && lastpacket->simulationtime > m_pEntity->GetSimulationTime())
			{
				faresprecordmutex.unlock();
				return lastpacket;
			}
		}
		faresprecordmutex.unlock();
	}
	return nullptr;
#endif
}

FarESPPlayer *CPlayerrecord::IsValidFarESPTarget()
{
#ifndef USE_FAR_ESP
	return nullptr;
#else
	if (variable::get().visuals.pf_enemy.b_faresp)
	{
		FarESPPlayer* packet = GetFarESPPacket();
		if (packet && m_pEntity != LocalPlayer.Entity && (!m_bIsSpectating || (packet->health > 0)) && m_pAnimStateServer[0])
			return packet;
	}
	return nullptr;
#endif
}

FarESPPlayer *CPlayerrecord::IsValidFarESPAimbotTarget()
{
#ifndef USE_FAR_ESP
	return nullptr;
#else
	if (variable::get().visuals.pf_enemy.b_faresp)
	{
		FarESPPlayer* packet = IsValidFarESPTarget();
		if (packet && (LocalPlayer.Entity && m_pEntity && m_pEntity->IsEnemy(LocalPlayer.Entity)))
			return packet;
	}

	return nullptr;
#endif
}

CSGOPacket* CPlayerrecord::GetServerSidePacket()
{
#ifndef USE_SERVER_SIDE
	return nullptr;
#else
	if (LastServerSidePacket.bAlive && Interfaces::Globals->curtime - LastServerSidePacket.flSimulationTime < 5.0f)
	{
		return &LastServerSidePacket;
	}
	return nullptr;
#endif
}

CSGOPacket *CPlayerrecord::IsValidServerSideTarget()
{
#ifndef USE_SERVER_SIDE
	return nullptr;
#else
	CSGOPacket* packet = GetServerSidePacket();
	if (packet && m_pEntity != LocalPlayer.Entity && !m_bIsSpectating)
		return packet;
	return nullptr;
#endif
}

CSGOPacket *CPlayerrecord::IsValidServerSideAimbotTarget()
{
#ifndef USE_SERVER_SIDE
	return nullptr;
#else
	CSGOPacket* packet = GetServerSidePacket();
	if (packet && LocalPlayer.Entity && m_pEntity->IsEnemy(LocalPlayer.Entity))
		return packet;
	return nullptr;
#endif
}

#ifdef _DEBUG
std::ofstream animationsfile;
std::ofstream animationsfile2;

void CPlayerrecord::WriteToAnimationFile(char* str)
{
	if (!animationsfile.is_open())
	{
		animationsfile.open("G:\\debug_an.txt", std::ios::out);
	}

	if (animationsfile.is_open())
	{
		animationsfile << str << std::endl;
	}
}


void CPlayerrecord::OutputAnimations()
{
	if (!animationsfile.is_open())
	{
		animationsfile.open("G:\\debug_an.txt", std::ios::out);
	}

	if (!animationsfile2.is_open())
	{
		animationsfile2.open("G:\\debug_an2.txt", std::ios::out);
	}

	if (animationsfile.is_open())
	{
#if 1
		bool update = false;
#else
		static float lastnextupdatetime = 0.0f;
		DWORD cl = GetServerClientEntity(1);
		CBaseEntity *pEnt = ServerClientToEntity(cl);
		float nextupdatetime = pEnt->GetNextLowerBodyyawUpdateTimeServer();
		bool update = false;

		if (nextupdatetime != lastnextupdatetime)
		{
			animationsfile << "Server LBY Update\n";
			lastnextupdatetime = nextupdatetime;
			update = true;
		}
#endif

		int numoverlays = m_pEntity->GetNumAnimOverlays();

		for (int i = 0; i < numoverlays; i++)
		{
			C_AnimationLayer *layer = m_pEntity->GetAnimOverlay(i);
			C_AnimationLayer *oldlayer = &lastoutput_layers[i];

#if 1
			if (layer->_m_nSequence != oldlayer->_m_nSequence)
			{
				animationsfile << "#" << m_pEntity->index << " OL: " << i << " Tick: " << Interfaces::Globals->tickcount << " Sequence: " << oldlayer->_m_nSequence << " -> " << layer->_m_nSequence << " Activity: " << GetSequenceActivityNameForModel(m_pEntity->GetModelPtr(), oldlayer->_m_nSequence) << " -> " << GetSequenceActivityNameForModel(m_pEntity->GetModelPtr(), layer->_m_nSequence) << "\n";
				update = true;
			}
#endif

			if (layer->_m_flCycle != oldlayer->_m_flCycle)
			{
				animationsfile << "#" << m_pEntity->index << " OL: " << i << " Tick: " << Interfaces::Globals->tickcount << " Cycle: " << oldlayer->_m_flCycle << " -> " << layer->_m_flCycle << "\n";
				update = true;
			}

			if (layer->_m_flPlaybackRate != oldlayer->_m_flPlaybackRate)
			{
				animationsfile << "#" << m_pEntity->index << " OL: " << i << " Tick: " << Interfaces::Globals->tickcount << " Playback: " << oldlayer->_m_flPlaybackRate << " -> " << layer->_m_flPlaybackRate << "\n";
				update = true;
			}

			if (layer->m_nOrder != oldlayer->m_nOrder)
			{
				animationsfile << "#" << m_pEntity->index << " OL: " << i << " Tick: " << Interfaces::Globals->tickcount << " Order: " << oldlayer->m_nOrder << " -> " << layer->m_nOrder << "\n";
				update = true;
			}

			if (layer->m_nInvalidatePhysicsBits != oldlayer->m_nInvalidatePhysicsBits)
			{
				animationsfile << "#" << m_pEntity->index << " OL: " << i << " Tick: " << Interfaces::Globals->tickcount << " Physics: " << oldlayer->m_nInvalidatePhysicsBits << " -> " << layer->m_nInvalidatePhysicsBits << "\n";
				update = true;
			}

			if (layer->m_flWeight != oldlayer->m_flWeight)
			{
				animationsfile << "#" << m_pEntity->index << " OL: " << i << " Tick: " << Interfaces::Globals->tickcount << " Weight: " << oldlayer->m_flWeight << " -> " << layer->m_flWeight << "\n";
				update = true;
			}


			if (layer->m_flWeightDeltaRate != oldlayer->m_flWeightDeltaRate)
			{
				animationsfile << "#" << m_pEntity->index << " OL: " << i << " Tick: " << Interfaces::Globals->tickcount << " WeightDeltaRate: " << oldlayer->m_flWeightDeltaRate << " -> " << layer->m_flWeightDeltaRate << "\n";
				update = true;
			}

			if (layer->m_flLayerAnimtime != oldlayer->m_flLayerAnimtime)
			{
				animationsfile << "#" << m_pEntity->index << " OL: " << i << " Tick: " << Interfaces::Globals->tickcount << " AnimTime: " << oldlayer->m_flLayerAnimtime << " -> " << layer->m_flLayerAnimtime << "\n";
				update = true;
			}

			if (update)
				*oldlayer = *layer;

			if (animationsfile2.is_open() && (i == 3 || i == 6))
			{
				animationsfile2 << "#" << m_pEntity->index << " OL: " << i << " Tick: " << Interfaces::Globals->tickcount << " Sequence: " << layer->_m_nSequence << " Activity: " << GetSequenceActivityNameForModel(m_pEntity->GetModelPtr(), layer->_m_nSequence) << "\n";
				animationsfile2 << "#" << m_pEntity->index << " OL: " << i << " Tick: " << Interfaces::Globals->tickcount << " Cycle: " << layer->_m_flCycle << "\n";
				animationsfile2 << "#" << m_pEntity->index << " OL: " << i << " Tick: " << Interfaces::Globals->tickcount << " Playback: " << layer->_m_flPlaybackRate << "\n";
				animationsfile2 << "#" << m_pEntity->index << " OL: " << i << " Tick: " << Interfaces::Globals->tickcount << " Order: " << layer->m_nOrder << "\n";
				animationsfile2 << "#" << m_pEntity->index << " OL: " << i << " Tick: " << Interfaces::Globals->tickcount << " Physics: " << layer->m_nInvalidatePhysicsBits << "\n";
				animationsfile2 << "#" << m_pEntity->index << " OL: " << i << " Tick: " << Interfaces::Globals->tickcount << " Weight: " << layer->m_flWeight << "\n";
				animationsfile2 << "#" << m_pEntity->index << " OL: " << i << " Tick: " << Interfaces::Globals->tickcount << " WeightDeltaRate: " << layer->m_flWeightDeltaRate << "\n";
				animationsfile2 << "#" << m_pEntity->index << " OL: " << i << " Tick: " << Interfaces::Globals->tickcount << " AnimTime: " << layer->m_flLayerAnimtime << "\n";
			}
		}

		if (update)
			animationsfile << "\n" << std::endl;

		animationsfile2 << "EyeYaw: " << AngleNormalize(m_angPristineEyeAngles.y) << " LBY: " << m_pEntity->GetLowerBodyYaw() << " Delta: " << AngleNormalize(AngleDiff(m_pEntity->GetLowerBodyYaw(), m_angPristineEyeAngles.y)) << "\n";
		animationsfile2 << "\n" << std::endl;
		animationsfile.flush();
		animationsfile2.flush();
	}
}
#endif

void CPlayerrecord::FSN_AnimateTicks(CTickrecord* _previousRecord, CTickrecord* _currentRecord, CCSGOPlayerAnimState* animstate, float _desyncYawAmount, float _desyncAbsYawAmount, bool Resolve, int mode, bool _RestoreAnimstate, bool _RestoreServerAnimLayers, bool _RunOnlySetupVelocitySetupLean)
{
	QAngle _chokedEyeAngles = { _currentRecord->m_EyeAngles.x, _currentRecord->m_EyeAngles.y, 0.0f };
	float _chokedLBY;
	using SimulateFn = unsigned char(__thiscall*)(CBaseEntity*);
	SimulateFn oSimulate = nullptr;
	float _prevduckamount = 0.0f;

	_prevduckamount = animstate->m_fDuckAmount;
	if (Hooks.m_bHookedBaseEntity)
		oSimulate = (SimulateFn)Hooks.BaseEntity->GetOriginalHookedSub4();

	if (!_previousRecord || _previousRecord->m_Dormant || m_pEntity->GetSpawnTime() != m_flSpawnTime)
	{
		animstate->m_flLastClientSideAnimationUpdateTime = m_flFirstCmdTickbaseTime - TICK_INTERVAL;
		animstate->m_flGoalFeetYaw = AngleNormalize(_currentRecord->m_EyeAngles.y + _desyncAbsYawAmount);
		animstate->m_bOnGround = _currentRecord->m_Flags & FL_ONGROUND;
	}

	// simulate player because he choked
	if (_previousRecord && !_previousRecord->m_Dormant && m_iTicksChoked > 0 && _currentRecord->m_flSpawnTime == _previousRecord->m_flSpawnTime)
	{
		if (oSimulate) oSimulate(m_pEntity);

		// backup the global times
		//const float _backupCurTime = Interfaces::Globals->curtime;
		//const float _backupFrameTime = Interfaces::Globals->frametime;
		const float _backupLBY = m_pEntity->GetLowerBodyYaw();

		// store some information
		float _ShotTime = m_pWeapon && _currentRecord->m_bFiredBullet ? m_pWeapon->GetLastShotTime() : FLT_MAX;
		int _TotalNewCommands = m_iTicksChoked + 1;
		float _TotalCommandTime = TICKS_TO_TIME(_TotalNewCommands);
		int _FinalCommandIndex = _TotalNewCommands - 1;
		int _SecondToLastCommandIndex = _TotalNewCommands - 2;
		int _numTicksToFinalEyeAngles = _currentRecord->m_bFiredBullet ? TIME_TO_TICKS(_ShotTime - m_flFirstCmdTickbaseTime) + 1 : _TotalNewCommands;

		// calculate slopes according to ticks choked or when shot
		QAngle _eyeAngleSlope = QAngle(
			AngleDiff(_currentRecord->m_EyeAngles.x, _previousRecord->m_EyeAngles.x),
			AngleDiff(_currentRecord->m_EyeAngles.y, _previousRecord->m_EyeAngles.y),
			AngleDiff(_currentRecord->m_EyeAngles.z, _previousRecord->m_EyeAngles.z))
			/ _numTicksToFinalEyeAngles;
		Vector _netOriginSlope = (_currentRecord->m_NetOrigin - _previousRecord->m_NetOrigin) / _TotalNewCommands;
		Vector _absOriginSlope = (_currentRecord->m_AbsOrigin - _previousRecord->m_AbsOrigin) / _TotalNewCommands;
		float _duckAmountSlope = (_currentRecord->m_DuckAmount - _previousRecord->m_DuckAmount) / _TotalNewCommands;
		float _duckSpeedSlope = (_currentRecord->m_DuckSpeed - _previousRecord->m_DuckSpeed) / _TotalNewCommands;
		Vector _minsSlope = (_currentRecord->m_Mins - _previousRecord->m_Mins) / _TotalNewCommands;
		Vector _maxsSlope = (_currentRecord->m_Maxs - _previousRecord->m_Maxs) / _TotalNewCommands;
		Vector _localVeloSlope = (_currentRecord->m_Velocity - _previousRecord->m_Velocity) / _TotalNewCommands;
		Vector _absVeloSlope = (_currentRecord->m_AbsVelocity - _previousRecord->m_AbsVelocity) / _TotalNewCommands;
		Vector _baseVeloSlope = (_currentRecord->m_BaseVelocity - _previousRecord->m_BaseVelocity) / _TotalNewCommands;

		// Restore the netvars to previous record
		QAngle _lerpedEyeAngles = NormalizeAngles_r(_previousRecord->m_EyeAngles);
		Vector _netOrigin = _previousRecord->m_NetOrigin;
		Vector _AbsOrigin = _previousRecord->m_AbsOrigin;
		float _DuckAmount = _previousRecord->m_DuckAmount;
		float _DuckSpeed = _previousRecord->m_DuckSpeed;
		Vector _Mins = _previousRecord->m_Mins;
		Vector _Maxs = _previousRecord->m_Maxs;
		Vector _localVelocity = _previousRecord->m_Velocity;
		Vector _absVelocity = _previousRecord->m_AbsVelocity;
		Vector _baseVelocity = _previousRecord->m_BaseVelocity;

		if (_currentRecord->m_bFiredBullet)
		{
			_chokedEyeAngles.y = AngleNormalize(_previousRecord->m_EyeAngles.y + _desyncYawAmount);
			_chokedLBY = AngleNormalize(_previousRecord->m_EyeAngles.y + _desyncAbsYawAmount);
		}
		else
		{
			_chokedEyeAngles.y = AngleNormalize(_currentRecord->m_EyeAngles.y + _desyncYawAmount);
			_chokedLBY = AngleNormalize(_currentRecord->m_EyeAngles.y + _desyncAbsYawAmount);
		}

		//Restore times to the last simulation time, so we animate like the server
		float curtime = _currentRecord->m_flFirstCmdTickbaseTime - TICK_INTERVAL;
		//Interfaces::Globals->curtime = _currentRecord->m_flFirstCmdTickbaseTime - TICK_INTERVAL;
		//Interfaces::Globals->frametime = Interfaces::Globals->interval_per_tick;

		// restore the on ground information

		if (_previousRecord->m_Flags & FL_ONGROUND)
			m_pEntity->AddFlag(FL_ONGROUND);
		else
			m_pEntity->RemoveFlag(FL_ONGROUND);

		bool _restoreInjury = false;
		float _timeOfLastInjury;
		int _lastHitgroup;
		int _relativeDirectionOfLastInjury;
		int _fireCount;
		int _currentMoveState = m_pEntity->GetMoveState();

		//Fix the move state so that SetupMovement works properly
		if (_currentMoveState == 0)
		{
			float speed = _currentRecord->m_Velocity.Length();
			float oldspeed = _previousRecord->m_Velocity.Length();
			if (_currentRecord->m_bIsUsingMovingLBYMeme
				|| (speed > 1.5f && (speed >= oldspeed || fabsf(speed - oldspeed) < 0.5f)))
			{
				m_pEntity->SetMoveState(_previousRecord->m_MoveState == 0 ? 2 : _previousRecord->m_MoveState);
			}
		}

		if (_RestoreAnimstate)
		{
			m_pEntity->WriteAnimLayers(_previousRecord->m_AnimLayer);
			_timeOfLastInjury = m_pEntity->GetTimeOfLastInjury();
			if (_timeOfLastInjury != _previousRecord->m_PlayerBackup.TimeOfLastInjury)
			{
				_lastHitgroup = m_pEntity->GetLastHitgroup();
				_relativeDirectionOfLastInjury = m_pEntity->GetRelativeDirectionOfLastInjury();
				_fireCount = m_pEntity->GetFireCount();

				//restore last injury
				m_pEntity->SetLastHitgroup(_previousRecord->m_PlayerBackup.LastHitgroup);
				m_pEntity->SetRelativeDirectionOfLastInjury(_previousRecord->m_PlayerBackup.RelativeDirectionOfLastInjury);
				m_pEntity->SetFireCount(_previousRecord->m_PlayerBackup.FireCount);
				_restoreInjury = true;
			}
		}

		Vector tm_velocity = m_lastvelocity;
		float tm_next_lby_update_time = m_next_lby_update_time;

		// restore original lby
		m_pEntity->SetLowerBodyYaw(_previousRecord->m_flLowerBodyYaw);

		for (int i = 0; i < _TotalNewCommands; i++)
		{
			//Increment curtime for each new command to get the exact simulation time for that tick
			curtime += TICK_INTERVAL;

			_netOrigin += _netOriginSlope;
			_AbsOrigin += _absOriginSlope;
			_DuckAmount += _duckAmountSlope;
			_DuckSpeed += _duckSpeedSlope;
			_Mins += _minsSlope;
			_Maxs += _maxsSlope;
			_localVelocity += _localVeloSlope;
			_absVelocity += _absVeloSlope;
			_baseVelocity += _baseVeloSlope;
			_lerpedEyeAngles += _eyeAngleSlope;
			NormalizeAngles(_lerpedEyeAngles);

			m_pEntity->SetNetworkOrigin(_netOrigin);
			m_pEntity->SetLocalOriginDirect(_netOrigin);
			m_pEntity->SetAbsOriginDirect(_AbsOrigin);
			m_pEntity->RemoveEFlag(EFL_DIRTY_ABSTRANSFORM);
			//if (!_currentRecord->m_bTickbaseShiftedBackwards)
			//{
			//	//Fix duck amount, by raxer
			//	float _frac = 1.0f - (_currentRecord->m_SimulationTime - curtime) / _TotalCommandTime;
			//	m_pEntity->SetDuckAmount(interpolate(_prevduckamount, _currentRecord->m_DuckAmount, _frac));
			//}
			//else
			{
				m_pEntity->SetDuckAmount(_DuckAmount);
			}
			m_pEntity->SetDuckSpeed(_DuckSpeed);
			m_pEntity->SetMins(_Mins);
			m_pEntity->SetMaxs(_Maxs);
			if (!_currentRecord->m_bIsUsingMovingLBYMeme)
			{
				m_pEntity->SetVelocity(_localVelocity);
				m_pEntity->SetAbsVelocityDirect(_absVelocity);
			}
			else
			{
				Vector _vecMovingVelocity = { 1.01f, 0.0f, 0.0f };
				if (m_bMovingLBYMemeFlipState[mode])
					_vecMovingVelocity.x = -_vecMovingVelocity.x;
				m_bMovingLBYMemeFlipState[mode] = !m_bMovingLBYMemeFlipState[mode];
				m_pEntity->SetVelocity(_vecMovingVelocity);
				m_pEntity->SetAbsVelocityDirect(_vecMovingVelocity);
			}
			m_pEntity->RemoveEFlag(EFL_DIRTY_ABSVELOCITY);
			m_pEntity->SetBaseVelocity(_baseVelocity);
			if (m_pEntity->FindGroundEntity())
				m_pEntity->AddFlag(FL_ONGROUND);
			else
				m_pEntity->RemoveFlag(FL_ONGROUND);

			//FIXME: handle when lby doesnt update (eg, the flick)
			Vector absolute_velocity = m_pEntity->GetAbsVelocityDirect();
			tm_velocity = g_LagCompensation.GetSmoothedVelocity(fmaxf(0.0f, curtime - m_pAnimStateServer[mode]->m_flLastClientSideAnimationUpdateTime) * 2000.0f, Vector(absolute_velocity.x, absolute_velocity.y, 0.0f), tm_velocity);
			float animation_speed = fminf(tm_velocity.Length(), 260.0f);
			float lowerbodyyaw_target = m_pEntity->GetLowerBodyYaw();
			bool lby_timer_updated = false;
			bool is_moving = false;

#ifdef SHIT_FIX_FOR_ON_SHOT
			if (_currentRecord->m_bFiredBullet)
			{
				if (AngleDiff(_currentRecord->m_EyeAngles.y, animstate->m_flGoalFeetYaw) < 0.0f)
				{
					_desyncAbsYawAmount = fabsf(_desyncAbsYawAmount);
				}
				else
				{
					_desyncAbsYawAmount = -fabsf(_desyncAbsYawAmount);
				}
			}
#endif


			if (animation_speed > 0.1f || fabsf(absolute_velocity.z) > 100.0f)
			{
				if (i == _FinalCommandIndex || curtime >= _ShotTime)
					lowerbodyyaw_target = _currentRecord->m_flLowerBodyYaw;
				else
				{
					if (Resolve)
						lowerbodyyaw_target = _chokedLBY;
					else
						lowerbodyyaw_target = _lerpedEyeAngles.y;
				}

				tm_next_lby_update_time = curtime + 0.22f;
				lby_timer_updated = true;
				is_moving = true;
			}
			else if (curtime > tm_next_lby_update_time)
			{
				tm_next_lby_update_time = curtime + 1.1f;
				lby_timer_updated = true;
				lowerbodyyaw_target = _currentRecord->m_flLowerBodyYaw;
			}

			if (_restoreInjury && curtime >= _timeOfLastInjury)
			{
				m_pEntity->SetLastHitgroup(_lastHitgroup);
				m_pEntity->SetRelativeDirectionOfLastInjury(_relativeDirectionOfLastInjury);
				m_pEntity->SetFireCount(_fireCount);
				m_pEntity->SetTimeOfLastInjury(_timeOfLastInjury);
			}

			if (i == _FinalCommandIndex)
			{
				if (_currentRecord->m_Flags & FL_ONGROUND)
					m_pEntity->AddFlag(FL_ONGROUND);
				else
					m_pEntity->RemoveFlag(FL_ONGROUND);
			}

			if (i == _FinalCommandIndex)
				m_pEntity->SetMoveState(_currentMoveState);

		
			// on the cmd they shot and after, use the shot angles
			if (curtime >= _ShotTime)
			{
				//On the first tick since shot time, set their desync to the previous yaw's desync amount
				if (Resolve)
				{
					//If they also shot on the previous tick, or the previous tick was not choked, then don't modify the goalfeetyaw for the first tick
					if (fabsf(curtime - _ShotTime) < TICK_INTERVAL)
						animstate->m_flGoalFeetYaw = AngleNormalizePositive(m_angEyeAnglesNotFiring.y + _desyncAbsYawAmount);
				}

				*m_pEntity->EyeAngles() = NormalizeAngles_r(_currentRecord->m_EyeAngles);
			}
			else if (i != _FinalCommandIndex)
			{
				//If they shot, set their desync to their previous yaw's desync amount
				if (Resolve)
				{
					if (!_currentRecord->m_bFiredBullet)
					{
						if (m_flTimeSinceStartedPressingUseKey == 0.0f)
							animstate->m_flGoalFeetYaw = AngleNormalizePositive(_currentRecord->m_EyeAngles.y + _desyncAbsYawAmount);
						*m_pEntity->EyeAngles() = _currentRecord->m_EyeAngles;
					}
					else
					{
						animstate->m_flGoalFeetYaw = AngleNormalizePositive(m_angEyeAnglesNotFiring.y + _desyncAbsYawAmount);
						*m_pEntity->EyeAngles() = _previousRecord->m_EyeAngles;
						m_pEntity->EyeAngles()->y = m_angEyeAnglesNotFiring.y;
					}
				}
				else
				{
					*m_pEntity->EyeAngles() = _lerpedEyeAngles;
				}
			}
			else
			{
				//Final tick and not firing
				if (Resolve && m_flTimeSinceStartedPressingUseKey == 0.0f)
					animstate->m_flGoalFeetYaw = AngleNormalizePositive(_currentRecord->m_EyeAngles.y + _desyncAbsYawAmount);

				*m_pEntity->EyeAngles() = _lerpedEyeAngles;
			}

			//Store the max desync delta for the final command in case we use it later
			if (i == _FinalCommandIndex && _currentRecord->m_flAbsMaxDesyncDelta == 0.0f)
			{
				m_pEntity->GetMaxDesyncDelta(_currentRecord);

				CBaseCombatWeapon *weapon = m_pEntity->GetActiveCSWeapon();

				float flMaxMovementSpeed = 260.0f;
				if (weapon)
					flMaxMovementSpeed = std::fmax(weapon->GetMaxSpeed3(), 0.001f);

				float m_flRunningSpeed = animation_speed / (flMaxMovementSpeed * 0.520f);
				float m_flDuckingSpeed = animation_speed / (flMaxMovementSpeed * 0.340f);

				float flRunningSpeed = clamp(m_flRunningSpeed, 0.0f, 1.0f);

				float flYawModifier = (((m_pAnimStateServer[mode]->m_flGroundFraction * -0.3f) - 0.2f) * flRunningSpeed) + 1.0f;

				float flNewDuckAmount = clamp(m_pEntity->GetDuckAmount() + m_pAnimStateServer[mode]->m_flHitGroundCycle, 0.0f, 1.0f);
				float time = fmaxf(curtime - m_pAnimStateServer[mode]->m_flLastClientSideAnimationUpdateTime, 0.0f);
				flNewDuckAmount = Approach(flNewDuckAmount, m_pAnimStateServer[mode]->m_fDuckAmount, time * 6.0f);
				flNewDuckAmount = clamp(flNewDuckAmount, 0.0f, 1.0f);

				if (flNewDuckAmount > 0.0f)
				{
					float flDuckingSpeed = clamp(m_flDuckingSpeed, 0.0f, 1.0f);
					flYawModifier = flYawModifier + ((flNewDuckAmount * flDuckingSpeed) * (0.5f - flYawModifier));
				}

				_currentRecord->m_flAbsMaxDesyncDelta = flYawModifier * m_pAnimStateServer[mode]->m_flMaxYaw;
			}
			
			m_pEntity->UpdateServerSideAnimation(mode, curtime, _RunOnlySetupVelocitySetupLean);

			//lby isn't set until after it's used in the server's SetupVelocity
			m_pEntity->SetLowerBodyYaw(lowerbodyyaw_target);
			animstate->m_flNextLowerBodyYawUpdateTime = tm_next_lby_update_time;
		}

#ifdef SHIT_FIX_FOR_ON_SHOT
		if (_previousRecord->m_bFiredBullet && _previousRecord->m_iTicksChoked > 0)
		{
			float testyaw = AngleNormalizePositive(_currentRecord->m_EyeAngles.y + _desyncAbsYawAmount);
			m_pEntity->SetAbsAngles(QAngle(0.0f, testyaw, 0.0f));
			animstate->m_flGoalFeetYaw = testyaw;

			float eye_goalfeet_delta = AngleDiff(_currentRecord->m_EyeAngles.y, testyaw);

			float new_body_yaw_pose = 0.0f; //not initialized?

			if (eye_goalfeet_delta < 0.0f || animstate->m_flMaxYaw == 0.0f)
			{
				if (animstate->m_flMinYaw != 0.0f)
					new_body_yaw_pose = (eye_goalfeet_delta / animstate->m_flMinYaw) * -58.0f;
			}
			else
			{
				new_body_yaw_pose = (eye_goalfeet_delta / animstate->m_flMaxYaw) * 58.0f;
			}

			animstate->m_arrPoseParameters[6].SetValue(m_pEntity, new_body_yaw_pose);
		}
#endif

		animstate->m_arrPoseParameters[9].SetValue(animstate->pBaseEntity, 1.0f - _currentRecord->m_DuckAmount);
		m_pEntity->SetNetworkOrigin(_currentRecord->m_NetOrigin);
		m_pEntity->SetLocalOriginDirect(_currentRecord->m_NetOrigin);
		m_pEntity->SetAbsOriginDirect(_currentRecord->m_AbsOrigin);
		m_pEntity->RemoveEFlag(EFL_DIRTY_ABSTRANSFORM);
		m_pEntity->SetDuckAmount(_currentRecord->m_DuckAmount);
		m_pEntity->SetDuckSpeed(_currentRecord->m_DuckSpeed);
		m_pEntity->SetMins(_currentRecord->m_Mins);
		m_pEntity->SetMaxs(_currentRecord->m_Maxs);
		m_pEntity->SetVelocity(_currentRecord->m_Velocity);
		m_pEntity->SetAbsVelocityDirect(_currentRecord->m_AbsVelocity);
		m_pEntity->RemoveEFlag(EFL_DIRTY_ABSVELOCITY);
		m_pEntity->SetBaseVelocity(_currentRecord->m_BaseVelocity);
		m_pEntity->SetFlags(_currentRecord->m_Flags);
		*m_pEntity->EyeAngles() = _currentRecord->m_EyeAngles;
		m_pEntity->SetEyeAngles(_currentRecord->m_EyeAngles);
		m_pEntity->SetPoseParameter(12, clamp(AngleNormalize(_currentRecord->m_EyeAngles.x), -90.0f, 90.0f));
		m_pEntity->SetLowerBodyYaw(_backupLBY);

		if (_restoreInjury)
		{
			m_pEntity->SetLastHitgroup(_lastHitgroup);
			m_pEntity->SetRelativeDirectionOfLastInjury(_relativeDirectionOfLastInjury);
			m_pEntity->SetFireCount(_fireCount);
			m_pEntity->SetTimeOfLastInjury(_timeOfLastInjury);
		}

		//restore server animations
		if (_RestoreServerAnimLayers)
		{
			m_pEntity->WriteAnimLayers(DataUpdateVars.m_PostAnimLayers);

			//force the feet to the current server value after finishing animating
			animstate->m_flFeetCycle = DataUpdateVars.m_flPostFeetCycle;
			animstate->m_flFeetWeight = DataUpdateVars.m_flPostFeetWeight;
		}

		// restore times
		//Interfaces::Globals->curtime = _backupCurTime;
		//Interfaces::Globals->frametime = _backupFrameTime;
		m_pEntity->SetLowerBodyYaw(_backupLBY);
	}
	else
	{
		//Not choking, respawned, or not enough records

		*m_pEntity->EyeAngles() = NormalizeAngles_r(m_angPristineEyeAngles);

		//float _backupTime = Interfaces::Globals->curtime;
		//float _backupFTime = Interfaces::Globals->frametime;
		float _backupLBY = m_pEntity->GetLowerBodyYaw();
		float curtime = m_flFirstCmdTickbaseTime;
		//Interfaces::Globals->curtime = curtime; //m_pEntity->GetSimulationTime();
		//Interfaces::Globals->frametime = Interfaces::Globals->interval_per_tick;

		if (oSimulate) oSimulate(m_pEntity);

		m_pEntity->RemoveEFlag(EFL_DIRTY_ABSTRANSFORM);
		m_pEntity->RemoveEFlag(EFL_DIRTY_ABSVELOCITY);

		// fix time delta in case the enemy came out of dormancy
		if (!_previousRecord || _previousRecord->m_Dormant || m_pEntity->GetSpawnTime() != m_flSpawnTime)
		{
			animstate->m_vVelocity = vecZero;
			m_pEntity->SetAbsVelocityDirect(vecZero);
			animstate->m_flLastClientSideAnimationUpdateTime = m_flFirstCmdTickbaseTime - TICK_INTERVAL;
		}
		else
		{
			//restore original LBY
			m_pEntity->SetLowerBodyYaw(_previousRecord->m_flLowerBodyYaw);
			m_pEntity->WriteAnimLayers(_previousRecord->m_AnimLayer);
		}

		m_pEntity->UpdateServerSideAnimation(mode, curtime);

		m_pEntity->SetLowerBodyYaw(_backupLBY);

		//Interfaces::Globals->curtime = _backupTime;
		//Interfaces::Globals->frametime = _backupFTime;

		if (_RestoreServerAnimLayers)
		{
			m_pEntity->WriteAnimLayers(DataUpdateVars.m_PostAnimLayers);
			animstate->m_flFeetCycle = DataUpdateVars.m_flPostFeetCycle;
			animstate->m_flFeetWeight = DataUpdateVars.m_flPostFeetWeight;
		}

		*m_pEntity->EyeAngles() = _currentRecord->m_EyeAngles;
		m_pEntity->SetEyeAngles(_currentRecord->m_EyeAngles);
		m_pEntity->SetPoseParameter(12, clamp(AngleNormalize(_currentRecord->m_EyeAngles.x), -90.0f, 90.0f));
	}
}

unsigned CPlayerrecord::FindDesyncDirection(CTickrecord* _previousRecord, CTickrecord* _currentRecord, CTickrecord *_prePreviousRecord, bool* pDidFind)
{
	const auto float_matches = [](const float float1, const float float2, float tolerance = 0.002f) { return fabsf(float1 - float2) < tolerance; };

	unsigned result = DESYNC_RESULTS_NEGATIVE;
	auto& Layer3 = DataUpdateVars.m_PostAnimLayers[3];
	auto* LastLayer3 = _previousRecord ? &_previousRecord->m_AnimLayer[3] : nullptr;

	if (!LastLayer3)
		return m_DesyncResults;

	bool overwritten = false;
	bool found = false;
	bool positive = false;
	bool bruteforce = false;

	/*
	if (Layer3.m_flWeight == 0.f && Layer3._m_flCycle == 0.f)
	{
		//_goalFeetYaw = _currentRecord->m_EyeAngles.y + 90.f; //right
		found = true;
		positive = true;
	}
	*/
	if (float_matches(Layer3.m_flWeightDeltaRate, 10.04290f)
		&& float_matches(LastLayer3->m_flWeightDeltaRate, 2.6813f)
		&& fabsf(
			AngleNormalize(_currentRecord->m_flLowerBodyYaw - _currentRecord->m_EyeAngles.y)
		) > 55.f)
	{
		//_goalFeetYaw = _currentRecord->m_EyeAngles.y - 90.f; //left
		found = true;
	}
	if (_prePreviousRecord
		&& float_matches(Layer3.m_flWeightDeltaRate, 2.6813f)
		&& float_matches(LastLayer3->m_flWeightDeltaRate, 2.6813f)
		&& float_matches(_prePreviousRecord->m_AnimLayer[3].m_flWeightDeltaRate, 2.6813f)
		&& fabsf(
			AngleNormalize(_currentRecord->m_flLowerBodyYaw - _currentRecord->m_EyeAngles.y)
			-
			AngleNormalize(_previousRecord->m_flLowerBodyYaw - _previousRecord->m_EyeAngles.y)
		) < 50.f)
	{
		//_goalFeetYaw = _currentRecord->m_EyeAngles.y - 90.f; //left
		found = true;
	}

#if 1
	if (!found)
	{
		if (Layer3.m_flWeightDeltaRate == 0.f && LastLayer3->m_flWeightDeltaRate == 0.f
			&& Layer3.m_flWeight == 1.f && LastLayer3->m_flWeight == 1.f
			&& GetSequenceActivity(m_pEntity, Layer3._m_nSequence) == ACT_CSGO_IDLE_TURN_BALANCEADJUST
			&& GetSequenceActivity(m_pEntity, LastLayer3->_m_nSequence) == ACT_CSGO_IDLE_TURN_BALANCEADJUST
			)
		{
			float dt = AngleNormalize(AngleDiff(m_pEntity->GetLowerBodyYaw(), m_angPristineEyeAngles.y));
			if (fabsf(dt) > 35)
			{
				//_goalFeetYaw = _currentRecord->m_EyeAngles.y - 90.f; //left
				found = true;
			}
		}
	}

	if (!found)
	{
		if ((float_matches(Layer3.m_flWeightDeltaRate, 2.6813f) && float_matches(LastLayer3->m_flWeightDeltaRate, 2.6813f))
			|| (float_matches(Layer3.m_flWeightDeltaRate, 10.2735481f, 0.05f) && float_matches(LastLayer3->m_flWeightDeltaRate, 10.2735481f, 0.05f))
			|| ((Layer3.m_flWeight > 7.f && LastLayer3->m_flWeight < 8.f)
				&&
				(float_matches(Layer3.m_flWeightDeltaRate, 7.46731186f, 0.9f) && float_matches(LastLayer3->m_flWeightDeltaRate, 7.46731186f, 0.9f)))
			)
		{
			float diff = AngleDiff(_currentRecord->m_flLowerBodyYaw, _currentRecord->m_EyeAngles.y);
			if (diff < -35.f)
				found = true;
		}
	}

	if (!found)
	{
		//choking 2 ticks with 180 yaw and no jitter:   weight 0.152941 weightdeltarate 7.08289
		if (float_matches(Layer3.m_flWeight, LastLayer3->m_flWeight)
			&& float_matches(Layer3.m_flWeightDeltaRate, LastLayer3->m_flWeightDeltaRate)
			&& Layer3.m_flWeight < 0.2f
			&& Layer3.m_flWeightDeltaRate > 7.f)
		{
			float diff = AngleDiff(_currentRecord->m_flLowerBodyYaw, _currentRecord->m_EyeAngles.y);
			if (fabsf(diff) > 35.f) //delta was positive 66
				found = true;
		}
	}

	if (!found)
	{
		//skeet's 'lower body yaw' checkbox checked
		auto lby_matches_eye = float_matches(_currentRecord->m_flLowerBodyYaw, AngleNormalize(_currentRecord->m_EyeAngles.y), 0.2f);
		auto delta = AngleNormalize(AngleDiff(_currentRecord->m_flLowerBodyYaw, AngleNormalize(_currentRecord->m_EyeAngles.y)));
		auto lby_delta_big = fabsf(delta) > 50.0f;
		Activities newact = GetSequenceActivity(m_pEntity, Layer3._m_nSequence);
		Activities prevact = GetSequenceActivity(m_pEntity, LastLayer3->_m_nSequence);

		int tickschoked = m_iTicksChoked;

		if ((lby_matches_eye || lby_delta_big)
			&& ((newact == ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING && prevact == ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING)
				|| (newact == ACT_CSGO_IDLE_TURN_BALANCEADJUST && prevact == ACT_CSGO_IDLE_TURN_BALANCEADJUST)
				)
			)
		{
			//skeet is weird, if choked amount is <= 2 then the fake is almost the real

			//FIXME: set these are identical
			if (Layer3.m_flWeightDeltaRate == 0.0f && LastLayer3->m_flWeightDeltaRate == 0.0f
				&& m_iTicksChoked == 1)
			{
				bruteforce = true;
				found = true;
			}
			else if (Layer3.m_flWeightDeltaRate <= 0.0f && LastLayer3->m_flWeightDeltaRate <= 0.0f
				|| (float_matches(Layer3.m_flWeightDeltaRate, 3.21949768f) && float_matches(LastLayer3->m_flWeightDeltaRate, 3.21949768f))
				)
			{
				if (m_bIsConsistentlyBalanceAdjusting && m_iTicksChoked == 3)
				{
					//FIXME: this isn't quite correct
					//how to correct this case: every lby update, keep the goalfeet the same, don't allow the legs to keep shifting
					//_chokedYaw = _currentRecord->m_EyeAngles.y;
					//_goalFeetYaw = m_goalfeetyaw;
					//_Resolve = false;
					//overwritten = true;
					bruteforce = true;
					found = true;
				}
				else
				{
					if (m_iTicksChoked > 2)
					{
						//skeet always flips eye yaw 0.5f secs after lby update in this mode
						if (m_flLastLBYUpdateTime + 0.5f >= GetServerTime())
						{
							positive = delta >= 0.0f;

							if (m_bJitteringYaw)
								positive = !positive;
						}
						else
						{
							positive = delta < 0.0f;

							if (m_bJitteringYaw)
								positive = !positive;
						}

						if (!m_bJitteringYaw)
						{
							if (Layer3.m_flWeight == 1.f && LastLayer3->m_flWeight == 1.f
								&& Layer3.m_flWeightDeltaRate == 0.f && LastLayer3->m_flWeightDeltaRate == 0.0f)
							{
								if (delta < -100.f)
								{
									//spin
									positive = false;
									found = true;
								}
								else
								{
									//using skeet body jitter
								}
							}
							/*
							else if (Layer3.m_flWeight == 0.f && LastLayer3->m_flWeight == 0.f
								&& Layer3.m_flWeightDeltaRate == 0.f && LastLayer3->m_flWeightDeltaRate == 0.0f)
							{
								positive = false;
								found = true;
							}
							*/
							else if (m_iTicksChoked >= 5)
							{
								if (Layer3.m_flWeight > 0.f && Layer3.m_flWeightDeltaRate < 0.f)
								{
									bool lastwasmatch = false;
									int matches = 0;
									for (int i = 0; i < min((int)m_Tickrecords.size(), 14) && matches < 2; ++i)
									{
										if (i + 1 > m_Tickrecords.size() - 1)
											break;
										CTickrecord *tick = m_Tickrecords[i];
										CTickrecord *lasttick = m_Tickrecords[i + 1];
										if (tick->m_AnimLayer[3].m_flWeight == 1.f && lasttick->m_AnimLayer[3].m_flWeight == 1.f
											&& tick->m_AnimLayer[3].m_flWeightDeltaRate == 0.f && lasttick->m_AnimLayer[3].m_flWeightDeltaRate == 0.0f)
										{
											++matches;
										}
									}
									if (matches > 1)
									{
										//using body jitter

										//FIXME: this is not perfect but better than nothing
										//overwritten = true;
										//_animstate->m_flGoalFeetYaw = AngleNormalizePositive(_currentRecord->m_EyeAngles.y + AngleDiff(_currentRecord->m_EyeAngles.y, _currentRecord->m_flLowerBodyYaw));
										//_chokedYaw = _animstate->m_flGoalFeetYaw;
										//_goalFeetYaw = _chokedYaw;
										//found = true;
										//bruteforce = true;
									}
								}
							}
						}
					}
					else
					{
						//FIXME: this isn't quite correct
						//_chokedYaw = _currentRecord->m_EyeAngles.y;//GetChokedYawFromYaw(_currentRecord->m_EyeAngles.y, _currentRecord->m_EyeAngles.x, &_goalFeetYaw, positive ? 20.0f : -20.0f);
						//_goalFeetYaw = m_goalfeetyaw;
						//overwritten = true;
						//found = true;
					}
				}
			}
		}
	}
#endif

	if (pDidFind)
		*pDidFind = found;

	if (!found || bruteforce)
	{
#if 1
		//Just return existing results
		return m_DesyncResults;
#else
		//TODO: FIX ME
		float angle_diff;
		bool is_not_flicking_yaw = true;

		for (size_t i = 0; i < min(m_Tickrecords.size(), 12); i++)
		{
			// get current record
			CTickrecord* _currentRecord1 = m_Tickrecords[i];

			if (_currentRecord1->m_bFiredBullet)
				continue;

			// sanity check
			if (i < m_Tickrecords.size() - 1)
			{
				// get record previous to current one
				CTickrecord* _previousRecord1 = m_Tickrecords[i + 1];

				if (_previousRecord1->m_bFiredBullet)
					continue;

				if (fabsf(AngleDiff(_currentRecord1->m_EyeAngles.y, _previousRecord1->m_EyeAngles.y)) > 15.0f)
				{
					is_not_flicking_yaw = false;
					break;
				}
			}

			if (!is_not_flicking_yaw)
				break;
		}
		if (m_flLastLBYUpdateTime != GetServerTime()
			&& _currentRecord->m_flFirstCmdTickbaseTime + TICKS_TO_TIME(m_iTicksChoked) <= m_next_lby_update_time
			&& is_not_flicking_yaw
			&& !_currentRecord->m_bFiredBullet
			&& (angle_diff = AngleDiff(m_pInflictedEntity->GetEyeAngles().y, fakeabsangles_delayed.y), fabsf(angle_diff) > 35.0f))
		{
			positive = angle_diff > 0.0f;
			/*
			bool _oldPositive = m_DesyncResults & DESYNC_RESULTS_POSITIVE;
			if (m_DesyncResults & DESYNC_RESULTS_FOUND && _oldPositive != positive)
			{
				float _DesyncTestAngle = AngleNormalize(_currentRecord->m_EyeAngles.y + positive ? 120.0f : -120.0f);
				PlayerBackup_t *backup = new PlayerBackup_t(m_pEntity);
				FSN_AnimateTicks(_previousRecord, _currentRecord, _DesyncTestAngle, true, true);
				float angle_diff2 = AngleDiff(m_pEntity->GetAbsAnglesDirect().y, fakeabsangles.y);
				if (fabsf(angle_diff2) < 15.0f)
				{
					positive = _oldPositive;
				}
				backup->RestoreData();
				delete backup;
			}
			*/

			found = true;
		}
		else
		{
			//Just return existing results
			return m_DesyncResults;
		}
#endif
	}

	if (found)
		result |= DESYNC_RESULTS_FOUND;
	if (positive)
	{
		result &= ~DESYNC_RESULTS_NEGATIVE;
		result |= DESYNC_RESULTS_POSITIVE;
	}
	//if (overwritten)
	//	result |= DESYNC_RESULTS_OVERWRITTEN;
	
	return result;
}

// returns which side the yaw is on
ResolveSides CPlayerrecord::GetSideFromYaw(float _Yaw)
{
	if (_Yaw == 0.0f)
		return ResolveSides::NONE;
	if (_Yaw < 0.0f)
	{
		if (_Yaw == -120.0f)
			return ResolveSides::NEGATIVE_60;

		return ResolveSides::NEGATIVE_35;
	}
	if (_Yaw == 120.0f)
		return ResolveSides::POSITIVE_60;
	return ResolveSides::POSITIVE_35;
};

// returns which yaw a side represents
float CPlayerrecord::GetYawFromSide(ResolveSides _Side)
{
	switch (_Side)
	{
	case ResolveSides::NONE:
		return 0.0f;
	case ResolveSides::NEGATIVE_60:
		return -120.0f;
	case ResolveSides::POSITIVE_60:
		return 120.0f;
	case ResolveSides::NEGATIVE_35:
		return -90.0f;
	case ResolveSides::POSITIVE_35:
		return 90.0f;
	}
	return -1.0f;
};

bool CPlayerrecord::GetBodyHitDesyncAmount(float* _desyncAmount, float* _desyncEyeYawAmount, CTickrecord* _currentRecord, CTickrecord* _previousRecord, CTickrecord* _prePreviousRecord, s_Impact_Info* BodyResolveInfo)
{
	bool _ResolverEnabled = variable::get().ragebot.b_resolver && !m_bIsBot;

	if (BodyResolveInfo && _ResolverEnabled && _currentRecord->m_iTicksChoked > 0)
	{
		*_desyncAmount = BodyResolveInfo->m_flDesyncDelta;
		*_desyncEyeYawAmount = clamp(BodyResolveInfo->m_flDesyncDelta * 2.0f, -120.0f, 120.0f);

		if (fabsf(*_desyncAmount) < 1.0f)
		{
			//not desyncing
			*_desyncAmount = 0.0f;
			*_desyncEyeYawAmount = 0.0f;
			return true;
		}

		if (BodyResolveInfo->IsNegative() && (m_iResolveSide == ResolveSides::POSITIVE_35 || m_iResolveSide == ResolveSides::POSITIVE_60))
		{
			*_desyncAmount = fabsf(*_desyncAmount);
			*_desyncEyeYawAmount = fabsf(*_desyncEyeYawAmount);
		}

		//Clamp desync amount
		float _maxDelta = m_pEntity->GetMaxDesyncDelta();
		*_desyncAmount = clamp(*_desyncAmount, -_maxDelta, _maxDelta);

		return true;
	}

	*_desyncAmount = 0.0f;
	* _desyncEyeYawAmount = 0.0f;
	return false;
}

void CPlayerrecord::SetResolveSide(int side, const char* reason)
{
	if (m_iResolveSide != side)
	{
		m_iResolveSide = (ResolveSides)side;
#if defined DEBUG_BH || defined _DEBUG || defined INTERNAL_DEBUG
		std::string victim_name = adr_util::sanitize_name((char*)m_pEntity->GetName().data());
		std::string ev = std::string(reason) + " switched " + victim_name + "'s resolveside to " + g_Visuals.GetResolveSide(side);
		g_Eventlog.AddEventElement(0x6, ev);
		g_Eventlog.OutputEvent(variable::get().visuals.i_shot_logs);
#endif
	}
}

void CPlayerrecord::ApplyBodyHitResolver(s_Impact_Info* BodyResolveInfo, float _desyncAmount, CCSGOPlayerAnimState* animstate, CTickrecord* _currentRecord, CTickrecord* _previousRecord)
{
	if (!_previousRecord)
		return;

	float EyeYaw = AngleNormalize(_currentRecord->m_EyeAngles.y);
	float flDuckSpeedClamp, flRunningSpeedClamp, flLerped, flSpeed, flMaxYaw, flMinYaw, flLadderCycle;
	bool bOnLadder;
	Vector &vVelocity = animstate->m_vVelocity;
	float velAngle = (atan2(-vVelocity.y, -vVelocity.x) * 180.0f) * (1.0f / M_PI);
	if (velAngle < 0.0f)
		velAngle += 360.0f;
	CTickrecord *oldtickrecord = _previousRecord;

	flDuckSpeedClamp = clamp(animstate->m_flDuckingSpeed, 0.0f, 1.0f);
	flRunningSpeedClamp = clamp(animstate->m_flRunningSpeed, 0.0f, 1.0f);
	flLerped = ((flDuckSpeedClamp - flRunningSpeedClamp) * animstate->m_fDuckAmount) + flRunningSpeedClamp;
	flSpeed = animstate->m_flSpeed;
	flMaxYaw = animstate->m_flMaxYaw;
	flMinYaw = animstate->m_flMinYaw;
	flLadderCycle = animstate->m_flLadderCycle;
	bOnLadder = animstate->m_bOnLadder;
	
	float flBiasMove = Bias(flLerped, 0.18f);
	float m_flCurrentMoveDirGoalFeetDelta, m_flGoalMoveDirGoalFeetDelta;
	m_flCurrentMoveDirGoalFeetDelta = oldtickrecord->m_pAnimStateServer[ResolveSides::NONE]->m_flCurrentMoveDirGoalFeetDelta;
	m_flGoalMoveDirGoalFeetDelta = oldtickrecord->m_pAnimStateServer[ResolveSides::NONE]->m_flGoalMoveDirGoalFeetDelta;

	bool bStrafing = _currentRecord->m_bStrafing;
	bool bRunLadderPose = bOnLadder || flLadderCycle > 0.0f;
	bool bMoving = flSpeed > 0.0f;
	QAngle angles;
	const Vector pseudoup = { 0.0f, 0.0f, 1.0f };
	Vector LadderNormal = _currentRecord->m_vecLadderNormal;
	VectorAngles(animstate->m_vecSetupLeanVelocityInterpolated, pseudoup, angles);
	//float BestAbsYaw;
	//int HitgroupHit;

	float DesyncDelta = AngleNormalize(_desyncAmount);
	float ResolvedGoalFeetYaw = AngleNormalizePositive(EyeYaw + DesyncDelta);
	float Delta = AngleDiff(EyeYaw, BodyResolveInfo->m_NetworkedEyeYaw);
	float LBYDelta = AngleDiff(_currentRecord->m_flLowerBodyYaw, BodyResolveInfo->m_NetworkedLBY);
	//if (fabsf(Delta) > 118.0f)
	//{
	//	ResolvedGoalFeetYaw = AngleNormalizePositive(EyeYaw - BodyResolveInfo->m_flDesyncDelta);
	//}

	float LBYEyeDelta = AngleDiff(_currentRecord->m_flLowerBodyYaw, EyeYaw);
	bool CurrentDeltaPositive = LBYEyeDelta > 0.0f;
	float LBYResolvedEyeDelta = AngleDiff(BodyResolveInfo->m_NetworkedLBY, BodyResolveInfo->m_NetworkedEyeYaw);
	bool OldDeltaPositive = LBYResolvedEyeDelta > 0.0f;

	//if (Impact.m_iBodyHitMisses % 2 != 0)
	//{
	//	ResolvedGoalFeetYaw = AngleNormalizePositive(EyeYaw - BodyResolveInfo->m_flDesyncDelta);
	//}

	if (bMoving)
		m_flGoalMoveDirGoalFeetDelta = AngleNormalize(AngleDiff(velAngle, ResolvedGoalFeetYaw));

	float m_flFeetVelDirDelta = AngleNormalize(AngleDiff(m_flGoalMoveDirGoalFeetDelta, m_flCurrentMoveDirGoalFeetDelta));
	m_flCurrentMoveDirGoalFeetDelta = _currentRecord->m_AnimLayer[7].m_flWeight >= 1.0f ? m_flGoalMoveDirGoalFeetDelta : AngleNormalize(((flBiasMove + 0.1f) * m_flFeetVelDirDelta) + m_flCurrentMoveDirGoalFeetDelta);
	m_pEntity->SetPoseParameter(7, m_flCurrentMoveDirGoalFeetDelta); //move_yaw

	float new_body_yaw_pose = 0.0f; //not initialized?
	float eye_goalfeet_delta = AngleDiff(EyeYaw, ResolvedGoalFeetYaw);
	if (eye_goalfeet_delta < 0.0f || flMaxYaw == 0.0f)
	{
		if (flMinYaw != 0.0f)
			new_body_yaw_pose = (eye_goalfeet_delta / flMinYaw) * -58.0f;
	}
	else
	{
		new_body_yaw_pose = (eye_goalfeet_delta / flMaxYaw) * 58.0f;
	}

	QAngle absangles = QAngle(0.0f, ResolvedGoalFeetYaw, 0.0f);

	if (bRunLadderPose)
	{
		QAngle ladderAngles;
		VectorAngles(LadderNormal, ladderAngles);
		m_pEntity->SetPoseParameter(4, AngleDiff(ladderAngles.y, absangles.y));
	}

	m_pEntity->SetPoseParameter(2, AngleNormalize(absangles.y - angles.y)); //lean_yaw

	m_pEntity->SetAbsAngles(absangles); //fix the abs angles to the goalfeetyaw
	m_pEntity->SetPoseParameter(11, new_body_yaw_pose); //fix the body yaw pose parameter
}

std::string CPlayerrecord::GetBaimReasonString()
{
	std::string ret = {};

	switch (m_iBaimReason)
	{
		case BAIM_REASON_HEAD_FIRED_BULLET:
		{
			//decrypts(0)
			ret = XorStr("FIRED BULLET");
			//encrypts(0)
			break;
		}
		case BAIM_REASON_HEAD_RESOLVED:
		{
			//decrypts(0)
			ret = XorStr("HEAD RESOLVED");
			//encrypts(0)
			break;
		}
		case BAIM_REASON_NONE:
		{
			//decrypts(0)
			ret = XorStr("HEAD");
			//encrypts(0)
			break;
		}
		case BAIM_REASON_FORCE:
		{
			//decrypts(0)
			ret = XorStr("FORCE BODY");
			//encrypts(0)
			break;
		}
		case BAIM_REASON_LETHAL:
		{
			//decrypts(0)
			ret = XorStr("LETHAL");
			//encrypts(0)
			break;
		}
		case BAIM_REASON_AIRBORNE_TARGET:
		{
			//decrypts(0)
			ret = XorStr("AIRBORNE");
			//encrypts(0)
			break;
		}
		case BAIM_REASON_AIRBORNE_LOCAL:
		{
			//decrypts(0)
			ret = XorStr("LOCAL AIRBORNE");
			//encrypts(0)
			break;
		}
		case BAIM_REASON_MOVING_TARGET:
		{
			//decrypts(0)
			ret = XorStr("MOVING");
			//encrypts(0)
			break;
		}
		case BAIM_REASON_FORWARD_TICKBASE:
		{
			//decrypts(0)
			ret = XorStr("FWD TICKBASE");
			//encrypts(0)
			break;
		}
		case BAIM_REASON_CANT_RESOLVE:
		{
			//decrypts(0)
			ret = XorStr("CANT RESOLVE");
			//encrypts(0)
			break;
		}
		case BAIM_REASON_NOT_BODY_HIT_RESOLVED:
		{
			//decrypts(0)
			ret = XorStr("NOT BH");
			//encrypts(0)
			break;
		}
		case BAIM_REASON_TOO_MANY_MISSES:
		{
			//decrypts(0)
			ret = XorStr("TOO MANY MISSES");
			//encrypts(0)
			break;
		}
		case BAIM_REASON_ZERO_TICKS:
		{
			//decrypts(0)
			ret = XorStr("ZERO TICKS");
			//encrypts(0)
			break;
		}
		case BAIM_REASON_BAD_HITCHANCE:
		{
			//decrypts(0)
			ret = XorStr("LOW HEAD HC");
			//encrypts(0)
			break;
		}
		case BAIM_REASON_NOT_ENOUGH_DAMAGE:
		{
			//decrypts(0)
			ret = XorStr("LOW HEAD DMG");
			//encrypts(0)
			break;
		}
		default:
		{
			//decrypts(0)
			ret = XorStr("NONE");
			//encrypts(0)
			break;
		}
	}

	return ret;
}

void CPlayerrecord::ResolveMovingPlayers(CTickrecord *_currentRecord, CTickrecord* _previousRecord, bool *PredictedSequenceMatches, float *PredictedCycles, float* PredictedWeights, float* PredictedPlaybackRates, float (*PredictedPoseParameters)[MAX_CSGO_POSE_PARAMS], CCSGOPlayerAnimState*&bestanimstate, float*&bestposes, bool& SearchForJitter)
{
	if (!m_bAllowMovingResolver || m_iResolveMode == RESOLVE_MODE_MANUAL || !variable::get().ragebot.b_resolver_moving)
		return;

	if (_currentRecord->m_AbsVelocity.Length2D() > 10.0f && _previousRecord && _previousRecord->m_AbsVelocity.Length2D() > 10.0f 
		/*&& _previousRecord && m_pAnimStateServer[ResolveSides::NONE]->m_bOnGround && !m_bIsHoldingDuck*/)
	{
		if (LocalPlayer.Entity && LocalPlayer.Entity->GetAlive())
		{
			//todo: use entity they are aa'ing 180 from
			CBaseEntity* Enemy = LocalPlayer.Entity;
			float FurthestAngle = FLT_MIN;
			float FurthestOtherAngle = FLT_MIN;
			float DeltaToOtherAngle;
			ResolveSides BestSide = ResolveSides::INVALID_RESOLVE_SIDE;
			ResolveSides CloseOtherSide = ResolveSides::INVALID_RESOLVE_SIDE;
			bool FurthestAngleIsTooCloseToOtherAngle = false;

			for (int i = 0; i < MAX_RESOLVE_SIDES; ++i)
			{
				if (i == ResolveSides::NONE)
					continue;

#ifdef NO_35
				if (i == ResolveSides::NEGATIVE_35 || i == ResolveSides::POSITIVE_35)
					continue;
#endif

				CCSGOPlayerAnimState *side = m_pAnimStateServer[i];

				float Angle = fabsf(AngleDiff(side->m_flGoalFeetYaw, m_pAnimStateServer[ResolveSides::NONE]->m_flGoalFeetYaw));
				if (Angle >= FurthestAngle)
				{
					float dt = fabsf(Angle - FurthestAngle);
					FurthestAngleIsTooCloseToOtherAngle = dt < 30.0f;
					if (FurthestAngleIsTooCloseToOtherAngle)
					{
						bool ThisIsPositive = g_LagCompensation.IsResolveSidePositive((ResolveSides)i);
						bool OtherIsPositive = g_LagCompensation.IsResolveSidePositive((ResolveSides)BestSide);

						if (ThisIsPositive != OtherIsPositive)
						{
							CloseOtherSide = BestSide;
							FurthestOtherAngle = FurthestAngle;
							DeltaToOtherAngle = dt;
						}
					}
					else
						CloseOtherSide = ResolveSides::INVALID_RESOLVE_SIDE;

					FurthestAngle = Angle;
					BestSide = (ResolveSides)i;
				}
			}

			if (FurthestAngleIsTooCloseToOtherAngle)
			{
				if (CloseOtherSide == ResolveSides::POSITIVE_35 || CloseOtherSide == ResolveSides::POSITIVE_60)
				{
					if (BestSide == ResolveSides::NEGATIVE_60 || BestSide == ResolveSides::NEGATIVE_35)
					{
						BestSide = CloseOtherSide;
					}
				}
			}

#if 0
			if (FurthestAngleIsTooCloseToOtherAngle)
			{
				QAngle AngleFromEnemyToUs = CalcAngle(Enemy->GetEyePosition(), m_pEntity->GetEyePosition());
				FurthestAngle = FLT_MIN;

				for (int i = 0; i < MAX_RESOLVE_SIDES; ++i)
				{
					if (i == ResolveSides::NONE)
						continue;

					CCSGOPlayerAnimState *side = m_pAnimStateServer[i];

					float Angle = fabsf(AngleDiff(AngleFromEnemyToUs.y, side->m_flGoalFeetYaw));
					if (Angle > FurthestAngle)
					{
						float dt = fabsf(Angle - FurthestAngle);
						FurthestAngleIsTooCloseToOtherAngle = dt < 1.5f;
						FurthestAngle = Angle;
						BestSide = (ResolveSides)i;
					}
				}
			}
#endif

			if (BestSide != ResolveSides::INVALID_RESOLVE_SIDE)
			{
				//if (/*bestside != m_nLastMovingResolveSide ||*/ Interfaces::Globals->realtime - m_flLastTimeResolvedFromMoving > 2.0f)
				//	m_iShotsMissed_MovingResolver = 0;

				m_nLastMovingResolveSide = BestSide;
				m_flLastTimeResolvedFromMoving = Interfaces::Globals->realtime;
				m_flLastMovingResolverDelta = 0.0f;

				if (m_iShotsMissed_MovingResolver >= MAX_MOVING_RESOLVER_SHOTS)
				{
					_currentRecord->m_bIsUsingMovingResolver = false;
				}
				else
				{
					ResolveSides sidetouse = GetMovingResolveSide(BestSide, _currentRecord, _previousRecord, GetBodyHitResolveInfo());

					_currentRecord->m_bIsUsingMovingResolver = true;
					_currentRecord->m_iResolveMode = RESOLVE_MODE_AUTOMATIC;
					if (sidetouse != ResolveSides::INVALID_RESOLVE_SIDE)
					{
						bestanimstate = m_pAnimStateServer[sidetouse];
						bestposes = PredictedPoseParameters[sidetouse];
						_currentRecord->m_iResolveSide = sidetouse;
						SetResolveSide(sidetouse, __FUNCTION__);
					}
					SearchForJitter = false;
				}
			}
		}

		return;

		//Don't try to resolve when they are accelerating/decelerating
		if (fabsf(_currentRecord->m_AbsVelocity.Length() - _previousRecord->m_AbsVelocity.Length()) > 2.5f)
			return;

		if (_currentRecord->m_AnimLayer[6].m_flWeight >= _previousRecord->m_AnimLayer[6].m_flWeight && _currentRecord->m_AnimLayer[6]._m_flPlaybackRate >= _previousRecord->m_AnimLayer[6]._m_flPlaybackRate)
		{
			QAngle move, last_move;
			VectorAngles(_currentRecord->m_AbsVelocity, move);

			const auto speed = min(_currentRecord->m_AbsVelocity.Length(), 260.0f);
			const auto max_movement_speed = m_pEntity->GetWeapon() ? max(m_pEntity->GetWeapon()->GetMaxSpeed3(), .001f) : 260.0f;
			const auto movement_speed = speed / (max_movement_speed * .520f);
			const auto ducking_speed = speed / (max_movement_speed * .340f);
			const float flYawModifier = (((m_pAnimStateServer[ResolveSides::NONE]->m_flGroundFraction * -.3f) - .2f) * movement_speed) + 1.f;

			//Do nothing if moving towards their desync angle
			if (fabsf(AngleDiff(move.y, _currentRecord->m_EyeAngles.y)) > 180.f - (m_pAnimStateServer[ResolveSides::NONE]->m_flMaxYaw * flYawModifier + 4.f))
				return;

			float deltas[ResolveSides::MAX_RESOLVE_SIDES];
			for (int i = 0; i < ResolveSides::MAX_RESOLVE_SIDES; ++i)
				deltas[i] = FLT_MAX;

			C_AnimationLayer *feet_layer = m_pEntity->GetAnimOverlayDirect(FEET_LAYER);
			float cycle = feet_layer->_m_flCycle;
			float weight = feet_layer->m_flWeight;
			float pbrate = feet_layer->_m_flPlaybackRate;
			float total = cycle;// +weight + pbrate;

			if (PredictedSequenceMatches[ResolveSides::NEGATIVE_60])
				deltas[ResolveSides::NEGATIVE_60] = fabsf((PredictedCycles[ResolveSides::NEGATIVE_60] /*+ PredictedWeights[ResolveSides::NEGATIVE_60] + PredictedPlaybackRates[ResolveSides::NEGATIVE_60]*/ ) - total);
			//if (PredictedSequenceMatches[ResolveSides::NEGATIVE_35])
			//	deltas[ResolveSides::NEGATIVE_35] = fabsf((PredictedCycles[ResolveSides::NEGATIVE_35] + PredictedWeights[ResolveSides::NEGATIVE_35] + PredictedPlaybackRates[ResolveSides::NEGATIVE_35]) - total);
			if (PredictedSequenceMatches[ResolveSides::POSITIVE_60])
				deltas[ResolveSides::POSITIVE_60] = fabsf((PredictedCycles[ResolveSides::POSITIVE_60] /* + PredictedWeights[ResolveSides::POSITIVE_60] + PredictedPlaybackRates[ResolveSides::POSITIVE_60]*/) - total);
			//if (PredictedSequenceMatches[ResolveSides::POSITIVE_35])
				//deltas[ResolveSides::POSITIVE_35] = fabsf((PredictedCycles[ResolveSides::POSITIVE_35] + PredictedWeights[ResolveSides::POSITIVE_35] + PredictedPlaybackRates[ResolveSides::POSITIVE_35]) - total);
			//if (PredictedSequenceMatches[ResolveSides::NONE])
				//deltas[ResolveSides::NONE] = fabsf((PredictedCycles[ResolveSides::NONE] /*+ PredictedWeights[ResolveSides::NONE] + PredictedPlaybackRates[ResolveSides::NONE]*/) - total);

			float cyclez[MAX_RESOLVE_SIDES];
			float weightz[MAX_RESOLVE_SIDES];
			float pbratez[MAX_RESOLVE_SIDES];
			for (int i = 0; i < MAX_RESOLVE_SIDES; ++i )
			{
				cyclez[i] = PredictedCycles[i];
				weightz[i] = PredictedWeights[i];
				pbratez[i] = PredictedPlaybackRates[i];
			}

			float bestdelta = FLT_MAX;
			ResolveSides bestside = ResolveSides::MAX_RESOLVE_SIDES;
			for (int i = 0; i < ResolveSides::MAX_RESOLVE_SIDES; ++i)
			{
				if (deltas[i] <= bestdelta)
				{
					bestside = (ResolveSides)i;
					bestdelta = deltas[i];
				}
			}

			if (bestside != ResolveSides::MAX_RESOLVE_SIDES)
			{
#if 0
				if (fabsf(AngleDiff(move.y, _currentRecord->m_EyeAngles.y)) <= 180.f - (58.0f * flYawModifier + 4.f)
					/*|| (m_nLastMovingResolveSide != ResolveSides::INVALID_RESOLVE_SIDE && fabsf(deltas[m_nLastMovingResolveSide] - m_flLastMovingResolverDelta) < 0.005f)*/)
#endif
				{
					//if (bestdelta <= m_flLastMovingResolverDelta
					//	|| bestside == m_nLastMovingResolveSide
					//	|| Interfaces::Globals->realtime - m_flLastTimeResolvedFromMoving > 1.5f
					//)
					{
						//if (/*bestside != m_nLastMovingResolveSide ||*/ Interfaces::Globals->realtime - m_flLastTimeResolvedFromMoving > 2.0f)
						//	m_iShotsMissed_MovingResolver = 0;

						m_nLastMovingResolveSide = bestside;
						m_flLastTimeResolvedFromMoving = Interfaces::Globals->realtime;
						m_flLastMovingResolverDelta = bestdelta;

						if (m_iShotsMissed_MovingResolver >= MAX_MOVING_RESOLVER_SHOTS)
						{
							_currentRecord->m_bIsUsingMovingResolver = false;
						}
						else
						{
							ResolveSides sidetouse = GetMovingResolveSide(bestside, _currentRecord, _previousRecord, GetBodyHitResolveInfo());

							_currentRecord->m_bIsUsingMovingResolver = true;
							_currentRecord->m_iResolveMode = RESOLVE_MODE_AUTOMATIC;

							if (sidetouse != ResolveSides::INVALID_RESOLVE_SIDE)
							{
								bestanimstate = m_pAnimStateServer[sidetouse];
								bestposes = PredictedPoseParameters[sidetouse];
								_currentRecord->m_iResolveSide = sidetouse;
								SetResolveSide(sidetouse, __FUNCTION__);
							}
							SearchForJitter = false;
						}
					}
				}
			}
		}
	}
}

void CPlayerrecord::DetectJitter(bool _foundSpecificSide, 
	CTickrecord* _currentRecord, 
	CTickrecord* _previousRecord, 
	float (*PredictedPoseParameters)[MAX_CSGO_POSE_PARAMS],
	float* &bestposes, 
	CCSGOPlayerAnimState*& bestanimstate)
{
	if (variable::get().ragebot.b_resolver_nojitter.get() || !m_bAllowJitterResolver || m_iResolveMode == RESOLVE_MODE_MANUAL)
		return;

	//jitter flipper
	if (!_foundSpecificSide && _currentRecord->m_iResolveSide != ResolveSides::NONE && _previousRecord && !_currentRecord->m_bFiredBullet)
	{
		CTickrecord* _prePreviousRecord = GetPrePreviousRecord();
		if (_prePreviousRecord && !_prePreviousRecord->m_Dormant && fabsf(AngleNormalize(_currentRecord->m_EyeAngles.x)) > 75.0f)
		{
			float diff = AngleDiff(_previousRecord->m_EyeAngles.y, _currentRecord->m_EyeAngles.y);// / TIME_TO_TICKS(m_pAnimStateServer[ResolveSides::NONE]->m_flLastClientSideAnimationUpdateTimeDelta);
			if (diff > 57.0f && (diff < 179.5f || diff > 180.0f))
			{
				if (Interfaces::Globals->realtime - m_flLastTimeDetectedJitter < 1.0f)
				{
					//force negative
					ResolveSides OppositeSide = ResolveSides::INVALID_RESOLVE_SIDE;

					switch (_currentRecord->m_iResolveSide)
					{
						case ResolveSides::POSITIVE_35:
						{
							OppositeSide = ResolveSides::NEGATIVE_35;
							break;
						}
						case ResolveSides::POSITIVE_60:
						{
							OppositeSide = ResolveSides::NEGATIVE_60;
							break;
						}
					};

					if (OppositeSide != ResolveSides::INVALID_RESOLVE_SIDE)
					{
						//Set the side
						m_pEntity->WritePoseParameters(PredictedPoseParameters[OppositeSide]);
						m_pEntity->SetAbsAngles({ 0.0f, m_pAnimStateServer[OppositeSide]->m_flGoalFeetYaw, 0.0f });
						bestanimstate = m_pAnimStateServer[OppositeSide];
						_currentRecord->m_iResolveSide = OppositeSide;
						bestposes = PredictedPoseParameters[OppositeSide];
#if defined DEBUG_BH || defined _DEBUG || defined INTERNAL_DEBUG
						std::string victim_name = adr_util::sanitize_name((char*)m_pEntity->GetName().data());
						std::string ev = __FUNCTION__ + std::string(" switched ") + victim_name + std::string("'s RECORD resolveside to ") + g_Visuals.GetResolveSide(OppositeSide);
						g_Eventlog.AddEventElement(0x6, ev);
						g_Eventlog.OutputEvent(variable::get().visuals.i_shot_logs);
#endif
					}
				}
				m_flLastTimeDetectedJitter = Interfaces::Globals->realtime;

			}
			else if (diff < -57.0f && (diff > -179.5f || diff < -180.0f))
			{
				if (Interfaces::Globals->realtime - m_flLastTimeDetectedJitter < 1.0f)
				{
					//force positive
					ResolveSides OppositeSide = ResolveSides::INVALID_RESOLVE_SIDE;

					switch (_currentRecord->m_iResolveSide)
					{
						case ResolveSides::NEGATIVE_35:
						{
							OppositeSide = ResolveSides::POSITIVE_35;
							break;
						}
						case ResolveSides::NEGATIVE_60:
						{
							OppositeSide = ResolveSides::POSITIVE_60;
							break;
						}
					};

					if (OppositeSide != ResolveSides::INVALID_RESOLVE_SIDE)
					{
						//Set the side
						m_pEntity->WritePoseParameters(PredictedPoseParameters[OppositeSide]);
						m_pEntity->SetAbsAngles({ 0.0f, m_pAnimStateServer[OppositeSide]->m_flGoalFeetYaw, 0.0f });
						bestanimstate = m_pAnimStateServer[OppositeSide];
						_currentRecord->m_iResolveSide = OppositeSide;
						bestposes = PredictedPoseParameters[OppositeSide];
#if defined DEBUG_BH || defined _DEBUG || defined INTERNAL_DEBUG
						std::string victim_name = adr_util::sanitize_name((char*)m_pEntity->GetName().data());
						std::string ev = __FUNCTION__ + std::string(" switched ") + victim_name + std::string("'s RECORD resolveside to ") + g_Visuals.GetResolveSide(OppositeSide);
						g_Eventlog.AddEventElement(0x6, ev);
						g_Eventlog.OutputEvent(variable::get().visuals.i_shot_logs);
#endif
					}
				}
				m_flLastTimeDetectedJitter = Interfaces::Globals->realtime;
			}
		}
	}
}

void CPlayerrecord::FixJumpFallPoseParameter(CTickrecord *_previousRecord)
{
	if (!m_pEntity->HasFlag(FL_ONGROUND) && Changed.Animations)
	{
		int CurrentActivity = GetSequenceActivity(m_pEntity, DataUpdateVars.m_PostAnimLayers[4]._m_nSequence);
		if (CurrentActivity == ACT_CSGO_FALL || CurrentActivity == ACT_CSGO_JUMP)
		{
			float startcycle;
			if (GetSequenceActivity(m_pEntity, _previousRecord->m_AnimLayer[4]._m_nSequence) != CurrentActivity)
			{
				//we know the animation was reset, so it started from 0
				startcycle = 0.0f;
				for (CCSGOPlayerAnimState** i = &m_pAnimStateServer[0]; i != &m_pAnimStateServer[MAX_RESOLVE_SIDES]; ++i)
				{
					CCSGOPlayerAnimState* animstate = *i;
					if (animstate)
						animstate->m_flTotalTimeInAir = 0.0f;
				}
			}
			else if (DataUpdateVars.m_PostAnimLayers[4]._m_flCycle < _previousRecord->m_AnimLayer[4]._m_flCycle)
			{
				//we know it was reset, so it started from 0
				startcycle = 0.0f;
				for (CCSGOPlayerAnimState** i = &m_pAnimStateServer[0]; i != &m_pAnimStateServer[MAX_RESOLVE_SIDES]; ++i)
				{
					CCSGOPlayerAnimState* animstate = *i;
					if (animstate)
						animstate->m_flTotalTimeInAir = 0.0f;
				}
			}
			else
			{
				//it incremented so see how many it incremented by
				//FIXME: what if it reset to 0 and incremented past the previous value? TODO: handle that case,
				//could check if delta between target and predicted value in backtrace is too much

				startcycle = _previousRecord->m_AnimLayer[4]._m_flCycle;
				for (int i = 0; i < MAX_RESOLVE_SIDES; ++i)
				{
					if (m_pAnimStateServer[i] && _previousRecord->m_pAnimStateServer[i])
						m_pAnimStateServer[i]->m_flTotalTimeInAir = _previousRecord->m_pAnimStateServer[i]->m_flTotalTimeInAir;
				}
			}

			int ticks_taken_to_get_to_server_cycle = m_pAnimStateServer[ResolveSides::NONE]->BacktraceCycle(startcycle, 4, false);
			if (ticks_taken_to_get_to_server_cycle == 0 || DataUpdateVars.m_PostAnimLayers[4]._m_flCycle >= 0.999f)
			{
				//if animation is finished already, fix up the total time in air
				int total_commands = m_iTicksChoked + 1;
				int ticks_left = total_commands - ticks_taken_to_get_to_server_cycle;
				ticks_taken_to_get_to_server_cycle += ticks_left;
			}

			float newdelta = (m_pAnimStateServer[ResolveSides::NONE]->m_flLastClientSideAnimationUpdateTimeDelta * ticks_taken_to_get_to_server_cycle);;
			for (CCSGOPlayerAnimState** i = &m_pAnimStateServer[0]; i != &m_pAnimStateServer[MAX_RESOLVE_SIDES]; ++i)
			{
				CCSGOPlayerAnimState* animstate = *i;
				if (animstate)
				{
					animstate->m_flTotalTimeInAir += newdelta;
				}
			}

			float v203 = (m_pAnimStateServer[ResolveSides::NONE]->m_flTotalTimeInAir - 0.72f) * 1.25f;
			v203 = clamp(v203, 0.0f, 1.0f);
			float newpose10 = (3.0f - (v203 + v203)) * (v203 * v203);
			newpose10 = clamp(newpose10, 0.0f, 1.0f);
			m_pAnimStateServer[ResolveSides::NONE]->m_arrPoseParameters[10].SetValue(m_pAnimStateServer[ResolveSides::NONE]->pBaseEntity, newpose10);
		}
	}
}

ResolveSides CPlayerrecord::GetMovingResolveSide(ResolveSides sidetouse, CTickrecord* _currentRecord, CTickrecord* _previousRecord, s_Impact_Info* BodyResolveInfo)
{
	if (sidetouse == ResolveSides::INVALID_RESOLVE_SIDE)
		return sidetouse;

	//Check if we have a body hit delta and use that for our desync amount
	if (/*_currentRecord->m_Velocity.Length() < 1.0f && */BodyResolveInfo && BodyResolveInfo->m_bIsBodyHitResolved && sidetouse != ResolveSides::NONE)
	{
		if (BodyResolveInfo->m_bIsNearMaxDesyncDelta)
		{
			if (sidetouse == ResolveSides::NEGATIVE_35)
				sidetouse = ResolveSides::NEGATIVE_60;
			else if (sidetouse == ResolveSides::POSITIVE_35)
				sidetouse = ResolveSides::POSITIVE_60;
		}
		else if (fabsf(BodyResolveInfo->m_flDesyncDelta) > 0.0f)
		{
			if (sidetouse == ResolveSides::NEGATIVE_60)
				sidetouse = ResolveSides::NEGATIVE_35;
			else if (sidetouse == ResolveSides::POSITIVE_60)
				sidetouse = ResolveSides::POSITIVE_35;
		}
	}

	return sidetouse;
}

void CPlayerrecord::PredictLowerBodyYaw(CTickrecord* _previousRecord, CTickrecord* _currentRecord)
{
	// predict lower body yaw
	if (_previousRecord && !_previousRecord->m_Dormant)
	{
		int _TotalNewCommands = m_iTicksChoked + 1;
		Vector _absVeloSlope = (_currentRecord->m_AbsVelocity - _previousRecord->m_AbsVelocity) / _TotalNewCommands;
		Vector _absVelo = _previousRecord->m_AbsVelocity;
		Vector _lastvelocity = m_lastvelocity;
		float _simTime = _currentRecord->m_flFirstCmdTickbaseTime - TICKS_TO_TIME(1);
		for (int i = 0; i < _TotalNewCommands; ++i)
		{
			_absVelo += _absVeloSlope;
			_simTime += TICK_INTERVAL;
			Vector _absVelTmp = _absVelo;
			_absVelTmp.z = 0.0f;
			_lastvelocity = g_LagCompensation.GetSmoothedVelocity(TICK_INTERVAL * 2000.0f, _absVelTmp, _lastvelocity);
			float m_flSpeed = fminf(_lastvelocity.Length(), 260.0f);

			if (m_flSpeed > 0.1f || fabsf(_absVelo.z) > 100.0f)
				m_next_lby_update_time = _simTime + 0.22f;
			else if (_simTime > m_next_lby_update_time)
				m_next_lby_update_time = _simTime + 1.1f;
		}
	}
	else
	{
		m_next_lby_update_time = m_pEntity->GetSimulationTime() + 1.1f;
	}

	for (int i = 0; i < MAX_RESOLVE_SIDES; ++i)
	{
		m_pAnimStateServer[i]->m_flNextLowerBodyYawUpdateTime = m_next_lby_update_time;
	}

	if (_currentRecord)
		_currentRecord->m_next_lby_update_time = m_next_lby_update_time;
}

void CPlayerrecord::RunStandingResolver(CTickrecord* _previousRecord, CTickrecord* _currentRecord, CTickrecord* _prePreviousRecord, bool& DetectedDesyncSide, s_Impact_Info* BodyResolveInfo)
{
	//automatic standing resolver was found to just plain not work reliably anymore
	const bool ENABLE_AUTOMATIC_RESOLVER = false;

	if (m_iResolveMode == RESOLVE_MODE_MANUAL)
		return;

	// try to find which direction they are desyncing when standing still
	if (ENABLE_AUTOMATIC_RESOLVER && (_currentRecord->m_bBalanceAdjust ||
		(m_iTicksChoked > 0 && !_currentRecord->m_bTickbaseShiftedBackwards && !_currentRecord->m_bFiredBullet && _previousRecord && _currentRecord->m_AbsVelocity.Length() < 1.0f /*&& m_pAnimStateServer[ResolveSides::NONE]->m_flTimeSinceStoppedMoving > 1.5f*/)))
	{
		if (_currentRecord->m_bBalanceAdjust)
		{
			C_AnimationLayer &firesequence = *m_pEntity->GetAnimOverlayDirect(LOWERBODY_LAYER);
			Activities act = GetSequenceActivity(m_pEntity, firesequence._m_nSequence);
			if (act != ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING)
			{
				float diff = 0.0f;

				if (!_previousRecord || (diff = AngleDiff(_previousRecord->m_EyeAngles.y, _currentRecord->m_EyeAngles.y), diff >= 58.0f || diff < 5.0f))
					m_BalanceAdjustResults = BALANCE_ADJUST_FOUND | BALANCE_ADJUST_NEGATIVE;
				else
					m_BalanceAdjustResults = BALANCE_ADJUST_FOUND | BALANCE_ADJUST_POSITIVE;
			}
			else
			{
				m_BalanceAdjustResults = 0;
			}
		}

		int _testDesyncResults = FindDesyncDirection(_previousRecord, _currentRecord, _prePreviousRecord, &DetectedDesyncSide);
		if (DetectedDesyncSide)
		{
			bool OldIsPositive = (m_DesyncResults & DESYNC_RESULTS_POSITIVE) ? true : false;
			bool NewIsPositive = (_testDesyncResults & DESYNC_RESULTS_POSITIVE) ? true : false;
			bool DetectedDifferentSide = !(m_DesyncResults & DESYNC_RESULTS_FOUND) || OldIsPositive != NewIsPositive;

			if (DetectedDifferentSide
				&&
				(!BodyResolveInfo || Interfaces::Globals->realtime - BodyResolveInfo->m_flLastResolveTime > 10.0f || (int)BodyResolveInfo->IsPositive() == NewIsPositive))
			{
				m_DesyncResults = _testDesyncResults;
				if (m_DesyncResults & DESYNC_RESULTS_POSITIVE)
				{
					if (m_iResolveSide != ResolveSides::POSITIVE_60 /*&& m_iResolveSide != ResolveSides::POSITIVE_35*/)
					{
						//if (!BodyResolveInfo || BodyResolveInfo->m_bIsNearMaxDesyncDelta || Interfaces::Globals->realtime - BodyResolveInfo->m_flLastResolveTime > 5.0f)
							SetResolveSide(ResolveSides::POSITIVE_60, "Side Detection");
						//else
						//	SetResolveSide(ResolveSides::POSITIVE_35, "Side Detection");
					}
				}
				else
				{
					if (m_iResolveSide != ResolveSides::NEGATIVE_60 /*&& m_iResolveSide != ResolveSides::NEGATIVE_35*/)
					{
						//if (!BodyResolveInfo || BodyResolveInfo->m_bIsNearMaxDesyncDelta || Interfaces::Globals->realtime - BodyResolveInfo->m_flLastResolveTime > 5.0f)
							SetResolveSide(ResolveSides::NEGATIVE_60, "Side Detection");
						//else
						//	SetResolveSide(ResolveSides::NEGATIVE_35, "Side Detection");
					}
				}

				m_iResolveMode = RESOLVE_MODE_AUTOMATIC;
				_currentRecord->m_iResolveMode = RESOLVE_MODE_AUTOMATIC;
				_currentRecord->m_iResolveSide = m_iResolveSide;
				_currentRecord->m_bIsUsingMovingResolver = false;
			}
		}
	}
}

void CPlayerrecord::GetDesyncAmountsToAnimate(float(*PredictionDeltasEyeAndDesync)[2], CTickrecord *_currentRecord, CTickrecord *_previousRecord, CTickrecord* _prePreviousRecord, s_Impact_Info* BodyResolveInfo)
{
	PredictionDeltasEyeAndDesync[ResolveSides::NONE][0] = 0.0f;
	PredictionDeltasEyeAndDesync[ResolveSides::NONE][1] = 0.0f;
	PredictionDeltasEyeAndDesync[ResolveSides::NEGATIVE_60][0] = -120.0f;
	PredictionDeltasEyeAndDesync[ResolveSides::NEGATIVE_60][1] = -60.0f;
	PredictionDeltasEyeAndDesync[ResolveSides::POSITIVE_60][0] = 120.0f;
	PredictionDeltasEyeAndDesync[ResolveSides::POSITIVE_60][1] = 60.0f;
	PredictionDeltasEyeAndDesync[ResolveSides::NEGATIVE_35][0] = -95.0f;
	PredictionDeltasEyeAndDesync[ResolveSides::NEGATIVE_35][1] = -35.0f;
	PredictionDeltasEyeAndDesync[ResolveSides::POSITIVE_35][0] = 95.0f;
	PredictionDeltasEyeAndDesync[ResolveSides::POSITIVE_35][1] = 35.0f;

	//Don't resolve if they are e-spamming
	if (m_flTimeSinceStartedPressingUseKey != 0.0f)
	{
		for (int i = 0; i < MAX_RESOLVE_SIDES; ++i)
		{
			PredictionDeltasEyeAndDesync[i][0] = 0.0f;
			PredictionDeltasEyeAndDesync[i][1] = 0.0f;
		}
	}

	float _BodyHitDesyncDelta = 0.0f, _BodyHitEyeDelta = 0.0f;

	if (BodyResolveInfo && GetBodyHitDesyncAmount(&_BodyHitDesyncDelta, &_BodyHitEyeDelta, _currentRecord, _previousRecord, _prePreviousRecord, BodyResolveInfo) && _BodyHitDesyncDelta != 0.0f)
	{
		float absdelta = fabsf(_BodyHitDesyncDelta);
		float abseyedelta = fabsf(_BodyHitEyeDelta);

		if (BodyResolveInfo->m_bIsNearMaxDesyncDelta)
		{
			PredictionDeltasEyeAndDesync[ResolveSides::POSITIVE_60][0] = abseyedelta;
			PredictionDeltasEyeAndDesync[ResolveSides::NEGATIVE_60][0] = -abseyedelta;
			PredictionDeltasEyeAndDesync[ResolveSides::POSITIVE_60][1] = absdelta;
			PredictionDeltasEyeAndDesync[ResolveSides::NEGATIVE_60][1] = -absdelta;
			_currentRecord->m_bUsedBodyHitResolveDelta = m_iResolveSide == ResolveSides::POSITIVE_60 || m_iResolveSide == ResolveSides::NEGATIVE_60;
		}
		else if (absdelta >= 0.0f)
		{
			PredictionDeltasEyeAndDesync[ResolveSides::POSITIVE_35][0] = abseyedelta;
			PredictionDeltasEyeAndDesync[ResolveSides::NEGATIVE_35][0] = -abseyedelta;
			PredictionDeltasEyeAndDesync[ResolveSides::POSITIVE_35][1] = absdelta;
			PredictionDeltasEyeAndDesync[ResolveSides::NEGATIVE_35][1] = -absdelta;
			_currentRecord->m_bUsedBodyHitResolveDelta = m_iResolveSide == ResolveSides::POSITIVE_35 || m_iResolveSide == ResolveSides::NEGATIVE_35;
		}
		_currentRecord->Impact.m_iBodyHitResolveStance = GetBodyHitStance(_currentRecord);
	}
}

void CPlayerrecord::FSN_AnimatePlayer(CTickrecord * _currentRecord, CTickrecord* _previousRecord, CTickrecord* _prePreviousRecord)
{
	// validate hooks
	if (!Hooks.m_bHookedBaseEntity)
		return;

	// get simulate func
	using SimulateFn = unsigned char(__thiscall *)(CBaseEntity*);
	SimulateFn oSimulate = nullptr;
	if (Hooks.m_bHookedBaseEntity)
		oSimulate = (SimulateFn)Hooks.BaseEntity->GetOriginalHookedSub4();

	//Override resolve modes if enemy is not legit
	if (m_iResolveMode == RESOLVE_MODE_NONE && (!m_bLegit || m_bForceNotLegit) && m_flNextLegitCheck != FLT_MIN)
	{
		m_iResolveMode = RESOLVE_MODE_BRUTE_FORCE;
		m_iResolveSide = ResolveSides::POSITIVE_60;
	}

	// valid tick record
	if (_currentRecord)
	{
		//Reset shots missed if they respawned
		if (m_pEntity->GetSpawnTime() != m_flSpawnTime)
		{
			m_iShotsMissed = 0;
			m_iShotsMissed_BalanceAdjust = 0;
			m_iShotsMissed_MovingResolver = 0;
			m_flLastTimeDetectedJitter = 0.0f;
		}

		//If player is not an enemy, just do basic animation updates for FPS increase
		if (!m_pEntity->IsEnemy(Interfaces::ClientEntList->GetBaseEntity(Interfaces::EngineClient->GetLocalPlayer())))
		{
			m_iOldResolveSide = INVALID_RESOLVE_SIDE;
			m_iResolveMode = RESOLVE_MODE_NONE;
			m_iShotsMissed_MovingResolver = 0;
			_currentRecord->m_iResolveMode = RESOLVE_MODE_NONE;
			_currentRecord->m_iResolveSide = ResolveSides::NONE;
			_currentRecord->m_bIsUsingMovingResolver = false;
			_currentRecord->Impact.m_iBodyHitResolveStance = MAX_IMPACT_RESOLVE_STANCES;
			m_flTimeSinceStartedPressingUseKey = 0.0f;

			//Fix the jumping animation for all the animstates
			bool jumping = (GetSequenceActivity(m_pEntity, DataUpdateVars.m_PostAnimLayers[4]._m_nSequence) == ACT_CSGO_JUMP) && !m_pEntity->HasFlag(FL_ONGROUND);

			for (CCSGOPlayerAnimState** i = &m_pAnimStateServer[0]; i != &m_pAnimStateServer[MAX_RESOLVE_SIDES]; ++i)
			{
				CCSGOPlayerAnimState* animstate = *i;
				animstate->m_bJumping = jumping;
			}

			FSN_AnimateTicks(_previousRecord, _currentRecord, m_pAnimStateServer[ResolveSides::NONE], 0.0f, 0.0f, false, ResolveSides::NONE);
			m_pEntity->CopyPoseParameters(_currentRecord->m_flPoseParams);
			for (auto& i : _currentRecord->m_flPredictedPoseParameters)
			{
				m_pEntity->CopyPoseParameters(i);
			}
			m_pEntity->CopyAnimLayers(_currentRecord->m_AnimLayer);

			m_pAnimStateServer[ResolveSides::NONE]->m_flFeetCycle = _currentRecord->m_flFeetCycle;
			m_pAnimStateServer[ResolveSides::NONE]->m_flFeetWeight = _currentRecord->m_flFeetWeight;

			//Copy the unresolved animstate to all the others
			for (int i = 0; i < MAX_RESOLVE_SIDES; ++i)
			{
				CCSGOPlayerAnimState* animstate = m_pAnimStateServer[i];
				_currentRecord->m_flPredictedGoalFeetYaw[i] = animstate->m_flGoalFeetYaw;

				if (i != ResolveSides::NONE)
					*animstate = *m_pAnimStateServer[ResolveSides::NONE];
			}

			*m_pEntity->EyeAngles() = _currentRecord->m_EyeAngles;
			m_pEntity->SetEyeAngles(_currentRecord->m_EyeAngles);

			PredictLowerBodyYaw(_previousRecord, _currentRecord);
		}
		else
		{
			bool _Resolve = variable::get().ragebot.b_resolver && !m_bIsBot && (!m_bLegit || m_bForceNotLegit);
			bool DetectedDesyncSide = false;
			s_Impact_Info *BodyResolveInfo = nullptr;

			if (_Resolve && LocalPlayer.Entity)
			{
				if (input::get().is_key_down(variable::get().ragebot.i_flipenemy_key) && Interfaces::Globals->realtime - m_flLastFlipSideTime > 0.2f)
				{
					QAngle angles;
					Interfaces::EngineClient->GetViewAngles(angles);
					QAngle toenemy = CalcAngle(LocalPlayer.Entity->GetEyePosition(), m_pEntity->GetAbsOriginDirect() + Vector(0.0f, 0.0f, 50.0f));
					float fov = GetFov(angles, toenemy);
					if (fov < 70.0f)
					{
						m_iResolveSide = g_LagCompensation.GetOppositeResolveSide(m_iResolveSide);
						m_flLastFlipSideTime = Interfaces::Globals->realtime;
					}
				}
			}

			// store the resolve mode
			_currentRecord->m_iResolveMode = m_iResolveMode;
			_currentRecord->m_iResolveSide = m_iResolveSide;
			_currentRecord->m_bIsUsingMovingResolver = variable::get().ragebot.b_resolver_moving && m_bAllowMovingResolver && m_iResolveMode != RESOLVE_MODE_MANUAL && _previousRecord && m_iShotsMissed_MovingResolver < MAX_MOVING_RESOLVER_SHOTS ? _previousRecord->m_bIsUsingMovingResolver : false;
			_currentRecord->Impact.m_iBodyHitResolveStance = MAX_IMPACT_RESOLVE_STANCES;

			//Reset moving resolver shots when they stop for the first time
			//if (_currentRecord->m_Velocity.Length() < 2.0f && _previousRecord && _previousRecord->m_Velocity.Length() > 3.0f)
			//	m_iShotsMissed_MovingResolver = 0;

			// Store the jumping state in each animstate since it won't be called on the client's gamemovement
			bool jumping = (GetSequenceActivity(m_pEntity, DataUpdateVars.m_PostAnimLayers[4]._m_nSequence) == ACT_CSGO_JUMP)
				&& !m_pEntity->HasFlag(FL_ONGROUND);

			for (CCSGOPlayerAnimState** i = &m_pAnimStateServer[0]; i != &m_pAnimStateServer[MAX_RESOLVE_SIDES]; ++i)
			{
				CCSGOPlayerAnimState* animstate = *i;
				animstate->m_bJumping = jumping;
			}

			if (!_Resolve)
			{
				//is legit player or resolver disabled
				//disable resolver
				//decrypts(0)
				SetResolveSide(ResolveSides::NONE, XorStr("Resolver Disabled"));
				//encrypts(0)
				_currentRecord->m_iResolveSide = ResolveSides::NONE;
				_currentRecord->m_iResolveMode = RESOLVE_MODE_NONE;
				m_iResolveMode = RESOLVE_MODE_NONE;
				m_flTimeSinceStartedPressingUseKey = 0.0f;
			}
			else
			{
				//not legit player

				if (variable::get().ragebot.b_resolver_experimental)
					BodyResolveInfo = GetBodyHitResolveInfo();

				//Standing automatic resolver was found to not be reliable anymore
				//RunStandingResolver(_previousRecord, _currentRecord, _prePreviousRecord, DetectedDesyncSide, BodyResolveInfo);

				if (m_bAllowMovingResolver 
					&& variable::get().ragebot.b_resolver_moving
					&& m_iResolveMode != RESOLVE_MODE_MANUAL
					&& _currentRecord->m_bIsUsingMovingResolver 
					&& m_nLastMovingResolveSide != ResolveSides::INVALID_RESOLVE_SIDE 
					&& m_iShotsMissed_MovingResolver < MAX_MOVING_RESOLVER_SHOTS)
				{
					ResolveSides sidetouse = GetMovingResolveSide(m_nLastMovingResolveSide, _currentRecord, _previousRecord, BodyResolveInfo);
					m_iResolveMode = RESOLVE_MODE_AUTOMATIC;
					_currentRecord->m_iResolveMode = RESOLVE_MODE_AUTOMATIC;
					_currentRecord->m_iResolveSide = sidetouse;
					//decrypts(0)
					SetResolveSide(sidetouse, XorStr("Moving Resolver"));
					//encrypts(0)
					_currentRecord->m_bIsUsingFreestandResolver = false;
				}
				else
				{
					if (m_iResolveMode != RESOLVE_MODE_MANUAL)
						m_iResolveMode = RESOLVE_MODE_BRUTE_FORCE;
					_currentRecord->m_iResolveMode = m_iResolveMode;
					_currentRecord->m_iResolveSide = m_iResolveSide;
					_currentRecord->m_bIsUsingMovingResolver = false;
					//if (m_iResolveSide == ResolveSides::NONE)
					//	_Resolve = false;
				}
			}

#if 0
			if (_currentRecord->m_bIsUsingMovingLBYMeme)
			{
				//Can't desync more than 35/-35 when using the moving lby meme
	
				if (m_iResolveSide == ResolveSides::NEGATIVE_60)
				{
					//decrypts(0)
					SetResolveSide(ResolveSides::NEGATIVE_35, XorStr("Move In Place"));
					//encrypts(0)
					_currentRecord->m_iResolveSide = ResolveSides::NEGATIVE_35;
				}
				else if (m_iResolveSide == ResolveSides::POSITIVE_60)
				{
					//decrypts(0)
					SetResolveSide(ResolveSides::POSITIVE_35, XorStr("Move In Place"));
					//encrypts(0)
					_currentRecord->m_iResolveSide = ResolveSides::POSITIVE_35;
				}
			}
#endif

			//Get the amount of desync to animate for each resolve side

			CCSGOPlayerAnimState *bestanimstate = m_pAnimStateServer[_currentRecord->m_iResolveSide];
			auto& PredictedPoseParameters = _currentRecord->m_flPredictedPoseParameters;
			float *bestposes = &PredictedPoseParameters[_currentRecord->m_iResolveSide][0];
			float PredictionDeltasEyeAndDesync[MAX_RESOLVE_SIDES][2];	

			if (_Resolve && _previousRecord && !_previousRecord->m_Dormant)
			{
				if (m_flTimeSinceStartedPressingUseKey != 0.0f)
					m_flTimeSinceStartedPressingUseKey += TICKS_TO_TIME(m_iTicksChoked + 1);

				if (_currentRecord->m_bFiredBullet)
				{
					m_flTimeSinceStartedPressingUseKey = 0.0f;
				}
				else
				{
					if (fabsf(AngleNormalize(_currentRecord->m_EyeAngles.x)) < 30.0f)
					{
						if (fabsf(AngleNormalize(_previousRecord->m_EyeAngles.x)) > 75.0f || _previousRecord->m_bFiredBullet)
							m_flTimeSinceStartedPressingUseKey += TICKS_TO_TIME(m_iTicksChoked + 1);
					}
					else
					{
						m_flTimeSinceStartedPressingUseKey = 0.0f;
					}
				}
			}
			else
			{
				m_flTimeSinceStartedPressingUseKey = 0.0f;
			}
			
			GetDesyncAmountsToAnimate(PredictionDeltasEyeAndDesync, _currentRecord, _previousRecord, _prePreviousRecord, BodyResolveInfo);

			//Vars used for detecting the moving resolve side
			C_AnimationLayer *feet_layer = m_pEntity->GetAnimOverlayDirect(FEET_LAYER);
			float PredictedLayerCycles[MAX_RESOLVE_SIDES];
			float PredictedLayerWeights[MAX_RESOLVE_SIDES];
			float PredictedLayerPlaybackRates[MAX_RESOLVE_SIDES];
			bool LayerSequencesMatch[MAX_RESOLVE_SIDES];

			//Animate all of the animstates
			for (int i = 0; i < MAX_RESOLVE_SIDES; ++i)
			{
				FSN_AnimateTicks(_previousRecord, _currentRecord, m_pAnimStateServer[i], PredictionDeltasEyeAndDesync[i][0], PredictionDeltasEyeAndDesync[i][1], i != ResolveSides::NONE, (ResolveSides)i, true, false);
				m_pEntity->CopyPoseParameters(PredictedPoseParameters[i]);
				_currentRecord->m_flPredictedGoalFeetYaw[i] = m_pAnimStateServer[i]->m_flGoalFeetYaw;

				if (LayerSequencesMatch[i] = feet_layer->_m_nSequence == DataUpdateVars.m_PostAnimLayers[6]._m_nSequence)
				{
					PredictedLayerCycles[i] = feet_layer->_m_flCycle;
					PredictedLayerWeights[i] = feet_layer->m_flWeight;
					PredictedLayerPlaybackRates[i] = feet_layer->_m_flPlaybackRate;
				}

				m_pAnimStateServer[i]->m_flFeetCycle = DataUpdateVars.m_flPostFeetCycle;
				m_pAnimStateServer[i]->m_flFeetWeight = DataUpdateVars.m_flPostFeetWeight;
			}

			//Restore the server animation layers
			m_pEntity->WriteAnimLayers(DataUpdateVars.m_PostAnimLayers);

			//Post processing code that can change the side to use at the last minute
			if (_Resolve)
			{
				if ((m_iResolveMode != RESOLVE_MODE_NONE || !BodyResolveInfo) && m_iResolveMode != RESOLVE_MODE_MANUAL)
				{
					bool SearchForJitter = true;
					ResolveMovingPlayers(_currentRecord, _previousRecord, LayerSequencesMatch, PredictedLayerCycles, PredictedLayerWeights, PredictedLayerPlaybackRates, PredictedPoseParameters, bestanimstate, bestposes, SearchForJitter);

					//detect jitters and flip the desync side to use if they are jittering
					DetectJitter(DetectedDesyncSide, _currentRecord, _previousRecord, PredictedPoseParameters, bestposes, bestanimstate);
				}

				if (_currentRecord->m_bIsUsingMovingResolver)
				{
					//Erase moving resolver flag if we used some other side
					ResolveSides sidetouse = GetMovingResolveSide(m_nLastMovingResolveSide, _currentRecord, _previousRecord, BodyResolveInfo);

					if (_currentRecord->m_iResolveSide != sidetouse)
						_currentRecord->m_bIsUsingMovingResolver = false;
				}
			}
			else
			{
				_currentRecord->m_bIsUsingMovingResolver = false;
			}

			_currentRecord->StoreOppositeResolveSide();

			m_pEntity->SetAbsAngles({ 0.0f, bestanimstate->m_flGoalFeetYaw, 0.0f });
			m_pEntity->WritePoseParameters(bestposes);

			if (_previousRecord)
			{
				//Fix the falling/jumping animation pose parameter because when people choke that pose parameter is desynced
				FixJumpFallPoseParameter(_previousRecord);
				const float _fixedJumpFallPose = m_pEntity->GetPoseParameterScaled(jump_fall);
				for (int i = 0; i < MAX_RESOLVE_SIDES; ++i)
				{
					_currentRecord->m_flPredictedPoseParameters[i][jump_fall] = _fixedJumpFallPose;
				}
			}

			// restore eye angles
			*m_pEntity->EyeAngles() = _currentRecord->m_EyeAngles;
			m_pEntity->SetEyeAngles(_currentRecord->m_EyeAngles);

			PredictLowerBodyYaw(_previousRecord, _currentRecord);
		}
	}
}

//NOTE: results for lower body yaw are not supported for anything but the local player
void CPlayerrecord::GetSetupVelocityResults(QAngle& eyeangles, SetUpVelocityResults_t& dest, float time_since_last_animation_update, float curtime, Vector& absVelocity, float duckAmount, float lby)
{
	auto entity = m_pEntity;
	float time = curtime != -1.0f ? curtime : Interfaces::Globals->curtime;

	//temporary vars so we don't modify the pristine values
	float tm_curfeetyaw;
	float tm_goalfeetyaw;
	float tm_last_animation_update_time;
	float tm_next_lby_update_time;
	float tm_predicted_lowerbodyyaw = lby;
	float tm_duckamount;
	Vector tm_velocity;
	if (!dest.m_use_output_as_input)
	{
		DebugBreak();
		//tm_curfeetyaw = m_curfeetyaw;
		//tm_goalfeetyaw = m_goalfeetyaw;
		//tm_last_animation_update_time = m_flLastAnimationUpdateTime;
		tm_next_lby_update_time = m_next_lby_update_time;
		//tm_predicted_lowerbodyyaw = m_predicted_lowerbodyyaw;
		//tm_duckamount = m_last_duckamount;
		tm_velocity = m_lastvelocity;
	}
	else
	{
		tm_curfeetyaw = dest.m_curfeetyaw;
		tm_goalfeetyaw = dest.m_goalfeetyaw;
		tm_last_animation_update_time = dest.m_last_animation_update_time;
		tm_next_lby_update_time = dest.m_next_lby_update_time;
		//tm_predicted_lowerbodyyaw = dest.m_predicted_lowerbodyyaw;
		tm_duckamount = dest.m_duckamount;
		tm_velocity = dest.m_velocity;
	}

	const float SpawnTime = entity->GetSpawnTime();
	if (SpawnTime != m_flSpawnTime)
	{
		tm_curfeetyaw = eyeangles.y;
		tm_goalfeetyaw = eyeangles.y;
		//m_wasonground = false;
		tm_duckamount = 0.0f;
		tm_velocity.Init(0, 0, 0);
	}

	//this is the command the server will update animations on

	CCSGOPlayerAnimState *animstate = m_pAnimStateServer[ResolveSides::NONE];

	QAngle predictionangles = eyeangles;
	NormalizeAngles(predictionangles);

	//*entity->EyeAngles() = eyeangles;
	float m_flEyeYaw = eyeangles.y;
	float m_flPitch = eyeangles.x + entity->GetThirdPersonRecoil();
	NormalizeAngle(m_flPitch);
	float m_flLastClientSideAnimationUpdateTimeDelta = time_since_last_animation_update == -1.0f ? fmaxf(0.0f, time - tm_last_animation_update_time) : time_since_last_animation_update;

	//entity->SetLocalAngles(predictionangles);
	//entity->m_angRotation() = predictionangles;
	//entity->SetAbsOrigin(entity->GetLocalOrigin());

	//Beginning of animstate->Update(eyeyaw, eyepitch, forceupdate)
	//Not full code, only the important things for us that we need to use on the client right now

	//animstate->m_vOrigin = entity->m_vecAbsOrigin();

	float flLastDuckAmount = tm_duckamount;
	float flNewDuckAmount = clamp(duckAmount + animstate->m_flHitGroundCycle, 0.0f, 1.0f);
	float flDuckSmooth = m_flLastClientSideAnimationUpdateTimeDelta * 6.0f;
	float flDuckDelta = flNewDuckAmount - flLastDuckAmount;

	if (flDuckDelta <= flDuckSmooth) {
		if (-flDuckSmooth > flDuckDelta)
			flNewDuckAmount = flLastDuckAmount - flDuckSmooth;
	}
	else {
		flNewDuckAmount = flDuckSmooth + flLastDuckAmount;
	}

	tm_duckamount = clamp(flNewDuckAmount, 0.0f, 1.0f);
	//End of Update and before SetupVelocity

	//SetupVelocity
	Vector absvelocity = absVelocity;
	float m_flAbsVelocityZ = absvelocity.z;
	absvelocity.z = 0.0f;

	tm_velocity = g_LagCompensation.GetSmoothedVelocity(m_flLastClientSideAnimationUpdateTimeDelta * 2000.0f, absvelocity, tm_velocity);

	float m_flSpeed = fminf(tm_velocity.Length(), 260.0f);

	// TODO: once C_CSGOPlayerAnimState::Update is fully reversed, replace the local player weapon with the animstate->pWeapon
	float flMaxMovementSpeed = 260.0f;
	if (entity && entity->GetWeapon())
		flMaxMovementSpeed = fmaxf(entity->GetWeapon()->GetMaxSpeed3(), 0.001f);

	float m_flRunningSpeed = m_flSpeed / (flMaxMovementSpeed * 0.520f);
	float m_flDuckingSpeed = m_flSpeed / (flMaxMovementSpeed * 0.340f);

	tm_curfeetyaw = tm_goalfeetyaw;
	tm_goalfeetyaw = clamp(tm_goalfeetyaw, -360.0f, 360.0f);
	float eye_feet_delta = AngleDiff(m_flEyeYaw, tm_goalfeetyaw);

	float flRunningSpeed = clamp(m_flRunningSpeed, 0.0f, 1.0f);
	float flYawModifier = (((animstate->m_flGroundFraction * -0.3f) - 0.2f) * m_flRunningSpeed) + 1.0f;

	if (tm_duckamount > 0.0f) {
		float flDuckingSpeed = clamp(m_flDuckingSpeed, 0.0f, 1.0f);
		flYawModifier = ((flDuckingSpeed * tm_duckamount) * (0.5f - flYawModifier)) + flYawModifier;
	}
	float flMaxYawModifier = flYawModifier * animstate->m_flMaxYaw;
	float flMinYawModifier = flYawModifier * animstate->m_flMinYaw;

	if (eye_feet_delta <= flMaxYawModifier) {
		if (flMinYawModifier > eye_feet_delta)
			tm_goalfeetyaw = fabs(flMinYawModifier) + m_flEyeYaw;
	}
	else {
		tm_goalfeetyaw = m_flEyeYaw - fabsf(flMaxYawModifier);
	}

	NormalizeAngle(tm_goalfeetyaw);

	if (m_flSpeed <= 0.1f && fabsf(m_flAbsVelocityZ) <= 100.0f) {
		float current_lowerbodyyaw = tm_predicted_lowerbodyyaw;
		float newgoalfeetyaw = ApproachAngle(
			current_lowerbodyyaw,
			tm_goalfeetyaw,
			m_flLastClientSideAnimationUpdateTimeDelta * 100.0f);
		tm_goalfeetyaw = newgoalfeetyaw;

		if (time > tm_next_lby_update_time) {

			float goalfeetyaw_eye_delta = AngleDiff(newgoalfeetyaw, m_flEyeYaw);

			if (fabsf(goalfeetyaw_eye_delta) > 35.0f) {
				tm_next_lby_update_time = time + 1.1f;
				dest.m_lby_timer_updated = true;
				if (current_lowerbodyyaw != m_flEyeYaw) {
					dest.m_lby_updated = true;
				}
			}
		}
	}
	else {
		float newgoalfeetyaw = ApproachAngle(
			m_flEyeYaw,
			tm_goalfeetyaw,
			((animstate->m_flGroundFraction * 20.0f) + 30.0f)
			* m_flLastClientSideAnimationUpdateTimeDelta);

		tm_goalfeetyaw = newgoalfeetyaw;
		tm_next_lby_update_time = time + 0.22f;
		dest.m_lby_timer_updated = true;
		if (tm_predicted_lowerbodyyaw != m_flEyeYaw) {
			dest.m_lby_updated = true;
		}
	}

	if (m_flSpeed <= 1.0f
		&& animstate->m_bOnGround
		&& !animstate->m_bOnLadder
		&& !animstate->m_bInHitGroundAnimation
		&& m_flLastClientSideAnimationUpdateTimeDelta > 0.0f
		&& (fabsf(AngleDiff(tm_curfeetyaw, tm_goalfeetyaw))
			/ m_flLastClientSideAnimationUpdateTimeDelta) > 120.0f)
	{
		dest.m_979_triggered = true;
	}

	float eye_goalfeet_delta = AngleDiff(m_flEyeYaw, tm_goalfeetyaw);

	dest.m_bodyyaw = 0.0f; //not initialized?

	if (eye_goalfeet_delta < 0.0f || animstate->m_flMaxYaw == 0.0f) {
		if (animstate->m_flMinYaw != 0.0f)
			dest.m_bodyyaw = (eye_goalfeet_delta / animstate->m_flMinYaw) * -58.0f;
	}
	else {
		dest.m_bodyyaw = (eye_goalfeet_delta / animstate->m_flMaxYaw) * 58.0f;
	}

	dest.m_absangles = { 0.0f, tm_goalfeetyaw, 0.0f };

	dest.m_ransetupvelocity = true;

	dest.m_curfeetyaw = tm_curfeetyaw;
	dest.m_goalfeetyaw = tm_goalfeetyaw;
	dest.m_last_animation_update_time = time;
	dest.m_next_lby_update_time = tm_next_lby_update_time;
	dest.m_predicted_lowerbodyyaw = tm_predicted_lowerbodyyaw;
	dest.m_duckamount = tm_duckamount;
	dest.m_velocity = tm_velocity;
}
//Keeps a record of a player without using any resolver or animation fix
void CPlayerrecord::FSN_UpdateUnresolvedAnimState()
{
	g_Info.m_PredictionMutex.lock();

	CTickrecord* _previousRecord;
	if (m_flSpawnTime != m_pEntity->GetSpawnTime() || (_previousRecord = GetPreviousRecord(), _previousRecord && _previousRecord->m_Dormant))
	{
		m_fake_curfeetyaw = m_fake_goalfeetyaw = m_fake_duckamount = m_fake_last_animation_update_time = 0.0f;
		m_fake_last_animation_update_time = Interfaces::Globals->curtime - TICK_INTERVAL;
		m_fake_velocity.Init();
	}
	SetUpVelocityResults_t results;
	results.m_curfeetyaw = m_fake_curfeetyaw;
	results.m_goalfeetyaw = m_fake_goalfeetyaw;
	results.m_duckamount = m_fake_duckamount;
	results.m_last_animation_update_time = m_fake_last_animation_update_time;
	results.m_predicted_lowerbodyyaw = m_pEntity->GetLowerBodyYaw();
	results.m_next_lby_update_time = FLT_MAX;
	results.m_use_output_as_input = true;

	GetSetupVelocityResults(m_pEntity->GetEyeAngles(), results, -1.0f, Interfaces::Globals->curtime, m_pEntity->GetAbsVelocityDirect(), m_pEntity->GetDuckAmount(), results.m_predicted_lowerbodyyaw);
	m_fake_curfeetyaw = results.m_curfeetyaw;
	m_fake_goalfeetyaw = results.m_goalfeetyaw;
	m_fake_duckamount = results.m_duckamount;
	m_fake_last_animation_update_time = results.m_last_animation_update_time;
	m_fake_velocity = results.m_velocity;
	fakeabsangles = results.m_absangles;
	fakebodyyaw = results.m_bodyyaw;

	CTickrecord *_currentRecord = GetCurrentRecord();
	if (_currentRecord)
	{
		_currentRecord->m_bCachedFakeAngles = true;//big hack for now. we aren't caching bones but we are doing this to prevent having to rewrite a bunch of other logic
		_currentRecord->m_angAbsAngles_Fake = fakeabsangles;
		_currentRecord->m_flBodyYaw_Fake = fakebodyyaw;
	}

	g_Info.m_PredictionMutex.unlock();
}

//Tries to find the best yaw to get the desired delta
#if 0
float CPlayerrecord::GetChokedYawFromYaw(float sendpacket_yaw, float pitch, float *goalfeet_yaw, float desired_delta)
{
	if (m_iTicksChoked == 0 || m_Tickrecords.size() < 2)
	{
		if (goalfeet_yaw)
			*goalfeet_yaw = m_pInflictedEntity->GetPlayerAnimState()->m_flGoalFeetYaw;
		return sendpacket_yaw;
	}

	CTickrecord *_currentRecord = m_Tickrecords.front();
	CTickrecord *_previousRecord = m_Tickrecords.at(1);

	if (_previousRecord->m_Dormant)
	{
		if (goalfeet_yaw)
			*goalfeet_yaw = m_pInflictedEntity->GetPlayerAnimState()->m_flGoalFeetYaw;
		return sendpacket_yaw;
	}

	QAngle desired_fakeangles = { pitch, sendpacket_yaw, 0.0f };
	QAngle desired_eyeangles = { pitch, sendpacket_yaw, 0.0f };

	SetUpVelocityResults_t results;

	float biggestdelta = 0.0f;
	float bestyaw = sendpacket_yaw;
	float bestgoalfeetyaw = sendpacket_yaw;
	float single_tick_time = TICKS_TO_TIME(1);
	int    _TotalNewCommands = m_iTicksChoked + 1;
	Vector _absVeloSlope = (_currentRecord->m_AbsVelocity - _previousRecord->m_AbsVelocity) / _TotalNewCommands;
	Vector tm_velocity = m_lastvelocity;
	float  _duckAmountSlope = (_currentRecord->m_DuckAmount - _previousRecord->m_DuckAmount) / _TotalNewCommands;
	float tm_next_lby_update_time = m_next_lby_update_time;
	float tm_lower_body_yaw = _currentRecord->m_flLowerBodyYaw;

	if (_currentRecord->m_flLowerBodyYaw != _previousRecord->m_flLowerBodyYaw)
		tm_lower_body_yaw = _previousRecord->m_flLowerBodyYaw;

	for (int add = 15; add <= 180; add += 15)
	{
		results.m_use_output_as_input = false;

		float sim_time = _currentRecord->m_flFirstCmdTickbaseTime;
		Vector _absVelocity = _previousRecord->m_AbsVelocity + _absVeloSlope;
		float _duckAmount = _previousRecord->m_DuckAmount + _duckAmountSlope;

		for (int i = 0; i < m_iTicksChoked; ++i)
		{
			bool lby_timer_updated = false;
			bool moving = false;
			tm_velocity = g_LagCompensation.GetSmoothedVelocity(single_tick_time * 2000.0f, Vector(_absVelocity.x, _absVelocity.y, 0.0f), tm_velocity);
			float m_flSpeed = fminf(tm_velocity.Length(), 260.0f);
			if (m_flSpeed > 0.1f || fabsf(_absVelocity.z) > 100.0f)
			{
				tm_next_lby_update_time = sim_time + 0.22f;
				tm_lower_body_yaw = _currentRecord->m_flLowerBodyYaw;
				lby_timer_updated = true;
				moving = true;
			}
			else if (sim_time > m_next_lby_update_time)
			{
				tm_next_lby_update_time = sim_time + 1.1f;
				tm_lower_body_yaw = _currentRecord->m_flLowerBodyYaw;
				lby_timer_updated = true;
			}

			//FIXME: extrapolate lby while moving
			const bool compensate_lby_flick = false; //NOTE: compensating for lby flick was bad against our aa
			QAngle temp_angles = desired_eyeangles;
			if (_currentRecord->m_bFiredBullet && sim_time >= _currentRecord->m_flLastShotTime)
				temp_angles = desired_fakeangles;
			else if (!moving && lby_timer_updated && (_currentRecord->m_flLowerBodyYaw != _previousRecord->m_flLowerBodyYaw || (compensate_lby_flick && _absVelocity.Length() <= 0.1f)))
				temp_angles.y = tm_lower_body_yaw;

			GetSetupVelocityResults(temp_angles, results, single_tick_time, sim_time, _absVelocity, _duckAmount, tm_lower_body_yaw);
			results.m_use_output_as_input = true;
			sim_time += single_tick_time;
			_absVelocity += _absVeloSlope;
			_duckAmount += _duckAmountSlope;
		}

		float goalfeetyaw = results.m_goalfeetyaw;
		//float realabsyaw = results.m_absangles.y;

		//now update to the desired yaw (sendpacket)
		GetSetupVelocityResults(desired_fakeangles, results, single_tick_time, sim_time, _absVelocity, _duckAmount, tm_lower_body_yaw);

		float delta = fabsf(AngleNormalize(AngleDiff(results.m_absangles.y, fakeabsangles_delayed.y)));
		bool deltamatchesdesired = fabsf(delta - desired_delta) < 5.0f;

		if (delta > biggestdelta /*|| deltamatchesdesired*/)
		{
			biggestdelta = delta;
			bestyaw = desired_eyeangles.y;
			bestgoalfeetyaw = goalfeetyaw;

			if (deltamatchesdesired)
				break;
		}

		int side_add = desired_delta > 0.0f ? add : -add;

		desired_eyeangles.y = AngleNormalize(sendpacket_yaw + (float)side_add);
	}

	if (goalfeet_yaw)
		*goalfeet_yaw = bestgoalfeetyaw;

	return bestyaw;
}
#endif

void CPlayerrecord::FSN_UpdateClientSideAnimation(CTickrecord *_currentRecord, CTickrecord *_previousRecord, CTickrecord *_prePreviousRecord)
{
	bool _respawned = false;

	C_CSGOPlayerAnimState* _animstate = m_pEntity->GetPlayerAnimState();

	// adjust animstate if it's valid
	if (_animstate)
	{
		float _spawnTime = m_pEntity->GetSpawnTime();

		// player spawntime changed
		if (_spawnTime != m_flSpawnTime)
		{
			// reset the animstate.
			// client does this on spawn and then updates the animations on its own, we stopped it from doing so in PreDataUpdate, so do it now
			oResetAnimState(_animstate);

			for (CCSGOPlayerAnimState** i = &m_pAnimStateServer[0]; i != &m_pAnimStateServer[MAX_RESOLVE_SIDES]; ++i)
			{
				CCSGOPlayerAnimState* animstate = *i;
				if (animstate)
				{
					animstate->Reset();
					// set feet cycle to the server's value
					animstate->m_flFeetCycle = DataUpdateVars.m_flPostFeetCycle;
					animstate->m_flFeetWeight = DataUpdateVars.m_flPostFeetWeight;
				}
			}

			// set feet cycle to the server's value
			_animstate->m_flFeetCycle = DataUpdateVars.m_flPostFeetCycle;
			_animstate->m_flFeetWeight = DataUpdateVars.m_flPostFeetWeight;

			// old feet data is invalid if player respawned
			_currentRecord->m_flOldFeetCycle = DataUpdateVars.m_flPostFeetCycle;;
			_currentRecord->m_flOldFeetWeight = DataUpdateVars.m_flPostFeetWeight;

			// set spawntime
			m_flSpawnTime = _spawnTime;

			m_lastvelocity = _animstate->m_vVelocity;

			_respawned = true;
		}

		// mark as shot
		if (_currentRecord->m_bFiredBullet)
		{
			// set flag
			_currentRecord->m_FiringFlags = SHOT;

			// backup angles
			m_angLastShotAngle = _currentRecord->m_EyeAngles;

			// get weapon
			auto weapon = m_pEntity->GetWeapon();

			// get last shot time
			m_flLastShotSimtime = weapon ? weapon->GetLastShotTime() : m_pEntity->GetSimulationTime();
		}

		//if (_currentRecord && _previousRecord && _currentRecord->m_SimulationTime > _previousRecord->m_SimulationTime)
		//{
		//	FSN_UpdateUnresolvedAnimState_TickByTick();
		//}

		//Animate the player into their final state
		FSN_AnimatePlayer(_currentRecord, _previousRecord, _prePreviousRecord);
	}

	// invalidate flags
	int _flags = 0;

	// update angles
	_currentRecord->m_AbsAngles = m_pEntity->GetAbsAnglesDirect();
	_currentRecord->m_LocalAngles = m_pEntity->GetLocalAnglesDirect();

	// store new pose parameters
	m_pEntity->CopyPoseParameters(_currentRecord->m_flPoseParams);

	// overwrite anims with server values
	_flags |= m_pEntity->WriteAnimLayers(DataUpdateVars.m_PostAnimLayers);

	// store anim layers
	m_pEntity->CopyAnimLayers(_currentRecord->m_AnimLayer);

	// invalidate physics
	if (_flags)
		m_pEntity->InvalidatePhysicsRecursive(_flags);

	// store updated animstate
	if (_animstate)
	{
		// backup animstate
		_currentRecord->m_animstate = *_animstate;

		// backup server animstate
		for (int i = 0; i < MAX_RESOLVE_SIDES; ++i)
		{
			if (_currentRecord->m_pAnimStateServer[i] && m_pAnimStateServer[i])
				*_currentRecord->m_pAnimStateServer[i] = *m_pAnimStateServer[i];
		}
		// backup specific animstate values
		m_lastvelocity = m_pAnimStateServer[ResolveSides::NONE]->m_vVelocity;
	}
}

CShotrecord* CPlayerrecord::MarkShot(CTickrecord* _record)
{
	CShotrecord *shot = nullptr;
	_record->m_flLastShotTime = 0.f;

	// player holds a valid weapon
	if (m_pWeapon)
	{
		// get accurate shottime
		const float _lastShotTime = m_pWeapon->GetLastShotTime();

		// player shot
		if (_lastShotTime >= m_flFirstCmdTickbaseTime)//m_pEntity->GetOldSimulationTime() + Interfaces::Globals->interval_per_tick)
		{
			// mark as shot record
			_record->m_bFiredBullet = true;
			shot = new CShotrecord;
			LocalPlayer.ShotMutex.Lock();
			if (shot)
			{
				shot->m_pInflictedEntity = nullptr; //not known yet, set in events
				shot->m_pInflictorEntity = m_pEntity;
				shot->m_Tickrecord = _record;
				shot->m_bDoesNotFoundTowardsStats = true;
				shot->m_bAck = false;
				shot->m_bMissed = false;
				shot->m_bTEImpactEffectAcked = false;
				shot->m_bTEBloodEffectAcked = false;
				shot->m_bImpactEventAcked = false;
				shot->m_bIsInflictor = true;
				shot->m_iTickCountWhenWeShot = TIME_TO_TICKS(_lastShotTime);
				shot->m_flCurtime = _lastShotTime;
				shot->m_iEnemySimulationTickCount = 0;
				shot->m_flRealTime = Interfaces::Globals->realtime;
				shot->m_flRealTimeAck = 0.0f;
				shot->m_flLatency = 0.0f;
				shot->m_vecLocalEyePos = m_pEntity->GetEyePosition(); //FIXME TODO: Fix me: this isn't shoot position and we can't get shoot position since bones aren't setup yet since we are in FSN, move this to CreateMove sometime
				shot->m_bForwardTracked = false;
				shot->m_bUsedBodyHitResolveDelta = false;
				shot->m_iBodyHitResolveStance = 0;
				shot->m_bShotAtBalanceAdjust = false;
				shot->m_bShotAtFreestanding = false;
				shot->m_bInflictorIsLocalPlayer = m_pEntity->IsLocalPlayer();

				ShotRecords.push_back(shot);
			}
			if (ShotRecords.size() > 7)
			{
				delete ShotRecords.front();
				ShotRecords.pop_front();
			}
			LocalPlayer.ShotMutex.Unlock();
		}
		else
		{
			m_angEyeAnglesNotFiring = m_pEntity->GetEyeAngles();
		}

		// set last shot time
		_record->m_flLastShotTime = _lastShotTime;
	}
	else
	{
		m_angEyeAnglesNotFiring = m_pEntity->GetEyeAngles();
	}
	return shot;
}

extern void AirAccelerate(CBaseEntity* player, QAngle &angle, float fmove, float smove);
extern void PlayerMove(CBaseEntity* player);
#include "UsedConvars.h"

void CPlayerrecord::ManualPredict()
{
	float tmpDir = 0.0f;
	float tmpChange = 0.0f;
	static float mod{ 1.f };
	float oldmod = mod;

	// pythagorean theorem
			// a^2 + b^2 = c^2
			// we know a and b, we square them and add them together, then root.
	float hyp = m_pEntity->GetVelocity().Length2D();

	// compute the base velocity for our new direction.
	// since at this point the hypotenuse is known for us and so is the angle.
	// we can compute the adjacent and opposite sides like so:
	// cos(x) = a / h -> a = cos(x) * h
	// sin(x) = o / h -> o = sin(x) * h
	Vector vel = m_pEntity->GetVelocity();
	vel.x = std::cos(DEG2RAD(tmpDir)) * hyp;
	vel.y = std::sin(DEG2RAD(tmpDir)) * hyp;

	// we hit the ground, set the upwards impulse and apply CS:GO speed restrictions.
	if (!(m_pEntity->GetFlags() & FL_ONGROUND))
	{
		// apply one tick of gravity.
		vel.z -= sv_gravity.GetVar()->GetFloat() * TICK_INTERVAL;

		// compute the ideal strafe angle for this velocity.
		float speed2d = vel.Length2D();
		float ideal = (speed2d > 0.f) ? RAD2DEG(std::asin(15.f / speed2d)) : 90.f;
		ideal = clamp(ideal, 0.f, 90.f);

		float smove = 0.f;
		float abschange = fabsf(tmpChange);

		if (abschange <= ideal || abschange >= 30.f) {
			smove = 450.f * oldmod;
		}

		else if (tmpChange > 0.f)
			smove = -450.f;

		else
			smove = 450.f;

		m_pEntity->SetVelocity(vel);

		// apply air accel.
		AirAccelerate(m_pEntity, QAngle(0.f, tmpDir, 0.f), 0.f, smove);
	}

	// predict player.
	// convert newly computed velocity
	// to origin and flags.
	PlayerMove(m_pEntity);

	Vector basevel = m_pEntity->GetBaseVelocity();
	vel = m_pEntity->GetVelocity();
	Vector origin = m_pEntity->GetNetworkOrigin();
	m_pEntity->SetAbsVelocityDirect(vel);
	m_pEntity->SetAbsOriginDirect(origin);
	m_pEntity->SetEFlags(m_pEntity->GetEFlags() & ~(EFL_DIRTY_ABSTRANSFORM | EFL_DIRTY_ABSVELOCITY));
}

int CPlayerrecord::GetButtons(FarESPPlayer* record, FarESPPlayer* previousrecord)
{
	float duckamount = record->duckamount;
	float prevduckamount = previousrecord->duckamount;
	int buttons = 0;

	if (record->fakeducking)
		buttons |= IN_BULLRUSH;

	if ((!(record->flags & FL_ONGROUND) && previousrecord->flags & FL_ONGROUND)
		|| record->flags & FL_WATERJUMP
		|| (!m_bLegit && m_iTicksChoked > 4 && !(previousrecord->flags & FL_ONGROUND))
		)
		buttons |= IN_JUMP;

	if ((duckamount > 0.0f && duckamount >= prevduckamount)
		|| record->flags & FL_DUCKING || record->flags & FL_ANIMDUCKING)
		buttons |= IN_DUCK;

	if (record->movestate == 1)
		buttons |= IN_SPEED;

	//Are they actually pressing movement buttons
	bool _IsAccelerating = true;

	Vector new_velocity_local = record->velocity /*- _currentRecord->m_BaseVelocity*/;
	Vector old_velocity_local = previousrecord->velocity /*- _previousRecord->m_BaseVelocity*/;
	float spd = new_velocity_local.Length2D();
	float oldspd = old_velocity_local.Length2D();

	if (spd <= MINIMUM_PLAYERSPEED_CONSIDERED_MOVING || (spd < oldspd && fabsf(spd - oldspd) > 0.1f))
		_IsAccelerating = false;

	if (_IsAccelerating)
	{
		buttons |= IN_FORWARD;
	}

	return buttons;
}

void CPlayerrecord::SimulatePlayer_FarESP(FarESPPlayer *_currentRecord, FarESPPlayer *_previousRecord, int time, bool SimulateInPlace, bool CalledfromCreateMove, bool useDirection, float &angDirection, float &angChange)
{
	START_PROFILING
	CUserCmd newcmd;
	CMoveData newdata;
	memset(&newdata, 0, sizeof(CMoveData));
	memset(&newcmd, 0, sizeof(CUserCmd));
	newcmd.viewangles = QAngle(_currentRecord->eyepitch, _currentRecord->eyeyaw, 0.0f);
	newcmd.tick_count = time;
	newcmd.command_number = time;
	bool IsMinWalking = false;

	g_Info.m_PredictionMutex.lock();
	static float mod{ 1.f };
	float oldmod = mod;

	if (!(m_pEntity->GetFlags() & FL_ONGROUND))
	{
		Vector vel = m_pEntity->GetVelocity();
		float speed2d = vel.Length2D();
		float ideal = (speed2d > 0.f) ? RAD2DEG(std::asin(15.f / speed2d)) : 90.f;
		ideal = clamp(ideal, 0.f, 90.f);

		float abschange = fabsf(angChange);

		if (abschange <= ideal || abschange >= 30.f) {

			angDirection += (ideal * mod);
			mod *= -1.f;
		}
	}
	g_Info.m_PredictionMutex.unlock();

	float duckamount = _currentRecord->duckamount;
	float prevduckamount = _previousRecord->duckamount;

	if (_currentRecord->fakeducking)
		newcmd.buttons |= IN_BULLRUSH;

	//TODO: FIX OLD BUTTONS

	if (!SimulateInPlace)
	{
		if ((!(_currentRecord->flags & FL_ONGROUND) && _previousRecord->flags & FL_ONGROUND)
			|| _currentRecord->flags & FL_WATERJUMP
			|| (!m_bLegit && m_iTicksChoked > 4 && !(_previousRecord->flags & FL_ONGROUND))
			)
			newcmd.buttons |= IN_JUMP;

		if ((duckamount > 0.0f && duckamount >= prevduckamount)
			|| _currentRecord->flags & FL_DUCKING || _currentRecord->flags & FL_ANIMDUCKING)
			newcmd.buttons |= IN_DUCK;

		if (_currentRecord->movestate == 1)
			newcmd.buttons |= IN_SPEED;

		//Are they actually pressing movement buttons
		bool _IsAccelerating = true;

		Vector new_velocity_local = _currentRecord->velocity /*- _currentRecord->m_BaseVelocity*/;
		Vector old_velocity_local = _previousRecord->velocity /*- _previousRecord->m_BaseVelocity*/;
		float spd = new_velocity_local.Length2D();
		float oldspd = old_velocity_local.Length2D();

		if (spd <= MINIMUM_PLAYERSPEED_CONSIDERED_MOVING || (spd < oldspd && fabsf(spd - oldspd) > 0.1f))
			_IsAccelerating = false;

		if (_IsAccelerating)
		{
			if (useDirection)
			{
				newcmd.viewangles.y = angDirection;
			}
			else
			{
				//Get forward velocity direction
				VectorAngles(new_velocity_local, newcmd.viewangles);
				newcmd.viewangles.x = newcmd.viewangles.z = 0.0f;
			}
			newcmd.buttons |= IN_FORWARD;
			newcmd.forwardmove = 450.0f;
		}

		if (_currentRecord->flags & FL_ONGROUND && spd > 1.0f && spd <= 75.0f && fabsf(spd - oldspd) < 4.0f && !(newcmd.buttons & IN_DUCK) && !(newcmd.buttons & IN_SPEED))
		{
			IsMinWalking = true;
		}
	}

	if (!IsMinWalking)
	{
		g_Info.m_PredictionMutex.lock();
		//CCSGameMovement *originalgamemovement = new CCSGameMovement(*(CCSGameMovement*)Interfaces::GameMovement);
		bool inprediction = Interfaces::Prediction->m_bInPrediction;
		float frametime = Interfaces::Globals->frametime;
		float curtime = Interfaces::Globals->curtime;
		float tickinterval = Interfaces::Globals->interval_per_tick;
		CBaseEntity* host = (*Interfaces::MoveHelperClient)->m_pHost;
		CBaseEntity** pPredictionPlayer = StaticOffsets.GetOffsetValueByType<CBaseEntity * *>(_predictionplayer);
		CBaseEntity* pOriginalPlayer = *pPredictionPlayer;
		CUserCmd originalplayercommand = *m_pEntity->m_PlayerCommand();
		CUserCmd* currentcommand = *m_pEntity->m_pCurrentCommand();
		DWORD originalrandomseed = *m_pPredictionRandomSeed;


		Interfaces::Prediction->m_bInPrediction = true;
		(*Interfaces::MoveHelperClient)->SetHost((CBasePlayer*)m_pEntity);
		*m_pPredictionRandomSeed = MD5_PseudoRandom(CurrentUserCmd.cmd->command_number) & 0x7fffffff;
		*m_pEntity->m_PlayerCommand() = newcmd;
		*m_pEntity->m_pCurrentCommand() = &newcmd;
		*pPredictionPlayer = m_pEntity;
		Interfaces::Globals->frametime = Interfaces::Prediction->m_bEnginePaused ? 0.0f : tickinterval;
		Interfaces::Globals->curtime = TICKS_TO_TIME(time);

		Interfaces::GameMovement->StartTrackPredictionErrors((CBasePlayer*)m_pEntity);
		m_pEntity->UpdateButtonState(newcmd.buttons);
		Interfaces::Prediction->SetupMove((C_BasePlayer*)m_pEntity, &newcmd, (*Interfaces::MoveHelperClient), &newdata);

		if (m_pEntity->ToPlayerRecord()->m_bAllowAnimationEvents)
			m_pEntity->SetIsCustomPlayer(false);

		Interfaces::GameMovement->ProcessMovement((CBasePlayer*)m_pEntity, &newdata);

		m_pEntity->SetIsCustomPlayer(true);
		Interfaces::Prediction->FinishMove((C_BasePlayer*)m_pEntity, &newcmd, &newdata);

#if 0
		g_pGameMovement->StartTrackPredictionErrors((CBasePlayer*)m_pEntity);
		m_pEntity->UpdateButtonState(newcmd.buttons);
		Interfaces::Prediction->SetupMove((C_BasePlayer*)m_pEntity, &newcmd, (*Interfaces::MoveHelperClient), &newdata);
		g_pGameMovement->SetupMovementBounds(&newdata); //Required because SetupMove is setting up the game's g_pGameMovement and not ours
		g_pGameMovement->ProcessMovement(m_pEntity, &newdata);
		Interfaces::Prediction->FinishMove((C_BasePlayer*)m_pEntity, &newcmd, &newdata);
		g_pGameMovement->FinishTrackPredictionErrors(m_pEntity);
		(*Interfaces::MoveHelperClient)->SetHost(nullptr);
#endif

		Vector basevel = m_pEntity->GetBaseVelocity();
		Vector vel = m_pEntity->GetVelocity();
		Vector origin = m_pEntity->GetNetworkOrigin();
		m_pEntity->SetAbsVelocityDirect(vel);
		m_pEntity->SetAbsOriginDirect(origin);
		m_pEntity->SetEFlags(m_pEntity->GetEFlags() & ~(EFL_DIRTY_ABSTRANSFORM | EFL_DIRTY_ABSVELOCITY));

		Interfaces::GameMovement->FinishTrackPredictionErrors((CBasePlayer*)m_pEntity);

		(*Interfaces::MoveHelperClient)->SetHost(host);

		CCSGOPlayerAnimState* animstate = m_pAnimStateServer[ResolveSides::NONE];
		if (animstate)
		{
			QAngle eyeangles = { _currentRecord->eyepitch, _currentRecord->eyeyaw, 0.0f };
			m_pEntity->SetEyeAngles(eyeangles);
			m_pEntity->EyeAngles()->x = eyeangles.x;
			m_pEntity->EyeAngles()->y = eyeangles.y;
#ifdef NO_PARALLEL_NETCODE
			float bk = Interfaces::Globals->curtime;
			Interfaces::Globals->curtime = TICKS_TO_TIME(time);
#endif
			m_pEntity->SetIsStrafing(_currentRecord->strafing);
			m_pEntity->UpdateServerSideAnimation(ResolveSides::NONE, Interfaces::Globals->curtime);
#ifdef NO_PARALLEL_NETCODE
			Interfaces::Globals->curtime = bk;
#endif
			m_pEntity->SetPoseParameter(12, clamp(_currentRecord->eyepitch, -90.0f, 90.0f));
			if (animstate->m_flSpeed > 0.1f || fabsf(vel.z) > 100.0f)
				m_pEntity->SetLowerBodyYaw(AngleNormalize(_currentRecord->eyeyaw));
		}

		*m_pEntity->m_PlayerCommand() = originalplayercommand;
		*m_pEntity->m_pCurrentCommand() = currentcommand;
		Interfaces::Globals->curtime = curtime;
		Interfaces::Globals->frametime = frametime;
		*pPredictionPlayer = pOriginalPlayer;
		Interfaces::Prediction->m_bInPrediction = inprediction;
		*m_pPredictionRandomSeed = originalrandomseed;
#if 0
		CCSGameMovement* gamemovement = (CCSGameMovement*)Interfaces::GameMovement;
		gamemovement->player = originalgamemovement->player;
		gamemovement->m_pCSPlayer = originalgamemovement->m_pCSPlayer;
		gamemovement->m_nOldWaterLevel = originalgamemovement->m_nOldWaterLevel;
		gamemovement->m_flWaterEntryTime = originalgamemovement->m_flWaterEntryTime;
		gamemovement->m_nOnLadder = originalgamemovement->m_nOnLadder;
		gamemovement->m_vecForward = originalgamemovement->m_vecForward;
		gamemovement->m_vecRight = originalgamemovement->m_vecRight;
		gamemovement->m_vecUp = originalgamemovement->m_vecUp;
		gamemovement->m_bSpeedCropped = originalgamemovement->m_bSpeedCropped;
		gamemovement->m_bProcessingMovement = originalgamemovement->m_bProcessingMovement;
		gamemovement->m_bInStuckTest = originalgamemovement->m_bInStuckTest;
		delete originalgamemovement;
#endif
		g_Info.m_PredictionMutex.unlock();
	}
	else
	{
		//minwalking, manual prediction

		// pythagorean theorem
			// a^2 + b^2 = c^2
			// we know a and b, we square them and add them together, then root.
		float hyp = m_pEntity->GetVelocity().Length2D();

		// compute the base velocity for our new direction.
		// since at this point the hypotenuse is known for us and so is the angle.
		// we can compute the adjacent and opposite sides like so:
		// cos(x) = a / h -> a = cos(x) * h
		// sin(x) = o / h -> o = sin(x) * h
		Vector vel = m_pEntity->GetVelocity();
		vel.x = std::cos(DEG2RAD(angDirection)) * hyp;
		vel.y = std::sin(DEG2RAD(angDirection)) * hyp;

		// we hit the ground, set the upwards impulse and apply CS:GO speed restrictions.
		if (newcmd.buttons & IN_JUMP && m_pEntity->GetFlags() & FL_ONGROUND) {

			//Start the jumping animation
			if (m_pEntity->ToPlayerRecord()->m_bAllowAnimationEvents)
				m_pEntity->DoAnimationEvent((PlayerAnimEvent_t)8, 0);

			if (!sv_enablebunnyhopping.GetVar()->GetInt()) {

				// 260 x 1.1 = 286 units/s.
				float max = m_pEntity->GetMaxSpeed() * 1.1f;

				// get current velocity.
				float speed = vel.Length();

				// reset velocity to 286 units/s.
				if (max > 0.f && speed > max)
					vel *= (max / speed);
			}

			// assume the player is bunnyhopping here so set the upwards impulse.
			vel.z = sv_jump_impulse.GetVar()->GetFloat();

			m_pEntity->SetVelocity(vel);
		}

		// we are not on the ground
		// apply gravity and airaccel.
		else if (!m_pEntity->HasFlag(FL_ONGROUND)){
			// apply one tick of gravity.
			vel.z -= sv_gravity.GetVar()->GetFloat() * TICK_INTERVAL;

			// compute the ideal strafe angle for this velocity.
			float speed2d = vel.Length2D();
			float ideal = (speed2d > 0.f) ? RAD2DEG(std::asin(15.f / speed2d)) : 90.f;
			ideal = clamp(ideal, 0.f, 90.f);

			float smove = 0.f;
			float abschange = fabsf(angChange);

			if (abschange <= ideal || abschange >= 30.f) {
				smove = 450.f * oldmod;
			}

			else if (angChange > 0.f)
				smove = -450.f;

			else
				smove = 450.f;

			m_pEntity->SetVelocity(vel);

			// apply air accel.
			AirAccelerate(m_pEntity, QAngle(0.f, angDirection, 0.f), 0.f, smove);
		}

		// predict player.
		// convert newly computed velocity
		// to origin and flags.
		PlayerMove(m_pEntity);

		Vector basevel = m_pEntity->GetBaseVelocity();
		vel = m_pEntity->GetVelocity();
		Vector origin = m_pEntity->GetNetworkOrigin();
		m_pEntity->SetAbsVelocityDirect(vel);
		m_pEntity->SetAbsOriginDirect(origin);
		m_pEntity->SetEFlags(m_pEntity->GetEFlags() & ~(EFL_DIRTY_ABSTRANSFORM | EFL_DIRTY_ABSVELOCITY));

		CCSGOPlayerAnimState *animstate = m_pAnimStateServer[ResolveSides::NONE];
		if (animstate)
		{
			QAngle eyeangles = { _currentRecord->eyepitch, _currentRecord->eyeyaw, 0.0f };
			m_pEntity->SetEyeAngles(eyeangles);
			m_pEntity->EyeAngles()->x = eyeangles.x;
			m_pEntity->EyeAngles()->y = eyeangles.y;
#ifdef NO_PARALLEL_NETCODE
			float curtime = Interfaces::Globals->curtime;
			Interfaces::Globals->curtime = TICKS_TO_TIME(time);
#endif
			m_pEntity->SetIsStrafing(_currentRecord->strafing);
			m_pEntity->UpdateServerSideAnimation(ResolveSides::NONE, TICKS_TO_TIME(time));
#ifdef NO_PARALLEL_NETCODE
			Interfaces::Globals->curtime = curtime;
#endif
			if (animstate->m_flSpeed > 0.1f || fabsf(vel.z) > 100.0f)
				m_pEntity->SetLowerBodyYaw(AngleNormalize(eyeangles.y));
		}
	}
	END_PROFILING
}

void CPlayerrecord::SimulatePlayer(CTickrecord *_currentRecord, CTickrecord *_previousRecord, int time, bool SimulateInPlace, bool CalledfromCreateMove, bool animate, bool isfinaltick, ResolveSides resolvemode, float pitch, float yaw, float *angDirection, float *angChange)
{
	START_PROFILING
	CUserCmd newcmd;
	CMoveData newdata;
	memset(&newdata, 0, sizeof(CMoveData));
	memset(&newcmd, 0, sizeof(CUserCmd));
	newcmd.viewangles = _currentRecord->m_EyeAngles;
	newcmd.tick_count = time;
	newcmd.command_number = time;
	bool IsMinWalking = false;
	float tmpDir = 0.0f;
	float tmpChange = 0.0f;
	if (angDirection)
		tmpDir = *angDirection;
	if (angChange)
		tmpChange = *angChange;

	g_Info.m_PredictionMutex.lock();
	static float mod{ 1.f };
	float oldmod = mod;

	if (angDirection && angChange && !(m_pEntity->GetFlags() & FL_ONGROUND))
	{
		Vector vel = m_pEntity->GetVelocity();
		float speed2d = vel.Length2D();
		float ideal = (speed2d > 0.f) ? RAD2DEG(std::asin(15.f / speed2d)) : 90.f;
		ideal = clamp(ideal, 0.f, 90.f);

		float abschange = fabsf(tmpChange);

		if (abschange <= ideal || abschange >= 30.f) {

			tmpDir += (ideal * mod);
			mod *= -1.f;
		}
	}
	g_Info.m_PredictionMutex.unlock();

	if (!SimulateInPlace)
	{
		if ((!(_currentRecord->m_Flags & FL_ONGROUND) && _previousRecord->m_Flags & FL_ONGROUND)
			|| _currentRecord->m_Flags & FL_WATERJUMP
			|| (!m_bLegit && m_iTicksChoked > 4 && !(_previousRecord->m_Flags & FL_ONGROUND))
			)
			newcmd.buttons |= IN_JUMP;

		if ((_currentRecord->m_DuckAmount > 0.0f && _currentRecord->m_DuckAmount >= _previousRecord->m_DuckAmount)
			|| _currentRecord->m_Flags & FL_DUCKING || _currentRecord->m_Flags & FL_ANIMDUCKING)
			newcmd.buttons |= IN_DUCK;

		if (_currentRecord->m_PlayerBackup.MoveState == 1)
			newcmd.buttons |= IN_SPEED;

		//Are they actually pressing movement buttons
		bool _IsAccelerating = true;

		Vector new_velocity_local = _currentRecord->m_AbsVelocity - _currentRecord->m_BaseVelocity;
		Vector old_velocity_local = _previousRecord->m_AbsVelocity - _previousRecord->m_BaseVelocity;
		float spd = new_velocity_local.Length2D();
		float oldspd = old_velocity_local.Length2D();

		if (spd <= MINIMUM_PLAYERSPEED_CONSIDERED_MOVING || (spd < oldspd && fabsf(spd - oldspd) > 0.1f))
			_IsAccelerating = false;

		if (_IsAccelerating)
		{
			if (angDirection)
			{
				newcmd.viewangles.y = tmpDir;
			}
			else
			{
				//Get forward velocity direction
				VectorAngles(new_velocity_local, newcmd.viewangles);
				newcmd.viewangles.x = newcmd.viewangles.z = 0.0f;
				tmpDir = newcmd.viewangles.y;
			}
			newcmd.buttons |= IN_FORWARD;
			newcmd.forwardmove = 450.0f;
		}

		if (_currentRecord->m_Flags & FL_ONGROUND && spd > 1.0f && spd <= 75.0f && fabsf(spd - oldspd) < 4.0f && !(newcmd.buttons & IN_DUCK) && !(newcmd.buttons & IN_SPEED))
		{
			IsMinWalking = true;
		}
	}

	if (!IsMinWalking)
	{
		g_Info.m_PredictionMutex.lock();
		bool inpred = Interfaces::Prediction->m_bInPrediction;
		Interfaces::Prediction->m_bInPrediction = true;
		(*Interfaces::MoveHelperClient)->SetHost((CBasePlayer*)m_pEntity);
		CBaseEntity** pPredictionPlayer = StaticOffsets.GetOffsetValueByType<CBaseEntity * *>(_predictionplayer);
		CBaseEntity* pOriginalPlayer = *pPredictionPlayer;
		CUserCmd originalplayercommand = *m_pEntity->m_PlayerCommand();
		*m_pEntity->m_PlayerCommand() = newcmd;
		*pPredictionPlayer = m_pEntity;
		DWORD rand = MD5_PseudoRandom(CurrentUserCmd.cmd->command_number) & 0x7fffffff;
		DWORD originalrandomseed = *m_pPredictionRandomSeed;
		*m_pPredictionRandomSeed = rand;
		float frametime = Interfaces::Globals->frametime;
		float curtime = Interfaces::Globals->curtime;
		float tickinterval = Interfaces::Globals->interval_per_tick;
		Interfaces::Globals->frametime = Interfaces::Prediction->m_bEnginePaused ? 0.0f : tickinterval;
		Interfaces::Globals->curtime = TICKS_TO_TIME(time);
		CUserCmd* currentcommand = *m_pEntity->m_pCurrentCommand();
		*m_pEntity->m_pCurrentCommand() = &newcmd;

		Interfaces::GameMovement->StartTrackPredictionErrors((CBasePlayer*)m_pEntity);

		m_pEntity->UpdateButtonState(newcmd.buttons);
		Interfaces::Prediction->SetupMove((C_BasePlayer*)m_pEntity, &newcmd, (*Interfaces::MoveHelperClient), &newdata);
		if (m_pEntity->ToPlayerRecord()->m_bAllowAnimationEvents)
			m_pEntity->SetIsCustomPlayer(false);
		Interfaces::GameMovement->ProcessMovement((CBasePlayer*)m_pEntity, &newdata);
		m_pEntity->SetIsCustomPlayer(true);
		Interfaces::Prediction->FinishMove((C_BasePlayer*)m_pEntity, &newcmd, &newdata);

		Vector basevel = m_pEntity->GetBaseVelocity();
		Vector vel = m_pEntity->GetVelocity();
		Vector origin = m_pEntity->GetNetworkOrigin();
		m_pEntity->SetAbsVelocityDirect(vel);
		m_pEntity->SetAbsOriginDirect(origin);
		m_pEntity->SetEFlags(m_pEntity->GetEFlags() & ~(EFL_DIRTY_ABSTRANSFORM | EFL_DIRTY_ABSVELOCITY));

		Interfaces::GameMovement->FinishTrackPredictionErrors((CBasePlayer*)m_pEntity);

		if (animate)
		{
			CCSGOPlayerAnimState* animstate = m_pAnimStateServer[resolvemode];
			if (animstate)
			{
				QAngle eyeangles = { pitch, yaw, 0.0f };
				m_pEntity->SetEyeAngles(eyeangles);
				m_pEntity->EyeAngles()->x = pitch;
				m_pEntity->EyeAngles()->y = yaw;
				m_pEntity->UpdateServerSideAnimation(resolvemode, Interfaces::Globals->curtime);
				m_pEntity->SetPoseParameter(12, clamp(pitch, -90.0f, 90.0f));
				if (animstate->m_flSpeed > 0.1f || fabsf(vel.z) > 100.0f)
					m_pEntity->SetLowerBodyYaw(AngleNormalize(yaw));
			}
		}

		//if (CalledfromCreateMove || m_pEntity->IsLocalPlayer())
		*m_pEntity->m_PlayerCommand() = originalplayercommand;
		*m_pEntity->m_pCurrentCommand() = currentcommand;
		Interfaces::Globals->curtime = curtime;
		Interfaces::Globals->frametime = frametime;
		*pPredictionPlayer = pOriginalPlayer;
		(*Interfaces::MoveHelperClient)->SetHost(!CalledfromCreateMove ? nullptr : LocalPlayer.Entity);
		Interfaces::Prediction->m_bInPrediction = inpred;
		*m_pPredictionRandomSeed = originalrandomseed;
		g_Info.m_PredictionMutex.unlock();
	}
	else
	{
		//minwalking, manual prediction


		// pythagorean theorem
			// a^2 + b^2 = c^2
			// we know a and b, we square them and add them together, then root.
		float hyp = m_pEntity->GetVelocity().Length2D(	);

		// compute the base velocity for our new direction.
		// since at this point the hypotenuse is known for us and so is the angle.
		// we can compute the adjacent and opposite sides like so:
		// cos(x) = a / h -> a = cos(x) * h
		// sin(x) = o / h -> o = sin(x) * h
		Vector vel = m_pEntity->GetVelocity();
		vel.x = std::cos(DEG2RAD(tmpDir)) * hyp;
		vel.y = std::sin(DEG2RAD(tmpDir)) * hyp;

		// we hit the ground, set the upwards impulse and apply CS:GO speed restrictions.
		if (newcmd.buttons & IN_JUMP && m_pEntity->GetFlags() & FL_ONGROUND) {

			//Start the jumping animation
			if (m_pEntity->ToPlayerRecord()->m_bAllowAnimationEvents)
				m_pEntity->DoAnimationEvent((PlayerAnimEvent_t)8, 0);

			if (!sv_enablebunnyhopping.GetVar()->GetInt()) {

				// 260 x 1.1 = 286 units/s.
				float max = m_pEntity->GetMaxSpeed() * 1.1f;

				// get current velocity.
				float speed = vel.Length();

				// reset velocity to 286 units/s.
				if (max > 0.f && speed > max)
					vel *= (max / speed);
			}

			// assume the player is bunnyhopping here so set the upwards impulse.
			vel.z = sv_jump_impulse.GetVar()->GetFloat();

			m_pEntity->SetVelocity(vel);
		}

		// we are not on the ground
		// apply gravity and airaccel.
		else {
			// apply one tick of gravity.
			vel.z -= sv_gravity.GetVar()->GetFloat() * TICK_INTERVAL;

			// compute the ideal strafe angle for this velocity.
			float speed2d = vel.Length2D();
			float ideal = (speed2d > 0.f) ? RAD2DEG(std::asin(15.f / speed2d)) : 90.f;
			ideal = clamp(ideal, 0.f, 90.f);

			float smove = 0.f;
			float abschange = fabsf(tmpChange);

			if (abschange <= ideal || abschange >= 30.f) {
				smove = 450.f * oldmod;
			}

			else if (tmpChange > 0.f)
				smove = -450.f;

			else
				smove = 450.f;

			m_pEntity->SetVelocity(vel);

			// apply air accel.
			AirAccelerate(m_pEntity, QAngle( 0.f, tmpDir, 0.f ), 0.f, smove);
		}

		// predict player.
		// convert newly computed velocity
		// to origin and flags.
		PlayerMove(m_pEntity);

		Vector basevel = m_pEntity->GetBaseVelocity();
		vel = m_pEntity->GetVelocity();
		Vector origin = m_pEntity->GetNetworkOrigin();
		m_pEntity->SetAbsVelocityDirect(vel);
		m_pEntity->SetAbsOriginDirect(origin);
		m_pEntity->SetEFlags(m_pEntity->GetEFlags() & ~(EFL_DIRTY_ABSTRANSFORM | EFL_DIRTY_ABSVELOCITY));

		if (animate)
		{
			CCSGOPlayerAnimState *animstate = m_pAnimStateServer[resolvemode];
			if (animstate)
			{
				QAngle eyeangles = { pitch, yaw, 0.0f };
				m_pEntity->SetEyeAngles(eyeangles);
				m_pEntity->EyeAngles()->x = pitch;
				m_pEntity->EyeAngles()->y = yaw;
#ifdef NO_PARALLEL_NETCODE
				float curtime = Interfaces::Globals->curtime;
				Interfaces::Globals->curtime = TICKS_TO_TIME(time);
#endif
				m_pEntity->UpdateServerSideAnimation(resolvemode, TICKS_TO_TIME(time));
#ifdef NO_PARALLEL_NETCODE
				Interfaces::Globals->curtime = curtime;
#endif
				if (animstate->m_flSpeed > 0.1f || fabsf(vel.z) > 100.0f)
					m_pEntity->SetLowerBodyYaw(AngleNormalize(yaw));
			}
		}
	}
	if (angDirection)
		*angDirection = tmpDir;
	END_PROFILING
}

void CPlayerrecord::FSN_UpdatePlayer()
{
	// invalid player
	if (!m_pEntity)
	{
		// reset values for next tick
		m_bNetUpdate = false;
		m_bNetUpdateSilent = false;
		Changed.Reset();
		m_bResolved = true;
		return;
	}

	CBaseCombatWeapon *_weapon = m_pEntity->GetWeapon();
	WeaponInfo_t *data;
	m_bHasWeaponInfo = false;
	if (_weapon && (data = _weapon->GetCSWpnData(), data))
	{
		m_WeaponInfo = *data;
		m_bHasWeaponInfo = true;
	}

	// get current simtime
	float _simtime = m_pEntity->GetSimulationTime();
	float _oldsimtime = m_pEntity->GetOldSimulationTime();
	if (m_pEntity->GetSpawnTime() != m_flSpawnTime)
	{
		m_pEntity->SetvphysicsCollisionState(m_ivphysicsCollisionState);
	}

	// do localplayer stuff
	if (m_bLocalPlayer)
	{
		// we received a netvar update
		if (m_bNetUpdate)
		{
			// did the simtime update? update localplayer info
			if (_simtime != LocalPlayer.flSimulationTime)
			{
				// get new origin
				const Vector _origin = m_pEntity->GetNetworkOrigin();

				LocalPlayer.LowerBodyYaw = m_pEntity->GetLowerBodyYaw();
				LocalPlayer.Previous_Origin = LocalPlayer.Current_Origin;
				m_bTeleporting = (_origin - LocalPlayer.Current_Origin).LengthSqr() > (64.0f * 64.0f);
				LocalPlayer.Current_Origin = _origin;
				LocalPlayer.flSimulationTime = _simtime;
				m_flNewestSimulationTime = _simtime;
			}

			// reset values for next tick
			m_bNetUpdate = false;
			m_bNetUpdateSilent = false;
			Changed.Reset();
		}

		// don't do player stuff afterwards
		return;
	}

	// check if simtime/entity is valid
	if (m_bDormant || !m_pEntity->GetAlive())
	{
		if (m_bNetUpdate)
		{
			m_bNetUpdate = false;
			m_bNetUpdateSilent = false;
			Changed.Reset();
		}

		m_bResolved = true;
		return;
	}

	CTickrecord *_prePreviousRecord = nullptr, *_previousRecord = nullptr, *_currentRecord = nullptr;

	if (!m_Tickrecords.empty())
	{
		_previousRecord = m_Tickrecords[0];
		if (_previousRecord->m_Dormant)
		{
			_previousRecord = nullptr;
		}
		else if (m_Tickrecords.size() > 1)
		{
			_prePreviousRecord = m_Tickrecords[1];

			if (_prePreviousRecord->m_Dormant)
				_prePreviousRecord = nullptr;
		}
	}

	// something is overriding the player animations, this is a dirty fix
	bool _overrideAnimations = true;
	bool _receivedGarbageRecord = false;
	bool _wasDormant = true;

	if (_previousRecord && !_previousRecord->m_Dormant && m_flSpawnTime == m_pEntity->GetSpawnTime())
	{
		_wasDormant = false;
		if (Changed.SimulationTime)
		{
			if (!Changed.Animations/* && !Changed.Origin*/)
			{
				//Server sent a garbage record, restore to the original values
				_simtime = _previousRecord->m_SimulationTime;
				_oldsimtime = _prePreviousRecord ? _prePreviousRecord->m_SimulationTime : 0.0f;
				m_pEntity->SetSimulationTime(_simtime);
				m_pEntity->SetOldSimulationTime(_oldsimtime);
				_receivedGarbageRecord = true;
			}
		}
	}

	// create new tickrecord if needed
	if (!_receivedGarbageRecord)
	{
		_currentRecord = CreateNewTickrecord(_previousRecord);

		//Update the fake player animstate (so we can see what a legit player sees)
		//FIXME: Move this to be on a per-frame basis instead of per-tick basis like an uninjected csgo
		FSN_UpdateUnresolvedAnimState();

		if (_currentRecord != _previousRecord)
		{
			FSN_UpdateClientSideAnimation(_currentRecord, _previousRecord, _prePreviousRecord);
			_overrideAnimations = false;

#ifdef _DEBUG
			if (m_pEntity != LocalPlayer.Entity)
				OutputAnimations();
#endif

			_currentRecord->m_PlayerBackup.Get(m_pEntity);

			int count = 0;
			for (auto& tick : m_Tickrecords)
			{
				if (tick->IsValid(m_pEntity))
					++count;
			}
			m_ValidRecordCount = count;

			if (m_ValidRecordCount > 0)
				m_flTotalInvalidRecordTime = 0.0f;
			else
				m_flTotalInvalidRecordTime += TICKS_TO_TIME(m_iTicksChoked + 1);

			if (m_bTeleporting || variable::get().visuals.pf_enemy.b_smooth_animation || m_ValidRecordCount == 0)
			{
				if (!_wasDormant && m_pEntity->IsEnemy(Interfaces::ClientEntList->GetBaseEntity(Interfaces::EngineClient->GetLocalPlayer())))
				{
					//simulate player movement to retrieve correct game movement values
					float lby = m_pEntity->GetLowerBodyYaw();
					_previousRecord->m_PlayerBackup.RestoreData(false, false);
					int time = TIME_TO_TICKS(_currentRecord->m_flFirstCmdTickbaseTime);
					/*
					int mode = 0;
					switch (_currentRecord->m_iResolveSide_WhenChoked)
					{
						case ResolveSides::NEGATIVE_120:
						case ResolveSides::NEGATIVE_90:
							mode = -1;
							break;
						case ResolveSides::POSITIVE_120:
						case ResolveSides::POSITIVE_90:
							mode = 1;
							break;
					}*/
					for (int i = 0; i < m_iTicksChoked + 1; ++i)
					{
						SimulatePlayer(_currentRecord, _previousRecord, time + i, false, false, false, i + 1 >= m_iTicksChoked + 1, ResolveSides::NONE);
					}
					auto& currentrecord_playerbackup = _currentRecord->m_PlayerBackup;
					QAngle server_aimpunch = currentrecord_playerbackup.LocalData.m_aimPunchAngle_;
					QAngle server_punchvel = currentrecord_playerbackup.LocalData.m_aimPunchAngleVel_;
					QAngle server_viewpunch = currentrecord_playerbackup.LocalData.m_viewPunchAngle_;
					currentrecord_playerbackup.WaterJumpTime = m_pEntity->GetWaterJumpTime();
					currentrecord_playerbackup.WaterJumpVel = m_pEntity->GetWaterJumpVel();
					currentrecord_playerbackup.SurfaceFriction = m_pEntity->GetSurfaceFriction();
					currentrecord_playerbackup.SurfaceData = m_pEntity->GetSurfaceData();
					currentrecord_playerbackup.SurfaceProps = m_pEntity->GetSurfaceProps();
					currentrecord_playerbackup.DuckUntilOnGround = m_pEntity->GetDuckUntilOnGround();
					currentrecord_playerbackup.DuckOverride = m_pEntity->GetDuckOverride();
					currentrecord_playerbackup.InDuckJump = m_pEntity->IsInDuckJump();
					currentrecord_playerbackup.StuckLast = m_pEntity->GetStuckLast();
					currentrecord_playerbackup.WaterJumpVel = m_pEntity->GetWaterJumpVel();
					currentrecord_playerbackup.LocalData = m_pEntity->GetLocalData()->localdata;
					currentrecord_playerbackup.StuckLast = m_pEntity->GetStuckLast();
					currentrecord_playerbackup.DuckingOrigin = m_pEntity->GetDuckingOrigin();
					currentrecord_playerbackup.TimeNotOnLadder = m_pEntity->GetTimeNotOnLadder();
					currentrecord_playerbackup.BaseVelocity = m_pEntity->GetBaseVelocity();
					currentrecord_playerbackup.GameMovementOnGround = m_pEntity->HasWalkMovedSinceLastJump();
					currentrecord_playerbackup.GroundAccelLinearFracLastTime = m_pEntity->GetGroundAccelLinearFracLastTime();
					currentrecord_playerbackup.LocalData.m_aimPunchAngle_ = server_aimpunch;
					currentrecord_playerbackup.LocalData.m_aimPunchAngleVel_ = server_punchvel;
					currentrecord_playerbackup.LocalData.m_viewPunchAngle_ = server_viewpunch;
					currentrecord_playerbackup.RestoreData(true, false);
					if (m_pEntity->GetLowerBodyYaw() == 0.0f && lby != 0.0f)
						int fuckcsgo = 1;
				}
			}
		}
		else
		{
			int count = 0;
			for (auto& tick : m_Tickrecords)
			{
				if (tick->IsValid(m_pEntity))
					++count;
			}
			m_ValidRecordCount = count;

			if (m_ValidRecordCount > 0)
				m_flTotalInvalidRecordTime = 0.0f;
			else
				m_flTotalInvalidRecordTime += TICKS_TO_TIME(m_iTicksChoked + 1);
		}

		if (_overrideAnimations)
		{
			//stop the game from overwriting our animations!!!
			_currentRecord = GetCurrentRecord();
			if (_currentRecord)
			{
				int flags = m_pEntity->WriteAnimLayers(_currentRecord->m_AnimLayer);
				flags |= m_pEntity->WritePoseParameters(_currentRecord->m_flPoseParams);
				if (flags)
					m_pEntity->InvalidatePhysicsRecursive(flags);
			}
		}

		// remove unneeded tickrecords
		if (g_Info.m_iServerTickrate > 0)
		{
			const size_t tickrate = g_Info.m_iServerTickrate;
			for (size_t i = m_Tickrecords.size(); i > tickrate; i = m_Tickrecords.size())
			{
				delete m_Tickrecords.back();
				m_Tickrecords.pop_back();
			}
		}

		if (_simtime > m_flNewestSimulationTime)
		{
			m_flNewestSimulationTime = _simtime;
			m_iNewestTickbase = TIME_TO_TICKS(m_flNewestSimulationTime) + 1;
		}
	}
	else
	{
		int count = 0;
		for (auto& tick : m_Tickrecords)
		{
			if (tick->IsValid(m_pEntity))
				++count;
		}
		m_ValidRecordCount = count;

		if (m_ValidRecordCount > 0)
			m_flTotalInvalidRecordTime = 0.0f;
		else
			m_flTotalInvalidRecordTime += TICKS_TO_TIME(m_iTicksChoked + 1);
	}

	// reset values for next tick
	m_bNetUpdate = false;
	m_bNetUpdateSilent = false;
	Changed.Reset();
}

void CPlayerrecord::CM_RestoreAnimations(CTickrecord* _target) const
{
	int _flags = 0;

	// set poseparams and adjust flags
	_flags |= m_pEntity->WritePoseParameters(_target->m_flPoseParams);

	// set anim layers and adjust flags
	_flags |= m_pEntity->WriteAnimLayers(_target->m_AnimLayer);

	// call with flags
	m_pEntity->InvalidatePhysicsRecursive(_flags);
}
void CPlayerrecord::CM_RestoreNetvars(CTickrecord* _target) const
{
	m_pEntity->SetFlags(_target->m_Flags);
	m_pEntity->SetMins(_target->m_Mins);
	m_pEntity->SetMaxs(_target->m_Maxs);
	m_pEntity->SetLowerBodyYaw(_target->m_flLowerBodyYaw);
	m_pEntity->SetDuckAmount(_target->m_DuckAmount);
	m_pEntity->SetDuckSpeed(_target->m_DuckSpeed);

	m_pEntity->SetLocalOriginDirect(_target->m_NetOrigin);
	m_pEntity->SetAbsOriginDirect(_target->m_AbsOrigin);

	m_pEntity->SetLocalAnglesDirect(_target->m_LocalAngles);
	m_pEntity->SetAbsAnglesDirect(_target->m_AbsAngles);

	m_pEntity->SetVelocity(_target->m_Velocity);
	m_pEntity->SetAbsVelocityDirect(_target->m_AbsVelocity);
	m_pEntity->SetIsStrafing(_target->m_bStrafing);

	m_pEntity->RemoveEFlag(EFL_DIRTY_ABSTRANSFORM);
	m_pEntity->RemoveEFlag(EFL_DIRTY_ABSVELOCITY);
	
	//NOTE: RESTORING EFLAGS FIXES TRACERAY NOT RETURNING THAT WE HIT THE PLAYER AS WELL AS COLLISION, BUT IS A MAJOR PERFORMANCE DROP, USE UTIL_ClipTraceToPlayers_Fixed AND USE CTraceFilterNoPlayers for TraceRay
	//auto eflags = m_pEntity->GetEFlags();
	//eflags |= (EFL_DIRTY_SURROUNDING_COLLISION_BOUNDS | EFL_DIRTY_SPATIAL_PARTITION);
	//m_pEntity->SetEFlags(eflags);
	//m_pEntity->UpdatePartition();
}
bool CPlayerrecord::CM_BacktrackPlayer(CTickrecord* _target, bool include_bones)
{
	// breaking lagcomp or invalid
	if (/*m_bTeleporting ||*/ !_target || m_TargetRecord == _target)
		return false;

	START_PROFILING

	_target->m_PlayerBackup.RestoreData();

	// restore anims
	//CM_RestoreAnimations(_target);

	// apply feet stuff
	//m_pEntity->SetGoalFeetYaw(_target->m_flGoalFeetYaw);
	//m_pEntity->SetCurrentFeetYaw(_target->m_flCurrentFeetYaw);

	// restore netvars
	//CM_RestoreNetvars(_target);

	// set the tickcount we want to backtrack to
	m_iTickcount = _target->m_iTickcount;

	// save target tick for the rest of the cheat
	m_TargetRecord = _target;

	if (include_bones)
		CacheBones(Interfaces::Globals->curtime, false, _target);

	m_bNeedsRestoring = true;
	END_PROFILING
	return true;
}

bool CPlayerrecord::FarESPPredict()
{
	FarESPPlayer* CurrentRecord = GetFarESPPacket(0);
	FarESPPlayer* PreviousRecord = GetFarESPPacket(1);
	FarESPPlayer* PrePreviousRecord = GetFarESPPacket(2);

	if (!CurrentRecord || !PreviousRecord || !PrePreviousRecord)
		return false;

	bool ducking = m_bIsHoldingDuck || m_bIsFakeDucking;

	// if the enemy is moving and on ground, check around them to see if we can hit there, so we don't simulate and scan tons of crap
	if (CurrentRecord->flags & FL_ONGROUND && CurrentRecord->velocity.Length() >= MINIMUM_PLAYERSPEED_CONSIDERED_MOVING)
	{
		Autowall_Output_t output;
		Vector testposition = m_pEntity->GetAbsOriginDirect() + Vector(0.0f, 0.0f, 64.0f);
		QAngle angletoenemy = CalcAngle(LocalPlayer.ShootPosition, m_pEntity->GetAbsOriginDirect() + Vector(0.0f, 0.0f, 64.0f));
		Vector vRight;
		AngleVectors(angletoenemy, nullptr, &vRight, nullptr);

#ifdef _DEBUG
		Interfaces::DebugOverlay->AddBoxOverlay(testposition + vRight * 35.0f, Vector(-4, -4, -4), Vector(4, 4, 4), angletoenemy, 0, 0, 255, 255, TICKS_TO_TIME(2));
#endif
		Autowall(LocalPlayer.ShootPosition, testposition + vRight * 35.0f, output, false, true, m_pEntity, HITBOX_BODY);

		if (!output.entity_hit)
		{
#ifdef _DEBUG
			Interfaces::DebugOverlay->AddBoxOverlay(testposition - vRight * 35.0f, Vector(-4, -4, -4), Vector(4, 4, 4), angletoenemy, 0, 0, 255, 255, TICKS_TO_TIME(2));
#endif
			Autowall(LocalPlayer.ShootPosition, testposition - vRight * 35.0f, output, false, true, m_pEntity, HITBOX_BODY);
			if (!output.entity_hit)
				return false;
		}
	}

	CCSGOPlayerAnimState *animstate = m_pAnimStateServer[ResolveSides::NONE];
	if (animstate)
	{
		animstate->m_flFeetCycle = CurrentRecord->animlayers[FEET_LAYER]._m_flCycle;
		animstate->m_flFeetWeight = CurrentRecord->animlayers[FEET_LAYER].m_flWeight;
		animstate->m_flLastClientSideAnimationUpdateTime = CurrentRecord->simulationtime;
	}

	float change = 0.f, dir = 0.f;
	bool usedirection = false;

	// get the direction of the current velocity.
	if (CurrentRecord->velocity.y != 0.f || CurrentRecord->velocity.x != 0.f)
	{
		dir = RAD2DEG(atan2(CurrentRecord->velocity.y, CurrentRecord->velocity.x));
		usedirection = true;
	}

	//QAngle angelz;
	//VectorAngles(CurrentRecord->m_Velocity, angelz);

	// we have more than one update
	// we can compute the direction.
	// get the delta time between the 2 most recent records.
	float dt = TICKS_TO_TIME(CurrentRecord->tickschoked + 1);

	// init to 0.
	float prevdir = 0.f;

	// get the direction of the prevoius velocity.
	if (PreviousRecord->velocity.y != 0.f || PreviousRecord->velocity.x != 0.f)
		prevdir = RAD2DEG(std::atan2(PreviousRecord->velocity.y, PreviousRecord->velocity.x));

	// compute the direction change per tick.
	change = (AngleNormalize(dir - prevdir) / dt) * TICK_INTERVAL;

	if (std::abs(change) > 6.f)
		change = 0.f;

	//now forward track the player
	int forwardtrackticks = TIME_TO_TICKS(g_ClientState->m_pNetChannel->GetLatency(FLOW_OUTGOING));
	int arrivaltick = g_ClientState->m_ClockDriftMgr.m_nServerTick + 1 + forwardtrackticks;
	//int simulationtick = TIME_TO_TICKS(CurrentRecord->m_flFirstCmdTickbaseTime) + CurrentRecord->m_iTicksChoked;
	int simtick = TIME_TO_TICKS(CurrentRecord->simulationtime); //-1
	int tickbase = TIME_TO_TICKS(CurrentRecord->simulationtime);
	//int lerpticks = TIME_TO_TICKS(g_LagCompensation.GetLerpTime());
	int deltaticks = CurrentRecord->tickschoked + 1;
	// get the delta in ticks between the last server net update
	// and the net update on which we created this record.
	int updatedelta = g_ClientState->m_ClockDriftMgr.m_nServerTick - simtick;
	int next = simtick;//simtick + 1;

	//if (forwardtrackticks <= deltaticks - updatedelta)
	//	return false;

	//int next = m_nLastFarESPPacketServerTickCount + 1;
	//if (next + deltaticks >= arrivaltick)
	//	goto ShootAnyway; //return


	bool extrapolated = false;

	//fix the buttons
	m_pEntity->GetLocalData()->localdata.m_nOldButtons = GetButtons(CurrentRecord, PreviousRecord);

	CCSGameMovement *originalgamemovement = new CCSGameMovement(*(CCSGameMovement*)Interfaces::GameMovement);
	CPrediction* originalprediction = (CPrediction*)malloc(sizeof(CPrediction));
	memcpy(originalprediction, (void*)Interfaces::Prediction, sizeof(CPrediction));
	float curtime = Interfaces::Globals->curtime;
	float frametime = Interfaces::Globals->frametime;
	for (;;)
	{
		// see if by predicting this amount of lag
		// we do not break stuff.
		next += deltaticks;
		if (next >= arrivaltick)
			break;

		for (int sim = 0; sim < deltaticks; ++sim)
		{
			++tickbase;
			dir = AngleNormalize(dir + change);

			if (animstate)
				animstate->m_flGoalFeetYaw = CurrentRecord->absyaw;

			m_pEntity->ToPlayerRecord()->m_bAllowAnimationEvents = true;
			SimulatePlayer_FarESP(CurrentRecord, PreviousRecord, tickbase, false, true, usedirection, dir, change);

			m_pEntity->SetPoseParameter(12, clamp(CurrentRecord->eyepitch, -90.0f, 90.0f));
			m_pEntity->ToPlayerRecord()->m_bAllowAnimationEvents = false;

			extrapolated = true;
		}
	}

	if (extrapolated)
	{
		CCSGameMovement* gamemovement = (CCSGameMovement*)Interfaces::GameMovement;
#if 0
		gamemovement->mv = originalgamemovement->mv;
		gamemovement->player = originalgamemovement->player;
		gamemovement->m_pCSPlayer = originalgamemovement->m_pCSPlayer;
		gamemovement->m_nOldWaterLevel = originalgamemovement->m_nOldWaterLevel;
		gamemovement->m_flWaterEntryTime = originalgamemovement->m_flWaterEntryTime;
		gamemovement->m_nOnLadder = originalgamemovement->m_nOnLadder;
		gamemovement->m_vecForward = originalgamemovement->m_vecForward;
		gamemovement->m_vecRight = originalgamemovement->m_vecRight;
		gamemovement->m_vecUp = originalgamemovement->m_vecUp;
		gamemovement->m_bSpeedCropped = originalgamemovement->m_bSpeedCropped;
		gamemovement->m_bProcessingMovement = originalgamemovement->m_bProcessingMovement;
		gamemovement->m_bInStuckTest = originalgamemovement->m_bInStuckTest;
		memcpy(Interfaces::Prediction, originalprediction, sizeof(CPrediction));
#endif
	}
	Interfaces::Globals->curtime = curtime;
	Interfaces::Globals->frametime = frametime;
	delete originalgamemovement;
	free(originalprediction);

	return extrapolated;
}

void CPlayerrecord::OnCreateMove()
{
	START_PROFILING
	// reset best tick
	m_TargetRecord = nullptr;
	if (m_RestoreTick && m_RestoreTickIsDynamic)
		delete m_RestoreTick;
	m_RestoreTickIsDynamic = false;
	m_RestoreTick = nullptr;

	// don't need to restore yet
	m_bNeedsRestoring = false;

	m_bIsUsingFarESP = false;
	m_bIsValidFarESPAimbotTarget = false;
	m_bIsUsingServerSide = false;
	m_bIsValidServerSideAimbotTarget = false;

	m_iTickcount = 0;
	m_iTickcount_ForwardTrack = 0;
	m_bCountResolverStatsEvenWhenForwardtracking = false;

	// is player
	if (m_pEntity && m_pEntity->IsPlayer())
	{
		if (!m_pEntity->IsLocalPlayer())
		{
			//LocalPlayer.DrawTextInFrontOfPlayer(TICKS_TO_TIME(2), 0, "%i", m_pEntity->GetTickBase());

			bool bIsValidTarget = !m_pEntity->GetDormant() && m_pEntity->GetAlive() && !m_pEntity->IsObserver();
			FarESPPlayer *FarESPPacket = IsValidFarESPTarget();
			CSGOPacket *ServerSidePacket = IsValidServerSideTarget();
			auto record = GetCurrentRecord();

			if (bIsValidTarget)
			{
				m_TargetRecord = record;

				//Cache the most recent record's bones if needed
				if (record)
				{
					record->m_PlayerBackup.RestoreData(true, record->m_bCachedBones);

					m_iTickcount = record->m_iTickcount;
					if (!record->m_bCachedBones)
					{
						AllowSetupBonesToUpdateAttachments = true;
						CacheBones(Interfaces::Globals->curtime, !record->m_bCachedBones, record);
						AllowSetupBonesToUpdateAttachments = false;
					}
					record->m_PlayerBackup.GetBoneData(m_pEntity);
				}

				if (ServerSidePacket)
				{
					//TODO: clean up this gross function
					goto ServerSideFromValidTarget;
				}

				NormalPacket:

				// backup current tick
				StorePlayerState(record);
			}
			else if ((FarESPPacket || ServerSidePacket) && m_pEntity->GetDormant())
			{
				//TODO: clean this up
				ServerSideFromValidTarget:

				// backup current tick
				StorePlayerState(nullptr);

#ifdef 	ALLOW_SERVER_SIDE_RESOLVER
				const bool bAllowServerSideBacktrack = true;
#else
				const bool bAllowServerSideBacktrack = false;
#endif

				if (ServerSidePacket && bAllowServerSideBacktrack)
				{
					serversidemutex.lock();
					if (ServerSidePacket->bAlive)
					{
						m_bIsValidServerSideAimbotTarget = m_pEntity->GetTeam() != LocalPlayer.Entity->GetTeam() || mp_teammates_are_enemies.GetVar()->GetInt() == 1;
						m_iLifeState = ServerSidePacket->bAlive ? LIFE_ALIVE : LIFE_DEAD;
						int tickcount = TIME_TO_TICKS(ServerSidePacket->flSimulationTime);
						//ResolveYawModes mode = pCPlayer->PersistentData.LastResolveModeUsed;
						bool ValidTick = g_LagCompensation.IsTickValid(tickcount);
						if (ValidTick /*|| bFakeWalking || (mode != BackTrackReal && (mode != BackTrackLby || m_pEntity->GetVelocity().Length() < MINIMUM_PLAYERSPEED_CONSIDERED_MOVING))*/)
						{
							if (m_bDormant)
							{
								m_pEntity->SetLifeState(m_iLifeState);
								m_pEntity->SetArmor(ServerSidePacket->armor);
								m_pEntity->SetHealth(ServerSidePacket->health);
								m_pEntity->SetHasHelmet(ServerSidePacket->helmet);
							}
							m_bDormant = false;
							m_pEntity->SetDormant(FALSE);
							//Entity->SetGoalFeetYaw(ServerSidePacket->goalfeetyaw);
							//Entity->SetCurrentFeetYaw(ServerSidePacket->curfeetyaw);
							m_pEntity->SetNetworkOrigin(ServerSidePacket->absorigin);
							m_pEntity->SetAbsOrigin(ServerSidePacket->absorigin);
							m_pEntity->SetLocalOrigin(ServerSidePacket->absorigin);
							m_pEntity->SetAbsAngles(ServerSidePacket->absangles);
							int flags = m_pEntity->WritePoseParameters(ServerSidePacket->flPoseParameters);
							flags |= m_pEntity->WriteAnimLayersFromPacket(ServerSidePacket->AnimLayer);
							if (flags)
								m_pEntity->InvalidatePhysicsRecursive(flags);
							m_pEntity->SetAngleRotation(ServerSidePacket->absangles);

							QAngle eyeanglesfromrecord;
							eyeanglesfromrecord.x = ServerSidePacket->flPoseParameters[12];
							eyeanglesfromrecord.x = eyeanglesfromrecord.x * (90.0f - -90.0f) + -90.0f;
							eyeanglesfromrecord.y = ServerSidePacket->flPoseParameters[11];
							eyeanglesfromrecord.y = eyeanglesfromrecord.y * (58.0f - -58.0f) + -58.0f;
							eyeanglesfromrecord.y = AngleNormalize(ServerSidePacket->absangles.y + eyeanglesfromrecord.y);
							eyeanglesfromrecord.z = 0.0f;

							m_pEntity->SetEyeAngles(eyeanglesfromrecord);
							if (ValidTick || !bIsValidTarget)
							{
								m_pEntity->SetSimulationTime(ServerSidePacket->flSimulationTime - 0.005f); //the subtract is a hack to make aimbot shoot when they are dormant
								m_iTickcount = tickcount;
							}
							m_bNeedsRestoring = true;
							m_GeneratedTickRecord.Initialize(m_pEntity);
							m_GeneratedTickRecord.m_iTickcount = tickcount;
							AllowSetupBonesToUpdateAttachments = true;
							CacheBones(Interfaces::Globals->curtime, true, &m_GeneratedTickRecord);
							AllowSetupBonesToUpdateAttachments = false;
							m_pEntity->DrawHitboxesFromCache(ColorRGBA(255, 255, 255, 255), TICKS_TO_TIME(2), m_GeneratedTickRecord.m_PlayerBackup.CachedBoneMatrices);

							m_TargetRecord = &m_GeneratedTickRecord;
							m_bIsUsingServerSide = true;
							if (record)
								record->m_PlayerBackup.GetBoneData(m_pEntity);
						}
					}
					serversidemutex.unlock();
				}
				else if (FarESPPacket)
				{
					faresprecordmutex.lock();
					//if (FarESPPacket->alive)
					{
						m_bIsValidFarESPAimbotTarget = m_pEntity->GetTeam() != LocalPlayer.Entity->GetTeam() || mp_teammates_are_enemies.GetVar()->GetInt() == 1;
						m_iLifeState = FarESPPacket->health > 0 ? LIFE_ALIVE : LIFE_DEAD;
						int tickcount = TIME_TO_TICKS(FarESPPacket->simulationtime);
						//ResolveYawModes mode = pCPlayer->PersistentData.LastResolveModeUsed;
						//bool ValidTick = g_LagCompensation.IsTickValid(tickcount);
						//if (m_bDormant)
						//if (ValidTick /*|| bFakeWalking || (mode != BackTrackReal && (mode != BackTrackLby || m_pEntity->GetVelocity().Length() < MINIMUM_PLAYERSPEED_CONSIDERED_MOVING))*/)
						{
							if (m_bDormant)
							{
								m_pEntity->SetLifeState(m_iLifeState);
								m_pEntity->SetArmor(FarESPPacket->armor);
								m_pEntity->SetMoveType(FarESPPacket->movetype);
								m_pEntity->SetHealth(FarESPPacket->health);
								m_pEntity->SetHasHelmet(FarESPPacket->helmet);
								m_pEntity->SetDuckAmount(FarESPPacket->duckamount);
								m_pEntity->SetVelocity(FarESPPacket->velocity);
								m_pEntity->SetAbsVelocity(FarESPPacket->velocity);
								m_pEntity->SetFlags(FarESPPacket->flags);
								m_pEntity->SetAllowAutoMovement(true);
								m_pEntity->SetDeadFlag(FarESPPacket->health <= 0);
								if (m_pEntity->GetAlive())
								{
									m_pEntity->SetObserverMode(OBS_MODE_NONE);
									m_pEntity->SetPlayerState(STATE_ACTIVE);
								}
								m_bTeleporting = FarESPPacket->teleporting;
								m_iTicksChoked = FarESPPacket->tickschoked;

								m_bTeleporting = FarESPPacket->teleporting;
								m_bIsHoldingDuck = FarESPPacket->ducking;
								m_bIsFakeDucking = FarESPPacket->fakeducking;
								m_pEntity->SetIsStrafing(FarESPPacket->strafing);
								m_pEntity->SetMoveState(FarESPPacket->movestate);
								m_pEntity->SetMoveCollide((MoveCollide_t)FarESPPacket->movecollide);
								auto& localdata = m_pEntity->GetLocalData()->localdata;
								localdata.m_nDuckTimeMsecs_ = FarESPPacket->localdata.ducktimemsecs;
								localdata.m_nDuckJumpTimeMsecs_ = FarESPPacket->localdata.duckjumptimemsecs;
								localdata.m_nDuckJumpTimeMsecs_ = FarESPPacket->localdata.jumptimemsecs;
								localdata.m_flFallVelocity_ = FarESPPacket->localdata.fallvelocity;
								localdata.m_flLastDuckTime_ = FarESPPacket->localdata.lastducktime;
								localdata.m_bDucked_ = FarESPPacket->localdata.ducked;
								localdata.m_bDucking_ = FarESPPacket->localdata.ducking;
								localdata.m_bInDuckJump_ = FarESPPacket->localdata.induckjump;

								m_pEntity->SetVecLadderNormal(FarESPPacket->laddernormal);
								m_pEntity->SetTimeNotOnLadder(FarESPPacket->timenotonladder);
								m_pEntity->SetMins(FarESPPacket->mins);
								m_pEntity->SetMaxs(FarESPPacket->maxs);
								m_pEntity->SetMaxSpeed(FarESPPacket->maxspeed);
								m_pEntity->SetStamina(FarESPPacket->stamina);
								m_pEntity->SetVelocityModifier(FarESPPacket->velocitymodifier);
								m_pEntity->SetTimeOfLastInjury(FarESPPacket->timeoflastinjury);
								m_pEntity->SetFireCount(FarESPPacket->firecount);
								m_pEntity->SetLastHitgroup(FarESPPacket->lasthitgroup);
								m_pEntity->SetRelativeDirectionOfLastInjury(FarESPPacket->relativedirectionoflastinjury);
								for (int i = 0; i < MAX_RESOLVE_SIDES; ++i)
								{
									if (m_pAnimStateServer[i])
									{
										m_pAnimStateServer[i]->m_flTotalTimeInAir = FarESPPacket->animstate.totaltimeinair;
										m_pAnimStateServer[i]->m_flFlashedStartTime = FarESPPacket->animstate.flashedstarttime;
										m_pAnimStateServer[i]->m_flFlashedEndTime = FarESPPacket->animstate.flashedendtime;
										m_pAnimStateServer[i]->m_bFlashed = FarESPPacket->animstate.flashed;
										m_pAnimStateServer[i]->m_bOnGround = FarESPPacket->animstate.onground;
										m_pAnimStateServer[i]->m_flGoalFeetYaw = FarESPPacket->absyaw;
										m_pAnimStateServer[i]->m_flFeetCycle = FarESPPacket->animlayers[FEET_LAYER]._m_flCycle;
										m_pAnimStateServer[i]->m_flFeetWeight = FarESPPacket->animlayers[FEET_LAYER].m_flWeight;
										m_pAnimStateServer[i]->m_flLastClientSideAnimationUpdateTime = FarESPPacket->simulationtime;
										m_pAnimStateServer[i]->m_vVelocity = FarESPPacket->velocity;
									}
								}
							}
							m_bDormant = false;
							m_pEntity->SetDormant(FALSE);
							m_pEntity->InvalidatePhysicsRecursive(POSITION_CHANGED | ANGLES_CHANGED | ANIMATION_CHANGED);
							int flags = m_pEntity->WritePoseParameters(FarESPPacket->poseparameters);
							flags |= m_pEntity->WriteAnimLayersFromPacket(FarESPPacket->animlayers);
							if (flags)
								m_pEntity->InvalidatePhysicsRecursive(flags);
							m_pEntity->SetNetworkOrigin(FarESPPacket->absorigin);
							m_pEntity->SetAbsOrigin(FarESPPacket->absorigin);
							m_pEntity->SetLocalOrigin(FarESPPacket->absorigin);
							m_pEntity->SetAbsAngles(QAngle(0.0f, FarESPPacket->absyaw, 0.0f));
							m_pEntity->GetAbsAnglesDirect().x = 0.0f;
							m_pEntity->GetLocalAnglesDirect().x = 0.0f;
							m_pEntity->SetFlags(FarESPPacket->flags);
							m_pEntity->SetWeaponHandle((EHANDLE)INVALID_EHANDLE_INDEX);
							if (FarESPPacket->weaponclassid != -1)
							{
								IClientNetworkable* m_pNetworkable;
								ClientClass* m_pClientClass = nullptr;
								for (ClientClass* pClass = Interfaces::Client->GetAllClasses(); pClass; pClass = pClass->m_pNext)
								{
									if (pClass->m_ClassID == FarESPPacket->weaponclassid)
									{
										m_pClientClass = pClass;
										break;
									}
								}

								IClientNetworkable*(*m_pCreateFn)(int edict, int serial);
								if (m_pClientClass)
								{
#if 0
									for (int i = 65; i < Interfaces::ClientEntList->GetHighestEntityIndex(); ++i)
									{
										CBaseEntity *ent = Interfaces::ClientEntList->GetBaseEntity(i);
										if (ent && ent->GetClientClass() && ent->GetClientClass()->m_ClassID == FarESPPacket->weaponclassid)
										{
											m_pEntity->SetWeaponHandle(ent->GetRefEHandle());
											break;
										}
									}
#endif
									if (!m_pEntity->GetWeapon())
									{
#if 1
										m_pCreateFn = (IClientNetworkable*(*)(int, int))m_pClientClass->m_pCreateFn;
										if (m_pNetworkable = m_pCreateFn(2000 - g_Info.m_iNumFakeWeaponsGenerated, 2000 - g_Info.m_iNumFakeWeaponsGenerated))
										{
											++g_Info.m_iNumFakeWeaponsGenerated;
											CBaseEntity* NewWeapon = m_pNetworkable->GetIClientUnknown()->GetBaseEntity();

											m_pNetworkable->PreDataUpdate(DATA_UPDATE_CREATED);
											m_pNetworkable->OnPreDataChanged(DATA_UPDATE_CREATED);

											NewWeapon->SetDormant(false);
											NewWeapon->SetOwnerHandle(m_pEntity->GetRefEHandle());
											NewWeapon->SetFlags(EF_NODRAW);
											*(int*)((DWORD)NewWeapon + g_NetworkedVariables.Offsets.m_iItemDefinitionIndex) = FarESPPacket->weaponitemdefinitionindex;
											*(bool*)((DWORD)NewWeapon + g_NetworkedVariables.Offsets.m_bInitialized) = true;

											m_pNetworkable->OnDataChanged(DATA_UPDATE_CREATED);
											m_pNetworkable->PostDataUpdate(DATA_UPDATE_CREATED);
											NewWeapon->Precache();
											NewWeapon->Spawn();

											float spd = ((CBaseCombatWeapon*)NewWeapon)->GetMaxSpeed3();
											m_pEntity->SetWeaponHandle(NewWeapon->GetRefEHandle());
										}
#endif
									}
								}
							}
#if 0
							QAngle eyeanglesfromrecord;
							eyeanglesfromrecord.x = FarESPPacket->flPoseParameters[12];
							eyeanglesfromrecord.x = eyeanglesfromrecord.x * (90.0f - -90.0f) + -90.0f;
							eyeanglesfromrecord.y = FarESPPacket->flPoseParameters[11];
							eyeanglesfromrecord.y = eyeanglesfromrecord.y * (58.0f - -58.0f) + -58.0f;
							eyeanglesfromrecord.y = AngleNormalize(FarESPPacket->absangles.y + eyeanglesfromrecord.y);
							eyeanglesfromrecord.z = 0.0f;
							m_pEntity->SetEyeAngles(eyeanglesfromrecord);
#else
							m_pEntity->SetEyeAngles(QAngle(FarESPPacket->eyepitch, FarESPPacket->eyeyaw, 0.0f));
							*m_pEntity->EyeAngles() = m_pEntity->GetEyeAngles();
#endif
							if (!bIsValidTarget)
							{
								m_pEntity->SetSimulationTime(FarESPPacket->simulationtime - 0.005f); //the subtract is a hack to make aimbot shoot when they are dormant
								m_iTickcount = CurrentUserCmd.cmd->tick_count;
							}

							if (m_pEntity->HasFlag(FL_ONGROUND))
								m_pEntity->SetGroundEntity(m_pEntity->FindGroundEntity());
							else
								m_pEntity->SetGroundEntity(NULL);

							m_pEntity->SetFlags(FarESPPacket->flags);

							m_bNeedsRestoring = true;

							bool extrapolated = FarESPPredict();

							float spd = m_pEntity->GetVelocity().Length();
							m_pEntity->SetSimulationTime(TICKS_TO_TIME(m_iTickcount));
							m_GeneratedTickRecord = CTickrecord(m_pEntity);
							m_GeneratedTickRecord.m_iTickcount = m_iTickcount;
							m_GeneratedTickRecord.m_PlayerBackup.Get(m_pEntity);
							m_vecLastFarESPPredictedPosition = m_GeneratedTickRecord.m_AbsOrigin;
							m_vecLastFarESPPredictedMins = m_GeneratedTickRecord.m_Mins;
							m_vecLastFarESPPredictedMaxs = m_GeneratedTickRecord.m_Maxs;
							m_vecLastFarESPPredictedAbsAngles = m_GeneratedTickRecord.m_AbsAngles;

							AllowSetupBonesToUpdateAttachments = true;
							CacheBones(Interfaces::Globals->curtime, true, &m_GeneratedTickRecord);
							AllowSetupBonesToUpdateAttachments = false;

							if (!extrapolated && (m_GeneratedTickRecord.m_PlayerBackup.CachedBoneMatrices->GetOrigin() - FarESPPacket->absorigin).LengthSqr() > 4096.0f)
							{
								//Hack to force bone position to current origin if setupbones didn't put them in the correct position
								matrix3x4_t OriginalMatrixInverted, EndMatrix;
								int numbones = m_pEntity->GetCachedBoneData()->Count();

								matrix3x4_t transform = m_pEntity->EntityToWorldTransform();
								//force the position of the matrix to the world transform since EntityToWorldTransform is returning the wrong position
								PositionMatrix(m_GeneratedTickRecord.m_PlayerBackup.CachedBoneMatrices->GetOrigin(), transform);
								MatrixInvert(transform, OriginalMatrixInverted);
								MatrixCopy(transform, EndMatrix);

								//Get a positional matrix of the correct origin
								PositionMatrix(FarESPPacket->absorigin, EndMatrix);

								//FIXME: angles?

								//Get a relative transform
								matrix3x4_t TransformedMatrix;
								ConcatTransforms(EndMatrix, OriginalMatrixInverted, TransformedMatrix);

								for (int i = 0; i < numbones; i++)
								{
									//Now concat the original matrix with the rotated one
									ConcatTransforms(TransformedMatrix, m_GeneratedTickRecord.m_PlayerBackup.CachedBoneMatrices[i], m_GeneratedTickRecord.m_PlayerBackup.CachedBoneMatrices[i]);
									//old matrix		dest new matrix
								}
							}

							//if (GetAsyncKeyState(VK_RETURN) && LocalPlayer.CurrentWeapon && Interfaces::Globals->curtime >= LocalPlayer.CurrentWeapon->GetNextPrimaryAttack())
						//		m_pEntity->DrawHitboxesFromCache(ColorRGBA(255, 255, 255, 255), 5.0f, m_GeneratedTickRecord.m_PlayerBackup.CachedBoneMatrices);

							m_pEntity->DrawHitboxesFromCache(ColorRGBA(255, 255, 255, 255), TICKS_TO_TIME(2), m_GeneratedTickRecord.m_PlayerBackup.CachedBoneMatrices);


							m_TargetRecord = &m_GeneratedTickRecord;
							m_bIsUsingFarESP = true;
							if (record)
								record->m_PlayerBackup.GetBoneData(m_pEntity);
						}
					}
					faresprecordmutex.unlock();
				}
				else if (bIsValidTarget)
				{
					goto NormalPacket;
				}
			}
		}
	}
	END_PROFILING
}
void CPlayerrecord::CM_RestorePlayer()
{
	START_PROFILING
	// valid player, restore
	if (m_pEntity && m_pEntity->IsPlayer() && m_pEntity != LocalPlayer.Entity)
	{
		// restore backup state
		if (m_RestoreTick)
		{
			if (m_bNeedsRestoring)
				m_RestoreTick->m_PlayerBackup.RestoreData();

			if (m_bIsUsingFarESP || m_bIsUsingServerSide)
			{
				m_pEntity->SetDormant(m_RestoreTick->m_PlayerBackup.Dormant);
				m_pEntity->SetArmor(m_RestoreTick->m_PlayerBackup.Armor);
				m_pEntity->SetHealth(m_RestoreTick->m_PlayerBackup.Health);
				m_pEntity->SetHasHelmet(m_RestoreTick->m_PlayerBackup.Helmet);
				m_pEntity->SetLifeState(m_RestoreTick->m_PlayerBackup.LifeState);
				m_pEntity->SetFlags(m_RestoreTick->m_PlayerBackup.Flags);
				m_pEntity->SetDuckAmount(m_RestoreTick->m_PlayerBackup.DuckAmount);
				m_pEntity->SetDeadFlag(m_RestoreTick->m_PlayerBackup.DeadFlag);
				m_bDormant = m_RestoreTick->m_PlayerBackup.Dormant;

				if (m_bIsUsingFarESP)
				{
					if (m_pEntity->GetWeapon() && ((CBaseEntity*)m_pEntity->GetWeapon())->GetOwner() == m_pEntity)
					{
						((CBaseEntity*)m_pEntity->GetWeapon())->GetClientNetworkable()->Release();
						--g_Info.m_iNumFakeWeaponsGenerated;
					}
					
					m_pEntity->SetWeaponHandle(m_RestoreTick->m_PlayerBackup.WeaponHandle);
				}
			}

			if (m_RestoreTickIsDynamic)
				delete m_RestoreTick;

			m_RestoreTick = nullptr;
			m_RestoreTickIsDynamic = false;
		}

		// reset target tickcount
		m_iTickcount = 0;
	}

	m_bNeedsRestoring = false;
	END_PROFILING
}

//HIGHLY not recommend using this because we can't check deadtime here!
bool CLagCompensation_imi::IsTickValid(int tick)
{
	auto nci = Interfaces::EngineClient->GetNetChannelInfo();
	// NOTE: predict the servertime.
	float serverTime = LocalPlayer.Entity->GetTickBase() * Interfaces::Globals->interval_per_tick;

	// NOTE: get the time of our target.
	auto flTargetTime = TICKS_TO_TIME(tick) - GetLerpTime();

	// NOTE: outgoing latency + const viewlag aka TICKS_TO_TIME(TIME_TO_TICKS(GetClientInterpAmount())) and clamp correct to sv_maxunlag
	auto correct = clamp(nci->GetAvgLatency(FLOW_OUTGOING) + GetLerpTime(), 0.f, sv_maxunlag.GetVar()->GetFloat());

	// NOTE: calculate delta.
	auto delta = correct - (serverTime - flTargetTime);

	float maxDelta = 0.2f;

	// NOTE: ensure this record isn't too old.
	return abs(delta) < maxDelta;
}

bool CLagCompensation_imi::IsTickValid(CTickrecord* record)
{
	auto nci = Interfaces::EngineClient->GetNetChannelInfo();

	// NOTE: predict the servertime.
	float serverTime = LocalPlayer.Entity->GetTickBase() * Interfaces::Globals->interval_per_tick;

	// NOTE: get the time of our target.
	float flTargetTime = record->m_SimulationTime;

	int latencyticks = max(0, TIME_TO_TICKS(g_ClientState->m_pNetChannel->GetLatency(FLOW_OUTGOING)));
	int server_tickcount = g_ClientState->m_ClockDriftMgr.m_nServerTick + latencyticks + 1;
	float server_time_at_frame_end_from_last_frame = TICKS_TO_TIME(server_tickcount - 1);

	int deltaticks = record->m_iTicksChoked + 1;
	int updatedelta = g_ClientState->m_ClockDriftMgr.m_nServerTick - record->m_iServerTick;
	if (latencyticks > deltaticks - updatedelta)
	{
		//only check if record is deleted if enemy would have sent another tick to the server already

		int flDeadtime = (server_time_at_frame_end_from_last_frame - sv_maxunlag.GetVar()->GetFloat());

		if (flTargetTime < flDeadtime)
			return false; //record won't be valid anymore
	}

	// NOTE: outgoing latency + const viewlag aka TICKS_TO_TIME(TIME_TO_TICKS(GetClientInterpAmount())) and clamp correct to sv_maxunlag
	float correct = clamp(nci->GetAvgLatency(FLOW_OUTGOING) + GetLerpTime(), 0.f, sv_maxunlag.GetVar()->GetFloat());
	float delta = correct - (serverTime - flTargetTime);
	return fabsf(delta) < 0.2f;
}

void __cdecl Parallel_FSN_UpdatePlayers(const int& index)
{
	// get playerrecord
	CPlayerrecord* _playerRecord = &m_PlayerRecords[index];

	// update playerrecord
	_playerRecord->UpdateInfo(index);

	// create tickrecord
	if (_playerRecord->m_bConnected && _playerRecord->m_pEntity)
		_playerRecord->FSN_UpdatePlayer();
}

void CLagCompensation_imi::FSN_UpdatePlayers()
{
	START_PROFILING
	// loop through all potential players
	RENDER_MUTEX.Lock();
	g_Info.m_SpectatorList.clear();

#ifndef NO_PARALLEL_NETCODE
	ParallelProcess(m_pParallelProcessIndexes, MAX_PLAYERS, &Parallel_FSN_UpdatePlayers);
#else
	
	for (int i = 1; i <= MAX_PLAYERS; i++)
	{
		// get playerrecord
		CPlayerrecord* _playerRecord = &m_PlayerRecords[i];

		// update playerrecord
		_playerRecord->UpdateInfo(i);

		// create tickrecord
		if (_playerRecord->m_bConnected && _playerRecord->m_pEntity)
			_playerRecord->FSN_UpdatePlayer();
	}
#endif
	RENDER_MUTEX.Unlock();
	END_PROFILING
}

void CLagCompensation_imi::OnCreateMove()
{
	if (!Interfaces::EngineClient->IsInGame() || !LocalPlayer.Entity)
		return;
	
	START_PROFILING
	// loop through all players
	for (int i = 1; i <= MAX_PLAYERS; i++)
	{
		// get playerrecord
		auto a = Interfaces::ClientEntList->GetBaseEntity(i);
		CPlayerrecord* _playerRecord = GetPlayerrecord(a);

		if (_playerRecord && _playerRecord->m_pEntity)
		{
			//Workaround for game manipulating eye angles even though we are saving/restoring them properly
			/*a->SetEyeAngles(_playerRecord->m_angPristineEyeAngles);
			*a->EyeAngles() = _playerRecord->m_angPristineEyeAngles;*/

			_playerRecord->OnCreateMove();
		}
	}
	END_PROFILING
}

extern bool isVisible(CBaseEntity* TempTarget, CBaseEntity* LocalEntity, Vector LocalEyePos, QAngle LocalAngles, Vector vecTarget);

void CLagCompensation_imi::CM_RestorePlayers()
{
	START_PROFILING

	// loop through all players
	for (int i = 1; i <= MAX_PLAYERS; i++)
	{
		// get playerrecord
		CPlayerrecord* _playerRecord = &m_PlayerRecords[i];

		// playerrecord valid
		if (_playerRecord->m_pEntity)
		{
			if (_playerRecord->m_bNeedsRestoring)
				_playerRecord->CM_RestorePlayer();

			if (!_playerRecord->m_pEntity->GetDormant() && _playerRecord->m_pEntity->GetAlive() && !_playerRecord->m_pEntity->IsSpectating())
			{
				auto _collideable = _playerRecord->m_pEntity->GetCollideable();
				_playerRecord->m_vecOBBMins = _collideable->OBBMins();
				_playerRecord->m_vecOBBMaxs = _collideable->OBBMaxs();

				if (!_playerRecord->m_bIsUsingFarESP && !_playerRecord->m_bIsUsingServerSide)
				{
					CM_PredictPlayer(_playerRecord);
				}
			}
			else if (_playerRecord->m_bIsUsingFarESP)
			{
				auto _collideable = _playerRecord->m_pEntity->GetCollideable();
				_playerRecord->m_vecOBBMins = _collideable->OBBMins();
				_playerRecord->m_vecOBBMaxs = _collideable->OBBMaxs();
			}
		}
	}
	END_PROFILING
}

void CLagCompensation_imi::CM_PredictPlayer(CPlayerrecord *_player)
{
	return;
#if 0
	if (!variable::get().visuals.pf_enemy.b_smooth_animation)
		return;
	CTickrecord *_currentRecord = _player->GetCurrentRecord();
	CTickrecord *_previousRecord = _player->GetPreviousRecord();
	if (_currentRecord && _previousRecord && !_previousRecord->m_Dormant)
	{
		CCSGOPlayerAnimState _backupAnimState = *_player->m_pAnimStateServer[_currentRecord->m_iResolveSide];
		s_Impact_Info *_ResolveInfo = nullptr;
		int _numCommandsLagged = g_ClientState->m_ClockDriftMgr.m_nServerTick - _player->m_nServerTickCount;
		int _time = 1 + TIME_TO_TICKS(_player->m_pAnimStateServer[ResolveSides::NONE]->m_flLastClientSideAnimationUpdateTime);
		int _deltaTicks = _previousRecord->m_iTicksChoked + 1;
		float choked_yaw;
		float _desyncAmount = 0.0f;
		float _desyncYawAmount = 0.0f;
		bool _useBodyResolver = false;
		if (!variable::get().ragebot.b_resolver || (_player->m_bLegit && !_player->m_bForceNotLegit))
		{
			choked_yaw = _currentRecord->m_EyeAngles.y;
		}
		else
		{
			CTickrecord* _prePreviousRecord = _player->m_Tickrecords.size() >= 3 ? _player->m_Tickrecords[2] : nullptr;
			_ResolveInfo = _player->GetBodyHitResolveInfo();
			if (_ResolveInfo)
			{
				_player->GetBodyHitDesyncAmount(&_desyncAmount, &_desyncYawAmount, _currentRecord, _previousRecord, _prePreviousRecord, _ResolveInfo);
				choked_yaw = AngleNormalize(_currentRecord->m_EyeAngles.y + _desyncYawAmount);
				_useBodyResolver = _desyncAmount != 0.0f;
			}
			else
			{
				_desyncYawAmount = CPlayerrecord::GetYawFromSide(_currentRecord->m_iResolveSide);
				_desyncAmount = AngleNormalize(_desyncYawAmount * 0.5f);
				choked_yaw = AngleNormalize(_currentRecord->m_EyeAngles.y + CPlayerrecord::GetYawFromSide(_currentRecord->m_iResolveSide));
			}
		}
		float pitch = _currentRecord->m_EyeAngles.x;
		if (_numCommandsLagged)
		{
			float change = 0.f, dir = 0.f;

			// get the direction of the current velocity.
			if (_currentRecord->m_Velocity.y != 0.f || _currentRecord->m_Velocity.x != 0.f)
				dir = RAD2DEG(atan2(_currentRecord->m_Velocity.y, _currentRecord->m_Velocity.x));

			QAngle angelz;
			VectorAngles(_currentRecord->m_Velocity, angelz);

			// we have more than one update
			// we can compute the direction.
			// get the delta time between the 2 most recent records.
			float dt = TICKS_TO_TIME(_currentRecord->m_iServerTick - _previousRecord->m_iServerTick);

			// init to 0.
			float prevdir = 0.f;

			// get the direction of the prevoius velocity.
			if (_previousRecord->m_AbsVelocity.y != 0.f || _previousRecord->m_AbsVelocity.x != 0.f)
				prevdir = RAD2DEG(std::atan2(_previousRecord->m_AbsVelocity.y, _previousRecord->m_AbsVelocity.x));

			// compute the direction change per tick.
			change = (AngleNormalize(dir - prevdir) / dt) * TICK_INTERVAL;

			if (std::abs(change) > 6.f)
				change = 0.f;

			CTickrecord* newprevioustickrecord = nullptr;
			CBaseEntity *pEntity = _player->m_pEntity;
			bool isfinaltick = false;

			for (int i = 0; i < _numCommandsLagged; ++i)
			{
				dir = AngleNormalize(dir + change);

				float yaw;
				if (i + 1 >= _deltaTicks)
				{
					yaw = _currentRecord->m_EyeAngles.y; //sendpacket tick
					isfinaltick = true;
				}
				else
				{
					//yaw = choked_yaw;
					yaw = _currentRecord->m_EyeAngles.y;
				}
				
				switch (_currentRecord->m_iResolveSide)
				{
					case ResolveSides::NEGATIVE_60:
						if (!isfinaltick)
							_player->m_pAnimStateServer[ResolveSides::NEGATIVE_60]->m_flGoalFeetYaw = AngleNormalizePositive(yaw - 60.0f);
						break;
					case ResolveSides::NEGATIVE_35:
						if (!isfinaltick)
							_player->m_pAnimStateServer[ResolveSides::NEGATIVE_35]->m_flGoalFeetYaw = AngleNormalizePositive(yaw - 35.0f);
						break;
					case ResolveSides::POSITIVE_60:
						if (!isfinaltick)
							_player->m_pAnimStateServer[ResolveSides::POSITIVE_60]->m_flGoalFeetYaw = AngleNormalizePositive(yaw + 60.0f);
						break;
					case ResolveSides::POSITIVE_35:
						if (!isfinaltick)
							_player->m_pAnimStateServer[ResolveSides::POSITIVE_35]->m_flGoalFeetYaw = AngleNormalizePositive(yaw + 35.0f);
						break;
					default:
						break;
				}

				_player->SimulatePlayer(_currentRecord, _previousRecord, _time + i, false, true, true, i + 1 >= _deltaTicks, _currentRecord->m_iResolveSide, pitch, yaw, &dir, &change);

				/*
				if (_useBodyResolver && i == (_deltaTicks - 2))
				{
					//second to last tick, generate new record
					newprevioustickrecord = new CTickrecord(*_currentRecord);
					newprevioustickrecord->m_animstate_server = new CCSGOPlayerAnimState(*_player->m_animstate_server);
					newprevioustickrecord->m_AbsAngles = pEntity->GetAbsAnglesDirect();
					newprevioustickrecord->m_AbsOrigin = pEntity->GetAbsOriginDirect();
					newprevioustickrecord->m_Velocity = pEntity->GetVelocity();
					newprevioustickrecord->m_AbsVelocity = pEntity->GetAbsVelocityDirect();
					newprevioustickrecord->m_NetOrigin = pEntity->GetNetworkOrigin();
					newprevioustickrecord->m_Origin = pEntity->GetLocalOriginDirect();
					newprevioustickrecord->m_bStrafing = pEntity->IsStrafing();
					newprevioustickrecord->m_vecLadderNormal = pEntity->GetVecLadderNormal();
					newprevioustickrecord->m_flLowerBodyYaw = pEntity->GetLowerBodyYaw();
					newprevioustickrecord->m_EyeAngles = *pEntity->EyeAngles();
					pEntity->CopyAnimLayers(newprevioustickrecord->m_AnimLayer);
				}
				*/
			}

			/*
			if (_useBodyResolver && _numCommandsLagged)
			{
				CTickrecord *newcurrenttickrecord = new CTickrecord(*_currentRecord);
				newcurrenttickrecord->m_animstate_server = _player->m_animstate_server;
				newcurrenttickrecord->m_AbsAngles = pEntity->GetAbsAnglesDirect();
				newcurrenttickrecord->m_AbsOrigin = pEntity->GetAbsOriginDirect();
				newcurrenttickrecord->m_Velocity = pEntity->GetVelocity();
				newcurrenttickrecord->m_AbsVelocity = pEntity->GetAbsVelocityDirect();
				newcurrenttickrecord->m_NetOrigin = pEntity->GetNetworkOrigin();
				newcurrenttickrecord->m_Origin = pEntity->GetLocalOriginDirect();
				newcurrenttickrecord->m_bStrafing = pEntity->IsStrafing();
				newcurrenttickrecord->m_vecLadderNormal = pEntity->GetVecLadderNormal();
				newcurrenttickrecord->m_flLowerBodyYaw = pEntity->GetLowerBodyYaw();
				newcurrenttickrecord->m_EyeAngles = *pEntity->EyeAngles();
				pEntity->CopyAnimLayers(newcurrenttickrecord->m_AnimLayer);

				_player->ApplyBodyHitResolver(_ResolveInfo, _desyncAmount, _player->m_animstate_server, newcurrenttickrecord, newprevioustickrecord ? newprevioustickrecord : _previousRecord);

				if (newprevioustickrecord)
					delete newprevioustickrecord;

				newcurrenttickrecord->m_animstate_server = nullptr;
				delete newcurrenttickrecord;
			}
			*/

			CBoneAccessor *accessor = _player->m_pEntity->GetBoneAccessor();
			_player->m_pEntity->SetPoseParameter(12, clamp(AngleNormalize(pitch), -90.0f, 90.0f));
			_player->m_pEntity->InvalidateBoneCache();
			_player->m_pEntity->SetLastOcclusionCheckFlags(0);
			_player->m_pEntity->SetLastOcclusionCheckFrameCount(Interfaces::Globals->framecount);
			AllowSetupBonesToUpdateAttachments = true;
			_player->m_pEntity->SetupBones(nullptr, -1, BONE_USED_BY_ANYTHING, _time);
			AllowSetupBonesToUpdateAttachments = false;
		}
		*_player->m_pAnimStateServer[_currentRecord->m_iResolveSide] = _backupAnimState;
	}
#endif
}

ConVar* cl_interp = nullptr;
ConVar* cl_interp_ratio = nullptr;
ConVar* cl_updaterate = nullptr;
ConVar* sv_minupdaterate = nullptr;
ConVar* sv_maxupdaterate = nullptr;

float CLagCompensation_imi::GetLerpTime()
{
	if (!cl_updaterate)
	{
		//decrypts(0)
		cl_interp = Interfaces::Cvar->FindVar(XorStr("cl_interp"));
		cl_updaterate = Interfaces::Cvar->FindVar(XorStr("cl_updaterate"));
		cl_interp_ratio = Interfaces::Cvar->FindVar(XorStr("cl_interp_ratio"));
		sv_minupdaterate = Interfaces::Cvar->FindVar(XorStr("sv_minupdaterate"));
		sv_maxupdaterate = Interfaces::Cvar->FindVar(XorStr("sv_maxupdaterate"));
		//encrypts(0)
	}
	float update_rate = clamp(cl_updaterate->GetFloat(), sv_minupdaterate->GetFloat(), sv_maxupdaterate->GetFloat());
	float lerp_ratio = cl_interp_ratio->GetFloat();

	if (lerp_ratio == 0.0f)
		lerp_ratio = 1.0f;

	float lerp_amount = cl_interp->GetFloat();

	//decrypts(0)
	static ConVar* pMin = Interfaces::Cvar->FindVar(XorStr("sv_client_min_interp_ratio"));
	static ConVar* pMax = Interfaces::Cvar->FindVar(XorStr("sv_client_max_interp_ratio"));
	//encrypts(0)

	if (pMin && pMax && pMin->GetFloat() != -1.0f)
	{
		lerp_ratio = clamp(lerp_ratio, pMin->GetFloat(), pMax->GetFloat());
	}
	else
	{
		if (lerp_ratio == 0.0f)
			lerp_ratio = 1.0f;
	}

	float ret = fmax(lerp_amount, lerp_ratio / update_rate);

	return ret;
}

CPlayerrecord* CLagCompensation_imi::GetPlayerrecord(CBaseEntity* _Entity)
{
	if (_Entity && _Entity->index > 0 && _Entity->index <= MAX_PLAYERS)
		return &m_PlayerRecords[_Entity->index];

	return nullptr;
}

CPlayerrecord* CLagCompensation_imi::GetPlayerrecord(int i)
{
	if (i >= 1 && i <= MAX_PLAYERS)
		return &m_PlayerRecords[i];

	return nullptr;
}

void CLagCompensation_imi::AdjustCMD(CBaseEntity* _Entity)
{
	// invalid
	if (!_Entity)
	{
		CTraceFilterPlayersOnly filter;
		filter.pSkip = (IHandleEntity*)LocalPlayer.Entity;
		filter.m_icollisionGroup = COLLISION_GROUP_NONE;

		Ray_t ray;
		Vector vecDir;
		AngleVectors(LocalPlayer.FinalEyeAngles, &vecDir);
		ray.Init(LocalPlayer.ShootPosition, LocalPlayer.ShootPosition + vecDir * 8192.0f);
		trace_t tr;
		Interfaces::EngineTrace->TraceRay(ray, MASK_SHOT, &filter, &tr);

		if (tr.m_pEnt && tr.m_pEnt->IsPlayer() && tr.m_pEnt->GetAlive() && !tr.m_pEnt->IsSpectating())
			CurrentUserCmd.cmd->tick_count = TIME_TO_TICKS(tr.m_pEnt->GetSimulationTime() + g_LagCompensation.GetLerpTime());

		return;
	}

	// get playerrecord
	CPlayerrecord* _playerRecord = GetPlayerrecord(_Entity);

	if (!_playerRecord || !_playerRecord->m_TargetRecord)
		return;

	// not breaking lagcomp
	if (_playerRecord->m_bIsUsingFarESP || _playerRecord->m_bIsUsingServerSide || !_playerRecord->m_bTeleporting || _playerRecord->m_iTickcount_ForwardTrack != 0)
	{
		// set backtrack tickcount
		CurrentUserCmd.cmd->tick_count = _playerRecord->m_iTickcount_ForwardTrack != 0 ? _playerRecord->m_iTickcount_ForwardTrack + TIME_TO_TICKS(GetLerpTime()) :  _playerRecord->m_TargetRecord->m_iTickcount + TIME_TO_TICKS(GetLerpTime());
		//CurrentUserCmd.cmd->tick_count--;
		//CurrentUserCmd.cmd->tick_count--;
		//printf("Shot at %f %f %f\n", _playerRecord->m_pEntity->GetAbsOriginDirect().x, _playerRecord->m_pEntity->GetAbsOriginDirect().y, _playerRecord->m_pEntity->GetAbsOriginDirect().z);

		// backtrack tickcount was 0, adjust
		if (!CurrentUserCmd.cmd->tick_count)
			CurrentUserCmd.cmd->tick_count = TIME_TO_TICKS(_Entity->GetSimulationTime() + GetLerpTime());
	}
	else
	{
		int _lerpticks = TIME_TO_TICKS(GetLerpTime());
		int _targettick = CurrentUserCmd.cmd->tick_count + _lerpticks;
		
		CurrentUserCmd.cmd->tick_count = _targettick;
	}
}

Vector CLagCompensation_imi::GetSmoothedVelocity(float delta, Vector& newvel, Vector& prevvel) {
	Vector result;
	Vector vecdelta = newvel - prevvel;
	float deltalength = vecdelta.Length();

	if (deltalength <= delta) 
	{
		if (-delta <= deltalength) 
		{
			result = newvel;
		}
		else
		{
			float radius = 1.f / (sqrtf(vecdelta.x*vecdelta.x + vecdelta.y*vecdelta.y + vecdelta.z*vecdelta.z) + FLT_EPSILON);
			result = prevvel - ((vecdelta * radius) * delta);
		}
	}
	else
	{
		float radius = 1.f / (sqrtf(vecdelta.x*vecdelta.x + vecdelta.y*vecdelta.y + vecdelta.z*vecdelta.z) + FLT_EPSILON);
		result = prevvel + ((vecdelta * radius) * delta);
	}
	return result;
}

//Gets the opposite side
ResolveSides CLagCompensation_imi::GetOppositeResolveSide(ResolveSides side)
{
	switch (side)
	{
		case ResolveSides::NEGATIVE_35:
			return ResolveSides::POSITIVE_35;
		case ResolveSides::NEGATIVE_60:
			return ResolveSides::POSITIVE_60;
		case ResolveSides::POSITIVE_60:
			return ResolveSides::NEGATIVE_60;
		case ResolveSides::POSITIVE_35:
			return ResolveSides::NEGATIVE_35;
	};
	return side;
}

//Gets the opposite side with a different delta
ResolveSides CLagCompensation_imi::GetOppositeResolveSideWithNewDelta(ResolveSides side)
{
	switch (side)
	{
		case ResolveSides::NEGATIVE_35:
			return ResolveSides::POSITIVE_60;
		case ResolveSides::NEGATIVE_60:
			return ResolveSides::POSITIVE_35;
		case ResolveSides::POSITIVE_60:
			return ResolveSides::NEGATIVE_35;
		case ResolveSides::POSITIVE_35:
			return ResolveSides::NEGATIVE_60;
	};
	return side;
}

bool CLagCompensation_imi::IsResolveSide35(ResolveSides side)
{
	return side == ResolveSides::POSITIVE_35 || side == ResolveSides::NEGATIVE_35;
}

bool CLagCompensation_imi::IsResolveSide60(ResolveSides side)
{
	return side == ResolveSides::POSITIVE_60 || side == ResolveSides::NEGATIVE_60;
}

bool CLagCompensation_imi::IsResolveSidePositive(ResolveSides side)
{
	return side == ResolveSides::POSITIVE_35 || side == ResolveSides::POSITIVE_60;
}

bool CLagCompensation_imi::IsResolveSideNegative(ResolveSides side)
{
	return side == ResolveSides::NEGATIVE_35 || side == ResolveSides::NEGATIVE_60;
}