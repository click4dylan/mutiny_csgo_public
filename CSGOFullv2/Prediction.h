#pragma once
#include "CreateMove.h"

class CPrediction
{
public:
	void SetupMove(CBaseEntity* ent, CUserCmd* cmd, void* move, void* movedata)
	{
		typedef void(__thiscall* fn)(void*, CBaseEntity*, CUserCmd*, void*, void*);
		VMT::getvfunc<fn>(this, 20)(this, ent, cmd, move, movedata);
	}

	void FinishMove(CBaseEntity* ent, CUserCmd* cmd, void* movedata)
	{
		typedef void(__thiscall* fn)(void*, CBaseEntity*, CUserCmd*, void*);
		VMT::getvfunc<fn>(this, 21)(this, ent, cmd, movedata);
	}

	void FinishCommand(C_BasePlayer *player)
	{
		typedef void(__thiscall* fn)(C_BasePlayer*);
		VMT::getvfunc<fn>(this, 25)(this, player);
	}
};

class CMovement
{
public:
	void ProcessMovement(CBaseEntity* ent, void* movedata)
	{
		typedef void(__thiscall* fn)(void*, CBaseEntity*, void*);
		VMT::getvfunc<fn>(this, 1)(this, ent, movedata);
	}

	void StartTrackPredictionErrors(CBaseEntity* ent)
	{
		typedef void(__thiscall* fn)(void*, CBaseEntity*);
		VMT::getvfunc<fn>(this, 3)(this, ent);
	}

	void FinishTrackPredictionErrors(CBaseEntity* ent)
	{
		typedef void(__thiscall* fn)(void*, CBaseEntity*);
		VMT::getvfunc<fn>(this, 4)(this, ent);
	}
};
