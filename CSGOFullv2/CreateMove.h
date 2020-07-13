#pragma once
#include "VTHook.h"
#include "Interfaces.h"
#include "Math.h"
#include "IGameMovement.h"

struct CreateMoveVars_t
{
	DWORD* FramePointer;
	bool bShouldFakeUnDuck;
	bool bShouldFakeDuck;
	struct LastShot_t
	{
		bool didshoot;
		QAngle viewangles;
	} LastShot;
	int iUnduckedChokedCount;
	int iDuckedChokedCount;
	int iQuickSwitchMode;

	CreateMoveVars_t::CreateMoveVars_t()
	{
		bShouldFakeUnDuck = false;
		bShouldFakeDuck = false;
		iUnduckedChokedCount = 0;
		iDuckedChokedCount = 0;
		iQuickSwitchMode = 0;
		LastShot.didshoot = false;
		LastShot.viewangles.Init(0, 0, 0);
	}
};

extern ModifiableUserCmd CurrentUserCmd;
extern ClientFrameStage_t LastFramestageNotifyStage;
extern CreateMoveVars_t CreateMoveVars;