#include "precompiled.h"
#include "WeaponController.h"
#include "LocalPlayer.h"
#include "Aimbot_imi.h"
#include "AutoBone_imi.h"
#include "AntiAim.h"
#include "Events.h"
#include "Eventlog.h"
#include "UsedConvars.h"
#include "Fakelag.h"

#include "Adriel/stdafx.hpp"
#include "Adriel/input.hpp"
#include "Adriel/console.hpp"
#include "Adriel/adr_util.hpp"
#include "TickbaseExploits.h"

CWeaponController g_WeaponController;

/*bool TraceForEnemy(Vector& testpos, const Vector& vecDir, CBaseEntity*& EntityHit, int& hitgroup, int& hitbox, float& dmg, Vector& resultpos)
{
	if (!g_Convars.Ragebot.ragebot_autowall->GetBool())
	{
		EntityHit = Autowall(testpos, LocalPlayer.ShootPosition, LocalPlayer.CurrentWeapon, &dmg, hitgroup, hitbox, nullptr, false, LocalPlayer.Entity, nullptr, false, resultpos);
		if (EntityHit && (dmg >= !g_Convars.Ragebot.ragebot_mindmg->GetFloat() || (float)EntityHit->GetHealth() - dmg <= 0.0f))
			if (EntityHit->IsPlayer() && MTargetting.IsPlayerAValidTarget(EntityHit) && MTargetting.IsHitgroupATarget(hitgroup, EntityHit)) //EntityHit pointer is checked inside the function
			{
				CustomPlayer *pCPlayer = EntityHit->ToCustomPlayer();
				if (pCPlayer->pBacktrackedTick && gLagCompensation.bTickIsValid(pCPlayer->pBacktrackedTick->tickcount))
					return true;
			}
	}
	else
	{
		//TODO FIXME: to use bUseAssumedHit we need to build a batch ray tracer with all players' hitboxes
		trace_t tr;
		UTIL_TraceLine(LocalPlayer.ShootPosition, testpos, MASK_SHOT, LocalPlayer.Entity, &tr);
		CTraceFilter filter;
		filter.pSkip = (IHandleEntity*)LocalPlayer.Entity;
		filter.m_icollisionGroup = COLLISION_GROUP_NONE;
		UTIL_ClipTraceToPlayers(LocalPlayer.ShootPosition, testpos + vecDir * 40.0f, MASK_SHOT, &filter, &tr);
		EntityHit = tr.m_pEnt;
		if (MTargetting.IsPlayerAValidTarget(EntityHit) && EntityHit->GetAlive() && MTargetting.IsHitgroupATarget(tr.hitgroup, EntityHit))
		{
			CustomPlayer *pCPlayer = EntityHit->ToCustomPlayer();
			if (pCPlayer->pBacktrackedTick && gLagCompensation.bTickIsValid(pCPlayer->pBacktrackedTick->tickcount))
			{
				hitgroup = tr.hitgroup;
				hitbox = tr.hitbox;
				dmg = 100; //FIXME: AUTOWALL HERE
				resultpos = tr.endpos;
				return true;
			}
		}
	}
	return false;
}*/

void CWeaponController::OnCreateMove()
{
	m_bShouldFire = false;
	float _backupcurtime = Interfaces::Globals->curtime;
	if (LocalPlayer.Entity)
		Interfaces::Globals->curtime = LocalPlayer.Entity->GetTickBase() * Interfaces::Globals->interval_per_tick;
	AutoCockRevolver();
	Interfaces::Globals->curtime = _backupcurtime;
}

