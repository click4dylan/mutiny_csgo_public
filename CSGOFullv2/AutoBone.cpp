#include "precompiled.h"
#include "AutoBone_imi.h"
#include "Removals.h"
#include "HitChance.h"
#include "LocalPlayer.h"
#include "WeaponController.h"
#include "CParallelProcessor.h"
#include "Aimbot_imi.h"
#include "AntiAim.h"
#include "UsedConvars.h"
#include "Eventlog.h"
#include "TickbaseExploits.h"

#include "Adriel/stdafx.hpp"
#include "Adriel/adr_util.hpp"

CAutoBone g_AutoBone;
CAutoBone m_AutoBoneState;

std::array<double, 5> RunTime;
std::mutex m_SaveHitboxMutex;

bool BoneVisible(int TargetHitbox, const Vector& TestHitboxPosition, CBaseEntity* pPlayer, float& DestDamage, int& DestHitbox, int& DestHitgroup, QAngle* LocalEyeAngles, bool& Penetrated)
{
	Autowall_Output_t output;
	bool scan_through_teammates = variable::get().ragebot.b_scan_through_teammates;
	CBaseEntity* EntityHit = Autowall(LocalPlayer.ShootPosition, TestHitboxPosition, output, !scan_through_teammates, false, pPlayer);
	if (EntityHit == pPlayer && output.damage_dealt > 0.0f)
	{
		Penetrated = output.penetrated_wall;
		DestDamage = output.damage_dealt;
		DestHitgroup = output.hitgroup_hit;
		DestHitbox = output.hitbox_hit;
		return true;
	}

	Penetrated = false;
	DestHitgroup = 0;
	DestDamage   = 0.0f;
	return false;
}

void CAutoBone::s_Profiling::Run()
{
	m_dbStartTime	  = QPCTime();
	m_dbAvgTimeToRun   = 0.0;
	m_dbTotalTimeToRun = 0.0;

	for (auto time : RunTime)
		m_dbTotalTimeToRun += time;

	if (!RunTime.empty())
		m_dbAvgTimeToRun = m_dbTotalTimeToRun / RunTime.size();

	if (Interfaces::Globals->tickcount != TickRanFrameStatistics)
	{
		m_iTimesRunThisFrame   = 0;
		TickRanFrameStatistics = Interfaces::Globals->tickcount;
	}
	++m_iTimesRunThisFrame;
}

void CAutoBone::s_Profiling::Finish()
{
	RunTime[CurrentUserCmd.cmd->command_number % RunTime.size()] = QPCTime() - m_dbStartTime;
}

void CAutoBone::SetupHitboxes(bool DisableHeadMultipoint, bool MinimumPointsOnly)
{
	START_PROFILING
	if (!m_bCanHitscan)
	{
		m_Hitboxes.fill(s_Hitbox{});
		END_PROFILING
		return;
	}

	auto& var = variable::get();

	MinimumPointsOnly = false;

	if (!MinimumPointsOnly)
	{
		for (int hitbox = HITBOX_HEAD; hitbox < HITBOX_MAX; ++hitbox)
		{
			auto* hb = &m_Hitboxes[hitbox];

			// head, neck
			if (hitbox <= HITBOX_LOWER_NECK)
			{
				if (hitbox == HITBOX_LOWER_NECK)
					hb->m_bTarget = false;
				else if (hb->m_bTarget = (m_bCanScanHead && !m_bCanScanBody) || (var.ragebot.hitscan_head.b_enabled && !var.ragebot.baim_main.b_force.get()))
				{
					hb->m_bMultipoint = m_bCanMultipoint && var.ragebot.hitscan_head.b_multipoint;
					hb->m_iPriority = var.ragebot.hitscan_head.i_priority;
				}
			}
			// pelvis, body
			else if (hitbox <= HITBOX_BODY)
			{
				if (hb->m_bTarget = var.ragebot.hitscan_stomach.b_enabled)
				{
					hb->m_bMultipoint = m_bCanMultipoint && var.ragebot.hitscan_stomach.b_multipoint && m_bHittingTargetFPS;
					hb->m_iPriority = var.ragebot.hitscan_stomach.i_priority;
				}
			}
			// thorax, chest, upper chest
			else if (hitbox <= HITBOX_UPPER_CHEST)
			{
				hb->m_bTarget = false;
				if (hitbox == HITBOX_THORAX || hitbox == HITBOX_CHEST)
				{
					if (hb->m_bTarget = var.ragebot.hitscan_chest.b_enabled)
					{
						hb->m_bMultipoint = m_bCanMultipoint && var.ragebot.hitscan_chest.b_multipoint && m_bHittingTargetFPS;
						hb->m_iPriority = var.ragebot.hitscan_chest.i_priority;
					}
				}
			}
			// left/right thigh, left/right calf
			else if (hitbox <= HITBOX_RIGHT_CALF)
			{
				if (hitbox >= HITBOX_LEFT_CALF)
				{
					hb->m_bTarget = false; //disable calves.. people complained
				}
				else
				{
					if (hb->m_bTarget = var.ragebot.hitscan_legs.b_enabled)
					{
						//knees are the multipoint for legs, thighs are the center point for the legs
						hb->m_bMultipoint = m_bCanMultipoint && var.ragebot.hitscan_legs.b_multipoint && m_bHittingTargetFPS;
						hb->m_iPriority = var.ragebot.hitscan_legs.i_priority;
					}
				}
			}
			// left/right feet
			else if (hitbox <= HITBOX_RIGHT_FOOT)
			{
				if (hb->m_bTarget = var.ragebot.hitscan_feet.b_enabled)
				{
					hb->m_bMultipoint = false;// m_bCanMultipoint && var.ragebot.hitscan_feet.b_multipoint && m_bHittingTargetFPS;
					hb->m_iPriority = var.ragebot.hitscan_feet.i_priority;
				}
			}
			// left/right hand
			else if (hitbox <= HITBOX_RIGHT_HAND)
			{
				if (hb->m_bTarget = var.ragebot.hitscan_hands.b_enabled)
				{
					hb->m_bMultipoint = false;
					hb->m_iPriority = var.ragebot.hitscan_hands.i_priority;
				}
			}
			// left/right forearm, upperarm
			else if (hitbox <= HITBOX_RIGHT_FOREARM)
			{
				//just use this for shoulder right now

				if (hb->m_bTarget = var.ragebot.hitscan_shoulders.b_enabled)
				//if (hb->m_bTarget = var.ragebot.hitscan_arms.b_enabled)
				{
					hb->m_bMultipoint = true;//m_bCanMultipoint && var.ragebot.hitscan_arms.b_multipoint && m_bHittingTargetFPS;
					hb->m_iPriority = var.ragebot.hitscan_arms.i_priority;
				}
			}
		}
	}
	else
	{
		// minimum points only - only aim at head, pelvis, right and left calf and right and left foot
		m_Hitboxes.fill(s_Hitbox{});

		// cover the head hitbox selection
		if ((m_bCanScanHead && !m_bCanScanBody) || !var.ragebot.baim_main.b_force.get())
		{
			m_Hitboxes[HITBOX_HEAD].m_bTarget = var.ragebot.hitscan_head.b_enabled;
			m_Hitboxes[HITBOX_HEAD].m_iPriority = var.ragebot.hitscan_head.i_priority;
		}

		// cover the chest hitbox selection
		m_Hitboxes[HITBOX_CHEST].m_bTarget = var.ragebot.hitscan_chest.b_enabled;
		m_Hitboxes[HITBOX_CHEST].m_iPriority = var.ragebot.hitscan_chest.i_priority;

		// cover the stomach hitbox selection
		m_Hitboxes[HITBOX_PELVIS].m_bTarget = var.ragebot.hitscan_stomach.b_enabled;
		m_Hitboxes[HITBOX_PELVIS].m_iPriority = var.ragebot.hitscan_stomach.i_priority;

		// cover the legs hitbox selection
		m_Hitboxes[HITBOX_LEFT_CALF].m_bTarget = var.ragebot.hitscan_legs.b_enabled;
		m_Hitboxes[HITBOX_LEFT_CALF].m_iPriority = var.ragebot.hitscan_legs.i_priority;
		m_Hitboxes[HITBOX_RIGHT_CALF].m_bTarget = var.ragebot.hitscan_legs.b_enabled;
		m_Hitboxes[HITBOX_RIGHT_CALF].m_iPriority = var.ragebot.hitscan_legs.i_priority;

		// cover the feet hitbox selection
		m_Hitboxes[HITBOX_LEFT_FOOT].m_bTarget = var.ragebot.hitscan_feet.b_enabled;
		m_Hitboxes[HITBOX_LEFT_FOOT].m_iPriority = var.ragebot.hitscan_feet.i_priority;
		m_Hitboxes[HITBOX_RIGHT_FOOT].m_bTarget = var.ragebot.hitscan_feet.b_enabled;
		m_Hitboxes[HITBOX_RIGHT_FOOT].m_iPriority = var.ragebot.hitscan_feet.i_priority;

		// cover the arms hitbox selection
		m_Hitboxes[HITBOX_LEFT_FOREARM].m_bTarget = var.ragebot.hitscan_arms.b_enabled;
		m_Hitboxes[HITBOX_LEFT_FOREARM].m_iPriority = var.ragebot.hitscan_arms.b_enabled;
		m_Hitboxes[HITBOX_RIGHT_FOREARM].m_bTarget = var.ragebot.hitscan_arms.b_enabled;
		m_Hitboxes[HITBOX_RIGHT_FOREARM].m_iPriority = var.ragebot.hitscan_arms.b_enabled;
	}

	m_bCanScanHead = m_Hitboxes[HITBOX_HEAD].m_bTarget || m_Hitboxes[HITBOX_LOWER_NECK].m_bTarget;

	if (!m_bCanScanHead)
	{
		m_bFinishedScanningHead = true;
		m_iNumHeadPoints = 0;
		m_iNumHeadPointsScanned = 0;
	}

	m_bCanScanBody = false;

	for (int i = HITBOX_PELVIS; i < HITBOX_MAX; ++i)
	{
		if (m_Hitboxes[i].m_bTarget)
		{
			m_bCanScanBody = true;
			break;
		}
	}
	END_PROFILING
}

