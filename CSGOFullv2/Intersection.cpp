#include "precompiled.h"
#include "misc.h"
#include "Intersection.h"
#include "Math.h"
#include "Interfaces.h"
#include "Includes.h"

//-----------------------------------------------------------------------------
//
// IntersectInfiniteRayWithSphere
//
// Returns whether or not there was an intersection. 
// Returns the two intersection points
//
//-----------------------------------------------------------------------------
bool IntersectInfiniteRayWithSphere(const Vector &vecRayOrigin, const Vector &vecRayDelta,
	const Vector &vecSphereCenter, float flRadius, float *pT1, float *pT2)
{
	// Solve using the ray equation + the sphere equation
	// P = o + dt
	// (x - xc)^2 + (y - yc)^2 + (z - zc)^2 = r^2
	// (ox + dx * t - xc)^2 + (oy + dy * t - yc)^2 + (oz + dz * t - zc)^2 = r^2
	// (ox - xc)^2 + 2 * (ox-xc) * dx * t + dx^2 * t^2 +
	//		(oy - yc)^2 + 2 * (oy-yc) * dy * t + dy^2 * t^2 +
	//		(oz - zc)^2 + 2 * (oz-zc) * dz * t + dz^2 * t^2 = r^2
	// (dx^2 + dy^2 + dz^2) * t^2 + 2 * ((ox-xc)dx + (oy-yc)dy + (oz-zc)dz) t +
	//		(ox-xc)^2 + (oy-yc)^2 + (oz-zc)^2 - r^2 = 0
	// or, t = (-b +/- sqrt( b^2 - 4ac)) / 2a
	// a = DotProduct( vecRayDelta, vecRayDelta );
	// b = 2 * DotProduct( vecRayOrigin - vecCenter, vecRayDelta )
	// c = DotProduct(vecRayOrigin - vecCenter, vecRayOrigin - vecCenter) - flRadius * flRadius;

	Vector vecSphereToRay;
	VectorSubtract(vecRayOrigin, vecSphereCenter, vecSphereToRay);

	float a = DotProduct(vecRayDelta, vecRayDelta);

	// This would occur in the case of a zero-length ray
	if (a == 0.0f)
	{
		*pT1 = *pT2 = 0.0f;
		return vecSphereToRay.LengthSqr() <= flRadius * flRadius;
	}

	float b = 2 * DotProduct(vecSphereToRay, vecRayDelta);
	float c = DotProduct(vecSphereToRay, vecSphereToRay) - flRadius * flRadius;
	float flDiscrim = b * b - 4 * a * c;
	if (flDiscrim < 0.0f)
		return false;

	flDiscrim = sqrt(flDiscrim);
	float oo2a = 0.5f / a;
	*pT1 = (-b - flDiscrim) * oo2a;
	*pT2 = (-b + flDiscrim) * oo2a;
	return true;
}

//-----------------------------------------------------------------------------
//
// IntersectRayWithSphere
//
// Returns whether or not there was an intersection. 
// Returns the two intersection points, clamped to (0,1)
//
//-----------------------------------------------------------------------------
bool CSphere::intersectsRay(const Ray_t& ray, Vector* vecIntersection, Vector* vecExitPoint, float scale)
{
	//bool isintersecting = IsRayIntersectingSphere(ray.m_Start, ray.m_Delta, m_vecCenter, m_flRadius * scale, 0.0f);
		
	
	//return true;

	
	float T1, T2;
	if (!IntersectInfiniteRayWithSphere(ray.m_Start, ray.m_Delta, m_vecCenter, m_flRadius * scale, &T1, &T2))
		return false;

	if (T1 > 1.0f || T2 < 0.0f)
		return false;

	if (vecIntersection)
		*vecIntersection = ray.m_Start + ray.m_Delta * T1;

	if (vecExitPoint)
		*vecExitPoint = ray.m_Start + ray.m_Delta * T2;

	return true;
	
}

bool CSphere::intersectsRay(const Ray_t& ray)
{
	float T1, T2;
	return IntersectInfiniteRayWithSphere(ray.m_Start, ray.m_Delta, m_vecCenter, m_flRadius, &T1, &T2);
}

void SetupCapsule(const Vector& vecMin, const Vector& vecMax, float flRadius, int hitbox, int hitgroup, int physicsbone, std::vector<CSphere>&m_cSpheres)
{
	if (!vecMin.IsFinite() || !vecMax.IsFinite() || vecMax.x < vecMin.x || vecMax.y < vecMin.y || vecMax.z < vecMin.z)
	{
		m_cSpheres.clear();
		return;
	}

	auto vecDelta = (vecMax - vecMin);
	VectorNormalizeFast(vecDelta);
	auto vecCenter = vecMin;

	size_t numSpheres = std::floor(vecMin.Dist(vecMax));

	if (numSpheres > 256 || vecDelta.Length() > 50.0f)
	{
		m_cSpheres.clear();
		return;
	}

	m_cSpheres.push_back(CSphere{ vecMin, flRadius, hitbox, hitgroup, physicsbone });

	for (size_t i = 1; i < numSpheres; ++i) {
		m_cSpheres.push_back(CSphere{ vecMin + vecDelta * static_cast< float >(i), flRadius, hitbox, hitgroup, physicsbone });
	}

	m_cSpheres.push_back(CSphere{ vecMax, flRadius, hitbox, hitgroup, physicsbone });
}

bool IntersectRayWithAABB(Vector& origin, Vector& dir, Vector& min, Vector& max, Vector *vecIntersection, Vector *vecExitPoint)
{
	float tmin, tmax, tymin, tymax, tzmin, tzmax;

	if (dir.x >= 0)
	{
		tmin = (min.x - origin.x) / dir.x;
		tmax = (max.x - origin.x) / dir.x;
	}
	else
	{
		tmin = (max.x - origin.x) / dir.x;
		tmax = (min.x - origin.x) / dir.x;
	}

	if (dir.y >= 0)
	{
		tymin = (min.y - origin.y) / dir.y;
		tymax = (max.y - origin.y) / dir.y;
	}
	else
	{
		tymin = (max.y - origin.y) / dir.y;
		tymax = (min.y - origin.y) / dir.y;
	}

	if (tmin > tymax || tymin > tmax)
		return false;

	if (tymin > tmin)
		tmin = tymin;

	if (tymax < tmax)
		tmax = tymax;

	if (dir.z >= 0)
	{
		tzmin = (min.z - origin.z) / dir.z;
		tzmax = (max.z - origin.z) / dir.z;
	}
	else
	{
		tzmin = (max.z - origin.z) / dir.z;
		tzmax = (min.z - origin.z) / dir.z;
	}

	if (tmin > tzmax || tzmin > tmax)
		return false;

	//behind us
	if (tmin < 0 || tmax < 0)
		return false;

	if (vecIntersection)
		*vecIntersection = { tmin, tymin, tzmin };

	if (vecExitPoint)
		*vecExitPoint = { tmax, tymax, tzmax };

	return true;
}