bool ItemPostFrame_ProcessPrimaryAttack(CBaseCombatWeapon *weaponbase, int buttons, bool *set_postpone_fire_ready_time = nullptr, int *fired_bullet = nullptr, bool *can_primary_attack = nullptr, float curtime = Interfaces::Globals->curtime)
{
	CBaseEntity *owner = ((CBaseEntity*)weaponbase)->GetOwner();
	if (!owner)
		return false;

	// (*(this + 0x5B4)()) = C_BaseCombatWeapon::GetMaxClip1
	bool fireonempty = weaponbase->GetFireOnEmpty();
	if (!weaponbase->GetClipOne() || weaponbase->GetMaxClip1() == -1 && !weaponbase->GetReserveAmmoCount(1))// 0x3244 = m_iClip1
		fireonempty = true;//*(weaponbase + 0x3286) = 1;                 // 0x3286 = m_bFireOnEmpty

	if (!(*g_pGameRules)->IsFreezePeriod()                   // !g_pGameRules->IsFreezePeriod()
		&& !owner->IsDefusing()                          // 0x3914 = m_bIsDefusing
		&& !owner->GetPlayerState()                          // 0x3910 = m_iPlayerState
		// (*(*weaponbase + 0x688)) = C_WeaponCSBase::IsFullAuto
		&& (owner->GetShotsFired() <= 0 || weaponbase->IsFullAuto() && weaponbase->GetClipOne())// 0xA370 = m_iShotsFired
		&& !owner->GetWaitForNoAttack())                        // 0x3954 = m_bWaitForNoAttack
	{
		// (*(*weaponbase + 0x758)) = C_WeaponCSBase::IsRevolver
		if (weaponbase->GetItemDefinitionIndex() != WEAPON_REVOLVER)
		{
		FIRE_BULLET:
			if (fired_bullet)
				*fired_bullet = 1;
			if (can_primary_attack)
				*can_primary_attack = true;
			// (*(*weaponbase + 0x514)) = C_BaseCombatWeapon::PrimaryAttack
			//(*(*weaponbase + 0x514))(weaponbase);
			//if (weaponbase->GetLastShotTime() != curtime)// 0x3360 = m_fLastShotTime
			//	weaponbase->SetLastShotTime(curtime);
			// (*(*weaponbase + 0x758)) = C_WeaponCSBase::IsRevolver
			if (weaponbase->GetItemDefinitionIndex() == WEAPON_REVOLVER)
			{
				// (*(*weaponbase + 0x694)) = C_WeaponCSBase::GetCycleTime
				//float cycle = weaponbase->GetCycleTime(1) * 1.7f;
				//float newsecondaryattacktime = curtime + cycle;
				//if (weaponbase->GetNextSecondaryAttack() != newsecondaryattacktime)// 0x321C = m_flNextSecondaryAttack
				//	weaponbase->SetNextSecondaryAttack(newsecondaryattacktime);

				return true;
			}
			return true;
		}

		if (can_primary_attack)
			*can_primary_attack = true;

		//float new_secondary_attack_time = curtime + 0.25f;
		//if (weaponbase->GetNextSecondaryAttack() != new_secondary_attack_time)
		//	weaponbase->SetNextSecondaryAttack(new_secondary_attack_time);

		if (weaponbase->GetActivity() != 208/*ACT_CSGO_FIRE_PRIMARY*/)  //208       // 0x3274 = m_Activity
		{
			//if (weaponbase->GetPostPoneFireReadyTime() != FLT_MAX) // 0x331C = m_flPostponeFireReadyTime
			//	weaponbase->SetPostPoneFireReadyTime(FLT_MAX);
			//C_BaseCombatWeapon::SendWeaponAnim(ACT_CSGO_FIRE_PRIMARY);
			if (set_postpone_fire_ready_time)
				*set_postpone_fire_ready_time = true;
			return false;
		}
		//if (weaponbase->GetMode())               // 0x32EC = m_weaponMode
		//	weaponbase->SetMode(0);
		if (curtime > weaponbase->GetPostPoneFireReadyTime()) // 0x331C = m_flPostponeFireReadyTime
		{
			if (fireonempty)//if (*(weaponbase + 0x3286)) //m_bFireOnEmpty
			{
				//if (weaponbase->GetPostPoneFireReadyTime() != FLT_MAX) // 0x331C = m_flPostponeFireReadyTime
				//	weaponbase->SetPostPoneFireReadyTime(FLT_MAX);
				//float new_time = curtime + 0.5f;
				//if (weaponbase->GetNextSecondaryAttack()) != new_time)
				//	weaponbase->SetNextSecondaryAttack(new_time);
				//if (weaponbase->GetNextPrimaryAttack() != weaponbase->GetNextSecondaryAttack())// 0x3218 = m_flNextPrimaryAttack
				//	weaponbase->SetNextPrimaryAttack(weaponbase->GetNextSecondaryAttack());
			}
			goto FIRE_BULLET;
		}
	}
	return false;
}


bool RunSecondaryAttack(CBaseCombatWeapon * cweaponcsbase, int *fired_bullet = nullptr, float curtime = Interfaces::Globals->curtime)
{
	CBaseEntity *owner = ((CBaseEntity*)cweaponcsbase)->GetOwner();

	if (owner)
	{
		//if (cweaponcsbase->GetClipTwo() != -1 && !cweaponcsbase->GetReserveAmmoCount(2)) // 0x3248 = m_iClip2
		//	*(cweaponcsbase + 0x3286) = 1; //m_bFireOnEmpty

		// C_WeaponCSBase::SecondaryAttack
		//(*(*cweaponcsbase + 0x518))(cweaponcsbase);
		if (fired_bullet)
			*fired_bullet = 2;
		return true;
	}

	if (fired_bullet)
		*fired_bullet = 0;
	return false;
}

bool ItemPostFrame_ProcessSecondaryAttack(CBaseCombatWeapon* cweaponcsbase, int buttons, int *fired_bullet = nullptr, float curtime = Interfaces::Globals->curtime)
{
	if (!cweaponcsbase)
		return false;
	CBaseEntity *owner = ((CBaseEntity*)cweaponcsbase)->GetOwner();
	if (!owner)
		return false;
	// !C_WeaponCSBase::IsRevolver()
	if (cweaponcsbase->GetItemDefinitionIndex() != WEAPON_REVOLVER)
		return RunSecondaryAttack(cweaponcsbase, fired_bullet, curtime);
	// !g_pGameRules->IsFreezePeriod() 
	if (!(*g_pGameRules)->IsFreezePeriod() && !owner->IsDefusing() && !owner->GetPlayerState())  // 0x3914 = m_bIsDefusing | 0x3910 = m_iPlayerState
	{
		bool fireonempty = cweaponcsbase->GetFireOnEmpty();

		if (!cweaponcsbase->GetClipOne() // 0x3244 = m_iClip1
			|| cweaponcsbase->GetMaxClip1() == -1
			&& !cweaponcsbase->GetReserveAmmoCount(1))
		{
			fireonempty = true;  // 0x3286 = m_bFireOnEmpty  //NOTE: game sets the actual value to true and not local variable
		}
		
		//if (cweaponcsbase->GetMode() != 1) // 0x32EC = m_weaponMode
		//	cweaponcsbase->SetMode(1);

		if (!fireonempty)
		{
			//if (cweaponcsbase->GetPostPoneFireReadyTime() != FLT_MAX) // 0x331C = m_flPostponeFireReadyTime
			//	cweaponcsbase->SetPostPoneFireReadyTime(FLT_MAX);
			if (cweaponcsbase->GetActivity() == /*ACT_CSGO_FIRE_PRIMARY*/ 208) // 0x3274 = m_Activity , 208
			{
				//C_BaseCombatWeapon::SendWeaponAnim(ACT_CSGO_FIRE_PRIMARY_OPT_2); //185
				return false;
			}
		}
		if (owner->GetShotsFired() <= 0)  // 0xA370 = m_iShotsFired
		{
			if (!fireonempty)
				return RunSecondaryAttack(cweaponcsbase, fired_bullet, curtime);
			if (cweaponcsbase->GetActivity() != /*ACT_CSGO_FIRE_PRIMARY*/ 208)
			{
				//if (cweaponcsbase->GetPostPoneFireReadyTime() != FLT_MAX) // 0x331C = m_flPostponeFireReadyTime
				//	cweaponcsbase->SetPostPoneFireReadyTime(FLT_MAX);
				//C_BaseCombatWeapon::SendWeaponAnim(ACT_CSGO_FIRE_PRIMARY); //208
				return false;
			}
			if (cweaponcsbase->GetPostPoneFireReadyTime() < curtime) // 0x331C = m_flPostponeFireReadyTime
				return RunSecondaryAttack(cweaponcsbase, fired_bullet, curtime);
		}
	}
	
	return false;
}