bool CAutoBone::Init(CBaseEntity* _Entity)
{
	m_pBestHitbox  = nullptr;
	m_iBestHitbox  = -1;
	m_pEntity	  = _Entity;
	m_Playerrecord = g_LagCompensation.GetPlayerrecord(_Entity);
	g_Info.UsingForwardTrack = false;

	// invalid entity, playerrecord or backtrack tick
	if (!m_pEntity || !m_Playerrecord || !m_Playerrecord->m_TargetRecord || !m_Playerrecord->m_pEntity)
		return false;

	// time for setupbones
	m_flTargetTime = Interfaces::Globals->curtime;

	// no recoil angles
	m_NoRecoilAngles = LocalPlayer.CurrentEyeAngles;
	g_Removals.DoNoRecoil(m_NoRecoilAngles);

	// weapon damage
	const auto wpn_data = LocalPlayer.CurrentWeapon->GetCSWpnData();
	m_iWeaponDamage		= wpn_data ? wpn_data->iDamage : 0;

	// target tick
	m_BacktrackTick = m_Playerrecord->m_TargetRecord;
//#ifdef UK_FIX
	//if (LocalPlayer.CanShiftShot && variable::get().ragebot.exploits.b_multi_tap.b_state)
	//{
	//	if (m_Playerrecord->GetOldestValidRecord())
	//		m_BacktrackTick = m_Playerrecord->GetOldestValidRecord();
	//}
//#endif

	// setup target hitboxes
	SetupHitboxes();
	return true;
}

#if 1
std::mutex parallel_mutex;
#endif

void __cdecl Parallel_Scan_Multipoint(multipoint_parameters_t& info)
{
	// check if something wants us to stop completely

	//parallel_mutex.lock();
	//printf("Parallel_Scan_Multipoint called with thread id %i\n", GetCurrentThreadId());
	//parallel_mutex.unlock();

	multipoint_parameters_t* volatile pInfo = &info;

	if (!g_AutoBone.m_bStopHitscan)
	{
		if (variable::get().visuals.b_aimbot_points)
		{
			parallel_mutex.lock();
			if (pInfo->IsMultipoint)
			{
				Interfaces::DebugOverlay->AddBoxOverlay(pInfo->BonePos, pInfo->DrawHitboxMinsBox, pInfo->DrawHitboxMaxsBox, pInfo->UnmodifiedEyeAngles, 255, 0, 255, 255, Interfaces::Globals->interval_per_tick * 2.0f);
			}
			else
			{
				Interfaces::DebugOverlay->AddBoxOverlay(pInfo->BonePos, pInfo->DrawHitboxMinsBox, pInfo->DrawHitboxMaxsBox, pInfo->UnmodifiedEyeAngles, 0, 255, 255, 255, Interfaces::Globals->interval_per_tick * 2.0f);
			}
			parallel_mutex.unlock();
		}
		// always scan center hitboxes
		if (!g_AutoBone.m_bStopMultipoint || !pInfo->IsMultipoint)
		{
			float _TestDamage = 0.0f;
			int _HitboxHit, _HitgroupHit;
			bool _Penetrated = false;

			// is bone visible
			if (BoneVisible(pInfo->Hitbox, pInfo->BonePos, pInfo->Entity, _TestDamage, _HitboxHit, _HitgroupHit, pInfo->NoRecoilAngles, _Penetrated))
			{
				if (pInfo->Hitbox == HITBOX_HEAD && ++g_AutoBone.m_iNumHeadPointsScanned == g_AutoBone.m_iNumHeadPoints)
					g_AutoBone.m_bFinishedScanningHead = true;
				// save hitbox pos
				// FIXME PERFORMANCE OPTIMIZATION: Should we do this outside of the thread?
				g_AutoBone.SaveBestHitboxPos(pInfo->Hitbox, _HitboxHit, _HitgroupHit, _TestDamage, pInfo->BonePos, pInfo->IsMultipoint, _Penetrated);

				// mark has hittable
				pInfo->CanHit = true;
			}
			else
			{
				//done after scanning due to multithreading
				if (pInfo->Hitbox == HITBOX_HEAD && ++g_AutoBone.m_iNumHeadPointsScanned == g_AutoBone.m_iNumHeadPoints)
					g_AutoBone.m_bFinishedScanningHead = true;
			}
		}
	}

	// mark thread as completed
#if 1
	//parallel_mutex.lock();
	//printf("Parallel_Scan_Multipoint finished thread id %i\n", GetCurrentThreadId());
	//parallel_mutex.unlock();
#endif
}

