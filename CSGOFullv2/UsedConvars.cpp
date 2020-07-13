#include "precompiled.h"
#include "UsedConvars.h"

char *zoomsensitivityratiomousestr = new char[29]{ 0, 21, 21, 23, 37, 9, 31, 20, 9, 19, 14, 19, 12, 19, 14, 3, 37, 8, 27, 14, 19, 21, 37, 23, 21, 15, 9, 31, 0 }; /*zoom_sensitivity_ratio_mouse*/
UsedConvar zoom_sensitivity_ratio (28, zoomsensitivityratiomousestr);

float flOriginalZoomSensitivityRatio = 1.0f;

void ResetZoomSensitivityRatio()
{
#if 0
	if (flOriginalZoomSensitivityRatio == ThirdPersonZoomSensitivityRatioTxt.flValue)
		flOriginalZoomSensitivityRatio = 1.0f;
	char zoomamt[16];
	sprintf(zoomamt, "%f", flOriginalZoomSensitivityRatio);
	zoom_sensitivity_ratio.SetValue(zoomamt);
#endif
}


char *weaponrecoildecay2expstr = new char[25]{ 13, 31, 27, 10, 21, 20, 37, 8, 31, 25, 21, 19, 22, 37, 30, 31, 25, 27, 3, 72, 37, 31, 2, 10, 0 }; /*weapon_recoil_decay2_exp*/
UsedConvar weapon_recoil_decay2_exp(24, weaponrecoildecay2expstr);
char *clpredcheckstuckstr = new char[19]{ 25, 22, 37, 10, 8, 31, 30, 37, 25, 18, 31, 25, 17, 9, 14, 15, 25, 17, 0 }; /*cl_pred_checkstuck*/
UsedConvar cl_pred_checkstuck(18, clpredcheckstuckstr);
char *svstaminarecoveryratestr = new char[23]{ 9, 12, 37, 9, 14, 27, 23, 19, 20, 27, 8, 31, 25, 21, 12, 31, 8, 3, 8, 27, 14, 31, 0 }; /*sv_staminarecoveryrate*/
UsedConvar sv_staminarecoveryrate(22, svstaminarecoveryratestr);
char *svrollanglestr = new char[13]{ 9, 12, 37, 8, 21, 22, 22, 27, 20, 29, 22, 31, 0 }; /*sv_rollangle*/
UsedConvar sv_rollangle(12, svrollanglestr);
char *svrollspeedstr = new char[13]{ 9, 12, 37, 8, 21, 22, 22, 9, 10, 31, 31, 30, 0 }; /*sv_rollspeed*/
UsedConvar sv_rollspeed(12, svrollspeedstr);
char *svmaxvelocitystr = new char[15]{ 9, 12, 37, 23, 27, 2, 12, 31, 22, 21, 25, 19, 14, 3, 0 }; /*sv_maxvelocity*/
UsedConvar sv_maxvelocity(14, svmaxvelocitystr);
char *svladderscalespeedstr = new char[22]{ 9, 12, 37, 22, 27, 30, 30, 31, 8, 37, 9, 25, 27, 22, 31, 37, 9, 10, 31, 31, 30, 0 }; /*sv_ladder_scale_speed*/
UsedConvar sv_ladder_scale_speed(21, svladderscalespeedstr);
char *svladderdampenstr = new char[17]{ 9, 12, 37, 22, 27, 30, 30, 31, 8, 37, 30, 27, 23, 10, 31, 20, 0 }; /*sv_ladder_dampen*/
UsedConvar sv_ladder_dampen(16, svladderdampenstr);
char *svbouncestr = new char[10]{ 9, 12, 37, 24, 21, 15, 20, 25, 31, 0 }; /*sv_bounce*/
UsedConvar sv_bounce(9, svbouncestr);
char *svjumpimpulsestr = new char[16]{ 9, 12, 37, 16, 15, 23, 10, 37, 19, 23, 10, 15, 22, 9, 31, 0 }; /*sv_jump_impulse*/
UsedConvar sv_jump_impulse(15, svjumpimpulsestr);
char *svenablebunnyhoppingstr = new char[22]{ 9, 12, 37, 31, 20, 27, 24, 22, 31, 24, 15, 20, 20, 3, 18, 21, 10, 10, 19, 20, 29, 0 }; /*sv_enablebunnyhopping*/
UsedConvar sv_enablebunnyhopping(21, svenablebunnyhoppingstr);
char *svautobunnyhoppingstr = new char[20]{ 9, 12, 37, 27, 15, 14, 21, 24, 15, 20, 20, 3, 18, 21, 10, 10, 19, 20, 29, 0 }; /*sv_autobunnyhopping*/
UsedConvar sv_autobunnyhopping(19, svautobunnyhoppingstr);
char *svgravitystr = new char[11]{ 9, 12, 37, 29, 8, 27, 12, 19, 14, 3, 0 }; /*sv_gravity*/
UsedConvar sv_gravity(10, svgravitystr);
char *svstaminamaxstr = new char[14]{ 9, 12, 37, 9, 14, 27, 23, 19, 20, 27, 23, 27, 2, 0 }; /*sv_staminamax*/
UsedConvar sv_staminamax(13, svstaminamaxstr);
char *svstaminalandcoststr = new char[19]{ 9, 12, 37, 9, 14, 27, 23, 19, 20, 27, 22, 27, 20, 30, 25, 21, 9, 14, 0 }; /*sv_staminalandcost*/
UsedConvar sv_staminalandcost(18, svstaminalandcoststr);
char *svstaminajumpcoststr = new char[19]{ 9, 12, 37, 9, 14, 27, 23, 19, 20, 27, 16, 15, 23, 10, 25, 21, 9, 14, 0 }; /*sv_staminajumpcost*/
UsedConvar sv_staminajumpcost(18, svstaminajumpcoststr);
char *svacceleratestr = new char[14]{ 9, 12, 37, 27, 25, 25, 31, 22, 31, 8, 27, 14, 31, 0 }; /*sv_accelerate*/
UsedConvar sv_accelerate(13, svacceleratestr);
char *svaccelerateuseweaponspeedstr = new char[31]{ 9, 12, 37, 27, 25, 25, 31, 22, 31, 8, 27, 14, 31, 37, 15, 9, 31, 37, 13, 31, 27, 10, 21, 20, 37, 9, 10, 31, 31, 30, 0 }; /*sv_accelerate_use_weapon_speed*/
UsedConvar sv_accelerate_use_weapon_speed(30, svaccelerateuseweaponspeedstr);
char *svairacceleratestr = new char[17]{ 9, 12, 37, 27, 19, 8, 27, 25, 25, 31, 22, 31, 8, 27, 14, 31, 0 }; /*sv_airaccelerate*/
UsedConvar sv_airaccelerate(16, svairacceleratestr);
char *svstopspeedstr = new char[13]{ 9, 12, 37, 9, 14, 21, 10, 9, 10, 31, 31, 30, 0 }; /*sv_stopspeed*/
UsedConvar sv_stopspeed(12, svstopspeedstr);
char *svfrictionstr = new char[12]{ 9, 12, 37, 28, 8, 19, 25, 14, 19, 21, 20, 0 }; /*sv_friction*/
UsedConvar sv_friction(11, svfrictionstr);
char *viewpunchdecaystr = new char[17]{ 12, 19, 31, 13, 37, 10, 15, 20, 25, 18, 37, 30, 31, 25, 27, 3, 0 }; /*view_punch_decay*/
UsedConvar view_punch_decay(16, viewpunchdecaystr);
char *svnoclipspeedstr = new char[15]{ 9, 12, 37, 20, 21, 25, 22, 19, 10, 9, 10, 31, 31, 30, 0 }; /*sv_noclipspeed*/
UsedConvar sv_noclipspeed(14, svnoclipspeedstr);
char *svnoclipacceleratestr = new char[20]{ 9, 12, 37, 20, 21, 25, 22, 19, 10, 27, 25, 25, 31, 22, 31, 8, 27, 14, 31, 0 }; /*sv_noclipaccelerate*/
UsedConvar sv_noclipaccelerate(19, svnoclipacceleratestr);
char *svoptimizedmovementstr = new char[21]{ 9, 12, 37, 21, 10, 14, 19, 23, 19, 0, 31, 30, 23, 21, 12, 31, 23, 31, 20, 14, 0 }; /*sv_optimizedmovement*/
UsedConvar sv_optimizedmovement(20, svoptimizedmovementstr);
char *svladderanglestr = new char[16]{ 9, 12, 37, 22, 27, 30, 30, 31, 8, 37, 27, 20, 29, 22, 31, 0 }; /*sv_ladder_angle*/
UsedConvar sv_ladder_angle(15, svladderanglestr);
char *svtimebetweenducksstr = new char[20]{ 9, 12, 37, 14, 19, 23, 31, 24, 31, 14, 13, 31, 31, 20, 30, 15, 25, 17, 9, 0 }; /*sv_timebetweenducks*/
UsedConvar sv_timebetweenducks(19, svtimebetweenducksstr);
char *weaponrecoildecay2linstr = new char[25]{ 13, 31, 27, 10, 21, 20, 37, 8, 31, 25, 21, 19, 22, 37, 30, 31, 25, 27, 3, 72, 37, 22, 19, 20, 0 }; /*weapon_recoil_decay2_lin*/
UsedConvar weapon_recoil_decay2_lin(24, weaponrecoildecay2linstr);
char *weaponrecoilveldecaystr = new char[24]{ 13, 31, 27, 10, 21, 20, 37, 8, 31, 25, 21, 19, 22, 37, 12, 31, 22, 37, 30, 31, 25, 27, 3, 0 }; /*weapon_recoil_vel_decay*/
UsedConvar weapon_recoil_vel_decay(23, weaponrecoilveldecaystr);
char *svspecacceleratestr = new char[18]{ 9, 12, 37, 9, 10, 31, 25, 27, 25, 25, 31, 22, 31, 8, 27, 14, 31, 0 }; /*sv_specaccelerate*/
UsedConvar sv_specaccelerate(17, svspecacceleratestr);
char *svspecspeedstr = new char[13]{ 9, 12, 37, 9, 10, 31, 25, 9, 10, 31, 31, 30, 0 }; /*sv_specspeed*/
UsedConvar sv_specspeed(12, svspecspeedstr);
char *svspecnoclipstr = new char[14]{ 9, 12, 37, 9, 10, 31, 25, 20, 21, 25, 22, 19, 10, 0 }; /*sv_specnoclip*/
UsedConvar sv_specnoclip(13, svspecnoclipstr);
char *svmaxspeedasdfstr = new char[12]{ 9, 12, 37, 23, 27, 2, 9, 10, 31, 31, 30, 0 }; /*sv_maxspeed*/
UsedConvar sv_maxspeed(11, svmaxspeedasdfstr);
char *clpredoptimizestr = new char[17]{ 25, 22, 37, 10, 8, 31, 30, 37, 21, 10, 14, 19, 23, 19, 0, 31, 0 }; /*cl_pred_optimize*/
UsedConvar cl_pred_optimize(16, clpredoptimizestr);
char *clpredictstr = new char[11]{ 25, 22, 37, 10, 8, 31, 30, 19, 25, 14, 0 }; /*cl_predict*/
UsedConvar cl_predict(10, clpredictstr);
char *clcamerafollowboneindexstr = new char[28]{ 25, 22, 37, 25, 27, 23, 31, 8, 27, 37, 28, 21, 22, 22, 21, 13, 37, 24, 21, 20, 31, 37, 19, 20, 30, 31, 2, 0 }; /*cl_camera_follow_bone_index*/
UsedConvar cl_camera_follow_bone_index(27, clcamerafollowboneindexstr);
char *sv_water_swim_modestr = new char[19]{ 9, 12, 37, 13, 27, 14, 31, 8, 37, 9, 13, 19, 23, 37, 23, 21, 30, 31, 0 }; /*sv_water_swim_mode*/
UsedConvar sv_water_swim_mode(18, sv_water_swim_modestr);
char *sv_water_movespeed_multiplierstr = new char[30]{ 9, 12, 37, 13, 27, 14, 31, 8, 37, 23, 21, 12, 31, 9, 10, 31, 31, 30, 37, 23, 15, 22, 14, 19, 10, 22, 19, 31, 8, 0 }; /*sv_water_movespeed_multiplier*/
UsedConvar sv_water_movespeed_multiplier(29, sv_water_movespeed_multiplierstr);
char *sv_air_pushaway_diststr = new char[21]{ 9, 12, 37, 27, 19, 8, 37, 10, 15, 9, 18, 27, 13, 27, 3, 37, 30, 19, 9, 14, 0 }; /*sv_air_pushaway_dist*/
UsedConvar sv_air_pushaway_dist(20, sv_air_pushaway_diststr);
char *sv_airaccelerate_parachutestr = new char[27]{ 9, 12, 37, 27, 19, 8, 27, 25, 25, 31, 22, 31, 8, 27, 14, 31, 37, 10, 27, 8, 27, 25, 18, 15, 14, 31, 0 }; /*sv_airaccelerate_parachute*/
UsedConvar sv_airaccelerate_parachute(26, sv_airaccelerate_parachutestr);
char *sv_airaccelerate_rappelstr = new char[24]{ 9, 12, 37, 27, 19, 8, 27, 25, 25, 31, 22, 31, 8, 27, 14, 31, 37, 8, 27, 10, 10, 31, 22, 0 }; /*sv_airaccelerate_rappel*/
UsedConvar sv_airaccelerate_rappel(23, sv_airaccelerate_rappelstr);
char *sv_ledge_mantle_helperstr = new char[23]{ 9, 12, 37, 22, 31, 30, 29, 31, 37, 23, 27, 20, 14, 22, 31, 37, 18, 31, 22, 10, 31, 8, 0 }; /*sv_ledge_mantle_helper*/
UsedConvar sv_ledge_mantle_helper(22, sv_ledge_mantle_helperstr);
char *sv_ledge_mantle_helper_debugstr = new char[29]{ 9, 12, 37, 22, 31, 30, 29, 31, 37, 23, 27, 20, 14, 22, 31, 37, 18, 31, 22, 10, 31, 8, 37, 30, 31, 24, 15, 29, 0 }; /*sv_ledge_mantle_helper_debug*/
UsedConvar sv_ledge_mantle_helper_debug(28, sv_ledge_mantle_helper_debugstr);
char *mpteammatesareenemiesstr = new char[25]{23, 10, 37, 14, 31, 27, 23, 23, 27, 14, 31, 9, 37, 27, 8, 31, 37, 31, 20, 31, 23, 19, 31, 9, 0}; /*mp_teammates_are_enemies*/
UsedConvar mp_teammates_are_enemies(24, mpteammatesareenemiesstr);
char *weaponaccuracynospreadstr = new char[25]{13, 31, 27, 10, 21, 20, 37, 27, 25, 25, 15, 8, 27, 25, 3, 37, 20, 21, 9, 10, 8, 31, 27, 30, 0}; /*weapon_accuracy_nospread*/
UsedConvar weapon_accuracy_nospread(24, weaponaccuracynospreadstr);
char *weapon_recoil_scalestr = new char[20]{ 13, 31, 27, 10, 21, 20, 37, 8, 31, 25, 21, 19, 22, 37, 9, 25, 27, 22, 31, 0 }; /*weapon_recoil_scale*/
UsedConvar weapon_recoil_scale(19, weapon_recoil_scalestr);
char *svmaxusrcmdprocessticksstr = new char[25]{ 9, 12, 37, 23, 27, 2, 15, 9, 8, 25, 23, 30, 10, 8, 21, 25, 31, 9, 9, 14, 19, 25, 17, 9, 0 }; /*sv_maxusrcmdprocessticks*/
UsedConvar sv_maxusrcmdprocessticks(24, svmaxusrcmdprocessticksstr);
char *svmaxusercmdfutureticksstr = new char[28]{ 9, 12, 37, 23, 27, 2, 37, 15, 9, 31, 8, 25, 23, 30, 37, 28, 15, 14, 15, 8, 31, 37, 14, 19, 25, 17, 9, 0 }; /*sv_max_usercmd_future_ticks*/
UsedConvar sv_max_usercmd_future_ticks(27, svmaxusercmdfutureticksstr);
char *cl_pred_doresetlatchstr = new char[21]{ 25, 22, 37, 10, 8, 31, 30, 37, 30, 21, 8, 31, 9, 31, 14, 22, 27, 14, 25, 18, 0 }; /*cl_pred_doresetlatch*/
UsedConvar cl_pred_doresetlatch(20, cl_pred_doresetlatchstr);
char *svinfiniteammostr = new char[17]{ 9, 12, 37, 19, 20, 28, 19, 20, 19, 14, 31, 37, 27, 23, 23, 21, 0 }; /*sv_infinite_ammo*/
UsedConvar sv_infinite_ammo(16, svinfiniteammostr);
char *ff_damage_reduction_bulletsstr = new char[28]{ 28, 28, 37, 30, 27, 23, 27, 29, 31, 37, 8, 31, 30, 15, 25, 14, 19, 21, 20, 37, 24, 15, 22, 22, 31, 14, 9, 0 }; /*ff_damage_reduction_bullets*/
UsedConvar ff_damage_reduction_bullets(27, ff_damage_reduction_bulletsstr);
char *ff_damage_bullet_penetrationstr = new char[29]{ 28, 28, 37, 30, 27, 23, 27, 29, 31, 37, 24, 15, 22, 22, 31, 14, 37, 10, 31, 20, 31, 14, 8, 27, 14, 19, 21, 20, 0 }; /*ff_damage_bullet_penetration*/
UsedConvar ff_damage_bullet_penetration(28, ff_damage_bullet_penetrationstr);
char *sv_penetration_typestr = new char[20]{ 9, 12, 37, 10, 31, 20, 31, 14, 8, 27, 14, 19, 21, 20, 37, 14, 3, 10, 31, 0 }; /*sv_penetration_type*/
UsedConvar sv_penetration_type(19, sv_penetration_typestr);
char *sv_clip_penetration_traces_to_playersstr = new char[38]{ 9, 12, 37, 25, 22, 19, 10, 37, 10, 31, 20, 31, 14, 8, 27, 14, 19, 21, 20, 37, 14, 8, 27, 25, 31, 9, 37, 14, 21, 37, 10, 22, 27, 3, 31, 8, 9, 0 }; /*sv_clip_penetration_traces_to_players*/
UsedConvar sv_clip_penetration_traces_to_players(37, sv_clip_penetration_traces_to_playersstr);
char *weaponaccuracyshotgunspreadpatternsstr = new char[40]{ 13, 31, 27, 10, 21, 20, 37, 27, 25, 25, 15, 8, 27, 25, 3, 37, 9, 18, 21, 14, 29, 15, 20, 37, 9, 10, 8, 31, 27, 30, 37, 10, 27, 14, 14, 31, 8, 20, 9, 0 }; /*weapon_accuracy_shotgun_spread_patterns*/
UsedConvar weapon_accuracy_shotgun_spread_patterns(39, weaponaccuracyshotgunspreadpatternsstr);
char *sv_maxunlagstr = new char[12]{ 9, 12, 37, 23, 27, 2, 15, 20, 22, 27, 29, 0 }; /*sv_maxunlag*/
UsedConvar sv_maxunlag(11, sv_maxunlagstr);
char *mp_coopmission_dzstr = new char[18]{ 23, 10, 37, 25, 21, 21, 10, 23, 19, 9, 9, 19, 21, 20, 37, 30, 0, 0 }; /*mp_coopmission_dz*/
UsedConvar mp_coopmission_dz(17, mp_coopmission_dzstr);
char *sv_stepheightstr = new char[14]{ 9, 12, 37, 9, 14, 31, 10, 18, 31, 19, 29, 18, 14, 0 }; /*sv_stepheight*/
UsedConvar sv_stepheight(13, sv_stepheightstr);