#include "imdlcache.h"



IMaterial* CMaterialSystem::FindMaterial(char const* pMaterialName, const char *pTextureGroupName, bool complain, const char *pComplainPrefix)
{
	typedef IMaterial*(__thiscall* OriginalFn)(void*, char const* pMaterialName, const char *pTextureGroupName, bool complain, const char *pComplainPrefix);
	return GetVFunc<OriginalFn>(this, 84)(this, pMaterialName, pTextureGroupName, complain, pComplainPrefix);
}