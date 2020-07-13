#pragma once
#include "IMaterial.h"

class CStudioRenderContext
{
public:
	void SetColorModulation(const float* pColor)
	{
		typedef void(__thiscall* SetColorModulation_t)(void*, const float*);
		GetVFunc<SetColorModulation_t>(this, 27)(this, pColor);
	}

	void SetAlphaModulation(float alpha)
	{
		typedef void(__thiscall* SetAlphaModulation_t)(void*, float);
		GetVFunc<SetAlphaModulation_t>(this, 28)(this, alpha);
	}

	void ForcedMaterialOverride(IMaterial* mat, OverrideType_t pOverride = OVERRIDE_NORMAL, int idk = 0)
	{
		typedef void(__thiscall* ForcedMaterialOverride_t)(void*, IMaterial*, OverrideType_t, int);
		GetVFunc<ForcedMaterialOverride_t>(this, 33)(this, mat, pOverride, idk);
	}
};
