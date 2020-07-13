#include "precompiled.h"
#include "misc.h"

#include "Interfaces.h"
#include "LocalPlayer.h"
#include "Overlay.h"
#include "Keys.h"
#include "AntiAim.h"
#include "IKeyValuesSystem.h"
#include "UsedConvars.h"

void *KeyValues::GetPtr(const char *keyName, void *defaultValue)
{
	KeyValues *dat = FindKey(keyName, false);
	if (dat)
	{
		switch (dat->m_iDataType)
		{
		case TYPE_PTR:
			return dat->m_pValue;

		case TYPE_WSTRING:
		case TYPE_STRING:
		case TYPE_FLOAT:
		case TYPE_INT:
		case TYPE_UINT64:
		default:
			return NULL;
		};
	}
	return defaultValue;
}

void KeyValues::SetPtr(const char *keyName, void *value)
{
	KeyValues *dat = FindKey(keyName, true);

	if (dat)
	{
		dat->m_pValue = value;
		dat->m_iDataType = TYPE_PTR;
	}
}

bool KeyValues::WriteAsBinary(CUtlBuffer &buffer)
{
#if 1
	using WriteAsBinaryFn = bool(__thiscall*)(KeyValues*, CUtlBuffer&);
	static WriteAsBinaryFn WriteAsBinaryGame = (WriteAsBinaryFn)FindMemoryPattern(MatchmakingHandle, std::string("55  8B  EC  83  E4  C0  83  EC  34  53  56  57  8B  7D  08  8B  D9  89  4C  24  28  F6  47  15  01"));
	return WriteAsBinaryGame(this, buffer);
#else
	if (buffer.IsText()) // must be a binary buffer
		return false;

	if (!buffer.IsValid()) // must be valid, no overflows etc
		return false;

	// Write subkeys:

	// loop through all our peers
	for (const KeyValues *dat = this; dat != NULL; dat = dat->m_pPeer)
	{
		// write type
		buffer.PutUnsignedChar(dat->m_iDataType);

		// write name
		buffer.PutString(dat->GetName());

		// write type
		switch (dat->m_iDataType)
		{
		case TYPE_NONE:
		{
			dat->m_pSub->WriteAsBinary(buffer);
			break;
		}
		case TYPE_STRING:
		{
			if (dat->m_sValue && *(dat->m_sValue))
			{
				buffer.PutString(dat->m_sValue);
			}
			else
			{
				buffer.PutString("");
			}
			break;
		}
		case TYPE_WSTRING:
		{
			int nLength = dat->m_wsValue ? V_wcslen(dat->m_wsValue) : 0;
			buffer.PutShort(nLength);
			for (int k = 0; k < nLength; ++k)
			{
				buffer.PutShort((unsigned short)dat->m_wsValue[k]);
			}
			break;
		}

		case TYPE_INT:
		{
			buffer.PutInt(dat->m_iValue);
			break;
		}

		case TYPE_UINT64:
		{
			buffer.PutInt64(*((int64 *)dat->m_sValue));
			break;
		}

		case TYPE_FLOAT:
		{
			buffer.PutFloat(dat->m_flValue);
			break;
		}
		case TYPE_COLOR:
		{
			buffer.PutUnsignedChar(dat->m_Color[0]);
			buffer.PutUnsignedChar(dat->m_Color[1]);
			buffer.PutUnsignedChar(dat->m_Color[2]);
			buffer.PutUnsignedChar(dat->m_Color[3]);
			break;
		}
		case TYPE_PTR:
		{
#if defined( PLATFORM_64BITS )
			// We only put an int here, because 32-bit clients do not expect 64 bits. It'll cause them to read the wrong
			// amount of data and then crash. Longer term, we may bump this up in size on all platforms, but short term 
			// we don't really have much of a choice other than sticking in something that appears to not be NULL.
			if (dat->m_pValue != 0 && (((int)(intp)dat->m_pValue) == 0))
				buffer.PutInt(31337); // Put not 0, but not a valid number. Yuck.
			else
				buffer.PutInt(((int)(intp)dat->m_pValue));
#else
			//FIXME DYLAN
			//buffer.PutPtr(dat->m_pValue);
#endif
			break;
		}

		default:
			break;
		}
	}

	// write tail, marks end of peers
	buffer.PutUnsignedChar(TYPE_NUMTYPES);

	return buffer.IsValid();
#endif
}


std::string get_hitgroup_name(int id)
{
	//decrypts(0)
	static std::unordered_map< int, std::string > hitgroup_names =
	{
		{ HITGROUP_GENERIC,  XorStr("Generic") },
		{ HITGROUP_HEAD,     XorStr("Head") },
		{ HITGROUP_CHEST,    XorStr("Chest") },
		{ HITGROUP_STOMACH,  XorStr("Stomach") },
		{ HITGROUP_LEFTARM,  XorStr("Left Arm") },
		{ HITGROUP_RIGHTARM, XorStr("Right Arm") },
		{ HITGROUP_LEFTLEG,  XorStr("Left Leg") },
		{ HITGROUP_RIGHTLEG, XorStr("Right Leg") },
		{ HITGROUP_NECK,	 XorStr("Neck") },
		{ HITGROUP_GEAR,     XorStr("Gear") },
	};
	//encrypts(0)

	if (hitgroup_names.find(id) == hitgroup_names.end())
		return std::to_string(id);

	return hitgroup_names[id];
}

