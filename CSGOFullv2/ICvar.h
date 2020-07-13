#pragma once

class ConVar;

enum InitReturnVal_t
{
	INIT_FAILED = 0,
	INIT_OK,

	INIT_LAST_VAL,
};

enum AppSystemTier_t
{
	APP_SYSTEM_TIER0 = 0,
	APP_SYSTEM_TIER1,
	APP_SYSTEM_TIER2,
	APP_SYSTEM_TIER3,

	APP_SYSTEM_TIER_OTHER,
};

struct AppSystemInfo_t
{
	const char* m_pModuleName;
	const char* m_pInterfaceName;
};

typedef void* (*CreateInterfaceFn)(const char* szName, int iReturn);

class IAppSystem
{
public:
	// Here's where the app systems get to learn about each other 
	virtual bool Connect(CreateInterfaceFn factory) = 0;  // 0
	virtual void Disconnect() = 0;  // 1

									// Here's where systems can access other interfaces implemented by this object
									// Returns NULL if it doesn't implement the requested interface
	virtual void* QueryInterface(const char* pInterfaceName) = 0;  // 2

																   // Init, shutdown
	virtual InitReturnVal_t Init() = 0;  // 3
	virtual void Shutdown() = 0;  // 4

								  // Returns all dependent libraries
	virtual const AppSystemInfo_t* GetDependencies() = 0;  // 5

														   // Returns the tier
	virtual AppSystemTier_t GetTier() = 0;  // 6

											// Reconnect to a particular interface
	virtual void Reconnect(CreateInterfaceFn factory, const char* pInterfaceName) = 0;  // 7

	// Returns whether or not the app system is a singleton
	virtual bool IsSingleton() = 0; //8
};

struct CVarDLLIdentifier_t;

class ICVar : public IAppSystem
{
public:
	virtual void			AllocateDLLIdentifier() = 0;
	virtual void			RegisterConCommand(ConVar *pCommandBase) = 0;
	virtual void			UnregisterConCommand(ConVar *pCommandBase) = 0;
	virtual void			UnregisterConCommands(CVarDLLIdentifier_t id) = 0;
	virtual const char*		GetCommandLineValue(const char *pVariableName) = 0;
	virtual ConVar *FindCommandBase(const char *name) = 0;
	virtual const ConVar *FindCommandBase(const char *name) const = 0;
	virtual ConVar			*FindVar(const char *var_name) = 0;
	virtual const ConVar	*FindVar(const char *var_name) const = 0;
	virtual ConVar*		*FindCommand(const char *name) = 0;
	virtual const ConVar* *FindCommand(const char *name) const = 0;
	virtual ConVar	*GetCommands(void) = 0;
	virtual const ConVar *GetCommands(void) const = 0;

	virtual void			InstallGlobalChangeCallback(int callback) = 0;
	virtual void			RemoveGlobalChangeCallback(int callback) = 0;
	virtual void			CallGlobalChangeCallbacks(ConVar *var, const char *pOldString, float flOldValue) = 0;

	virtual void			InstallConsoleDisplayFunc(void* pDisplayFunc) = 0;
	virtual void			RemoveConsoleDisplayFunc(void* pDisplayFunc) = 0;
	virtual void			ConsoleColorPrintf(const /*Color*/int& clr, const char *pFormat, ...) const = 0;
	virtual void			ConsolePrintf(const char *pFormat, ...) const = 0;
	virtual void			ConsoleDPrintf(const char *pFormat, ...) const = 0;

	// Reverts cvars which contain a specific flag
	virtual void			RevertFlaggedConVars(int nFlag) = 0;

	// Method allowing the engine ICvarQuery interface to take over
	// A little hacky, owing to the fact the engine is loaded
	// well after ICVar, so we can't use the standard connect pattern
	virtual void			InstallCVarQuery(void *pQuery) = 0;


	//void RegisterConCommandNew(ConVar *pCommandBase)
	//{
		//return GetVFunc<void(__thiscall*)(void*, ConVar*)>(this, 9)(this, pCommandBase);
	//}
};