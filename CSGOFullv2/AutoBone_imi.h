#pragma once
#include "Includes.h"
#include "CPlayerrecord.h"
#include "AutoWall.h"
#include "Targetting.h"

struct s_Hitbox
{
	bool m_bTarget;
	int m_iPriority;
	bool m_bMultipoint;

	s_Hitbox(bool _shouldTarget, bool _multipoint, int _priority = 0)
	{
		m_iPriority = _priority;
		m_bTarget = _shouldTarget;
		m_bMultipoint = _multipoint;
	}
	s_Hitbox()
	{
		m_iPriority = 0;
		m_bTarget = m_bMultipoint = false;
	}
};

class CAutoBone
{
protected:
	struct s_Profiling
	{
		double m_dbStartTime = 0.f;
		double m_dbAvgTimeToRun = 0.f;
		double m_dbTotalTimeToRun = 0.f;

		int m_iTimesRunThisFrame = 0;
		int TickRanFrameStatistics = 0;

		void Run();
		void Finish();
	} Profiling;

	std::array<s_Hitbox, HITBOX_MAX> m_Hitboxes;

	float m_flTargetTime = 0.f;

	void SetupHitboxes(bool DisableHeadMultipoint = false, bool MinimumPointsOnly = false);

	bool Init(CBaseEntity* _Entity);

	CBaseEntity* m_pEntity = nullptr;
	CPlayerrecord* m_Playerrecord = nullptr;
	CTickrecord* m_BacktrackTick = nullptr;

	bool m_RunningHistoryScan = false;
	bool m_bHittingTargetFPS = false;
	bool m_bCanHitscan = false;
	bool m_bCanMultipoint = false;
	bool m_bDidPlayerRespawn = false;
	bool m_bCanScanHead = false;
	bool m_bCanScanBody = false;

	QAngle m_NoRecoilAngles = QAngle(0.f, 0.f, 0.f);

	bool CreateAndTracePoints();
	bool RunHitscan(bool MinimumPointsOnly = false);
	bool CanHitRecord();
	bool ShouldPriorityHead(int* reason = nullptr);
	bool ShouldPrioritizeBody(int* reason = nullptr);
public:
	std::atomic<bool> m_bStopHitscan = false;
	std::atomic<bool> m_bStopMultipoint = false;
	std::atomic<bool> m_bFinishedScanningHead = false;
	int m_iNumHeadPoints;
	std::atomic<int> m_iNumHeadPointsScanned;
	SavedHitboxPos* m_pBestHitbox = nullptr;
	int m_iBestHitbox = -1;
	int m_iWeaponDamage = 0; //base damage of our weapon
	float GetMinDamageScaled(SavedHitboxPos* savedinfo);
	void GetAngleToHitboxInfo(const Vector& HitboxPosition, QAngle& EyeAnglesToHitbox, QAngle& EyeAnglesToSend, QAngle& ServerFireBulletAngles);

	CAutoBone() {}
	CAutoBone& operator=(const CAutoBone& other) 
	{
		Profiling = other.Profiling;
		m_Hitboxes = other.m_Hitboxes;
		m_flTargetTime = other.m_flTargetTime;
		m_pEntity = other.m_pEntity;
		m_Playerrecord = other.m_Playerrecord;
		m_BacktrackTick = other.m_BacktrackTick;
		m_RunningHistoryScan = other.m_RunningHistoryScan;
		m_bHittingTargetFPS = other.m_bHittingTargetFPS;
		m_bDidPlayerRespawn = other.m_bDidPlayerRespawn;
		m_bCanHitscan = other.m_bCanHitscan;
		m_bCanMultipoint = other.m_bCanMultipoint;
		m_bCanScanHead = other.m_bCanScanHead;
		m_bCanScanBody = other.m_bCanScanBody;
		m_NoRecoilAngles = other.m_NoRecoilAngles;
		m_bStopHitscan = other.m_bStopHitscan.load();
		m_bStopMultipoint = other.m_bStopMultipoint.load();
		m_bFinishedScanningHead = other.m_bFinishedScanningHead.load();
		m_iNumHeadPoints = other.m_iNumHeadPoints;
		m_iNumHeadPointsScanned = other.m_iNumHeadPointsScanned.load();
		m_pBestHitbox = other.m_pBestHitbox;
		m_iBestHitbox = other.m_iBestHitbox;
		m_iWeaponDamage = other.m_iWeaponDamage;
		return *this;
	}
	
	bool HitboxStatsAreBetter(SavedHitboxPos *existing, SavedHitboxPos* newhitbox);
	void SaveBestHitboxPos(int hitboxid, int hitboxhit, int hitgrouphit, float fldamage, const Vector& origin, bool ismultipoint, bool penetrated);
	bool Run_AutoBone(CBaseEntity* _Entity);
};

#ifdef _DEBUG
extern CAutoBone m_AutoBoneState;
#endif

extern CAutoBone g_AutoBone;

struct multipoint_parameters_t
{
	Vector DrawHitboxMinsBox;
	Vector DrawHitboxMaxsBox;
	QAngle UnmodifiedEyeAngles;
	CBaseEntity* Entity;
	QAngle *NoRecoilAngles;
	int Hitbox;
	Vector BonePos;
	bool CanHit;
	bool IsMultipoint;
};