//Can actually run primary/secondary attack given these buttons
bool CWeaponController::WeaponCanFire(int buttons, int *did_shoot, float curtime) const
{
	if (did_shoot)
		*did_shoot = 0;
	if (!LocalPlayer.Entity)
		return false;
	CBaseCombatWeapon *weapon = LocalPlayer.Entity->GetWeapon();
	if (!weapon)
		return false;
	CBaseEntity *owner = LocalPlayer.Entity;//((CBaseEntity*)weapon)->GetOwner();
	if (!owner)
		return false;

	//void CBasePlayer::ItemPostFrame()
	if (curtime < owner->GetNextAttack())
		return false;

	if (LocalPlayer.WeaponWillFireBurstShotThisTick)
		return true;
	if (weapon->WeaponHasBurst() && weapon->GetBurstShotsRemaining() > 0 && curtime >= weapon->GetNextBurstShotTime())
		return true;

	// no weapon or knife or empty
	if (weapon->IsEmpty() && !LocalPlayer.WeaponVars.IsKnife)
		return false;

	// is single shot weapon
	if (!(buttons & IN_ATTACK2) && !weapon->IsFullAuto() && (!weapon->WeaponHasBurst() || weapon->GetMode() == 0))
	{
		if (owner->GetShotsFired() != 0)
			return false;
	}

	// rebuilt from game
	auto definitionindex = weapon->GetItemDefinitionIndex();
	bool inzoom = (weapon->IsSniper(true) || definitionindex == WEAPON_AUG || definitionindex == WEAPON_SG556) && ((buttons & IN_ATTACK2) || (buttons & IN_ZOOM));
	bool cocked_revolver = false;
	bool can_primary_attack = false;

	if (!weapon->IsReloading() || curtime < owner->GetNextAttack())
	{
		if (buttons & IN_ATTACK && curtime >= weapon->GetNextPrimaryAttack())
		{
			bool res = ItemPostFrame_ProcessPrimaryAttack(weapon, buttons, nullptr, did_shoot, &can_primary_attack, curtime);
			if (definitionindex == WEAPON_REVOLVER)
				res = can_primary_attack;
			return res;
		}
		else if (inzoom && curtime >= weapon->GetNextSecondaryAttack())
		{
			//ItemPostFrame_ProcessZoomAction
			return true;
		}
		else if (buttons & IN_ATTACK2 && curtime >= weapon->GetNextSecondaryAttack())
		{
			return ItemPostFrame_ProcessSecondaryAttack(weapon, buttons, nullptr, curtime);
		}
		else if (!(buttons & IN_RELOAD) 
			|| weapon->GetMaxClip1() == -1
			|| weapon->IsReloading()
			|| curtime <= weapon->GetNextPrimaryAttack())
		{
			if (!(buttons & (IN_ZOOM | IN_ATTACK2 | IN_ATTACK)))
			{
				//C_WeaponCSBase::ItemPostFrame_ProcessIdleNoAction(v1, csplayer);
			}
		}
		else
		{
			if (definitionindex == WEAPON_REVOLVER)
			{
				//uncocks the revolver

				//if (m_WeaponMode != 1)
					//m_WeaponMode = 1;
				//if (m_flPostponeFireReadyTime != FLT_MAX)
					//m_flPostponeFireReadyTime = FLT_MAX;
				//if (m_Activity == ACT_CSGO_FIRE_PRIMARY) // 0x3274 = m_Activity  act 208
				//	C_BaseCombatWeapon::SendWeaponAnim(ACT_CSGO_FIRE_PRIMARY_OPT_2); // ACT_CSGO_FIRE_PRIMARY_OPT_2 = 185
			}
			// (*(*v1 + 0x510)) = C_BaseCombatWeapon::PrimaryAttack
			//(*(*v1 + 0x510))(v1);
		}
		return false;
	}

	return false;
}

//Will the weapon actually fire THIS TICK given these buttons
int CWeaponController::WeaponDidFire(int buttons, float curtime)
{
	int did_shoot = 0;
	WeaponCanFire(buttons, &did_shoot, curtime);
	return did_shoot;
}

bool CWeaponController::ShouldSetPostPoneFireReadyTime(CBaseCombatWeapon *weapon, int buttons) const
{
	CBaseEntity *owner;

	if (!weapon || weapon->GetItemDefinitionIndex() != WEAPON_REVOLVER || !(owner = ((CBaseEntity*)weapon)->GetOwner()))
		return false;

	// rebuilt from game
	bool result = false;
	float curtime = Interfaces::Globals->curtime;
	if (!weapon->IsReloading() || curtime < owner->GetNextAttack())
	{
		if (buttons & IN_ATTACK && curtime >= weapon->GetNextPrimaryAttack())
			ItemPostFrame_ProcessPrimaryAttack(weapon, buttons, &result);
	}

	return result;
}

