#if 0

#include "precompiled.h"
#include "Aimbot_imi.h"
#include "AutoBone_imi.h"
#include "Eventlog.h"
#include "LocalPlayer.h"
#include "WeaponController.h"
#include "Assistance.h"
#include "Events.h"
#include "UsedConvars.h"
#include "INetchannelInfo.h"
#include "Removals.h"
#include "datamap.h"

#include "Adriel/stdafx.hpp"
#include "Adriel/renderer.hpp"
#include "Adriel/adr_util.hpp"
#include "Adriel/console.hpp"

#include "Adriel/input.hpp"

class CLegitbot
{
protected:
	QAngle m_angAimAngles;

	Target_s m_LastTarget;
	int m_iPossibleTargetIndex = 0;
	int m_iIgnorePlayerIndex;
	float m_flLastSortTime = 0.0f;

	bool m_bHasTarget = false;

	std::vector<Target_s> m_PossibleTargets;

	bool IsValidTarget(int i);
	bool IsValidTarget(CBaseEntity* Player);
	bool IsValidTarget(CPlayerrecord* Player);

	bool TargetEntity(Target_s* Target);
	void TargetEntities();

	bool Finalize();
public:
	void SetLowestFovEntity();

	Target_s m_LowestFOVTarget;
	bool m_bTracedEnemy = false;

	void ClearIgnorePlayerIndex()
	{
		m_iIgnorePlayerIndex = -1;
	}

	void ClearAimbotTarget();
	void ClearPossibleTargets()
	{
		m_PossibleTargets.clear();
		m_LowestFOVTarget.Reset();
		m_iPossibleTargetIndex = 0;
	}
	void GetTargettableEntityListUnsorted(std::vector<Target_s>&list, Target_s& lowestfovtarget);
	QAngle& GetAimAngles() { return m_angAimAngles; }
	bool FoundEnemy() const { return m_bTracedEnemy; }
	bool HasTarget() const { return m_bHasTarget; }
	int GetTarget() const { return m_LastTarget.m_iEntIndex; }

	void Run();

	Target_s m_LastTarget;
};

void CLegitbot::ClearAimbotTarget()
{
	LocalPlayer.UseDoubleTapHitchance = false;
	m_LastTarget.Reset();
	m_bHasTarget = false;
}

bool CLegitbot::IsValidTarget(int i)
{
	return i != INVALID_PLAYER && IsValidTarget(Interfaces::ClientEntList->GetBaseEntity(i));
}
bool CLegitbot::IsValidTarget(CBaseEntity* Player)
{
	if (!LocalPlayer.Entity)
		return false;

	return Player && Player->IsPlayer() && !Player->GetImmune() && Player->IsEnemy(LocalPlayer.Entity);
}
bool CLegitbot::IsValidTarget(CPlayerrecord* Player)
{
	return IsValidTarget(Player->m_pEntity);
}

void CLegitbot::SetLowestFovEntity()
{
	START_PROFILING
		m_LowestFOVTarget.Reset();

	for (int i = 1; i <= MAX_PLAYERS; i++)
	{
		// get player record
		CPlayerrecord* _playerRecord = &m_PlayerRecords[i];

		// invalid playerrecord
		if (!_playerRecord)
			continue;

		auto _Entity = _playerRecord->m_pEntity;

		bool _IsValidFarESPTarget = _Entity && _playerRecord->IsValidFarESPAimbotTarget();

		// invalid/dead entity
		if (!_Entity || (!_Entity->GetAlive() && !_IsValidFarESPTarget))
			continue;

		// not a valid target
		if (!_IsValidFarESPTarget && !_playerRecord->IsValidTarget())
			continue;

		Target_s _Target = Target_s();
		_Target.m_iEntIndex = _Entity->index;
		_Target.m_flFOV = GetFov(LocalPlayer.ShootPosition, _Entity->GetEyePosition(), LocalPlayer.CurrentEyeAngles);

		if (_Target.m_flFOV < m_LowestFOVTarget.m_flFOV || m_LowestFOVTarget.m_iEntIndex == INVALID_PLAYER)
		{
			Vector EyePos = _Entity->GetLocalOriginDirect() - LocalPlayer.Entity->GetLocalOriginDirect();
			VectorAngles(EyePos, _Target.m_AtTargetAngle);

			m_LowestFOVTarget = _Target;
		}
	}

	END_PROFILING
}

