#include "precompiled.h"
#include "math.h"
#include <math.h>

//For extrapolation
#include "GameMemory.h"
#include "Interfaces.h"

void inline ExtrapolateOrigin(Vector* origin, Vector& targetorigin, Vector* velocity, float extraptime)
{
	if (*velocity != vecZero)
	{
		//compensate because entities are received half way through the frame loop on average
		//extrapolate for 1 tick into the future
		float ExtrapolateTime = extraptime;// +(ReadFloat((uintptr_t)&Interfaces::Globals->frametime) * 0.5f) + (ReadFloat((uintptr_t)&Interfaces::Globals->interval_per_tick));
		targetorigin.x = origin->x + velocity->x * ExtrapolateTime;
		targetorigin.y = origin->y + velocity->y * ExtrapolateTime;
		targetorigin.z = origin->z + velocity->z * ExtrapolateTime;// +9.81f * ExtrapolateTime * ExtrapolateTime / 2;
	}
	else
	{
		targetorigin = *origin;
	}
}

#if 0
void inline SinCos(float radians, float *sine, float *cosine)
{
	*sine = sinf(radians);
	*cosine = cosf(radians);
}
#endif

void VectorAngles(const Vector& forward, QAngle &angles)
{
	float yaw, pitch;

	if (forward.y == 0 && forward.x == 0)
	{
		yaw = 0;
		pitch = float((forward.z > 0) ? 270 : 90);
	}
	else
	{
		yaw = RAD2DEG(atan2(forward.y, forward.x));

		if (yaw < 0.f) yaw += 360.f;

		pitch = RAD2DEG(atan2(-forward.z, forward.Length2D()));

		if (pitch < 0.f) pitch += 360.f;
	}

	angles.x = pitch;
	angles.y = yaw;
	angles.z = 0;
}

//-----------------------------------------------------------------------------
// Forward direction vector with a reference up vector -> Euler angles
//-----------------------------------------------------------------------------

void VectorAngles(const Vector &forward, const Vector &pseudoup, QAngle &angles)
{
	//Assert(s_bMathlibInitialized);

	Vector left;

	CrossProduct(pseudoup, forward, left);
	VectorNormalizeFast(left);

	float xyDist = sqrtf(forward[0] * forward[0] + forward[1] * forward[1]);

	// enough here to get angles?
	if (xyDist > 0.001f)
	{
		// (yaw)	y = ATAN( forward.y, forward.x );		-- in our space, forward is the X axis
		angles[1] = RAD2DEG(atan2f(forward[1], forward[0]));

		// The engine does pitch inverted from this, but we always end up negating it in the DLL
		// UNDONE: Fix the engine to make it consistent
		// (pitch)	x = ATAN( -forward.z, sqrt(forward.x*forward.x+forward.y*forward.y) );
		angles[0] = RAD2DEG(atan2f(-forward[2], xyDist));

		float up_z = (left[1] * forward[0]) - (left[0] * forward[1]);

		// (roll)	z = ATAN( left.z, up.z );
		angles[2] = RAD2DEG(atan2f(left[2], up_z));
	}
	else	// forward is mostly Z, gimbal lock-
	{
		// (yaw)	y = ATAN( -left.x, left.y );			-- forward is mostly z, so use right for yaw
		angles[1] = RAD2DEG(atan2f(-left[0], left[1])); //This was originally copied from the "void MatrixAngles( const matrix3x4_t& matrix, float *angles )" code, and it's 180 degrees off, negated the values and it all works now (Dave Kircher)

														// The engine does pitch inverted from this, but we always end up negating it in the DLL
														// UNDONE: Fix the engine to make it consistent
														// (pitch)	x = ATAN( -forward.z, sqrt(forward.x*forward.x+forward.y*forward.y) );
		angles[0] = RAD2DEG(atan2f(-forward[2], xyDist));

		// Assume no roll in this case as one degree of freedom has been lost (i.e. yaw == roll)
		angles[2] = 0;
	}
}

void VectorAngles3D(Vector& vecForward, QAngle& vecAngles)
{
	Vector vecView; ;
	if (vecForward[1] == 0.f && vecForward[0] == 0.f)
	{
		vecView[0] = 0.f; ;
		vecView[1] = 0.f; ;
	}
	else
	{
		vecView[1] = atan2(vecForward[1], vecForward[0]) * 180.f / M_PI; ;

		if (vecView[1] < 0.f)
			vecView[1] += 360; ;

		vecView[2] = sqrt(vecForward[0] * vecForward[0] + vecForward[1] * vecForward[1]); ;

		vecView[0] = atan2(vecForward[2], vecView[2]) * 180.f / M_PI; ;
	}

	vecAngles[0] = -vecView[0]; ;
	vecAngles[1] = vecView[1]; ;
	vecAngles[2] = 0.f; ;
}

