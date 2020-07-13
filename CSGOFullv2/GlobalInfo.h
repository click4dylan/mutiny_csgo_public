#pragma once
#include "Includes.h"

struct sScreenSize
{
	int Width  = 0;
	int Height = 0;
};

extern std::mutex g_SpreadMutex;

class CGlobalInfo
{
public:
	CBaseEntity* m_pC4   = nullptr;
	float m_flDefuseTime = 0.f;
	sScreenSize ScreenSize;

	float m_flUnchokedYaw = 0.f;
	float m_flChokedYaw = 0.f;

	int m_iServerTickrate = 0;

	bool m_bShouldSlide = false;

	IMaterial* GlassMaterial = nullptr;
	IMaterial* LitMaterial   = nullptr;
	IMaterial* UnlitMaterial = nullptr;

	std::vector< Vector > m_Spread;

	bool m_bNextShouldChoke = false;
	bool m_bForceSend = false;
	bool m_bShouldChoke		= false;
	bool m_bLastShouldChoke = false;
	int  m_iNumCommandsToChoke = 0;
	bool LevelisLoaded = false;
	int m_iNumFakeWeaponsGenerated = 0;

	bool UsingForwardTrack = false;

	std::list<int>m_SpectatorList;
	std::mutex m_PredictionMutex;

	void SetNextShouldChoke(bool choke) { m_bNextShouldChoke = choke; }
	bool ShouldChokeNext() const { return m_bNextShouldChoke; }
	void SetForceSend(bool send) { m_bForceSend = send; }
	bool IsForcedToSend() const { return m_bForceSend; }
	void SetShouldChoke(bool choke) { m_bShouldChoke = choke; }
	bool ShouldChoke() const { return m_bShouldChoke; }
	int GetNumCommandsToChoke() const { return m_iNumCommandsToChoke; }
	void HandleChoke();
};

extern CGlobalInfo g_Info;