std::string get_hitbox_name(int id)
{
	//decrypts(0)
	static std::unordered_map< int, std::string > hitbox_names =
	{
		{ HITBOX_HEAD,				XorStr("Head") },
		{ HITBOX_LOWER_NECK,		XorStr("Neck") },
		{ HITBOX_PELVIS,			XorStr("Pelvis") },
		{ HITBOX_BODY,				XorStr("Body") },
		{ HITBOX_THORAX,			XorStr("Thorax") },
		{ HITBOX_CHEST,				XorStr("Chest") },
		{ HITBOX_UPPER_CHEST,		XorStr("Upper Chest") },
		{ HITBOX_LEFT_THIGH,		XorStr("Left Thigh") },
		{ HITBOX_RIGHT_THIGH,		XorStr("Right Thigh") },
		{ HITBOX_LEFT_CALF,			XorStr("Left Calf") },
		{ HITBOX_RIGHT_CALF,		XorStr("Right Calf") },
		{ HITBOX_LEFT_FOOT,			XorStr("Left Foot") },
		{ HITBOX_RIGHT_FOOT,		XorStr("Right Foot") },
		{ HITBOX_LEFT_HAND,			XorStr("Left Hand") },
		{ HITBOX_RIGHT_HAND,		XorStr("Right Hand") },
		{ HITBOX_LEFT_UPPER_ARM,	XorStr("Left Upperarm") },
		{ HITBOX_LEFT_FOREARM,		XorStr("Left Forearm") },
		{ HITBOX_RIGHT_UPPER_ARM,	XorStr("Right Upperarm") },
		{ HITBOX_RIGHT_FOREARM,		XorStr("Right Forearm") },
	};
	//encrypts(0)

	if (hitbox_names.find(id) == hitbox_names.end())
		return std::to_string(id);

	return hitbox_names[id];
}

inline int GetMaxprocessableUserCmds()
{
#if 1
	auto gamerules = GetGamerules();
	if (gamerules && gamerules->IsValveServer())
		return 8;

	return 16;
#else
	//Valve does not replicate the convar to the client, so we don't know what values the server is using
	if (!sv_maxusrcmdprocessticks.GetVar())
		return 16;
	int val = sv_maxusrcmdprocessticks.GetVar()->GetFloat();
	if (val > 0)
		return min(val, CMD_MAXBACKUP - 1);
	return CMD_MAXBACKUP - 1;
#endif
}

bool Voice_IsRecording()
{
	return *(bool*)(StaticOffsets.GetOffsetValue(_IsVoiceRecording_Ptr));
}

CRC32_t CUserCmd::GetChecksum()
{
	return StaticOffsets.GetOffsetValueByType<CRC32_t(__thiscall*)(CUserCmd*)>(_UserCmd_GetChecksum) (this);
}

void ModifiableUserCmd::Reset(CUserCmd* newcmd)
{
	cmd = newcmd;
	PressingAimbotKey = false;
	PressingTriggerbotKey = false;
	PressingJumpButton = IsJumping();
	bManuallyFiring = LocalPlayer.IsManuallyFiring(LocalPlayer.Entity->GetWeapon());
	bFinalTick = *(bool*)(*FramePointer - 0x1B);
	pbSendPacket = (bool*)(*FramePointer - 0x1C);
	bOriginalSendPacket = *pbSendPacket;
	bSendPacket = *pbSendPacket;

	if (g_ClientState->chokedcommands == 0)
		CreateMoveVars.LastShot.didshoot = false;
}

bool ModifiableUserCmd::IsUserCmdAndPlayerNotMoving()
{
	if (cmd->buttons & IN_USE)
		int i = 0;
	return !LocalPlayer.Entity || (cmd->weaponselect == 0 && cmd->forwardmove == 0.0f && cmd->sidemove == 0.0f && (cmd->buttons & ~(IN_DUCK | IN_BULLRUSH)) == 0 && LocalPlayer.Entity->GetVelocity().Length() < 0.1f);
}

FORCEINLINE void VectorMAInline(const float* start, float scale, const float* direction, float* dest)
{
	dest[0] = start[0] + direction[0] * scale;
	dest[1] = start[1] + direction[1] * scale;
	dest[2] = start[2] + direction[2] * scale;
}

FORCEINLINE void VectorMAInline(const Vector& start, float scale, const Vector& direction, Vector& dest)
{
	dest.x = start.x + direction.x*scale;
	dest.y = start.y + direction.y*scale;
	dest.z = start.z + direction.z*scale;
}

FORCEINLINE void VectorMA(const Vector& start, float scale, const Vector& direction, Vector& dest)
{
	VectorMAInline(start, scale, direction, dest);
}

FORCEINLINE void VectorMA(const float * start, float scale, const float *direction, float *dest)
{
	VectorMAInline(start, scale, direction, dest);
}


//-----------------------------------------------------------------------------
// Purpose: Converts engine-format euler angles to a quaternion
// Input  : angles - Right-handed Euler angles in degrees as follows:
//				[0]: PITCH: Clockwise rotation around the Y axis.
//				[1]: YAW:	Counterclockwise rotation around the Z axis.
//				[2]: ROLL:	Counterclockwise rotation around the X axis.
//			*outQuat - quaternion of form (i,j,k,real)
//-----------------------------------------------------------------------------
void AngleQuaternion(const QAngle &angles, Quaternion &outQuat)
{

	float sr, sp, sy, cr, cp, cy;

	SinCos(DEG2RAD(angles.y) * 0.5f, &sy, &cy);
	SinCos(DEG2RAD(angles.x) * 0.5f, &sp, &cp);
	SinCos(DEG2RAD(angles.z) * 0.5f, &sr, &cr);

	// NJS: for some reason VC6 wasn't recognizing the common subexpressions:
	float srXcp = sr * cp, crXsp = cr * sp;
	outQuat.x = srXcp*cy - crXsp*sy; // X
	outQuat.y = crXsp*cy + srXcp*sy; // Y

	float crXcp = cr * cp, srXsp = sr * sp;
	outQuat.z = crXcp*sy - srXsp*cy; // Z
	outQuat.w = crXcp*cy + srXsp*sy; // W (real component)
}

