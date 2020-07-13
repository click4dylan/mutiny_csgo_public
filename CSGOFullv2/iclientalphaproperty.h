#pragma once
#include <float.h>

class IClientUnknown;

// Rendering constants
// if this is changed, update common/MaterialSystem/Sprite.cpp
enum RenderMode_t
{
	kRenderNormal = 0,		// src
	kRenderTransColor,		// c*a+dest*(1-a)
	kRenderTransTexture,	// src*a+dest*(1-a)
	kRenderGlow,			// src*a+dest -- No Z buffer checks -- Fixed size in screen space
	kRenderTransAlpha,		// src*srca+dest*(1-srca)
	kRenderTransAdd,		// src*a+dest
	kRenderEnvironmental,	// not drawn, used for environmental effects
	kRenderTransAddFrameBlend, // use a fractional frame value to blend between animation frames
	kRenderTransAlphaAdd,	// src + dest*(1-a)
	kRenderWorldGlow,		// Same as kRenderGlow but not fixed size in screen space
	kRenderNone,			// Don't render.

	kRenderModeCount,		// must be last
};

enum RenderFx_t
{
	kRenderFxNone = 0,
	kRenderFxPulseSlow,
	kRenderFxPulseFast,
	kRenderFxPulseSlowWide,
	kRenderFxPulseFastWide,

	kRenderFxFadeSlow,
	kRenderFxFadeFast,
	kRenderFxSolidSlow,
	kRenderFxSolidFast,
	kRenderFxStrobeSlow,

	kRenderFxStrobeFast,
	kRenderFxStrobeFaster,
	kRenderFxFlickerSlow,
	kRenderFxFlickerFast,
	kRenderFxNoDissipation,

	kRenderFxFadeOut,
	kRenderFxFadeIn,
	kRenderFxPulseFastWider,
	kRenderFxGlowShell,			// Glowing Shell

	kRenderFxMax
};

enum ClientAlphaDistanceFadeMode_t
{
	CLIENT_ALPHA_DISTANCE_FADE_USE_CENTER = 0,
	CLIENT_ALPHA_DISTANCE_FADE_USE_NEAREST_BBOX,

	CLIENT_ALPHA_DISTANCE_FADE_MODE_COUNT,
};


class IClientAlphaProperty
{
public:
	// Gets at the containing class...
	virtual IClientUnknown*	GetIClientUnknown() = 0;

	// Sets a constant alpha modulation value
	virtual void SetAlphaModulation(unsigned char a) = 0;

	// Sets an FX function
	// NOTE: kRenderFxFadeSlow, kRenderFxFadeFast, kRenderFxSolidSlow, kRenderFxSolidFast all need a start time only.
	// kRenderFxFadeIn/kRenderFxFadeOut needs start time + duration
	// All other render fx require no parameters
	virtual void SetRenderFX(RenderFx_t nRenderFx, RenderMode_t nRenderMode, float flStartTime = FLT_MAX, float flDuration = 0.0f) = 0;

	// Sets fade parameters
	virtual void SetFade(float flGlobalFadeScale, float flDistFadeStart, float flDistFadeEnd) = 0;

	// Sets desync offset, used to make sine waves not match
	virtual void SetDesyncOffset(int nOffset) = 0;

	// Allows the owner to override alpha.
	// The method IClientRenderable::OverrideAlphaModulation will be called
	// to allow the owner to optionally return a different alpha modulation
	virtual void EnableAlphaModulationOverride(bool bEnable) = 0;

	// Allows the owner to override projected shadow alpha.
	// The method IClientRenderable::OverrideShadowAlphaModulation will be called
	// to allow the owner to optionally return a different alpha modulation for the shadow
	virtual void EnableShadowAlphaModulationOverride(bool bEnable) = 0;

	// Sets the distance fade mode
	virtual void SetDistanceFadeMode(ClientAlphaDistanceFadeMode_t nFadeMode) = 0;
};

class CClientAlphaProperty : public IClientAlphaProperty
{
	// Inherited from IClientAlphaProperty
public:
	virtual IClientUnknown*	GetIClientUnknown();
	virtual void SetAlphaModulation(unsigned char a);
	virtual void SetRenderFX(RenderFx_t nRenderFx, RenderMode_t nRenderMode, float flStartTime = FLT_MAX, float flDuration = 0.0f);
	virtual void SetFade(float flGlobalFadeScale, float flDistFadeMinDist, float flDistFadeMaxDist);
	virtual void SetDesyncOffset(int nOffset);
	virtual void EnableAlphaModulationOverride(bool bEnable);
	virtual void EnableShadowAlphaModulationOverride(bool bEnable);
	virtual void SetDistanceFadeMode(ClientAlphaDistanceFadeMode_t nFadeMode);

};