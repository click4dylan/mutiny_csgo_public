#pragma once

class GameTypes
{
public:
	virtual ~GameTypes();
	virtual bool Initialize(bool);
	virtual bool IsInitialized(void)const;
	virtual char SetGameTypeAndMode(char const*, char const*);
	virtual char GetGameTypeAndModeFromAlias(char const*, int &, int &);
	virtual char SetGameTypeAndMode(int, int);
	virtual void* SetAndParseExtendedServerInfo(void *); //KeyValues*
	virtual void CheckShouldSetDefaultGameModeAndType(char const*);
	virtual int	GetCurrentGameType(void)const;
	virtual int GetCurrentGameMode(void)const;
	virtual const char*	GetCurrentMapName(void);
	virtual int GetCurrentGameTypeNameID(void);
	virtual int GetCurrentGameModeNameID(void);
	virtual char ApplyConvarsForCurrentMode(bool);
	virtual int DisplayConvarsForCurrentMode(void);
	virtual int GetWeaponProgressionForCurrentModeCT(void);
	virtual int GetWeaponProgressionForCurrentModeT(void);
	virtual int GetNoResetVoteThresholdForCurrentModeCT(void);
	virtual int	GetNoResetVoteThresholdForCurrentModeT(void);
	virtual int GetGameTypeFromInt(int);
	virtual int GetGameModeFromInt(int, int);
	virtual char GetGameModeAndTypeIntsFromStrings(char const*, char const*, int &, int &);
	virtual char GetGameModeAndTypeNameIdsFromStrings(char const*, char const*, char const*&, char const*&);
	virtual bool CreateOrUpdateWorkshopMapGroup(char const*, int const&); //CUtlStringList const&
	virtual bool IsWorkshopMapGroup(char const*);
	virtual int GetRandomMapGroup(char const*, char const*);
	virtual int GetFirstMap(char const*);
	virtual int GetRandomMap(char const*);
	virtual int GetNextMap(char const*, char const*);
	virtual int GetMaxPlayersForTypeAndMode(int, int);
	virtual bool IsValidMapGroupName(char const*);
	virtual bool IsValidMapInMapGroup(char const*, char const*);
	virtual bool IsValidMapGroupForTypeAndMode(char const*, char const*, char const*);
	virtual bool ApplyConvarsForMap(char const*, bool);
	virtual char GetMapInfo(char const*, unsigned int &);
	virtual int GetTModelsForMap(char const*);
	virtual int GetCTModelsForMap(char const*);
	virtual int GetHostageModelsForMap(char const*);
	virtual int GetDefaultGameTypeForMap(char const*);
	virtual int GetDefaultGameModeForMap(char const*);
	virtual int GetTViewModelArmsForMap(char const*);
	virtual int GetCTViewModelArmsForMap(char const*);
	virtual int GetRequiredAttrForMap(char const*);
	virtual int GetRequiredAttrValueForMap(char const*);
	virtual int GetRequiredAttrRewardForMap(char const*);
	virtual int GetRewardDropListForMap(char const*);
	virtual int GetMapGroupMapList(char const*);
	virtual int GetRunMapWithDefaultGametype(void);
	virtual void SetRunMapWithDefaultGametype(bool);
	virtual char GetLoadingScreenDataIsCorrect(void);
	virtual void SetLoadingScreenDataIsCorrect(bool);
	virtual char SetCustomBotDifficulty(int);
	virtual int GetCustomBotDifficulty(void);
	virtual int GetCurrentServerNumSlots(void);
	virtual int GetCurrentServerSettingInt(char const*, int);
	virtual char GetGameTypeFromMode(char const*, char const*&);
	virtual char LoadMapEntry(void *); //KeyValues*
};

extern GameTypes **gametypes;

bool IsPlayingGuardian();