bool CAutoBone::CreateAndTracePoints()
{
	START_PROFILING

	SmallForwardBuffer _multipoints;

	for (int _Hitbox = HITBOX_HEAD; _Hitbox < HITBOX_MAX; ++_Hitbox)
	{
		if (!m_Hitboxes[_Hitbox].m_bTarget)
			continue;

		bool _ShouldUseBoneID;

		switch (_Hitbox)
		{
			//special case hitboxes
			case HITBOX_LEFT_FOREARM:
			case HITBOX_RIGHT_FOREARM:
			case HITBOX_LEFT_THIGH:
			case HITBOX_RIGHT_THIGH:
				_ShouldUseBoneID = true;
				break;
			//do not aim at these
			case HITBOX_LOWER_NECK:
			case HITBOX_LEFT_UPPER_ARM:
			case HITBOX_RIGHT_UPPER_ARM:
			case HITBOX_LEFT_CALF:
			case HITBOX_RIGHT_CALF:
			case HITBOX_UPPER_CHEST:
			//case HITBOX_CHEST:
				continue;

			//hitboxes that should use normal behavior
			default:
				_ShouldUseBoneID = false;
				break;
		};

		// init draw helpers
		const Vector DrawHitboxMinsBox = { -0.5f, -0.5f, -0.5f };
		const Vector DrawHitboxMaxsBox = { 0.5f, 0.5f, 0.5f };
		const QAngle _UnmodifiedEyeAngles = *m_pEntity->EyeAngles();

		multipoint_parameters_t parameters;
		parameters.DrawHitboxMinsBox = DrawHitboxMinsBox;
		parameters.DrawHitboxMaxsBox = DrawHitboxMaxsBox;
		parameters.UnmodifiedEyeAngles = _UnmodifiedEyeAngles;
		parameters.Entity = m_pEntity;
		parameters.NoRecoilAngles = &m_NoRecoilAngles;
		parameters.Hitbox = _Hitbox;
		parameters.CanHit = false;
		parameters.IsMultipoint = false;

		bool _multipoint = m_Hitboxes[_Hitbox].m_bMultipoint;
		if (_multipoint && _ShouldUseBoneID)
		{
			//shoulders and knees have a separate codepath with less calculations

			int bone = -1;
			switch (_Hitbox)
			{
				//SHOULDERS
				case HITBOX_LEFT_FOREARM:
				{
					//decrypts(0)
					bone = m_pEntity->LookupBone((char*)XorStr("arm_upper_l"));
					//encrypts(0)
					break;
				}
				case HITBOX_RIGHT_FOREARM:
				{
					//decrypts(0)
					bone = m_pEntity->LookupBone((char*)XorStr("arm_upper_r"));
					//encrypts(0)
					break;
				}
				//KNEES
				case HITBOX_LEFT_THIGH:
				{
					//decrypts(0)
					bone = m_pEntity->LookupBone((char*)XorStr("leg_lower_l"));
					//encrypts(0)
					break;
				}
				case HITBOX_RIGHT_THIGH:
				{
					//decrypts(0)
					bone = m_pEntity->LookupBone((char*)XorStr("leg_lower_r"));
					//encrypts(0)
					break;
				}
			};
	
			if (bone != -1)
			{
				CStudioHdr* hdr = m_pEntity->GetModelPtr();
				QAngle ang;
				matrix3x4_t bonetoworld;
				m_pEntity->GetCachedBoneMatrix(bone, bonetoworld);
				MatrixAngles(bonetoworld, ang, parameters.BonePos);
				_multipoints.write< multipoint_parameters_t >(parameters);
			}

			continue;
		}
		else if (!m_bCanMultipoint || m_bStopMultipoint)
		{
			_multipoint = false;
		}

		Vector _minsmaxs[2];
		matrix3x4_t _rotatedMatrix;
		float _radius;

		// skip if we couldn't get hitboxpos
		if (!m_pEntity->GetBoneTransformed(_Hitbox, &_rotatedMatrix, &_radius, _minsmaxs, nullptr))
			continue;

		auto &var = variable::get();

		// init locals
		Vector vMin, vMax;

		if (_radius != -1.0f)
		{
			//sphere
			VectorTransformZ(_minsmaxs[0], _rotatedMatrix, vMin);
			VectorTransformZ(_minsmaxs[1], _rotatedMatrix, vMax);
		}
		else
		{
			//bbox
			TransformAABB(_rotatedMatrix, _minsmaxs[0], _minsmaxs[1], vMin, vMax);
		}

		// get hitbox center point
		Vector vCenter = (vMin + vMax) * 0.5f, forward;
		Vector center = (vMin + vMax) * 0.5f;

		// get forward from Hitbox
		m_pEntity->GetDirectionFromHitbox(&forward, nullptr, nullptr, _Hitbox);

		// get studiobox
		mstudiobbox_t* _hitbox = m_pEntity->GetModelPtr()->_m_pStudioHdr->pHitboxSet(m_pEntity->GetHitboxSet())->pHitbox(_Hitbox);//Interfaces::ModelInfoClient->GetStudioModel(m_pEntity->GetModel())->pHitboxSet(m_pEntity->GetHitboxSet())->pHitbox(_Hitbox);

		if (_radius < 0.0f)
			_radius = 0.0f;

		// get adjusted center point
		Vector _top = vMax + forward * _radius;
		Vector _bottom = vMin - forward * _radius;
		Vector vAdjustedCenter = (_top + _bottom) * 0.5f;

		// FIXME: this is calculating from enemy hitbox center to our shoot position! Is this intentional?
		QAngle curAngles = CalcAngle(center, LocalPlayer.ShootPosition);

		Vector forward_angle;
		AngleVectors(curAngles, &forward_angle);

		// enemy eyeangles
		QAngle _EyeAngles = { _UnmodifiedEyeAngles.x, 0.0f, _UnmodifiedEyeAngles.z };

		// get at enemy angle
		QAngle _AtEnemy = CalcAngle(Vector(0.0f, LocalPlayer.ShootPosition.y, vCenter.z), vCenter);

		// check our relative rotation towards the entity
		const float FOV = GetFov(_AtEnemy, _EyeAngles);
		const bool bIsForwardOrBack = FOV <= 45.0f || FOV >= 135.0f;

		bool bUseV2Multipoints = true;

		// calculate pointscale

		std::vector<Vector> vecArray;

		float _pointScale = 0.f;

		if (_Hitbox == HITBOX_HEAD && var.ragebot.b_use_alternative_multipoint)
		{
			_pointScale = var.ragebot.hitscan_head.f_pointscale / 100.0f;
		}
		else  if (_Hitbox == HITBOX_HEAD)
		{
			_pointScale = clamp(var.ragebot.hitscan_head.f_pointscale / 100.0f, 0.65f, 1.0f);
			++m_iNumHeadPoints;
			bUseV2Multipoints = false;
			_pointScale = fmaxf(_radius, 1.0f) * _pointScale;
		}
		else
		{
			switch (_Hitbox)
			{
			case HITBOX_LOWER_NECK:
			default:
				break;
			case HITBOX_UPPER_CHEST:
				_pointScale = var.ragebot.b_use_alternative_multipoint ? var.ragebot.hitscan_chest.f_pointscale / 100.0f : 6.0f * (var.ragebot.hitscan_chest.f_pointscale / 100.0f);
				break;
			case HITBOX_THORAX:
				_pointScale = var.ragebot.b_use_alternative_multipoint ? var.ragebot.hitscan_chest.f_pointscale / 100.0f : 4.0f * (var.ragebot.hitscan_chest.f_pointscale / 100.0f);
				break;
			case HITBOX_PELVIS:
				_pointScale = var.ragebot.b_use_alternative_multipoint ? var.ragebot.hitscan_stomach.f_pointscale / 100.0f : 4.0f * (var.ragebot.hitscan_stomach.f_pointscale / 100.0f);
				break;
			case HITBOX_LEFT_FOREARM:
			case HITBOX_RIGHT_FOREARM:
				_pointScale = var.ragebot.b_use_alternative_multipoint ? var.ragebot.hitscan_arms.f_pointscale / 100.0f : 1.8f * (var.ragebot.hitscan_arms.f_pointscale / 100.0f);
				break;
			case HITBOX_LEFT_CALF:
			case HITBOX_RIGHT_CALF:
				_pointScale = var.ragebot.b_use_alternative_multipoint ? var.ragebot.hitscan_legs.f_pointscale / 100.0f : 2.3f * (var.ragebot.hitscan_legs.f_pointscale / 100.0f);
				break;
			case HITBOX_LEFT_FOOT:
			case HITBOX_RIGHT_FOOT:
				_pointScale = var.ragebot.b_use_alternative_multipoint ? var.ragebot.hitscan_feet.f_pointscale / 100.0f : 2.3f * (var.ragebot.hitscan_feet.f_pointscale / 100.0f);
				break;
			};
		}

		// add center point

		parameters.BonePos = var.ragebot.b_use_alternative_multipoint ? center : vAdjustedCenter;
		_multipoints.write< multipoint_parameters_t >(parameters);

		Vector right = forward_angle.Cross(Vector(0, 0, 1));
		Vector left = Vector(-right.x, -right.y, right.z);

		Vector top = Vector(0, 0, 1);
		Vector bot = Vector(0, 0, -1);

		// add others if needed
		if (_multipoint && _pointScale != 0.0f)
		{
			parameters.IsMultipoint = true;

			if (var.ragebot.b_use_alternative_multipoint)
			{
				if (_Hitbox == HITBOX_HEAD)
				{
					for (auto i = 0; i < 4; ++i)
					{
						vecArray.emplace_back(center);
					}

					vecArray[1] += top * (_hitbox->radius * _pointScale);
					vecArray[2] += right * (_hitbox->radius * _pointScale);
					vecArray[3] += left * (_hitbox->radius * _pointScale);
				}
				else
				{
					for (auto i = 0; i < 2; ++i)
					{
						vecArray.emplace_back(center);
					}
					vecArray[0] += right * (_hitbox->radius * _pointScale);
					vecArray[1] += left * (_hitbox->radius * _pointScale);
				}

				// add them
				for (Vector cur : vecArray)
				{
					parameters.BonePos = cur;
					_multipoints.write< multipoint_parameters_t >(parameters);
				}

			}
			else if (bUseV2Multipoints)
			{
				Vector _vecRight;
				AngleVectors(CalcAngle(LocalPlayer.ShootPosition, vCenter), nullptr, &_vecRight, nullptr);
				Vector _nudgeAmount = _vecRight * _pointScale;

				parameters.BonePos = vCenter + _nudgeAmount;
				_multipoints.write< multipoint_parameters_t >(parameters);
				parameters.BonePos = vCenter - _nudgeAmount;
				_multipoints.write< multipoint_parameters_t >(parameters);

				if (_Hitbox == HITBOX_HEAD)
					m_iNumHeadPoints += 2;
			}
			else
			{
				// init locals
				Vector points[8];
				float flHalfRadius = _pointScale * 0.5f;
				float flCenterRadiusSum = vCenter.z + _pointScale;
				float flCenterRadiusDifference = vCenter.z - _pointScale;

				// we're not looking at the entities sides
				if (bIsForwardOrBack)
				{
					if (_AtEnemy.y > 0.f)
					{
						//front of skull top right
						points[0] = { vMax.x + flHalfRadius, vMax.y - flHalfRadius, flCenterRadiusSum };
						//front of skull top left
						points[1] = { vMax.x + flHalfRadius, vMax.y + flHalfRadius, flCenterRadiusSum };
						//front of skull bottom right
						points[2] = { vMin.x + flHalfRadius, vMin.y - flHalfRadius, flCenterRadiusDifference };
						//front of skull bottom left
						points[3] = { vMin.x + flHalfRadius, vMin.y + flHalfRadius, flCenterRadiusDifference };
					}
					else
					{
						//back of skull top left
						points[0] = { vMax.x - flHalfRadius, vMax.y + flHalfRadius, flCenterRadiusSum };
						//back of skull top right
						points[1] = { vMax.x - flHalfRadius, vMax.y - flHalfRadius, flCenterRadiusSum };
						//back of skull bottom left
						points[2] = { vMin.x - flHalfRadius, vMin.y + flHalfRadius, flCenterRadiusDifference };
						//back of skull bottom right
						points[3] = { vMin.x - flHalfRadius, vMin.y - flHalfRadius, flCenterRadiusDifference };
					}
				}
				// lookint at their sides
				else
				{
					if (_AtEnemy.y > 0.f)
					{
						//back of skull top right
						points[0] = { vMax.x - flHalfRadius, vMax.y - flHalfRadius, flCenterRadiusSum };
						//front of skull top right
						points[1] = { vMax.x + flHalfRadius, vMax.y - flHalfRadius, flCenterRadiusSum };
						//back of skull bottom right
						points[2] = { vMin.x - flHalfRadius, vMin.y - flHalfRadius, flCenterRadiusDifference };
						//front of skull bottom right
						points[3] = { vMin.x + flHalfRadius, vMin.y - flHalfRadius, flCenterRadiusDifference };
					}
					else
					{
						//front of skull top left
						points[0] = { vMax.x + flHalfRadius, vMax.y + flHalfRadius, flCenterRadiusSum };
						//back of skull top left
						points[1] = { vMax.x - flHalfRadius, vMax.y + flHalfRadius, flCenterRadiusSum };
						//front of skull bottom left
						points[2] = { vMin.x + flHalfRadius, vMin.y + flHalfRadius, flCenterRadiusDifference };
						//back of skull bottom left
						points[3] = { vMin.x - flHalfRadius, vMin.y + flHalfRadius, flCenterRadiusDifference };
					}
				}

				if (!var.ragebot.b_use_alternative_multipoint)
				{
					//bottom
					points[4] = vMin - forward * _pointScale;

					// top
					if (_pointScale == 1.f)
						points[5] = vMax + forward * .9f;
					else
						points[5] = vMax + forward * _pointScale;

					if (_Hitbox == HITBOX_LEFT_FOOT || _Hitbox == HITBOX_RIGHT_FOOT)
					{
						//Local space points
						const Vector& lMin = _minsmaxs[0];
						const Vector& lMax = _minsmaxs[1];
						const Vector vLocalCenter = (lMin + lMax) * 0.5f;

						Vector vecPoint = { vLocalCenter.x + (lMax.x - vLocalCenter.x), vLocalCenter.y, vLocalCenter.z };
						VectorRotate(vecPoint, _rotatedMatrix, points[6]);
						points[6] += _rotatedMatrix.GetOrigin();

						vecPoint = { vLocalCenter.x + (lMin.x - vLocalCenter.x), vLocalCenter.y, vLocalCenter.z };
						VectorRotate(vecPoint, _rotatedMatrix, points[7]);
						points[7] += _rotatedMatrix.GetOrigin();
					}
					else
					{
						if (bIsForwardOrBack)
						{
							points[6] = { vAdjustedCenter.x, vAdjustedCenter.y + _pointScale * .75f, vAdjustedCenter.z };
							points[7] = { vAdjustedCenter.x, vAdjustedCenter.y - _pointScale * .75f, vAdjustedCenter.z };
						}
						else
						{
							points[6] = { vAdjustedCenter.x + _pointScale * .75f, vAdjustedCenter.y, vAdjustedCenter.z };
							points[7] = { vAdjustedCenter.x - _pointScale * .75f, vAdjustedCenter.y, vAdjustedCenter.z };
						}
					}

					// add them
					for (const auto& point : points)
					{
						parameters.BonePos = point;
						_multipoints.write< multipoint_parameters_t >(parameters);
					}

					if (_Hitbox == HITBOX_HEAD)
						m_iNumHeadPoints += 8;
				}
			}
		}
	}

	const unsigned _NumPoints = (const unsigned)_multipoints.getcount<multipoint_parameters_t>();

	END_PROFILING

	//decrypts(0)
	START_PROFILING_CUSTOM(parallelscanmultipoint, XorStr("Parallel_Scan_Multipoint"));
	//encrypts(0)

	ParallelProcess(_multipoints.getdata<multipoint_parameters_t>(), _NumPoints, &Parallel_Scan_Multipoint);

	END_PROFILING_CUSTOM(parallelscanmultipoint)

	bool _foundHittablePoint = false;

	for (multipoint_parameters_t* param = _multipoints.begin<multipoint_parameters_t>(); param != _multipoints.end<multipoint_parameters_t>(); ++param)
	{
		if (param->CanHit)
		{
			_foundHittablePoint = true;
			break;
		}
	}

	//Only count points as hittable if both sides line up
	if (variable::get().ragebot.b_safe_point)
	{
		/*
		ResolveSides OppositeResolveSide = m_BacktrackTick->m_iOppositeResolveSide;
		if (OppositeResolveSide == ResolveSides::INVALID_RESOLVE_SIDE || OppositeResolveSide == m_BacktrackTick->m_iResolveSide || OppositeResolveSide == ResolveSides::NONE)
			OppositeResolveSide = ResolveSides::POSITIVE_60;
		
		if (fabsf(AngleDiff(m_BacktrackTick->m_flPredictedGoalFeetYaw[OppositeResolveSide], m_BacktrackTick->m_AbsAngles.y)) < 5.0f)
		{
			bool FoundLargeDelta = false;
			float LargestDelta = 0.0f;
			ResolveSides BestSide = ResolveSides::INVALID_RESOLVE_SIDE;

			for (int i = 0; i < MAX_RESOLVE_SIDES; ++i)
			{
				if (i == m_BacktrackTick->m_iResolveSide || i == OppositeResolveSide)
					continue;
				
				float Delta = fabsf(AngleDiff(m_BacktrackTick->m_flPredictedGoalFeetYaw[i], m_BacktrackTick->m_AbsAngles.y));
				if (Delta >= LargestDelta)
				{
					BestSide = (ResolveSides)i;
					LargestDelta = Delta;
				}
			}

			if (BestSide != ResolveSides::INVALID_RESOLVE_SIDE)
				OppositeResolveSide = BestSide;
		}
		*/

		if (_foundHittablePoint && /*OppositeResolveSide != m_BacktrackTick->m_iResolveSide &&*/ (!m_Playerrecord->m_bLegit || m_Playerrecord->m_bForceNotLegit))
		{
			PlayerBackup_t *backup = new PlayerBackup_t(m_pEntity);
			if (backup)
			{
				//m_pEntity->DrawHitboxesFromCache(ColorRGBA(0, 255, 0, 255), TICKS_TO_TIME(2), m_pEntity->GetBoneAccessor()->GetBoneArrayForWrite());
				
				for (ResolveSides OppositeResolveSide = (ResolveSides)0; OppositeResolveSide < MAX_RESOLVE_SIDES; OppositeResolveSide = (ResolveSides)((int)OppositeResolveSide + 1))
				{
					if (OppositeResolveSide == m_BacktrackTick->m_iResolveSide)
						continue;

					m_pEntity->WritePoseParameters(m_BacktrackTick->m_flPredictedPoseParameters[OppositeResolveSide]);
					m_pEntity->SetAbsAnglesDirect(QAngle(0.0f, m_BacktrackTick->m_flPredictedGoalFeetYaw[OppositeResolveSide], 0.0f));

					AllowSetupBonesToUpdateAttachments = false;
					m_pEntity->InvalidateBoneCache();
					m_pEntity->SetLastOcclusionCheckFlags(0);
					m_pEntity->SetLastOcclusionCheckFrameCount(Interfaces::Globals->framecount);
					m_pEntity->ReevaluateAnimLOD();
					m_pEntity->SetupBonesRebuilt(nullptr, -1, BONE_USED_BY_HITBOX, Interfaces::Globals->curtime);

					//m_pEntity->DrawHitboxesFromCache(ColorRGBA(255, 0, 0, 255), TICKS_TO_TIME(2), m_pEntity->GetBoneAccessor()->GetBoneArrayForWrite());

					for (auto& info : m_BacktrackTick->m_BestHitbox)
					{
						if (info.bsaved && (info.index != HITBOX_HEAD || variable::get().ragebot.b_safe_point_head))
						{
							trace_t tr;
							Ray_t ray;
							Vector vecDir = info.origin - LocalPlayer.ShootPosition;
							VectorNormalizeFast(vecDir);
							ray.Init(LocalPlayer.ShootPosition, info.origin + vecDir * 40.0f);
							Interfaces::EngineTrace->ClipRayToEntity(ray, MASK_SHOT, (IHandleEntity*)m_pEntity, &tr);

							if (tr.m_pEnt != m_pEntity)
							{
								info.bsaved = false;
								//Interfaces::DebugOverlay->AddBoxOverlay(info.origin, Vector(-0.25, -0.25, -0.25), Vector(0.25, 0.25, 0.25), angZero, 255, 0, 0, 255, TICKS_TO_TIME(2));
								//Interfaces::DebugOverlay->AddTextOverlay(info.origin, TICKS_TO_TIME(10), "EXCLUDE");
							}
						}
					}
				}

				backup->RestoreData();
				delete backup;
			}
		}
	}

	return _foundHittablePoint;
}

