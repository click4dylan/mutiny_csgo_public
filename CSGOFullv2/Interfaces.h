#pragma once

#include "IBaseClientDLL.h"
#include "CClientEntityList.h"
#include "IClientModeShared.h"
//#include "ICvar.h"
#include "IEngineClient.h"
#include "IEngineTrace.h"
#include "IGlobalVarsBase.h"
#include "IInputSystem.h"
//#include "Interfaces.h"
//#include "ISurface.h"
#include "IVPanel.h"
//#include "IVModelInfo.h"
//#include "IVModelRender.h"
//#include "IMaterialSystem.h"
//#include "IMaterial.h"
//#include "IRenderView.h"
#include "IPrediction.h"
#include "VPhysics.h"
#include "IGameMovement.h"
#include "IModelInfoClient.h"
//#include "IVDebugOverlay.h"
#include "StudioRender.h"
#include "ICvar.h"
#include "TE_FireBullets.h"
#include "ivdebugoverlay.h"
#include "IVRenderView.h"
#include "CMaterialSystem.h"
#include "IVModelRender.h"
#include "misc.h"
#include "IInput.h"
#include "IViewRenderBeams.h"
#include "C_CSGameRules.h"
#include "C_CSGameTypes.h"
#include "IFileSystem.h"
#include "Hud.h"
//#include "IEngineSound.h"
#include "IClientLeafSystem.h"
#include "IPhysicsCollision.h"

class IGameEventManager2;
class IMDLCache;
class CMaterialSystem;
class IVModelRender;
class CGameMovement;
class CMoveHelperClient;
class CGameEvent;
class IViewRenderBeams;
class IEngineSound;
class C_TEMuzzleFlash;
class IVModelInfoClient;
class CSplitScreen;
class IPhysicsCollision;

namespace Interfaces // Interfaces
{
	extern IBaseClientDll*			Client;
	extern IClientModeShared*		ClientMode;
	extern CClientEntityList*		ClientEntList;
	extern ICVar*					Cvar;
	extern IInputSystem*			InputSystem;
	extern IEngineClient*			EngineClient;
	extern IEngineTrace*			EngineTrace;
	extern IEngineSound*			EngineSound;
	extern IGlobalVarsBase*			Globals;
	//extern ISurface*				Surface;
	extern IVPanel*					VPanel;
	//extern IVModelRender*			ModelRender;
	//extern IVModelInfo*				ModelInfo;
	//extern IMaterialSystem*			MaterialSystem;
	//extern IMaterial*				Material;
	extern IVRenderView*			RenderView;
	extern CPrediction*				Prediction;
	extern IPhysicsSurfaceProps*	Physprops;
	extern IVModelRender*			ModelRender;
	extern CGameMovement* GameMovement;
	extern IVModelInfoClient* ModelInfoClient;
	extern IVDebugOverlay*			DebugOverlay;
	extern CMaterialSystem*		MatSystem;
	extern CInput* Input;
	extern CStudioRenderContext*			StudioRender;
	extern IGameEventManager2*		GameEventManager;
	extern C_TEFireBullets* TE_FireBullets;
	extern ISurface*				Surface;
	extern C_TEEffectDispatch* TE_EffectDispatch;
	extern IMDLCache* MDLCache;
	extern CMoveHelperClient **MoveHelperClient;
	extern IViewRenderBeams* Beams;
	extern C_TEMuzzleFlash* TE_MuzzleFlash;
	extern C_CSGameRules **GameRules;
	extern IGameTypes **GameTypes;
	extern IFileSystem *FileSystem;
	extern IClientLeafSystem* ClientLeafSystem;
	extern IPhysicsCollision* PhysicsCollision;
	extern CHud *Hud;
}

template <typename T>
T* CaptureInterface(HANDLE modulehandle, const char* strInterface)
{
	typedef T* (*CreateInterfaceFn)(const char* szName, int iReturn);
	char *createinterfacestr = new char[16]{ 57, 8, 31, 27, 14, 31, 51, 20, 14, 31, 8, 28, 27, 25, 31, 0 }; /*CreateInterface*/
	DecStr(createinterfacestr, 15);
	CreateInterfaceFn CreateInterface = (CreateInterfaceFn)GetProcAddress((HMODULE)modulehandle, createinterfacestr);
	EncStr(createinterfacestr, 15);

	return CreateInterface(strInterface, 0);
}

inline void UTIL_TraceLine(const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, CBaseEntity* ignore, trace_t* ptr)
{
	Ray_t ray;
	ray.Init(vecAbsStart, vecAbsEnd);
	CTraceFilter filter;
	filter.pSkip = (IHandleEntity*)ignore;

	Interfaces::EngineTrace->TraceRay(ray, mask, &filter, ptr);
}

inline void UTIL_TraceLine(const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, CBaseEntity* ignore, int collisionGroup, trace_t* ptr)
{
	Ray_t ray;
	ray.Init(vecAbsStart, vecAbsEnd);

	CTraceFilter filter;
	filter.pSkip = (IHandleEntity*)ignore;
	filter.m_icollisionGroup = collisionGroup;

	Interfaces::EngineTrace->TraceRay(ray, mask, &filter, ptr);
}

inline void UTIL_TraceLine(const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, CTraceFilter* filter, trace_t* ptr)
{
	Ray_t ray;
	ray.Init(vecAbsStart, vecAbsEnd);

	Interfaces::EngineTrace->TraceRay(ray, mask, filter, ptr);
}

inline void UTIL_TraceHull(Vector& src, Vector& end, Vector& mins, Vector& maxs, unsigned int mask, ITraceFilter* filter, trace_t& tr)
{
	Ray_t ray;
	ray.Init(src, end, mins, maxs);
	Interfaces::EngineTrace->TraceRay(ray, mask, filter, &tr);
}