void inline AngleVectors(const QAngle &angles, Vector *forward)
{
	float sp, sy, cp, cy;
	SinCos(DEG2RAD(angles.y), &sy, &cy);
	SinCos(DEG2RAD(angles.x), &sp, &cp);

	forward->x = cp*cy;
	forward->y = cp*sy;
	forward->z = -sp;
}

void AngleVectors(const QAngle &angles, Vector *forward, Vector *right, Vector *up)
{
	float sr, sp, sy, cr, cp, cy;

	SinCos(DEG2RAD(angles.x), &sp, &cp);
	SinCos(DEG2RAD(angles.y), &sy, &cy);
	SinCos(DEG2RAD(angles.z), &sr, &cr);

	if (forward)
	{
		forward->x = cp*cy;
		forward->y = cp*sy;
		forward->z = -sp;
	}

	if (right)
	{
		right->x = (-1 * sr*sp*cy + -1 * cr*-sy);
		right->y = (-1 * sr*sp*sy + -1 * cr*cy);
		right->z = -1 * sr*cp;
	}

	if (up)
	{
		up->x = (cr*sp*cy + -sr*-sy);
		up->y = (cr*sp*sy + -sr*cy);
		up->z = cr*cp;
	}
}

float GetFov(const QAngle& viewAngle, const QAngle& aimAngle)
{
	Vector ang, aim;

	AngleVectors(viewAngle, &aim);
	AngleVectors(aimAngle, &ang);

	return RAD2DEG(acos(aim.Dot(ang) / aim.LengthSqr()));
}

float GetFovRegardlessOfDistance(const QAngle& viewAngle, const QAngle& aimAngle, float Distance)
{
	Vector ang, aim;

	AngleVectors(viewAngle, &aim);
	AngleVectors(aimAngle, &ang);

	return sin(acos(aim.Dot(ang) / aim.LengthSqr())) * Distance;
}

float GetFov(Vector source, Vector destination, QAngle angles)
{
	Vector forward;
	QAngle ang = CalcAngle(source, destination);
	AngleVectors(ang, &forward);

	Vector forward2;
	AngleVectors(angles, &forward2);

	return (acosf(forward2.Dot(forward) / std::pow(forward2.Length(), 2.0f)) * (180.0f / M_PI));
}

QAngle CalcAngle(Vector& src, Vector& dst)
{
	QAngle angles;
	VectorAngles(dst - src, angles);

	return angles;
}

float CalcAngle(ImVec2& v1, ImVec2& v2)
{
	float radians = atan2(v1.y - v2.y, v1.x - v2.x);
	return RAD2DEG(radians);
}

float GetFovFromCoords(const Vector& from, const Vector& to, const QAngle& anglelooking)
{
	// Convert angles to normalized directional forward vector
	Vector Forward;
	AngleVectors(anglelooking, &Forward);

	// Get delta vector between our local eye position and passed vector
	Vector Delta = to - from;

	// Normalize our delta vector
	VectorNormalizeFast(Delta);

	// Get dot product between delta position and directional forward vectors
	float DotProduct = Forward.Dot(Delta);

	// Time to calculate the field of view
	return RAD2DEG(acos(DotProduct));
}

//FOV is scaled based off distance
float GetFovFromCoordsRegardlessofDistance(const Vector& from, const Vector& to, const QAngle& anglelooking)
{
	// Convert angles to normalized directional forward vector
	Vector Forward;
	AngleVectors(anglelooking, &Forward);

	// Get delta vector between our local eye position and passed vector
	Vector Delta = to - from;

	float Distance = Delta.Length();

	// Normalize our delta vector
	VectorNormalizeFast(Delta);

	// Get dot product between delta position and directional forward vectors
	float DotProduct = Forward.Dot(Delta);

	// Time to calculate the field of view
	return RAD2DEG(sin(DEG2RAD(acos(DotProduct))) * Distance);
}


float VectorDistance(Vector v1, Vector v2)
{
	float sqin = powf(v1.x - v2.x, 2) + powf(v1.y - v2.y, 2) + powf(v1.z - v2.z, 2);
	float sqout;
	SSESqrt(&sqout, &sqin);
	return sqout; //FASTSQRT(pow(v1.x - v2.x, 2) + pow(v1.y - v2.y, 2) + pow(v1.z - v2.z, 2));
}

void VectorTransform(Vector& in1, matrix3x4a_t& in2, Vector &out)
{
	out.x = in1.Dot(in2.m_flMatVal[0]) + in2.m_flMatVal[0][3];
	out.y = in1.Dot(in2.m_flMatVal[1]) + in2.m_flMatVal[1][3];
	out.z = in1.Dot(in2.m_flMatVal[2]) + in2.m_flMatVal[2][3];
}

void VectorTransform(Vector& in1, const matrix3x4a_t& in2, Vector &out)
{
	out.x = in1.Dot(in2.m_flMatVal[0]) + in2.m_flMatVal[0][3];
	out.y = in1.Dot(in2.m_flMatVal[1]) + in2.m_flMatVal[1][3];
	out.z = in1.Dot(in2.m_flMatVal[2]) + in2.m_flMatVal[2][3];
}

