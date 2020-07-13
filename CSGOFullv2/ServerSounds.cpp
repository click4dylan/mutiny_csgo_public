#include "precompiled.h"
#include "ServerSounds.h"
#include "LocalPlayer.h"

#include "Adriel/stdafx.hpp"

void CServerSounds::Start()
{
	m_utlCurSoundList.RemoveAll();
	Interfaces::EngineSound->GetActiveSounds(m_utlCurSoundList);

	// No active sounds.
	if (!m_utlCurSoundList.Count())
		return;

	// Accumulate sounds for esp correction
	for (int iter = 0; iter < m_utlCurSoundList.Count(); iter++)
	{
		SndInfo_t& sound = m_utlCurSoundList[iter];
		if (sound.m_nSoundSource == 0) // Most likely invalid
			continue;

		CBaseEntity* Entity = Interfaces::ClientEntList->GetBaseEntity(sound.m_nSoundSource);
		CBaseEntity* player;

		if (!Entity)
			continue;

		if (Entity->IsWeapon())
			player = Entity->GetOwner();
		else
			player = Entity;

		if (!player || player == LocalPlayer.Entity || sound.m_pOrigin->IsZero() || player->entindex() > 65)
			continue;

		if (!ValidSound(sound))
			continue;

#ifndef IMI_MENU
		if (variable::get().visuals.i_visualize_sound > 0)
		{
			// footsteps only
			if (variable::get().visuals.i_visualize_sound == 2)
			{
				char sound_name[MAX_PATH];
				if (Interfaces::FileSystem->String(sound.m_filenameHandle, sound_name, sizeof(sound_name)))
				{
					//decrypts(0)
					auto foots = XorStr("foots");
					//encrypts(0)
					if (strlen(sound_name) == 0 || !strstr(sound_name, foots))
						continue;
				}
			}

			ImColor color;
			const bool enemy = player->IsEnemy(LocalPlayer.Entity);

			if (enemy && variable::get().visuals.b_visualize_sound_enemy)
			{
				color = variable::get().visuals.col_visualize_sound_enemy.col_color;
			}
			else if(!enemy && variable::get().visuals.b_visualize_sound_teammate)
			{
				color = variable::get().visuals.col_visualize_sound_teammate.col_color;
			}

			g_Beams.render_beam_ring(*player->GetAbsOrigin(), color);
		}
#endif


		//Missing scoreboard life/team checks.


		SetupAdjustPlayer(player, sound);

		m_cSoundPlayers[player->entindex()].Override(sound);
	}

	for (int iter = 1; iter <= MAX_PLAYERS; iter++)
	{
		CBaseEntity* player = Interfaces::ClientEntList->GetBaseEntity(iter);
		if (!player || !player->GetDormant() || !player->GetAlive())
			continue;

		AdjustPlayerBegin(player);
	}

	m_utlvecSoundBuffer = m_utlCurSoundList;
}

void CServerSounds::SetupAdjustPlayer(CBaseEntity* player, SndInfo_t& sound)
{
	int entindex = player->entindex();

	if (player->entindex() > 64)
		return;

	Vector src3D, dst3D;
	trace_t tr;
	Ray_t ray;
	CTraceFilter filter;

	filter.pSkip = (IHandleEntity*)player;
	src3D = (*sound.m_pOrigin) + Vector(0, 0, 1); // So they dont dig into ground incase shit happens /shrug
	dst3D = src3D - Vector(0, 0, 100);
	ray.Init(src3D, dst3D);

	Interfaces::EngineTrace->TraceRay(ray, MASK_PLAYERSOLID, &filter, &tr);

	// step = (tr.fraction < 0.20)
	// shot = (tr.fraction > 0.20)
	// stand = (tr.fraction > 0.50)
	// crouch = (tr.fraction < 0.50)

	// Corrects origin and important flags.

	// Player stuck, idk how this happened
	if (tr.allsolid)
	{
		m_cSoundPlayers[entindex].m_iReceiveTime = -1;
	}

	*sound.m_pOrigin = ((tr.fraction < 0.97) ? tr.endpos : *sound.m_pOrigin);
	m_cSoundPlayers[entindex].m_nFlags = player->GetFlags();
	m_cSoundPlayers[entindex].m_nFlags |= (tr.fraction < 0.50f ? FL_DUCKING : 0) | (tr.fraction != 1 ? FL_ONGROUND : 0);   // Turn flags on
	m_cSoundPlayers[entindex].m_nFlags &= (tr.fraction > 0.50f ? ~FL_DUCKING : 0) | (tr.fraction == 1 ? ~FL_ONGROUND : 0); // Turn flags off
}

