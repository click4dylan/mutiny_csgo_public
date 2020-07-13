#include "precompiled.h"
#include "Fakelag.h"

#include "LocalPlayer.h"
#include "UsedConvars.h"
#include "WeaponController.h"
#include "INetchannelInfo.h"
#include "TickbaseExploits.h"
#include "misc.h"
#include "Aimbot_imi.h"

#include "Adriel/adr_util.hpp"

Fakelag g_Fakelag;

float Fakelag::get_visual_choke()
{
	return visual_choke;
}

void Fakelag::get_visual_progress()
{
	if (!Interfaces::EngineClient->IsInGame())
		return;

	auto& var = variable::get();

	visual_choke = 1.f;

	if (g_Info.m_iNumCommandsToChoke >= 2)
		visual_choke = static_cast<float>(g_ClientState->chokedcommands) / static_cast<float>(g_Info.m_iNumCommandsToChoke);
}

int Fakelag::mode_step()
{
	if (m_step_limit >= LocalPlayer.Config_GetFakelagMax())
		m_step_limit = LocalPlayer.Config_GetFakelagMin();
	else
		++m_step_limit;

	return m_step_limit;
}

int Fakelag::mode_adaptive()
{
	float vel = LocalPlayer.Entity ? LocalPlayer.Entity->GetVelocity().Length2D() : 250.f;
	float max_vel = LocalPlayer.CurrentWeapon ? LocalPlayer.CurrentWeapon->GetMaxSpeed3() : 250.f;

	// if we're moving max speed, just divide by 1 to get max fakelag
	if (vel >= max_vel)
		max_vel = 1;

	return static_cast<int>(MAX_TICKS_TO_CHOKE * (vel / max_vel));
}

int Fakelag::mode_pingpong()
{
	if (m_pingpong_limit >= LocalPlayer.Config_GetFakelagMax())
		m_pingpong_increase = false;
	else if (m_pingpong_limit <= LocalPlayer.Config_GetFakelagMin())
		m_pingpong_increase = true;

	if (m_pingpong_increase)
		++m_pingpong_limit;
	else
		--m_pingpong_limit;

	return m_pingpong_limit;
}

int Fakelag::mode_latency_seed(int &limit)
{
	int min = LocalPlayer.Config_GetFakelagMin() + 1;
	int max = LocalPlayer.Config_GetFakelagMax() - 2;

	min = clamp(min, 1, MAX_TICKS_TO_CHOKE);
	max = clamp(max, 1, MAX_TICKS_TO_CHOKE);

	int amt = adr_util::random_int(min, max);

	if (m_latencyseed_lastavg > Interfaces::EngineClient->GetNetChannelInfo()->GetAvgLatency(FLOW_OUTGOING))
		limit -= amt;
	else if (m_latencyseed_lastavg < Interfaces::EngineClient->GetNetChannelInfo()->GetAvgLatency(FLOW_OUTGOING))
		limit += amt;

	m_latencyseed_lastavg = Interfaces::EngineClient->GetNetChannelInfo()->GetAvgLatency(FLOW_OUTGOING);

	return limit;
}

