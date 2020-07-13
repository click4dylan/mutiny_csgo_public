#include "precompiled.h"
#include "Assistance.h"
#include "WeaponController.h"
#include "LocalPlayer.h"
#include "AntiAim.h"
#include "UsedConvars.h"
#include "TickbaseExploits.h"
#include "INetchannelInfo.h"

#include "Adriel/stdafx.hpp"
#include "Adriel/input.hpp"
#include "Adriel/adr_util.hpp"

CAssistance g_Assistance;

#define CheckIfNonValidNumber(x) (fpclassify(x) == FP_INFINITE || fpclassify(x) == FP_NAN || fpclassify(x) == FP_SUBNORMAL)

void CAssistance::FixButtons(CUserCmd* cmd)
{
	if (LocalPlayer.Entity->GetMoveType() == MOVETYPE_LADDER && (cmd->forwardmove != 0.0f || cmd->sidemove != 0.0f || cmd->upmove != 0.0f))
		return;

	cmd->buttons &= ~IN_MOVELEFT;
	cmd->buttons &= ~IN_MOVERIGHT;
	cmd->buttons &= ~IN_FORWARD;
	cmd->buttons &= ~IN_BACK;

	static bool sw = false;
	sw = !sw;
	if (CurrentUserCmd.bSendPacket && LocalPlayer.Entity && LocalPlayer.Entity->GetFlags() & FL_ONGROUND && LocalPlayer.Entity->GetMoveType() != MOVETYPE_LADDER
		&& variable::get().ragebot.b_enabled && variable::get().ragebot.b_antiaim)
	{
		//Desync the feet for other players

		if (cmd->forwardmove < 0.0f)
			cmd->buttons |= IN_FORWARD;
		else if (cmd->forwardmove > 0.0f)
			cmd->buttons |= IN_BACK;

		if (cmd->sidemove < 0.0f)
			cmd->buttons |= IN_MOVERIGHT;
		else if (cmd->sidemove > 0.0f)
			cmd->buttons |= IN_MOVELEFT;
		return;
	}

	if (cmd->sidemove < 0.0f)
		cmd->buttons |= IN_MOVELEFT;
	else if (cmd->sidemove > 0.0f)
		cmd->buttons |= IN_MOVERIGHT;

	if (cmd->forwardmove < 0.0f)
		cmd->buttons |= IN_BACK;
	else if (cmd->forwardmove > 0.0f)
		cmd->buttons |= IN_FORWARD;

#if 0
#ifndef PRODUCTION
	static bool slide = false;
	static float nexttime = FLT_MIN;
	if (g_Input.IsKeyPressed(VK_F2) && Interfaces::Globals->realtime > nexttime)
	{
		slide = !slide;
		nexttime = Interfaces::Globals->realtime + 1.5f;
	}

	if (slide)
	{
		cmd->buttons &= ~IN_MOVELEFT;
		cmd->buttons &= ~IN_MOVERIGHT;
		cmd->buttons &= ~IN_FORWARD;
		cmd->buttons &= ~IN_BACK;

		if (cmd->forwardmove < 0.0f)
			cmd->buttons |= IN_FORWARD;
		else if (cmd->forwardmove > 0.0f)
			cmd->buttons |= IN_BACK;

		if (cmd->sidemove < 0.0f)
			cmd->buttons |= IN_MOVERIGHT;
		else if (cmd->sidemove > 0.0f)
			cmd->buttons |= IN_MOVELEFT;
	}
#endif
#endif
}

__forceinline float get_ideal_rotation(float speed) {
	// 15.f is the ideal angle of rotation
	float factor = RAD2DEG(std::atan2(15.f, speed));
	// clamp it from 0 to 45 for maximum movement
	return clamp(factor, 0.f, 45.f) * Interfaces::Globals->interval_per_tick;
}

__forceinline void rotate_movement(float yaw_to_rotate_towards) {
	const float rot = DEG2RAD(CurrentUserCmd.cmd->viewangles.y - yaw_to_rotate_towards);

	const float new_forward = (std::cos(rot) * CurrentUserCmd.cmd->forwardmove) - (std::sin(rot) * CurrentUserCmd.cmd->sidemove);
	const float new_side = (std::sin(rot) * CurrentUserCmd.cmd->forwardmove) + (std::cos(rot) * CurrentUserCmd.cmd->sidemove);

	CurrentUserCmd.cmd->forwardmove = new_forward;
	CurrentUserCmd.cmd->sidemove = new_side;
}