bool CAutoBone::RunHitscan(bool MinimumPointsOnly)
{
	START_PROFILING

	m_Playerrecord->CM_BacktrackPlayer(m_BacktrackTick);

	SetupHitboxes(false, MinimumPointsOnly);

	// have we found a point to target
	bool _canHit = false;

	// don't stop hitscan yet
	m_bStopHitscan = false;

	// don't stop multipoint yet
	m_bStopMultipoint = false;

	// reset scan head
	m_bFinishedScanningHead = false;
	m_iNumHeadPoints = 0;
	m_iNumHeadPointsScanned = 0;

	if (!m_BacktrackTick->m_bCachedBones)
	{
		m_Playerrecord->CacheBones(Interfaces::Globals->curtime, true, m_BacktrackTick);
	}
	else
	{
		//for some reason we stop shooting, the bones are not being setup/restored properly. set them up again
		m_Playerrecord->CacheBones(Interfaces::Globals->curtime, true, m_BacktrackTick);
	}

	if (CreateAndTracePoints())
		_canHit = true;

	// set this to not run hitscan when we don't have to
	m_BacktrackTick->m_bRanHitscan = true;
	m_BacktrackTick->GetLocalPlayerVars_Hitscan();

	END_PROFILING
	return _canHit;
}

float CAutoBone::GetMinDamageScaled(SavedHitboxPos* savedinfo)
{
	float scaled_damage = static_cast<float>(m_iWeaponDamage);
	scaled_damage *= powf(LocalPlayer.CurrentWeapon->GetCSWpnData()->flRangeModifier, ((savedinfo->origin - savedinfo->localplayer_shootpos).Length() * 0.002f));
	ScaleDamage(savedinfo->actual_hitgroup_due_to_penetration, m_BacktrackTick->m_pEntity, LocalPlayer.CurrentWeapon->GetCSWpnData()->flArmorRatio, scaled_damage);
	scaled_damage -= 1.0f;
	const float config_mindmg = savedinfo->penetrated ? static_cast<float>(variable::get().ragebot.i_mindmg_aw) : static_cast<float>(variable::get().ragebot.i_mindmg);

	scaled_damage = fmaxf(0.0f, fminf(scaled_damage, config_mindmg));

	// if the config damage was greater than our scaled damage, don't shoot.
	if (config_mindmg > scaled_damage)
		scaled_damage = savedinfo->damage + 1;

	return scaled_damage;
}

