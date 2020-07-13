#pragma once
#include "GameMemory.h"

class ICollideable
{
public:
	virtual void pad0();
	virtual const Vector& OBBMins() const;
	virtual const Vector& OBBMaxs() const;
	QAngle GetCollisionAngles()
	{
		return ((QAngle(__thiscall*)(void))ReadInt(ReadInt((uintptr_t)this) + m_dwGetCollisionAngles))();
	}
};