void VectorITransform(const Vector *in1, const matrix3x4_t& in2, Vector *out)
{
	//Assert(s_bMathlibInitialized);
	float in1t[3];

	in1t[0] = in1->x - in2[0][3];
	in1t[1] = in1->y - in2[1][3];
	in1t[2] = in1->z - in2[2][3];

	out->x = in1t[0] * in2[0][0] + in1t[1] * in2[1][0] + in1t[2] * in2[2][0];
	out->y = in1t[0] * in2[0][1] + in1t[1] * in2[1][1] + in1t[2] * in2[2][1];
	out->z = in1t[0] * in2[0][2] + in1t[1] * in2[1][2] + in1t[2] * in2[2][2];
}

// assume in2 is a rotation and rotate the input vector
void VectorIRotate(const Vector *in1, const matrix3x4_t& in2, Vector *out)
{
	//Assert(s_bMathlibInitialized);
	//Assert(in1 != out);
	out->x = DotProduct((const float*)in1, in2[0]);
	out->y = DotProduct((const float*)in1, in2[1]);
	out->z = DotProduct((const float*)in1, in2[2]);
}

//-----------------------------------------------------------------------------
// Clears the trace
//-----------------------------------------------------------------------------
static void Collision_ClearTrace(const Vector &vecRayStart, const Vector &vecRayDelta, CBaseTrace *pTrace)
{
	pTrace->startpos = vecRayStart;
	pTrace->endpos = vecRayStart;
	pTrace->endpos += vecRayDelta;
	pTrace->startsolid = false;
	pTrace->allsolid = false;
	pTrace->fraction = 1.0f;
	pTrace->contents = 0;
}

