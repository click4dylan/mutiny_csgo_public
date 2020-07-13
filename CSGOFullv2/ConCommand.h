#pragma once
#include "utlvector.h"
#include "UtlString.hpp"

struct CCommand
{
	int m_nArgc;
	int m_nArgv0Size;
	char m_pArgSBuffer[512];
	char m_pArgvBuffer[512];
	const char *m_ppArgv[64];

	const char *Arg(int nIndex) const
	{
		if (nIndex < 0 || nIndex >= m_nArgc)
			return "";
		return m_ppArgv[nIndex];
	};

	const char *operator[](int nIndex) const
	{
		return Arg(nIndex);
	};
};

typedef void(*FnCommandCallbackVoid_t)(void);
typedef void(*FnCommandCallback_t)(const CCommand &command);
#define COMMAND_COMPLETION_MAXITEMS		64
#define COMMAND_COMPLETION_ITEM_LENGTH	64
typedef int(*FnCommandCompletionCallback)(const char *partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);

class ConCommandBase;

class ICommandCallback
{
public:
	virtual void CommandCallback(const CCommand &command) = 0;
};

class ICommandCompletionCallback
{
public:
	virtual int  CommandCompletionCallback(const char *pPartial, CUtlVector< CUtlString > &commands) = 0;
};

class IConCommandBaseAccessor
{
public:
	// Flags is a combination of FCVAR flags in cvar.h.
	// hOut is filled in with a handle to the variable.
	virtual bool RegisterConCommandBase(ConCommandBase *pVar) = 0;
};

class ConCommand
{
public:
	ConCommand(const char *pName, /*FnCommandCallbackV1_t*/FnCommandCallbackVoid_t callback, const char *pHelpString = 0, int flags = 0, FnCommandCompletionCallback completionFunc = 0);

private:
	char pad_0x00[0x20];

private:
	char m_bCallbackType;
};

enum CommandCallbackTypes
{
	TYPE_VERSION1 = 1, //legacy
	TYPE_VERSION1_ARG = 2,
	TYPE_VERSION2 = 4
};

extern ConCommand* RegisterConCommand(const char* name, void* oncalled, void* oncomplete);
extern void Command_SDR(const CCommand& cmd);
extern void Command_SetRelay(const CCommand& cmd);
extern void Command_ResetCookie(const CCommand& cmd);
extern void Command_RetryValve(const CCommand& cmd);
extern std::string g_DesiredRelayCluster;