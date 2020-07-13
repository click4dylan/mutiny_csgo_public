#pragma once
#include "misc.h"

#define square( x ) ( x * x )

//http://stackoverflow.com/questions/5289613/generate-random-float-between-two-floats
inline float SORandomFloat(float a, float b) {
	float random = ((float)rand()) / (float)RAND_MAX;
	float diff = b - a;
	float r = random * diff;
	return a + r;
}

extern inline void ExtrapolateOrigin(Vector* origin, Vector& targetorigin, Vector* velocity, float extraptime);
extern inline void SinCos(float radians, float *sine, float *cosine);
extern void VectorAngles(const Vector& forward, QAngle &angles);
extern void VectorAngles(const Vector& forward, const Vector& pseudoup, QAngle &angles);
extern void VectorAngles3D(Vector& vecForward, QAngle& vecAngles);
extern void AngleVectors(const QAngle& angles, Vector *forward);
extern void AngleVectors(const QAngle &angles, Vector *forward, Vector *right, Vector *up);
extern QAngle CalcAngle(Vector& v1, Vector& v2);
extern float CalcAngle(ImVec2& v1, ImVec2& v2);
extern float GetFov(Vector source, Vector destination, QAngle angles);
extern float GetFov(const QAngle& viewAngle, const QAngle& aimAngle);
extern float GetFovRegardlessOfDistance(const QAngle& viewAngle, const QAngle& aimAngle, float Distance);
extern float GetFovFromCoords(const Vector& from, const Vector& to, const QAngle& anglelooking);
extern float GetFovFromCoordsRegardlessofDistance(const Vector& from, const Vector& to, const QAngle& anglelooking);
extern float VectorDistance(Vector v1, Vector v2);
extern float DotProductT(const float* a, const float* b);
extern void VectorTransformA(const float *in1, const matrix3x4_t& in2, float *out);
extern void VectorTransformZ(const Vector& in1, const matrix3x4_t &in2, Vector &out);
extern Vector VectorTransformR(Vector& in1, matrix3x4_t* in2);
extern void VectorTransform(Vector& in1, matrix3x4a_t& in2, Vector &out);
extern void VectorTransform(Vector& in1, const matrix3x4a_t& in2, Vector &out);
extern void VectorTransform(Vector& in1, matrix3x4_t& in2, Vector &out);
extern void VectorTransform(Vector& in1, const matrix3x4_t& in2, Vector &out);
extern float GetDelta(float hspeed, float maxspeed, float airaccelerate);
extern Vector ExtrapolateTick(Vector p0, Vector v0);

extern inline float RandFloat(float M, float N);

extern Vertex_t RotateVertex(Vector2D& vec, Vector2D& vertex, const float angle);

// sperg cried about the previous method, 
//here's not only a faster one but inaccurate as well to trigger more people
#if 0
inline float FASTSQRT(float x)
{
	unsigned int i = *(unsigned int*)&x;

	i += 127 << 23;
	// approximation of square root
	i >>= 1;
	return *(float*)&i;
}
#else

#endif

__forceinline float DotProduct(const Vector& a, const Vector& b)
{
	CHECK_VALID(a);
	CHECK_VALID(b);
	return(a.x*b.x + a.y*b.y + a.z*b.z);
}

__forceinline float DotProduct(Vector& a, Vector& b)
{
	CHECK_VALID(a);
	CHECK_VALID(b);
	return(a.x*b.x + a.y*b.y + a.z*b.z);
}

__forceinline float DotProduct(Vector* a, Vector* b)
{
	CHECK_VALID(a);
	CHECK_VALID(b);
	return(a->x*b->x + a->y*b->y + a->z*b->z);
}

__forceinline float DotProduct(const Vector* a, const Vector* b)
{
	CHECK_VALID(a);
	CHECK_VALID(b);
	return(a->x*b->x + a->y*b->y + a->z*b->z);
}

inline void ClampViewAngles(QAngle& viewangles)
{
	fmod(viewangles.x, 89.0);
	fmod(viewangles.y, 360.0);
	viewangles.z = 0;
}

inline float DotProduct(const Vector2D& a, const Vector2D& b) { return(a.x*b.x + a.y*b.y); }
//inline Vector2D operator*(float fl, const Vector2D& v) { return v * fl; }
#include <xmmintrin.h>

inline void _SSE_RSqrtInline(float a, float* out)
{
	__m128  xx = _mm_load_ss(&a);
	__m128  xr = _mm_rsqrt_ss(xx);
	__m128  xt;
	xt = _mm_mul_ss(xr, xr);
	xt = _mm_mul_ss(xt, xx);
	xt = _mm_sub_ss(_mm_set_ss(3.f), xt);
	xt = _mm_mul_ss(xt, _mm_set_ss(0.5f));
	xr = _mm_mul_ss(xr, xt);
	_mm_store_ss(out, xr);
}

// FIXME: Change this back to a #define once we get rid of the vec_t version
FORCEINLINE void VectorNormalizeFast(Vector& vec)
{
#if 0
#ifndef DEBUG // stop crashing my edit-and-continue!
#if defined(__i386__) || defined(_M_IX86)
#define DO_SSE_OPTIMIZATION
#endif
#endif
#endif
#define DO_SSE_OPTIMIZATION

#if defined( DO_SSE_OPTIMIZATION )
	float sqrlen = vec.LengthSqr() + 1.0e-10f, invlen;
	_SSE_RSqrtInline(sqrlen, &invlen);
	vec.x *= invlen;
	vec.y *= invlen;
	vec.z *= invlen;
#else
	extern float (FASTCALL *pfVectorNormalize)(Vector& v);
	return (*pfVectorNormalize)(vec);
#endif
}

#if 0
//Todo: put me in area where compiler doesn't spew 1000 errors
inline void SSESqrt(float * __restrict pOut, float * __restrict pIn)
{
	_mm_store_ss(pOut, _mm_sqrt_ss(_mm_load_ss(pIn)));
	// compiles to movss, sqrtss, movss
}
#endif