//-----------------------------------------------------------------------------
// Intersects a ray against a box
//-----------------------------------------------------------------------------
bool IntersectRayWithBox(const Vector &vecRayStart, const Vector &vecRayDelta,
	const Vector &boxMins, const Vector &boxMaxs, float flTolerance, CBaseTrace *pTrace, float *pFractionLeftSolid)
{
	Collision_ClearTrace(vecRayStart, vecRayDelta, pTrace);

	BoxTraceInfo_t trace;

	if (IntersectRayWithBox(vecRayStart, vecRayDelta, boxMins, boxMaxs, flTolerance, &trace))
	{
		pTrace->startsolid = trace.startsolid;
		if (trace.t1 < trace.t2 && trace.t1 >= 0.0f)
		{
			pTrace->fraction = trace.t1;
			VectorMA(pTrace->startpos, trace.t1, vecRayDelta, pTrace->endpos);
			pTrace->contents = CONTENTS_SOLID;
			pTrace->plane.normal = vecZero;
			if (trace.hitside >= 3)
			{
				trace.hitside -= 3;
				pTrace->plane.dist = boxMaxs[trace.hitside];
				pTrace->plane.normal[trace.hitside] = 1.0f;
				pTrace->plane.type = trace.hitside;
			}
			else
			{
				pTrace->plane.dist = -boxMins[trace.hitside];
				pTrace->plane.normal[trace.hitside] = -1.0f;
				pTrace->plane.type = trace.hitside;
			}
			//dylan added
			if (pFractionLeftSolid)
			{
				*pFractionLeftSolid = trace.t2;
			}
			return true;
		}

		if (pTrace->startsolid)
		{
			pTrace->allsolid = (trace.t2 <= 0.0f) || (trace.t2 >= 1.0f);
			pTrace->fraction = 0;
			if (pFractionLeftSolid)
			{
				*pFractionLeftSolid = trace.t2;
			}
			pTrace->endpos = pTrace->startpos;
			pTrace->contents = CONTENTS_SOLID;
			pTrace->plane.dist = pTrace->startpos[0];
			pTrace->plane.normal.Init(1.0f, 0.0f, 0.0f);
			pTrace->plane.type = 0;
			pTrace->startpos = vecRayStart + (vecRayDelta * trace.t2);
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Intersects a ray against a box
//-----------------------------------------------------------------------------
bool IntersectRayWithBox(const Vector &vecRayStart, const Vector &vecRayDelta,
	const Vector &boxMins, const Vector &boxMaxs, float flTolerance, BoxTraceInfo_t *pTrace)
{
	int			i;
	float		d1, d2;
	float		f;

	pTrace->t1 = -1.0f;
	pTrace->t2 = 1.0f;
	pTrace->hitside = -1;

	// UNDONE: This makes this code a little messy
	pTrace->startsolid = true;

	for (i = 0; i < 6; ++i)
	{
		if (i >= 3)
		{
			d1 = vecRayStart[i - 3] - boxMaxs[i - 3];
			d2 = d1 + vecRayDelta[i - 3];
		}
		else
		{
			d1 = -vecRayStart[i] + boxMins[i];
			d2 = d1 - vecRayDelta[i];
		}

		// if completely in front of face, no intersection
		if (d1 > 0 && d2 > 0)
		{
			// UNDONE: Have to revert this in case it's still set
			// UNDONE: Refactor to have only 2 return points (true/false) from this function
			pTrace->startsolid = false;
			return false;
		}

		// completely inside, check next face
		if (d1 <= 0 && d2 <= 0)
			continue;

		if (d1 > 0)
		{
			pTrace->startsolid = false;
		}

		// crosses face
		if (d1 > d2)
		{
			f = d1 - flTolerance;
			if (f < 0)
			{
				f = 0;
			}
			f = f / (d1 - d2);
			if (f > pTrace->t1)
			{
				pTrace->t1 = f;
				pTrace->hitside = i;
			}
		}
		else
		{
			// leave
			f = (d1 + flTolerance) / (d1 - d2);
			if (f < pTrace->t2)
			{
				pTrace->t2 = f;
			}
		}
	}

	return pTrace->startsolid || (pTrace->t1 < pTrace->t2 && pTrace->t1 >= 0.0f);
}

//-----------------------------------------------------------------------------
// Intersects a ray against a box
//-----------------------------------------------------------------------------
bool IntersectRayWithBox(const Ray_t &ray, const Vector &boxMins, const Vector &boxMaxs,
	float flTolerance, CBaseTrace *pTrace, float *pFractionLeftSolid)
{
	if (!ray.m_IsRay)
	{
		Vector vecExpandedMins = boxMins - ray.m_Extents;
		Vector vecExpandedMaxs = boxMaxs + ray.m_Extents;
		bool bIntersects = IntersectRayWithBox(ray.m_Start, ray.m_Delta, vecExpandedMins, vecExpandedMaxs, flTolerance, pTrace, pFractionLeftSolid);
		pTrace->startpos += ray.m_StartOffset;
		pTrace->endpos += ray.m_StartOffset;
		return bIntersects;
	}
	return IntersectRayWithBox(ray.m_Start, ray.m_Delta, boxMins, boxMaxs, flTolerance, pTrace, pFractionLeftSolid);
}

bool IntersectRayWithBoxSimple(const Vector& vecRayStart, const Vector& vecRayDelta, const Vector& boxMins, const Vector& boxMaxs)
{
	// r.dir is unit direction vector of ray
	Vector dirfrac = { 1.0f / vecRayDelta.x, 1.0f / vecRayDelta.y, 1.0f / vecRayDelta.z };
	const Vector& lb = boxMins;
	const Vector& rt = boxMaxs;
	//dirfrac.x = 1.0f / r.dir.x;
	//dirfrac.y = 1.0f / r.dir.y;
	//dirfrac.z = 1.0f / r.dir.z;
	// lb is the corner of AABB with minimal coordinates - left bottom, rt is maximal corner
	// r.org is origin of ray
	float t1 = (lb.x - vecRayStart.x)*dirfrac.x;
	float t2 = (rt.x - vecRayStart.x)*dirfrac.x;
	float t3 = (lb.y - vecRayStart.y)*dirfrac.y;
	float t4 = (rt.y - vecRayStart.y)*dirfrac.y;
	float t5 = (lb.z - vecRayStart.z)*dirfrac.z;
	float t6 = (rt.z - vecRayStart.z)*dirfrac.z;
	float t;

	float tmin = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
	float tmax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));

	// if tmax < 0, ray (line) is intersecting AABB, but the whole AABB is behind us
	if (tmax < 0)
	{
		t = tmax;
		return false;
	}

	// if tmin > tmax, ray doesn't intersect AABB
	if (tmin > tmax)
	{
		t = tmax;
		return false;
	}

	t = tmin;
	return true;
}

// Intersect ray R(t) = p + t*d against AABB a. When intersecting,
// return intersection distance tmin and point q of intersection
int IntersectRayAABB(Vector &p, Vector &d, Vector &mins, Vector &maxs, float & tmin, Vector & q)
{
	tmin = 0.0f; // set to -FLT_MAX to get first hit on line
	float tmax = FLT_MAX; // set to max distance ray can travel (for segment)
	// For all three slabs
	for (int i = 0; i < 3; i++) {
		if (fabsf(d[i]) < FLT_EPSILON) {
			// Ray is parallel to slab. No hit if origin not within slab
			if (p[i] < mins[i] || p[i] > maxs[i]) return 0;
		}
		else {
			// Compute intersection t value of ray with near and far plane of slab
			float ood = 1.0f / d[i];
			float t1 = (mins[i] - p[i]) * ood;
			float t2 = (maxs[i] - p[i]) * ood;
			// Make t1 be intersection with near plane, t2 with far plane
			if (t1 > t2) std::swap(t1, t2);
			// Compute the intersection of slab intersection intervals
			if (t1 > tmin) tmin = t1;
			if (t2 > tmax) tmax = t2;
			// Exit with no collision as soon as slab intersection becomes empty
			if (tmin > tmax) return 0;
		}
	}
	// Ray intersects all 3 slabs. Return point (q) and intersection t value (tmin)
	q = p + d * tmin;
	return 1;
}

//-----------------------------------------------------------------------------
// Intersects a ray against an OBB
//-----------------------------------------------------------------------------
bool IntersectRayWithOBB(const Vector &vecRayStart, const Vector &vecRayDelta,
	const matrix3x4_t &matOBBToWorld, const Vector &vecOBBMins, const Vector &vecOBBMaxs,
	float flTolerance, CBaseTrace *pTrace, Vector* vecExitPos)
{
	Collision_ClearTrace(vecRayStart, vecRayDelta, pTrace);

	// FIXME: Make it work with tolerance
	//Assert(flTolerance == 0.0f);

	// OPTIMIZE: Store this in the box instead of computing it here
	// compute center in local space
	Vector vecBoxExtents = (vecOBBMins + vecOBBMaxs) * 0.5;
	Vector vecBoxCenter;

	// transform to world space
	VectorTransformZ(vecBoxExtents, matOBBToWorld, vecBoxCenter);

	// calc extents from local center
	vecBoxExtents = vecOBBMaxs - vecBoxExtents;

	// OPTIMIZE: This is optimized for world space.  If the transform is fast enough, it may make more
	// sense to just xform and call UTIL_ClipToBox() instead.  MEASURE THIS.

	// save the extents of the ray along 
	Vector extent, uextent;
	Vector segmentCenter = vecRayStart + vecRayDelta - vecBoxCenter;

	extent.Init();

	// check box axes for separation
	for (int j = 0; j < 3; j++)
	{
		extent[j] = vecRayDelta.x * matOBBToWorld[0][j] + vecRayDelta.y * matOBBToWorld[1][j] + vecRayDelta.z * matOBBToWorld[2][j];
		uextent[j] = fabsf(extent[j]);
		float coord = segmentCenter.x * matOBBToWorld[0][j] + segmentCenter.y * matOBBToWorld[1][j] + segmentCenter.z * matOBBToWorld[2][j];
		coord = fabsf(coord);

		if (coord >(vecBoxExtents[j] + uextent[j]))
			return false;
	}

	// now check cross axes for separation
	float tmp, cextent;
	Vector cross = vecRayDelta.Cross(segmentCenter);
	cextent = cross.x * matOBBToWorld[0][0] + cross.y * matOBBToWorld[1][0] + cross.z * matOBBToWorld[2][0];
	cextent = fabsf(cextent);
	tmp = vecBoxExtents[1] * uextent[2] + vecBoxExtents[2] * uextent[1];
	if (cextent > tmp)
		return false;

	cextent = cross.x * matOBBToWorld[0][1] + cross.y * matOBBToWorld[1][1] + cross.z * matOBBToWorld[2][1];
	cextent = fabsf(cextent);
	tmp = vecBoxExtents[0] * uextent[2] + vecBoxExtents[2] * uextent[0];
	if (cextent > tmp)
		return false;

	cextent = cross.x * matOBBToWorld[0][2] + cross.y * matOBBToWorld[1][2] + cross.z * matOBBToWorld[2][2];
	cextent = fabsf(cextent);
	tmp = vecBoxExtents[0] * uextent[1] + vecBoxExtents[1] * uextent[0];
	if (cextent > tmp)
		return false;

	// !!! We hit this box !!! compute intersection point and return
	// Compute ray start in bone space
	Vector start;
	VectorITransform(&vecRayStart, matOBBToWorld, &start);

	// extent is ray.m_Delta in bone space, recompute delta in bone space
	extent *= 2.0f;

	// delta was prescaled by the current t, so no need to see if this intersection
	// is closer
	trace_t boxTrace;
	float fractionleftsolid; //dylan added
	if (!IntersectRayWithBox(start, extent, vecOBBMins, vecOBBMaxs, flTolerance, pTrace, &fractionleftsolid))
		return false;

	//dylan added
	if (vecExitPos)
	{
		if (pTrace->allsolid)
			*vecExitPos = vecRayStart + vecRayDelta;
		else
		{
			Vector exit = start + extent * fractionleftsolid;
			VectorTransformZ(exit, matOBBToWorld, *vecExitPos);
		}
	}

	// Fix up the start/end pos and fraction
	Vector vecTemp;
	VectorTransformZ(pTrace->endpos, matOBBToWorld, vecTemp);
	pTrace->endpos = vecTemp;

	pTrace->startpos = vecRayStart;
	pTrace->fraction *= 2.0f;

	// Fix up the plane information
	float flSign = pTrace->plane.normal[pTrace->plane.type];
	pTrace->plane.normal[0] = flSign * matOBBToWorld[0][pTrace->plane.type];
	pTrace->plane.normal[1] = flSign * matOBBToWorld[1][pTrace->plane.type];
	pTrace->plane.normal[2] = flSign * matOBBToWorld[2][pTrace->plane.type];
	pTrace->plane.dist = DotProduct(pTrace->endpos, pTrace->plane.normal);
	pTrace->plane.type = 3;

	return true;
}

//-----------------------------------------------------------------------------
// returns true if there's an intersection between ray and sphere
//-----------------------------------------------------------------------------
bool IsRayIntersectingSphere(const Vector &vecRayOrigin, const Vector &vecRayDelta,
	const Vector& vecCenter, float flRadius, float flTolerance)
{
	// For this algorithm, find a point on the ray  which is closest to the sphere origin
	// Do this by making a plane passing through the sphere origin
	// whose normal is parallel to the ray. Intersect that plane with the ray.
	// Plane: N dot P = I, N = D (ray direction), I = C dot N = C dot D
	// Ray: P = O + D * t
	// D dot ( O + D * t ) = C dot D
	// D dot O + D dot D * t = C dot D
	// t = (C - O) dot D / D dot D
	// Clamp t to (0,1)
	// Find distance of the point on the ray to the sphere center.
	//Assert(flTolerance >= 0.0f);
	flRadius += flTolerance;

	Vector vecRayToSphere;
	VectorSubtract(vecCenter, vecRayOrigin, vecRayToSphere);
	float flNumerator = DotProduct(vecRayToSphere, vecRayDelta);

	float t;
	if (flNumerator <= 0.0f)
	{
		t = 0.0f;
	}
	else
	{
		float flDenominator = DotProduct(vecRayDelta, vecRayDelta);
		if (flNumerator > flDenominator)
			t = 1.0f;
		else
			t = flNumerator / flDenominator;
	}

	Vector vecClosestPoint;
	VectorMA(vecRayOrigin, t, vecRayDelta, vecClosestPoint);
	return (vecClosestPoint.DistToSqr(vecCenter) <= flRadius * flRadius);

	// NOTE: This in an alternate algorithm which I didn't use because I'd have to use a sqrt
	// So it's probably faster to do this other algorithm. I'll leave the comments here
	// for how to go back if we want to

	// Solve using the ray equation + the sphere equation
	// P = o + dt
	// (x - xc)^2 + (y - yc)^2 + (z - zc)^2 = r^2
	// (ox + dx * t - xc)^2 + (oy + dy * t - yc)^2 + (oz + dz * t - zc)^2 = r^2
	// (ox - xc)^2 + 2 * (ox-xc) * dx * t + dx^2 * t^2 +
	//		(oy - yc)^2 + 2 * (oy-yc) * dy * t + dy^2 * t^2 +
	//		(oz - zc)^2 + 2 * (oz-zc) * dz * t + dz^2 * t^2 = r^2
	// (dx^2 + dy^2 + dz^2) * t^2 + 2 * ((ox-xc)dx + (oy-yc)dy + (oz-zc)dz) t +
	//		(ox-xc)^2 + (oy-yc)^2 + (oz-zc)^2 - r^2 = 0
	// or, t = (-b +/- sqrt( b^2 - 4ac)) / 2a
	// a = DotProduct( vecRayDelta, vecRayDelta );
	// b = 2 * DotProduct( vecRayOrigin - vecCenter, vecRayDelta )
	// c = DotProduct(vecRayOrigin - vecCenter, vecRayOrigin - vecCenter) - flRadius * flRadius;
	// Valid solutions are possible only if b^2 - 4ac >= 0
	// Therefore, compute that value + see if we got it	
}

//-----------------------------------------------------------------------------
// Box support map
//-----------------------------------------------------------------------------
inline void ComputeSupportMap(const Vector &vecDirection, const Vector &vecBoxMins,
	const Vector &vecBoxMaxs, float pDist[2])
{
	int nIndex = (vecDirection.x > 0.0f);
	pDist[nIndex] = vecBoxMaxs.x * vecDirection.x;
	pDist[1 - nIndex] = vecBoxMins.x * vecDirection.x;

	nIndex = (vecDirection.y > 0.0f);
	pDist[nIndex] += vecBoxMaxs.y * vecDirection.y;
	pDist[1 - nIndex] += vecBoxMins.y * vecDirection.y;

	nIndex = (vecDirection.z > 0.0f);
	pDist[nIndex] += vecBoxMaxs.z * vecDirection.z;
	pDist[1 - nIndex] += vecBoxMins.z * vecDirection.z;
}

inline void ComputeSupportMap(const Vector &vecDirection, int i1, int i2,
	const Vector &vecBoxMins, const Vector &vecBoxMaxs, float pDist[2])
{
	int nIndex = (vecDirection[i1] > 0.0f);
	pDist[nIndex] = vecBoxMaxs[i1] * vecDirection[i1];
	pDist[1 - nIndex] = vecBoxMins[i1] * vecDirection[i1];

	nIndex = (vecDirection[i2] > 0.0f);
	pDist[nIndex] += vecBoxMaxs[i2] * vecDirection[i2];
	pDist[1 - nIndex] += vecBoxMins[i2] * vecDirection[i2];
}

//-----------------------------------------------------------------------------
// Intersects a ray against an OBB
//-----------------------------------------------------------------------------
static int s_ExtIndices[3][2] =
{
	{ 2, 1 },
	{ 0, 2 },
	{ 0, 1 },
};

static int s_MatIndices[3][2] =
{
	{ 1, 2 },
	{ 2, 0 },
	{ 1, 0 },
};

bool IntersectRayWithOBB(const Ray_t &ray, const matrix3x4_t &matOBBToWorld,
	const Vector &vecOBBMins, const Vector &vecOBBMaxs, float flTolerance, CBaseTrace *pTrace, Vector *vecExitPos)
{

	//Transform ray into model space of hitbox so we only have to deal with an AABB instead of OBB
	Vector ray_trans, dir_trans;
	VectorITransform(&ray.m_Start, matOBBToWorld, &ray_trans);
	VectorIRotate(&ray.m_Delta, matOBBToWorld, &dir_trans); //only rotate direction vector! no translation!

	//return IntersectRayWithAABB(ray_trans, dir_trans,(Vector&) vecOBBMins, (Vector&)vecOBBMaxs, &pTrace->endpos, vecExitPos);

	//NOTE: THE SOURCE 2013+ VERSION OF THIS FUNCTION IS BROKEN, IT SEEMS TO ALWAYS RETURN THAT THE RAY CAN HIT THE OBB EVEN IF THE RAY ISN'T LONG ENOUGH TO REACH THE OBB
	//SO I REVERTED THIS TO SOURCE2007 IntersectRayWithAABB

	if (ray.m_IsRay)
	{
		return IntersectRayWithOBB(ray.m_Start, ray.m_Delta, matOBBToWorld,
			vecOBBMins, vecOBBMaxs, flTolerance, pTrace, vecExitPos);
	}

	Collision_ClearTrace(ray.m_Start + ray.m_StartOffset, ray.m_Delta, pTrace);

	// Compute a bounding sphere around the bloated OBB
	Vector vecOBBCenter;
	VectorAdd(vecOBBMins, vecOBBMaxs, vecOBBCenter);
	vecOBBCenter *= 0.5f;
	vecOBBCenter.x += matOBBToWorld[0][3];
	vecOBBCenter.y += matOBBToWorld[1][3];
	vecOBBCenter.z += matOBBToWorld[2][3];

	Vector vecOBBHalfDiagonal;
	VectorSubtract(vecOBBMaxs, vecOBBMins, vecOBBHalfDiagonal);
	vecOBBHalfDiagonal *= 0.5f;

	float flRadius = vecOBBHalfDiagonal.Length() + ray.m_Extents.Length();
	if (!IsRayIntersectingSphere(ray.m_Start, ray.m_Delta, vecOBBCenter, flRadius, flTolerance))
		return false;

	// Ok, we passed the trivial reject, so lets do the dirty deed.
	// Basically we're going to do the GJK thing explicitly. We'll shrink the ray down
	// to a point, and bloat the OBB by the ray's extents. This will generate facet
	// planes which are perpendicular to all of the separating axes typically seen in
	// a standard seperating axis implementation.

	// We're going to create a number of planes through various vertices in the OBB
	// which represent all of the separating planes. Then we're going to bloat the planes
	// by the ray extents.

	// We're going to do all work in OBB-space because it's easier to do the
	// support-map in this case

	// First, transform the ray into the space of the OBB
	Vector vecLocalRayOrigin, vecLocalRayDirection;
	VectorITransform(&ray.m_Start, matOBBToWorld, &vecLocalRayOrigin);
	VectorIRotate(ray.m_Delta, matOBBToWorld, vecLocalRayDirection);

	// Next compute all separating planes
	Vector pPlaneNormal[15];
	float ppPlaneDist[15][2];

	int i;
	for (i = 0; i < 3; ++i)
	{
		// Each plane needs to be bloated an amount = to the abs dot product of 
		// the ray extents with the plane normal
		// For the OBB planes, do it in world space; 
		// and use the direction of the OBB (the ith column of matOBBToWorld) in world space vs extents
		pPlaneNormal[i].Init();
		pPlaneNormal[i][i] = 1.0f;

		float flExtentDotNormal =
			FloatMakePositive(matOBBToWorld[0][i] * ray.m_Extents.x) +
			FloatMakePositive(matOBBToWorld[1][i] * ray.m_Extents.y) +
			FloatMakePositive(matOBBToWorld[2][i] * ray.m_Extents.z);

		ppPlaneDist[i][0] = vecOBBMins[i] - flExtentDotNormal;
		ppPlaneDist[i][1] = vecOBBMaxs[i] + flExtentDotNormal;

		// For the ray-extents planes, they are bloated by the extents
		// Use the support map to determine which
		VectorCopy(matOBBToWorld[i], pPlaneNormal[i + 3].Base());
		ComputeSupportMap(pPlaneNormal[i + 3], vecOBBMins, vecOBBMaxs, ppPlaneDist[i + 3]);
		ppPlaneDist[i + 3][0] -= ray.m_Extents[i];
		ppPlaneDist[i + 3][1] += ray.m_Extents[i];

		// Now the edge cases... (take the cross product of x,y,z axis w/ ray extent axes
		// given by the rows of the obb to world matrix. 
		// Compute the ray extent bloat in world space because it's easier...

		// These are necessary to compute the world-space versions of
		// the edges so we can compute the extent dot products
		float flRayExtent0 = ray.m_Extents[s_ExtIndices[i][0]];
		float flRayExtent1 = ray.m_Extents[s_ExtIndices[i][1]];
		const float *pMatRow0 = matOBBToWorld[s_MatIndices[i][0]];
		const float *pMatRow1 = matOBBToWorld[s_MatIndices[i][1]];

		// x axis of the OBB + world ith axis
		pPlaneNormal[i + 6].Init(0.0f, -matOBBToWorld[i][2], matOBBToWorld[i][1]);
		ComputeSupportMap(pPlaneNormal[i + 6], 1, 2, vecOBBMins, vecOBBMaxs, ppPlaneDist[i + 6]);
		flExtentDotNormal =
			FloatMakePositive(pMatRow0[0]) * flRayExtent0 +
			FloatMakePositive(pMatRow1[0]) * flRayExtent1;
		ppPlaneDist[i + 6][0] -= flExtentDotNormal;
		ppPlaneDist[i + 6][1] += flExtentDotNormal;

		// y axis of the OBB + world ith axis
		pPlaneNormal[i + 9].Init(matOBBToWorld[i][2], 0.0f, -matOBBToWorld[i][0]);
		ComputeSupportMap(pPlaneNormal[i + 9], 0, 2, vecOBBMins, vecOBBMaxs, ppPlaneDist[i + 9]);
		flExtentDotNormal =
			FloatMakePositive(pMatRow0[1]) * flRayExtent0 +
			FloatMakePositive(pMatRow1[1]) * flRayExtent1;
		ppPlaneDist[i + 9][0] -= flExtentDotNormal;
		ppPlaneDist[i + 9][1] += flExtentDotNormal;

		// z axis of the OBB + world ith axis
		pPlaneNormal[i + 12].Init(-matOBBToWorld[i][1], matOBBToWorld[i][0], 0.0f);
		ComputeSupportMap(pPlaneNormal[i + 12], 0, 1, vecOBBMins, vecOBBMaxs, ppPlaneDist[i + 12]);
		flExtentDotNormal =
			FloatMakePositive(pMatRow0[2]) * flRayExtent0 +
			FloatMakePositive(pMatRow1[2]) * flRayExtent1;
		ppPlaneDist[i + 12][0] -= flExtentDotNormal;
		ppPlaneDist[i + 12][1] += flExtentDotNormal;
	}

	float enterfrac, leavefrac;
	float d1[2], d2[2];
	float f;

	int hitplane = -1;
	int hitside = -1;
	enterfrac = -1.0f;
	leavefrac = 1.0f;

	pTrace->startsolid = true;

	Vector vecLocalRayEnd;
	VectorAdd(vecLocalRayOrigin, vecLocalRayDirection, vecLocalRayEnd);

	for (i = 0; i < 15; ++i)
	{
		// FIXME: Not particularly optimal since there's a lot of 0's in the plane normals
		float flStartDot = DotProduct(pPlaneNormal[i], vecLocalRayOrigin);
		float flEndDot = DotProduct(pPlaneNormal[i], vecLocalRayEnd);

		// NOTE: Negative here is because the plane normal + dist 
		// are defined in negative terms for the far plane (plane dist index 0)
		d1[0] = -(flStartDot - ppPlaneDist[i][0]);
		d2[0] = -(flEndDot - ppPlaneDist[i][0]);

		d1[1] = flStartDot - ppPlaneDist[i][1];
		d2[1] = flEndDot - ppPlaneDist[i][1];

		int j;
		for (j = 0; j < 2; ++j)
		{
			// if completely in front near plane or behind far plane no intersection
			if (d1[j] > 0 && d2[j] > 0)
				return false;

			// completely inside, check next plane set
			if (d1[j] <= 0 && d2[j] <= 0)
				continue;

			if (d1[j] > 0)
			{
				pTrace->startsolid = false;
			}

			// crosses face
			float flDenom = 1.0f / (d1[j] - d2[j]);
			if (d1[j] > d2[j])
			{
				f = d1[j] - flTolerance;
				if (f < 0)
				{
					f = 0;
				}
				f *= flDenom;
				if (f > enterfrac)
				{
					enterfrac = f;
					hitplane = i;
					hitside = j;
				}
			}
			else
			{
				// leave
				f = (d1[j] + flTolerance) * flDenom;
				if (f < leavefrac)
				{
					leavefrac = f;
				}
			}
		}
	}

	if (enterfrac < leavefrac && enterfrac >= 0.0f)
	{
		pTrace->fraction = enterfrac;
		VectorMA(pTrace->startpos, enterfrac, ray.m_Delta, pTrace->endpos);
		pTrace->contents = CONTENTS_SOLID;

		// Need to transform the plane into world space...
		cplane_t temp;
		temp.normal = pPlaneNormal[hitplane];
		temp.dist = ppPlaneDist[hitplane][hitside];
		if (hitside == 0)
		{
			temp.normal *= -1.0f;
			temp.dist *= -1.0f;
		}
		temp.type = 3;

		MatrixITransformPlane(matOBBToWorld, temp, pTrace->plane);
		return true;
	}

	if (pTrace->startsolid)
	{
		pTrace->allsolid = (leavefrac <= 0.0f) || (leavefrac >= 1.0f);
		if (vecExitPos && !pTrace->allsolid)
		{			
#if _DEBUG
			//decrypts(0)
			MessageBoxA(NULL, XorStr("Non-Ray exitpos not implemented!"), "", MB_OK);
			//encrypts(0)
#endif

			DebugBreak();
		}
		pTrace->fraction = 0;
		pTrace->endpos = pTrace->startpos;
		pTrace->contents = CONTENTS_SOLID;
		pTrace->plane.dist = pTrace->startpos[0];
		pTrace->plane.normal.Init(1.0f, 0.0f, 0.0f);
		pTrace->plane.type = 0;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Intersects a ray against an OBB
//-----------------------------------------------------------------------------
bool IntersectRayWithOBB(const Vector &vecRayOrigin, const Vector &vecRayDelta,
	const Vector &vecBoxOrigin, const QAngle &angBoxRotation,
	const Vector &vecOBBMins, const Vector &vecOBBMaxs, float flTolerance, CBaseTrace *pTrace)
{
	if (angBoxRotation == angZero)
	{
		Vector vecAbsMins, vecAbsMaxs;
		VectorAdd(vecBoxOrigin, vecOBBMins, vecAbsMins);
		VectorAdd(vecBoxOrigin, vecOBBMaxs, vecAbsMaxs);
		return IntersectRayWithBox(vecRayOrigin, vecRayDelta, vecAbsMins, vecAbsMaxs, flTolerance, pTrace);
	}

	matrix3x4_t obbToWorld;
	AngleMatrix(angBoxRotation, vecBoxOrigin, obbToWorld);
	return IntersectRayWithOBB(vecRayOrigin, vecRayDelta, obbToWorld, vecOBBMins, vecOBBMaxs, flTolerance, pTrace);
}

//-----------------------------------------------------------------------------
// Intersects a ray against an OBB, returns t1 and t2
//-----------------------------------------------------------------------------
bool IntersectRayWithOBB(const Vector &vecRayStart, const Vector &vecRayDelta,
	const matrix3x4_t &matOBBToWorld, const Vector &vecOBBMins, const Vector &vecOBBMaxs,
	float flTolerance, BoxTraceInfo_t *pTrace)
{
	// FIXME: Two transforms is pretty expensive. Should we optimize this?
	Vector start, delta;
	VectorITransform(&vecRayStart, matOBBToWorld, &start);
	VectorIRotate(vecRayDelta, matOBBToWorld, delta);

	return IntersectRayWithBox(start, delta, vecOBBMins, vecOBBMaxs, flTolerance, pTrace);
}

//-----------------------------------------------------------------------------
// Intersects a ray against an OBB
//-----------------------------------------------------------------------------
bool IntersectRayWithOBB(const Ray_t &ray, const Vector &vecBoxOrigin, const QAngle &angBoxRotation,
	const Vector &vecOBBMins, const Vector &vecOBBMaxs, float flTolerance, CBaseTrace *pTrace)
{
	if (angBoxRotation == angZero)
	{
		Vector vecWorldMins, vecWorldMaxs;
		VectorAdd(vecBoxOrigin, vecOBBMins, vecWorldMins);
		VectorAdd(vecBoxOrigin, vecOBBMaxs, vecWorldMaxs);
		return IntersectRayWithBox(ray, vecWorldMins, vecWorldMaxs, flTolerance, pTrace);
	}

	if (ray.m_IsRay)
	{
		return IntersectRayWithOBB(ray.m_Start, ray.m_Delta, vecBoxOrigin, angBoxRotation, vecOBBMins, vecOBBMaxs, flTolerance, pTrace);
	}

	matrix3x4_t matOBBToWorld;
	AngleMatrix(angBoxRotation, vecBoxOrigin, matOBBToWorld);
	return IntersectRayWithOBB(ray, matOBBToWorld, vecOBBMins, vecOBBMaxs, flTolerance, pTrace);
}

void TRACE_HITBOX(CBaseEntity* Entity, Ray_t& ray, trace_t &tr, std::vector<CSphere>&m_cSpheres, std::vector<COBB>&m_cOBBs, Vector *vecExitPoint)
{
	tr.m_pEnt = nullptr;
	int bestphysicsbone;
	int besthitgroup = 0;
	int besthitbox;
	Vector bestendpos;
	bool bestisneck = false;
	bool bestisextremity = false;
	float bestdistancetoentrypoint;
	//float worstdistancetoentrypoint;
	float worstdistancetoexitpoint;
	Vector bestexitpoint;
	Vector entrypoint;
	Vector exitpoint;
	float distancetoentrypoint;
	float distancetoexitpoint;

	for (auto& i : m_cSpheres)
	{
		if (i.intersectsRay(ray, &entrypoint, &exitpoint))
		{
			//Interfaces::DebugOverlay->AddBoxOverlay(intersectpos, Vector(-1, -1, -1), Vector(1, 1, 1), QAngle(0, 0, 0), 0, 255, 0, 255, 4.0f);
			const int hitgroup = i.Hitgroup;
			bool isextremity = hitgroup >= HITGROUP_LEFTARM && hitgroup <= HITGROUP_NECK;
			bool isneck = hitgroup == HITGROUP_NECK;
			distancetoentrypoint = (entrypoint - ray.m_Start).Length();
			distancetoexitpoint = (exitpoint - ray.m_Start).Length();

			if (besthitgroup)
			{
#if 1
				if (distancetoexitpoint > worstdistancetoexitpoint)
				{
					bestexitpoint = exitpoint;
					worstdistancetoexitpoint = distancetoexitpoint;
				}
#else
				if (distancetoentrypoint > worstdistancetoentrypoint)
				{
					bestexitpoint = exitpoint;
					worstdistancetoentrypoint = distancetoentrypoint;
				}
#endif

				if (distancetoentrypoint < bestdistancetoentrypoint)
				{
					if (!bestisextremity)
					{
						if (!isextremity)
						{
							bestdistancetoentrypoint = distancetoentrypoint;
							goto SetBest;
						}
					}
					else
					{
						if (isneck || !bestisneck)
						{
							bestdistancetoentrypoint = distancetoentrypoint;
							goto SetBest;
						}
					}
				}
				else
				{
					if (bestisextremity && (!isextremity || (isneck && !bestisneck)))
					{
						bestdistancetoentrypoint = distancetoentrypoint;
						goto SetBest;
					}
				}
			}
			else
			{
				bestexitpoint = exitpoint;
				bestdistancetoentrypoint = distancetoentrypoint;
				//worstdistancetoentrypoint = distancetoentrypoint;
				worstdistancetoexitpoint = distancetoexitpoint;

			SetBest:
				besthitgroup = hitgroup;
				besthitbox = i.Hitbox;
				bestendpos = entrypoint;
				bestisneck = isneck;
				bestisextremity = isextremity;
				bestphysicsbone = i.PhysicsBone;
			}
		}
	}

	for (auto& i : m_cOBBs)
	{
		trace_t newtr;
		//const Ray_t &ray, const matrix3x4_t &matOBBToWorld,
		//	const Vector &vecOBBMins, const Vector &vecOBBMaxs, float flTolerance, CBaseTrace *pTrace
		matrix3x4_t matrix_rotation;
		AngleMatrix(i.angles, matrix_rotation);

		matrix3x4_t matrix_new;
		ConcatTransforms(*i.boneMatrix, matrix_rotation, matrix_new);

		QAngle angles;
		MatrixAngles(matrix_new, angles);
		if (IntersectRayWithOBB(ray, matrix_new, i.vecbbMin, i.vecbbMax, 0.f, &newtr, &exitpoint))
		//if (IntersectRayWithOBB(ray, i.vecbbMin, i.vecbbMax, i.angles, *i.boneMatrix, &entrypoint, &exitpoint))
		{
			entrypoint = newtr.endpos;
			const int hitgroup = i.hitgroup;
			bool isextremity = hitgroup >= HITGROUP_LEFTARM && hitgroup <= HITGROUP_NECK;
			bool isneck = hitgroup == HITGROUP_NECK;
			distancetoentrypoint = (entrypoint - ray.m_Start).Length();
			distancetoexitpoint = (exitpoint - ray.m_Start).Length();

			if (besthitgroup)
			{
#if 1
				if (distancetoexitpoint > worstdistancetoexitpoint)
				{
					bestexitpoint = exitpoint;
					worstdistancetoexitpoint = distancetoexitpoint;
				}
#else
				if (distancetoentrypoint > worstdistancetoentrypoint)
				{
					bestexitpoint = exitpoint;
					worstdistancetoentrypoint = distancetoentrypoint;
				}
#endif

				if (distancetoentrypoint < bestdistancetoentrypoint)
				{
					if (!bestisextremity)
					{
						if (!isextremity)
						{
							bestdistancetoentrypoint = distancetoentrypoint;
							goto SetBest2;
						}
					}
					else
					{
						if (isneck || !bestisneck)
						{
							bestdistancetoentrypoint = distancetoentrypoint;
							goto SetBest2;
						}
					}
				}
				else
				{
					if (bestisextremity && (!isextremity || (isneck && !bestisneck)))
					{
						bestdistancetoentrypoint = distancetoentrypoint;
						goto SetBest2;
					}
				}
			}
			else
			{
				bestexitpoint = exitpoint;
				bestdistancetoentrypoint = distancetoentrypoint;
				//worstdistancetoentrypoint = distancetoentrypoint;
				worstdistancetoexitpoint = distancetoexitpoint;

			SetBest2:
				besthitgroup = hitgroup;
				besthitbox = i.hitbox;
				bestendpos = entrypoint;
				bestisneck = isneck;
				bestisextremity = isextremity;
				bestphysicsbone = i.physicsbone;
			}
		}
	}

	if (besthitgroup)
	{
		float flMaxLength = ((ray.m_Start + ray.m_Delta) - ray.m_Start).Length();
		float flLengthToHitbox = (bestendpos - ray.m_Start).Length();
		tr.m_pEnt = Entity;
		tr.endpos = bestendpos;
		tr.fraction = flLengthToHitbox / flMaxLength;
		tr.hitbox = besthitbox;
		tr.hitgroup = besthitgroup;
		tr.allsolid = true;
		tr.startsolid = true;

		CStudioHdr *pStudioHdr = Entity->GetModelPtr();
		const mstudiobbox_t *pbox = pStudioHdr->_m_pStudioHdr->pHitboxSet(Entity->GetHitboxSet())->pHitbox(besthitbox);
		const mstudiobone_t *pBone = pStudioHdr->pBone(pbox->bone);
		tr.contents = pBone->contents | CONTENTS_HITBOX;
		tr.physicsbone = bestphysicsbone;//pBone->physicsbone;
		//decrypts(0)
		tr.surface.name = XorStr("**studio**");
		//encrypts(0)
		tr.surface.flags = SURF_HITBOX;
		tr.surface.surfaceProps = Interfaces::Physprops->GetSurfaceIndex(pBone->pszSurfaceProp());

		if (vecExitPoint)
			*vecExitPoint = bestexitpoint;

		//entry point
		//Interfaces::DebugOverlay->AddBoxOverlay(bestendpos, Vector(-0.25f, -0.25f, -0.25f), Vector(0.1f, 0.1f, 0.1f), angZero, 0, 255, 0, 100, TICKS_TO_TIME(2));
		//exit point
		//Interfaces::DebugOverlay->AddBoxOverlay(bestexitpoint, Vector(-0.5f, -0.5f, -0.5f), Vector(0.5f, 0.5f, 0.5f), angZero, 255, 0, 0, 255, TICKS_TO_TIME(2));
		return;
	}
	tr.endpos = ray.m_Start + ray.m_Delta;
	tr.fraction = 1.0f;
}
void TRACE_HITBOX_SCALE(CBaseEntity* Entity, Ray_t& ray, trace_t &tr, std::vector<CSphere>&m_cSpheres, std::vector<COBB>&m_cOBBs, Vector *vecExitPoint, float scale)
{
	tr.m_pEnt = nullptr;
	int besthitgroup = 0;
	int besthitbox;
	int bestphysicsbone;
	Vector bestendpos;
	bool bestisneck = false;
	bool bestisextremity = false;
	float bestdistancetoentrypoint;
	float worstdistancetoexitpoint;
	//float worstdistancetoentrypoint;
	Vector bestexitpoint;
	Vector entrypoint;
	Vector exitpoint;
	float distancetoentrypoint;
	float distancetoexitpoint;

	for (auto& i : m_cSpheres)
	{
		if (i.intersectsRay(ray, &entrypoint, &exitpoint, scale))
		{
			//Interfaces::DebugOverlay->AddBoxOverlay(intersectpos, Vector(-1, -1, -1), Vector(1, 1, 1), QAngle(0, 0, 0), 0, 255, 0, 255, 4.0f);
			const int hitgroup = i.Hitgroup;
			bool isextremity = hitgroup >= HITGROUP_LEFTARM && hitgroup <= HITGROUP_NECK;
			bool isneck = hitgroup == HITGROUP_NECK;
			distancetoentrypoint = (entrypoint - ray.m_Start).Length();
			distancetoexitpoint = (exitpoint - ray.m_Start).Length();

			if (besthitgroup)
			{
#if 1
				if (distancetoexitpoint > worstdistancetoexitpoint)
				{
					bestexitpoint = exitpoint;
					worstdistancetoexitpoint = distancetoexitpoint;
				}
#else
				if (distancetoentrypoint > worstdistancetoentrypoint)
				{
					bestexitpoint = exitpoint;
					worstdistancetoentrypoint = distancetoentrypoint;
				}
#endif

				if (distancetoentrypoint < bestdistancetoentrypoint)
				{
					if (!bestisextremity)
					{
						if (!isextremity)
						{
							bestdistancetoentrypoint = distancetoentrypoint;
							goto SetBest;
						}
					}
					else
					{
						if (isneck || !bestisneck)
						{
							bestdistancetoentrypoint = distancetoentrypoint;
							goto SetBest;
						}
					}
				}
				else
				{
					if (bestisextremity && (!isextremity || (isneck && !bestisneck)))
					{
						bestdistancetoentrypoint = distancetoentrypoint;
						goto SetBest;
					}
				}
			}
			else
			{
				bestexitpoint = exitpoint;
				bestdistancetoentrypoint = distancetoentrypoint;
				//worstdistancetoentrypoint = distancetoentrypoint;
				worstdistancetoexitpoint = distancetoexitpoint;

			SetBest:
				besthitgroup = hitgroup;
				besthitbox = i.Hitbox;
				bestendpos = entrypoint;
				bestisneck = isneck;
				bestisextremity = isextremity;
				bestphysicsbone = i.PhysicsBone;
			}
		}
	}

	for (auto& i : m_cOBBs)
	{
		trace_t newtr;
		//const Ray_t &ray, const matrix3x4_t &matOBBToWorld,
		//	const Vector &vecOBBMins, const Vector &vecOBBMaxs, float flTolerance, CBaseTrace *pTrace
		if (IntersectRayWithOBB(ray, *i.boneMatrix, i.vecbbMin, i.vecbbMax, 0.f, &newtr, &exitpoint))
		//if (IntersectRayWithOBB(ray, i.vecbbMin, i.vecbbMax, *i.boneMatrix, &entrypoint, &exitpoint))
		{
			entrypoint = newtr.endpos;
			const int hitgroup = i.hitgroup;
			bool isextremity = hitgroup >= HITGROUP_LEFTARM && hitgroup <= HITGROUP_NECK;
			bool isneck = hitgroup == HITGROUP_NECK;
			distancetoentrypoint = (entrypoint - ray.m_Start).Length();
			distancetoexitpoint = (exitpoint - ray.m_Start).Length();

			if (besthitgroup)
			{
#if 1
				if (distancetoexitpoint > worstdistancetoexitpoint)
				{
					bestexitpoint = exitpoint;
					worstdistancetoexitpoint = distancetoexitpoint;
				}
#else
				if (distancetoentrypoint > worstdistancetoentrypoint)
				{
					bestexitpoint = exitpoint;
					worstdistancetoentrypoint = distancetoentrypoint;
				}
#endif

				if (distancetoentrypoint < bestdistancetoentrypoint)
				{
					if (!bestisextremity)
					{
						if (!isextremity)
						{
							bestdistancetoentrypoint = distancetoentrypoint;
							goto SetBest2;
						}
					}
					else
					{
						if (isneck || !bestisneck)
						{
							bestdistancetoentrypoint = distancetoentrypoint;
							goto SetBest2;
						}
					}
				}
				else
				{
					if (bestisextremity && (!isextremity || (isneck && !bestisneck)))
					{
						bestdistancetoentrypoint = distancetoentrypoint;
						goto SetBest2;
					}
				}
			}
			else
			{
				bestexitpoint = exitpoint;
				bestdistancetoentrypoint = distancetoentrypoint;
				//worstdistancetoentrypoint = distancetoentrypoint;
				worstdistancetoexitpoint = distancetoexitpoint;

			SetBest2:
				besthitgroup = hitgroup;
				besthitbox = i.hitbox;
				bestendpos = entrypoint;
				bestisneck = isneck;
				bestisextremity = isextremity;
				bestphysicsbone = i.physicsbone;
			}
		}
	}

	if (besthitgroup)
	{
		float flMaxLength = ((ray.m_Start + ray.m_Delta) - ray.m_Start).Length();
		float flLengthToHitbox = (bestendpos - ray.m_Start).Length();
		tr.m_pEnt = Entity;
		tr.endpos = bestendpos;
		tr.fraction = flLengthToHitbox / flMaxLength;
		tr.hitbox = besthitbox;
		tr.hitgroup = besthitgroup;
		tr.allsolid = true;
		tr.startsolid = true;

		CStudioHdr *pStudioHdr = Entity->GetModelPtr();
		const mstudiobbox_t *pbox = pStudioHdr->_m_pStudioHdr->pHitboxSet(Entity->GetHitboxSet())->pHitbox(besthitbox);
		const mstudiobone_t *pBone = pStudioHdr->pBone(pbox->bone);
		tr.contents = pBone->contents | CONTENTS_HITBOX;
		tr.physicsbone = bestphysicsbone;// pBone->physicsbone;
		//decrypts(0)
		tr.surface.name = XorStr("**studio**");
		//encrypts(0)
		tr.surface.flags = SURF_HITBOX;
		tr.surface.surfaceProps = Interfaces::Physprops->GetSurfaceIndex(pBone->pszSurfaceProp());
		if (vecExitPoint)
			*vecExitPoint = bestexitpoint;

		//entry point
		//Interfaces::DebugOverlay->AddBoxOverlay(bestendpos, Vector(-0.25f, -0.25f, -0.25f), Vector(0.1f, 0.1f, 0.1f), angZero, 0, 255, 0, 100, TICKS_TO_TIME(2));
		//exit point
		//Interfaces::DebugOverlay->AddBoxOverlay(bestexitpoint, Vector(-0.5f, -0.5f, -0.5f), Vector(0.5f, 0.5f, 0.5f), angZero, 255, 0, 0, 255, TICKS_TO_TIME(2));
		return;
	}
	tr.endpos = ray.m_Start + ray.m_Delta;
	tr.fraction = 1.0f;
}