void VectorTransform(Vector& in1, matrix3x4_t& in2, Vector &out)
{
	out.x = in1.Dot(in2.m_flMatVal[0]) + in2.m_flMatVal[0][3];
	out.y = in1.Dot(in2.m_flMatVal[1]) + in2.m_flMatVal[1][3];
	out.z = in1.Dot(in2.m_flMatVal[2]) + in2.m_flMatVal[2][3];
}

void VectorTransform(Vector& in1, const matrix3x4_t& in2, Vector &out)
{
	out.x = in1.Dot(in2.m_flMatVal[0]) + in2.m_flMatVal[0][3];
	out.y = in1.Dot(in2.m_flMatVal[1]) + in2.m_flMatVal[1][3];
	out.z = in1.Dot(in2.m_flMatVal[2]) + in2.m_flMatVal[2][3];
}

Vector VectorTransformR(Vector& in1, matrix3x4_t* in2)
{
	Vector out;
	out.x = in1.Dot(in2->m_flMatVal[0]) + in2->m_flMatVal[0][3];
	out.y = in1.Dot(in2->m_flMatVal[1]) + in2->m_flMatVal[1][3];
	out.z = in1.Dot(in2->m_flMatVal[2]) + in2->m_flMatVal[2][3];
	return out;
}

float DotProductT(const float* a, const float* b)
{
	return (a[0] * b[0] + a[1] * b[1] + a[2] * b[2]);
}

void VectorTransformA(const float *in1, const matrix3x4_t& in2, float *out)
{

	out[0] = DotProductT(in1, in2[0]) + in2[0][3];
	out[1] = DotProductT(in1, in2[1]) + in2[1][3];
	out[2] = DotProductT(in1, in2[2]) + in2[2][3];
}


void inline VectorTransformZ(const Vector& in1, const matrix3x4_t &in2, Vector &out)
{
	VectorTransformA(&in1.x, in2, &out.x);
}

float GetDelta(float hspeed, float maxspeed, float airaccelerate)
{
	float term = (30.0f - (airaccelerate * maxspeed / 66.0f)) / hspeed;

	if (term < 1.0f && term > -1.0f) {
		return acosf(term);
	}

	return 0.f;
}

inline float RandFloat(float M, float N)
{
	return (float)(M + (rand() / (RAND_MAX / (N - M))));
}

Vertex_t RotateVertex(Vector2D & vec, Vector2D& vertexPos, const float angle)
{
    float c = cos(DEG2RAD(angle));
    float s = sin(DEG2RAD(angle));

    Vertex_t ret;
    ret.m_Position.x = vec.x + (vertexPos.x - vec.x) * c - (vertexPos.y - vec.y) * s;
    ret.m_Position.y = vec.y + (vertexPos.x - vec.x) * s + (vertexPos.y - vec.y) * c;

    return ret;
}

inline Vector ExtrapolateTick(Vector p0, Vector v0)
{
	return vecZero; //dylan fix
	//return p0 + (v0 * I::Globals->interval_per_tick);
}

// assume in2 is a rotation and rotate the input vector
void VectorRotate(const float *in1, const matrix3x4_t& in2, float *out)
{
	//Assert(s_bMathlibInitialized);
	//Assert(in1 != out);
	out[0] = DotProduct(in1, in2[0]);
	out[1] = DotProduct(in1, in2[1]);
	out[2] = DotProduct(in1, in2[2]);
}

// assume in2 is a rotation and rotate the input vector
void VectorRotate(const Vector &in1, const QAngle &in2, Vector &out)
{
	matrix3x4_t matRotate;
	AngleMatrix(in2, matRotate);
	VectorRotate(in1, matRotate, out);
}

// assume in2 is a rotation and rotate the input vector
Vector VectorRotateR(const Vector &in1, const QAngle &in2)
{
	Vector out;
	matrix3x4_t matRotate;
	AngleMatrix(in2, matRotate);
	VectorRotate(in1, matRotate, out);
	return out;
}

// assume in2 is a rotation and rotate the input vector
void VectorRotate(const Vector &in1, const Quaternion &in2, Vector &out)
{
	matrix3x4_t matRotate;
	QuaternionMatrix(in2, matRotate);
	VectorRotate(in1, matRotate, out);
}


// rotate by the inverse of the matrix
void VectorIRotate(const float *in1, const matrix3x4_t& in2, float *out)
{
	//Assert(s_bMathlibInitialized);
	//Assert(in1 != out);
	out[0] = in1[0] * in2[0][0] + in1[1] * in2[1][0] + in1[2] * in2[2][0];
	out[1] = in1[0] * in2[0][1] + in1[1] * in2[1][1] + in1[2] * in2[2][1];
	out[2] = in1[0] * in2[0][2] + in1[1] * in2[1][2] + in1[2] * in2[2][2];
}