void CAssistance::Autostrafe()
{
#if 0
	auto GetDegreeFromVelocity = [](float VelocityLength2D)
	{
		float retn = RAD2DEG(asinf(30.f / VelocityLength2D));
		float v2 = 90.f;

		if (!isfinite(retn) || retn > 90.f || (v2 = 0.f, retn < 0.f))
			return v2;
		else
			return retn;
	};

#if 0
	auto GetVelocityYawStep = [](Vector Velocity, float CircleStrafeYaw)
	{
		float degVelocity = RAD2DEG(atan2(Velocity.y, Velocity.x)), retn = 1.5f;

		Vector start, end;
		trace_t trace;
		CTraceFilterSimple filter;
		filter.SetPassEntity((IHandleEntity*)LocalPlayer.Entity);
		start = end = LocalPlayer.Entity->GetAbsOriginDirect() + Vector(0, 0, 24.f);
		float v2 = 0.f;

		while (true)
		{
			v2 = degVelocity + CircleStrafeYaw;

			end.x += (Interfaces::Globals->interval_per_tick * (cos(DEG2RAD(v2)) * Velocity.Length2D()));
			end.y += (Interfaces::Globals->interval_per_tick * (sin(DEG2RAD(v2)) * Velocity.Length2D()));

			Interfaces::EngineTrace->TraceRay(Ray_t(start, end, Vector(-20.f, -20.f, 0), Vector(20.f, 20.f, 32.f)), MASK_SOLID | CONTENTS_HITBOX, &filter, &trace);

			if (trace.DidHit())
			{
				if (Convars::mp_solid_teammates->GetInt() > 0) // NOTE: teammates are solid so dont ignore them in the trace since we can collide with them
				{
					Interfaces::DebugOverlay->AddLineOverlay(start, end, 255, 0, 0, 0, Interfaces::Globals->interval_per_tick * 3.f);
					break;
				}
				else
				{
					if (trace.m_pEnt
						&& trace.m_pEnt->entindex() != Interfaces::EngineClient->GetLocalPlayer()
						&& (trace.m_pEnt->GetTeam() != LocalPlayer.Entity->GetTeam()
							|| trace.m_pEnt == Interfaces::ClientEntList->GetBaseEntity(0))) // NOTE: don't ignore enemys or the world entity since we can collide with them
					{
						Interfaces::DebugOverlay->AddLineOverlay(start, end, 255, 0, 0, 0, Interfaces::Globals->interval_per_tick * 3.f);
						break;
					}
				}
			}
			else
				Interfaces::DebugOverlay->AddLineOverlay(start, end, 0, 255, 0, 0, Interfaces::Globals->interval_per_tick * 3.f);

			retn -= Interfaces::Globals->interval_per_tick;

			if (retn == 0.f) break;

			start = end;
			degVelocity = v2;
		}

		return retn;
	};
#endif

	if (LocalPlayer.Entity->GetMoveType() != MOVETYPE_WALK
		|| LocalPlayer.Entity->GetFlags() & FL_ONGROUND)
		return;

	auto Velocity = LocalPlayer.Entity->GetVelocity();
	Velocity.z = 0;

	static bool Switch = false;
	float SwitchFloat = (Switch) ? 1.f : -1.f;
	Switch = !Switch;

	if (CurrentUserCmd.cmd->forwardmove > 0.f)
		CurrentUserCmd.cmd->forwardmove = 0.f;

	static bool inStrafing = false;

	Vector viewDirection;
	QAngle localAngles;
	Interfaces::EngineClient->GetViewAngles(localAngles);
	AngleVectors(localAngles, &viewDirection);

	float currentAngle = RAD2DEG(acos(viewDirection.Dot(Velocity) / (viewDirection.Length() * Velocity.Length())));

	if (currentAngle > 180.f)
		currentAngle = 360.f - currentAngle;

	if (inStrafing && currentAngle < 10.f)
		inStrafing = false;

	inStrafing = false;

	float v18 = clamp(RAD2DEG(asinf(15.f / Velocity.Length2D())), 0.f, 90.f);
	auto YawDelta = NormalizeFloatr(m_angStrafeAngle.y - m_flOldYaw), AbsYawDelta = abs(YawDelta);
	m_flOldYaw = m_angStrafeAngle.y;

	if (YawDelta > 0.f)
		CurrentUserCmd.cmd->sidemove = -450.f;
	else if (YawDelta < 0.f)
		CurrentUserCmd.cmd->sidemove = 450.f;

	if (AbsYawDelta <= v18 || AbsYawDelta >= 30.f)
	{
		QAngle VelocityAngles;
		VectorAngles(Velocity, VelocityAngles);
		float v42 = NormalizeFloatr(m_angStrafeAngle.y - VelocityAngles.y);
		float v26 = GetDegreeFromVelocity(Velocity.Length2D()) * 2.f/*retrack value*/;

		if (v42 <= v26 || Velocity.Length2D() <= 15.f)
		{
			if (-v26 <= v42 || Velocity.Length2D() <= 15.f)
			{
				m_angStrafeAngle.y += (v18 * SwitchFloat);
				CurrentUserCmd.cmd->sidemove = SwitchFloat * 450.f;
			}
			else
			{
				m_angStrafeAngle.y = VelocityAngles.y - v26;
				CurrentUserCmd.cmd->sidemove = 450.f;
			}
		}
		else
		{
			m_angStrafeAngle.y = VelocityAngles.y + v26;
			CurrentUserCmd.cmd->sidemove = -450.f;
		}
	}

	FixButtons(CurrentUserCmd.cmd);

#if 0
	auto GetDegreeFromVelocity = [](float VelocityLength2D)
	{
		auto tmp = RAD2DEG(atan2(30.f, VelocityLength2D)); //TODO: atan is apperently wrong so find the right math func

		if (CheckIfNonValidNumber(tmp) || tmp > 90.f)
			return 90.f;
		else if (tmp < 0.f)
			return 0.f;
		else
			return tmp;
	};

	auto GetVelocityYawStep = [](Vector Velocity, float NewCircleYaw)
	{
		auto degVelocity = RAD2DEG(atan2(Velocity.y, Velocity.x)), retn = 1.5f;

		Vector start = LocalPlayer.Current_Origin, end = LocalPlayer.Current_Origin;
		Ray_t ray;
		CGameTrace trace;
		CTraceFilterWorldAndPropsOnly filter;

		while (true)
		{
			end.x = end.x + ((cos(DEG2RAD(degVelocity + NewCircleYaw)) * Velocity.Length2D()) * Interfaces::Globals->frametime);
			end.y = end.y + ((sin(DEG2RAD(degVelocity + NewCircleYaw)) * Velocity.Length2D()) * Interfaces::Globals->frametime);
			end *= Interfaces::Globals->frametime;

			ray.Init(start, end, Vector(-20.f, -20.f, 0), Vector(20.f, 20.f, 32.f));
			Interfaces::EngineTrace->TraceRay(ray, CONTENTS_SOLID, &filter, &trace);

			if (trace.fraction < 1.0 || trace.allsolid || trace.startsolid)
				break;

			retn -= Interfaces::Globals->frametime;

			if (retn == 0.f)
				break;

			start = end;
			degVelocity += (degVelocity + NewCircleYaw);
		}

		return retn;
	};

	if (LocalPlayer.MoveType != MOVETYPE_WALK)
		return;

	auto Velocity = LocalPlayer.Entity->GetVelocity();
	Velocity.z = 0;

	static bool Switch = false;
	float SwitchFloat = (Switch) ? 1.f : -1.f;
	Switch = !Switch;

	static bool in_strafing = false;

	/*if ((GetAsyncKeyState('F') & 0x8000))
	{
	//		CircleStrafe(cmd);
	float tmp = GetDegreeFromVelocity(Velocity.Length2D());
	CircleYaw = NormalizeFloat(CircleYaw + tmp);

	float v14 = GetVelocityYawStep(Velocity, tmp);

	if (v14 != 0.f)
	CircleYaw += (((Interfaces::GlobalVars->frametime * 128.f) * v14) * v14);

	cmd->viewangles.y = NormalizeFloat(CircleYaw);
	cmd->sidemove = -450.f;
	return;
	}*/

	if (LocalPlayer.Entity->GetFlags() & FL_ONGROUND)
		return;

	if (CurrentUserCmd.cmd->forwardmove > 0.f)
		CurrentUserCmd.cmd->forwardmove = 0.f;

	//auto StrafeAngle = RAD2DEG(atan(15.f / Velocity.Length2D()));
	auto StrafeAngle = 850.f / Velocity.Length2D();

	if (StrafeAngle > 90.f)
		StrafeAngle = 90.f;
	else if (StrafeAngle < 0.f)
		StrafeAngle = 0.f;

	auto YawDelta = NormalizeFloatr(LocalPlayer.FinalEyeAngles.y - m_flOldYaw);
	m_flOldYaw = LocalPlayer.FinalEyeAngles.y;

	if (YawDelta > 0.f)
		CurrentUserCmd.cmd->sidemove = -450.f;
	else if (YawDelta < 0.f)
		CurrentUserCmd.cmd->sidemove = 450.f;

	auto AbsYawDelta = abs(YawDelta);

	if (AbsYawDelta <= StrafeAngle || AbsYawDelta >= 30.f)
	{
		QAngle VelocityAngles;
		VectorAngles(Velocity, VelocityAngles);

		auto VelocityAngleYawDelta = NormalizeFloatr(LocalPlayer.FinalEyeAngles.y - VelocityAngles.y);
		auto VelocityDegree = 15.f;//GetDegreeFromVelocity(Velocity.Length2D()) * 2.f/*retrack value*/;

		if (VelocityAngleYawDelta <= VelocityDegree || Velocity.Length2D() <= 15.f)
		{
			if (-(VelocityDegree) <= VelocityAngleYawDelta || Velocity.Length2D() <= 15.f)
			{
				LocalPlayer.FinalEyeAngles.y += (StrafeAngle * SwitchFloat);
				CurrentUserCmd.cmd->sidemove = 450.f * SwitchFloat;
			}
			else
			{
				LocalPlayer.FinalEyeAngles.y = VelocityAngles.y - VelocityDegree;
				CurrentUserCmd.cmd->sidemove = 450.f;
			}
		}
		else
		{
			LocalPlayer.FinalEyeAngles.y = VelocityAngles.y + VelocityDegree;
			CurrentUserCmd.cmd->sidemove = -450.f;
		}
	}

	CurrentUserCmd.cmd->buttons &= ~(IN_MOVELEFT | IN_MOVERIGHT | IN_FORWARD | IN_BACK);

	if (CurrentUserCmd.cmd->sidemove <= 0.0)
		CurrentUserCmd.cmd->buttons |= IN_MOVELEFT;
	else
		CurrentUserCmd.cmd->buttons |= IN_MOVERIGHT;

	if (CurrentUserCmd.cmd->forwardmove <= 0.0)
		CurrentUserCmd.cmd->buttons |= IN_BACK;
	else
		CurrentUserCmd.cmd->buttons |= IN_FORWARD;
#endif
#endif

	auto& var = variable::get();

	if (!var.misc.b_autostrafe)
		return;

	if (!LocalPlayer.Entity || LocalPlayer.Entity->GetMoveType() == MOVETYPE_NOCLIP || LocalPlayer.Entity->GetMoveType() == MOVETYPE_LADDER)
		return;

	if (LocalPlayer.Entity->IsOnGround() && !(CurrentUserCmd.cmd->buttons & IN_JUMP))
		return;

	static bool side_switch = false;
	side_switch = !side_switch;

	CurrentUserCmd.cmd->forwardmove = 0.f;
	CurrentUserCmd.cmd->sidemove = side_switch ? 450.f : -450.f;

	QAngle velocity_angle;
	Vector velocity = LocalPlayer.Entity->GetAbsVelocityDirect();

	VectorAngles(velocity, velocity_angle);

	float velocity_yaw = velocity_angle.y;

	float ideal_rotation = get_ideal_rotation(velocity.Length2D()) / Interfaces::Globals->interval_per_tick;

	if (!side_switch)
		ideal_rotation *= -1.f;

	float delta = velocity_yaw - CurrentUserCmd.cmd->viewangles.y;
	NormalizeAngle(delta);

	float ideal_yaw_rot = std::fabsf(delta) < 5.f ? ideal_rotation + velocity_yaw : ideal_rotation + CurrentUserCmd.cmd->viewangles.y;

	rotate_movement(ideal_yaw_rot);
}

