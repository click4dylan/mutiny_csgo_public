#include "precompiled.h"
#include "Interfaces.h"

void(__fastcall *pUTIL_ClipTraceToPlayers) (const Vector&, const Vector&, unsigned int, ITraceFilter *, trace_t *);
void(__fastcall *pUTIL_ClipTraceToPlayers2) (const Vector&, const Vector&, unsigned int, ITraceFilter *, trace_t *, float);
CClientState *g_ClientState;

BOOLEAN(__thiscall *IsPaused) (DWORD ClientState);
int host_tickcount;
DWORD* m_pPredictionRandomSeed;


IBaseClientDll*			Interfaces::Client = nullptr;
IClientModeShared*		Interfaces::ClientMode = nullptr;
CClientEntityList*		Interfaces::ClientEntList;
ICVar*					Interfaces::Cvar;
IEngineClient*			Interfaces::EngineClient = nullptr;
IEngineTrace*			Interfaces::EngineTrace = nullptr;
IEngineSound*			Interfaces::EngineSound;
IGlobalVarsBase*		Interfaces::Globals = nullptr;
IVPanel*				Interfaces::VPanel;
//IVModelRender*		Interfaces::ModelRender;
//IVModelInfo*			Interfaces::ModelInfo;
//IMaterialSystem*		Interfaces::MaterialSystem;
//IMaterial*			Interfaces::Material;
IVRenderView*			Interfaces::RenderView = nullptr;
CPrediction*			Interfaces::Prediction;
IVModelRender*			Interfaces::ModelRender;
CGameMovement*          Interfaces::GameMovement;
IPhysicsSurfaceProps*	Interfaces::Physprops;
IVModelInfoClient*      Interfaces::ModelInfoClient;
IVDebugOverlay*			Interfaces::DebugOverlay;
CMaterialSystem*		Interfaces::MatSystem;
IGameEventManager2*		Interfaces::GameEventManager;
C_TEFireBullets*		Interfaces::TE_FireBullets;
ISurface*				Interfaces::Surface;
CInput*					Interfaces::Input;
CStudioRenderContext*	Interfaces::StudioRender;
IInputSystem*			Interfaces::InputSystem;
C_CSGameRules			**Interfaces::GameRules;
IGameTypes				**Interfaces::GameTypes;
IFileSystem*			Interfaces::FileSystem = nullptr;
CHud*					Interfaces::Hud = nullptr;
IClientLeafSystem*		Interfaces::ClientLeafSystem = nullptr;
IPhysicsCollision*		Interfaces::PhysicsCollision;

C_TEMuzzleFlash* Interfaces::TE_MuzzleFlash;
C_TEEffectDispatch* Interfaces::TE_EffectDispatch;
IMDLCache* Interfaces::MDLCache;
CMoveHelperClient **Interfaces::MoveHelperClient;
IViewRenderBeams* Interfaces::Beams;

#include "Trace.h"
#include "CPlayerrecord.h"
#include "LocalPlayer.h"

void UTIL_ClipTraceToPlayers_Fixed(const Ray_t& ray, trace_t *ptr, CBaseEntity* pSkipEntity, bool OnlyTeammates, bool NoTeammates)
{
	trace_t best_trace;
	float closest_distance = FLT_MAX;

	//FIXME: potential optimization: get an array of people closest to furthest so we can exit the loop
	for (CPlayerrecord *iter = &m_PlayerRecords[1]; iter != &m_PlayerRecords[MAX_PLAYERS]; ++iter)
	{
		CPlayerrecord *record = iter;
		if (record->m_pEntity != pSkipEntity && record->m_pEntity && ((record->m_bConnected && !record->m_bDormant) || record->IsValidFarESPTarget()) && record->m_pEntity->GetAlive())
		{
			bool _scanPlayer = true;
			if (OnlyTeammates)
				_scanPlayer = !record->m_pEntity->IsEnemy(LocalPlayer.Entity);
			else if (NoTeammates)
				_scanPlayer = record->m_pEntity->IsEnemy(LocalPlayer.Entity);

			if (_scanPlayer)
			{
				CTickrecord *_targetTick = record->m_TargetRecord;
				if (!_targetTick)
				{
#ifdef INTERNAL_DEBUG
					if (IsDebuggerPresent())
						DebugBreak();
#endif
					continue;
				}
				CBaseEntity *_ent = record->m_pEntity;
				bool _IsUsingNetworkedESP = record->m_bIsUsingFarESP || record->m_bIsUsingServerSide;
				if ((!record->m_bDormant || _IsUsingNetworkedESP))
				{
					trace_t tr;
					// can't trust bones on far esp targets
					//if (!_IsUsingNetworkedESP)
					//{
						//Do a trace to the surrounding bounding box to see if we hit it, for optimization
						if (_targetTick && !IntersectRayWithBox(ray.m_Start, ray.m_Delta, _targetTick->m_HitboxWorldMins, _targetTick->m_HitboxWorldMaxs, 0.0f, &tr))
							continue;

						Interfaces::EngineTrace->ClipRayToEntity(ray, MASK_SHOT, (IHandleEntity*)_ent, &tr);
					//}
					//else
					//{
					//	TRACE_HITBOX(_ent, (Ray_t&)ray, tr, _targetTick->m_cSpheres, _targetTick->m_cOBBs);
					//}
					if (tr.m_pEnt == _ent)
					{
						float dist = (tr.endpos - ray.m_Start).Length();
						if (!best_trace.m_pEnt || dist < closest_distance)
						{
							closest_distance = dist;
							best_trace = tr;
						}
					}
				}
			}
		}
	}

	if (best_trace.m_pEnt)
		*ptr = best_trace;
}

