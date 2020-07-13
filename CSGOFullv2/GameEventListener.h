/// listener header file
#pragma once
#include "Interfaces.h"
#include <string>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class IGameEventVisitor2;
class CGameEventDescriptor;
class IGameEventVisitor2;

class CGameEvent
{
public:
	virtual ~CGameEvent();
	virtual const char* GetName(void)const;
	virtual bool  IsReliable(void)const;
	virtual bool  IsLocal(void)const;
	virtual bool  IsEmpty(const char *keyName = NULL)const;
	virtual bool  GetBool(const char *keyName = NULL, bool defaultValue = false)const;
	virtual int  GetInt(const char *keyName = NULL, int defaultValue = 0)const;
	virtual unsigned long long  GetUint64(const char *keyName = NULL, unsigned long long defaultValue = 0)const;
	virtual float GetFloat(const char *keyName = NULL, float defaultValue = 0.0f)const;
	virtual const char* GetString(const char *keyName = NULL, const char *defaultValue = "")const;
	virtual const wchar_t* GetWString(const char *keyName = NULL, const wchar_t *defaultValue = L"")const;
	virtual void* GetPtr(const char *keyName = NULL)const;
	virtual void SetBool(const char *keyName, bool value);
	virtual void SetInt(const char *keyName, int value);
	virtual void SetUint64(char const*keyName, unsigned long long value);
	virtual void SetFloat(const char *keyName, float value);
	virtual void SetString(const char *keyName, char const* value);
	virtual void SetWString(const char *keyName, wchar_t const* value);
	virtual void SetPtr(const char *keyName, void const* value);
	virtual bool ForEventData(IGameEventVisitor2 * value)const;

	CGameEventDescriptor	*m_pDescriptor;
	KeyValues				*m_pDataKeys;
};

class IGameEventListener2
{
public:
	virtual ~IGameEventListener2(void) {}

	virtual void FireGameEvent(CGameEvent*) = 0;

	virtual int IndicateEventHandling(void) {
		return 0x2A;
	}
};

class IGameEventManager2
{
public:
	bool AddListener(IGameEventListener2* listener, std::string name, bool serverSide);
	void RemoveListener(IGameEventListener2* listener);
	bool FireEventClientSide(CGameEvent* gameEvent);
};


typedef void(*EventFunction)(CGameEvent*);
class CGameEventListener : public IGameEventListener2
{
private:
	char* eventName;
	EventFunction gameEventFunction;
public:
	CGameEventListener(const char* eventName, EventFunction gameEventFunction, bool serverSide);

	
	virtual void FireGameEvent(CGameEvent* gameEvent)
	{
		//if (LevelIsLoaded && LocalPlayer && LocalPlayer.Entity->GetAlive() && Interfaces::EngineClient->IsInGame())
		{
			this->gameEventFunction(gameEvent);
		}
	}
};