void CAssistance::BunnyHop()
{
	m_angStrafeAngle = LocalPlayer.FinalEyeAngles;
	if (variable::get().misc.b_autostrafe && (CurrentUserCmd.cmd->buttons & IN_JUMP) && !(LocalPlayer.Entity->GetFlags() & FL_ONGROUND))
		Autostrafe();

	m_angPostStrafe = LocalPlayer.FinalEyeAngles;

#if 0
	if (LocalPlayer.Entity && CurrentUserCmd.cmd)
	{
		int flags = LocalPlayer.Entity->GetFlags();
		printf("flags: %d | injump: %d | not on partial ground: %d | on ground: %d\n", flags, CurrentUserCmd.cmd->buttons & IN_JUMP, !(flags & FL_PARTIALGROUND), flags & FL_ONGROUND);
	}
#endif

	if (variable::get().misc.b_bhop && (CurrentUserCmd.cmd->buttons & IN_JUMP && !(LocalPlayer.Entity->GetFlags() & FL_PARTIALGROUND || LocalPlayer.Entity->GetFlags() & FL_ONGROUND)))
	{
		// todo: nit; fix movement speed by checking current duck amount to prevent deceleration in air
		//if (variable::get().misc.b_bhop_duck)
		//{
		//	CurrentUserCmd.cmd->buttons |= IN_DUCK;
		//}

		CurrentUserCmd.cmd->buttons &= ~IN_JUMP;
	}
}