//-----------------------------------------------------------------------------
// Quaternion sphereical linear interpolation
//-----------------------------------------------------------------------------

void QuaternionSlerp(const Quaternion &p, const Quaternion &q, float t, Quaternion &qt)
{
	Quaternion q2;
	// 0.0 returns p, 1.0 return q.

	// decide if one of the quaternions is backwards
	QuaternionAlign(p, q, q2);

	QuaternionSlerpNoAlign(p, q2, t, qt);
}

void QuaternionSlerpNoAlign(const Quaternion &p, const Quaternion &q, float t, Quaternion &qt)
{
	float omega, cosom, sinom, sclp, sclq;
	int i;

	// 0.0 returns p, 1.0 return q.

	cosom = p[0] * q[0] + p[1] * q[1] + p[2] * q[2] + p[3] * q[3];

	if ((1.0f + cosom) > 0.000001f) {
		if ((1.0f - cosom) > 0.000001f) {
			omega = acosf(cosom);
			sinom = sinf(omega);
			sclp = sinf((1.0f - t)*omega) / sinom;
			sclq = sinf(t*omega) / sinom;
		}
		else {
			// TODO: add short circuit for cosom == 1.0f?
			sclp = 1.0f - t;
			sclq = t;
		}
		for (i = 0; i < 4; i++) {
			qt[i] = sclp * p[i] + sclq * q[i];
		}
	}
	else {

		qt[0] = -q[1];
		qt[1] = q[0];
		qt[2] = -q[3];
		qt[3] = q[2];
		sclp = sinf((1.0f - t) * (0.5f * M_PI));
		sclq = sinf(t * (0.5f * M_PI));
		for (i = 0; i < 3; i++) {
			qt[i] = sclp * p[i] + sclq * qt[i];
		}
	}
}

//-----------------------------------------------------------------------------
// make sure quaternions are within 180 degrees of one another, if not, reverse q
//-----------------------------------------------------------------------------

