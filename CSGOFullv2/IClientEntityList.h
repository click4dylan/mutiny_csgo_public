#pragma once

class IClientEntityList {
public:
	enum indices : size_t {
		ONENTITYCREATED = 0,
		ONENTITYDELETED = 1,
	};
};

class IHandleEntity;
class CBaseHandle;

using OnCreateEntityFn = void(__thiscall*)(void*, IHandleEntity*, CBaseHandle);
extern OnCreateEntityFn oOnAddEntity;
using OnDeleteEntityFn = void(__thiscall*)(void*, IHandleEntity*, CBaseHandle);
extern OnDeleteEntityFn oOnRemoveEntity;