void UTIL_ClipTraceToPlayers_Fixed(Vector& start, Vector& end, trace_t *ptr, CBaseEntity* pSkipEntity, bool OnlyTeammates, bool NoTeammates)
{
	Ray_t ray;
	ray.Init(start, end);
	UTIL_ClipTraceToPlayers_Fixed(ray, ptr, pSkipEntity, OnlyTeammates, NoTeammates);
}

void UTIL_TraceHull(Vector& src, Vector& end, Vector& mins, Vector& maxs, unsigned int mask, CBaseEntity *ignore, int collisionGroup, trace_t& tr)
{
	Ray_t ray;
	ray.Init(src, end, mins, maxs);
	CTraceFilter filter;
	filter.pSkip = (IHandleEntity*)ignore;
	filter.m_icollisionGroup = collisionGroup;
	Interfaces::EngineTrace->TraceRay(ray, mask, &filter, &tr);
}

void inline UTIL_TraceRay(const Ray_t &ray, unsigned int mask, CBaseEntity *ignore, int collisionGroup, trace_t *ptr)
{
	CTraceFilter traceFilter;
	traceFilter.pSkip = (IHandleEntity*)ignore;
	traceFilter.m_icollisionGroup = collisionGroup;

	Interfaces::EngineTrace->TraceRay(ray, mask, &traceFilter, ptr);
}

float UTIL_FindWaterSurface(const Vector &position, float minz, float maxz)
{
	Vector vecStart, vecEnd;
	vecStart.Init(position.x, position.y, maxz);
	vecEnd.Init(position.x, position.y, minz);

	Ray_t ray;
	trace_t tr;
	CWaterTraceFilter waterTraceFilter;
	ray.Init(vecStart, vecEnd);
	Interfaces::EngineTrace->TraceRay(ray, MASK_WATER, &waterTraceFilter, &tr);

	return tr.endpos.z;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Given a position and a ray, return the shortest distance between the two.
 * If 'pos' is beyond either end of the ray, the returned distance is negated.
 */
inline float DistanceToRay(const Vector &pos, const Vector &rayStart, const Vector &rayEnd, float *along = NULL, Vector *pointOnRay = NULL)
{
	Vector to = pos - rayStart;
	Vector dir = rayEnd - rayStart;
	float length = dir.NormalizeInPlace();

	float rangeAlong = DotProduct(dir, to);
	if (along)
	{
		*along = rangeAlong;
	}

	float range;

	if (rangeAlong < 0.0f)
	{
		// off start point
		range = -(pos - rayStart).Length();

		if (pointOnRay)
		{
			*pointOnRay = rayStart;
		}
	}
	else if (rangeAlong > length)
	{
		// off end point
		range = -(pos - rayEnd).Length();

		if (pointOnRay)
		{
			*pointOnRay = rayEnd;
		}
	}
	else // within ray bounds
	{
		Vector onRay = rayStart + dir * rangeAlong;
		range = (pos - onRay).Length();

		if (pointOnRay)
		{
			*pointOnRay = onRay;
		}
	}

	return range;
}

void UTIL_ClipTraceToPlayers(const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, ITraceFilter* filter, trace_t* tr, float minimumRange, float smallestFraction)
{
#if 1
	trace_t playerTrace;
	Ray_t ray;
	if (smallestFraction < 0.0f)
		smallestFraction = tr->fraction;
	const float maxRange = 60.0f;

	ray.Init(vecAbsStart, vecAbsEnd);

	for (int k = 1; k <= MAX_PLAYERS; ++k)
	{
		CBaseEntity *player = Interfaces::ClientEntList->GetBaseEntity(k);

		if (!player || !player->GetAliveVMT())
			continue;

		if (player->GetDormant())
			continue;

		if (filter && filter->ShouldHitEntity((IHandleEntity*)player, mask) == false)
			continue;

		float range = DistanceToRay(player->WorldSpaceCenter(), vecAbsStart, vecAbsEnd);


		if (range < minimumRange || range > maxRange)
			continue;

		Interfaces::EngineTrace->ClipRayToEntity(ray, mask | CONTENTS_HITBOX, (IHandleEntity*)player, &playerTrace);
		if (playerTrace.fraction < smallestFraction)
		{
			// we shortened the ray - save off the trace
			*tr = playerTrace;
			smallestFraction = playerTrace.fraction;
		}
	}
#else
	int useless;
	int useless2 = (DWORD)&useless;
	DWORD useless3 = (DWORD)&vecAbsStart.x;

	__asm {
		mov esi, tr
		mov edi, useless3

		push flFraction
		push useless2
		push flMinimumRange
		push tr
		push filter
		push mask
		mov	 edx, vecAbsEnd
		mov  ecx, vecAbsStart
		call pUTIL_ClipTraceToPlayers
		add	 esp, 0x18
	}
#endif
}