#pragma once
#include "Includes.h"

enum FreestandingType_t : int
{
	FREESTAND_NONE,
	FREESTAND_LEFT,
	FREESTAND_RIGHT,
	FREESTAND_BACK,
};

enum ManualType_t : int
{
	MANUAL_NONE,
	MANUAL_LEFT,
	MANUAL_RIGHT,
	MANUAL_BACK,
};

class CAntiAim
{
public:
	float m_flNextLBYUpdateTime = 0.f;
	bool m_b_lby_after_stopping_already_updated_once = false;

	bool m_bManualSwapDesyncDir = false;
	bool m_bSwayDesyncStyle = false;
	int m_iManualType = 0;
	int m_iFreestandType = 0;

protected:
	bool m_bLBYUpdated = false;
	bool m_bLBYWillUpdate = false;
	bool m_bThisTickIsLBYUpdate = false;
	bool m_bNextTickIsLBYUpdate = false;
	bool m_bAfterNextTickIsLBYUpdate = false;
	bool m_bIsMoving = false;
	bool m_bInAir = false;
	bool m_bFuckupResolver = true;
	bool m_bIsBreakingLBY = false;


	float m_flDesyncBase = 0.f;
	float m_flSideMove = 0.f;
	float m_flBreakYaw = -1337.f;
	float m_flLastDesiredRealYaw = 0.f;
	float m_flLastDesiredFakeYaw = 0.f;

	float WallThickness(Vector from, Vector to, CBaseEntity* skip, CBaseEntity* skip2, Vector& endpos);
	void FreeStanding(float* _FreeStandingYaw, float* _FreeStandingFakeYaw);

public:
	bool ApplyFreestanding(float & base_yaw, CBaseEntity* local_entity, CBaseEntity* closest_enemy = nullptr);
	bool V4Freestanding(float& base_yaw, float& lby);
protected:
	float GetMaxDesyncDelta();
	
	bool ShouldRun();
	void Pitch();
	void YawBase(float& _yaw);
	void YawAdjust(float& _yaw);
	void Experimental();
	void ApplyCorrectAngle(float choked_yaw, float lby_delta);
	void SetYawAndBreakLBY(float desired_yaw, float desired_lby_delta);
public:

	enum Yawbase_e
	{
		BASE_NONE,
		BASE_LOCALVIEW,
		BASE_ATTARGET,
		BASE_FREESTANDING,
		//BASE_MOVEMENTDIR,
		//BASE_FREESTANDING
	};

	enum Yaw_e
	{
		YAW_NONE,
		YAW_180,
		YAW_LEFT,
		YAW_RIGHT,
		YAW_SPIN,
		YAW_FASTSPIN,
		YAW_DYNSIDE,
		YAW_EVADE,
		YAW_DEVASTATE,
		YAW_EXPERIMENTAL,
	};

	enum FakeYaw_e
	{
		FAKEYAW_NONE,
		FAKEYAW_LOCAL,
		FAKEYAW_LEFT,
		FAKEYAW_RIGHT,
		FAKEYAW_SPIN,
		FAKEYAW_FASTSPIN,
		FAKEYAW_DYNSIDE,
		FAKEYAW_EVADE,
	};

	enum Pitch_e
	{
		PITCH_NONE,
		PITCH_UP,
		PITCH_DOWN,
		PITCH_JITTER,
		PITCH_LISPDOWN,
		PITCH_LISPUP,
		PITCH_FAKEDOWN,
		PITCH_FAKEUP,
		PITCH_FAKEZERO
	};

	void PreCreateMove();
	void Run();
	void SlowWalk();
};

extern CAntiAim g_AntiAim;