//#include "bone_setup.h"

//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#include "dbg.h"
//#include "mathlib/mathlib.h"
#include "bone_setup.h"
#include <string.h>

//#include "collisionutils.h"
//#include "vstdlib/random.h"
//#include "tier0/vprof.h"
#include "bone_accessor.h"
//#include "mathlib/ssequaternion.h"
#include "bitvec.h"
//#include "datamanager.h"
#include "convar.h"
//#include "tier0/tslist.h"
#include "vphysics_interface.h"
#ifdef CLIENT_DLL
#include "posedebugger.h"
#endif

#include "studio.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "memdbgon.h"

class CBoneSetup
{
public:
	CBoneSetup(const CStudioHdr *pStudioHdr, int boneMask, const float poseParameter[], IPoseDebugger *pPoseDebugger = NULL);
	void InitPose(Vector pos[], Quaternion q[]);
	void AccumulatePose(Vector pos[], Quaternion q[], int sequence, float cycle, float flWeight, float flTime, CIKContext *pIKContext);
	void CalcAutoplaySequences(Vector pos[], Quaternion q[], float flRealTime, CIKContext *pIKContext);
private:
	void AddSequenceLayers(Vector pos[], Quaternion q[], mstudioseqdesc_t &seqdesc, int sequence, float cycle, float flWeight, float flTime, CIKContext *pIKContext);
	void AddLocalLayers(Vector pos[], Quaternion q[], mstudioseqdesc_t &seqdesc, int sequence, float cycle, float flWeight, float flTime, CIKContext *pIKContext);
public:
	const CStudioHdr *m_pStudioHdr;
	int m_boneMask;
	const float *m_flPoseParameter;
	IPoseDebugger *m_pPoseDebugger;
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void BuildBoneChain(
	const CStudioHdr *pStudioHdr,
	const matrix3x4_t &rootxform,
	const Vector pos[],
	const Quaternion q[],
	int	iBone,
	matrix3x4_t *pBoneToWorld)
{
	CBoneBitList boneComputed;
	BuildBoneChain(pStudioHdr, rootxform, pos, q, iBone, pBoneToWorld, boneComputed);
	return;
}


//-----------------------------------------------------------------------------
// Purpose: return a sub frame rotation for a single bone
//-----------------------------------------------------------------------------
void ExtractAnimValue(int frame, mstudioanimvalue_t *panimvalue, float scale, float &v1, float &v2)
{
	if (!panimvalue)
	{
		v1 = v2 = 0;
		return;
	}

	// Avoids a crash reading off the end of the data
	// There is probably a better long-term solution; Ken is going to look into it.
	if ((panimvalue->num.total == 1) && (panimvalue->num.valid == 1))
	{
		v1 = v2 = panimvalue[1].value * scale;
		return;
	}

	int k = frame;

	// find the data list that has the frame
	while (panimvalue->num.total <= k)
	{
		k -= panimvalue->num.total;
		panimvalue += panimvalue->num.valid + 1;
		if (panimvalue->num.total == 0)
		{
			//Assert(0); // running off the end of the animation stream is bad
			v1 = v2 = 0;
			return;
		}
	}
	if (panimvalue->num.valid > k)
	{
		// has valid animation data
		v1 = panimvalue[k + 1].value * scale;

		if (panimvalue->num.valid > k + 1)
		{
			// has valid animation blend data
			v2 = panimvalue[k + 2].value * scale;
		}
		else
		{
			if (panimvalue->num.total > k + 1)
			{
				// data repeats, no blend
				v2 = v1;
			}
			else
			{
				// pull blend from first data block in next list
				v2 = panimvalue[panimvalue->num.valid + 2].value * scale;
			}
		}
	}
	else
	{
		// get last valid data block
		v1 = panimvalue[panimvalue->num.valid].value * scale;
		if (panimvalue->num.total > k + 1)
		{
			// data repeats, no blend
			v2 = v1;
		}
		else
		{
			// pull blend from first data block in next list
			v2 = panimvalue[panimvalue->num.valid + 2].value * scale;
		}
	}
}


void ExtractAnimValue(int frame, mstudioanimvalue_t *panimvalue, float scale, float &v1)
{
	if (!panimvalue)
	{
		v1 = 0;
		return;
	}

	int k = frame;

	while (panimvalue->num.total <= k)
	{
		k -= panimvalue->num.total;
		panimvalue += panimvalue->num.valid + 1;
		if (panimvalue->num.total == 0)
		{
			//Assert(0); // running off the end of the animation stream is bad
			v1 = 0;
			return;
		}
	}
	if (panimvalue->num.valid > k)
	{
		v1 = panimvalue[k + 1].value * scale;
	}
	else
	{
		// get last valid data block
		v1 = panimvalue[panimvalue->num.valid].value * scale;
	}
}

//-----------------------------------------------------------------------------
// Purpose: return a sub frame rotation for a single bone
//-----------------------------------------------------------------------------
void CalcBoneQuaternion(int frame, float s,
	const Quaternion &baseQuat, const RadianEuler &baseRot, const Vector &baseRotScale,
	int iBaseFlags, const Quaternion &baseAlignment,
	const mstudioanim_t *panim, Quaternion &q)
{
	if (panim->flags & STUDIO_ANIM_RAWROT)
	{
		q = *(panim->pQuat48());
		//Assert(q.IsValid());
		return;
	}

	if (panim->flags & STUDIO_ANIM_RAWROT2)
	{
		q = *(panim->pQuat64());
		//Assert(q.IsValid());
		return;
	}

	if (!(panim->flags & STUDIO_ANIM_ANIMROT))
	{
		if (panim->flags & STUDIO_ANIM_DELTA)
		{
			q.Init(0.0f, 0.0f, 0.0f, 1.0f);
		}
		else
		{
			q = baseQuat;
		}
		return;
	}

	mstudioanim_valueptr_t *pValuesPtr = panim->pRotV();

	if (s > 0.001f)
	{
		QuaternionAligned	q1, q2;
		RadianEuler			angle1, angle2;

		ExtractAnimValue(frame, pValuesPtr->pAnimvalue(0), baseRotScale.x, angle1.x, angle2.x);
		ExtractAnimValue(frame, pValuesPtr->pAnimvalue(1), baseRotScale.y, angle1.y, angle2.y);
		ExtractAnimValue(frame, pValuesPtr->pAnimvalue(2), baseRotScale.z, angle1.z, angle2.z);

		if (!(panim->flags & STUDIO_ANIM_DELTA))
		{
			angle1.x = angle1.x + baseRot.x;
			angle1.y = angle1.y + baseRot.y;
			angle1.z = angle1.z + baseRot.z;
			angle2.x = angle2.x + baseRot.x;
			angle2.y = angle2.y + baseRot.y;
			angle2.z = angle2.z + baseRot.z;
		}

		//Assert(angle1.IsValid() && angle2.IsValid());
		if (angle1.x != angle2.x || angle1.y != angle2.y || angle1.z != angle2.z)
		{
			AngleQuaternion(angle1, q1);
			AngleQuaternion(angle2, q2);

#ifdef _X360
			fltx4 q1simd, q2simd, qsimd;
			q1simd = LoadAlignedSIMD(q1);
			q2simd = LoadAlignedSIMD(q2);
			qsimd = QuaternionBlendSIMD(q1simd, q2simd, s);
			StoreUnalignedSIMD(q.Base(), qsimd);
#else
			QuaternionBlend(q1, q2, s, q);
#endif
		}
		else
		{
			AngleQuaternion(angle1, q);
		}
	}
	else
	{
		RadianEuler			angle;

		ExtractAnimValue(frame, pValuesPtr->pAnimvalue(0), baseRotScale.x, angle.x);
		ExtractAnimValue(frame, pValuesPtr->pAnimvalue(1), baseRotScale.y, angle.y);
		ExtractAnimValue(frame, pValuesPtr->pAnimvalue(2), baseRotScale.z, angle.z);

		if (!(panim->flags & STUDIO_ANIM_DELTA))
		{
			angle.x = angle.x + baseRot.x;
			angle.y = angle.y + baseRot.y;
			angle.z = angle.z + baseRot.z;
		}

		//Assert(angle.IsValid());
		AngleQuaternion(angle, q);
	}

	//Assert(q.IsValid());

	// align to unified bone
	if (!(panim->flags & STUDIO_ANIM_DELTA) && (iBaseFlags & BONE_FIXED_ALIGNMENT))
	{
		QuaternionAlign(baseAlignment, q, q);
	}
}

inline void CalcBoneQuaternion(int frame, float s,
	const mstudiobone_t *pBone,
	const mstudiolinearbone_t *pLinearBones,
	const mstudioanim_t *panim, Quaternion &q)
{
	if (pLinearBones)
	{
		CalcBoneQuaternion(frame, s, pLinearBones->quat(panim->bone), pLinearBones->rot(panim->bone), pLinearBones->rotscale(panim->bone), pLinearBones->flags(panim->bone), pLinearBones->qalignment(panim->bone), panim, q);
	}
	else
	{
		CalcBoneQuaternion(frame, s, pBone->quat, pBone->rot, pBone->rotscale, pBone->flags, pBone->qAlignment, panim, q);
	}
}





//-----------------------------------------------------------------------------
// Purpose: return a sub frame position for a single bone
//-----------------------------------------------------------------------------
void CalcBonePosition(int frame, float s,
	const Vector &basePos, const Vector &baseBoneScale,
	const mstudioanim_t *panim, Vector &pos)
{
	if (panim->flags & STUDIO_ANIM_RAWPOS)
	{
		pos = *(panim->pPos());
		//Assert(pos.IsValid());

		return;
	}
	else if (!(panim->flags & STUDIO_ANIM_ANIMPOS))
	{
		if (panim->flags & STUDIO_ANIM_DELTA)
		{
			pos.Init(0.0f, 0.0f, 0.0f);
		}
		else
		{
			pos = basePos;
		}
		return;
	}

	mstudioanim_valueptr_t *pPosV = panim->pPosV();
	int					j;

	if (s > 0.001f)
	{
		float v1, v2;
		for (j = 0; j < 3; j++)
		{
			ExtractAnimValue(frame, pPosV->pAnimvalue(j), baseBoneScale[j], v1, v2);
			pos[j] = v1 * (1.0 - s) + v2 * s;
		}
	}
	else
	{
		for (j = 0; j < 3; j++)
		{
			ExtractAnimValue(frame, pPosV->pAnimvalue(j), baseBoneScale[j], pos[j]);
		}
	}

	if (!(panim->flags & STUDIO_ANIM_DELTA))
	{
		pos.x = pos.x + basePos.x;
		pos.y = pos.y + basePos.y;
		pos.z = pos.z + basePos.z;
	}

	//Assert(pos.IsValid());
}


inline void CalcBonePosition(int frame, float s,
	const mstudiobone_t *pBone,
	const mstudiolinearbone_t *pLinearBones,
	const mstudioanim_t *panim, Vector &pos)
{
	if (pLinearBones)
	{
		CalcBonePosition(frame, s, pLinearBones->pos(panim->bone), pLinearBones->posscale(panim->bone), panim, pos);
	}
	else
	{
		CalcBonePosition(frame, s, pBone->pos, pBone->posscale, panim, pos);
	}
}

void Studio_CalcBoneToBoneTransform(const CStudioHdr *pStudioHdr, int inputBoneIndex, int outputBoneIndex, matrix3x4_t& matrixOut)
{
	mstudiobone_t *pbone = pStudioHdr->pBone(inputBoneIndex);

	matrix3x4_t inputToPose;
	MatrixInvert(pbone->poseToBone, inputToPose);
	ConcatTransforms(pStudioHdr->pBone(outputBoneIndex)->poseToBone, inputToPose, matrixOut);
}

class CIKSolver
{
public:
	//-------- SOLVE TWO LINK INVERSE KINEMATICS -------------
	// Author: Ken Perlin
	//
	// Given a two link joint from [0,0,0] to end effector position P,
	// let link lengths be a and b, and let norm |P| = c.  Clearly a+b <= c.
	//
	// Problem: find a "knee" position Q such that |Q| = a and |P-Q| = b.
	//
	// In the case of a point on the x axis R = [c,0,0], there is a
	// closed form solution S = [d,e,0], where |S| = a and |R-S| = b:
	//
	//    d2+e2 = a2                  -- because |S| = a
	//    (c-d)2+e2 = b2              -- because |R-S| = b
	//
	//    c2-2cd+d2+e2 = b2           -- combine the two equations
	//    c2-2cd = b2 - a2
	//    c-2d = (b2-a2)/c
	//    d - c/2 = (a2-b2)/c / 2
	//
	//    d = (c + (a2-b2/c) / 2      -- to solve for d and e.
	//    e = sqrt(a2-d2)

