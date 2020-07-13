#pragma once
#include "Includes.h"

class CUserCmd;

class CAssistance
{
protected:

	void Autostrafe();

public:
	QAngle m_angStrafeAngle = { 0,0,0 };
	QAngle m_angPostStrafe = { 0,0,0 };
	float m_flOldYaw = 0.f;

	void FixButtons(CUserCmd* cmd);
	void BunnyHop();
	void FixMovement();
	void LagCrouch();
	void Teleport();
	void FakeLagOnPeek();
	bool LineGoesThroughSmoke(Vector startPos, Vector endPos);
};

extern CAssistance g_Assistance;