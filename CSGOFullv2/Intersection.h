#pragma once
#include "Trace.h"
#include "misc.h"
#include <vector>

struct Ray_t;
class CGameTrace;
typedef CGameTrace trace_t;

class COBB
{
public:
	Vector vecbbMin;
	Vector vecbbMax;
	QAngle angles;
	matrix3x4_t* boneMatrix;
	int hitbox;
	int hitgroup;
	int physicsbone;

	COBB(void){};
	COBB(const Vector& bbMin, const Vector& bbMax, const QAngle& angAngles, matrix3x4_t* matrix, int ihitbox, int ihitgroup, int iphysicsbone)
	{
		vecbbMin   = bbMin;
		vecbbMax   = bbMax;
		angles	 = angAngles;
		boneMatrix = matrix;
		hitbox = ihitbox;
		hitgroup = ihitgroup;
		physicsbone = iphysicsbone;
	};
};

class CSphere
{
public:
	Vector m_vecCenter;
	float m_flRadius = 0.f;
	//float   m_flRadius2 = 0.f; // r^2

	CSphere(void){};
	CSphere(const Vector& vecCenter, float flRadius, int hitbox, int hitgroup, int physicsbone)
	{
		m_vecCenter = vecCenter;
		m_flRadius  = flRadius;
		Hitbox		= hitbox;
		Hitgroup	= hitgroup;
		PhysicsBone = physicsbone;
	};

	int Hitbox;
	int Hitgroup;
	int PhysicsBone;

	bool intersectsRay(const Ray_t& ray);
	bool intersectsRay(const Ray_t& ray, Vector* vecIntersection, Vector* vecExitPoint, float scale = 1.f);
};

//-----------------------------------------------------------------------------
// Intersects a ray against a box
//-----------------------------------------------------------------------------
struct BoxTraceInfo_t
{
	float t1;
	float t2;
	int hitside;
	bool startsolid;
};

void SetupCapsule(const Vector& vecMin, const Vector& vecMax, float flRadius, int hitbox, int hitgroup, int physicsbone, std::vector< CSphere >& m_cSpheres);

//-----------------------------------------------------------------------------
// IntersectRayWithOBB
//
// Purpose: Computes the intersection of a ray with a oriented box (OBB)
// Output : Returns true if there is an intersection + trace information
//-----------------------------------------------------------------------------
bool IntersectRayWithOBB(const Vector& vecRayStart, const Vector& vecRayDelta, const matrix3x4_t& matOBBToWorld, const Vector& vecOBBMins, const Vector& vecOBBMaxs, float flTolerance, CBaseTrace* pTrace, Vector* vecExitPos = nullptr);

bool IntersectRayWithOBB(const Vector& vecRayOrigin, const Vector& vecRayDelta, const Vector& vecBoxOrigin, const QAngle& angBoxRotation, const Vector& vecOBBMins, const Vector& vecOBBMaxs, float flTolerance, CBaseTrace* pTrace);

bool IntersectRayWithOBB(const Ray_t& ray, const Vector& vecBoxOrigin, const QAngle& angBoxRotation, const Vector& vecOBBMins, const Vector& vecOBBMaxs, float flTolerance, CBaseTrace* pTrace);

bool IntersectRayWithOBB(const Ray_t& ray, const matrix3x4_t& matOBBToWorld, const Vector& vecOBBMins, const Vector& vecOBBMaxs, float flTolerance, CBaseTrace* pTrace, Vector* vecExitPos = nullptr);

bool IntersectRayWithOBB(const Vector& vecRayStart, const Vector& vecRayDelta, const matrix3x4_t& matOBBToWorld, const Vector& vecOBBMins, const Vector& vecOBBMaxs, float flTolerance, BoxTraceInfo_t* pTrace);

//credits to http://www.scratchapixel.com/ for the nice explanation of the algorithm and
//An Efficient and Robust Ray–Box Intersection Algorithm, Amy Williams et al. 2004.
//for inventing it :D
bool IntersectRayWithAABB(Vector& origin, Vector& dir, Vector& min, Vector& max, Vector* vecIntersection, Vector* vecExitPoint);

//-----------------------------------------------------------------------------
// IntersectRayWithBox
//
// Purpose: Computes the intersection of a ray with a box (AABB)
// Output : Returns true if there is an intersection + trace information
//-----------------------------------------------------------------------------
bool IntersectRayWithBox(const Vector& rayStart, const Vector& rayDelta, const Vector& boxMins, const Vector& boxMaxs, float epsilon, CBaseTrace* pTrace, float* pFractionLeftSolid = NULL);
bool IntersectRayWithBox(const Ray_t& ray, const Vector& boxMins, const Vector& boxMaxs, float epsilon, CBaseTrace* pTrace, float* pFractionLeftSolid = NULL);
bool IntersectRayWithBox(const Vector& vecRayStart, const Vector& vecRayDelta, const Vector& boxMins, const Vector& boxMaxs, float flTolerance, BoxTraceInfo_t* pTrace);

bool IntersectRayWithBoxSimple(const Vector& vecRayStart, const Vector& vecRayDelta, const Vector& boxMins, const Vector& boxMaxs);
// Intersect ray R(t) = p + t*d against AABB a. When intersecting,
// return intersection distance tmin and point q of intersection
int IntersectRayAABB(Vector &p, Vector &d, Vector &mins, Vector &maxs, float & tmin, Vector & q);

void TRACE_HITBOX(CBaseEntity* Entity, Ray_t& ray, trace_t& tr, std::vector< CSphere >& m_cSpheres, std::vector< COBB >& m_cOBBs, Vector* vecExitPoint = nullptr);
void TRACE_HITBOX_SCALE(CBaseEntity* Entity, Ray_t& ray, trace_t& tr, std::vector< CSphere >& m_cSpheres, std::vector< COBB >& m_cOBBs, Vector* vecExitPoint = nullptr, float scale = 1.f);

bool IsRayIntersectingSphere(const Vector &vecRayOrigin, const Vector &vecRayDelta,
	const Vector& vecCenter, float flRadius, float flTolerance);