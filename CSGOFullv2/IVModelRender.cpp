#include "precompiled.h"
#include "IVModelRender.h"

// Model Render Code

void IVModelRender::ForcedMaterialOverride(IMaterial *mat)
{
	typedef void(__thiscall *OriginalFn)(void *, IMaterial *, int, int);
	GetVFunc<OriginalFn>(this, 1)(this, mat, 0, 0);
}

bool IVModelRender::IsForcedMaterialOverride()
{
	typedef bool(__thiscall *OriginalFn)(void*);
	return GetVFunc<OriginalFn>(this, 2)(this);
}

void IVModelRender::DrawModelExecute(void* ctx, void *state, const ModelRenderInfo_t &pInfo, matrix3x4_t *pCustomBoneToWorld)
{
	typedef void(__thiscall* OriginalFn)(void*, void* ctx, void *state, const ModelRenderInfo_t &pInfo, matrix3x4_t *pCustomBoneToWorld);
	return GetVFunc<OriginalFn>(this, 21)(this, ctx, state, pInfo, pCustomBoneToWorld);
}