void CLegitbot::GetTargettableEntityListUnsorted(std::vector<Target_s>&list, Target_s& lowestfovtarget)
{
	START_PROFILING
		list.clear();
	lowestfovtarget.Reset();

	// loop through all streamed entities
	for (int i = 1; i <= MAX_PLAYERS; i++)
	{
		// get player record
		CPlayerrecord* _playerRecord = &m_PlayerRecords[i];

		// get entity
		auto _Entity = _playerRecord->m_pEntity;

		bool _IsValidFarESPTarget = _Entity && _playerRecord->IsValidFarESPAimbotTarget();

		// invalid/dead entity
		if (!_Entity || (!_Entity->GetAlive() && !_IsValidFarESPTarget))
			continue;

		// not a valid target
		if (!_IsValidFarESPTarget && !_playerRecord->IsValidTarget())
			continue;

		Target_s _Target;
		_Target.m_Weight = 0;
		_Target.m_iEntIndex = _Entity->index;
		_Target.m_flFOV = GetFov(LocalPlayer.ShootPosition, _Entity->GetEyePosition(), LocalPlayer.CurrentEyeAngles);

		Vector EyePos = _Entity->GetLocalOriginDirect() - LocalPlayer.Current_Origin;
		VectorAngles(EyePos, _Target.m_AtTargetAngle);

		// todo: nit; somehow incorporate v4's sorting into this. not in beta though, got no time atm.
		// until then, we'll just use the best record if available or best fov
		auto _record = _playerRecord->GetBestRecord(false);
		if (_record)
			_Target.m_Weight = _record->GetWeight();

		if (_Target.m_flFOV < lowestfovtarget.m_flFOV || lowestfovtarget.m_iEntIndex == INVALID_PLAYER)
		{
			lowestfovtarget = _Target;
		}

		// add target
		list.emplace_back(_Target);
	}
	END_PROFILING
}

void CLegitbot::TargetEntities()
{
	Target_s _lowestFOV;
	bool _gotFreshList = false;
	if (m_flLastSortTime != Interfaces::Globals->curtime)
	{
		//First, get a fresh list of players
		std::vector<Target_s> _freshList;
		GetTargettableEntityListUnsorted(_freshList, _lowestFOV);
		m_flLastSortTime = Interfaces::Globals->curtime;

		//If there are none available, just clear our lists and exit
		if (_freshList.empty())
		{
			ClearAimbotTarget();
			ClearPossibleTargets();
			ClearIgnorePlayerIndex();
			return;
		}

		for (auto i = 0; i < m_PossibleTargets.size(); ++i)
		{
			Target_s &old = m_PossibleTargets[i];
			if (std::find(_freshList.begin(), _freshList.end(), old) == _freshList.end())
			{
				//no longer a valid target
				if (m_LowestFOVTarget.m_iEntIndex == old.m_iEntIndex)
					m_LowestFOVTarget = _lowestFOV;
				//erase this target from the list of scannable targets
				m_PossibleTargets.erase(m_PossibleTargets.begin() + i);
				if (m_iPossibleTargetIndex > i)
					m_iPossibleTargetIndex = max(0, m_iPossibleTargetIndex - 1); //decrease the index since we removed an entity
			}
		}

		if (m_PossibleTargets.empty())
		{
			m_PossibleTargets = _freshList;
			m_LowestFOVTarget = _lowestFOV;
			m_iPossibleTargetIndex = 0;

			if (m_PossibleTargets.empty())
			{
				ClearAimbotTarget();
				ClearPossibleTargets();
				ClearIgnorePlayerIndex();
				return; //still empty
			}
		}

		_gotFreshList = true;
	}
	else
	{
		if (m_PossibleTargets.empty())
		{
			ClearAimbotTarget();
			ClearPossibleTargets();
			ClearIgnorePlayerIndex();
			return;
		}
	}

	int MaxTargetsPerTick = variable::get().ragebot.i_targets_per_tick;
	if (m_iIgnorePlayerIndex != -1) //we are excluding a person that we already scanned this tick
		++MaxTargetsPerTick;

	int _maxTargets = min(m_iPossibleTargetIndex + MaxTargetsPerTick, (int)m_PossibleTargets.size());
	//unsigned char _numChecked = 0;
	//unsigned char _CheckedEntIndexes[MAX_PLAYERS + 1]{ 0 };
	//bool _foundTarget = false;

	for (; m_iPossibleTargetIndex < _maxTargets; ++m_iPossibleTargetIndex)
	{
		auto target = &m_PossibleTargets[m_iPossibleTargetIndex];
		if (target->m_iEntIndex != m_iIgnorePlayerIndex)
		{
			//++_numChecked;
			//_CheckedEntIndexes[target->m_iEntIndex] = 1;

			if (TargetEntity(target))
			{
				//	_foundTarget = true;
				break;
			}
		}
		else
		{
			//if (target->m_iEntIndex <= MAX_PLAYERS)
			//{
			//	++_numChecked;
			//	_CheckedEntIndexes[target->m_iEntIndex] = 1;
			//}
		}
	}

	if (m_iPossibleTargetIndex >= (int)m_PossibleTargets.size())
	{
		// scanned all targets. note: there is a flaw here
		// if we have for example 2 targets per tick and only 1 was valid anymore this tick, we will have only scanned 1 person instead of 2
		// todo: come up with a better way than what we used to to deal with this case that doesn't break the whole ragebot
		ClearPossibleTargets();
		ClearIgnorePlayerIndex();
		if (_gotFreshList)
			m_LowestFOVTarget = _lowestFOV;
	}
}

