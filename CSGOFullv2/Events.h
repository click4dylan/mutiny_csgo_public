#pragma once
#include "GameEventListener.h"
#include "Includes.h"
#include <vector>

void CreateEventListeners();

void GameEvent_PlayerHurt(CGameEvent* gameEvent);
void GameEvent_BulletImpact(CGameEvent* gameEvent);
void GameEvent_ItemPurchase(CGameEvent* gameEvent);
void GameEvent_RoundStart(CGameEvent* gameEvent);
void GameEvent_PlayerDeath(CGameEvent* gameEvent);
void GameEvent_SwitchTeam(CGameEvent* gameEvent);
void GameEvent_StartHalftime(CGameEvent* gameEvent);
void GameEvent_RoundMVP(CGameEvent* gameEvent);
void GameEvent_RoundEnd(CGameEvent* gameEvent);
void GameEvent_BombExploded(CGameEvent* gameEvent);
void GameEvent_WeaponFire(CGameEvent* gameEvent);

struct GameEvent_PlayerHurt_t
{
	int targetuserid;
	int attackeruserid;
	int hitgroup;
	int dmg_health;
	int health;
	std::string weapon;
};

struct GameEvent_Impact_t
{
	Vector impactpos;
	int attackeruserid;
};

struct GameEvent_PlayerDeath_t
{
	int user_id;
	int attacker_id;
	int assister_id;
};

extern void GameEvent_PlayerHurt_ProcessQueuedEvent(const GameEvent_PlayerHurt_t &gameEvent);
extern void GameEvent_PlayerHurt_PreProcessQueuedEvent(const GameEvent_PlayerHurt_t &gameEvent);
extern void GameEvent_Impact_ProcessQueuedEvent(const GameEvent_Impact_t &gameEvent);
extern void GameEvent_PlayerDeath_ProcessQueuedEvent(const GameEvent_PlayerDeath_t& gameEvent);

extern std::vector<GameEvent_PlayerHurt_t> g_GameEvent_PlayerHurt_Queue;
extern std::vector<GameEvent_Impact_t> g_GameEvent_Impact_Queue;
extern std::vector<GameEvent_PlayerDeath_t> g_GameEvent_PlayerDeath_Queue;

inline CBaseEntity* GetPlayerEntityFromEvent(CGameEvent* gameEvent)
{
	//decrypts(0)
	int player = Interfaces::EngineClient->GetPlayerForUserID(gameEvent->GetInt(XorStr("userid"), -1));
	//encrypts(0)
	return Interfaces::ClientEntList->GetBaseEntity(player);
}

inline int GetAttackerUserID(CGameEvent* gameEvent)
{
	//decrypts(0)
	int attacker = gameEvent->GetInt(XorStr("attacker"), -1);
	//encrypts(0)
	return attacker;
}

inline int GetHitgroupFromEvent(CGameEvent* gameEvent)
{
	//decrypts(0)
	int hitgroup = gameEvent->GetInt(XorStr("hitgroup"), -1);
	//encrypts(0)
	return hitgroup;
}

inline int GetDamageFromEvent(CGameEvent* gameEvent)
{
	//decrypts(0)
	int dmg = gameEvent->GetInt(XorStr("dmg_health"), -1);
	//encrypts(0)
	return dmg;
}

inline int GetHealthFromEvent(CGameEvent* gameEvent)
{
	//decrypts(0)
	int health = gameEvent->GetInt(XorStr("health"), -1);
	//encrypts(0)
	return health;
}

class CShotrecord;
extern void FillImpactTempEntResultsAndResolveFromPlayerHurt(CBaseEntity* EntityHit, CPlayerrecord* pCPlayer, CShotrecord* shotrecord, bool CalledFromQueuedEventProcessor);
extern bool g_bBombExploded;