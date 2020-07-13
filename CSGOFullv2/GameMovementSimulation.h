#if 0

#ifndef __GAMEMOVE_H__
#define __GAMEMOVE_H__

#ifdef _MSC_VER
#pragma once
#endif

#include "stdafx.h"
#include "misc.h"
#include "Trace.h"

struct GameSimulationInfo
{
public:
	float speed, newspeed, accelspeed, addspeed, currentspeed, control, friction, drop, gravity, spd, wishspeed, wishspd, fmove, smove;
	float	backoff;
	float	change;
	float angle;
	Vector forward, right, up, dest, wishvel, wishdir, vec3_origin;
	trace_t pm, trace;
	int			bumpcount, numbumps;
	Vector		dir;
	float		d;
	int			numplanes;
	Vector		planes[128];
	Vector		primal_velocity, original_velocity;
	Vector      new_velocity;
	Vector		end;
	float		time_left, allFraction;
	int			blocked;
};

class CGameMovementSimulation
{
private:
	GameSimulationInfo m_EntityStruct[128];

public:
	CGameMovementSimulation(void);
	void SetAbsOrigin(CBaseEntity* pBaseEntity, const Vector &vec);
	void TracePlayerBBox(const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm, CBaseEntity* pBaseEntity);
	void FullWalkMove(CBaseEntity* pBaseEntity);
	void Friction(CBaseEntity* pBaseEntity);
	void CheckVelocity(CBaseEntity* pBaseEntity);
	void Accelerate(CBaseEntity* pBaseEntity, Vector& wishdir, float wishspeed, float accel);
	void WalkMove(CBaseEntity* pBaseEntity);
	void StayOnGround(CBaseEntity* pBaseEntity);
	void FinishGravity(CBaseEntity* pBaseEntity);
	void StartGravity(CBaseEntity* pBaseEntity);
	void CheckFalling(CBaseEntity* pBaseEntity);
	void AirAccelerate(CBaseEntity *pBaseEntity, Vector& wishdir, float wishspeed, float accel);
	int TryPlayerMove(CBaseEntity *pBaseEntity, Vector *pFirstDest, trace_t *pFirstTrace);
	void AirMove(CBaseEntity *pBaseEntity);
	int ClipVelocity(CBaseEntity *pBaseEntity, Vector& in, Vector& normal, Vector& out, float overbounce);
};

extern CGameMovementSimulation* GameMovementSimulation;

#endif

#endif