void CAutoBone::GetAngleToHitboxInfo(const Vector& HitboxPosition, QAngle& EyeAnglesToHitbox, QAngle& EyeAnglesToSend, QAngle& ServerFireBulletAngles)
{
	EyeAnglesToHitbox = CalcAngle(LocalPlayer.ShootPosition, (Vector&)HitboxPosition);
	NormalizeAngles(EyeAnglesToHitbox);
	EyeAnglesToHitbox.z = 0.0f;
	EyeAnglesToSend = EyeAnglesToHitbox;
	g_Removals.DoNoRecoil(EyeAnglesToSend);
	ServerFireBulletAngles = EyeAnglesToSend;
	g_Removals.AddRecoil(ServerFireBulletAngles);
}

bool CAutoBone::CanHitRecord()
{
	START_PROFILING

	m_Playerrecord->CM_BacktrackPlayer(m_BacktrackTick);

	//for some reason we stop shooting, the bones are not being setup/restored properly. set them up again
	m_Playerrecord->CacheBones(Interfaces::Globals->curtime, true, m_BacktrackTick);

	int reason = BAIM_REASON_NONE;
	int reason_head = HEAD_REASON_NONE;

	auto &var = variable::get();

	bool has_head = false, has_body = false;

	var.ragebot.is_hitscanning(&has_body, &has_head);

	const bool _prioritizeBody = ShouldPrioritizeBody(&reason);
	const bool _prioritizeHead = false;// ShouldPriorityHead(&reason_head);

	const bool force_baim = has_body && (var.ragebot.baim_main.b_after_misses && m_Playerrecord->m_iShotsMissed >= var.ragebot.baim_main.i_after_misses) || (m_bCanScanBody && var.ragebot.baim_main.b_force.get()) || (m_bCanScanBody && LocalPlayer.WeaponVars.IsTaser);
	const bool baim_if_lethal = m_bCanScanBody && var.ragebot.baim_main.b_lethal;
	const bool moving = m_BacktrackTick->m_bMoving;
	const bool ignore_limbs = var.ragebot.b_ignore_limbs_if_moving && m_BacktrackTick->m_AbsVelocity.Length() > var.ragebot.f_ignore_limbs_if_moving;
	
	const float _Health = (float)m_pEntity->GetHealth();

	m_Playerrecord->m_iBaimReason = reason;
	m_Playerrecord->m_iHeadReason = reason_head;

	// best hitboxes
	SavedHitboxPos *_hitboxBest = nullptr, *_hitboxBestBody = nullptr, *_hitboxBestHead = nullptr;
	SavedHitboxPos *_pelvisHitbox = &m_BacktrackTick->m_BestHitbox[HITBOX_PELVIS];

	int _hitboxNumBest = -1, _hitboxNumBestBody = -1, _hitboxNumBestHead = -1;

	const float _mindmg = static_cast<float>(var.ragebot.i_mindmg);
	const float _mindmg_aw = static_cast<float>(var.ragebot.i_mindmg_aw);
	
	if (m_bCanScanBody)
	{
		int last_body_hitbox = !ignore_limbs ? HITBOX_RIGHT_FOREARM : HITBOX_UPPER_CHEST;

		SavedHitboxPos * _finalHitbox = &m_BacktrackTick->m_BestHitbox[last_body_hitbox];

		for (SavedHitboxPos* _hitbox = &m_BacktrackTick->m_BestHitbox[HITBOX_PELVIS]; _hitbox <= _finalHitbox; ++_hitbox)
		{
			if (_hitbox->bsaved && HitboxStatsAreBetter(_hitboxBest, _hitbox))
			{
				_hitboxBest = _hitbox;
				_hitboxNumBest = _hitbox->index;
			}
		}

		//Override best body hitbox with pelvis if it is not too much different than the existing one
		if (_hitboxBest && _pelvisHitbox->bsaved && _hitboxBest != _pelvisHitbox)
		{
			bool _Lethal = _pelvisHitbox->damage >= _Health;
			if (_Lethal || fabsf(_pelvisHitbox->damage - _hitboxBest->damage) <= 8.0f)
			{
				//only if meets mindamage or is lethal
				if (_Lethal || _pelvisHitbox->damage >= GetMinDamageScaled(_pelvisHitbox))
				{
					_hitboxBest = _pelvisHitbox;
					_hitboxNumBest = HITBOX_PELVIS;

					if (_Lethal)
						m_Playerrecord->m_iBaimReason = BAIM_REASON_LETHAL;
				}
			}
		}

		_hitboxBestBody = _hitboxBest;
		_hitboxNumBestBody = _hitboxNumBest;
	}

	//now check the head
	if (!force_baim)
	{
		SavedHitboxPos * _finalHitbox = &m_BacktrackTick->m_BestHitbox[HITBOX_LOWER_NECK];
		for (SavedHitboxPos* _hitbox = &m_BacktrackTick->m_BestHitbox[HITBOX_HEAD]; _hitbox <= _finalHitbox; ++_hitbox)
		{
			if (_hitbox->bsaved)
			{
				if (!_prioritizeBody || !_hitboxBest || _prioritizeHead)
				{
					if (!baim_if_lethal || !_hitboxBest || _hitboxBest->damage < _Health)
					{
						if (HitboxStatsAreBetter(_hitboxBest, _hitbox))
						{
							_hitboxBest = _hitbox;
							_hitboxNumBest = _hitbox->index;

							m_Playerrecord->m_iBaimReason = BAIM_REASON_NONE;
							m_Playerrecord->m_iHeadReason = HEAD_REASON_HITBOX_BETTER;
						}
					}
				}

				if (HitboxStatsAreBetter(_hitboxBestHead, _hitbox))
				{
					_hitboxBestHead = _hitbox;
					_hitboxNumBestHead = _hitbox->index;
				}
			}
		}
	}

	// we have a hitbox and match the config settings
	if (_hitboxBest)
	{
		if (_hitboxBestHead && _hitboxBest >= &m_BacktrackTick->m_BestHitbox[HITBOX_PELVIS] && _prioritizeBody && m_Playerrecord->m_iBaimReason != BAIM_REASON_FORWARD_TICKBASE && m_Playerrecord->m_iBaimReason != BAIM_REASON_LOCAL_MULTITAP)
		{
			//If we are prioritizing body and head is SIGNIFICANTLY better than body, shoot at head anyway
			if (_hitboxBestBody->damage < _Health && (_hitboxBestHead->damage >= _Health || _hitboxBestHead->damage - _hitboxBestBody->damage >= 20.0f))
			{
				_hitboxBest = _hitboxBestHead;
				_hitboxNumBest = _hitboxBestHead->index;
				m_Playerrecord->m_iBaimReason = BAIM_REASON_NONE;
				m_Playerrecord->m_iHeadReason = HEAD_REASON_BETTER_DAMAGE;
			}
		}

		bool satisfies_hitchance = weapon_accuracy_nospread.GetVar()->GetInt() > 0 || LocalPlayer.WeaponWillFireBurstShotThisTick || LocalPlayer.WeaponVars.IsBurstableWeapon && LocalPlayer.CurrentWeapon->GetBurstShotsRemaining() > 0;
		
		_hitboxBest->hitchance = satisfies_hitchance ? 100.0f : 0.0f;

		if (_hitboxBest->damage >= GetMinDamageScaled(_hitboxBest) || _hitboxBest->damage >= m_pEntity->GetHealth())
		{
			QAngle EyeAnglesToHitbox, EyeAnglesToSend, ServerFireBulletAngles;
			GetAngleToHitboxInfo(_hitboxBest->origin, EyeAnglesToHitbox, EyeAnglesToSend, ServerFireBulletAngles);

			_hitboxBest->localplayer_eyeangles = EyeAnglesToHitbox;

			Vector backup1, backup2;
			LocalPlayer.CopyShootPositionVars(backup1, backup2);

			LocalPlayer.FixShootPosition(EyeAnglesToSend, true);
			Vector NewShootPos = LocalPlayer.Entity->Weapon_ShootPosition();

			LocalPlayer.WriteShootPositionVars(backup1, backup2);

			Autowall_Output_t output;
			Vector vecDir;
			AngleVectors(ServerFireBulletAngles, &vecDir);
			VectorNormalizeFast(vecDir);
			Vector NewEndPos = NewShootPos + vecDir * 8192.0f;
			
			//Interfaces::DebugOverlay->AddLineOverlay(NewShootPos, NewEndPos, 255, 0, 0, true, 0.1f);

			if (Autowall(NewShootPos, NewEndPos, output, false, false, m_pEntity) != m_pEntity)
			{
				//couldn't shoot the player anymore, just return false
				END_PROFILING
				return false;
			}

			// run hitchance
			if (!satisfies_hitchance)
			{
				//QAngle destAngle = CalcAngle(LocalPlayer.ShootPosition, _hitboxBest->origin);
				//g_Removals.DoNoRecoil(destAngle);

				float hitchance_to_use = LocalPlayer.UseDoubleTapHitchance ? var.ragebot.f_doubletap_hitchance : (_hitboxBest->actual_hitbox_due_to_penetration == HITBOX_HEAD || _hitboxBest->actual_hitbox_due_to_penetration == HITBOX_LOWER_NECK)
					? variable::get().ragebot.f_hitchance
					: variable::get().ragebot.f_body_hitchance;

				satisfies_hitchance = Hitchance(m_pEntity, EyeAnglesToSend, hitchance_to_use, HITGROUP_GENERIC, &_hitboxBest->hitchance);
			}

			bool is_head;
			if (is_head = (_hitboxNumBest == HITBOX_HEAD || _hitboxNumBest == HITBOX_LOWER_NECK))
				m_Playerrecord->m_iBaimReason = BAIM_REASON_BAD_HITCHANCE;

			if (satisfies_hitchance)
			{
				// save final hitbox
				m_pBestHitbox = _hitboxBest;
				m_iBestHitbox = _hitboxNumBest;

				if (is_head)
					m_Playerrecord->m_iBaimReason = BAIM_REASON_NONE;

				END_PROFILING
				return true;
			}
		}
		else if (_hitboxBest->index == HITBOX_HEAD || _hitboxBest->index == HITBOX_LOWER_NECK)
		{
			m_Playerrecord->m_iBaimReason = BAIM_REASON_NOT_ENOUGH_DAMAGE;
		}

		//find if another hitbox group that satisfies hitchance and mindamage if we couldn't hit the main one
		SavedHitboxPos *_testGroup = _hitboxBestHead;
		int _testIndex = _hitboxNumBestHead;
		bool _testIsBody = false;

		if (_hitboxNumBest == HITBOX_HEAD || _hitboxNumBest == HITBOX_LOWER_NECK)
		{
			_testGroup = _hitboxBestBody;
			_testIndex = _hitboxNumBestBody;
			_testIsBody = true;
		}

		if (_testGroup)
		{
			if (_testGroup->damage >= GetMinDamageScaled(_testGroup) || _testGroup->damage >= m_pEntity->GetHealth())
			{
				QAngle EyeAnglesToHitbox, EyeAnglesToSend, ServerFireBulletAngles;
				GetAngleToHitboxInfo(_testGroup->origin, EyeAnglesToHitbox, EyeAnglesToSend, ServerFireBulletAngles);

				_testGroup->localplayer_eyeangles = EyeAnglesToHitbox;

				Vector backup1, backup2;
				LocalPlayer.CopyShootPositionVars(backup1, backup2);

				LocalPlayer.FixShootPosition(EyeAnglesToSend, true);
				Vector NewShootPos = LocalPlayer.Entity->Weapon_ShootPosition();

				LocalPlayer.WriteShootPositionVars(backup1, backup2);

				Autowall_Output_t output;
				Vector vecDir;
				AngleVectors(ServerFireBulletAngles, &vecDir);
				VectorNormalizeFast(vecDir);
				Vector NewEndPos = NewShootPos + vecDir * 8192.0f;
				if (Autowall(NewShootPos, NewEndPos, output, false, false, m_pEntity) != m_pEntity)
				{
					//couldn't shoot the player anymore, just return false

					END_PROFILING
					return false;
				}
				
				satisfies_hitchance = weapon_accuracy_nospread.GetVar()->GetInt() > 0 || LocalPlayer.WeaponWillFireBurstShotThisTick || LocalPlayer.WeaponVars.IsBurstableWeapon && LocalPlayer.CurrentWeapon->GetBurstShotsRemaining() > 0;

				if (!satisfies_hitchance)
				{
					//QAngle destAngle = CalcAngle(LocalPlayer.ShootPosition, _testGroup->origin);
					//g_Removals.DoNoRecoil(destAngle);

					float hitchance_to_use = LocalPlayer.UseDoubleTapHitchance ? var.ragebot.f_doubletap_hitchance : (_testGroup->actual_hitbox_due_to_penetration == HITBOX_HEAD || _testGroup->actual_hitbox_due_to_penetration == HITBOX_LOWER_NECK)
						? variable::get().ragebot.f_hitchance
						: variable::get().ragebot.f_body_hitchance;

					satisfies_hitchance = Hitchance(m_pEntity, EyeAnglesToSend, hitchance_to_use, HITGROUP_GENERIC, &_testGroup->hitchance);
				}

				if (satisfies_hitchance)
				{
					// save final hitbox
					m_pBestHitbox = _testGroup;
					m_iBestHitbox = _testIndex;

					if (!_testIsBody)
						m_Playerrecord->m_iBaimReason = BAIM_REASON_NONE;

					END_PROFILING
					return true;
				}
			}
		}
	}

	END_PROFILING
	return false;
}

