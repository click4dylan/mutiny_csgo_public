//If you need this function, ask dylan for the latest reversed code. This isn't used client-side anymore since server-side
//hitboxes are networked now to the cheat

#include "precompiled.h"

#ifdef HOOK_LAG_COMPENSATION
#include "VTHook.h"
#include "LocalPlayer.h"
#include "INetchannelInfo.h"
#include "FrameUpdatePostEntityThink.h"

StartLagCompensationFn oStartLagCompensation = NULL;

bool BacktrackPlayer(CBaseEntity *pEnt, float flTargetTime, std::deque<LagCompensationRecord*>*records)
{
	Vector org, mins, maxs;
	QAngle ang;

	Vector prevOrg = pEnt->GetAbsOriginServer();

	LagCompensationRecord* prevRecord = NULL;
	LagCompensationRecord* record = NULL;

	// check if we have at least one entry
	if (records->size() <= 0)
	{
		printf("BacktrackPlayer: client(%d) has no records!\n", pEnt->entindexServer());
		return false;
	}

	unsigned curr = 0;
	bool bFoundRecord = false;

	// Walk context looking for any invalidating event
	while (curr < records->size())
	{
		// remember last record
		prevRecord = record;

		// get next record
		record = records->at(curr);

		if (!record->bAlive)
		{
			printf("BacktrackPlayer: client(%d) not alive, not lag compensating!\n", pEnt->entindexServer());
			// entity must be alive, lost track
			return false;
		}

		Vector delta = record->absorigin - prevOrg;
		if (delta.LengthSqr() > (64.0f * 64.0f))
		{
			Vector absorigin = pEnt->GetAbsOriginServer();
			printf("BacktrackPlayer: client(%d) teleported, not lag compensating!\nCurrent Origin: %f %f %f\n", pEnt->entindexServer(), absorigin.x, absorigin.y, absorigin.z);
			// lost track, too much difference
			return false;
		}

		// did we find a context smaller than target time ?
		if (record->flSimulationTime <= flTargetTime)
		{
			bFoundRecord = true;
			break; // hurra, stop
		}

		prevOrg = record->absorigin;

		// go one step back
		curr++;
	}

	if (!bFoundRecord)
	{
		printf("No valid positions in history for BacktrackPlayer client(%d)\n", pEnt->entindexServer());
		return false;
	}

	float frac = 0.0f;
	if (prevRecord &&
		(record->flSimulationTime < flTargetTime) &&
		(record->flSimulationTime < prevRecord->flSimulationTime))
	{
		// we didn't find the exact time but have a valid previous record
		// so interpolate between these two records;

		//Assert(prevRecord->flSimulationTime > record->flSimulationTime);
		//Assert(flTargetTime < prevRecord->flSimulationTime);

		// calc fraction between both records
		frac = (flTargetTime - record->flSimulationTime) /
			(prevRecord->flSimulationTime - record->flSimulationTime);

		//Assert(frac > 0 && frac < 1); // should never extrapolate

		ang = Lerp(frac, record->absangles, prevRecord->absangles);
		org = Lerp(frac, record->absorigin, prevRecord->absorigin);
		//mins = Lerp(frac, record->m_vecMins, prevRecord->m_vecMins);
		//maxs = Lerp(frac, record->m_vecMaxs, prevRecord->m_vecMaxs);

		printf("Found approximate lag compensation record for client(%d)\nTargetTime: %f RecordTime: %f PrevRecordTime: %f\nDelta 1: %f Delta 2: %f\n", pEnt->entindexServer(), flTargetTime, record->flSimulationTime, prevRecord->flSimulationTime, flTargetTime - record->flSimulationTime, prevRecord->flSimulationTime - record->flSimulationTime);
	}
	else
	{
		// we found the exact record or no other record to interpolate with
		// just copy these values since they are the best we have
		ang = record->absangles;
		org = record->absorigin;
		//mins = record->m_vecMins;
		//maxs = record->m_vecMaxs;

		printf("Found exact lag compensation record for client(%d)\nTargetTime: %f RecordTime: %f PrevRecordTime: %f\n", pEnt->entindexServer(), flTargetTime, record->flSimulationTime, prevRecord ? prevRecord->flSimulationTime : 0.0f);

#if 1
		QAngle eyeanglesfromrecord;
		eyeanglesfromrecord.x = record->body_pitch;
		eyeanglesfromrecord.y = ClampYr(ClampYr(record->absangles.y) + record->body_yaw);
		eyeanglesfromrecord.z = 0.0f;
		printf("Record origin: %f %f %f EyeAngles: %f %f %f body_yaw: %f\n\n", org.x, org.y, org.z, eyeanglesfromrecord.x, eyeanglesfromrecord.y, eyeanglesfromrecord.z, record->body_yaw);
#endif
	}
	return true;
}

