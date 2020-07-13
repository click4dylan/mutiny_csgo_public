#pragma once
#include "Includes.h"

class CRemovals
{
public:
	void DoNoRecoil(QAngle& Angles) const;
	void AddRecoil(QAngle& Angles) const;
	void DoNoVisRecoil();
	void DoNoSpread();
	void DoNoFlash();
	void DoNoSmoke(bool force = false);
};

extern CRemovals g_Removals;