bool CAutoBone::ShouldPriorityHead(int* reason)
{
	if (!m_bCanScanHead)
		return false;

	auto* _BodyResolveInfo = m_Playerrecord->GetBodyHitResolveInfo();

	if (_BodyResolveInfo && _BodyResolveInfo->m_bIsBodyHitResolved)
	{
		if (reason != nullptr)
		{
			*reason = HEAD_REASON_BLOOD;
		}
		return true;
	}

	return false;
}

bool CAutoBone::ShouldPrioritizeBody(int* reason)
{
	if (!m_bCanScanBody)
		return false;

	/*
		A lot of this is questionable seeing as we can now hit moving a lot of the time should we really priority body aim if they are tickbase etc
	*/

	auto& var = variable::get();

	// -- body aim conditions --

	// prioritize body if we missed too often

	if (var.ragebot.baim_main.b_after_health && m_pEntity->GetHealth() <= var.ragebot.baim_main.body_aim_health && m_bCanHitscan)
	{
		if (reason != nullptr)
		{
			*reason = BAIM_REASON_HEALTH;
		}

		return true;
	}

	if (var.ragebot.baim_main.b_after_misses && m_Playerrecord->m_iShotsMissed >= var.ragebot.baim_main.i_after_misses && m_bCanScanBody)
	{
		if (reason != nullptr)
		{
			*reason = BAIM_REASON_TOO_MANY_MISSES;
		}
		return true;
	}

	if (var.ragebot.baim_main.b_force.get())
	{
		if (reason != nullptr)
		{
			*reason = BAIM_REASON_FORCE;
		}
		return true;
	}

	// when we're in air - this is the most retardest thing I have ever seen 
	if (var.ragebot.baim_main.b_airborne_local && LocalPlayer.Entity->IsInAir())
	{
		if (reason != nullptr)
		{
			*reason = BAIM_REASON_AIRBORNE_LOCAL;
		}
		return true;
	}

	// when they're in air
	if (var.ragebot.baim_main.b_airborne_target && m_pEntity->IsInAir())
	{
		if (reason != nullptr)
		{
			*reason = BAIM_REASON_AIRBORNE_TARGET;
		}
		return true;
	}

	// when they're moving and target speed matches
	if (var.ragebot.baim_main.b_moving_target && m_pEntity->GetVelocity().Length() > var.ragebot.baim_main.f_moving_target)
	{
		if (reason != nullptr)
		{
			*reason = BAIM_REASON_MOVING_TARGET;
		}
		return true;
	}

	// prioritize body if they shifted tickbase forwards
#if 0
	if (m_BacktrackTick->m_bTickbaseShiftedForwards)
	{
		if (reason != nullptr)
		{
			*reason = BAIM_REASON_FORWARD_TICKBASE;
		}
		return true;
	}
#endif


	bool is_multi_tapping = (g_Tickbase.m_bReadyToShiftTickbase && Interfaces::Globals->realtime >= g_Tickbase.m_flDelayTickbaseShiftUntilThisTime); // we shoud priority body aim if we are multi tapping

	if (is_multi_tapping && var.ragebot.exploits.b_multi_tap.b_state)
	{
		if (reason != nullptr)
		{
			*reason = BAIM_REASON_LOCAL_MULTITAP;
		}
		return true;
	}

	// -- force for the head --  

	// don't prioritize body if we resolved correctly
	if (m_BacktrackTick->IsRealTick() || (m_Playerrecord->m_bLegit && !m_Playerrecord->m_bForceNotLegit))
	{
		if (reason != nullptr)
		{
			*reason = BAIM_REASON_HEAD_RESOLVED;
		}
		return false;
	}

	// don't prioritize body if player shot
#if 0
	if (m_BacktrackTick->m_bFiredBullet) //this is questionable
	{
		if (reason != nullptr)
		{
			*reason = BAIM_REASON_HEAD_FIRED_BULLET;
		}
		return false;
	}
#endif

	if (reason != nullptr)
	{
		*reason = BAIM_REASON_NONE;
	}

	return false;
}

