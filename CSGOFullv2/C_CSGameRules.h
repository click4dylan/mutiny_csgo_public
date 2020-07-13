#pragma once

class edict_t;
class KeyValues;

#include "entitydefines.h"

class C_BaseCombatWeapon;
class CViewVectors;
class QAngle;


class C_CSGameRules
{
public:
	virtual const char *Name(void);
	virtual char Init(void);
	virtual void PostInit(void);
	virtual void Shutdown(void);
	virtual void LevelInitPreEntity(void);
	virtual void LevelInitPostEntity(void);
	virtual void LevelShutdownPreEntity(void);
	virtual void LevelShutdownPostEntity(void);
	virtual void OnSave(void);
	virtual void OnRestore(void);
	virtual void SafeRemoveIfDesired(void);
	virtual char IsPerFrame(void);
	virtual ~C_CSGameRules();
	virtual void PreRender(void);
	virtual void Update(float);
	virtual void PostRender(void);
	virtual bool Damage_IsTimeBased(int);
	virtual bool Damage_ShouldGibCorpse(int);
	virtual bool Damage_ShowOnHUD(int);
	virtual bool Damage_NoPhysicsForce(int);
	virtual bool Damage_ShouldNotBleed(int);
	virtual int Damage_GetTimeBased(void);
	virtual int Damage_GetShouldGibCorpse(void);
	virtual int Damage_GetShowOnHud(void);
	virtual int Damage_GetNoPhysicsForce(void);
	virtual int Damage_GetShouldNotBleed(void);
	virtual int SwitchToNextBestWeapon(C_BaseCombatCharacter *, C_BaseCombatWeapon *);
	virtual C_BaseCombatWeapon *GetNextBestWeapon(C_BaseCombatCharacter *, C_BaseCombatWeapon *);
	virtual bool ShouldCollide(int, int);
	virtual int DefaultFOV(void);
	virtual CViewVectors *GetViewVectors(void)const;
	virtual float GetAmmoDamage(C_BaseEntity *, C_BaseEntity *, int);
	virtual float GetDamageMultiplier(void);
	virtual bool IsMultiplayer(void);
	virtual const unsigned char* GetEncryptionKey(void);
	virtual bool InRoundRestart(void);
	virtual bool CheckAchievementsEnabled(int);
	virtual void RegisterScriptFunctions(void);
	virtual void ClientCommandKeyValues(edict_t *, KeyValues *);
	virtual bool IsConnectedUserInfoChangeAllowed(C_BasePlayer *);
	virtual bool IsBonusChallengeTimeBased(void);
	virtual bool AllowThirdPersonCamera(void);
	virtual int GetGameTypeName(void);
	virtual int GetGameType(void);
	virtual bool ForceSplitScreenPlayersOnToSameTeam(void);
	virtual bool IsTopDown(void);
	virtual QAngle* GetTopDownMovementAxis(void);
	virtual int GetMaxHumanPlayers(void)const;
	virtual int GetCaptureValueForPlayer(C_BasePlayer *);
	virtual bool TeamMayCapturePoint(int, int);
	virtual bool PlayerMayCapturePoint(C_BasePlayer *, int, char *, int);
	virtual bool PlayerMayBlockPoint(C_BasePlayer *, int, char *, int);
	virtual bool PointsMayBeCaptured(void);
	virtual void SetLastCapPointChanged(int);
	virtual float GetRoundRestartTime(void);
	virtual bool IsGameRestarting(void);
	virtual bool IgnorePlayerKillCommand(void)const;
	virtual int GetNextRespawnWave(int, C_BasePlayer *);

	bool IsFreezePeriod();
	bool IsValveServer();
	void* GetSurvivalGamerules();
	void* GetCustomizedGameRules();

};

extern C_CSGameRules **g_pGameRules;
inline C_CSGameRules* GetGamerules() { return *g_pGameRules; }