void Fakelag::run()
{
	// init max choke
	auto gamerules = GetGamerules();
	int maxChoke = min(MAX_USER_CMDS - 1, MAX_TICKS_TO_CHOKE); // MAX_USER_CMDS - 1;

	auto& var = variable::get();

	if (!LocalPlayer.IsFakeLaggingOnPeek && !LocalPlayer.IsFakeDucking && !LocalPlayer.IsTeleporting && (var.ragebot.exploits.b_hide_shots || var.ragebot.exploits.b_multi_tap.get() || var.ragebot.exploits.b_hide_record || var.ragebot.exploits.b_nasa_walk.get()))
		maxChoke = min(maxChoke, g_Tickbase.GetFakelagLimit());

	int _enforcedMinimum = 0;
	bool _didEnforceMinimum = false;

	bool is_tick_base_ready = false;

	if (variable::get().ragebot.exploits.i_ticks_to_wait >= 10 && variable::get().ragebot.exploits.b_multi_tap.get() && !LocalPlayer.IsFakeDucking)
	{
		if (g_Ragebot.GetTarget() != INVALID_PLAYER)
		{
			CPlayerrecord* _playerRecord = &m_PlayerRecords[g_Ragebot.GetTarget()];

			if (_playerRecord && _playerRecord->m_pEntity &&  _playerRecord->m_pEntity->GetAlive())
				is_tick_base_ready = true;
		}
	}


	if (LocalPlayer.Config_IsDesyncing())
	{
		// force choked amount to never hit below 3 to sustain consistent desync
		_enforcedMinimum = min(1, maxChoke);
	}

	// fakelag disabled or inair only but we're on ground
	if ((!LocalPlayer.Config_IsFakelagging() && !LocalPlayer.Config_IsDesyncing()) || is_tick_base_ready)
	{
		if (!_enforcedMinimum || is_tick_base_ready)
		{
			g_Info.m_iNumCommandsToChoke = 0;

			if (g_ClientState->chokedcommands > g_Info.m_iNumCommandsToChoke)
				g_Info.SetShouldChoke(false);

			return;
		}

		_didEnforceMinimum = true;
	}

	// don't run fakelag if we force choke or force send
	if (!g_Info.ShouldChoke() && g_Info.ShouldChokeNext())
		return;

	if (g_Info.IsForcedToSend())
		return;

	// setup locals
	int _limit = 1;
	bool InAttack = false;

	// no weapon, we're not attacking
	if (!LocalPlayer.CurrentWeapon)
		InAttack = false;
	// we're attacking
	else if ((g_WeaponController.WeaponDidFire(CurrentUserCmd.cmd->buttons | IN_ATTACK2) == 2 && CurrentUserCmd.IsSecondaryAttacking()) ||
		(g_WeaponController.WeaponDidFire(CurrentUserCmd.cmd->buttons | IN_ATTACK) == 1 && CurrentUserCmd.IsAttacking()))
		InAttack = true;

	int move_type = 0;
	bool is_fakelagging = LocalPlayer.Config_IsFakelagging(&move_type);

	// do fake lag
	if (!InAttack && (is_fakelagging || _didEnforceMinimum))
	{
		if (_didEnforceMinimum)
		{
			_limit = g_Info.m_iNumCommandsToChoke;
		}
		else
		{
			int mode = LocalPlayer.Config_GetFakelagMode();

			// static
			if (mode == 0)
			{
				_limit = LocalPlayer.Config_GetFakelagStatic();
			}
			// step
			else if (mode == 1)
			{
				_limit = mode_step();
			}
			// adaptive | pingpong when standing
			else if (mode == 2)
			{
				if (move_type <= 0)
					_limit = mode_pingpong();
				else
					_limit = mode_adaptive();
			}
			// pingpong | latency when standing
			else if (mode == 3)
			{
				if (move_type <= 0)
					_limit = mode_latency_seed(_limit);
				else
					_limit = mode_pingpong();
			}
			// latency | random when standing
			else if (mode == 4)
			{
				if (move_type <= 0)
					_limit = adr_util::random_int(LocalPlayer.Config_GetFakelagMin(), LocalPlayer.Config_GetFakelagMax());
				else
					_limit = mode_latency_seed(_limit);
			}
			// random | nothing while standing
			else if (mode == 5)
			{
				if (move_type <= 0)
					_limit = LocalPlayer.Config_GetFakelagStatic();
				else
					_limit = adr_util::random_int(LocalPlayer.Config_GetFakelagMin(), LocalPlayer.Config_GetFakelagMax());
			}

			// apply disruption
			if (LocalPlayer.Config_IsDisruptingFakelag() && !var.ragebot.exploits.b_multi_tap.get())
			{
				// calculate chance for the disrupt
				if (adr_util::random_float(0.f, 100.f) < LocalPlayer.Config_GetFakelagDisruptChance())
				{
					// calculate random decrease or increase
					int dir = adr_util::random_int(0, 1);
					if (dir == 0)
					{
						_limit += adr_util::random_int(1, 2);
					}
					else
					{
						_limit -= adr_util::random_int(1, 2);
					}
				}
			}
		}

		if (var.ragebot.exploits.b_nasa_walk.get())
		{
			_limit = min(_limit, 4);
		}

		// clamp fakelag amount
		g_Info.m_iNumCommandsToChoke = clamp(_limit, _enforcedMinimum, maxChoke);

		// ghetto fix for fakelag while talking
		if (g_Info.m_iNumCommandsToChoke > 2 && Voice_IsRecording())
		{
			g_Info.m_iNumCommandsToChoke = 2;
		}

		//If we can still choke commands, do so
		if (g_ClientState->chokedcommands < g_Info.m_iNumCommandsToChoke)
			g_Info.SetShouldChoke(true);
	}
}