bool CAutoBone::HitboxStatsAreBetter(SavedHitboxPos *existing, SavedHitboxPos* newhitbox)
{
	float _health = (float)m_pEntity->GetHealth();

	if (!existing || !existing->bsaved)
	{
		return true;
	}
	else if (newhitbox->damage >= existing->damage)
	{
		//always override existing multipoints if this is better
		if (existing->bismultipoint)
		{
			//prefer head instead of neck unless damage is significantly better
			if (existing->index == HITBOX_HEAD && newhitbox->index == HITBOX_LOWER_NECK)
			{
				if (newhitbox->damage - existing->damage > 6.0f || (newhitbox->damage >= _health && existing->damage < _health))
					return true;

				return false;
			}
			return true;
		}
		else
		{
			//only override existing point if the new damage is significantly better or the the new point is lethal and the original isn't
			if (newhitbox->damage - existing->damage > 6.0f || (newhitbox->damage >= _health && existing->damage < _health))
				return true;
		}
	}
	else if (!newhitbox->bismultipoint)
	{
		if (existing->index == HITBOX_HEAD && newhitbox->index == HITBOX_LOWER_NECK)
		{
			//prefer head over neck regardless of centerness
		}
		else
		{
			//override the existing point if this is not a multipoint and it's not too much worse than the multipoint

			if (newhitbox->damage >= _health)
				return true;

			if (existing->damage - newhitbox->damage <= 6.0f)
			{
				if (newhitbox->damage >= GetMinDamageScaled(newhitbox))// || existing->damage < _destmindamage)
					return true;
			}
		}
	}
	return false;
}

void CAutoBone::SaveBestHitboxPos(int hitboxid, int hitboxhit, int hitgrouphit, float fldamage, const Vector& origin, bool ismultipoint, bool penetrated)
{
	m_SaveHitboxMutex.lock();

	if (hitboxid == HITBOX_HEAD || hitboxid == HITBOX_LOWER_NECK)
	{
		m_BacktrackTick->m_bHeadIsVisible = true;

		if (!m_BacktrackTick->m_bHasHeadNotBehindWall)
			m_BacktrackTick->m_bHasHeadNotBehindWall = !penetrated;
	}
	else
	{
		m_BacktrackTick->m_bBodyIsVisible = true;

		if (!m_BacktrackTick->m_bHasBodyPartNotBehindWall)
			m_BacktrackTick->m_bHasBodyPartNotBehindWall = !penetrated;
	}

	SavedHitboxPos* dest = &m_BacktrackTick->m_BestHitbox[hitboxid];
	SavedHitboxPos newhitbox;
	newhitbox.index = hitboxid;
	newhitbox.bismultipoint = ismultipoint;
	newhitbox.penetrated = penetrated;
	//newhitbox.actual_hitbox_due_to_penetration = hitboxhit;
	//newhitbox.actual_hitgroup_due_to_penetration = hitgrouphit;
	newhitbox.damage = fldamage;
	//newhitbox.origin = origin;

	float _health = (float)m_pEntity->GetHealth();
	//float _basedamage = fmaxf(0.0f, (float)g_AutoBone.m_iWeaponDamage - 5.0f);

	if (HitboxStatsAreBetter(dest, &newhitbox))
	{
		//save this hitbox
		dest->index = hitboxid;
		dest->penetrated = penetrated;
		dest->bsaved = true;
		dest->forwardtracked = false;
		dest->bismultipoint = ismultipoint;
		dest->origin = origin;
		dest->damage = fldamage;
		dest->actual_hitbox_due_to_penetration = hitboxhit;
		dest->actual_hitgroup_due_to_penetration = hitgrouphit;
		dest->localplayer_shootpos = LocalPlayer.ShootPosition;
		dest->localplayer_origin = LocalPlayer.Entity->GetLocalOrigin();
		dest->localplayer_eyeangles = CalcAngle(LocalPlayer.ShootPosition, (Vector&)origin);
		dest->localplayer_eyeangles.z = 0.0f;
		NormalizeAngles(dest->localplayer_eyeangles);
	}

	if (fldamage > m_BacktrackTick->m_flBestHitboxDamage)
	{
		m_BacktrackTick->m_flBestHitboxDamage = fldamage;

		float scaled_damage = static_cast<float>(m_iWeaponDamage);
		scaled_damage *= powf(LocalPlayer.CurrentWeapon->GetCSWpnData()->flRangeModifier, ((dest->origin - dest->localplayer_shootpos).Length() * 0.002f));
		ScaleDamage(dest->actual_hitgroup_due_to_penetration, m_BacktrackTick->m_pEntity, LocalPlayer.CurrentWeapon->GetCSWpnData()->flArmorRatio, scaled_damage);
		scaled_damage -= 1.0f;

		const float config_mindmg = dest->penetrated ? static_cast<float>(variable::get().ragebot.i_mindmg_aw) : static_cast<float>(variable::get().ragebot.i_mindmg);

		scaled_damage = fmaxf(0.0f, fminf(scaled_damage, config_mindmg));

		// if the config damage was greater than our scaled damage, don't shoot.
		if (config_mindmg > scaled_damage)
			scaled_damage = fldamage + 1;

		m_BacktrackTick->m_flBestScaledMinDamage = scaled_damage;
	}

	// stop multipoint if we already did more than the base weapon damage
	float mindamage = GetMinDamageScaled(dest);
	if (fldamage >= mindamage)
	{
		//float multiplier = ((float)g_AutoBone.m_iWeaponDamage * 3.0f);
		//always multipoint the head if we didn't get a head multiplier
		if (hitboxid == HITBOX_HEAD)
		{
			if (g_AutoBone.m_iNumHeadPointsScanned == g_AutoBone.m_iNumHeadPoints)
			{
				g_AutoBone.m_bStopMultipoint = true;
				g_AutoBone.m_bFinishedScanningHead = true;
			}
			//float multiplier = ((float)g_AutoBone.m_iWeaponDamage * 3.0f);
			//if (fldamage >= multiplier)
			//{
				//g_AutoBone.m_bStopMultipoint = true;
				//g_AutoBone.m_bFinishedScanningHead = true;
			//}

			//if (g_AutoBone.m_iNumHeadPointsScanned == g_AutoBone.m_iNumHeadPoints)
			//	g_AutoBone.m_bFinishedScanningHead = true;
		}
		else
		{
			if (g_AutoBone.m_bFinishedScanningHead)
			{
				if (fldamage >= mindamage || fldamage >= _health)
					g_AutoBone.m_bStopMultipoint = true;
			}
		}
	}

	m_SaveHitboxMutex.unlock();
}

