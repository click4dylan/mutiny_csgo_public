#pragma once

#include "Includes.h"
#include "IEngineSound.h"
#include "utlvector.h"

class CServerSounds
{
public:
	// Call before and after ESP.
	void Start();
	void Finish();

private:

	void AdjustPlayerBegin(CBaseEntity* player);
	void AdjustPlayerFinish();
	void SetupAdjustPlayer(CBaseEntity* player, SndInfo_t& sound);

	bool ValidSound(SndInfo_t& sound);

	struct SoundPlayer
	{
		void Override(SndInfo_t& sound) {
			m_iIndex = sound.m_nSoundSource;
			m_vecOrigin = *sound.m_pOrigin;
			m_iReceiveTime = GetTickCount();
		}

		int m_iIndex = 0;
		int m_iReceiveTime = 0;
		Vector m_vecOrigin = Vector(0, 0, 0);

		int m_nFlags = 0;
		CBaseEntity* player = nullptr;
		Vector m_vAbsOrigin = Vector(0, 0, 0);
		bool m_Dormant = false;
	};
	std::array<SoundPlayer, 65> m_cSoundPlayers;
	CUtlVector<SndInfo_t> m_utlvecSoundBuffer;
	CUtlVector<SndInfo_t> m_utlCurSoundList;
	std::vector<SoundPlayer> m_arRestorePlayers;
};

extern CServerSounds g_ServerSounds;