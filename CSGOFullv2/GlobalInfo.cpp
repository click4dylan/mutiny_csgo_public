#include "precompiled.h"
#include "GlobalInfo.h"
#include "WeaponController.h"
#include "LocalPlayer.h"
#include "UsedConvars.h"
#include "Aimbot_imi.h"
#include "../Adriel/variable.hpp"

CGlobalInfo g_Info;
std::mutex g_SpreadMutex;

void CGlobalInfo::HandleChoke()
{
	// init max amount to choke
#if defined DYLAN_VAC
	const int maxChoke = 16;
#else
	const int maxChoke = LocalPlayer.IsAllowedUntrusted() ? 14 : (MAX_USER_CMDS - 1);
#endif

	if (!m_bShouldChoke && m_bNextShouldChoke)
	{
		m_bShouldChoke = true;
		m_bNextShouldChoke = false;
	}

	bool is_tick_base_ready = false;

	if (variable::get().ragebot.exploits.i_ticks_to_wait >= 10 && variable::get().ragebot.exploits.b_multi_tap.get() && !LocalPlayer.IsFakeDucking)
	{
		if (g_Ragebot.GetTarget() != INVALID_PLAYER)
		{
			CPlayerrecord* _playerRecord = &m_PlayerRecords[g_Ragebot.GetTarget()];

			if (_playerRecord && _playerRecord->m_pEntity &&  _playerRecord->m_pEntity->GetAlive())
			{
				is_tick_base_ready = true;
			}
		}
	}

	if (m_bForceSend)
	{
		m_bShouldChoke = false;
		m_bForceSend = false;
	}

	// should and can choke
	if (m_bShouldChoke && g_ClientState->chokedcommands < maxChoke && !is_tick_base_ready)
		CurrentUserCmd.bSendPacket = false;
	else
		CurrentUserCmd.bSendPacket = true;

	// backup last choke
	m_bLastShouldChoke = !CurrentUserCmd.bSendPacket;
}