void CWeaponController::AutoCockRevolver()
{
	if (!variable::get().ragebot.b_enabled)
		return;

	// reset
	m_bRevolverWillFire = false;

	bool _RelyOnTickBase = false;//LocalPlayer.m_flAverageFrameTime < Interfaces::Globals->interval_per_tick;

	if (!LocalPlayer.Entity || !LocalPlayer.Entity->GetWeapon() || LocalPlayer.Entity->GetWeapon()->GetItemDefinitionIndex() != WEAPON_REVOLVER)
		return;

	if (_RelyOnTickBase)
	{
		int _DidShoot = 0;
		bool _CanPrimaryAttack = WeaponCanFire(CurrentUserCmd.cmd->buttons | IN_ATTACK, &_DidShoot);
		m_bRevolverWillFire = _DidShoot == 1;

		if (!m_bRevolverWillFire && _CanPrimaryAttack && LocalPlayer.Entity->GetWeapon()->GetClipOne())
			CurrentUserCmd.cmd->buttons |= IN_ATTACK;
	}

	static int _TicksCocked = -1;

	// reset and don't run if we don't hold a revolver or can't shoot for some reason or are dead
	if (!LocalPlayer.IsAlive ||
		!LocalPlayer.WeaponVars.IsRevolver ||
		!LocalPlayer.CurrentWeapon->GetClipOne() ||
		TICKS_TO_TIME(LocalPlayer.Entity->GetTickBase()) < LocalPlayer.CurrentWeapon->GetNextPrimaryAttack() ||
		TICKS_TO_TIME(LocalPlayer.Entity->GetTickBase()) < LocalPlayer.Entity->GetNextAttack())
	{
		_TicksCocked = -1;
		return;
	}

	const int _TicksBeforeFire = (int)(0.25f / (round(Interfaces::Globals->interval_per_tick * 1000000.f) / 1000000.f)) - 1;

	// start at 0 ticks
	if (_TicksCocked == -1)
		_TicksCocked = 0;

	// we shouldn't cock anymore and will fire next tick
	if (_TicksCocked == _TicksBeforeFire)
	{
		_TicksCocked = 0;
		if (!_RelyOnTickBase)
			m_bRevolverWillFire = true;
	}
	else
	{
		if (_RelyOnTickBase)
		{
			if (_TicksBeforeFire > _TicksCocked)
				_TicksCocked++;
			else
				_TicksCocked = 0;
		}
		else
		{
			if (_TicksBeforeFire > _TicksCocked)
				CurrentUserCmd.cmd->buttons |= IN_ATTACK;

			// we want to attack
			if (CurrentUserCmd.cmd->buttons & IN_ATTACK)
				_TicksCocked++;
			else
				_TicksCocked = 0;
		}
	}

	// don't allow shooting if we started cocking
	if (_TicksCocked)
		CurrentUserCmd.cmd->buttons &= ~IN_ATTACK2;
}

static int last_weapon_index = -1;

