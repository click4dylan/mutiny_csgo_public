#include "precompiled.h"
#include "GameEventListener.h"
#include "EncryptString.h"
//////////////////////////////////////
//listener cpp file
////////////////////////////////

bool IGameEventManager2::AddListener(IGameEventListener2* listener, std::string name, bool serverSide)
{
	typedef bool(__thiscall* OriginalFn)(void*, IGameEventListener2*, const char*, bool);
	return GetVFunc<OriginalFn>(this, 3)(this, listener, name.c_str(), serverSide);
}

void IGameEventManager2::RemoveListener(IGameEventListener2* listener)
{
	typedef void(__thiscall* OriginalFn)(void*, IGameEventListener2*);
	GetVFunc<OriginalFn>(this, 5)(this, listener);
}

bool IGameEventManager2::FireEventClientSide(CGameEvent* gameEvent)
{
	typedef bool(__thiscall* OriginalFn)(void*, CGameEvent*);
	return GetVFunc<OriginalFn>(this, 8)(this, gameEvent);
}

CGameEventListener::CGameEventListener(const char* eventName, EventFunction gameEventFunction, bool serverSide)
{
	this->eventName = (char*)eventName;
	this->gameEventFunction = gameEventFunction;
	Interfaces::GameEventManager->AddListener(this, this->eventName, serverSide);
}