void CAssistance::FixMovement()
{
	if ((!variable::get().ragebot.b_enabled || !variable::get().ragebot.b_antiaim) && !variable::get().misc.b_autostrafe )
		return;

	QAngle real_angles;
	Interfaces::EngineClient->GetViewAngles(real_angles);

	// adjust for roll nospread
	if (!(LocalPlayer.Entity->GetFlags() & FL_ONGROUND) && real_angles.z != 0.f)
		CurrentUserCmd.cmd->sidemove = 0.f;

	Vector move = Vector{ CurrentUserCmd.cmd->forwardmove, CurrentUserCmd.cmd->sidemove, CurrentUserCmd.cmd->upmove };
	const float len = move.Length2D();

	// don't need to apply movefixes, just clamp and fix buttons
	if (len == 0.f)
	{
		CurrentUserCmd.cmd->forwardmove = clamp(CurrentUserCmd.cmd->forwardmove, -450.f, 450.f);
		CurrentUserCmd.cmd->sidemove = clamp(CurrentUserCmd.cmd->sidemove, -450.f, 450.f);
		CurrentUserCmd.cmd->upmove = clamp(CurrentUserCmd.cmd->upmove, -320.f, 320.f);

		FixButtons(CurrentUserCmd.cmd);
		return;
	}

	QAngle move_angle;
	VectorAngles(move, move_angle);

	float yaw = DEG2RAD(CurrentUserCmd.cmd->viewangles.y - real_angles.y + move_angle.y);

	CurrentUserCmd.cmd->forwardmove = cos(yaw) * len;
	CurrentUserCmd.cmd->sidemove = sin(yaw) * len;

	//if ((CurrentUserCmd.cmd->viewangles.x >= 89.0f) || (CurrentUserCmd.cmd->viewangles.x <= -89.0f))
	//{
	//	if (g_Assistance.m_angStrafeAngle.x >= 0.0f && g_Assistance.m_angStrafeAngle.x <= 89.0f)
	//		g_Assistance.m_angStrafeAngle.x = CurrentUserCmd.cmd->viewangles.x + 180.0f;
	//
	//	if (g_Assistance.m_angStrafeAngle.x <= 0.0f && g_Assistance.m_angStrafeAngle.x >= -89.0f)
	//		g_Assistance.m_angStrafeAngle.x = CurrentUserCmd.cmd->viewangles.x - 180.0f;
	//}
	//
	//const QAngle adjusted = movenormang + (CurrentUserCmd.cmd->viewangles - g_Assistance.m_angStrafeAngle);
	//
	//AngleVectors(adjusted, &dir);
	//
	//const Vector set = dir * len;
	//
	//if ((CurrentUserCmd.cmd->viewangles.x > 89.0f) || (CurrentUserCmd.cmd->viewangles.x < -89.0f))
	//	CurrentUserCmd.cmd->forwardmove = set.x;
	//else if ((CurrentUserCmd.cmd->viewangles.x == 89.0f || CurrentUserCmd.cmd->viewangles.x == -89.0f))
	//	CurrentUserCmd.cmd->forwardmove = -set.x;
	//else
	//	CurrentUserCmd.cmd->forwardmove = set.x;
	//
	//if ((CurrentUserCmd.cmd->viewangles.x >= 89.0f) || (CurrentUserCmd.cmd->viewangles.x <= -89.0f))
	//	CurrentUserCmd.cmd->sidemove = -set.y;
	//else
	//	CurrentUserCmd.cmd->sidemove = set.y;

	//clamp moves
	CurrentUserCmd.cmd->forwardmove = clamp(CurrentUserCmd.cmd->forwardmove, -450.f, 450.f);
	CurrentUserCmd.cmd->sidemove = clamp(CurrentUserCmd.cmd->sidemove, -450.f, 450.f);
	CurrentUserCmd.cmd->upmove = clamp(CurrentUserCmd.cmd->upmove, -320.f, 320.f);

	FixButtons(CurrentUserCmd.cmd);
}