void CServerSounds::Finish()
{
	// Do any finishing code here. If we add smtn like sonar radar this will be useful.

	AdjustPlayerFinish();
}

void CServerSounds::AdjustPlayerFinish()
{
	// Restore and clear saved players for next loop.
#if 0
	for (size_t i = 0; i < m_arRestorePlayers.size(); ++i)
	{
		std::vector<SoundPlayer>::value_type& RestorePlayer = m_arRestorePlayers.at(i);

		CBaseEntity* player = RestorePlayer.player;
		*CAST(int*, player, g_NetworkedVariables.Offsets.m_fFlags) = RestorePlayer.m_nFlags;
		*CAST(Vector*, player, g_NetworkedVariables.Offsets.m_vecOrigin) = RestorePlayer.m_vecOrigin;
		player->SetAbsOrigin(RestorePlayer.m_vAbsOrigin);
		//*(bool*)((DWORD)player + 233) = RestorePlayer.m_bDormant; // dormant check
	}
#endif
	m_arRestorePlayers.clear();
}

void CServerSounds::AdjustPlayerBegin(CBaseEntity* player)
{
	// Adjusts player's origin and other vars so we can show full-ish esp.

	int EXPIRE_DURATION = 450; // miliseconds-ish?
	int entindex = player->entindex();

	auto& sound_player = m_cSoundPlayers[entindex];
	bool sound_expired = (int)GetTickCount() - sound_player.m_iReceiveTime > EXPIRE_DURATION;
	if (sound_expired)
	{
		g_Visuals.m_customESP[entindex].m_bIsDormant = false;
		g_Visuals.m_customESP[entindex].m_bWasDormant = false;
		return;
	}

	SoundPlayer current_player;
	current_player.player = player;
	//current_player.m_bDormant = true;
	current_player.m_nFlags = player->GetFlags();
	current_player.m_vecOrigin = player->GetLocalOriginDirect();
	current_player.m_vAbsOrigin = player->GetAbsOriginDirect();
	m_arRestorePlayers.emplace_back(current_player);

	if (!sound_expired)
	{
		//*(bool*)((DWORD)player + 233) = false; // dormant check

		if (!g_Visuals.m_customESP[entindex].m_bIsDormant)
			g_Visuals.arr_alpha[entindex] = 255;
		g_Visuals.m_customESP[entindex].m_bIsDormant = true;
		g_Visuals.m_customESP[entindex].m_bWasDormant = false;
	}
	//*CAST(int*, player, g_NetworkedVariables.Offsets.m_fFlags) = sound_player.m_nFlags;
	//*CAST(Vector*, player, g_NetworkedVariables.Offsets.m_vecOrigin) = sound_player.m_vecOrigin;
	//player->SetAbsOrigin(sound_player.m_vecOrigin);
}

bool CServerSounds::ValidSound(SndInfo_t& sound)
{
	// Use only server dispatched sounds.

	// We don't want the sound to keep following client's predicted origin.
	for (int iter = 0; iter < m_utlvecSoundBuffer.Count(); iter++)
	{
		SndInfo_t& cached_sound = m_utlvecSoundBuffer[iter];
		if (cached_sound.m_nGuid == sound.m_nGuid)
		{
			return false;
		}
	}

	return true;
}

CServerSounds g_ServerSounds;