#pragma once
#ifdef HOOK_LAG_COMPENSATION
#include "misc.h"
#include <deque>

struct LagCompensationRecord
{
	float flSimulationTime;
	BOOLEAN bAlive;
	QAngle absangles;
	Vector absorigin;
	float body_yaw;
	float body_pitch;
	float goalfeetyaw;
	float curfeetyaw;
	int tickschoked;
	CUserCmd currentusercmd;
	CUserCmd lastusercmd;
};

extern std::deque<LagCompensationRecord*> gLagCompensationRecords[65];
#endif