bool CAutoBone::Run_AutoBone(CBaseEntity* _Entity)
{
	// init variables
	if (!Init(_Entity))
	{
		Profiling.Finish();
		return false;
	}

	START_PROFILING

	// start profiling
	Profiling.Run();

	// fps check yay
	m_bHittingTargetFPS = Interfaces::Globals->absoluteframetime + (Profiling.m_dbAvgTimeToRun * Profiling.m_iTimesRunThisFrame) <= TICK_INTERVAL;
	m_bCanHitscan = variable::get().ragebot.is_hitscanning(&m_bCanScanBody, &m_bCanScanHead);
	m_bCanMultipoint = variable::get().ragebot.is_multipointing();

	std::vector< CTickrecord* > _ExcludedRecords;

	const bool _IsUsingNetworkESP = m_Playerrecord->m_bIsUsingFarESP || m_Playerrecord->m_bIsUsingServerSide;

	// determine whether we can backtrack the player
	const bool _CanBacktrack = !m_Playerrecord->m_bTeleporting && !_IsUsingNetworkESP;

	bool _CanHitCurrentRecord = false;

	CTickrecord* _CurrentRecord = m_BacktrackTick;

	// always scan current record
	if (!m_BacktrackTick->m_bRanHitscan || !m_BacktrackTick->ScanStillValid())
	{
		//CTickrecord *_previousRecord;
		bool _ScanCurrentRecord = true;

#if 0
		if (!m_BacktrackTick->m_bRanHitscan && (_previousRecord = m_Playerrecord->GetPreviousRecord()) && _previousRecord->m_bRanHitscan)
		{
			//find out if this record is similar to the previous one and don't bother scanning if the previous one isn't hittable
			if (!_previousRecord->m_bBodyIsVisible && !_previousRecord->m_bHeadIsVisible)
			{
				if (m_BacktrackTick->m_DuckAmount == _previousRecord->m_DuckAmount
					&& (m_BacktrackTick->m_NetOrigin - _previousRecord->m_NetOrigin).Length() < 0.08f
					&& fabsf(AngleDiff(m_BacktrackTick->m_EyeAngles.x, _previousRecord->m_EyeAngles.x)) < 8.0f
					&& fabsf(AngleDiff(m_BacktrackTick->m_EyeAngles.y, _previousRecord->m_EyeAngles.y)) < 8.0f
					&& (LocalPlayer.ShootPosition - _previousRecord->LocalVarsScan.m_ShootPos).Length() < 0.5f)
				{
					m_BacktrackTick->m_bRanHitscan = true;
					m_BacktrackTick->m_bHeadIsVisible = false;
					m_BacktrackTick->m_bBodyIsVisible = false;
					m_BacktrackTick->LocalVarsScan = _previousRecord->LocalVarsScan;
					_ScanCurrentRecord = false;
				}
			}
		}
#endif

		if (_ScanCurrentRecord 
			&& (_IsUsingNetworkESP || m_BacktrackTick->GetWeight() >= 0)
			&& RunHitscan())
		{
			_CanHitCurrentRecord = true;
			g_Ragebot.m_bTracedEnemy = true;
		}
	}

	bool _ForwardTracked = false;

	// we can backtrack
	if (_CanBacktrack)
	{
		if (!(m_BacktrackTick = m_Playerrecord->GetBestRecord(true)))
		{
			//no records available at all

			if (g_Ragebot.ForwardTrack(m_Playerrecord, false, &_ForwardTracked))
			{
				g_Info.UsingForwardTrack = true;
				Profiling.Finish();
				END_PROFILING
				return true;
			}

			Profiling.Finish();
			END_PROFILING
			return false;
		}


		// this is the ideal best record available
		CTickrecord *_idealBacktrackTick = m_BacktrackTick;

		// find the best record that we can still hit without having to hitscan again
		while (m_BacktrackTick)
		{
			// exclude this record from future iterations
			_ExcludedRecords.push_back(m_BacktrackTick);

			// already scanned this record
			if (m_BacktrackTick->m_bRanHitscan && m_BacktrackTick->ScanStillValid())
			{
				// head or body visible and mindamage hit
				if ((m_BacktrackTick->m_bHeadIsVisible || m_BacktrackTick->m_bBodyIsVisible) && m_BacktrackTick->HasMinDamage())
				{
					g_Ragebot.m_bTracedEnemy = true;

					//If hitting target fps, run hitchance on all of the scanned hitboxes
					
					// restore player to selected tick
					m_Playerrecord->CM_BacktrackPlayer(m_BacktrackTick);
#if 0
					if (m_bHittingTargetFPS)
					{
						// run hitchance
						if (CanHitRecord())
						{
							// update esp position
							m_Playerrecord->m_vecLastBacktrackOrigin = m_BacktrackTick->m_AbsOrigin;

							// we can hit
							Profiling.Finish();
							END_PROFILING
							return true;
						}
					}
					else
#endif
					//when not hitting target fps, don't check any more records to save FPS
					{
						// run hitchance
						if (!CanHitRecord())
						{
							Profiling.Finish();
							END_PROFILING
							return false;
						}

						// we can hit
						Profiling.Finish();
						END_PROFILING
						return true;
					}
				}
			}

			//get the next best record
			m_BacktrackTick = m_Playerrecord->GetBestRecord(true, &_ExcludedRecords);
		}

		if (_idealBacktrackTick->m_bRanHitscan && _idealBacktrackTick->ScanStillValid())
		{
			// we are standing in the same position, hitscanning this again will do us no good

			if (!_idealBacktrackTick->m_bHeadIsVisible && !_idealBacktrackTick->m_bBodyIsVisible)
			{
				// only forward track if that record was not hittable
				if (g_Ragebot.ForwardTrack(m_Playerrecord, false, &_ForwardTracked))
				{
					g_Info.UsingForwardTrack = true;
					g_Ragebot.m_bTracedEnemy = true;
					Profiling.Finish();
					END_PROFILING
					return true;
				}
			}

			Profiling.Finish();
			END_PROFILING
			return false;
		}

#if 0
		CTickrecord *oldest_record = m_Playerrecord->GetOldestValidRecord();
		CTickrecord *newest_record = m_Playerrecord->GetNewestValidRecord();

		if (_idealBacktrackTick)
		{
			if (!_idealBacktrackTick->m_bHeadIsVisible && !_idealBacktrackTick->m_bBodyIsVisible)
			{
				if (oldest_record && !oldest_record->m_bTickbaseShiftedBackwards && !oldest_record->m_bIsDuplicateTickbase && !oldest_record->m_bTeleporting)
				{
					if (oldest_record->m_bHeadIsVisible || oldest_record->m_bBodyIsVisible)
						_idealBacktrackTick = oldest_record;
				}

				/* this is unlikely to ever get triggered at all but could be worth implementing.. */
				if (newest_record && !newest_record->m_bTickbaseShiftedBackwards && !newest_record->m_bIsDuplicateTickbase && !newest_record->m_bTeleporting)
				{
					if (newest_record->m_bHeadIsVisible || newest_record->m_bBodyIsVisible)
						_idealBacktrackTick = newest_record;
				}
			}
		}
#endif

		// we are forced to hitscan again because we moved our position from all backtrackable records that we hitscanned previously
		m_BacktrackTick = _idealBacktrackTick;

		// restore player to selected tick
		m_Playerrecord->CM_BacktrackPlayer(_idealBacktrackTick);

		// we moved from all backtrackable records, hitscan the best one with minimum amount of points again to prevent too much fps drop
		if (RunHitscan(true))
		{
			g_Ragebot.m_bTracedEnemy = true;

			if (CanHitRecord())
			{
				Profiling.Finish();
				END_PROFILING
				return true;
			}

			// no point in forward tracking if we were able to trace the player already
			Profiling.Finish();
			END_PROFILING
			return false;
		}

		if (g_Ragebot.ForwardTrack(m_Playerrecord, false, &_ForwardTracked))
		{
			g_Info.UsingForwardTrack = true;
			g_Ragebot.m_bTracedEnemy = true;
			Profiling.Finish();
			END_PROFILING
			return true;
		}

		Profiling.Finish();
		END_PROFILING
		return false;
	}

	//can't backtrack, just hitchance the current record and be done with it

	//first try forward tracking before the above
	if (!_CanHitCurrentRecord)
	{
		if (g_Ragebot.ForwardTrack(m_Playerrecord, false/*!_CanBacktrack*/, &_ForwardTracked))
		{
			g_Info.UsingForwardTrack = true;
			g_Ragebot.m_bTracedEnemy = true;
			Profiling.Finish();
			END_PROFILING
			return true;
		}
	}
	else
	{
		//see if we can hit the current record

		m_BacktrackTick = _CurrentRecord;

		// restore player to selected tick
		m_Playerrecord->CM_BacktrackPlayer(_CurrentRecord);

		if (CanHitRecord())
		{
			Profiling.Finish();
			END_PROFILING
			return true;
		}
	}

	Profiling.Finish();
	END_PROFILING
	return false;
}