void QuaternionAlign(const Quaternion &p, const Quaternion &q, Quaternion &qt)
{
	//Assert(s_bMathlibInitialized);

	// FIXME: can this be done with a quat dot product?

	int i;
	// decide if one of the quaternions is backwards
	float a = 0;
	float b = 0;
	for (i = 0; i < 4; i++)
	{
		a += (p[i] - q[i])*(p[i] - q[i]);
		b += (p[i] + q[i])*(p[i] + q[i]);
	}
	if (a > b)
	{
		for (i = 0; i < 4; i++)
		{
			qt[i] = -q[i];
		}
	}
	else if (&qt != &q)
	{
		for (i = 0; i < 4; i++)
		{
			qt[i] = q[i];
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Converts a quaternion into engine angles
// Input  : *quaternion - q3 + q0.i + q1.j + q2.k
//			*outAngles - PITCH, YAW, ROLL
//-----------------------------------------------------------------------------
void QuaternionAngles(const Quaternion &q, QAngle &angles)
{
	// FIXME: doing it this way calculates too much data, needs to do an optimized version...
	matrix3x4_t matrix;
	QuaternionMatrix(q, matrix);
	MatrixAngles(matrix, angles);
}

//-----------------------------------------------------------------------------
// Purpose: Converts a quaternion into engine angles
// Input  : *quaternion - q3 + q0.i + q1.j + q2.k
//			*outAngles - PITCH, YAW, ROLL
//-----------------------------------------------------------------------------
void QuaternionAngles(const Quaternion &q, RadianEuler &angles)
{
	// FIXME: doing it this way calculates too much data, needs to do an optimized version...
	matrix3x4_t matrix;
	QuaternionMatrix(q, matrix);
	MatrixAngles(matrix, angles);
}

//-----------------------------------------------------------------------------
// Make sure the quaternion is of unit length
//-----------------------------------------------------------------------------
float QuaternionNormalize(Quaternion &q)
{
	float radius, iradius;

	radius = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];

	if (radius) // > FLT_EPSILON && ((radius < 1.0f - 4*FLT_EPSILON) || (radius > 1.0f + 4*FLT_EPSILON))
	{
		radius = sqrtf(radius);
		iradius = 1.0f / radius;
		q[3] *= iradius;
		q[2] *= iradius;
		q[1] *= iradius;
		q[0] *= iradius;
	}
	return radius;
}

void QuaternionMatrix(const Quaternion &q, matrix3x4_t& matrix)
{
	matrix[0][0] = 1.0 - 2.0 * q.y * q.y - 2.0 * q.z * q.z;
	matrix[1][0] = 2.0 * q.x * q.y + 2.0 * q.w * q.z;
	matrix[2][0] = 2.0 * q.x * q.z - 2.0 * q.w * q.y;

	matrix[0][1] = 2.0f * q.x * q.y - 2.0f * q.w * q.z;
	matrix[1][1] = 1.0f - 2.0f * q.x * q.x - 2.0f * q.z * q.z;
	matrix[2][1] = 2.0f * q.y * q.z + 2.0f * q.w * q.x;

	matrix[0][2] = 2.0f * q.x * q.z + 2.0f * q.w * q.y;
	matrix[1][2] = 2.0f * q.y * q.z - 2.0f * q.w * q.x;
	matrix[2][2] = 1.0f - 2.0f * q.x * q.x - 2.0f * q.y * q.y;

	matrix[0][3] = 0.0f;
	matrix[1][3] = 0.0f;
	matrix[2][3] = 0.0f;
}

void QuaternionMatrix(const Quaternion &q, const Vector &pos, matrix3x4_t &matrix)
{
	QuaternionMatrix(q, matrix);

	matrix[0][3] = pos.x;
	matrix[1][3] = pos.y;
	matrix[2][3] = pos.z;
}

void MatrixAngles(const matrix3x4_t& matrix, float *angles)
{
	float forward[3];
	float left[3];
	float up[3];

	//
	// Extract the basis vectors from the matrix. Since we only need the Z
	// component of the up vector, we don't get X and Y.
	//
	forward[0] = matrix[0][0];
	forward[1] = matrix[1][0];
	forward[2] = matrix[2][0];
	left[0] = matrix[0][1];
	left[1] = matrix[1][1];
	left[2] = matrix[2][1];
	up[2] = matrix[2][2];

	float xyDist = sqrtf(forward[0] * forward[0] + forward[1] * forward[1]);

	// enough here to get angles?
	if (xyDist > 0.001f)
	{
		// (yaw)	y = ATAN( forward.y, forward.x );		-- in our space, forward is the X axis
		angles[1] = RAD2DEG(atan2f(forward[1], forward[0]));

		// (pitch)	x = ATAN( -forward.z, sqrt(forward.x*forward.x+forward.y*forward.y) );
		angles[0] = RAD2DEG(atan2f(-forward[2], xyDist));

		// (roll)	z = ATAN( left.z, up.z );
		angles[2] = RAD2DEG(atan2f(left[2], up[2]));
	}
	else	// forward is mostly Z, gimbal lock-
	{
		// (yaw)	y = ATAN( -left.x, left.y );			-- forward is mostly z, so use right for yaw
		angles[1] = RAD2DEG(atan2f(-left[0], left[1]));

		// (pitch)	x = ATAN( -forward.z, sqrt(forward.x*forward.x+forward.y*forward.y) );
		angles[0] = RAD2DEG(atan2f(-forward[2], xyDist));

		// Assume no roll in this case as one degree of freedom has been lost (i.e. yaw == roll)
		angles[2] = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Generates Euler angles given a left-handed orientation matrix. The
//			columns of the matrix contain the forward, left, and up vectors.
// Input  : matrix - Left-handed orientation matrix.
//			angles[PITCH, YAW, ROLL]. Receives right-handed counterclockwise
//				rotations in degrees around Y, Z, and X respectively.
//-----------------------------------------------------------------------------

void MatrixAngles(const matrix3x4_t& matrix, RadianEuler &angles, Vector &position)
{
	MatrixGetColumn(matrix, 3, position);
	MatrixAngles(matrix, angles);
}

void MatrixAngles(const matrix3x4_t &matrix, Quaternion &q, Vector &pos)
{
	float trace;
	trace = matrix[0][0] + matrix[1][1] + matrix[2][2] + 1.0f;
	if (trace > 1.0f + 1.192092896e-07F)
	{
		// VPROF_INCREMENT_COUNTER("MatrixQuaternion A",1);
		q.x = (matrix[2][1] - matrix[1][2]);
		q.y = (matrix[0][2] - matrix[2][0]);
		q.z = (matrix[1][0] - matrix[0][1]);
		q.w = trace;
	}
	else if (matrix[0][0] > matrix[1][1] && matrix[0][0] > matrix[2][2])
	{
		// VPROF_INCREMENT_COUNTER("MatrixQuaternion B",1);
		trace = 1.0f + matrix[0][0] - matrix[1][1] - matrix[2][2];
		q.x = trace;
		q.y = (matrix[1][0] + matrix[0][1]);
		q.z = (matrix[0][2] + matrix[2][0]);
		q.w = (matrix[2][1] - matrix[1][2]);
	}
	else if (matrix[1][1] > matrix[2][2])
	{
		// VPROF_INCREMENT_COUNTER("MatrixQuaternion C",1);
		trace = 1.0f + matrix[1][1] - matrix[0][0] - matrix[2][2];
		q.x = (matrix[0][1] + matrix[1][0]);
		q.y = trace;
		q.z = (matrix[2][1] + matrix[1][2]);
		q.w = (matrix[0][2] - matrix[2][0]);
	}
	else
	{
		// VPROF_INCREMENT_COUNTER("MatrixQuaternion D",1);
		trace = 1.0f + matrix[2][2] - matrix[0][0] - matrix[1][1];
		q.x = (matrix[0][2] + matrix[2][0]);
		q.y = (matrix[2][1] + matrix[1][2]);
		q.z = trace;
		q.w = (matrix[1][0] - matrix[0][1]);
	}

	QuaternionNormalize(q);

	MatrixGetColumn(matrix, 3, pos);
}

void MatrixGetColumn(const matrix3x4_t& in, int column, Vector &out)
{
	out.x = in[0][column];
	out.y = in[1][column];
	out.z = in[2][column];
}

void ConcatTransforms(const matrix3x4_t& in1, const matrix3x4_t& in2, matrix3x4_t& out)
{
	//Assert(s_bMathlibInitialized);
	if (&in1 == &out)
	{
		matrix3x4_t in1b;
		MatrixCopy(in1, in1b);
		ConcatTransforms(in1b, in2, out);
		return;
	}
	if (&in2 == &out)
	{
		matrix3x4_t in2b;
		MatrixCopy(in2, in2b);
		ConcatTransforms(in1, in2b, out);
		return;
	}
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
		in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
		in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
		in1[0][2] * in2[2][2];
	out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] +
		in1[0][2] * in2[2][3] + in1[0][3];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
		in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
		in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
		in1[1][2] * in2[2][2];
	out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] +
		in1[1][2] * in2[2][3] + in1[1][3];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
		in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
		in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
		in1[2][2] * in2[2][2];
	out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] +
		in1[2][2] * in2[2][3] + in1[2][3];
}

// BUGBUG: Why doesn't this call angle diff?!?!?
float ApproachAngle(float target, float value, float speed)
{
	target = anglemod(target);
	value = anglemod(value);

	float delta = target - value;

	// Speed is assumed to be positive
	if (speed < 0)
		speed = -speed;

	if (delta < -180)
		delta += 360;
	else if (delta > 180)
		delta -= 360;

	if (delta > speed)
		value += speed;
	else if (delta < -speed)
		value -= speed;
	else
		value = target;

	return value;
}

float Approach(float target, float value, float speed)
{
	float delta = target - value;

	if (delta > speed)
		value += speed;
	else if (delta < -speed)
		value -= speed;
	else
		value = target;

	return value;
}

// BUGBUG: Why do we need both of these?
float AngleDiff(float destAngle, float srcAngle)
{
	float delta;

	delta = fmodf(destAngle - srcAngle, 360.0f);
	if (destAngle > srcAngle)
	{
		if (delta >= 180)
			delta -= 360;
	}
	else
	{
		if (delta <= -180)
			delta += 360;
	}
	return delta;
}

float Bias(float x, float biasAmt)
{
	// WARNING: not thread safe
	static float lastAmt = -1;
	static float lastExponent = 0;
	if (lastAmt != biasAmt)
	{
		lastExponent = log(biasAmt) * -1.4427f; // (-1.4427 = 1 / log(0.5))
	}
	return pow(x, lastExponent);
}

void Vector2D::NormalizeInPlace()
{
	float length2d;
	float dot = x * x + y * y;
	SSESqrt(&length2d, &dot);

	if (length2d == 0.0f)
	{
		x = y = 0.0f;
	}
	else
	{
		const float norm = 1.0f / length2d;
		x *= norm;
		y *= norm;
	}
}

void AddPointToBounds(const Vector& v, Vector& mins, Vector& maxs)
{
	//Assert(s_bMathlibInitialized);
	int		i;
	float	val;

	for (i = 0; i<3; i++)
	{
		val = v[i];
		if (val < mins[i])
			mins[i] = val;
		if (val > maxs[i])
			maxs[i] = val;
	}
}

/*
KeyValues::KeyValues(const char* setName)
{
	ASSIGNVARANDIFNZERODO(_Constructor, reinterpret_cast<void(__thiscall*)(void*, const char*)>(StaticOffsets.GetOffsetValue(_KeyValues_Constructor)))
		_Constructor(this, setName);
}
*/

bool KeyValues::LoadFromBuffer(char const * resourceName, const char * pBuffer, void * pFileSystem, const char * pPathID, GetSymbolProc_t pfnEvaluateSymbolProc)
{
	ASSIGNVARANDIFNZERODO(_LoadFromBuffer, reinterpret_cast<bool(__thiscall*)(KeyValues*, const char*, const char*, void*, const char*, GetSymbolProc_t, int unk)>(StaticOffsets.GetOffsetValue(_KeyValues_LoadFromBuffer)))
		return _LoadFromBuffer(this, resourceName, pBuffer, pFileSystem, pPathID, pfnEvaluateSymbolProc, 0);
	else
		return false;
}

void KeyValues::SetName(const char * setName)
{
	m_iKeyName = KeyValuesSystem()->GetSymbolForString(setName);
}

//-----------------------------------------------------------------------------
// Purpose: Set the integer value of a keyName. 
//-----------------------------------------------------------------------------
void KeyValues::SetInt(const char *keyName, int value)
{
	KeyValues *dat = FindKey(keyName, true);

	if (dat)
	{
		dat->m_iValue = value;
		dat->m_iDataType = TYPE_INT;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the string value of a keyName. 
//-----------------------------------------------------------------------------
void KeyValues::SetString(const char *keyName, const char *value)
{
	KeyValues *dat = FindKey(keyName, true);

	if (dat)
	{
		// delete the old value
		delete[] dat->m_sValue;
		// make sure we're not storing the WSTRING  - as we're converting over to STRING
		delete[] dat->m_wsValue;
		dat->m_wsValue = NULL;

		if (!value)
		{
			// ensure a valid value
			value = "";
		}

		// allocate memory for the new value and copy it in
		int len = Q_strlen(value);
		dat->m_sValue = new char[len + 1];
		Q_memcpy(dat->m_sValue, value, len + 1);

		dat->m_iDataType = TYPE_STRING;
	}
}

void KeyValues::SetWString(const char *keyName, const wchar_t *value)
{
	KeyValues *dat = FindKey(keyName, true);
	if (dat)
	{
		// delete the old value
		delete[] dat->m_wsValue;
		// make sure we're not storing the STRING  - as we're converting over to WSTRING
		delete[] dat->m_sValue;
		dat->m_sValue = NULL;

		if (!value)
		{
			// ensure a valid value
			value = L"";
		}

		// allocate memory for the new value and copy it in
		int len = wcslen(value);
		dat->m_wsValue = new wchar_t[len + 1];
		Q_memcpy(dat->m_wsValue, value, (len + 1) * sizeof(wchar_t));

		dat->m_iDataType = TYPE_WSTRING;
	}
}



#define TRACK_KV_ADD( ptr, name ) 
#define TRACK_KV_REMOVE( ptr )	

//-----------------------------------------------------------------------------
// Purpose: Return the first subkey in the list
//-----------------------------------------------------------------------------
KeyValues *KeyValues::GetFirstSubKey()
{
	return m_pSub;
}

//-----------------------------------------------------------------------------
// Purpose: Return the next subkey
//-----------------------------------------------------------------------------
KeyValues *KeyValues::GetNextKey()
{
	return m_pPeer;
}


//-----------------------------------------------------------------------------
// Purpose: Sets this key's peer to the KeyValues passed in
//-----------------------------------------------------------------------------
void KeyValues::SetNextKey(KeyValues *pDat)
{
	m_pPeer = pDat;
}


KeyValues* KeyValues::GetFirstTrueSubKey()
{
	KeyValues *pRet = m_pSub;
	while (pRet && pRet->m_iDataType != TYPE_NONE)
		pRet = pRet->m_pPeer;

	return pRet;
}

KeyValues* KeyValues::GetNextTrueSubKey()
{
	KeyValues *pRet = m_pPeer;
	while (pRet && pRet->m_iDataType != TYPE_NONE)
		pRet = pRet->m_pPeer;

	return pRet;
}

KeyValues* KeyValues::GetFirstValue()
{
	KeyValues *pRet = m_pSub;
	while (pRet && pRet->m_iDataType == TYPE_NONE)
		pRet = pRet->m_pPeer;

	return pRet;
}

KeyValues* KeyValues::GetNextValue()
{
	KeyValues *pRet = m_pPeer;
	while (pRet && pRet->m_iDataType == TYPE_NONE)
		pRet = pRet->m_pPeer;

	return pRet;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
KeyValues::KeyValues(const char *setName)
{
	CallConstructor(setName);
}

void KeyValues::CallConstructor(const char *setName)
{
	TRACK_KV_ADD(this, setName);

	Init();
	SetName(setName);
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
KeyValues::KeyValues(const char *setName, const char *firstKey, const char *firstValue)
{
	TRACK_KV_ADD(this, setName);

	Init();
	SetName(setName);
	SetString(firstKey, firstValue);
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
KeyValues::KeyValues(const char *setName, const char *firstKey, const wchar_t *firstValue)
{
	TRACK_KV_ADD(this, setName);

	Init();
	SetName(setName);
	SetWString(firstKey, firstValue);
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
KeyValues::KeyValues(const char *setName, const char *firstKey, int firstValue)
{
	TRACK_KV_ADD(this, setName);

	Init();
	SetName(setName);
	SetInt(firstKey, firstValue);
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
KeyValues::KeyValues(const char *setName, const char *firstKey, const char *firstValue, const char *secondKey, const char *secondValue)
{
	TRACK_KV_ADD(this, setName);

	Init();
	SetName(setName);
	SetString(firstKey, firstValue);
	SetString(secondKey, secondValue);
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
KeyValues::KeyValues(const char *setName, const char *firstKey, int firstValue, const char *secondKey, int secondValue)
{
	TRACK_KV_ADD(this, setName);

	Init();
	SetName(setName);
	SetInt(firstKey, firstValue);
	SetInt(secondKey, secondValue);
}

//-----------------------------------------------------------------------------
// Purpose: Initialize member variables
//-----------------------------------------------------------------------------
void KeyValues::Init()
{
	m_iKeyName = 0;
	m_iKeyNameCaseSensitive1 = 0;
	m_iKeyNameCaseSensitive2 = 0;
	m_iDataType = TYPE_NONE;

	m_pSub = NULL;
	m_pPeer = NULL;
	m_pChain = NULL;

	m_sValue = NULL;
	m_wsValue = NULL;
	m_pValue = NULL;

	m_bHasEscapeSequences = false;
}

//-----------------------------------------------------------------------------
// Purpose: memory allocator
//-----------------------------------------------------------------------------
void *KeyValues::operator new(size_t iAllocSize)
{
	//MEM_ALLOC_CREDIT();
	return KeyValuesSystem()->AllocKeyValuesMemory(iAllocSize);
}

void *KeyValues::operator new(size_t iAllocSize, int nBlockUse, const char *pFileName, int nLine)
{
	//MemAlloc_PushAllocDbgInfo(pFileName, nLine);
	void *p = KeyValuesSystem()->AllocKeyValuesMemory(iAllocSize);
	//MemAlloc_PopAllocDbgInfo();
	return p;
}

//-----------------------------------------------------------------------------
// Purpose: deallocator
//-----------------------------------------------------------------------------
void KeyValues::operator delete(void *pMem)
{
	KeyValuesSystem()->FreeKeyValuesMemory(pMem);
}


void KeyValues::operator delete(void *pMem, int nBlockUse, const char *pFileName, int nLine)
{
	KeyValuesSystem()->FreeKeyValuesMemory(pMem);
}

//-----------------------------------------------------------------------------
// Purpose: remove everything
//-----------------------------------------------------------------------------
void KeyValues::RemoveEverything()
{
	KeyValues *dat;
	KeyValues *datNext = NULL;
	for (dat = m_pSub; dat != NULL; dat = datNext)
	{
		datNext = dat->m_pPeer;
		dat->m_pPeer = NULL;
#if defined NO_MALLOC_OVERRIDE || defined NO_MEMOVERRIDE_NEW_DELETE
		dat->CallDestructor();
		FREE(dat);
#else
		delete dat;
#endif
	}

	for (dat = m_pPeer; dat && dat != this; dat = datNext)
	{
		datNext = dat->m_pPeer;
		dat->m_pPeer = NULL;
#if defined NO_MALLOC_OVERRIDE || defined NO_MEMOVERRIDE_NEW_DELETE
		dat->CallDestructor();
		FREE(dat);
#else
		delete dat;
#endif
	}

#if defined NO_MALLOC_OVERRIDE || defined NO_MEMOVERRIDE_NEW_DELETE
	FREE(m_sValue);
#else
	delete[] m_sValue;
#endif
	m_sValue = NULL;
#if defined NO_MALLOC_OVERRIDE || defined NO_MEMOVERRIDE_NEW_DELETE
	FREE(m_wsValue);
#else
	delete[] m_wsValue;
#endif
	m_wsValue = NULL;
}

void KeyValues::CallDestructor()
{
	TRACK_KV_REMOVE(this);

	RemoveEverything();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
KeyValues::~KeyValues()
{
	CallDestructor();
}

void KeyValues::deleteThis()
{
#if defined NO_MALLOC_OVERRIDE || defined NO_MEMOVERRIDE_NEW_DELETE
	CallDestructor();
	FREE(this);
#else
	delete this;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Find a keyValue, create it if it is not found.
//			Set bCreate to true to create the key if it doesn't already exist 
//			(which ensures a valid pointer will be returned)
//-----------------------------------------------------------------------------
#if defined NO_MALLOC_OVERRIDE || defined NO_MEMOVERRIDE_NEW_DELETE
KeyValues *KeyValues::FindKey(const char *keyName, bool bCreate)
{
	return StaticOffsets.GetOffsetValueByType<KeyValues* (__thiscall*)(KeyValues*, const char*, bool)>(_FindKey)(this, keyName, bCreate);
}
#else
KeyValues *KeyValues::FindKey(const char *keyName, bool bCreate)
{
	// return the current key if a NULL subkey is asked for
	if (!keyName || !keyName[0])
		return this;

	// look for '/' characters deliminating sub fields
	char szBuf[256];
	const char *subStr = strchr(keyName, '/');
	const char *searchStr = keyName;

	// pull out the substring if it exists
	if (subStr)
	{
		int size = subStr - keyName;
		Q_memcpy(szBuf, keyName, size);
		szBuf[size] = 0;
		searchStr = szBuf;
	}

	// lookup the symbol for the search string
	HKeySymbol iSearchStr = KeyValuesSystem()->GetSymbolForString(searchStr, bCreate);
	if (iSearchStr == INVALID_KEY_SYMBOL)
	{
		// not found, couldn't possibly be in key value list
		return NULL;
	}

	KeyValues *lastItem = NULL;
	KeyValues *dat;
	// find the searchStr in the current peer list
	for (dat = m_pSub; dat != NULL; dat = dat->m_pPeer)
	{
		lastItem = dat;	// record the last item looked at (for if we need to append to the end of the list)

		// symbol compare
		if (dat->m_iKeyName == iSearchStr)
		{
			break;
		}
	}

	if (!dat && m_pChain)
	{
		dat = m_pChain->FindKey(keyName, false);
	}

	// make sure a key was found
	if (!dat)
	{
		if (bCreate)
		{
			// we need to create a new key
			dat = new KeyValues(searchStr);
			//			Assert(dat != NULL);

						// insert new key at end of list
			if (lastItem)
			{
				lastItem->m_pPeer = dat;
			}
			else
			{
				m_pSub = dat;
			}
			dat->m_pPeer = NULL;

			// a key graduates to be a submsg as soon as it's m_pSub is set
			// this should be the only place m_pSub is set
			m_iDataType = TYPE_NONE;
		}
		else
		{
			return NULL;
		}
	}

	// if we've still got a subStr we need to keep looking deeper in the tree
	if (subStr)
	{
		// recursively chain down through the paths in the string
		return dat->FindKey(subStr + 1, bCreate);
	}

	return dat;
}
#endif


#if defined NO_MALLOC_OVERRIDE || defined NO_MEMOVERRIDE_NEW_DELETE
int KeyValues::GetInt(const char *keyName, int defaultValue)
{
	return StaticOffsets.GetOffsetValueByType<int (__thiscall*)(KeyValues*, const char*, int)>(_GetInt)(this, keyName, defaultValue);
}
#else
int KeyValues::GetInt(const char *keyName, int defaultValue)
{
	KeyValues *dat = FindKey(keyName, false);
	if (dat)
	{
		switch (dat->m_iDataType)
		{
		case TYPE_STRING:
			return atoi(dat->m_sValue);
		case TYPE_WSTRING:
#ifdef _WIN32
			return _wtoi(dat->m_wsValue);
#else
			DevMsg("TODO: implement _wtoi\n");
			return 0;
#endif
		case TYPE_FLOAT:
			return (int)dat->m_flValue;
		case TYPE_UINT64:
			// can't convert, since it would lose data
			Assert(0);
			return 0;
		case TYPE_INT:
		case TYPE_PTR:
		default:
			return dat->m_iValue;
		};
	}
	return defaultValue;
}
#endif

// NOTE: This is just the transpose not a general inverse
void MatrixInvert(const matrix3x4_t& in, matrix3x4_t& out)
{
	if (&in == &out)
	{
		_swap(out[0][1], out[1][0]);
		_swap(out[0][2], out[2][0]);
		_swap(out[1][2], out[2][1]);
	}
	else
	{
		// transpose the matrix
		out[0][0] = in[0][0];
		out[0][1] = in[1][0];
		out[0][2] = in[2][0];

		out[1][0] = in[0][1];
		out[1][1] = in[1][1];
		out[1][2] = in[2][1];

		out[2][0] = in[0][2];
		out[2][1] = in[1][2];
		out[2][2] = in[2][2];
	}

	// now fix up the translation to be in the other space
	float tmp[3];
	tmp[0] = in[0][3];
	tmp[1] = in[1][3];
	tmp[2] = in[2][3];

	out[0][3] = -DotProduct(tmp, out[0]);
	out[1][3] = -DotProduct(tmp, out[1]);
	out[2][3] = -DotProduct(tmp, out[2]);
}

void VectorCopy(RadianEuler const& src, RadianEuler &dst)
{
	CHECK_VALID(src);
	dst.x = src.x;
	dst.y = src.y;
	dst.z = src.z;
}

//-----------------------------------------------------------------------------
// Transform a plane
//-----------------------------------------------------------------------------
void MatrixTransformPlane(const matrix3x4_t &src, const cplane_t &inPlane, cplane_t &outPlane)
{
	// What we want to do is the following:
	// 1) transform the normal into the new space.
	// 2) Determine a point on the old plane given by plane dist * plane normal
	// 3) Transform that point into the new space
	// 4) Plane dist = DotProduct( new normal, new point )

	// An optimized version, which works if the plane is orthogonal.
	// 1) Transform the normal into the new space
	// 2) Realize that transforming the old plane point into the new space
	// is given by [ d * n'x + Tx, d * n'y + Ty, d * n'z + Tz ]
	// where d = old plane dist, n' = transformed normal, Tn = translational component of transform
	// 3) Compute the new plane dist using the dot product of the normal result of #2

	// For a correct result, this should be an inverse-transpose matrix
	// but that only matters if there are nonuniform scale or skew factors in this matrix.
	VectorRotate(inPlane.normal, src, outPlane.normal);
	outPlane.dist = inPlane.dist * DotProduct(outPlane.normal, outPlane.normal);
	outPlane.dist += outPlane.normal.x * src[0][3] + outPlane.normal.y * src[1][3] + outPlane.normal.z * src[2][3];
}

void MatrixITransformPlane(const matrix3x4_t &src, const cplane_t &inPlane, cplane_t &outPlane)
{
	// The trick here is that Tn = translational component of transform,
	// but for an inverse transform, Tn = - R^-1 * T
	Vector vecTranslation;
	MatrixGetColumn(src, 3, vecTranslation);

	Vector vecInvTranslation;
	VectorIRotate(vecTranslation, src, vecInvTranslation);

	VectorIRotate(inPlane.normal, src, outPlane.normal);
	outPlane.dist = inPlane.dist * DotProduct(outPlane.normal, outPlane.normal);
	outPlane.dist -= outPlane.normal.x * vecInvTranslation[0] + outPlane.normal.y * vecInvTranslation[1] + outPlane.normal.z * vecInvTranslation[2];
}

//-----------------------------------------------------------------------------
// Rotates a AABB into another space; which will inherently grow the box. 
// (same as TransformAABB, but doesn't take the translation into account)
//-----------------------------------------------------------------------------
void RotateAABB(const matrix3x4_t &transform, const Vector &vecMinsIn, const Vector &vecMaxsIn, Vector &vecMinsOut, Vector &vecMaxsOut)
{
	Vector localCenter;
	VectorAdd(vecMinsIn, vecMaxsIn, localCenter);
	localCenter *= 0.5f;

	Vector localExtents;
	VectorSubtract(vecMaxsIn, localCenter, localExtents);

	Vector newCenter;
	VectorRotate(localCenter, transform, newCenter);

	Vector newExtents;
	newExtents.x = DotProductAbs(localExtents, transform[0]);
	newExtents.y = DotProductAbs(localExtents, transform[1]);
	newExtents.z = DotProductAbs(localExtents, transform[2]);

	VectorSubtract(newCenter, newExtents, vecMinsOut);
	VectorAdd(newCenter, newExtents, vecMaxsOut);
}

//-----------------------------------------------------------------------------
// Transforms a AABB into another space; which will inherently grow the box.
//-----------------------------------------------------------------------------
void TransformAABB(const matrix3x4_t& transform, const Vector &vecMinsIn, const Vector &vecMaxsIn, Vector &vecMinsOut, Vector &vecMaxsOut)
{
	Vector localCenter;
	VectorAdd(vecMinsIn, vecMaxsIn, localCenter);
	localCenter *= 0.5f;

	Vector localExtents;
	VectorSubtract(vecMaxsIn, localCenter, localExtents);

	Vector worldCenter;
	VectorTransform(localCenter, transform, worldCenter);

	Vector worldExtents;
	worldExtents.x = DotProductAbs(localExtents, transform[0]);
	worldExtents.y = DotProductAbs(localExtents, transform[1]);
	worldExtents.z = DotProductAbs(localExtents, transform[2]);

	VectorSubtract(worldCenter, worldExtents, vecMinsOut);
	VectorAdd(worldCenter, worldExtents, vecMaxsOut);
}

//-----------------------------------------------------------------------------
// Do a piecewise addition of the quaternion elements. This actually makes little 
// mathematical sense, but it's a cheap way to simulate a slerp.
//-----------------------------------------------------------------------------
void QuaternionBlend(const Quaternion &p, const Quaternion &q, float t, Quaternion &qt)
{
	//Assert(s_bMathlibInitialized);
#if ALLOW_SIMD_QUATERNION_MATH
	fltx4 psimd, qsimd, qtsimd;
	psimd = LoadUnalignedSIMD(p.Base());
	qsimd = LoadUnalignedSIMD(q.Base());
	qtsimd = QuaternionBlendSIMD(psimd, qsimd, t);
	StoreUnalignedSIMD(qt.Base(), qtsimd);
#else
	// decide if one of the quaternions is backwards
	Quaternion q2;
	QuaternionAlign(p, q, q2);
	QuaternionBlendNoAlign(p, q2, t, qt);
#endif
}

void QuaternionBlendNoAlign(const Quaternion &p, const Quaternion &q, float t, Quaternion &qt)
{
	//Assert(s_bMathlibInitialized);
	float sclp, sclq;
	int i;

	// 0.0 returns p, 1.0 return q.
	sclp = 1.0f - t;
	sclq = t;
	for (i = 0; i < 4; i++) {
		qt[i] = sclp * p[i] + sclq * q[i];
	}
	QuaternionNormalize(qt);
}