bool CLegitbot::Run_AutoBone(CBaseEntity* Entity)
{

}

bool CLegitbot::TargetEntity(Target_s* Target)
{
	CBaseEntity* _Entity = Interfaces::ClientEntList->GetBaseEntity(Target->m_iEntIndex);

	if (!_Entity)
	{
		m_LastTarget.Reset();
		return false;
	}

	// net player record for entity
	CPlayerrecord* _playerRecord = g_LagCompensation.GetPlayerrecord(_Entity);

	// no playerrecord or spectating or no target tick
	if (!_playerRecord || !_playerRecord->m_pEntity || (_playerRecord->m_bSpectating && !_playerRecord->IsValidFarESPAimbotTarget()) || !_playerRecord->m_TargetRecord)
	{
		m_LastTarget.Reset();
		return false;
	}

	if (g_AutoBone.Run_AutoBone(_Entity))
	{
		m_LastTarget = *Target;
		m_LastTarget.m_pBestHitbox = g_AutoBone.m_pBestHitbox;
		//m_iLastChecked = 0;
		return true;
	}

	m_LastTarget.Reset();
	return false;
}

bool CLegitbot::Finalize()
{
	m_iIgnorePlayerIndex = -1;

	if (m_LastTarget.m_iEntIndex == INVALID_PLAYER || g_AutoBone.m_pBestHitbox == nullptr)
		return false;

	CPlayerrecord* _playerRecord = &m_PlayerRecords[m_LastTarget.m_iEntIndex];
	WeaponInfo_t* wpn_data = LocalPlayer.CurrentWeapon->GetCSWpnData();

	if (!wpn_data || !_playerRecord || !_playerRecord->m_pEntity)
	{
		ClearAimbotTarget();
		return false;
	}

	Vector& vecHitboxPos = g_AutoBone.m_pBestHitbox->origin;

#ifndef IMI_MENU
	g_Visuals.m_lastBestAimbotPos = vecHitboxPos;
	g_Visuals.m_lastBestDamage = g_AutoBone.m_pBestHitbox->damage;
	g_Visuals.m_prioritize_body = (g_AutoBone.m_pBestHitbox->actual_hitbox_due_to_penetration != HITBOX_HEAD && g_AutoBone.m_pBestHitbox->actual_hitbox_due_to_penetration != HITBOX_LOWER_NECK);
#endif

#if defined _DEBUG || defined INTERNAL_DEBUG 
	Interfaces::DebugOverlay->AddTextOverlay(Vector(vecHitboxPos.x, vecHitboxPos.y, vecHitboxPos.z - 1.0f), 0.05f, "Aimbot Target");
	Interfaces::DebugOverlay->AddBoxOverlay(vecHitboxPos, Vector(-2, -2, -2), Vector(1, 1, 1), LocalPlayer.CurrentEyeAngles, 255, 0, 0, 255, 0.05f);
#endif

	if (g_WeaponController.WeaponDidFire(CurrentUserCmd.cmd->buttons | IN_ATTACK) != 1 || !variable::get().ragebot.b_autofire)
		return false;

	m_angAimAngles = g_AutoBone.m_pBestHitbox->localplayer_eyeangles;
	return true;
}

void CLegitbot::Run()
{
	m_angAimAngles = LocalPlayer.FinalEyeAngles;
	m_bTracedEnemy = false;
	g_Info.UsingForwardTrack = false;
	m_iIgnorePlayerIndex = -1;

	if (IsValidTarget(m_LastTarget.m_iEntIndex))
	{
		m_iIgnorePlayerIndex = m_LastTarget.m_iEntIndex;
		CPlayerrecord* _playerRecord = g_LagCompensation.GetPlayerrecord(m_LastTarget.m_iEntIndex);

		if (_playerRecord && _playerRecord->m_pEntity && _playerRecord->m_pEntity->GetAlive() && _playerRecord->m_pEntity->IsEnemy(LocalPlayer.Entity)
			&& !((_playerRecord->m_bSpectating || _playerRecord->m_bDormant) && !_playerRecord->IsValidFarESPAimbotTarget()) && _playerRecord->m_TargetRecord)
		{
			if (TargetEntity(&m_LastTarget))
			{
				SetLowestFovEntity();
				m_bHasTarget = Finalize();
				return;
			}
		}
	}

	ClearAimbotTarget();

	if (m_iPossibleTargetIndex == 0)
	{
		GetTargettableEntityListUnsorted(m_PossibleTargets, m_LowestFOVTarget);
		m_flLastSortTime = Interfaces::Globals->realtime;
	}
	else
		SetLowestFovEntity();

	TargetEntities();

	m_bHasTarget = Finalize();
}

CLegitbot g_Legitbot;
#endif