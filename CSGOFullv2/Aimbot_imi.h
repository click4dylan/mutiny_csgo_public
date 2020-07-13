#pragma once
#include "Includes.h"
#include "CPlayerrecord.h"
#include <stack>

const int BASIC_HITBOXES[9] = { HITBOX_HEAD, HITBOX_CHEST, HITBOX_PELVIS, HITBOX_LEFT_HAND, HITBOX_RIGHT_HAND, HITBOX_LEFT_CALF, HITBOX_RIGHT_CALF, HITBOX_LEFT_FOOT, HITBOX_RIGHT_FOOT };

struct Target_s
{
	int m_iEntIndex;
	SavedHitboxPos* m_pBestHitbox;
	Vector m_Aimpos;
	QAngle m_AtTargetAngle;
	float m_flFOV;
	int m_Weight;

	void Reset()
	{
		m_iEntIndex = INVALID_PLAYER;

		m_pBestHitbox = nullptr;
		m_flFOV = FLT_MAX;
		m_Weight = INT_MIN;
		m_Aimpos.Init();
		m_AtTargetAngle.Init();
	}

	Target_s()
	{
		Reset();
	}

	int operator!=(const Target_s& other) const
	{
		return m_iEntIndex != other.m_iEntIndex;
	}

	int operator==(const Target_s& other) const
	{
		return m_iEntIndex == other.m_iEntIndex;
	}
};

class CRagebot
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

	void SortTargettableEntityList(std::vector<Target_s>&list);
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
	void OutputMissedShots();
	CBaseEntity* ForwardTrack(CPlayerrecord* pCPlayer, bool ForceForwardTrack, bool* AlreadyForwardTracked);
};

#ifdef _DEBUG
//push states
extern CRagebot m_RagebotState;
#endif

extern CRagebot g_Ragebot;

#if 0
class CLegitbot_imi
{
protected:
	ConVar* weapon_recoil_scale = nullptr;

	QAngle m_angAimAngles;
	QAngle m_StartAimAngles;
	QAngle m_LastAimAngles;

	Target_s m_LastTarget;
	Target_s m_CurrentTarget;

	bool m_bHasTarget = false;

	bool m_bAimbotNeedsRCS = false;

	QAngle m_LastPunchAngles;
	QAngle m_RCSDelta;

	std::vector<Target_s> m_PossibleTargets;

	float m_flAimTime = 0.f;
	float m_flCurAimTime = 0.f;

	void ClearAimbotTarget()
	{
		m_bHasTarget = false;
	}

	void Update();

	bool IsInHitboxRadius(CBaseEntity* entity, int hitbox, float scale);

	void SortTargettableEntityList();
	bool TargetEntity(Target_s* Target);
	void TargetEntities();

	bool FinishedAim(CBaseEntity* entity);

	void ApplyRCS();

	bool Finalize();
public:
	QAngle& GetAimAngles() { return m_angAimAngles; }
	QAngle& GetRCSAngles() { return m_RCSDelta; }
	bool HasTarget() const { return m_bHasTarget; }
	int GetTarget() const { return m_LastTarget.m_iEntIndex; }
	void Run();
};
#endif

//extern CLegitbot_imi g_Legitbot;