#include "precompiled.h"
#include "RadianEuler.h"

#include "misc.h"

//-----------------------------------------------------------------------------
// Purpose: Converts radian-euler axis aligned angles to a quaternion
// Input  : *pfAngles - Right-handed Euler angles in radians
//			*outQuat - quaternion of form (i,j,k,real)
//-----------------------------------------------------------------------------
void AngleQuaternion(const RadianEuler &angles, Quaternion &outQuat)
{
	//	//Assert( angles.IsValid() );

	float sr, sp, sy, cr, cp, cy;

	SinCos(angles.z * 0.5f, &sy, &cy);
	SinCos(angles.y * 0.5f, &sp, &cp);
	SinCos(angles.x * 0.5f, &sr, &cr);

	// NJS: for some reason VC6 wasn't recognizing the common subexpressions:
	float srXcp = sr * cp, crXsp = cr * sp;
	outQuat.x = srXcp*cy - crXsp*sy; // X
	outQuat.y = crXsp*cy + srXcp*sy; // Y

	float crXcp = cr * cp, srXsp = sr * sp;
	outQuat.z = crXcp*sy - srXsp*cy; // Z
	outQuat.w = crXcp*cy + srXsp*sy; // W (real component)
}