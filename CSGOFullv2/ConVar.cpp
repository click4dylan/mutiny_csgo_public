#include "precompiled.h"
#include "ConVar.h"
#include "VTHook.h"
#include "Interfaces.h"
void ConVar::SetValue(const char* value)
{
	typedef void(__thiscall* OriginalFn)(void*, const char*);
	return GetVFunc<OriginalFn>(this, 14)(this, value);
}

void ConVar::SetValue(float value)
{
	typedef void(__thiscall* OriginalFn)(void*, float);
	return GetVFunc<OriginalFn>(this, 15)(this, value);
}

void ConVar::SetValue(int value)
{
	typedef void(__thiscall* OriginalFn)(void*, int);
	return GetVFunc<OriginalFn>(this, 16)(this, value);
}

void ConVar::SetValue(Color value)
{
	typedef void(__thiscall* OriginalFn)(void*, Color);
	return GetVFunc<OriginalFn>(this, 17)(this, value);
}

char* ConVar::GetName()
{
	typedef char*(__thiscall* OriginalFn)(void*);
	return GetVFunc<OriginalFn>(this, 5)(this);
}

char* ConVar::GetDefault()
{
	return pszDefaultValue;
}

float ConVar::GetFloat()
{
	int xored = ReadInt((uintptr_t)&fValue) ^ (int)this;//*reinterpret_cast< int* >(&fValue) ^ (int)this;
	return *reinterpret_cast<float*>(&xored);
}

int ConVar::GetInt()
{
	int xored = ReadInt((uintptr_t)&nValue) ^ (int)this;//*reinterpret_cast< int* >(&fValue) ^ (int)this;
	return xored;
}

bool ConVar::GetBool()
{
	int xored = ReadInt((uintptr_t)&nValue) ^ (int)this;//*reinterpret_cast< int* >(&fValue) ^ (int)this;
	return *reinterpret_cast<bool*>(&xored);
}

char* ConVar::GetString()
{
	int xored = ReadInt((uintptr_t)&nValue) ^ (int)this;
	return reinterpret_cast<char*>(&xored);
}

#if 1
SpoofedConvar::SpoofedConvar() {}

SpoofedConvar::SpoofedConvar(const char* szCVar) {
	m_pOriginalCVar = Interfaces::Cvar->FindVar(szCVar);
	Spoof();
}
SpoofedConvar::SpoofedConvar(ConVar* pCVar) {
	m_pOriginalCVar = pCVar;
	Spoof();
}
SpoofedConvar::~SpoofedConvar() {
	if (IsSpoofed()) {
		DWORD dwOld;
		static int garbage = 0;
		SetFlags(m_iOriginalFlags);
		SetString(m_szOriginalValue);

		VirtualProtect((LPVOID)m_pOriginalCVar->pszName, 128, PAGE_READWRITE, &dwOld);
		garbage = 1;
		strcpy((char*)m_pOriginalCVar->pszName, m_szOriginalName);
		VirtualProtect((LPVOID)m_pOriginalCVar->pszName, 128, dwOld, &dwOld);

		//Unregister dummy cvar
		Interfaces::Cvar->UnregisterConCommand(m_pDummyCVar);
		free(m_pDummyCVar);
		garbage = 0;
		m_pDummyCVar = nullptr;
	}
}
bool SpoofedConvar::IsSpoofed() {
	return m_pDummyCVar != nullptr;
}
char *spoofstr = new char[5]{ 30, 37, 95, 9, 0 }; /*d_%s*/
void SpoofedConvar::Spoof() {
	if (!IsSpoofed() && m_pOriginalCVar) {
		static int garbage2 = 0;
		//Save old name value and flags so we can restore the cvar lates if needed
		m_iOriginalFlags = m_pOriginalCVar->nFlags;
		strcpy(m_szOriginalName, m_pOriginalCVar->pszName);
		strcpy(m_szOriginalValue, m_pOriginalCVar->pszDefaultValue);
		garbage2 = 12;
		DecStr(spoofstr, 4);
		sprintf_s(m_szDummyName, 128, spoofstr, m_szOriginalName);
		EncStr(spoofstr, 4);

		//Create the dummy cvar
		m_pDummyCVar = (ConVar*)malloc(sizeof(ConVar));
		if (!m_pDummyCVar) return;
		garbage2 = 1;
		memcpy(m_pDummyCVar, m_pOriginalCVar, sizeof(ConVar));

		m_pDummyCVar->pNext = nullptr;
		//Register it
		Interfaces::Cvar->RegisterConCommand(m_pDummyCVar);

		//Fix "write access violation" bullshit
		DWORD dwOld;
		VirtualProtect((LPVOID)m_pOriginalCVar->pszName, 128, PAGE_READWRITE, &dwOld);
		garbage2 = 109;
		//Rename the cvar
		strcpy((char*)m_pOriginalCVar->pszName, m_szDummyName);

		VirtualProtect((LPVOID)m_pOriginalCVar->pszName, 128, dwOld, &dwOld);
		garbage2 = 0;
		SetFlags(FCVAR_NONE);
	}
}
void SpoofedConvar::SetFlags(int flags) {
	if (IsSpoofed()) {
		m_pOriginalCVar->nFlags = flags;
	}
}
int SpoofedConvar::GetFlags() {
	return m_pOriginalCVar->nFlags;
}
void SpoofedConvar::SetInt(int iValue) {
	if (IsSpoofed()) {
		m_pOriginalCVar->SetValue(iValue);
	}
}
void SpoofedConvar::SetBool(bool bValue) {
	if (IsSpoofed()) {
		m_pOriginalCVar->SetValue(bValue);
	}
}
void SpoofedConvar::SetFloat(float flValue) {
	if (IsSpoofed()) {
		m_pOriginalCVar->SetValue(flValue);
	}
}
void SpoofedConvar::SetString(const char* szValue) {
	if (IsSpoofed()) {
		m_pOriginalCVar->SetValue(szValue);
	}
}

MinspecCvar::MinspecCvar(const char* szCVar, const char* newname, float newvalue) : m_pConVar(nullptr)
{
	m_pConVar = Interfaces::Cvar->FindVar(szCVar);
	m_newvalue = newvalue;
	m_szReplacementName = newname;
	Spoof();
}

MinspecCvar::~MinspecCvar()
{
	if (ValidCvar())
	{
		Interfaces::Cvar->UnregisterConCommand(m_pConVar);
		m_pConVar->pszName = m_szOriginalName;
		m_pConVar->SetValue(m_OriginalValue);
		Interfaces::Cvar->RegisterConCommand(m_pConVar);
	}
}

bool MinspecCvar::ValidCvar()
{
	return m_pConVar != nullptr;
}
void MinspecCvar::Spoof()
{
	if (ValidCvar())
	{
		Interfaces::Cvar->UnregisterConCommand(m_pConVar);
		m_szOriginalName = m_pConVar->pszName;
		m_OriginalValue = m_pConVar->GetFloat();
		m_pConVar->nFlags = FCVAR_CLIENTDLL;
		m_pConVar->pszName = m_szReplacementName;
		Interfaces::Cvar->RegisterConCommand(m_pConVar);
		m_pConVar->SetValue(m_newvalue);
	}
}

int MinspecCvar::GetInt()
{
	if (ValidCvar()) {
		return m_pConVar->GetInt();
	}
	return 0;
}

float MinspecCvar::GetFloat()
{
	if (ValidCvar()) {
		return m_pConVar->GetFloat();
	}
	return 0.0f;
}

const char* MinspecCvar::GetString()
{
	if (ValidCvar()) {
		return m_pConVar->GetString();
	}
	return nullptr;
}
#endif