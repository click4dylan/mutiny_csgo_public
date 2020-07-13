#pragma once
#include <vector>
#include <deque>
#include <mutex>
#include "LocalPlayer.h"

class Vector;
class QAngle;
class ITraceFilter;

enum WaypointFlags
{
	WAYPOINT_FLAG_JUMP = (1 << 0),
	WAYPOINT_FLAG_CROUCH = (1 << 1),
	WAYPOINT_FLAG_CAMP = (1 << 2),
	WAYPOINT_FLAG_TEMP = (1 << 3),
	WAYPOINT_FLAG_ONEWAY = (1 << 4),
	WAYPOINT_FLAG_BOMBSITE = (1 << 5),
	WAYPOINT_FLAG_SAFEZONE = (1 << 6),
	WAYPOINT_FLAG_CTSPAWN = (1 << 7),
	WAYPOINT_FLAG_TSPAWN = (1 << 8),
	WAYPOINT_FLAG_LADDER_TOP = (1 << 9),
	WAYPOINT_FLAG_LADDER_STEP = (1 << 10),
	WAYPOINT_FLAG_LADDER_BOTTOM = (1 << 11),
	WAYPOINT_FLAG_ALWAYS_DRAW = (1 << 12),
	WAYPOINT_FLAG_ONEWAY_VICTIM = (1 << 13),
	WAYPOINT_FLAG_FAKEDUCK = (1 << 14),
	WAYPOINT_FLAG_USE = (1 << 15),
	WAYPOINT_FLAG_PATH = (1 << 16),
	WAYPOINT_FLAG_BREAKABLE = (1 << 17),
	WAYPOINT_FLAG_BROKEN = (1 << 18), //temporary flag that indicates whether or not we shot it
};

struct Waypoint
{
	Vector   m_vecOrigin;
	QAngle   m_vecAngles;
	Vector   m_vecNormal;
	Vector   m_vecMins;
	Vector   m_vecMaxs;
	unsigned m_iFlags;

	unsigned IsLadder() const { return m_iFlags & (WAYPOINT_FLAG_LADDER_TOP | WAYPOINT_FLAG_LADDER_STEP | WAYPOINT_FLAG_LADDER_BOTTOM); }
	unsigned IsLadderTop() const { return m_iFlags & (WAYPOINT_FLAG_LADDER_TOP); }
	unsigned IsLadderStep() const { return m_iFlags & (WAYPOINT_FLAG_LADDER_STEP); }
	unsigned IsLadderBottom() const { return m_iFlags & (WAYPOINT_FLAG_LADDER_BOTTOM); }
	unsigned IsPath() const { return m_iFlags & (WAYPOINT_FLAG_PATH); }
	unsigned IsBreakable() const { return m_iFlags & (WAYPOINT_FLAG_BREAKABLE); }
	unsigned IsBroken() const { return m_iFlags & (WAYPOINT_FLAG_BROKEN); }
	void	 Break() { m_iFlags |= WAYPOINT_FLAG_BROKEN; }
	unsigned IsCampSpot() const
	{
		return (m_iFlags & (WAYPOINT_FLAG_CAMP | WAYPOINT_FLAG_ONEWAY | WAYPOINT_FLAG_FAKEDUCK));
	}
	void ResetTemporaryFlags() 
	{ 
		m_iFlags &= ~WAYPOINT_FLAG_BROKEN; 
	}
	bool ShouldWalkHere() const 
	{
		if (m_iFlags & (WAYPOINT_FLAG_TEMP | WAYPOINT_FLAG_ONEWAY_VICTIM))
			return false;
		if (m_iFlags & WAYPOINT_FLAG_BREAKABLE)
		{
			if (m_iFlags & WAYPOINT_FLAG_BROKEN)
				return false;
			if (!LocalPlayer.CurrentWeapon || !LocalPlayer.WeaponVars.IsGun)
				return false;
		}
		return true;
	}
};

extern std::vector<Waypoint*> m_Waypoints;
extern std::recursive_mutex m_WaypointsMutex;
extern Waypoint* m_CurrentWaypoint;
extern std::deque<Waypoint*> m_LastWaypoints;

bool TraceWaypoint(Waypoint* waypoint, const Vector& startpos, ITraceFilter* filter, const float diameter, const Vector& mins, const Vector& maxs, const int referenceindex = 1);
Waypoint* ScanForNewWaypoint();
Waypoint * ScanForNearbyBreakable();
void Waypoint_OnCreateMove();
void ClearAllWaypoints();
void ReadWaypoints();
void SaveWaypoints();
Waypoint* CreateWaypoint();
void DeleteWaypoint(Waypoint* wp);
void DeleteWaypoint(std::vector<Waypoint*>::iterator& wp);
Waypoint* GetWaypointFromScreenPosition(ImVec2 CursorPos, Vector* WorldPosition = nullptr, Vector* Direction = nullptr);
void SortWaypointsByFOVAndDistance(std::vector<Waypoint*>&List);
void SortWaypointsByFOVAndDistance(std::deque<Waypoint*>&List);
void SortWaypointsByDistance(std::vector<Waypoint*>&List);
void SortWaypointsByDistance(std::deque<Waypoint*>&List);
void SortWaypointsByFOV(std::deque<Waypoint*>&List);
void SortWaypointsByFOV(std::vector<Waypoint*>&List);
void ResetAllWaypointTemporaryFlags();
void ResetGlobalWalkBotVariables();
void ResetGlobalWaypointSystemVariables();
void StoreCurrentWaypoint();