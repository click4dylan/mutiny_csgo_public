#pragma once
#include "misc.h"
#include "dataupdatetypes.h"

class IClientRenderable;
class CBaseEntity;

class C_TEMuzzleFlash
{
	char pad[12];
	Vector		m_vecOrigin_;
	QAngle		m_vecAngles;
	float		m_flScale;
	int			m_nType;
};