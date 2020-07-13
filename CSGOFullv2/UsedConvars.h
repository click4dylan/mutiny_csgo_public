#pragma once
#include "ICvar.h"
#include "ConVar.h"
#include "EncryptString.h"
#include "Interfaces.h"

// todo: nit; make a wrapper class for usedconvars with 'setup' function to find all the cvars once; to prevent nullptrs on rebuilding

class UsedConvar
{
public:
	UsedConvar::UsedConvar(int length, char *strng)
	{
		cvar = nullptr;
		len = length;
		str = strng; 
	}

	ConVar* GetVar()
	{
		if (cvar)
			return cvar;
		
		DecStr(str, len);
		cvar = Interfaces::Cvar->FindVar(str);
		EncStr(str, len);
		if (cvar)
			delete[] str;
		return cvar;
	}

	void SetValue(char* val)
	{
		if (cvar)
			cvar->SetValue(val);
	}

	float GetFloat()
	{
		if (cvar)
			return cvar->GetFloat();
		return 0.0f;
	}

	int GetInt()
	{
		if (cvar)
			return cvar->GetInt();
		return 0;
	}

	bool GetBool()
	{
		if (cvar)
			return cvar->GetBool();
		return 0;
	}

private:
	ConVar *cvar;
	int len;
	char *str;
};

extern UsedConvar zoom_sensitivity_ratio;

extern float flOriginalZoomSensitivityRatio;
void ResetZoomSensitivityRatio();

extern UsedConvar weapon_recoil_decay2_exp;
extern UsedConvar cl_pred_checkstuck;
extern UsedConvar sv_staminarecoveryrate;
extern UsedConvar sv_rollangle;
extern UsedConvar sv_rollspeed;
extern UsedConvar sv_maxvelocity;
extern UsedConvar sv_ladder_scale_speed;
extern UsedConvar sv_ladder_dampen;
extern UsedConvar sv_bounce;
extern UsedConvar sv_accelerate;
extern UsedConvar sv_jump_impulse;
extern UsedConvar sv_enablebunnyhopping;
extern UsedConvar sv_autobunnyhopping;
extern UsedConvar sv_gravity;
extern UsedConvar sv_staminamax;
extern UsedConvar sv_staminalandcost;
extern UsedConvar sv_staminajumpcost;
extern UsedConvar sv_accelerate;
extern UsedConvar sv_accelerate_use_weapon_speed;
extern UsedConvar sv_airaccelerate;
extern UsedConvar sv_stopspeed;
extern UsedConvar sv_friction;
extern UsedConvar view_punch_decay;
extern UsedConvar sv_noclipspeed;
extern UsedConvar sv_noclipaccelerate;
extern UsedConvar sv_optimizedmovement;
extern UsedConvar sv_ladder_angle;
extern UsedConvar sv_timebetweenducks;
extern UsedConvar weapon_recoil_decay2_lin;
extern UsedConvar weapon_recoil_vel_decay;
extern UsedConvar sv_specaccelerate;
extern UsedConvar sv_specspeed;
extern UsedConvar sv_specnoclip;
extern UsedConvar sv_maxspeed;
extern UsedConvar cl_pred_optimize;
extern UsedConvar cl_predict;
extern UsedConvar cl_camera_follow_bone_index;
extern UsedConvar sv_water_swim_mode;
extern UsedConvar sv_water_movespeed_multiplier;
extern UsedConvar sv_air_pushaway_dist;
extern UsedConvar sv_airaccelerate_parachute;
extern UsedConvar sv_airaccelerate_rappel;
extern UsedConvar sv_ledge_mantle_helper;
extern UsedConvar sv_ledge_mantle_helper_debug;
extern UsedConvar mp_teammates_are_enemies;
extern UsedConvar weapon_accuracy_nospread;
extern UsedConvar weapon_recoil_scale;
extern UsedConvar sv_maxusrcmdprocessticks;
extern UsedConvar sv_max_usercmd_future_ticks;
extern UsedConvar cl_pred_doresetlatch;
extern UsedConvar sv_infinite_ammo;
extern UsedConvar ff_damage_reduction_bullets;
extern UsedConvar ff_damage_bullet_penetration;
extern UsedConvar sv_penetration_type;
extern UsedConvar sv_clip_penetration_traces_to_players;
extern UsedConvar weapon_accuracy_shotgun_spread_patterns;
extern UsedConvar sv_maxunlag;
extern UsedConvar mp_coopmission_dz;
extern UsedConvar sv_stepheight;