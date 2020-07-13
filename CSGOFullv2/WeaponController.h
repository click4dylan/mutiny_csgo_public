#pragma once
#include "Includes.h"
#include "AutoWall.h"
#include "Targetting.h"

class CWeaponController
{
protected:
	bool m_bShouldFire = false;
	bool m_bRevolverWillFire = false;
public:
	void OnCreateMove();

	bool RevolverWillFire() const { return m_bRevolverWillFire; }
	bool WeaponCanFire(int buttons, int* did_shoot = nullptr, float curtime = Interfaces::Globals->curtime) const;
	bool ShouldSetPostPoneFireReadyTime(CBaseCombatWeapon *weapon, int buttons) const;
	void AutoCockRevolver();
	bool ShouldFire() const { return m_bShouldFire; }
	void SetShouldFire(bool ShouldFire) { m_bShouldFire = ShouldFire; }
	void FireBullet(bool secondary = false);
	bool ShouldDelayShot();
	int WeaponDidFire(int buttons, float curtime = Interfaces::Globals->curtime);
};

extern CWeaponController g_WeaponController;