	static float findD(float a, float b, float c) {
		return (c + (a*a - b * b) / c) / 2;
	}
	static float findE(float a, float d) { return sqrt(a*a - d * d); }

	// This leads to a solution to the more general problem:
	//
	//   (1) R = Mfwd(P)         -- rotate P onto the x axis
	//   (2) Solve for S
	//   (3) Q = Minv(S)         -- rotate back again

	float Mfwd[3][3];
	float Minv[3][3];

	bool solve(float A, float B, float const P[], float const D[], float Q[]) {
		float R[3];
		defineM(P, D);
		rot(Minv, P, R);
		float r = length(R);
		float d = findD(A, B, r);
		float e = findE(A, d);
		float S[3] = { d,e,0 };
		rot(Mfwd, S, Q);
		return d > (r - B) && d < A;
	}

	// If "knee" position Q needs to be as close as possible to some point D,
	// then choose M such that M(D) is in the y>0 half of the z=0 plane.
	//
	// Given that constraint, define the forward and inverse of M as follows:

	void defineM(float const P[], float const D[]) {
		float *X = Minv[0], *Y = Minv[1], *Z = Minv[2];

		// Minv defines a coordinate system whose x axis contains P, so X = unit(P).
		int i;
		for (i = 0; i < 3; i++)
			X[i] = P[i];
		normalize(X);

		// Its y axis is perpendicular to P, so Y = unit( E - X(E·X) ).

		float dDOTx = dot(D, X);
		for (i = 0; i < 3; i++)
			Y[i] = D[i] - dDOTx * X[i];
		normalize(Y);

		// Its z axis is perpendicular to both X and Y, so Z = X×Y.

		cross(X, Y, Z);

		// Mfwd = (Minv)T, since transposing inverts a rotation matrix.

		for (i = 0; i < 3; i++) {
			Mfwd[i][0] = Minv[0][i];
			Mfwd[i][1] = Minv[1][i];
			Mfwd[i][2] = Minv[2][i];
		}
	}