inline void UTIL_TraceHull(Vector& src, Vector& end, Vector& mins, Vector& maxs, ITraceFilter* filter, trace_t& tr)
{
	Ray_t ray;
	ray.Init(src, end, mins, maxs);
	Interfaces::EngineTrace->TraceRay(ray, 0x200400B, filter, &tr);
}

extern void UTIL_TraceHull(Vector& src, Vector& end, Vector& mins, Vector& maxs, unsigned int mask, CBaseEntity *ignore, int collisionGroup, trace_t& tr);
extern void UTIL_ClipTraceToPlayers_Fixed(const Ray_t& ray, trace_t *ptr, CBaseEntity* pSkipEntity, bool OnlyTeammates = false, bool NoTeammates = false);
extern void UTIL_ClipTraceToPlayers_Fixed(Vector &start, Vector &end, trace_t *ptr, CBaseEntity* pSkipEntity, bool OnlyTeammates = false, bool NoTeammates = false);

#if 0 //fixme eventually
inline void UTIL_TraceEntity(CBaseEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd,
	unsigned int mask, const IHandleEntity *ignore, int collisionGroup, trace_t *ptr)
{
	ICollideable *pCollision;
	pCollision = pEntity->GetCollideable();

	CTraceFilterEntityIgnoreOther traceFilter(pEntity, pIgnore, nCollisionGroup);
	Interfaces::EngineTrace->SweepCollideable(pCollision, vecAbsStart, vecAbsEnd, pCollision->GetCollisionAngles(), mask, &traceFilter, ptr);
}
#endif

extern void(__fastcall *pUTIL_ClipTraceToPlayers) (const Vector&, const Vector&, unsigned int, ITraceFilter *, trace_t *);
extern void(__fastcall *pUTIL_ClipTraceToPlayers2) (const Vector&, const Vector&, unsigned int, ITraceFilter *, trace_t *, float);
extern void UTIL_ClipTraceToPlayer(CPlayerrecord* playerrecord, Ray_t &ray, trace_t* tr);

void UTIL_ClipTraceToPlayers(const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, ITraceFilter* filter, trace_t* tr, float minimumRange = 0.0f, float smallestFraction = -1.0f);

inline void UTIL_ClipRayToEntity(Vector end, Vector start, trace_t* oldtrace, CBaseEntity* ent)
{
	auto mins = ent->GetMins(), maxs = ent->GetMaxs();

	auto dir(end - start);

	dir.NormalizeInPlace();

	auto center = (maxs + mins) / 2;

	auto pos(center + ent->GetNetworkOrigin());

	auto to = (pos - start);

	auto range_along = dir.Dot(to);

	float range;

	if (range_along < 0.f)
		range = -to.Length();
	else if (range_along > dir.Length())
		range = -(pos - end).Length();
	else
	{
		auto ray(pos - ((dir * range_along) + start));

		range = ray.Length();
	}

	if (range <= 60.f)
	{
		trace_t tr;
		Ray_t ray;
		ray.Init(start, end);

		Interfaces::EngineTrace->ClipRayToEntity(ray, MASK_SHOT, (IHandleEntity*)ent, &tr);

		if (oldtrace->fraction > tr.fraction)
			*oldtrace = tr;
	}
}

inline void UTIL_TraceLineIgnoreTwoEntities(const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, CBaseEntity *ignore, CBaseEntity *ignore2, int collisionGroup, trace_t *ptr)
{
	Ray_t ray;
	ray.Init(vecAbsStart, vecAbsEnd);
	CTraceFilterSkipTwoEntities traceFilter((IHandleEntity*)ignore, (IHandleEntity*)ignore2, collisionGroup);
	Interfaces::EngineTrace->TraceRay(ray, mask, &traceFilter, ptr);
}

inline void UTIL_TraceLineIgnoreThreeEntities(const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, CBaseEntity *ignore, CBaseEntity *ignore2, CBaseEntity* ignore3, int collisionGroup, trace_t *ptr)
{
	Ray_t ray;
	ray.Init(vecAbsStart, vecAbsEnd);
	CTraceFilterSkipThreeEntities traceFilter((IHandleEntity*)ignore, (IHandleEntity*)ignore2, (IHandleEntity*)ignore3, collisionGroup);
	Interfaces::EngineTrace->TraceRay(ray, mask, &traceFilter, ptr);
}

extern void UTIL_TraceRay(const Ray_t &ray, unsigned int mask, CBaseEntity *ignore, int collisionGroup, trace_t *ptr);

inline void UTIL_TraceRay(const Ray_t &ray, unsigned int mask, IHandleEntity *ignore, int collisionGroup, trace_t *ptr)
{
	UTIL_TraceRay(ray, mask, (CBaseEntity*)ignore, collisionGroup, ptr);
}

inline void UTIL_TraceRay(const Ray_t &ray, unsigned int mask, ITraceFilter* filter, trace_t *ptr)
{
	Interfaces::EngineTrace->TraceRay(ray, mask, filter, ptr);
}

extern float UTIL_FindWaterSurface(const Vector &position, float minz, float maxz);

#if 0
inline bool HandleBulletPenetration(WeaponInfo_t *wpn_data, FireBulletData& data)
{
	__asm {
		push wpn_data
	}
}
#endif

extern CClientState* g_ClientState;
extern CSplitScreen* g_SplitScreenMgr;

extern BOOLEAN(__thiscall *IsPaused) (DWORD ClientState);
extern int host_tickcount;
extern DWORD* m_pPredictionRandomSeed;