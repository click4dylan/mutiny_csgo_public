#pragma once
#include "misc.h"
class ConVar
{
public:
	void SetValue(const char *value);
	void SetValue(float value);
	void SetValue(int value);
	void SetValue(Color value);
	void InternalSetString(const char* str);
	char* GetName();
	char* GetDefault();

	char pad_0x0000[0x4]; //0x0000
	ConVar* pNext; //0x0004 
	__int32 bRegistered; //0x0008 
	const char* pszName; //0x000C 
	const char* pszHelpString; //0x0010 
	__int32 nFlags; //0x0014 
	char pad_0x0018[0x4]; //0x0018
	ConVar* pParent; //0x001C 
	char* pszDefaultValue; //0x0020 
	char* strString; //0x0024 
	__int32 StringLength; //0x0028 
	float fValue; //0x002C 
	__int32 nValue; //0x0030 
	__int32 bHasMin; //0x0034 
	float fMinVal; //0x0038 
	__int32 bHasMax; //0x003C 
	float fMaxVal; //0x0040 
	void* fnChangeCallback; //0x0044 

	float GetFloat();
	int GetInt();
	bool GetBool();
	char* GetString();
};//Size=0x0048

class SpoofedConvar {
public:
	SpoofedConvar();
	SpoofedConvar(const char* szCVar);
	SpoofedConvar(ConVar* pCVar);

	~SpoofedConvar();

	bool           IsSpoofed();
	void           Spoof();

	void           SetFlags(int flags);
	int            GetFlags();

	void           SetBool(bool bValue);
	void           SetInt(int iValue);
	void           SetFloat(float flValue);
	void           SetString(const char* szValue);

private:
	ConVar* m_pOriginalCVar = nullptr;
	ConVar* m_pDummyCVar = nullptr;

	char m_szDummyName[128];
	char m_szDummyValue[128];
	char m_szOriginalName[128];
	char m_szOriginalValue[128];
	int m_iOriginalFlags;
};

class MinspecCvar
{
public:
	MinspecCvar(const char* szCVar, const char* newname, float newvalue);
	~MinspecCvar();

	bool ValidCvar();
	void Spoof();

	template<typename T>
	void SetValue(T value);

	int	GetInt();
	float GetFloat();
	const char* GetString();
private:
	ConVar* m_pConVar;

	const char* m_szOriginalName;
	const char* m_szReplacementName;
	float m_OriginalValue;
	float m_newvalue;
};

template<typename T>
inline void MinspecCvar::SetValue(T value)
{
	m_pConVar->SetValue(value);
}