void CWeaponController::FireBullet(bool secondary)
{
	// don't shoot when we animate or don't choke
	//	if (!LocalPlayer.WeaponVars.IsRevolver && (CurrentUserCmd.GetCommandsChoked() == 0 || !g_Info.m_bShouldChoke))
	//	return;

	auto& var = variable::get();

	//ToDo: do we need this?
	if (g_Ragebot.HasTarget())
	{
		CBaseEntity* _Entity = Interfaces::ClientEntList->GetBaseEntity(g_Ragebot.GetTarget());
		CPlayerrecord* _playerRecord = g_LagCompensation.GetPlayerrecord(_Entity);

		if (_playerRecord && _playerRecord->m_pEntity && g_AutoBone.m_pBestHitbox != nullptr)
		{
			_playerRecord->LastShot.m_iTargetHitgroup = g_AutoBone.m_pBestHitbox->actual_hitgroup_due_to_penetration;
			_playerRecord->LastShot.m_iTargetHitbox = g_AutoBone.m_pBestHitbox->actual_hitbox_due_to_penetration;
			_playerRecord->LastShot.m_angEyeAngles = _playerRecord->m_pEntity->GetEyeAngles();
			auto bestrecord = _playerRecord->m_TargetRecord;

#if defined DEBUG_SHOTS || defined _DEBUG || defined INTERNAL_DEBUG || defined TEST_BUILD
			// debug shot information
			// helper funcs since we can't use stringstream :(
			auto get_float_str = [](float f, int places_after_decimal = 0) -> std::string
			{
				return std::to_string(f).substr(0, std::to_string(f).find('.') + places_after_decimal);
			};
			auto get_hex_string = [=](int num) -> std::string 
			{
				char buffer[32];
				itoa(num, buffer, 16);
				//decrypts(0)
				std::string ret = XorStr("0x");
				//encrypts(0)
				return ret + buffer;
			};

			// detect fake fires
			// todo: nit; this shit is broken beyond belief.
			//LocalPlayer.FakeFired = false;//LocalPlayer.LastAmmo == (LocalPlayer.CurrentWeapon ? LocalPlayer.CurrentWeapon->GetClipOne() : -1);
			//
			//if (last_weapon_index == LocalPlayer.CurrentWeaponItemDefinitionIndex)
			//{
			//	LocalPlayer.LastAmmo = (LocalPlayer.CurrentWeapon ? LocalPlayer.CurrentWeapon->GetClipOne() : -1);
			//}
			//else
			//{
			//	last_weapon_index = LocalPlayer.CurrentWeaponItemDefinitionIndex;
			//}

			const int hitgroup = g_AutoBone.m_pBestHitbox->actual_hitgroup_due_to_penetration;

			player_info_t info;
			Interfaces::EngineClient->GetPlayerInfo(_playerRecord->m_pEntity->index, &info);
			std::string name = adr_util::sanitize_name(info.name);

			const auto bh_resolve_info = bestrecord ? _playerRecord->GetBodyHitResolveInfo(bestrecord) : nullptr;

			unsigned int errors = 0;

			std::string shot_info{};

			auto get_bool_str = [](bool b) -> std::string
			{
				return b ? XorStr("true") : XorStr("false");
			};

			auto get_resolve_mode_str = [](CTickrecord* rec) -> std::string
			{
				std::string mode_str{};
				switch (rec->m_iResolveMode)
				{
					case RESOLVE_MODE_AUTOMATIC:
					{
						//decrypts(0)
						mode_str = XorStr("automatic");
						//encrypts(0)
						break;
					}
					case RESOLVE_MODE_BRUTE_FORCE:
					{
						//decrypts(0)
						mode_str = XorStr("bruteforce");
						//encrypts(0)
						break;
					}
					case RESOLVE_MODE_NONE:
					{
						//decrypts(0)
						mode_str = XorStr("none");
						//encrypts(0)
						break;
					}
					case RESOLVE_MODE_MANUAL:
					{
						//decrypts(0)
						mode_str = XorStr("manual");
						//encrypts(0)
						break;
					}
					default:
					{
						//decrypts(0)
						mode_str = XorStr("unknown");
						//encrypts(0)
						break;
					}
				}

				if (rec->m_bIsUsingBalanceAdjustResolver)
				{
					//decrypts(0)
					mode_str += XorStr(" with balance adjust resolver ");
					//encrypts(0)
				}
				if (rec->m_bIsUsingFreestandResolver)
				{
					//decrypts(0)
					mode_str += XorStr(" with freestand resolver ");
					//encrypts(0)
				}
				if (rec->m_bIsUsingMovingResolver)
				{
					//decrypts(0)
					mode_str += XorStr(" with moving resolver ");
					//encrypts(0)
				}

				return mode_str;
			};

			auto get_desync_results_str = [](int desync_results) -> std::string
			{
				std::string info;
				// | dr:{desync results flags - found-positive-negative-donotresolve-missed-freestanding}
				//decrypts(0)
				if (desync_results & (1 << 0))
					info += XorStr(" Found ");
				if (desync_results & (1 << 1))
					info += XorStr(" Positive ");
				if (desync_results & (1 << 2))
					info += XorStr(" Negative ");
				if (desync_results & (1 << 3))
					info += XorStr(" Do Not Resolve ");
				if (desync_results & (1 << 4))
					info += XorStr(" Missed ");
				if (desync_results & (1 << 5))
					info += XorStr(" Freestanding ");
				//encrypts(0)

				return info;
			};

			auto get_ba_results_str = [](int ba_results) -> std::string
			{
				std::string info;
				// | bar:{balance adjust results flags - found-postiive-negative-missed}
				//decrypts(0)
				if (ba_results & (1 << 0))
					info += XorStr(" Found ");
				if (ba_results & (1 << 1))
					info += XorStr(" Positive ");
				if (ba_results & (1 << 2))
					info += XorStr(" Negative ");
				if (ba_results & (1 << 3))
					info += XorStr(" Missed ");
				//encrypts(0)

				return info;
			};

			//decrypts(0)

			// shot information
			shot_info += XorStr("\n=== SHOT INFORMATION ===\n");
			shot_info += XorStr("Shot At: "); shot_info += name.data(); shot_info += XorStr(" [ Entity Index: "); shot_info += std::to_string(_playerRecord->m_pEntity->index); shot_info += XorStr(" ]'s "); shot_info += get_hitgroup_name(hitgroup).data(); shot_info += "\n";

			// is there any older shot records not acked
			if (LocalPlayer.Entity)
			{
				auto local_player_record = &m_PlayerRecords[LocalPlayer.Entity->index];
				if (local_player_record)
				{
					const int non_acked_count = std::count_if(local_player_record->ShotRecords.begin(), local_player_record->ShotRecords.end(), [](CShotrecord* a) { return a && !a->m_bAck && fabsf(Interfaces::Globals->realtime - a->m_flRealTime) < 1.0f; });
					shot_info += XorStr("Older Records Not Acked Present: "); shot_info += get_bool_str(non_acked_count > 0);
					if (non_acked_count > 0)
					{
						shot_info += XorStr(" | Count: "); shot_info += std::to_string(non_acked_count);
					}
					shot_info += "\n";
				}
			}

			// current ammo before the shot
			shot_info += XorStr("Ammo: "); shot_info += LocalPlayer.CurrentWeapon ? std::to_string(LocalPlayer.CurrentWeapon->GetClipOne()) : XorStr("unable to determine ammo due to LocalPlayer.CurrentWeapon being null"); shot_info += "\n";

			// | fps: {imgui fps}:{was fps >= tickrate}
			shot_info += XorStr("FPS: "); shot_info += get_float_str(ImGui::GetIO().Framerate, 2); shot_info += (ImGui::GetIO().Framerate >= (1.f / Interfaces::Globals->interval_per_tick)) ? XorStr(" (higher than server framerate)") : XorStr(" (lower than server framerate)"); shot_info += "\n";

			// | mp: {is multipoint}
			shot_info += XorStr("Shot At Multipoint: "); shot_info += get_bool_str(g_AutoBone.m_pBestHitbox->bismultipoint); shot_info += "\n";

			// | sp:{current weapon spread + inaccuracy}
			if (LocalPlayer.CurrentWeapon != nullptr && weapon_accuracy_nospread.GetVar() && weapon_accuracy_nospread.GetVar()->GetInt() <= 0)
			{
				shot_info += XorStr("Inaccuracy: "); 
				shot_info += get_float_str(LocalPlayer.CurrentWeapon->GetInaccuracy(), 7); 
				shot_info += "\n";
				shot_info += XorStr("Spread: ");
				shot_info += get_float_str(LocalPlayer.CurrentWeapon->GetWeaponSpread(), 7);
				shot_info += "\n";
			}

			// x: {has hide shots}:{has hide record}:{has multitap}:{has teleport}
			shot_info += XorStr("Exploits Enabled:");
			if (var.ragebot.exploits.b_hide_shots)
				shot_info += XorStr(" Hide Shots ");
			if (var.ragebot.exploits.b_hide_record)
				shot_info += XorStr(" Hide Record ");
			if (var.ragebot.exploits.b_multi_tap.get())
				shot_info += XorStr(" Multitap ");
			if (var.ragebot.exploits.b_nasa_walk.get())
				shot_info += XorStr(" Teleport ");
			shot_info += "\n";

			// s: {should shift shot}:{ready to shift}:{shift amt}
			shot_info += XorStr("Applied Tickbase Shift: "); shot_info += get_bool_str(LocalPlayer.ApplyTickbaseShift);
			if (LocalPlayer.ApplyTickbaseShift)
			{
				shot_info += XorStr(" - Ready To Shift Tickbase: "); shot_info += get_bool_str(g_Tickbase.m_bReadyToShiftTickbase); shot_info += XorStr(" - "); shot_info += std::to_string(g_Tickbase.m_iNumFakeCommandsToSend); shot_info += XorStr(" Fake Commands"); shot_info += "\n";
			}
			else
			{
				shot_info += "\n";
			}

			// aw/dmg:{calculated mindmg}:{mindmg}
			if (g_AutoBone.m_pBestHitbox->penetrated)
			{
				shot_info += XorStr("Autowall Damage Calculated: "); shot_info += get_float_str(g_AutoBone.m_pBestHitbox->damage); shot_info += XorStr(" | Minimum Autowall Damage: "); shot_info += std::to_string(variable::get().ragebot.i_mindmg_aw); shot_info += "\n";
				if (g_AutoBone.m_pBestHitbox->damage < (float)variable::get().ragebot.i_mindmg_aw)
				{
					errors |= (1 << 6);
				}
			}
			else
			{
				shot_info += XorStr("Visible Damage Calculated: "); shot_info += get_float_str(g_AutoBone.m_pBestHitbox->damage); shot_info += XorStr(" | Minimum Visible Damage: "); shot_info += std::to_string(variable::get().ragebot.i_mindmg); shot_info += "\n";
				if (g_AutoBone.m_pBestHitbox->damage < (float)variable::get().ragebot.i_mindmg)
				{
					errors |= (1 << 7);
				}
			}

			// | hc: {calculated hc}:{min hc}
			if (weapon_accuracy_nospread.GetVar() && weapon_accuracy_nospread.GetVar()->GetInt() < 1)
			{
				if (g_AutoBone.m_pBestHitbox->actual_hitbox_due_to_penetration == HITBOX_HEAD || g_AutoBone.m_pBestHitbox->actual_hitbox_due_to_penetration == HITBOX_LOWER_NECK)
				{
					shot_info += XorStr("Head Hitchance Calculated: "); shot_info += (g_AutoBone.m_pBestHitbox->hitchance == 99.99f ? XorStr("Max Accuracy") : get_float_str(g_AutoBone.m_pBestHitbox->hitchance)); shot_info += XorStr(" | Minimum Head Hitchance: "); shot_info += get_float_str(variable::get().ragebot.f_hitchance); shot_info += "\n";
					if (g_AutoBone.m_pBestHitbox->hitchance != 99.99 && g_AutoBone.m_pBestHitbox->hitchance < variable::get().ragebot.f_hitchance - 1.f)
					{
						errors |= (1 << 8);
					}
				}
				else
				{
					shot_info += XorStr("Body Hitchance Calculated: "); shot_info += (g_AutoBone.m_pBestHitbox->hitchance == 99.99f ? XorStr("Max Accuracy") : get_float_str(g_AutoBone.m_pBestHitbox->hitchance)); shot_info += XorStr(" | Minimum Body Hitchance: "); shot_info += get_float_str(variable::get().ragebot.f_body_hitchance); shot_info += "\n";
					if (g_AutoBone.m_pBestHitbox->hitchance != 99.99 && g_AutoBone.m_pBestHitbox->hitchance < variable::get().ragebot.f_body_hitchance - 1.f)
					{
						errors |= (1 << 9);
					}
				}
			}

			// | b: {baim reason}
			shot_info += XorStr("Baim Reason: "); shot_info += _playerRecord->GetBaimReasonString(); shot_info += "\n";
			// | m: {shotsmissed}:{shots missed balance adjust}:{should be baiming}
			shot_info += XorStr("Misses: "); shot_info += std::to_string(_playerRecord->m_iShotsMissed); shot_info += XorStr(" ( "); shot_info += std::to_string(_playerRecord->m_iShotsMissed_BalanceAdjust); shot_info += XorStr(" Balance Adjust Misses )");
			if (variable::get().ragebot.baim_main.b_after_misses)
			{
				// | sbm: {should baim after misses}
				shot_info += XorStr("Should Baim After Misses: "); shot_info += get_bool_str(_playerRecord->m_iShotsMissed >= variable::get().ragebot.baim_main.i_after_misses); shot_info += "\n";
			}
			// | vr: {validrecordcount}
			shot_info += XorStr("Target Valid Record Count: "); shot_info += std::to_string(_playerRecord->m_ValidRecordCount); shot_info += "\n";

			if (bestrecord)
			{
				// | smd:{scaled mindmg}

				//this code needs to be rewritten, we should not use bestscaledmindamage here..

				shot_info += XorStr("Scaled Mindmg: "); shot_info += get_float_str(bestrecord->m_flBestScaledMinDamage); shot_info += "\n";

				if (bestrecord->m_flBestScaledMinDamage < g_AutoBone.m_pBestHitbox->damage)
				{
					if (g_AutoBone.m_pBestHitbox->penetrated && g_AutoBone.m_pBestHitbox->damage < (float)variable::get().ragebot.i_mindmg_aw)
					{
						errors |= (1 << 6);
					}
					else if (!g_AutoBone.m_pBestHitbox->penetrated && g_AutoBone.m_pBestHitbox->damage < (float)variable::get().ragebot.i_mindmg)
					{
						errors |= (1 << 7);
					}
				}

				// | ft:{hitgroup name from forwardtrack}
				if (_playerRecord->m_iTickcount_ForwardTrack != 0 && _playerRecord->GetCurrentRecord())
				{
					shot_info += XorStr("ForwardTracked a ");  shot_info += get_hitgroup_name(_playerRecord->GetCurrentRecord()->m_BestHitbox_ForwardTrack.actual_hitgroup_due_to_penetration).data(); shot_info += "\n";
				}

				// h:{health stored in the tickrecord}
				shot_info += XorStr("Target Health: "); shot_info += std::to_string(bestrecord->m_Health); shot_info += "\n";

				if (_playerRecord->m_iBaimReason == BAIM_REASON_LETHAL && bestrecord->m_Health >= 100)
				{
					errors |= (1 << 1);
				}
				if (bestrecord->m_bBodyIsVisible && (hitgroup == HITGROUP_HEAD || hitgroup == HITGROUP_NECK))
				{
					if (_playerRecord->m_iBaimReason > BAIM_REASON_NONE)
					{
						errors |= (1 << 3);
					}

					if (variable::get().ragebot.baim_main.i_after_misses && _playerRecord->m_iShotsMissed >= variable::get().ragebot.baim_main.i_after_misses)
					{
						errors |= (1 << 4);
					}
				}

				// | v: {velocity length}:{our velocity length}
				if (bestrecord->m_bMoving)
				{
					shot_info += XorStr("Target Velocity Length: "); shot_info += get_float_str(bestrecord->m_Velocity.Length(), 2);
					if (LocalPlayer.Entity != nullptr)
					{
						shot_info += XorStr(" | Local AbsVelocity Length: "); shot_info += get_float_str(LocalPlayer.Entity->GetAbsVelocityDirect().Length(), 2); shot_info += "\n";
					}
					else
					{
						shot_info += "\n";
					}
				}


				// | d: {dormant}:{far esp}
				shot_info += XorStr("Target Dormant: "); shot_info += get_bool_str(bestrecord->m_Dormant);
				if (variable::get().visuals.pf_enemy.b_faresp)
				{
					shot_info += XorStr("Target On FarESP: "); shot_info += get_bool_str(_playerRecord->m_bIsUsingFarESP); shot_info += "\n";
				}
				else
				{
					shot_info += "\n";
				}

				// | hs: {has ran hitscan}
				//shot_info += XorStr(" | hs: "); shot_info += get_bool_str(bestrecord->m_bRanHitscan);
				if (!bestrecord->m_bRanHitscan && _playerRecord->m_iTickcount_ForwardTrack == 0)
				{
					errors |= (1 << 0);
				}
				// | vis: {head visible}:{body visible}
				shot_info += XorStr("Target Head Visible: "); shot_info += get_bool_str(bestrecord->m_bHeadIsVisible); shot_info += XorStr(" | Target Body Visible: "); shot_info += get_bool_str(bestrecord->m_bBodyIsVisible); shot_info += "\n";
				// | w: {weight of record}
				shot_info += XorStr("Target Weight: "); shot_info += std::to_string(bestrecord->GetWeight()); shot_info += "\n"; 
				// | fb: {fired bullet}
				shot_info += XorStr("Target Fired Bullet: "); shot_info += get_bool_str(bestrecord->m_bFiredBullet); shot_info += "\n";
				// | tc: {ticks choked}:{our ticks choked}
				shot_info += XorStr("Ticks Target Choked: "); shot_info += std::to_string(bestrecord->m_iTicksChoked); shot_info += XorStr(" | Our Ticks Choked: "); shot_info += std::to_string(g_ClientState->chokedcommands); shot_info += "\n";

				// | e:b = backwards tickbase exploit
				if (bestrecord->m_bTickbaseShiftedBackwards)
				{
					shot_info += XorStr("Exploiting: Tickbase Shifted Backwards"); shot_info += "\n";
				}
				// | e:f = forwards tickbase exploit
				if (bestrecord->m_bTickbaseShiftedForwards)
				{
					shot_info += XorStr("Exploiting: Tickbase Shifted Forwards"); shot_info += "\n";
				}

				// | t: {is teleporting}
				shot_info += XorStr("Target Teleporting: "); shot_info += get_bool_str(bestrecord->m_bTeleporting); shot_info += "\n";
				// | ba: {triggered 979 animation on this tick}
				shot_info += XorStr("Target Triggered Balance Adjust: "); shot_info += get_bool_str(bestrecord->m_bBalanceAdjust); shot_info += "\n";
				// | lg: {legit}:{force not legit}
				shot_info += XorStr("Target Is Legit: "); shot_info += get_bool_str(bestrecord->m_bLegit); shot_info += XorStr(" | Target Is Forced Not Legit: "); shot_info += get_bool_str(bestrecord->m_bForceNotLegit); shot_info += "\n";
				// | r: {resolve mode str}:{is body hit resolved}
				shot_info += XorStr("Resolve Mode: "); shot_info += get_resolve_mode_str(bestrecord); shot_info += "\n";

				if (bh_resolve_info)
				{
					shot_info += XorStr("Is Using EXP Resolver: "); shot_info += get_bool_str(bh_resolve_info->m_bIsBodyHitResolved); shot_info += "\n";
				}

				if (_playerRecord->m_DesyncResults != 0)
				{
					shot_info += XorStr("Desync Results Flags: "); shot_info += get_desync_results_str(_playerRecord->m_DesyncResults); shot_info += "\n";
				}

				if (_playerRecord->m_BalanceAdjustResults != 0)
				{
					shot_info += XorStr("Balance Adjust Results Flags: "); shot_info += get_ba_results_str(_playerRecord->m_BalanceAdjustResults); shot_info += "\n";
				}

				// | rs:{resolve side:resolve side when choked} adds ** ** if the side was used
				shot_info += XorStr("Resolver Side: "); shot_info += g_Visuals.GetResolveSide(bestrecord->m_iResolveSide); 
				shot_info += "\n";


				if (bh_resolve_info != nullptr)
				{
					// | bs: {each body hit resolve stance if resolved - seperated from 0 - 5:each body hit miss:desync amt}
					// 0 - standing | 1 - running | 2 - walking | 3 - ducking | 4 - duckwalking | 5 - in air
					auto Stance = _playerRecord->Impact.ResolveStances[CPlayerrecord::ImpactResolveStances::Standing];
					shot_info += XorStr("EXP Resolver Info:\n");
					shot_info += XorStr("\tStanding Info | Is EXP Resolved: "); 
					shot_info += get_bool_str(Stance.m_bIsBodyHitResolved);
					shot_info += XorStr(" | Body Hit Misses: "); 
					shot_info += std::to_string(Stance.m_iShotsMissed);
					shot_info += XorStr(" | Desync Delta: "); shot_info += get_float_str(Stance.m_flDesyncDelta, 2);
					shot_info += "\n";

					Stance = _playerRecord->Impact.ResolveStances[CPlayerrecord::ImpactResolveStances::Running];
					shot_info += XorStr("\tRunning Info | Is EXP Resolved: "); 
					shot_info += get_bool_str(Stance.m_bIsBodyHitResolved);
					shot_info += XorStr(" | Body Hit Misses: "); shot_info += std::to_string(Stance.m_iShotsMissed);
					shot_info += XorStr(" | Desync Delta: "); shot_info += get_float_str(Stance.m_flDesyncDelta, 2); 
					shot_info += "\n";

					Stance = _playerRecord->Impact.ResolveStances[CPlayerrecord::ImpactResolveStances::Walking];
					shot_info += XorStr("\tWalking Info | Is EXP Resolved: "); 
					shot_info += get_bool_str(Stance.m_bIsBodyHitResolved);
					shot_info += XorStr(" | Body Hit Misses: "); shot_info += std::to_string(Stance.m_iShotsMissed);
					shot_info += XorStr(" | Desync Delta: "); shot_info += get_float_str(Stance.m_flDesyncDelta, 2); 
					shot_info += "\n";
				}

				shot_info += "\n";

				// | th:{every ticks choked}:{zero count}:{zero percentage}
				shot_info += XorStr("Target Tick History: ");
				for (size_t i = 0; i < _playerRecord->m_iTicksChokedHistory.size(); ++i)
				{
					shot_info += std::to_string(_playerRecord->m_iTicksChokedHistory[i]);
					if (i < _playerRecord->m_iTicksChokedHistory.size() - 1)
						shot_info += "-";
				}

			}
			else
			{
				errors |= (1 << 2);
			}

			shot_info += XorStr("\n\n====================================\n");

			// | err:{errors}
			if (errors > 0)
			{
				shot_info += XorStr("!! SHOT RECORD ERRORS PRESENT: ");
				if (errors & (1 << 0))
				{
					shot_info += XorStr(" no hitscan ");
				}
				if (errors & (1 << 1))
				{
					shot_info += XorStr(" wrong lethal ");
				}
				if (errors & (1 << 2))
				{
					shot_info += XorStr(" no best record ");
				}
				if (errors & (1 << 3))
				{
					shot_info += XorStr(" should have baimed (baim reason) ");
				}
				if (errors & (1 << 4))
				{
					shot_info += XorStr(" should have baimed (misses) ");
				}
				if (errors & (1 << 5))
				{
					shot_info += XorStr(" animstate heap corruption ");
				}
				if (errors & (1 << 6))
				{
					shot_info += XorStr(" aw mindmg logic error ");
				}
				if (errors & (1 << 7))
				{
					shot_info += XorStr(" mindmg logic error ");
				}
				if (errors & (1 << 8))
				{
					shot_info += XorStr(" head hc logic error ");
				}
				if (errors & (1 << 9))
				{
					shot_info += XorStr(" body hc logic error ");
				}
			}

			//encrypts(0)
			g_Eventlog.AddEventElement(errors > 0 ? 0x0E : 0x01, shot_info.data());
			g_Eventlog.OutputEvent(1);

#if defined _DEBUG || defined INTERNAL_DEBUG
			printf("%s\n", shot_info.data());
#endif
#endif
		}
	}

	if (!(CurrentUserCmd.cmd_backup.buttons & IN_ATTACK2))
	{
		CurrentUserCmd.cmd->buttons |= !secondary ? IN_ATTACK : IN_ATTACK2;
		if (!LocalPlayer.IsFakeDucking && !LocalPlayer.IsTeleporting)
		{
			g_Info.SetShouldChoke(false);
			g_Info.SetForceSend(true);
		}
	}
}