	//------------ GENERAL VECTOR MATH SUPPORT -----------

	static float dot(float const a[], float const b[]) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

	static float length(float const v[]) { return sqrt(dot(v, v)); }

	static void normalize(float v[]) {
		float norm = length(v);
		for (int i = 0; i < 3; i++)
			v[i] /= norm;
	}

	static void cross(float const a[], float const b[], float c[]) {
		c[0] = a[1] * b[2] - a[2] * b[1];
		c[1] = a[2] * b[0] - a[0] * b[2];
		c[2] = a[0] * b[1] - a[1] * b[0];
	}

	static void rot(float const M[3][3], float const src[], float dst[]) {
		for (int i = 0; i < 3; i++)
			dst[i] = dot(M[i], src);
	}
};



//-----------------------------------------------------------------------------
// Purpose: visual debugging code
//-----------------------------------------------------------------------------
#if 1
inline void debugLine(const Vector& origin, const Vector& dest, int r, int g, int b, bool noDepthTest, float duration) { };
#else
extern void drawLine(const Vector &p1, const Vector &p2, int r = 0, int g = 0, int b = 1, bool noDepthTest = true, float duration = 0.1);
void debugLine(const Vector& origin, const Vector& dest, int r, int g, int b, bool noDepthTest, float duration)
{
	drawLine(origin, dest, r, g, b, noDepthTest, duration);
}
#endif


//-----------------------------------------------------------------------------
// Purpose: for a 2 bone chain, find the IK solution and reset the matrices
//-----------------------------------------------------------------------------
bool Studio_SolveIK(mstudioikchain_t *pikchain, Vector &targetFoot, matrix3x4_t *pBoneToWorld)
{
	if (pikchain->pLink(0)->kneeDir.LengthSqr() > 0.0)
	{
		Vector targetKneeDir, targetKneePos;
		// FIXME: knee length should be as long as the legs
		Vector tmp = pikchain->pLink(0)->kneeDir;
		VectorRotate(tmp, pBoneToWorld[pikchain->pLink(0)->bone], targetKneeDir);
		MatrixPosition(pBoneToWorld[pikchain->pLink(1)->bone], targetKneePos);
		return Studio_SolveIK(pikchain->pLink(0)->bone, pikchain->pLink(1)->bone, pikchain->pLink(2)->bone, targetFoot, targetKneePos, targetKneeDir, pBoneToWorld);
	}
	else
	{
		return Studio_SolveIK(pikchain->pLink(0)->bone, pikchain->pLink(1)->bone, pikchain->pLink(2)->bone, targetFoot, pBoneToWorld);
	}
}


#define KNEEMAX_EPSILON 0.9998 // (0.9998 is about 1 degree)

//-----------------------------------------------------------------------------
// Purpose: Solve Knee position for a known hip and foot location, but no specific knee direction preference
//-----------------------------------------------------------------------------

bool Studio_SolveIK(int iThigh, int iKnee, int iFoot, Vector &targetFoot, matrix3x4_t *pBoneToWorld)
{
	Vector worldFoot, worldKnee, worldThigh;

	MatrixPosition(pBoneToWorld[iThigh], worldThigh);
	MatrixPosition(pBoneToWorld[iKnee], worldKnee);
	MatrixPosition(pBoneToWorld[iFoot], worldFoot);

	//debugLine( worldThigh, worldKnee, 0, 0, 255, true, 0 );
	//debugLine( worldKnee, worldFoot, 0, 0, 255, true, 0 );

	Vector ikFoot, ikKnee;

	ikFoot = targetFoot - worldThigh;
	ikKnee = worldKnee - worldThigh;

	float l1 = (worldKnee - worldThigh).Length();
	float l2 = (worldFoot - worldKnee).Length();
	float l3 = (worldFoot - worldThigh).Length();

	// leg too straight to figure out knee?
	if (l3 > (l1 + l2) * KNEEMAX_EPSILON)
	{
		return false;
	}

	Vector ikHalf = (worldFoot - worldThigh) * (l1 / l3);

	// FIXME: what to do when the knee completely straight?
	Vector ikKneeDir = ikKnee - ikHalf;
	VectorNormalize(ikKneeDir);

	return Studio_SolveIK(iThigh, iKnee, iFoot, targetFoot, worldKnee, ikKneeDir, pBoneToWorld);
}

//-----------------------------------------------------------------------------
// Purpose: Realign the matrix so that its X axis points along the desired axis.
//-----------------------------------------------------------------------------
void Studio_AlignIKMatrix(matrix3x4_t &mMat, const Vector &vAlignTo)
{
	Vector tmp1, tmp2, tmp3;

	// Column 0 (X) becomes the vector.
	tmp1 = vAlignTo;
	VectorNormalize(tmp1);
	MatrixSetColumn(tmp1, 0, mMat);

	// Column 1 (Y) is the cross of the vector and column 2 (Z).
	MatrixGetColumn(mMat, 2, tmp3);
	tmp2 = tmp3.Cross(tmp1);
	VectorNormalize(tmp2);
	// FIXME: check for X being too near to Z
	MatrixSetColumn(tmp2, 1, mMat);

	// Column 2 (Z) is the cross of columns 0 (X) and 1 (Y).
	tmp3 = tmp1.Cross(tmp2);
	MatrixSetColumn(tmp3, 2, mMat);
}


//-----------------------------------------------------------------------------
// Purpose: Solve Knee position for a known hip and foot location, and a known knee direction
//-----------------------------------------------------------------------------

bool Studio_SolveIK(int iThigh, int iKnee, int iFoot, Vector &targetFoot, Vector &targetKneePos, Vector &targetKneeDir, matrix3x4_t *pBoneToWorld)
{
	Vector worldFoot, worldKnee, worldThigh;

	MatrixPosition(pBoneToWorld[iThigh], worldThigh);
	MatrixPosition(pBoneToWorld[iKnee], worldKnee);
	MatrixPosition(pBoneToWorld[iFoot], worldFoot);

	//debugLine( worldThigh, worldKnee, 0, 0, 255, true, 0 );
	//debugLine( worldThigh, worldThigh + targetKneeDir, 0, 0, 255, true, 0 );
	// debugLine( worldKnee, targetKnee, 0, 0, 255, true, 0 );

	Vector ikFoot, ikTargetKnee, ikKnee;

	ikFoot = targetFoot - worldThigh;
	ikKnee = targetKneePos - worldThigh;

	float l1 = (worldKnee - worldThigh).Length();
	float l2 = (worldFoot - worldKnee).Length();

	// exaggerate knee targets for legs that are nearly straight
	// FIXME: should be configurable, and the ikKnee should be from the original animation, not modifed
	float d = (targetFoot - worldThigh).Length() - min(l1, l2);
	d = max(l1 + l2, d);
	// FIXME: too short knee directions cause trouble
	d = d * 100;

	ikTargetKnee = ikKnee + targetKneeDir * d;

	// debugLine( worldKnee, worldThigh + ikTargetKnee, 0, 0, 255, true, 0 );

	int color[3] = { 0, 255, 0 };

	// too far away? (0.9998 is about 1 degree)
	if (ikFoot.Length() > (l1 + l2) * KNEEMAX_EPSILON)
	{
		VectorNormalize(ikFoot);
		VectorScale(ikFoot, (l1 + l2) * KNEEMAX_EPSILON, ikFoot);
		color[0] = 255; color[1] = 0; color[2] = 0;
	}

	// too close?
	// limit distance to about an 80 degree knee bend
	float minDist = max(fabs(l1 - l2) * 1.15, min(l1, l2) * 0.15);
	if (ikFoot.Length() < minDist)
	{
		// too close to get an accurate vector, just use original vector
		ikFoot = (worldFoot - worldThigh);
		VectorNormalize(ikFoot);
		VectorScale(ikFoot, minDist, ikFoot);
	}

	CIKSolver ik;
	if (ik.solve(l1, l2, ikFoot.Base(), ikTargetKnee.Base(), ikKnee.Base()))
	{
		matrix3x4_t& mWorldThigh = pBoneToWorld[iThigh];
		matrix3x4_t& mWorldKnee = pBoneToWorld[iKnee];
		matrix3x4_t& mWorldFoot = pBoneToWorld[iFoot];

		//debugLine( worldThigh, ikKnee + worldThigh, 255, 0, 0, true, 0 );
		//debugLine( ikKnee + worldThigh, ikFoot + worldThigh, 255, 0, 0, true,0 );

		// debugLine( worldThigh, ikKnee + worldThigh, color[0], color[1], color[2], true, 0 );
		// debugLine( ikKnee + worldThigh, ikFoot + worldThigh, color[0], color[1], color[2], true,0 );


		// build transformation matrix for thigh
		Studio_AlignIKMatrix(mWorldThigh, ikKnee);
		Studio_AlignIKMatrix(mWorldKnee, ikFoot - ikKnee);


		mWorldKnee[0][3] = ikKnee.x + worldThigh.x;
		mWorldKnee[1][3] = ikKnee.y + worldThigh.y;
		mWorldKnee[2][3] = ikKnee.z + worldThigh.z;

		mWorldFoot[0][3] = ikFoot.x + worldThigh.x;
		mWorldFoot[1][3] = ikFoot.y + worldThigh.y;
		mWorldFoot[2][3] = ikFoot.z + worldThigh.z;

		return true;
	}
	else
	{
		/*
		debugLine( worldThigh, worldThigh + ikKnee, 255, 0, 0, true, 0 );
		debugLine( worldThigh + ikKnee, worldThigh + ikFoot, 255, 0, 0, true, 0 );
		debugLine( worldThigh + ikFoot, worldThigh, 255, 0, 0, true, 0 );
		debugLine( worldThigh + ikKnee, worldThigh + ikTargetKnee, 255, 0, 0, true, 0 );
		*/
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------


CIKContext::CIKContext()
{
	m_target.EnsureCapacity(12); // FIXME: this sucks, shouldn't it be grown?
	m_iFramecounter = -1;
	m_pStudioHdr = NULL;
	m_flTime = -1.0f;
	m_target.SetSize(0);
}


void CIKContext::Init(const CStudioHdr *pStudioHdr, const QAngle &angles, const Vector &pos, float flTime, int iFramecounter, int boneMask)
{
	m_pStudioHdr = pStudioHdr;
	m_ikChainRule.RemoveAll(); // m_numikrules = 0;
	if (pStudioHdr->numikchains())
	{
		m_ikChainRule.SetSize(pStudioHdr->numikchains());

		// FIXME: Brutal hackery to prevent a crash
		if (m_target.Count() == 0)
		{
			m_target.SetSize(12);
			memset(m_target.Base(), 0, sizeof(m_target[0])*m_target.Count());
			ClearTargets();
		}

	}
	else
	{
		m_target.SetSize(0);
	}
	AngleMatrix(angles, pos, m_rootxform);
	m_iFramecounter = iFramecounter;
	m_flTime = flTime;
	m_boneMask = boneMask;
}

//-----------------------------------------------------------------------------
// Purpose: build boneToWorld transforms for a specific bone
//-----------------------------------------------------------------------------
void CIKContext::BuildBoneChain(
	const Vector pos[],
	const Quaternion q[],
	int	iBone,
	matrix3x4_t *pBoneToWorld,
	CBoneBitList &boneComputed)
{
	//Assert(m_pStudioHdr->boneFlags(iBone) & m_boneMask);
	::BuildBoneChain(m_pStudioHdr, m_rootxform, pos, q, iBone, pBoneToWorld, boneComputed);
}



//-----------------------------------------------------------------------------
// Purpose: build boneToWorld transforms for a specific bone
//-----------------------------------------------------------------------------
void BuildBoneChain(
	const CStudioHdr *pStudioHdr,
	const matrix3x4_t &rootxform,
	const Vector pos[],
	const Quaternion q[],
	int	iBone,
	matrix3x4_t *pBoneToWorld,
	CBoneBitList &boneComputed)
{
	if (boneComputed.IsBoneMarked(iBone))
		return;

	matrix3x4_t bonematrix;
	QuaternionMatrix(q[iBone], pos[iBone], bonematrix);

	int parent = pStudioHdr->boneParent(iBone);
	if (parent == -1)
	{
		ConcatTransforms(rootxform, bonematrix, pBoneToWorld[iBone]);
	}
	else
	{
		// evil recursive!!!
		BuildBoneChain(pStudioHdr, rootxform, pos, q, parent, pBoneToWorld, boneComputed);
		ConcatTransforms(pBoneToWorld[parent], bonematrix, pBoneToWorld[iBone]);
	}
	boneComputed.MarkBone(iBone);
}


//-----------------------------------------------------------------------------
// Purpose: turn a specific bones boneToWorld transform into a pos and q in parents bonespace
//-----------------------------------------------------------------------------
void SolveBone(
	const CStudioHdr *pStudioHdr,
	int	iBone,
	matrix3x4_t *pBoneToWorld,
	Vector pos[],
	Quaternion q[]
)
{
	int iParent = pStudioHdr->boneParent(iBone);

	matrix3x4_t worldToBone;
	MatrixInvert(pBoneToWorld[iParent], worldToBone);

	matrix3x4_t local;
	ConcatTransforms(worldToBone, pBoneToWorld[iBone], local);

	MatrixAngles(local, q[iBone], pos[iBone]);
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void CIKTarget::SetOwner(int entindex, const Vector &pos, const QAngle &angles)
{
	latched.owner = entindex;
	latched.absOrigin = pos;
	latched.absAngles = angles;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void CIKTarget::ClearOwner(void)
{
	latched.owner = -1;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int CIKTarget::GetOwner(void)
{
	return latched.owner;
}

//-----------------------------------------------------------------------------
// Purpose: update the latched IK values that are in a moving frame of reference
//-----------------------------------------------------------------------------

#if 0
void CIKTarget::UpdateOwner(int entindex, const Vector &pos, const QAngle &angles)
{
	if (pos == latched.absOrigin && angles == latched.absAngles)
		return;

	matrix3x4_t in, out;
	AngleMatrix(angles, pos, in);
	AngleIMatrix(latched.absAngles, latched.absOrigin, out);

	matrix3x4_t tmp1, tmp2;
	QuaternionMatrix(latched.q, latched.pos, tmp1);
	ConcatTransforms(out, tmp1, tmp2);
	ConcatTransforms(in, tmp2, tmp1);
	MatrixAngles(tmp1, latched.q, latched.pos);
}
#endif


//-----------------------------------------------------------------------------
// Purpose: sets the ground position of an ik target
//-----------------------------------------------------------------------------

void CIKTarget::SetPos(const Vector &pos)
{
	est.pos = pos;
}

//-----------------------------------------------------------------------------
// Purpose: sets the ground "identity" orientation of an ik target
//-----------------------------------------------------------------------------

void CIKTarget::SetAngles(const QAngle &angles)
{
	AngleQuaternion(angles, est.q);
}

//-----------------------------------------------------------------------------
// Purpose: sets the ground "identity" orientation of an ik target
//-----------------------------------------------------------------------------

void CIKTarget::SetQuaternion(const Quaternion &q)
{
	est.q = q;
}

//-----------------------------------------------------------------------------
// Purpose: calculates a ground "identity" orientation based on the surface
//			normal of the ground and the desired ground identity orientation
//-----------------------------------------------------------------------------

void CIKTarget::SetNormal(const Vector &normal)
{
	// recalculate foot angle based on slope of surface
	matrix3x4_t m1;
	Vector forward, right;
	QuaternionMatrix(est.q, m1);

	MatrixGetColumn(m1, 1, right);
	forward = CrossProduct(right, normal);
	right = CrossProduct(normal, forward);
	MatrixSetColumn(forward, 0, m1);
	MatrixSetColumn(right, 1, m1);
	MatrixSetColumn(normal, 2, m1);
	QAngle a1;
	Vector p1;
	MatrixAngles(m1, est.q, p1);
}


//-----------------------------------------------------------------------------
// Purpose: estimates the ground impact at the center location assuming a the edge of 
//			an Z axis aligned disc collided with it the surface.
//-----------------------------------------------------------------------------

void CIKTarget::SetPosWithNormalOffset(const Vector &pos, const Vector &normal)
{
	// assume it's a disc edge intersecting with the floor, so try to estimate the z location of the center
	est.pos = pos;
	if (normal.z > 0.9999)
	{
		return;
	}
	// clamp at 45 degrees
	else if (normal.z > 0.707)
	{
		// tan == sin / cos
		float tan = sqrt(1 - normal.z * normal.z) / normal.z;
		est.pos.z = est.pos.z - est.radius * tan;
	}
	else
	{
		est.pos.z = est.pos.z - est.radius;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void CIKTarget::SetOnWorld(bool bOnWorld)
{
	est.onWorld = bOnWorld;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

bool CIKTarget::IsActive()
{
	return (est.flWeight > 0.0f);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void CIKTarget::IKFailed(void)
{
	latched.deltaPos.Init();
	latched.deltaQ.Init();
	latched.pos = ideal.pos;
	latched.q = ideal.q;
	est.latched = 0.0;
	est.flWeight = 0.0;
	est.onWorld = false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void CIKTarget::MoveReferenceFrame(Vector &deltaPos, QAngle &deltaAngles)
{
	est.pos -= deltaPos;
	latched.pos -= deltaPos;
	offset.pos -= deltaPos;
	ideal.pos -= deltaPos;
}



//-----------------------------------------------------------------------------
// Purpose: Invalidate any IK locks.
//-----------------------------------------------------------------------------

void CIKContext::ClearTargets(void)
{
	int i;
	for (i = 0; i < m_target.Count(); i++)
	{
		m_target[i].latched.iFramecounter = -9999;
	}
}

#if 0
//-----------------------------------------------------------------------------
// Purpose: run all animations that automatically play and are driven off of poseParameters
//-----------------------------------------------------------------------------
void CBoneSetup::CalcAutoplaySequences(
	Vector pos[],
	Quaternion q[],
	float flRealTime,
	CIKContext *pIKContext
)
{
	//	ASSERT_NO_REENTRY();

	int			i;
	if (pIKContext)
	{
		pIKContext->AddAutoplayLocks(pos, q);
	}

	unsigned short *pList = NULL;
	int count = m_pStudioHdr->GetAutoplayList(&pList);
	for (i = 0; i < count; i++)
	{
		int sequenceIndex = pList[i];
		mstudioseqdesc_t &seqdesc = ((CStudioHdr *)m_pStudioHdr)->pSeqdesc(sequenceIndex);
		if (seqdesc.flags & STUDIO_AUTOPLAY)
		{
			float cycle = 0;
			float cps = Studio_CPS(m_pStudioHdr, seqdesc, sequenceIndex, m_flPoseParameter);
			cycle = flRealTime * cps;
			cycle = cycle - (int)cycle;

			AccumulatePose(pos, q, sequenceIndex, cycle, 1.0, flRealTime, pIKContext);
		}
	}

	if (pIKContext)
	{
		pIKContext->SolveAutoplayLocks(pos, q);
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void Studio_BuildMatrices(
	const CStudioHdr *pStudioHdr,
	const QAngle& angles,
	const Vector& origin,
	const Vector pos[],
	const Quaternion q[],
	int iBone,
	float flScale,
	matrix3x4_t bonetoworld[MAXSTUDIOBONES],
	int boneMask
)
{
	int i, j;

	int					chain[MAXSTUDIOBONES] = {};
	int					chainlength = 0;

	if (iBone < -1 || iBone >= pStudioHdr->numbones())
		iBone = 0;

	// build list of what bones to use
	if (iBone == -1)
	{
		// all bones
		chainlength = pStudioHdr->numbones();
		for (i = 0; i < pStudioHdr->numbones(); i++)
		{
			chain[chainlength - i - 1] = i;
		}
	}
	else
	{
		// only the parent bones
		i = iBone;
		while (i != -1)
		{
			chain[chainlength++] = i;
			i = pStudioHdr->boneParent(i);
		}
	}

	matrix3x4_t bonematrix;
	matrix3x4_t rotationmatrix; // model to world transformation
	AngleMatrix(angles, origin, rotationmatrix);

	// Account for a change in scale
	if (flScale < 1.0f - FLT_EPSILON || flScale > 1.0f + FLT_EPSILON)
	{
		Vector vecOffset;
		MatrixGetColumn(rotationmatrix, 3, vecOffset);
		vecOffset -= origin;
		vecOffset *= flScale;
		vecOffset += origin;
		MatrixSetColumn(vecOffset, 3, rotationmatrix);

		// Scale it uniformly
		VectorScale(rotationmatrix[0], flScale, rotationmatrix[0]);
		VectorScale(rotationmatrix[1], flScale, rotationmatrix[1]);
		VectorScale(rotationmatrix[2], flScale, rotationmatrix[2]);
	}

	for (j = chainlength - 1; j >= 0; j--)
	{
		i = chain[j];
		if (pStudioHdr->boneFlags(i) & boneMask)
		{
			QuaternionMatrix(q[i], pos[i], bonematrix);

			if (pStudioHdr->boneParent(i) == -1)
			{
				ConcatTransforms(rotationmatrix, bonematrix, bonetoworld[i]);
			}
			else
			{
				ConcatTransforms(bonetoworld[pStudioHdr->boneParent(i)], bonematrix, bonetoworld[i]);
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:  Lookup a bone controller
//-----------------------------------------------------------------------------


#if 0
static mstudiobonecontroller_t* FindController(const CStudioHdr *pStudioHdr, int iController)
{
	// find first controller that matches the index
	for (int i = 0; i < pStudioHdr->numbonecontrollers(); i++)
	{
		if (pStudioHdr->pBonecontroller(i)->inputfield == iController)
			return pStudioHdr->pBonecontroller(i);
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: converts a ranged bone controller value into a 0..1 encoded value
// Output: 	ctlValue contains 0..1 encoding.
//			returns clamped ranged value
//-----------------------------------------------------------------------------

float Studio_SetController(const CStudioHdr *pStudioHdr, int iController, float flValue, float &ctlValue)
{
	if (!pStudioHdr)
		return flValue;

	mstudiobonecontroller_t *pbonecontroller = FindController(pStudioHdr, iController);
	if (!pbonecontroller)
	{
		ctlValue = 0;
		return flValue;
	}

	// wrap 0..360 if it's a rotational controller
	if (pbonecontroller->type & (STUDIO_XR | STUDIO_YR | STUDIO_ZR))
	{
		// ugly hack, invert value if end < start
		if (pbonecontroller->end < pbonecontroller->start)
			flValue = -flValue;

		// does the controller not wrap?
		if (pbonecontroller->start + 359.0 >= pbonecontroller->end)
		{
			if (flValue > ((pbonecontroller->start + pbonecontroller->end) / 2.0) + 180)
				flValue = flValue - 360;
			if (flValue < ((pbonecontroller->start + pbonecontroller->end) / 2.0) - 180)
				flValue = flValue + 360;
		}
		else
		{
			if (flValue > 360)
				flValue = flValue - (int)(flValue / 360.0) * 360.0;
			else if (flValue < 0)
				flValue = flValue + (int)((flValue / -360.0) + 1) * 360.0;
		}
	}

	ctlValue = (flValue - pbonecontroller->start) / (pbonecontroller->end - pbonecontroller->start);
	if (ctlValue < 0) ctlValue = 0;
	if (ctlValue > 1) ctlValue = 1;

	float flReturnVal = ((1.0 - ctlValue)*pbonecontroller->start + ctlValue * pbonecontroller->end);

	// ugly hack, invert value if a rotational controller and end < start
	if (pbonecontroller->type & (STUDIO_XR | STUDIO_YR | STUDIO_ZR) &&
		pbonecontroller->end < pbonecontroller->start)
	{
		flReturnVal *= -1;
	}

	return flReturnVal;
}


//-----------------------------------------------------------------------------
// Purpose: converts a 0..1 encoded bone controller value into a ranged value
// Output: 	returns ranged value
//-----------------------------------------------------------------------------

float Studio_GetController(const CStudioHdr *pStudioHdr, int iController, float ctlValue)
{
	if (!pStudioHdr)
		return 0.0;

	mstudiobonecontroller_t *pbonecontroller = FindController(pStudioHdr, iController);
	if (!pbonecontroller)
		return 0;

	return ctlValue * (pbonecontroller->end - pbonecontroller->start) + pbonecontroller->start;
}
#endif

#if 0
//-----------------------------------------------------------------------------
// Purpose: Calculates default values for the pose parameters
// Output: 	fills in an array
//-----------------------------------------------------------------------------

void Studio_CalcDefaultPoseParameters(const CStudioHdr *pStudioHdr, float flPoseParameter[], int nCount)
{
	int nPoseCount = pStudioHdr->GetNumPoseParameters();
	int nNumParams = min(nCount, MAXSTUDIOPOSEPARAM);

	for (int i = 0; i < nNumParams; ++i)
	{
		// Default to middle of the pose parameter range
		flPoseParameter[i] = 0.5f;
		if (i < nPoseCount)
		{
			const mstudioposeparamdesc_t &Pose = ((CStudioHdr *)pStudioHdr)->pPoseParameter(i);

			// Want to try for a zero state.  If one doesn't exist set it to .5 by default.
			if (Pose.start < 0.0f && Pose.end > 0.0f)
			{
				float flPoseDelta = Pose.end - Pose.start;
				flPoseParameter[i] = -Pose.start / flPoseDelta;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: converts a ranged pose parameter value into a 0..1 encoded value
// Output: 	ctlValue contains 0..1 encoding.
//			returns clamped ranged value
//-----------------------------------------------------------------------------

float Studio_SetPoseParameter(const CStudioHdr *pStudioHdr, int iParameter, float flValue, float &ctlValue)
{
	if (iParameter < 0 || iParameter >= pStudioHdr->GetNumPoseParameters())
	{
		return 0;
	}

	const mstudioposeparamdesc_t &PoseParam = ((CStudioHdr *)pStudioHdr)->pPoseParameter(iParameter);

	//Assert(IsFinite(flValue));

	if (PoseParam.loop)
	{
		float wrap = (PoseParam.start + PoseParam.end) / 2.0 + PoseParam.loop / 2.0;
		float shift = PoseParam.loop - wrap;

		flValue = flValue - PoseParam.loop * floor((flValue + shift) / PoseParam.loop);
	}

	ctlValue = (flValue - PoseParam.start) / (PoseParam.end - PoseParam.start);

	if (ctlValue < 0) ctlValue = 0;
	if (ctlValue > 1) ctlValue = 1;

	//Assert(IsFinite(ctlValue));

	return ctlValue * (PoseParam.end - PoseParam.start) + PoseParam.start;
}

#endif
//-----------------------------------------------------------------------------
// Purpose: converts a 0..1 encoded pose parameter value into a ranged value
// Output: 	returns ranged value
//-----------------------------------------------------------------------------

float Studio_GetPoseParameter(const CStudioHdr *pStudioHdr, int iParameter, float ctlValue)
{
	if (iParameter < 0 || iParameter >= pStudioHdr->GetNumPoseParameters())
	{
		return 0;
	}

	const mstudioposeparamdesc_t &PoseParam = ((CStudioHdr *)pStudioHdr)->pPoseParameter(iParameter);

	return ctlValue * (PoseParam.end - PoseParam.start) + PoseParam.start;
}


#pragma warning (disable : 4701)

#if 0
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
static int ClipRayToHitbox(const Ray_t &ray, mstudiobbox_t *pbox, matrix3x4_t& matrix, trace_t &tr)
{
	const float flProjEpsilon = 0.01f;
	// scale by current t so hits shorten the ray and increase the likelihood of early outs
	Vector delta2;
	VectorScale(ray.m_Delta, (0.5f * tr.fraction), delta2);

	// OPTIMIZE: Store this in the box instead of computing it here
	// compute center in local space
	Vector boxextents;
	boxextents.x = (pbox->bbmin.x + pbox->bbmax.x) * 0.5;
	boxextents.y = (pbox->bbmin.y + pbox->bbmax.y) * 0.5;
	boxextents.z = (pbox->bbmin.z + pbox->bbmax.z) * 0.5;
	Vector boxCenter;
	// transform to world space
	VectorTransform(boxextents, matrix, boxCenter);
	// calc extents from local center
	boxextents.x = pbox->bbmax.x - boxextents.x;
	boxextents.y = pbox->bbmax.y - boxextents.y;
	boxextents.z = pbox->bbmax.z - boxextents.z;
	// OPTIMIZE: This is optimized for world space.  If the transform is fast enough, it may make more
	// sense to just xform and call UTIL_ClipToBox() instead.  MEASURE THIS.

	// save the extents of the ray along 
	Vector extent, uextent;
	Vector segmentCenter;
	segmentCenter.x = ray.m_Start.x + delta2.x - boxCenter.x;
	segmentCenter.y = ray.m_Start.y + delta2.y - boxCenter.y;
	segmentCenter.z = ray.m_Start.z + delta2.z - boxCenter.z;

	extent.Init();

	// check box axes for separation
	for (int j = 0; j < 3; j++)
	{
		extent[j] = delta2.x * matrix[0][j] + delta2.y * matrix[1][j] + delta2.z * matrix[2][j];
		uextent[j] = fabsf(extent[j]);
		float coord = segmentCenter.x * matrix[0][j] + segmentCenter.y * matrix[1][j] + segmentCenter.z * matrix[2][j];
		coord = fabsf(coord);

		if (coord > (boxextents[j] + uextent[j]))
			return -1;
	}

	// now check cross axes for separation
	float tmp, tmpfix, cextent;
	Vector cross;
	CrossProduct(delta2, segmentCenter, cross);
	cextent = cross.x * matrix[0][0] + cross.y * matrix[1][0] + cross.z * matrix[2][0];
	cextent = fabsf(cextent);
	tmp = boxextents[1] * uextent[2] + boxextents[2] * uextent[1];
	tmpfix = max(tmp, flProjEpsilon);
	if (cextent > tmpfix)
		return -1;

	//	if ( cextent > tmp && cextent <= tmpfix )
	//		DevWarning( "ClipRayToHitbox trace precision error case\n" );

	cextent = cross.x * matrix[0][1] + cross.y * matrix[1][1] + cross.z * matrix[2][1];
	cextent = fabsf(cextent);
	tmp = boxextents[0] * uextent[2] + boxextents[2] * uextent[0];
	tmpfix = max(tmp, flProjEpsilon);
	if (cextent > tmpfix)
		return -1;

	//	if ( cextent > tmp && cextent <= tmpfix )
	//		DevWarning( "ClipRayToHitbox trace precision error case\n" );

	cextent = cross.x * matrix[0][2] + cross.y * matrix[1][2] + cross.z * matrix[2][2];
	cextent = fabsf(cextent);
	tmp = boxextents[0] * uextent[1] + boxextents[1] * uextent[0];
	tmpfix = max(tmp, flProjEpsilon);
	if (cextent > tmpfix)
		return -1;

	//	if ( cextent > tmp && cextent <= tmpfix )
	//		DevWarning( "ClipRayToHitbox trace precision error case\n" );

		// !!! We hit this box !!! compute intersection point and return
	Vector start;

	// Compute ray start in bone space
	VectorITransform(ray.m_Start, matrix, start);
	// extent is delta2 in bone space, recompute delta in bone space
	VectorScale(extent, 2, extent);

	// delta was prescaled by the current t, so no need to see if this intersection
	// is closer
	trace_t boxTrace;
	if (!IntersectRayWithBox(start, extent, pbox->bbmin, pbox->bbmax, 0.0f, &boxTrace))
		return -1;

	//Assert(IsFinite(boxTrace.fraction));
	tr.fraction *= boxTrace.fraction;
	tr.startsolid = boxTrace.startsolid;
	int hitside = boxTrace.plane.type;
	if (boxTrace.plane.normal[hitside] >= 0)
	{
		hitside += 3;
	}
	return hitside;
}
#endif

#pragma warning (default : 4701)


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool SweepBoxToStudio(IPhysicsSurfaceProps *pProps, const Ray_t& ray, CStudioHdr *pStudioHdr, mstudiohitboxset_t *set,
	matrix3x4_t **hitboxbones, int fContentsMask, trace_t &tr)
{
	tr.fraction = 1.0;
	tr.startsolid = false;

	// OPTIMIZE: Partition these?
	Ray_t clippedRay = ray;
	int hitbox = -1;
	for (int i = 0; i < set->numhitboxes; i++)
	{
		mstudiobbox_t *pbox = set->pHitbox(i);

		// Filter based on contents mask
		int fBoneContents = pStudioHdr->pBone(pbox->bone)->contents;
		if ((fBoneContents & fContentsMask) == 0)
			continue;

		//FIXME: Won't work with scaling!
		trace_t obbTrace;
		if (IntersectRayWithOBB(clippedRay, *hitboxbones[pbox->bone], pbox->bbmin, pbox->bbmax, 0.0f, &obbTrace))
		{
			tr.startpos = obbTrace.startpos;
			tr.endpos = obbTrace.endpos;
			tr.plane = obbTrace.plane;
			tr.startsolid = obbTrace.startsolid;
			tr.allsolid = obbTrace.allsolid;

			// This logic here is to shorten the ray each time to get more early outs
			tr.fraction *= obbTrace.fraction;
			clippedRay.m_Delta *= obbTrace.fraction;
			hitbox = i;
			if (tr.startsolid)
				break;
		}
	}

	if (hitbox >= 0)
	{
		tr.hitgroup = set->pHitbox(hitbox)->group;
		tr.hitbox = hitbox;
		const mstudiobone_t *pBone = pStudioHdr->pBone(set->pHitbox(hitbox)->bone);
		tr.contents = pBone->contents | CONTENTS_HITBOX;
		tr.physicsbone = pBone->physicsbone;
		tr.surface.name = "**studio**";
		tr.surface.flags = SURF_HITBOX;
		tr.surface.surfaceProps = pProps->GetSurfaceIndex(pBone->pszSurfaceProp());

		//Assert(tr.physicsbone >= 0);
		return true;
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: lookup bone by name
//-----------------------------------------------------------------------------

int Studio_BoneIndexByName(const CStudioHdr *pStudioHdr, const char *pName)
{
	if (pStudioHdr)
	{
		// binary search for the bone matching pName
		int start = 0, end = pStudioHdr->numbones() - 1;
		const byte *pBoneTable = pStudioHdr->GetBoneTableSortedByName();
		mstudiobone_t *pbones = pStudioHdr->pBone(0);
		while (start <= end)
		{
			int mid = (start + end) >> 1;
			int cmp = stricmp(pbones[pBoneTable[mid]].pszName(), pName);

			if (cmp < 0)
			{
				start = mid + 1;
			}
			else if (cmp > 0)
			{
				end = mid - 1;
			}
			else
			{
				return pBoneTable[mid];
			}
		}
	}

	return -1;
}