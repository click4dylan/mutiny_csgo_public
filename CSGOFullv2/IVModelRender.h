#pragma once
#include "CMaterialSystem.h"

// Model Render Class
class IVModelRender
{
public:
	void DrawModelExecute(void* ctx, void *state, const ModelRenderInfo_t &pInfo, matrix3x4_t *pCustomBoneToWorld = NULL);
	void ForcedMaterialOverride(IMaterial *mat);
	bool IsForcedMaterialOverride();
};