void __fastcall Hooks::StartLagCompensation(void* thisptr, int edx, void *player, int lagCompensationType, const Vector& weaponPos, const QAngle &weaponAngles, float weaponRange)
{
	CBaseEntity* const ent = (CBaseEntity*)player;
	CUserCmd *cmd = (CUserCmd*)*(DWORD*)((DWORD)player + 0x0F60); //GetCurrentUserCmd is in server.dll "CLagCompensationManager::StartLagCompensation with NULL CUserCmd!!!"

	if (!cmd)
	{
		// This can hit if m_hActiveWeapon incorrectly gets set to a weapon not actually owned by the local player 
		// (since lag comp asks for the GetPlayerOwner() which will have m_pCurrentCommand == NULL, 
		//  not the player currently executing a CUserCmd).
		printf("CLagCompensationManager::StartLagCompensation with NULL CUserCmd!!!\n");
		oStartLagCompensation(thisptr, player, lagCompensationType, weaponPos, weaponAngles, weaponRange);
		return;
	}

	AllocateConsole();

	// correct is the amount of time we have to correct game time
	float correct = 0.0f;
	float latency = 0.0f;

	int entindex = ent->entindexServer();

	INetChannelInfo *nci = GetPlayerNetInfoServer(entindex);
	if (nci)
		latency = nci->GetLatency(FLOW_OUTGOING);

	// add network latency
	correct += latency;

	// NOTE:  do these computations in float time, not ticks, to avoid big roundoff error accumulations in the math
	// add view interpolation latency see C_BaseEntity::GetInterpolationAmount()
	float m_fLerpTime = *(float*)((DWORD)ent + 0x0BF4);
	correct += m_fLerpTime;

	correct = clamp(correct, 0, 1.0f); //clamp to sv_maxunlag

	// correct tick send by player 
	float flTargetTime = TICKS_TO_TIME(cmd->tick_count) - m_fLerpTime;

	static IGlobalVarsBase** gppGlobals = nullptr;
	if (!gppGlobals)
	{
		const char* gpGlobalsServerSig = "8B  15  ??  ??  ??  ??  66  0F  6E  50  08  F3  0F  10  62  10";
		gppGlobals = (IGlobalVarsBase**)FindMemoryPattern(GetModuleHandleA("server.dll"), (char*)gpGlobalsServerSig, strlen(gpGlobalsServerSig));
		if (!gppGlobals)
			DebugBreak();
		gppGlobals = (IGlobalVarsBase**)*(DWORD*)((DWORD)gppGlobals + 2);
	}

	IGlobalVarsBase* gpGlobals = (IGlobalVarsBase*)*gppGlobals;

	// calculate difference between tick sent by player and our latency based tick
	float deltaTime = correct - (gpGlobals->curtime - flTargetTime);

	if (fabs(deltaTime) > 0.2f)
	{
		// difference between cmd time and latency is too big > 200ms, use time correction based on latency
		printf("StartLagCompensation: delta too big (%.3f)\n", deltaTime );
		flTargetTime = gpGlobals->curtime - correct;
	}

	for (int i = 0; i < GetServerClientCount(); i++)
	{
		DWORD cl = GetServerClientEntity(i);
		if (cl && ((CBaseEntity*)cl)->IsActive() && ((CBaseEntity*)cl)->IsConnected())// && !((CBaseEntity*)cl)->IsFakeClient())
		{
			CBaseEntity *pEnt = ServerClientToEntity(cl);
			if (pEnt && pEnt != ent)
			{
				// Move entity back in time and remember that fact
				BacktrackPlayer(pEnt, flTargetTime, &gLagCompensationRecords[pEnt->entindexServer()]);
			}
		}
	}

	oStartLagCompensation(thisptr, player, lagCompensationType, weaponPos, weaponAngles, weaponRange);
}
#endif