int temp_fakelag_limit = 1;

void CAssistance::LagCrouch()
{
	static bool _WasOff = true;
	static bool _WaitUntilZeroChoked = false;
	bool _StopFakeDucking = false;
	bool _IsOn = input::get().is_key_down(variable::get().misc.i_fakeduck_key);

	LocalPlayer.IsFakeDucking = false;

	if (!_IsOn && !_WasOff)
	{
		if (LocalPlayer.Entity->GetDuckAmount() < 1.0f)
		{
			_IsOn = true;
			_StopFakeDucking = true;
		}
	}

	if (_IsOn)
	{
		LocalPlayer.IsFakeDucking = true;

		if (_WasOff)
			_WaitUntilZeroChoked = true;

		if (_WaitUntilZeroChoked && g_ClientState->chokedcommands == 0)
			_WaitUntilZeroChoked = false;

		g_Info.SetShouldChoke(g_ClientState->chokedcommands < min(GetMaxprocessableUserCmds() - 1, 14));

		if (_StopFakeDucking || _WaitUntilZeroChoked || g_ClientState->chokedcommands > 6)
			CurrentUserCmd.cmd->buttons |= IN_DUCK;
		else
			CurrentUserCmd.cmd->buttons &= ~IN_DUCK;
	}

	_WasOff = !_IsOn;
}

void CAssistance::Teleport()
{
	bool _IsOn = variable::get().ragebot.exploits.b_nasa_walk.get();

	if (!_IsOn 
		|| LocalPlayer.IsFakeDucking 
		|| (!g_Tickbase.m_bReadyToShiftTickbase)
		|| !LocalPlayer.IsAllowedUntrusted()
		|| (CurrentUserCmd.cmd->forwardmove == 0.0f && CurrentUserCmd.cmd->sidemove == 0.0f)
		|| LocalPlayer.Entity->GetVelocity().Length() < 5.0f)
	{
		LocalPlayer.IsTeleporting = false;
		return;
	}

	LocalPlayer.IsTeleporting = true;
	g_Info.SetShouldChoke(g_ClientState->chokedcommands < 14);
}

bool CAssistance::LineGoesThroughSmoke(Vector startPos, Vector endPos)
{
	auto _Func = StaticOffsets.GetOffsetValueByType<bool(*)(Vector, Vector)>(_LineGoesThroughSmoke);

	if (_Func)
		return _Func(startPos, endPos);

	return false;
}