bool CWeaponController::ShouldDelayShot()
{
	if(!variable::get().ragebot.b_antiaim || !LocalPlayer.Config_IsFakelagging() || g_Info.m_iNumCommandsToChoke == 0)
		return false;

	if (LocalPlayer.WeaponWillFireBurstShotThisTick)
		return false;

	//if(LocalPlayer.Config_IsDesyncing() && LocalPlayer.Entity->GetVelocity().Length() <= 0.1f)
	//{
	//	if (Interfaces::Globals->curtime > LocalPlayer.m_next_lby_update_time ||
	//		/*m_bPreflicking not set yet so just run the check here below for it*/
	//		(g_AntiAim.m_b_lby_after_stopping_already_updated_once && (Interfaces::Globals->curtime + TICKS_TO_TIME(1)) > LocalPlayer.m_next_lby_update_time) )
	//			return true; //let us still break lby
	//}

	//bool PreventShotBacktrack = LocalPlayer.PreventShotBacktracking && !LocalPlayer.IsFakeLaggingOnPeek && !LocalPlayer.IsFakeDucking;

	//if (PreventShotBacktrack)
	//{
		//if (LocalPlayer.CreateFakeCommands)
		//	return true;
	//}
	//else
	//{
		//Don't sendpacket twice
	//	if (g_ClientState->chokedcommands == 0)
	//		return true;
	//}

	if((!input::get().is_key_down(variable::get().misc.i_fakeduck_key) || !LocalPlayer.IsFakeDucking) && !LocalPlayer.IsTeleporting)
	{
		//if (!PreventShotBacktrack || !LocalPlayer.CreateFakeCommands)
		{
			g_Info.SetShouldChoke(false);
			g_Info.SetForceSend(true); //force this usercmd to send
		}
	}

	return false;
}
