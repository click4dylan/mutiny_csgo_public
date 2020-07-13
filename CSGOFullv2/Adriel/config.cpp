#include "config.hpp"
#include "adr_util.hpp"
#include "console.hpp"

#include <fstream>
#include <experimental/filesystem>
#include "custom_def.hpp"
#include "../VMProtectDefs.h"

std::string encrypt_decrypt(std::string to_encrypt)
{
	const auto key = 'Z';
	auto output = to_encrypt;

	for (auto i = 0; i < static_cast<int>(to_encrypt.size()); i++)
		output[i] = to_encrypt[i] ^ key;

	return output;
}

config_item* config::find(std::string str_catg, std::string str_name)
{
	for (auto i = 0; i < static_cast<int>(item.size()); i++)
	{
		if (item[i].str_catg == str_catg && item[i].str_name == str_name)
			return &item[i];
	}

	return nullptr;
}

config::config()
{
	//decrypts(0)
	str_file_end = XorStr("mutiny");
	//encrypts(0)

	//decrypts(0)
	str_dir = adr_util::get_disk() + XorStr("clientdata");
	//encrypts(0)

	CreateDirectory(str_dir.c_str(), nullptr);

	//decrypts(0)
	str_dir += XorStr("\\csgo");
	//encrypts(0)

	CreateDirectory(str_dir.c_str(), nullptr);
}

config::~config() = default;

void config::initialize()
{
	// UI
	auto& var = variable::get();

	//decrypts(0)
	setup_var(&var.ui.b_stream_proof, false, XorStr("User Interface"), XorStr("b_stream_proof"));
	setup_var(&var.ui.b_only_one, false, XorStr("User Interface"), XorStr("b_only_one"));
	setup_var(&var.ui.f_menu_time, 0.7f, XorStr("User Interface"), XorStr("f_menu_time"));
	setup_var(&var.ui.b_use_tooltips, true, XorStr("User Interface"), XorStr("b_use_tooltips"));
	setup_var(&var.ui.b_allow_manual_edits, false, XorStr("User Interface"), XorStr("b_allow_manual_edits"));
	setup_var(&var.ui.b_allow_untrusted, false, XorStr("User Interface"), XorStr("b_allow_untrusted"));

	//setup_var(&var.ui.col_controller, color_var(ImColor(0.95f, 0.95f, 0.95f, 1.f), false, 0.f), XorStr("User Interface"), XorStr("col_controller"));
	setup_var(&var.ui.col_controller, color_var(ImColor(0.15f, 0.60f, 0.78f, 1.f), false, 0.f), XorStr("User Interface"), XorStr("col_controller"));
	setup_var(&var.ui.col_text, color_var(ImColor(1.f, 1.f, 1.f, 1.f), false, 0.f), XorStr("User Interface"), XorStr("col_text"));
	setup_var(&var.ui.col_background, color_var(ImColor(1.f, 1.f, 1.f, 1.f), false, 0.f), XorStr("User Interface"), XorStr("col_background"));

	static std::vector<std::string> vec_grouped = {XorStr("Pistols"), XorStr("Smgs"), XorStr("Rifles"), XorStr("Shotguns"), XorStr("Snipers"), XorStr("Heavys"), "", ""};

	// RAGEBOT
	{
		setup_var(&var.ragebot.b_enabled, false, XorStr("Rage"), XorStr("b_enabled"));
		setup_var(&var.ragebot.b_multithreaded, true, XorStr("Rage"), XorStr("b_multithreaded"));
		setup_var(&var.ragebot.b_autofire, false, XorStr("Rage"), XorStr("b_autofire"));
		setup_var(&var.ragebot.b_autoscope, false, XorStr("Rage"), XorStr("b_autoscope"));
		setup_var(&var.ragebot.b_autostop, false, XorStr("Rage"), XorStr("b_autostop"));
		setup_var(&var.ragebot.b_antiaim, false, XorStr("Rage"), XorStr("b_antiaim"));
		setup_var(&var.ragebot.b_silent, true, XorStr("Rage"), XorStr("b_silent"));
		setup_var(&var.ragebot.b_resolver, true, XorStr("Rage"), XorStr("b_resolver"));
		setup_var(&var.ragebot.b_resolver_nojitter, bool_sw(false, 0, 0), XorStr("Rage"), XorStr("b_resolver_nojitter"));
		setup_var(&var.ragebot.b_resolver_nov6build, false, XorStr("Rage"), XorStr("b_resolver_nov6build"));
		setup_var(&var.ragebot.b_resolver_experimental, true, XorStr("Rage"), XorStr("b_resolver_experimental"));
		setup_var(&var.ragebot.b_resolver_moving, true, XorStr("Rage"), XorStr("b_resolver_moving"));
		setup_var(&var.ragebot.f_resolver_experimental_leniency_exclude, 1.1f, XorStr("Rage"), XorStr("f_resolver_experimental_leniency_exclude"));
		setup_var(&var.ragebot.f_resolver_experimental_leniency_impact, 0.0f, XorStr("Rage"), XorStr("f_resolver_experimental_leniency_impact"));
		setup_var(&var.ragebot.i_flipenemy_key, 0, XorStr("Rage"), XorStr("i_flipenemy_key"));
		setup_var(&var.ragebot.b_ignore_limbs_if_moving, true, XorStr("Rage"), XorStr("b_ignore_limbs_if_moving"));
		setup_var(&var.ragebot.f_ignore_limbs_if_moving, 0.1f, XorStr("Rage"), XorStr("f_ignore_limbs_if_moving"));
		setup_var(&var.ragebot.b_scan_through_teammates, false, XorStr("Rage"), XorStr("b_scan_through_teammates"));
		setup_var(&var.ragebot.b_make_all_misses_count, false, XorStr("Rage"), XorStr("b_make_all_misses_count"));
		setup_var(&var.ragebot.b_strict_hitboxes, false, XorStr("Rage"), XorStr("b_strict_hitboxes"));
		setup_var(&var.ragebot.b_forwardtrack, bool_sw(true, 0, 0), XorStr("Rage"), XorStr("b_forwardtrack"));
		setup_var(&var.ragebot.i_targets_per_tick, 2, XorStr("Rage"), XorStr("i_targets_per_tick"));
		setup_var(&var.ragebot.b_lagcomp_use_simtime, false, XorStr("Rage"), XorStr("b_lagcomp_use_simtime"));
		setup_var(&var.ragebot.b_use_alternative_multipoint, true, XorStr("Rage"), XorStr("b_use_alternative_multipoint"));
		setup_var(&var.ragebot.b_safe_point, true, XorStr("Rage"), XorStr("b_safe_point"));
		setup_var(&var.ragebot.b_safe_point_head, false, XorStr("Rage"), XorStr("b_safe_point_head"));

		setup_var(&var.ragebot.exploits.b_hide_shots, false, XorStr("Rage"), XorStr("b_hide_shots"));
		setup_var(&var.ragebot.exploits.b_hide_record, false, XorStr("Rage"), XorStr("b_hide_record"));
		setup_var(&var.ragebot.exploits.b_multi_tap, bool_sw(false, 0, 0), XorStr("Rage"), XorStr("b_multi_tap"));
		setup_var(&var.ragebot.exploits.b_nasa_walk, bool_sw(false, 0, 0), XorStr("Rage"), XorStr("b_nasa_walk"));
		setup_var(&var.ragebot.exploits.i_ticks_to_wait, 7, XorStr("Rage"), XorStr("i_ticks_to_wait"));
		setup_var(&var.ragebot.exploits.b_disable_fakelag_while_multitapping, true, XorStr("Rage"), XorStr("b_disable_fakelag_while_multitapping"));

		setup_var(&var.ragebot.i_sort, 0, XorStr("Rage"), XorStr("i_sort"));

		setup_var(&var.ragebot.b_advanced_multipoint, false, XorStr("Rage"), XorStr("b_advanced_multipoint"));

		setup_var(&var.ragebot.i_mindmg, 12, XorStr("Rage"), XorStr("i_mindmg"));
		setup_var(&var.ragebot.i_mindmg_aw, 12, XorStr("Rage"), XorStr("i_mindmg_aw"));

		setup_var(&var.ragebot.f_hitchance, 65.f, XorStr("Rage"), XorStr("f_hitchance"));
		setup_var(&var.ragebot.f_body_hitchance, 55.f, XorStr("Rage"), XorStr("f_body_hitchance"));

		// rage - hitscan
		setup_var(&var.ragebot.hitscan_head.b_enabled, false, XorStr("Rage - Hitscan - Head"), XorStr("b_enabled"));
		setup_var(&var.ragebot.hitscan_head.b_multipoint, false, XorStr("Rage - Hitscan - Head"), XorStr("b_multipoint"));
		setup_var(&var.ragebot.hitscan_head.f_pointscale, 65.f, XorStr("Rage - Hitscan - Head"), XorStr("f_pointscale"));
		setup_var(&var.ragebot.hitscan_head.i_priority, 5, XorStr("Rage - Hitscan - Head"), XorStr("i_priority"));

		setup_var(&var.ragebot.hitscan_shoulders.b_enabled, false, XorStr("Rage - Hitscan - Shoulders"), XorStr("b_enabled"));
		setup_var(&var.ragebot.hitscan_shoulders.b_multipoint, false, XorStr("Rage - Hitscan - Shoulders"), XorStr("b_multipoint"));
		setup_var(&var.ragebot.hitscan_shoulders.f_pointscale, 55.f, XorStr("Rage - Hitscan - Shoulders"), XorStr("f_pointscale"));
		setup_var(&var.ragebot.hitscan_shoulders.i_priority, 3, XorStr("Rage - Hitscan - Shoulders"), XorStr("i_priority"));

		setup_var(&var.ragebot.hitscan_hands.b_enabled, false, XorStr("Rage - Hitscan - Hands"), XorStr("b_enabled"));
		setup_var(&var.ragebot.hitscan_hands.b_multipoint, false, XorStr("Rage - Hitscan - Hands"), XorStr("b_multipoint"));
		setup_var(&var.ragebot.hitscan_hands.f_pointscale, 55.f, XorStr("Rage - Hitscan - Hands"), XorStr("f_pointscale"));
		setup_var(&var.ragebot.hitscan_hands.i_priority, 3, XorStr("Rage - Hitscan - Hands"), XorStr("i_priority"));

		setup_var(&var.ragebot.hitscan_chest.b_enabled, false, XorStr("Rage - Hitscan - Chest"), XorStr("b_enabled"));
		setup_var(&var.ragebot.hitscan_chest.b_multipoint, false, XorStr("Rage - Hitscan - Chest"), XorStr("b_multipoint"));
		setup_var(&var.ragebot.hitscan_chest.f_pointscale, 55.f, XorStr("Rage - Hitscan - Chest"), XorStr("f_pointscale"));
		setup_var(&var.ragebot.hitscan_chest.i_priority, 3, XorStr("Rage - Hitscan - Chest"), XorStr("i_priority"));

		setup_var(&var.ragebot.hitscan_stomach.b_enabled, false, XorStr("Rage - Hitscan - Stomach"), XorStr("b_enabled"));
		setup_var(&var.ragebot.hitscan_stomach.b_multipoint, false, XorStr("Rage - Hitscan - Stomach"), XorStr("b_multipoint"));
		setup_var(&var.ragebot.hitscan_stomach.f_pointscale, 55.f, XorStr("Rage - Hitscan - Stomach"), XorStr("f_pointscale"));
		setup_var(&var.ragebot.hitscan_stomach.i_priority, 4, XorStr("Rage - Hitscan - Stomach"), XorStr("i_priority"));

		setup_var(&var.ragebot.hitscan_arms.b_enabled, false, XorStr("Rage - Hitscan - Arms"), XorStr("b_enabled"));
		setup_var(&var.ragebot.hitscan_arms.b_multipoint, false, XorStr("Rage - Hitscan - Arms"), XorStr("b_multipoint"));
		setup_var(&var.ragebot.hitscan_arms.f_pointscale, 55.f, XorStr("Rage - Hitscan - Arms"), XorStr("f_pointscale"));
		setup_var(&var.ragebot.hitscan_arms.i_priority, 1, XorStr("Rage - Hitscan - Arms"), XorStr("i_priority"));

		setup_var(&var.ragebot.hitscan_legs.b_enabled, false, XorStr("Rage - Hitscan - Thighs"), XorStr("b_enabled"));
		setup_var(&var.ragebot.hitscan_legs.b_multipoint, false, XorStr("Rage - Hitscan - Thighs"), XorStr("b_multipoint"));
		setup_var(&var.ragebot.hitscan_legs.f_pointscale, 55.f, XorStr("Rage - Hitscan - Thighs"), XorStr("f_pointscale"));
		setup_var(&var.ragebot.hitscan_legs.i_priority, 2, XorStr("Rage - Hitscan - Thighs"), XorStr("i_priority"));

		setup_var(&var.ragebot.hitscan_feet.b_enabled, false, XorStr("Rage - Hitscan - Feet"), XorStr("b_enabled"));
		setup_var(&var.ragebot.hitscan_feet.b_multipoint, false, XorStr("Rage - Hitscan - Feet"), XorStr("b_multipoint"));
		setup_var(&var.ragebot.hitscan_feet.f_pointscale, 55.f, XorStr("Rage - Hitscan - Feet"), XorStr("f_pointscale"));
		setup_var(&var.ragebot.hitscan_feet.i_priority, 0, XorStr("Rage - Hitscan - Feet"), XorStr("i_priority"));

		// rage - baim
		setup_var(&var.ragebot.baim_main.b_enabled, false, XorStr("Rage - Baim"), XorStr("b_enabled"));
		setup_var(&var.ragebot.baim_main.b_force, bool_sw(false, 0, 0), XorStr("Rage - Baim"), XorStr("b_force"));
		setup_var(&var.ragebot.baim_main.b_lethal, false, XorStr("Rage - Baim"), XorStr("b_lethal"));
		setup_var(&var.ragebot.baim_main.b_cant_resolve, false, XorStr("Rage - Baim"), XorStr("b_cant_resolve"));
		setup_var(&var.ragebot.baim_main.b_not_body_hit_resolved, false, XorStr("Rage - Baim"), XorStr("b_not_body_hit_resolved"));
		setup_var(&var.ragebot.baim_main.b_airborne_target, false, XorStr("Rage - Baim"), XorStr("b_airborne_target"));
		setup_var(&var.ragebot.baim_main.b_airborne_local, false, XorStr("Rage - Baim"), XorStr("b_airborne_local"));
		setup_var(&var.ragebot.baim_main.b_moving_target, false, XorStr("Rage - Baim"), XorStr("b_moving_target"));
		setup_var(&var.ragebot.baim_main.b_after_misses, false, XorStr("Rage - Baim"), XorStr("b_after_misses"));
		setup_var(&var.ragebot.baim_main.b_after_health, false, XorStr("Rage - Baim"), XorStr("b_after_health"));

		setup_var(&var.ragebot.baim_main.i_after_misses, 1, XorStr("Rage - Baim"), XorStr("i_after_misses"));
		setup_var(&var.ragebot.baim_main.f_moving_target, 60.f, XorStr("Rage - Baim"), XorStr("f_moving_target"));
		setup_var(&var.ragebot.baim_main.body_aim_health, 10.f, XorStr("Rage - Baim"), XorStr("body_aim_health"));


		// rage - fakelag
		setup_var(&var.ragebot.i_fakelag_mode, 0, XorStr("Rage - Fakelag"), XorStr("i_fakelag_mode"));
		setup_var(&var.ragebot.i_fakelag_ticks, 0, XorStr("Rage - Fakelag"), XorStr("i_fakelag_ticks"));
		setup_var(&var.ragebot.b_fakelag_peek, false, XorStr("Rage - Fakelag"), XorStr("b_fakelag_peek"));
		setup_var(&var.ragebot.b_fakelag_land, false, XorStr("Rage - Fakelag"), XorStr("b_fakelag_land"));

		// rage - hvh
		setup_var(&var.ragebot.b_freestand_dormant, false, XorStr("Rage - HVH"), XorStr("b_freestand_dormant"));
		setup_var(&var.ragebot.f_freestanding_walldist, 25.f, XorStr("Rage - HVH"), XorStr("f_freestanding_walldist"));

		setup_var(&var.ragebot.b_auto_backwards, false, XorStr("Rage - HVH"), XorStr("b_auto_backwards"));
		setup_var(&var.ragebot.i_auto_backwards, 1, XorStr("Rage - HVH"), XorStr("i_auto_backwards"));

		setup_var(&var.ragebot.f_fake_latency, 0.f, XorStr("Rage - HVH"), XorStr("f_fake_latency"));

		setup_var(&var.ragebot.i_double_tap, 0, XorStr("Rage - HVH"), XorStr("i_double_tap"));

		setup_var(&var.ragebot.i_manual_aa_left_key, VK_LEFT, XorStr("Rage - HVH"), XorStr("i_manual_aa_left_key"));
		setup_var(&var.ragebot.i_manual_aa_right_key, VK_RIGHT, XorStr("Rage - HVH"), XorStr("i_manual_aa_right_key"));
		setup_var(&var.ragebot.i_manual_aa_back_key, VK_DOWN, XorStr("Rage - HVH"), XorStr("i_manual_aa_back_key"));
		setup_var(&var.ragebot.i_manual_aa_lean_dir, VK_MENU, XorStr("Rage - HVH"), XorStr("i_manual_aa_lean_dir"));

		setup_var(&var.ragebot.i_override_key, 0, XorStr("Rage - HVH"), XorStr("i_override_key"));

		// rage - hvh - antiaim
		setup_var(&var.ragebot.standing.b_desync, false, XorStr("Rage - HVH - Standing"), XorStr("b_desync"));
		setup_var(&var.ragebot.standing.b_apply_sway, false, XorStr("Rage - HVH - Standing"), XorStr("b_apply_sway"));
		setup_var(&var.ragebot.standing.b_sway_wait, false, XorStr("Rage - HVH - Standing"), XorStr("b_sway_wait"));
		setup_var(&var.ragebot.standing.f_sway_min, 0.1f, XorStr("Rage - HVH - Standing"), XorStr("f_sway_min"));
		setup_var(&var.ragebot.standing.f_sway_max, 0.25f, XorStr("Rage - HVH - Standing"), XorStr("f_sway_max"));
		setup_var(&var.ragebot.standing.f_desync_amt, 120.0f, XorStr("Rage - HVH - Standing"), XorStr("f_desync_amt"));
		setup_var(&var.ragebot.standing.f_sway_speed, 5.0f, XorStr("Rage - HVH - Standing"), XorStr("f_sway_speed"));
		setup_var(&var.ragebot.standing.f_desync_amt_fast, 120.0f, XorStr("Rage - HVH - Standing"), XorStr("f_desync_amt_fast"));
		setup_var(&var.ragebot.standing.f_lby_delta, 160.0f, XorStr("Rage - HVH - Standing"), XorStr("f_lby_delta"));
		setup_var(&var.ragebot.standing.b_jitter, false, XorStr("Rage - HVH - Standing"), XorStr("b_jitter"));
		setup_var(&var.ragebot.standing.f_jitter, 0.f, XorStr("Rage - HVH - Standing"), XorStr("f_jitter"));
		setup_var(&var.ragebot.standing.f_jitter_fast, 0.f, XorStr("Rage - HVH - Standing"), XorStr("f_jitter_fast"));
		setup_var(&var.ragebot.standing.f_spin_speed, 0.f, XorStr("Rage - HVH - Standing"), XorStr("f_spin_speed"));
		setup_var(&var.ragebot.standing.f_yaw_offset, 180.f, XorStr("Rage - HVH - Standing"), XorStr("f_yaw_offset"));
		setup_var(&var.ragebot.standing.i_desync_style, 0, XorStr("Rage - HVH - Standing"), XorStr("i_desync_style"));
		setup_var(&var.ragebot.standing.i_pitch, 0, XorStr("Rage - HVH - Standing"), XorStr("i_pitch"));
		setup_var(&var.ragebot.standing.i_yaw, 0, XorStr("Rage - HVH - Standing"), XorStr("i_yaw"));
		setup_var(&var.ragebot.standing.f_custom_pitch, 89.f, XorStr("Rage - HVH - Standing"), XorStr("f_custom_pitch"));
		setup_var(&var.ragebot.standing.b_apply_freestanding, false, XorStr("Rage - HVH - Standing"), XorStr("b_apply_freestanding"));
		setup_var(&var.ragebot.standing.b_apply_walldtc, false, XorStr("Rage - HVH - Standing"), XorStr("b_apply_walldtc"));
		setup_var(&var.ragebot.standing.b_break_lby_by_flicking, true, XorStr("Rage - HVH - Standing"), XorStr("b_break_lby_by_flicking"));

		setup_var(&var.ragebot.standing.fakelag.b_enabled, false, XorStr("Rage - Fakelag - Standing"), XorStr("b_enabled"));
		setup_var(&var.ragebot.standing.fakelag.b_disrupt, false, XorStr("Rage - Fakelag - Standing"), XorStr("b_disrupt"));
		setup_var(&var.ragebot.standing.fakelag.i_disable_flags, 0, XorStr("Rage - Fakelag - Standing"), XorStr("i_disable_flags"));
		setup_var(&var.ragebot.standing.fakelag.f_disrupt_chance, 50.f, XorStr("Rage - Fakelag - Standing"), XorStr("f_disrupt_chance"));
		setup_var(&var.ragebot.standing.fakelag.i_max_ticks, 16, XorStr("Rage - Fakelag - Standing"), XorStr("i_max_ticks"));
		setup_var(&var.ragebot.standing.fakelag.i_min_ticks, 1, XorStr("Rage - Fakelag - Standing"), XorStr("i_min_ticks"));
		setup_var(&var.ragebot.standing.fakelag.i_mode, 0, XorStr("Rage - Fakelag - Standing"), XorStr("i_mode"));
		setup_var(&var.ragebot.standing.fakelag.i_static_ticks, 1, XorStr("Rage - Fakelag - Standing"), XorStr("i_static_ticks"));
		setup_var(&var.ragebot.standing.fakelag.i_trigger_flags, 0, XorStr("Rage - Fakelag - Standing"), XorStr("i_trigger_flags"));



		setup_var(&var.ragebot.moving.b_desync, false, XorStr("Rage - HVH - Moving"), XorStr("b_desync"));
		setup_var(&var.ragebot.moving.b_apply_sway, false, XorStr("Rage - HVH - Moving"), XorStr("b_apply_sway"));
		setup_var(&var.ragebot.moving.b_sway_wait, false, XorStr("Rage - HVH - Moving"), XorStr("b_sway_wait"));
		setup_var(&var.ragebot.moving.f_sway_min, 0.1f, XorStr("Rage - HVH - Moving"), XorStr("f_sway_min"));
		setup_var(&var.ragebot.moving.f_sway_max, 0.25f, XorStr("Rage - HVH - Moving"), XorStr("f_sway_max"));
		setup_var(&var.ragebot.moving.f_desync_amt, 120.0f, XorStr("Rage - HVH - Moving"), XorStr("f_desync_amt"));
		setup_var(&var.ragebot.moving.f_sway_speed, 5.0f, XorStr("Rage - HVH - Moving"), XorStr("f_sway_speed"));
		setup_var(&var.ragebot.moving.f_desync_amt_fast, 120.0f, XorStr("Rage - HVH - Moving"), XorStr("f_desync_amt_fast"));
		setup_var(&var.ragebot.moving.f_lby_delta, 160.0f, XorStr("Rage - HVH - Moving"), XorStr("f_lby_delta"));
		setup_var(&var.ragebot.moving.b_jitter, false, XorStr("Rage - HVH - Moving"), XorStr("b_jitter"));
		setup_var(&var.ragebot.moving.f_jitter, 0.f, XorStr("Rage - HVH - Moving"), XorStr("f_jitter"));
		setup_var(&var.ragebot.moving.f_jitter_fast, 0.f, XorStr("Rage - HVH - Moving"), XorStr("f_jitter_fast"));
		setup_var(&var.ragebot.moving.f_spin_speed, 0.f, XorStr("Rage - HVH - Moving"), XorStr("f_spin_speed"));
		setup_var(&var.ragebot.moving.f_yaw_offset, 180.f, XorStr("Rage - HVH - Moving"), XorStr("f_yaw_offset"));
		setup_var(&var.ragebot.moving.i_desync_style, 0, XorStr("Rage - HVH - Moving"), XorStr("i_desync_style"));
		setup_var(&var.ragebot.moving.i_pitch, 0, XorStr("Rage - HVH - Moving"), XorStr("i_pitch"));
		setup_var(&var.ragebot.moving.i_yaw, 0, XorStr("Rage - HVH - Moving"), XorStr("i_yaw"));
		setup_var(&var.ragebot.moving.f_custom_pitch, 89.f, XorStr("Rage - HVH - Moving"), XorStr("f_custom_pitch"));
		setup_var(&var.ragebot.moving.b_apply_freestanding, false, XorStr("Rage - HVH - Moving"), XorStr("b_apply_freestanding"));
		setup_var(&var.ragebot.moving.b_apply_walldtc, false, XorStr("Rage - HVH - Moving"), XorStr("b_apply_walldtc"));

		setup_var(&var.ragebot.moving.fakelag.b_enabled, false, XorStr("Rage - Fakelag - Moving"), XorStr("b_enabled"));
		setup_var(&var.ragebot.moving.fakelag.b_disrupt, false, XorStr("Rage - Fakelag - Moving"), XorStr("b_disrupt"));
		setup_var(&var.ragebot.moving.fakelag.i_disable_flags, 0, XorStr("Rage - Fakelag - Moving"), XorStr("i_disable_flags"));
		setup_var(&var.ragebot.moving.fakelag.f_disrupt_chance, 50.f, XorStr("Rage - Fakelag - Moving"), XorStr("f_disrupt_chance"));
		setup_var(&var.ragebot.moving.fakelag.i_max_ticks, 16, XorStr("Rage - Fakelag - Moving"), XorStr("i_max_ticks"));
		setup_var(&var.ragebot.moving.fakelag.i_min_ticks, 1, XorStr("Rage - Fakelag - Moving"), XorStr("i_min_ticks"));
		setup_var(&var.ragebot.moving.fakelag.i_mode, 0, XorStr("Rage - Fakelag - Moving"), XorStr("i_mode"));
		setup_var(&var.ragebot.moving.fakelag.i_static_ticks, 1, XorStr("Rage - Fakelag - Moving"), XorStr("i_static_ticks"));
		setup_var(&var.ragebot.moving.fakelag.i_trigger_flags, 0, XorStr("Rage - Fakelag - Moving"), XorStr("i_trigger_flags"));



		setup_var(&var.ragebot.minwalking.b_desync, false, XorStr("Rage - HVH - Minwalking"), XorStr("b_desync"));
		setup_var(&var.ragebot.minwalking.b_apply_sway, false, XorStr("Rage - HVH - Minwalking"), XorStr("b_apply_sway"));
		setup_var(&var.ragebot.minwalking.b_sway_wait, false, XorStr("Rage - HVH - Minwalking"), XorStr("b_sway_wait"));
		setup_var(&var.ragebot.minwalking.f_sway_min, 0.1f, XorStr("Rage - HVH - Minwalking"), XorStr("f_sway_min"));
		setup_var(&var.ragebot.minwalking.f_sway_max, 0.25f, XorStr("Rage - HVH - Minwalking"), XorStr("f_sway_max"));
		setup_var(&var.ragebot.minwalking.f_desync_amt, 120.0f, XorStr("Rage - HVH - Minwalking"), XorStr("f_desync_amt"));
		setup_var(&var.ragebot.minwalking.f_sway_speed, 5.0f, XorStr("Rage - HVH - Minwalking"), XorStr("f_sway_speed"));
		setup_var(&var.ragebot.minwalking.f_desync_amt_fast, 120.0f, XorStr("Rage - HVH - Minwalking"), XorStr("f_desync_amt_fast"));
		setup_var(&var.ragebot.minwalking.f_lby_delta, 160.0f, XorStr("Rage - HVH - Minwalking"), XorStr("f_lby_delta"));
		setup_var(&var.ragebot.minwalking.b_jitter, false, XorStr("Rage - HVH - Minwalking"), XorStr("b_jitter"));
		setup_var(&var.ragebot.minwalking.f_jitter, 0.f, XorStr("Rage - HVH - Minwalking"), XorStr("f_jitter"));
		setup_var(&var.ragebot.minwalking.f_jitter_fast, 0.f, XorStr("Rage - HVH - Minwalking"), XorStr("f_jitter_fast"));
		setup_var(&var.ragebot.minwalking.f_spin_speed, 0.f, XorStr("Rage - HVH - Minwalking"), XorStr("f_spin_speed"));
		setup_var(&var.ragebot.minwalking.f_yaw_offset, 180.f, XorStr("Rage - HVH - Minwalking"), XorStr("f_yaw_offset"));
		setup_var(&var.ragebot.minwalking.i_desync_style, 0, XorStr("Rage - HVH - Minwalking"), XorStr("i_desync_style"));
		setup_var(&var.ragebot.minwalking.i_pitch, 0, XorStr("Rage - HVH - Minwalking"), XorStr("i_pitch"));
		setup_var(&var.ragebot.minwalking.i_yaw, 0, XorStr("Rage - HVH - Minwalking"), XorStr("i_yaw"));
		setup_var(&var.ragebot.minwalking.f_custom_pitch, 89.f, XorStr("Rage - HVH - Minwalking"), XorStr("f_custom_pitch"));
		setup_var(&var.ragebot.minwalking.b_apply_freestanding, false, XorStr("Rage - HVH - Minwalking"), XorStr("b_apply_freestanding"));
		setup_var(&var.ragebot.minwalking.b_apply_walldtc, false, XorStr("Rage - HVH - Minwalking"), XorStr("b_apply_walldtc"));

		setup_var(&var.ragebot.minwalking.fakelag.b_enabled, false, XorStr("Rage - Fakelag - Minwalking"), XorStr("b_enabled"));
		setup_var(&var.ragebot.minwalking.fakelag.b_disrupt, false, XorStr("Rage - Fakelag - Minwalking"), XorStr("b_disrupt"));
		setup_var(&var.ragebot.minwalking.fakelag.i_disable_flags, 0, XorStr("Rage - Fakelag - Minwalking"), XorStr("i_disable_flags"));
		setup_var(&var.ragebot.minwalking.fakelag.f_disrupt_chance, 50.f, XorStr("Rage - Fakelag - Minwalking"), XorStr("f_disrupt_chance"));
		setup_var(&var.ragebot.minwalking.fakelag.i_max_ticks, 16, XorStr("Rage - Fakelag - Minwalking"), XorStr("i_max_ticks"));
		setup_var(&var.ragebot.minwalking.fakelag.i_min_ticks, 1, XorStr("Rage - Fakelag - Minwalking"), XorStr("i_min_ticks"));
		setup_var(&var.ragebot.minwalking.fakelag.i_mode, 0, XorStr("Rage - Fakelag - Minwalking"), XorStr("i_mode"));
		setup_var(&var.ragebot.minwalking.fakelag.i_static_ticks, 1, XorStr("Rage - Fakelag - Minwalking"), XorStr("i_static_ticks"));
		setup_var(&var.ragebot.minwalking.fakelag.i_trigger_flags, 0, XorStr("Rage - Fakelag - Minwalking"), XorStr("i_trigger_flags"));



		setup_var(&var.ragebot.in_air.b_desync, false, XorStr("Rage - HVH - Airborne"), XorStr("b_desync"));
		setup_var(&var.ragebot.in_air.b_apply_sway, false, XorStr("Rage - HVH - Airborne"), XorStr("b_apply_sway"));
		setup_var(&var.ragebot.in_air.b_sway_wait, false, XorStr("Rage - HVH - Airborne"), XorStr("b_sway_wait"));
		setup_var(&var.ragebot.in_air.f_sway_min, 0.1f, XorStr("Rage - HVH - Airborne"), XorStr("f_sway_min"));
		setup_var(&var.ragebot.in_air.f_sway_max, 0.25f, XorStr("Rage - HVH - Airborne"), XorStr("f_sway_max"));
		setup_var(&var.ragebot.in_air.f_desync_amt, 120.0f, XorStr("Rage - HVH - Airborne"), XorStr("f_desync_amt"));
		setup_var(&var.ragebot.in_air.f_sway_speed, 5.0f, XorStr("Rage - HVH - Airborne"), XorStr("f_sway_speed"));
		setup_var(&var.ragebot.in_air.f_desync_amt_fast, 120.0f, XorStr("Rage - HVH - Airborne"), XorStr("f_desync_amt_fast"));
		setup_var(&var.ragebot.in_air.f_lby_delta, 160.0f, XorStr("Rage - HVH - Airborne"), XorStr("f_lby_delta"));
		setup_var(&var.ragebot.in_air.b_jitter, false, XorStr("Rage - HVH - Airborne"), XorStr("b_jitter"));
		setup_var(&var.ragebot.in_air.f_jitter, 0.f, XorStr("Rage - HVH - Airborne"), XorStr("f_jitter"));
		setup_var(&var.ragebot.in_air.f_jitter_fast, 0.f, XorStr("Rage - HVH - Airborne"), XorStr("f_jitter_fast"));
		setup_var(&var.ragebot.in_air.f_spin_speed, 0.f, XorStr("Rage - HVH - Airborne"), XorStr("f_spin_speed"));
		setup_var(&var.ragebot.in_air.f_yaw_offset, 180.f, XorStr("Rage - HVH - Airborne"), XorStr("f_yaw_offset"));
		setup_var(&var.ragebot.in_air.i_desync_style, 0, XorStr("Rage - HVH - Airborne"), XorStr("i_desync_style"));
		setup_var(&var.ragebot.in_air.i_pitch, 0, XorStr("Rage - HVH - Airborne"), XorStr("i_pitch"));
		setup_var(&var.ragebot.in_air.i_yaw, 0, XorStr("Rage - HVH - Airborne"), XorStr("i_yaw"));
		setup_var(&var.ragebot.in_air.f_custom_pitch, 89.f, XorStr("Rage - HVH - Airborne"), XorStr("f_custom_pitch"));
		setup_var(&var.ragebot.in_air.b_apply_freestanding, false, XorStr("Rage - HVH - Airborne"), XorStr("b_apply_freestanding"));
		setup_var(&var.ragebot.in_air.b_apply_walldtc, false, XorStr("Rage - HVH - Airborne"), XorStr("b_apply_walldtc"));

		setup_var(&var.ragebot.in_air.fakelag.b_enabled, false, XorStr("Rage - Fakelag - Airborne"), XorStr("b_enabled"));
		setup_var(&var.ragebot.in_air.fakelag.b_disrupt, false, XorStr("Rage - Fakelag - Airborne"), XorStr("b_disrupt"));
		setup_var(&var.ragebot.in_air.fakelag.i_disable_flags, 0, XorStr("Rage - Fakelag - Airborne"), XorStr("i_disable_flags"));
		setup_var(&var.ragebot.in_air.fakelag.f_disrupt_chance, 50.f, XorStr("Rage - Fakelag - Airborne"), XorStr("f_disrupt_chance"));
		setup_var(&var.ragebot.in_air.fakelag.i_max_ticks, 16, XorStr("Rage - Fakelag - Airborne"), XorStr("i_max_ticks"));
		setup_var(&var.ragebot.in_air.fakelag.i_min_ticks, 1, XorStr("Rage - Fakelag - Airborne"), XorStr("i_min_ticks"));
		setup_var(&var.ragebot.in_air.fakelag.i_mode, 0, XorStr("Rage - Fakelag - Airborne"), XorStr("i_mode"));
		setup_var(&var.ragebot.in_air.fakelag.i_static_ticks, 1, XorStr("Rage - Fakelag - Airborne"), XorStr("i_static_ticks"));
		setup_var(&var.ragebot.in_air.fakelag.i_trigger_flags, 0, XorStr("Rage - Fakelag - Airborne"), XorStr("i_trigger_flags"));

	}
	//encrypts(0)

	// LEGITBOT
	{
		//decrypts(0)
		setup_var(&var.legitbot.aim.b_enabled, bool_sw(false, 0, 0), XorStr("Aim - Legit"), XorStr("i_type"));
		setup_var(&var.legitbot.aim.b_lag_compensation, false, XorStr("Aim - Legit"), XorStr("b_lag_compensation"));
		setup_var(&var.legitbot.aim.b_per_weapon, false, XorStr("Aim - Legit"), XorStr("b_per_weapon"));
		//encrypts(0)
		for (auto i = 0; i < static_cast<int>(var.legitbot.aim_cfg.size()); i++)
		{
			std::string str_name = "";
			if (i < 66)
			{
				if (adr_util::is_knife((ItemDefinitionIndex)(i)) || adr_util::is_utility((ItemDefinitionIndex)(i)) || adr_util::is_glove((ItemDefinitionIndex)(i)))
					continue;

				str_name = adr_util::get_weapon_name((ItemDefinitionIndex)(i));
				//decrypts(0)
				std::string cmp = XorStr("knife");
				//encrypts(0)
				if (str_name.compare(cmp) == 0)
					continue;
			}
			else
			{
				str_name = vec_grouped[i - 66];
				if (str_name.empty())
					continue;
			}

			//decrypts(0)
			setup_var(&var.legitbot.aim_cfg[i].f_fov, 0.f, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("f_fov"));
			setup_var(&var.legitbot.aim_cfg[i].b_incross, false, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("b_incross"));
			setup_var(&var.legitbot.aim_cfg[i].b_randomize_rcs, false, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("b_randomize_rcs"));
			setup_var(&var.legitbot.aim_cfg[i].b_randomize_speed, false, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("b_randomize_speed"));
			setup_var(&var.legitbot.aim_cfg[i].b_dynamic, false, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("b_dynamic"));
			setup_var(&var.legitbot.aim_cfg[i].i_retarget_time, 0, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("i_retarget_time"));
			setup_var(&var.legitbot.aim_cfg[i].f_speed, 0.f, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("f_speed"));
			setup_var(&var.legitbot.aim_cfg[i].f_y_rcs, 0.f, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("f_y_rcs"));
			setup_var(&var.legitbot.aim_cfg[i].f_x_rcs, 0.f, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("f_x_rcs"));
			setup_var(&var.legitbot.aim_cfg[i].b_autowall, false, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("b_autowall"));
			setup_var(&var.legitbot.aim_cfg[i].i_activation_shots, 0, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("i_activation_shots"));
			setup_var(&var.legitbot.aim_cfg[i].i_slow_moving, 0, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("i_slow_moving"));
			setup_var(&var.legitbot.aim_cfg[i].b_auto_pistol, false, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("b_auto_pistol"));
			setup_var(&var.legitbot.aim_cfg[i].b_check_scope, false, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("b_check_scope"));
			setup_var(&var.legitbot.aim_cfg[i].b_check_smoke, false, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("b_check_smoke"));
			setup_var(&var.legitbot.aim_cfg[i].b_check_flash, false, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("b_check_flash"));
			setup_var(&var.legitbot.aim_cfg[i].b_silentaim, false, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("b_silentaim"));
			setup_var(&var.legitbot.aim_cfg[i].b_lock, false, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("b_lock"));
			setup_var(&var.legitbot.aim_cfg[i].b_auto_scope, false, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("b_auto_scope"));
			setup_var(&var.legitbot.aim_cfg[i].b_auto_stop, false, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("b_auto_stop"));
			setup_var(&var.legitbot.aim_cfg[i].b_auto_shoot, false, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("b_auto_shoot"));
			setup_var(&var.legitbot.aim_cfg[i].b_skip_incross, false, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("b_skip_incross"));
			setup_var(&var.legitbot.aim_cfg[i].f_hitchance, 0.f, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("f_hitchance"));
			setup_var(&var.legitbot.aim_cfg[i].b_first_shot_assist, false, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("b_first_shot_assist"));
			setup_var(&var.legitbot.aim_cfg[i].b_skip_incross, false, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("b_skip_incross"));
			setup_var(&var.legitbot.aim_cfg[i].b_aim_step, false, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("b_aim_step"));
			setup_var(&var.legitbot.aim_cfg[i].b_wait_for_in_attack, false, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("b_wait_for_in_attack"));
			setup_var(&var.legitbot.aim_cfg[i].i_speed_type, 0, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("i_speed_type"));
			setup_var(&var.legitbot.aim_cfg[i].b_aim_time, false, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("b_aim_time"));

			setup_var(&var.legitbot.aim_cfg[i].b_standalone_rcs, false, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("b_standalone_rcs"));
			setup_var(&var.legitbot.aim_cfg[i].b_standalone_rcs_randomize_speed, false, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("b_standalone_rcs_randomize_speed"));
			setup_var(&var.legitbot.aim_cfg[i].b_standalone_rcs_randomize_rcs, false, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("b_standalone_rcs_randomize_rcs"));
			setup_var(&var.legitbot.aim_cfg[i].b_standalone_rcs_after_kill, false, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("b_standalone_rcs_after_kill"));
			setup_var(&var.legitbot.aim_cfg[i].f_standalone_rcs_speed, 0.f, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("f_standalone_rcs_speed"));
			setup_var(&var.legitbot.aim_cfg[i].f_y_standalone_rcs, 0.f, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("f_y_standalone_rcs"));
			setup_var(&var.legitbot.aim_cfg[i].f_x_standalone_rcs, 0.f, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), XorStr("f_x_standalone_rcs"));
			//setup_var( &var.legitbot.aim_cfg[ i ].i_standalone_rcs_activation_shots, 0, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str() ), XorStr( "i_standalone_rcs_activation_shots" ) );
			//encrypts(0)

			//decrypts(0)
			for (auto j = 0; j < static_cast<int>(var.legitbot.aim_cfg[i].hitboxes.size()); j++)
				setup_var(&std::get<2>(var.legitbot.aim_cfg[i].hitboxes[j]), false, adr_util::string::format(XorStr("Aim - Legit - %s"), str_name.c_str()), adr_util::string::format(XorStr("Hitbox - %s"), std::get<0>(var.legitbot.aim_cfg[i].hitboxes[j]).c_str()));
			//encrypts(0)
		}
	}

	// TRIGGERBOT
	{
		//decrypts(0)
		setup_var(&var.legitbot.trigger.b_enabled, bool_sw(false, 0, 0), XorStr("Aim - Trigger"), XorStr("i_type"));

		for (auto i = 0; i < static_cast<int>(var.legitbot.trigger_cfg.size()); i++)
		{
			auto str_name = vec_grouped[i];
			if (str_name.empty())
				continue;

			setup_var(&var.legitbot.trigger_cfg[i].i_delay, 0, adr_util::string::format(XorStr("Aim - Trigger - %s"), str_name.c_str()), XorStr("i_delay"));
			setup_var(&var.legitbot.trigger_cfg[i].f_hitchance, 0.f, adr_util::string::format(XorStr("Aim - Trigger - %s"), str_name.c_str()), XorStr("f_hitchance"));
			setup_var(&var.legitbot.trigger_cfg[i].b_auto_scope, false, adr_util::string::format(XorStr("Aim - Trigger - %s"), str_name.c_str()), XorStr("b_auto_scope"));
			setup_var(&var.legitbot.trigger_cfg[i].b_check_smoke, false, adr_util::string::format(XorStr("Aim - Trigger - %s"), str_name.c_str()), XorStr("b_check_smoke"));
			setup_var(&var.legitbot.trigger_cfg[i].b_autowall, false, adr_util::string::format(XorStr("Aim - Trigger - %s"), str_name.c_str()), XorStr("b_autowall"));
			setup_var(&var.legitbot.trigger_cfg[i].b_check_flash, false, adr_util::string::format(XorStr("Aim - Trigger - %s"), str_name.c_str()), XorStr("b_check_flash"));
			setup_var(&var.legitbot.trigger_cfg[i].b_check_scope, false, adr_util::string::format(XorStr("Aim - Trigger - %s"), str_name.c_str()), XorStr("b_check_scope"));
			setup_var(&var.legitbot.trigger_cfg[i].b_filter_head, false, adr_util::string::format(XorStr("Aim - Trigger - %s"), str_name.c_str()), XorStr("b_filter_head"));
			setup_var(&var.legitbot.trigger_cfg[i].b_filter_chest, false, adr_util::string::format(XorStr("Aim - Trigger - %s"), str_name.c_str()), XorStr("b_filter_chest"));
			setup_var(&var.legitbot.trigger_cfg[i].b_filter_stomach, false, adr_util::string::format(XorStr("Aim - Trigger - %s"), str_name.c_str()), XorStr("b_filter_stomach"));
			setup_var(&var.legitbot.trigger_cfg[i].b_filter_arms, false, adr_util::string::format(XorStr("Aim - Trigger - %s"), str_name.c_str()), XorStr("b_filter_arms"));
			setup_var(&var.legitbot.trigger_cfg[i].b_filter_legs, false, adr_util::string::format(XorStr("Aim - Trigger - %s"), str_name.c_str()), XorStr("b_filter_legs"));
		}
		//encrypts(0)
	}

	// VISUALS
	{
		//decrypts(0)

		// visuals - root
		setup_var(&var.visuals.b_enabled, false, XorStr("Visuals"), XorStr("b_enabled"));
		setup_var(&var.visuals.b_screenshot_safe, false, XorStr("Visuals"), XorStr("b_screenshot_safe"));

		setup_var(&var.visuals.spectator_list.b_enabled, bool_sw(false, 0, 0), XorStr("Visuals - Spectator List"), XorStr("b_enabled"));
		setup_var(&var.visuals.spectator_list.f_alpha, 1.f, XorStr("Visuals - Spectator List"), XorStr("f_alpha"));

		// visuals - local player
		setup_var(&var.visuals.pf_local_player.vf_main.b_enabled, false, XorStr("Visuals - Local"), XorStr("b_enabled"));
		setup_var(&var.visuals.pf_local_player.vf_main.f_timeout, 0.f, XorStr("Visuals - Local"), XorStr("f_timeout"));
		setup_var(&var.visuals.pf_local_player.vf_main.b_box, false, XorStr("Visuals - Local"), XorStr("b_box"));
		setup_var(&var.visuals.pf_local_player.vf_main.b_render, true, XorStr("Visuals - Local"), XorStr("b_render"));
		setup_var(&var.visuals.pf_local_player.vf_main.b_name, false, XorStr("Visuals - Local"), XorStr("b_name"));
		setup_var(&var.visuals.pf_local_player.vf_main.b_info, false, XorStr("Visuals - Local"), XorStr("b_info"));
		setup_var(&var.visuals.pf_local_player.vf_main.col_name, color_var(Color::White().ToImGUI()), XorStr("Visuals - Local"), XorStr("col_name"));
		setup_var(&var.visuals.pf_local_player.vf_main.col_ammo, color_var(Color::White().ToImGUI()), XorStr("Visuals - Local"), XorStr("col_ammo"));

		setup_var(&var.visuals.pf_local_player.vf_main.col_main, color_var(Color::White().ToImGUI()), XorStr("Visuals - Local"), XorStr("col_main"));

		setup_var(&var.visuals.pf_local_player.b_weapon, false, XorStr("Visuals - Local"), XorStr("b_weapon"));
		setup_var(&var.visuals.pf_local_player.b_health, false, XorStr("Visuals - Local"), XorStr("b_health"));
		setup_var(&var.visuals.pf_local_player.b_armor, false, XorStr("Visuals - Local"), XorStr("b_armor"));
		setup_var(&var.visuals.pf_local_player.b_money, false, XorStr("Visuals - Local"), XorStr("b_money"));
		setup_var(&var.visuals.pf_local_player.b_kevlar_helm, false, XorStr("Visuals - Local"), XorStr("b_kevlar_helm"));
		setup_var(&var.visuals.pf_local_player.b_bomb_carrier, false, XorStr("Visuals - Local"), XorStr("b_bomb_carrier"));
		setup_var(&var.visuals.pf_local_player.b_has_kit, false, XorStr("Visuals - Local"), XorStr("b_has_kit"));
		setup_var(&var.visuals.pf_local_player.b_scoped, false, XorStr("Visuals - Local"), XorStr("b_scoped"));
		setup_var(&var.visuals.pf_local_player.b_pin_pull, false, XorStr("Visuals - Local"), XorStr("b_pin_pull"));
		setup_var(&var.visuals.pf_local_player.b_blind, false, XorStr("Visuals - Local"), XorStr("b_blind"));
		setup_var(&var.visuals.pf_local_player.b_burn, false, XorStr("Visuals - Local"), XorStr("b_burn"));
		setup_var(&var.visuals.pf_local_player.b_bomb_interaction, false, XorStr("Visuals - Local"), XorStr("b_bomb_interaction"));
		setup_var(&var.visuals.pf_local_player.b_reload, false, XorStr("Visuals - Local"), XorStr("b_reload"));
		setup_var(&var.visuals.pf_local_player.b_vuln, false, XorStr("Visuals - Local"), XorStr("b_vuln"));
		setup_var(&var.visuals.pf_local_player.b_resolver, false, XorStr("Visuals - Local"), XorStr("b_resolver"));
		setup_var(&var.visuals.pf_local_player.b_assistant_hc, false, XorStr("Visuals - Local"), XorStr("b_assistant_hc"));
		setup_var(&var.visuals.pf_local_player.b_assistant_mindmg, false, XorStr("Visuals - Local"), XorStr("b_assistant_mindmg"));
		setup_var(&var.visuals.pf_local_player.b_fakeduck, false, XorStr("Visuals - Local"), XorStr("b_fakeduck"));
		setup_var(&var.visuals.pf_local_player.b_aimbot_debug, false, XorStr("Visuals - Local"), XorStr("b_aimbot_debug"));
		setup_var(&var.visuals.pf_local_player.b_entity_debug, false, XorStr("Visuals - Local"), XorStr("b_entity_debug"));

		setup_var(&var.visuals.pf_local_player.b_backtrack, false, XorStr("Visuals - Local"), XorStr("b_backtrack"));
		setup_var(&var.visuals.pf_local_player.col_backtrack, color_var(Color::White().ToImGUI()), XorStr("Visuals - Local"), XorStr("col_backtrack"));

		setup_var(&var.visuals.pf_local_player.b_offscreen_esp, false, XorStr("Visuals - Local"), XorStr("b_offscreen_esp"));
		setup_var(&var.visuals.pf_local_player.f_offscreen_esp, 0.f, XorStr("Visuals - Local"), XorStr("f_offscreen_esp"));
		setup_var(&var.visuals.pf_local_player.col_offscreen_esp, color_var(Color(0,0,0,0).ToImGUI()), XorStr("Visuals - Local"), XorStr("col_offscreen_esp"));

		setup_var(&var.visuals.pf_local_player.b_smooth_animation, false, XorStr("Visuals - Local"), XorStr("b_smooth_animation"));

		setup_var(&var.visuals.pf_local_player.vf_main.glow.b_enabled, false, XorStr("Visuals - Glow - Local"), XorStr("b_enabled"));
		setup_var(&var.visuals.pf_local_player.vf_main.glow.b_fullbloom, false, XorStr("Visuals - Glow - Local"), XorStr("b_fullbloom"));
		setup_var(&var.visuals.pf_local_player.vf_main.glow.i_type, 0, XorStr("Visuals - Glow - Local"), XorStr("i_type"));
		setup_var(&var.visuals.pf_local_player.vf_main.glow.f_blend, 100.f, XorStr("Visuals - Glow - Local"), XorStr("f_blend"));
		setup_var(&var.visuals.pf_local_player.vf_main.glow.col_color, color_var(Color::White().ToImGUI()), XorStr("Visuals - Glow - Local"), XorStr("col_color"));

		setup_var(&var.visuals.pf_local_player.vf_main.chams.b_enabled, false, XorStr("Visuals - Chams - Local"), XorStr("b_enabled"));
		setup_var(&var.visuals.pf_local_player.vf_main.chams.b_xqz, false, XorStr("Visuals - Chams - Local"), XorStr("b_xqz"));
		setup_var(&var.visuals.pf_local_player.vf_main.chams.i_mat_invisible, 0, XorStr("Visuals - Chams - Local"), XorStr("i_mat_invisible"));
		setup_var(&var.visuals.pf_local_player.vf_main.chams.i_mat_desync_type, 0, XorStr("Visuals - Chams - Local"), XorStr("i_mat_desync_type"));
		setup_var(&var.visuals.pf_local_player.vf_main.chams.i_mat_visible, 0, XorStr("Visuals - Chams - Local"), XorStr("i_mat_visible"));
		setup_var(&var.visuals.pf_local_player.vf_main.chams.col_invisible, color_var(Color::White().ToImGUI()), XorStr("Visuals - Chams - Local"), XorStr("col_invisible"));
		setup_var(&var.visuals.pf_local_player.vf_main.chams.col_visible, color_var(Color::White().ToImGUI()), XorStr("Visuals - Chams - Local"), XorStr("col_visible"));

		// visuals - enemies
		setup_var(&var.visuals.pf_enemy.vf_main.b_enabled, false, XorStr("Visuals - Enemy"), XorStr("b_enabled"));
		setup_var(&var.visuals.pf_enemy.vf_main.f_timeout, 0.f, XorStr("Visuals - Enemy"), XorStr("f_timeout"));
		setup_var(&var.visuals.pf_enemy.vf_main.b_box, false, XorStr("Visuals - Enemy"), XorStr("b_box"));
		setup_var(&var.visuals.pf_enemy.vf_main.b_render, true, XorStr("Visuals - Enemy"), XorStr("b_render"));
		setup_var(&var.visuals.pf_enemy.vf_main.b_name, false, XorStr("Visuals - Enemy"), XorStr("b_name"));
		setup_var(&var.visuals.pf_enemy.vf_main.b_info, false, XorStr("Visuals - Enemy"), XorStr("b_info"));
		setup_var(&var.visuals.pf_enemy.vf_main.col_name, color_var(Color::White().ToImGUI()), XorStr("Visuals - Enemy"), XorStr("col_name"));
		setup_var(&var.visuals.pf_enemy.vf_main.col_ammo, color_var(Color::White().ToImGUI()), XorStr("Visuals - Enemy"), XorStr("col_ammo"));

		setup_var(&var.visuals.pf_enemy.vf_main.col_main, color_var(Color::Red().ToImGUI()), XorStr("Visuals - Enemy"), XorStr("col_main"));

		setup_var(&var.visuals.pf_enemy.b_entity_debug, false, XorStr("Visuals - Enemy"), XorStr("b_entity_debug"));
		setup_var(&var.visuals.pf_enemy.b_weapon, false, XorStr("Visuals - Enemy"), XorStr("b_weapon"));
		setup_var(&var.visuals.pf_enemy.b_health, false, XorStr("Visuals - Enemy"), XorStr("b_health"));
		setup_var(&var.visuals.pf_enemy.b_armor, false, XorStr("Visuals - Enemy"), XorStr("b_armor"));
		setup_var(&var.visuals.pf_enemy.b_money, false, XorStr("Visuals - Enemy"), XorStr("b_money"));
		setup_var(&var.visuals.pf_enemy.b_kevlar_helm, false, XorStr("Visuals - Enemy"), XorStr("b_kevlar_helm"));
		setup_var(&var.visuals.pf_enemy.b_bomb_carrier, false, XorStr("Visuals - Enemy"), XorStr("b_bomb_carrier"));
		setup_var(&var.visuals.pf_enemy.b_has_kit, false, XorStr("Visuals - Enemy"), XorStr("b_has_kit"));
		setup_var(&var.visuals.pf_enemy.b_scoped, false, XorStr("Visuals - Enemy"), XorStr("b_scoped"));
		setup_var(&var.visuals.pf_enemy.b_pin_pull, false, XorStr("Visuals - Enemy"), XorStr("b_pin_pull"));
		setup_var(&var.visuals.pf_enemy.b_blind, false, XorStr("Visuals - Enemy"), XorStr("b_blind"));
		setup_var(&var.visuals.pf_enemy.b_burn, false, XorStr("Visuals - Enemy"), XorStr("b_burn"));
		setup_var(&var.visuals.pf_enemy.b_bomb_interaction, false, XorStr("Visuals - Enemy"), XorStr("b_bomb_interaction"));
		setup_var(&var.visuals.pf_enemy.b_reload, false, XorStr("Visuals - Enemy"), XorStr("b_reload"));
		setup_var(&var.visuals.pf_enemy.b_vuln, false, XorStr("Visuals - Enemy"), XorStr("b_vuln"));
		setup_var(&var.visuals.pf_enemy.b_resolver, false, XorStr("Visuals - Enemy"), XorStr("b_resolver"));
		setup_var(&var.visuals.pf_enemy.b_fakeduck, false, XorStr("Visuals - Enemy"), XorStr("b_fakeduck"));
		setup_var(&var.visuals.pf_enemy.b_aimbot_debug, false, XorStr("Visuals - Enemy"), XorStr("b_aimbot_debug"));
		setup_var(&var.visuals.pf_enemy.b_assistant_hc, false, XorStr("Visuals - Enemy"), XorStr("b_assistant_hc"));
		setup_var(&var.visuals.pf_enemy.b_assistant_mindmg, false, XorStr("Visuals - Enemy"), XorStr("b_assistant_mindmg"));

		setup_var(&var.visuals.pf_enemy.b_faresp, false, XorStr("Visuals - Enemy"), XorStr("b_faresp"));
		setup_var(&var.visuals.pf_enemy.i_faresp, 20, XorStr("Visuals - Enemy"), XorStr("i_faresp"));

		setup_var(&var.visuals.pf_enemy.b_backtrack, false, XorStr("Visuals - Enemy"), XorStr("b_backtrack"));
		setup_var(&var.visuals.pf_enemy.col_backtrack, color_var(Color::White().ToImGUI()), XorStr("Visuals - Enemy"), XorStr("col_backtrack"));

		setup_var(&var.visuals.pf_enemy.b_offscreen_esp, false, XorStr("Visuals - Enemy"), XorStr("b_offscreen_esp"));
		setup_var(&var.visuals.pf_enemy.f_offscreen_esp, 15.f, XorStr("Visuals - Enemy"), XorStr("f_offscreen_esp"));
		setup_var(&var.visuals.pf_enemy.col_offscreen_esp, color_var(Color::Red().ToImGUI()), XorStr("Visuals - Enemy"), XorStr("col_offscreen_esp"));

		setup_var(&var.visuals.pf_enemy.b_smooth_animation, false, XorStr("Visuals - Enemy"), XorStr("b_smooth_animation"));

		setup_var(&var.visuals.pf_enemy.vf_main.glow.b_enabled, false, XorStr("Visuals - Glow - Enemy"), XorStr("b_enabled"));
		setup_var(&var.visuals.pf_enemy.vf_main.glow.b_fullbloom, false, XorStr("Visuals - Glow - Enemy"), XorStr("b_fullbloom"));
		setup_var(&var.visuals.pf_enemy.vf_main.glow.i_type, 0, XorStr("Visuals - Glow - Enemy"), XorStr("i_type"));
		setup_var(&var.visuals.pf_enemy.vf_main.glow.f_blend, 100.f, XorStr("Visuals - Glow - Enemy"), XorStr("f_blend"));
		setup_var(&var.visuals.pf_enemy.vf_main.glow.col_color, color_var(Color::Red().ToImGUI()), XorStr("Visuals - Glow - Enemy"), XorStr("col_color"));

		setup_var(&var.visuals.pf_enemy.vf_main.chams.b_enabled, false, XorStr("Visuals - Chams - Enemy"), XorStr("b_enabled"));
		setup_var(&var.visuals.pf_enemy.vf_main.chams.b_xqz, false, XorStr("Visuals - Chams - Enemy"), XorStr("b_xqz"));
		setup_var(&var.visuals.pf_enemy.vf_main.chams.i_mat_invisible, 0, XorStr("Visuals - Chams - Enemy"), XorStr("i_mat_invisible"));
		setup_var(&var.visuals.pf_enemy.vf_main.chams.i_mat_visible, 0, XorStr("Visuals - Chams - Enemy"), XorStr("i_mat_visible"));
		setup_var(&var.visuals.pf_enemy.vf_main.chams.col_invisible, color_var(Color::Magenta().ToImGUI()), XorStr("Visuals - Chams - Enemy"), XorStr("col_invisible"));
		setup_var(&var.visuals.pf_enemy.vf_main.chams.col_visible, color_var(Color::Red().ToImGUI()), XorStr("Visuals - Chams - Enemy"), XorStr("col_visible"));


		// visuals - teammates
		setup_var(&var.visuals.pf_teammate.vf_main.b_enabled, false, XorStr("Visuals - Teammate"), XorStr("b_enabled"));
		setup_var(&var.visuals.pf_teammate.vf_main.f_timeout, 0.f, XorStr("Visuals - Teammate"), XorStr("f_timeout"));
		setup_var(&var.visuals.pf_teammate.vf_main.b_box, false, XorStr("Visuals - Teammate"), XorStr("b_box"));
		setup_var(&var.visuals.pf_teammate.vf_main.b_render, true, XorStr("Visuals - Teammate"), XorStr("b_render"));
		setup_var(&var.visuals.pf_teammate.vf_main.b_name, false, XorStr("Visuals - Teammate"), XorStr("b_name"));
		setup_var(&var.visuals.pf_teammate.vf_main.b_info, false, XorStr("Visuals - Teammate"), XorStr("b_info"));
		setup_var(&var.visuals.pf_teammate.vf_main.col_name, color_var(Color::White().ToImGUI()), XorStr("Visuals - Teammate"), XorStr("col_name"));
		setup_var(&var.visuals.pf_teammate.vf_main.col_ammo, color_var(Color::White().ToImGUI()), XorStr("Visuals - col_ammo"), XorStr("col_ammo"));

		setup_var(&var.visuals.pf_teammate.vf_main.col_main, color_var(Color::Green().ToImGUI()), XorStr("Visuals - Teammate"), XorStr("col_main"));

		setup_var(&var.visuals.pf_teammate.b_entity_debug, false, XorStr("Visuals - Teammate"), XorStr("b_entity_debug"));
		setup_var(&var.visuals.pf_teammate.b_weapon, false, XorStr("Visuals - Teammate"), XorStr("b_weapon"));
		setup_var(&var.visuals.pf_teammate.b_health, false, XorStr("Visuals - Teammate"), XorStr("b_health"));
		setup_var(&var.visuals.pf_teammate.b_armor, false, XorStr("Visuals - Teammate"), XorStr("b_armor"));
		setup_var(&var.visuals.pf_teammate.b_money, false, XorStr("Visuals - Teammate"), XorStr("b_money"));
		setup_var(&var.visuals.pf_teammate.b_kevlar_helm, false, XorStr("Visuals - Teammate"), XorStr("b_kevlar_helm"));
		setup_var(&var.visuals.pf_teammate.b_bomb_carrier, false, XorStr("Visuals - Teammate"), XorStr("b_bomb_carrier"));
		setup_var(&var.visuals.pf_teammate.b_has_kit, false, XorStr("Visuals - Teammate"), XorStr("b_has_kit"));
		setup_var(&var.visuals.pf_teammate.b_scoped, false, XorStr("Visuals - Teammate"), XorStr("b_scoped"));
		setup_var(&var.visuals.pf_teammate.b_pin_pull, false, XorStr("Visuals - Teammate"), XorStr("b_pin_pull"));
		setup_var(&var.visuals.pf_teammate.b_blind, false, XorStr("Visuals - Teammate"), XorStr("b_blind"));
		setup_var(&var.visuals.pf_teammate.b_burn, false, XorStr("Visuals - Teammate"), XorStr("b_burn"));
		setup_var(&var.visuals.pf_teammate.b_bomb_interaction, false, XorStr("Visuals - Teammate"), XorStr("b_bomb_interaction"));
		setup_var(&var.visuals.pf_teammate.b_reload, false, XorStr("Visuals - Teammate"), XorStr("b_reload"));
		setup_var(&var.visuals.pf_teammate.b_vuln, false, XorStr("Visuals - Teammate"), XorStr("b_vuln"));
		setup_var(&var.visuals.pf_teammate.b_resolver, false, XorStr("Visuals - Teammate"), XorStr("b_resolver"));
		setup_var(&var.visuals.pf_teammate.b_fakeduck, false, XorStr("Visuals - Teammate"), XorStr("b_fakeduck"));
		setup_var(&var.visuals.pf_teammate.b_aimbot_debug, false, XorStr("Visuals - Teammate"), XorStr("b_aimbot_debug"));
		setup_var(&var.visuals.pf_teammate.b_assistant_hc, false, XorStr("Visuals - Teammate"), XorStr("b_assistant_hc"));
		setup_var(&var.visuals.pf_teammate.b_assistant_mindmg, false, XorStr("Visuals - Teammate"), XorStr("b_assistant_mindmg"));

		setup_var(&var.visuals.pf_teammate.b_smooth_animation, false, XorStr("Visuals - Teammate"), XorStr("b_smooth_animation"));

		setup_var(&var.visuals.pf_teammate.b_backtrack, false, XorStr("Visuals - Teammate"), XorStr("b_backtrack"));
		setup_var(&var.visuals.pf_teammate.col_backtrack, color_var(Color::White().ToImGUI()), XorStr("Visuals - Teammate"), XorStr("col_backtrack"));

		setup_var(&var.visuals.pf_teammate.b_offscreen_esp, false, XorStr("Visuals - Teammate"), XorStr("b_offscreen_esp"));
		setup_var(&var.visuals.pf_teammate.f_offscreen_esp, 15.f, XorStr("Visuals - Teammate"), XorStr("f_offscreen_esp"));
		setup_var(&var.visuals.pf_teammate.col_offscreen_esp, color_var(Color::Green().ToImGUI()), XorStr("Visuals - Teammate"), XorStr("col_offscreen_esp"));

		setup_var(&var.visuals.pf_teammate.vf_main.glow.b_enabled, false, XorStr("Visuals - Glow - Teammate"), XorStr("b_enabled"));
		setup_var(&var.visuals.pf_teammate.vf_main.glow.b_fullbloom, false, XorStr("Visuals - Glow - Teammate"), XorStr("b_fullbloom"));
		setup_var(&var.visuals.pf_teammate.vf_main.glow.i_type, 0, XorStr("Visuals - Glow - Teammate"), XorStr("i_type"));
		setup_var(&var.visuals.pf_teammate.vf_main.glow.f_blend, 100.f, XorStr("Visuals - Glow - Teammate"), XorStr("f_blend"));
		setup_var(&var.visuals.pf_teammate.vf_main.glow.col_color, color_var(Color::Green().ToImGUI()), XorStr("Visuals - Glow - Teammate"), XorStr("col_color"));

		setup_var(&var.visuals.pf_teammate.vf_main.chams.b_enabled, false, XorStr("Visuals - Chams - Teammate"), XorStr("b_enabled"));
		setup_var(&var.visuals.pf_teammate.vf_main.chams.b_xqz, false, XorStr("Visuals - Chams - Teammate"), XorStr("b_xqz"));
		setup_var(&var.visuals.pf_teammate.vf_main.chams.i_mat_invisible, 0, XorStr("Visuals - Chams - Teammate"), XorStr("i_mat_invisible"));
		setup_var(&var.visuals.pf_teammate.vf_main.chams.i_mat_visible, 0, XorStr("Visuals - Chams - Teammate"), XorStr("i_mat_visible"));
		setup_var(&var.visuals.pf_teammate.vf_main.chams.col_invisible, color_var(Color::Cyan().ToImGUI()), XorStr("Visuals - Chams - Teammate"), XorStr("col_invisible"));
		setup_var(&var.visuals.pf_teammate.vf_main.chams.col_visible, color_var(Color::Green().ToImGUI()), XorStr("Visuals - Chams - Teammate"), XorStr("col_visible"));

		// visuals - bomb
		setup_var(&var.visuals.vf_bomb.b_box, false, XorStr("Visuals - Bomb"), XorStr("b_box"));
		setup_var(&var.visuals.vf_bomb.f_timeout, 0.f, XorStr("Visuals - Bomb"), XorStr("f_timeout"));
		setup_var(&var.visuals.vf_bomb.b_enabled, false, XorStr("Visuals - Bomb"), XorStr("b_enabled"));
		setup_var(&var.visuals.vf_bomb.b_name, false, XorStr("Visuals - Bomb"), XorStr("b_name"));
		setup_var(&var.visuals.vf_bomb.b_render, true, XorStr("Visuals - Bomb"), XorStr("b_render"));
		setup_var(&var.visuals.vf_bomb.b_info, false, XorStr("Visuals - Bomb"), XorStr("b_info"));
		setup_var(&var.visuals.vf_bomb.col_main, color_var(Color::Orange().ToImGUI()), XorStr("Visuals - Bomb"), XorStr("col_main"));
		setup_var(&var.visuals.vf_bomb.col_name, color_var(Color::Orange().ToImGUI()), XorStr("Visuals - Bomb"), XorStr("col_name"));

		setup_var(&var.visuals.vf_bomb.glow.b_enabled, false, XorStr("Visuals - Glow - Bomb"), XorStr("b_enabled"));
		setup_var(&var.visuals.vf_bomb.glow.b_fullbloom, false, XorStr("Visuals - Glow - Bomb"), XorStr("b_fullbloom"));
		setup_var(&var.visuals.vf_bomb.glow.i_type, 0, XorStr("Visuals - Glow - Bomb"), XorStr("i_type"));
		setup_var(&var.visuals.vf_bomb.glow.f_blend, 100.f, XorStr("Visuals - Glow - Bomb"), XorStr("f_blend"));
		setup_var(&var.visuals.vf_bomb.glow.col_color, color_var(Color::Orange().ToImGUI()), XorStr("Visuals - Glow - Bomb"), XorStr("col_color"));

		setup_var(&var.visuals.vf_bomb.chams.b_enabled, false, XorStr("Visuals - Chams - Bomb"), XorStr("b_enabled"));
		setup_var(&var.visuals.vf_bomb.chams.b_xqz, false, XorStr("Visuals - Chams - Bomb"), XorStr("b_xqz"));
		setup_var(&var.visuals.vf_bomb.chams.i_mat_invisible, 0, XorStr("Visuals - Chams - Bomb"), XorStr("i_mat_invisible"));
		setup_var(&var.visuals.vf_bomb.chams.i_mat_visible, 0, XorStr("Visuals - Chams - Bomb"), XorStr("i_mat_visible"));
		setup_var(&var.visuals.vf_bomb.chams.col_invisible, color_var(Color::Orange().ToImGUI()), XorStr("Visuals - Chams - Bomb"), XorStr("col_invisible"));
		setup_var(&var.visuals.vf_bomb.chams.col_visible, color_var(Color::Orange().ToImGUI()), XorStr("Visuals - Chams - Bomb"), XorStr("col_visible"));

		// visuals - projectiles
		setup_var(&var.visuals.vf_projectile.b_box, false, XorStr("Visuals - Proj"), XorStr("b_box"));
		setup_var(&var.visuals.vf_projectile.f_timeout, 0.f, XorStr("Visuals - Proj"), XorStr("f_timeout"));
		setup_var(&var.visuals.vf_projectile.b_enabled, false, XorStr("Visuals - Proj"), XorStr("b_enabled"));
		setup_var(&var.visuals.vf_projectile.b_name, false, XorStr("Visuals - Proj"), XorStr("b_name"));
		setup_var(&var.visuals.vf_projectile.b_render, true, XorStr("Visuals - Proj"), XorStr("b_render"));
		setup_var(&var.visuals.vf_projectile.b_info, false, XorStr("Visuals - Proj"), XorStr("b_info"));
		setup_var(&var.visuals.vf_projectile.col_main, color_var(ImColor(191, 85, 236)), XorStr("Visuals - Proj"), XorStr("col_main"));
		setup_var(&var.visuals.vf_projectile.col_name, color_var(ImColor(191, 85, 236)), XorStr("Visuals - Proj"), XorStr("col_name"));

		setup_var(&var.visuals.vf_projectile.glow.b_enabled, false, XorStr("Visuals - Glow - Proj"), XorStr("b_enabled"));
		setup_var(&var.visuals.vf_projectile.glow.b_fullbloom, false, XorStr("Visuals - Glow - Proj"), XorStr("b_fullbloom"));
		setup_var(&var.visuals.vf_projectile.glow.i_type, 0, XorStr("Visuals - Glow - Proj"), XorStr("i_type"));
		setup_var(&var.visuals.vf_projectile.glow.f_blend, 100.f, XorStr("Visuals - Glow - Proj"), XorStr("f_blend"));
		setup_var(&var.visuals.vf_projectile.glow.col_color, color_var(ImColor(191, 85, 236)), XorStr("Visuals - Glow - Proj"), XorStr("col_color"));

		setup_var(&var.visuals.vf_projectile.chams.b_enabled, false, XorStr("Visuals - Chams - Proj"), XorStr("b_enabled"));
		setup_var(&var.visuals.vf_projectile.chams.b_xqz, false, XorStr("Visuals - Chams - Proj"), XorStr("b_xqz"));
		setup_var(&var.visuals.vf_projectile.chams.i_mat_invisible, 0, XorStr("Visuals - Chams - Proj"), XorStr("i_mat_invisible"));
		setup_var(&var.visuals.vf_projectile.chams.i_mat_visible, 0, XorStr("Visuals - Chams - Proj"), XorStr("i_mat_visible"));
		setup_var(&var.visuals.vf_projectile.chams.col_invisible, color_var(ImColor(191, 85, 236)), XorStr("Visuals - Chams - Proj"), XorStr("col_invisible"));
		setup_var(&var.visuals.vf_projectile.chams.col_visible, color_var(ImColor(191, 85, 236)), XorStr("Visuals - Chams - Proj"), XorStr("col_visible"));


		// visuals - dropped weapons
		setup_var(&var.visuals.vf_weapon.b_box, false, XorStr("Visuals - Weap"), XorStr("b_box"));
		setup_var(&var.visuals.vf_weapon.f_timeout, 0.f, XorStr("Visuals - Weap"), XorStr("f_timeout"));
		setup_var(&var.visuals.vf_weapon.b_enabled, false, XorStr("Visuals - Weap"), XorStr("b_enabled"));
		setup_var(&var.visuals.vf_weapon.b_name, false, XorStr("Visuals - Weap"), XorStr("b_name"));
		setup_var(&var.visuals.vf_weapon.b_render, true, XorStr("Visuals - Weap"), XorStr("b_render"));
		setup_var(&var.visuals.vf_weapon.b_info, false, XorStr("Visuals - Weap"), XorStr("b_info"));
		setup_var(&var.visuals.vf_weapon.col_main, color_var(Color::Cyan().ToImGUI()), XorStr("Visuals - Weap"), XorStr("col_main"));
		setup_var(&var.visuals.vf_weapon.col_name, color_var(Color::White().ToImGUI()), XorStr("Visuals - Weap"), XorStr("col_name"));

		setup_var(&var.visuals.vf_weapon.glow.b_enabled, false, XorStr("Visuals - Glow - Weap"), XorStr("b_enabled"));
		setup_var(&var.visuals.vf_weapon.glow.b_fullbloom, false, XorStr("Visuals - Glow - Weap"), XorStr("b_fullbloom"));
		setup_var(&var.visuals.vf_weapon.glow.i_type, 0, XorStr("Visuals - Glow - Weap"), XorStr("i_type"));
		setup_var(&var.visuals.vf_weapon.glow.f_blend, 100.f, XorStr("Visuals - Glow - Weap"), XorStr("f_blend"));
		setup_var(&var.visuals.vf_weapon.glow.col_color, color_var(Color::Cyan().ToImGUI()), XorStr("Visuals - Glow - Weap"), XorStr("col_color"));

		setup_var(&var.visuals.vf_weapon.chams.b_enabled, false, XorStr("Visuals - Chams - Weap"), XorStr("b_enabled"));
		setup_var(&var.visuals.vf_weapon.chams.b_xqz, false, XorStr("Visuals - Chams - Weap"), XorStr("b_xqz"));
		setup_var(&var.visuals.vf_weapon.chams.i_mat_invisible, 0, XorStr("Visuals - Chams - Weap"), XorStr("i_mat_invisible"));
		setup_var(&var.visuals.vf_weapon.chams.i_mat_visible, 0, XorStr("Visuals - Chams - Weap"), XorStr("i_mat_visible"));
		setup_var(&var.visuals.vf_weapon.chams.col_invisible, color_var(Color::Cyan().ToImGUI()), XorStr("Visuals - Chams - Weap"), XorStr("col_invisible"));
		setup_var(&var.visuals.vf_weapon.chams.col_visible, color_var(Color::Cyan().ToImGUI()), XorStr("Visuals - Chams - Weap"), XorStr("col_visible"));


		// visuals - ragdolls
		setup_var(&var.visuals.vf_ragdolls.b_box, false, XorStr("Visuals - Ragdoll"), XorStr("b_box"));
		setup_var(&var.visuals.vf_ragdolls.f_timeout, 0.f, XorStr("Visuals - Ragdoll"), XorStr("f_timeout"));
		setup_var(&var.visuals.vf_ragdolls.b_enabled, false, XorStr("Visuals - Ragdoll"), XorStr("b_enabled"));
		setup_var(&var.visuals.vf_ragdolls.b_name, false, XorStr("Visuals - Ragdoll"), XorStr("b_name"));
		setup_var(&var.visuals.vf_ragdolls.b_render, true, XorStr("Visuals - Ragdoll"), XorStr("b_render"));
		setup_var(&var.visuals.vf_ragdolls.b_info, false, XorStr("Visuals - Ragdoll"), XorStr("b_info"));
		setup_var(&var.visuals.vf_ragdolls.col_main, color_var(Color::White().ToImGUI()), XorStr("Visuals - Ragdoll"), XorStr("col_main"));
		setup_var(&var.visuals.vf_ragdolls.col_name, color_var(Color::White().ToImGUI()), XorStr("Visuals - Ragdoll"), XorStr("col_name"));

		setup_var(&var.visuals.vf_ragdolls.glow.b_enabled, false, XorStr("Visuals - Glow - Ragdoll"), XorStr("b_enabled"));
		setup_var(&var.visuals.vf_ragdolls.glow.b_fullbloom, false, XorStr("Visuals - Glow - Ragdoll"), XorStr("b_fullbloom"));
		setup_var(&var.visuals.vf_ragdolls.glow.i_type, 0, XorStr("Visuals - Glow - Ragdoll"), XorStr("i_type"));
		setup_var(&var.visuals.vf_ragdolls.glow.f_blend, 100.f, XorStr("Visuals - Glow - Ragdoll"), XorStr("f_blend"));
		setup_var(&var.visuals.vf_ragdolls.glow.col_color, color_var(Color::White().ToImGUI()), XorStr("Visuals - Glow - Ragdoll"), XorStr("col_color"));

		setup_var(&var.visuals.vf_ragdolls.chams.b_enabled, false, XorStr("Visuals - Chams - Ragdoll"), XorStr("b_enabled"));
		setup_var(&var.visuals.vf_ragdolls.chams.b_xqz, false, XorStr("Visuals - Chams - Ragdoll"), XorStr("b_xqz"));
		setup_var(&var.visuals.vf_ragdolls.chams.i_mat_invisible, 0, XorStr("Visuals - Chams - Ragdoll"), XorStr("i_mat_invisible"));
		setup_var(&var.visuals.vf_ragdolls.chams.i_mat_visible, 0, XorStr("Visuals - Chams - Ragdoll"), XorStr("i_mat_visible"));
		setup_var(&var.visuals.vf_ragdolls.chams.col_invisible, color_var(Color::White().ToImGUI()), XorStr("Visuals - Chams - Ragdoll"), XorStr("col_invisible"));
		setup_var(&var.visuals.vf_ragdolls.chams.col_visible, color_var(Color::White().ToImGUI()), XorStr("Visuals - Chams - Ragdoll"), XorStr("col_visible"));


		// visuals - arms
		setup_var(&var.visuals.vf_arms.b_box, false, XorStr("Visuals - Arms"), XorStr("b_box"));
		setup_var(&var.visuals.vf_arms.f_timeout, 0.f, XorStr("Visuals - Arms"), XorStr("f_timeout"));
		setup_var(&var.visuals.vf_arms.b_enabled, false, XorStr("Visuals - Arms"), XorStr("b_enabled"));
		setup_var(&var.visuals.vf_arms.b_name, false, XorStr("Visuals - Arms"), XorStr("b_name"));
		setup_var(&var.visuals.vf_arms.b_render, true, XorStr("Visuals - Arms"), XorStr("b_render"));
		setup_var(&var.visuals.vf_arms.b_info, false, XorStr("Visuals - Arms"), XorStr("b_info"));
		setup_var(&var.visuals.vf_arms.col_main, color_var(Color::White().ToImGUI()), XorStr("Visuals - Arms"), XorStr("col_main"));
		setup_var(&var.visuals.vf_arms.col_name, color_var(Color::White().ToImGUI()), XorStr("Visuals - Arms"), XorStr("col_name"));

		setup_var(&var.visuals.vf_arms.glow.b_enabled, false, XorStr("Visuals - Glow - Arms"), XorStr("b_enabled"));
		setup_var(&var.visuals.vf_arms.glow.b_fullbloom, false, XorStr("Visuals - Glow - Arms"), XorStr("b_fullbloom"));
		setup_var(&var.visuals.vf_arms.glow.i_type, 0, XorStr("Visuals - Glow - Arms"), XorStr("i_type"));
		setup_var(&var.visuals.vf_arms.glow.f_blend, 100.f, XorStr("Visuals - Glow - Arms"), XorStr("f_blend"));
		setup_var(&var.visuals.vf_arms.glow.col_color, color_var(Color::White().ToImGUI()), XorStr("Visuals - Glow - Arms"), XorStr("col_color"));

		setup_var(&var.visuals.vf_arms.chams.b_enabled, false, XorStr("Visuals - Chams - Arms"), XorStr("b_enabled"));
		setup_var(&var.visuals.vf_arms.chams.b_xqz, false, XorStr("Visuals - Chams - Arms"), XorStr("b_xqz"));
		setup_var(&var.visuals.vf_arms.chams.i_mat_invisible, 0, XorStr("Visuals - Chams - Arms"), XorStr("i_mat_invisible"));
		setup_var(&var.visuals.vf_arms.chams.i_mat_visible, 0, XorStr("Visuals - Chams - Arms"), XorStr("i_mat_visible"));
		setup_var(&var.visuals.vf_arms.chams.col_invisible, color_var(Color::White().ToImGUI()), XorStr("Visuals - Chams - Arms"), XorStr("col_invisible"));
		setup_var(&var.visuals.vf_arms.chams.col_visible, color_var(Color::White().ToImGUI()), XorStr("Visuals - Chams - Arms"), XorStr("col_visible"));


		// visuals - viewmodel
		setup_var(&var.visuals.vf_viewweapon.b_box, false, XorStr("Visuals - Viewmodel"), XorStr("b_box"));
		setup_var(&var.visuals.vf_viewweapon.f_timeout, 0.f, XorStr("Visuals - Viewmodel"), XorStr("f_timeout"));
		setup_var(&var.visuals.vf_viewweapon.b_enabled, false, XorStr("Visuals - Viewmodel"), XorStr("b_enabled"));
		setup_var(&var.visuals.vf_viewweapon.b_name, false, XorStr("Visuals - Viewmodel"), XorStr("b_name"));
		setup_var(&var.visuals.vf_viewweapon.b_render, true, XorStr("Visuals - Viewmodel"), XorStr("b_render"));
		setup_var(&var.visuals.vf_viewweapon.b_info, false, XorStr("Visuals - Viewmodel"), XorStr("b_info"));
		setup_var(&var.visuals.vf_viewweapon.col_main, color_var(Color::White().ToImGUI()), XorStr("Visuals - Viewmodel"), XorStr("col_main"));
		setup_var(&var.visuals.vf_viewweapon.col_name, color_var(Color::White().ToImGUI()), XorStr("Visuals - Viewmodel"), XorStr("col_name"));

		setup_var(&var.visuals.vf_viewweapon.glow.b_enabled, false, XorStr("Visuals - Glow - Viewmodel"), XorStr("b_enabled"));
		setup_var(&var.visuals.vf_viewweapon.glow.b_fullbloom, false, XorStr("Visuals - Glow - Viewmodel"), XorStr("b_fullbloom"));
		setup_var(&var.visuals.vf_viewweapon.glow.i_type, 0, XorStr("Visuals - Glow - Viewmodel"), XorStr("i_type"));
		setup_var(&var.visuals.vf_viewweapon.glow.f_blend, 100.f, XorStr("Visuals - Glow - Viewmodel"), XorStr("f_blend"));
		setup_var(&var.visuals.vf_viewweapon.glow.col_color, color_var(Color::Cyan().ToImGUI()), XorStr("Visuals - Glow - Viewmodel"), XorStr("col_color"));

		setup_var(&var.visuals.vf_viewweapon.chams.b_enabled, false, XorStr("Visuals - Chams - Viewmodel"), XorStr("b_enabled"));
		setup_var(&var.visuals.vf_viewweapon.chams.b_xqz, false, XorStr("Visuals - Chams - Viewmodel"), XorStr("b_xqz"));
		setup_var(&var.visuals.vf_viewweapon.chams.i_mat_invisible, 0, XorStr("Visuals - Chams - Viewmodel"), XorStr("i_mat_invisible"));
		setup_var(&var.visuals.vf_viewweapon.chams.i_mat_visible, 0, XorStr("Visuals - Chams - Viewmodel"), XorStr("i_mat_visible"));
		setup_var(&var.visuals.vf_viewweapon.chams.col_invisible, color_var(Color::Cyan().ToImGUI()), XorStr("Visuals - Chams - Viewmodel"), XorStr("col_invisible"));
		setup_var(&var.visuals.vf_viewweapon.chams.col_visible, color_var(Color::Cyan().ToImGUI()), XorStr("Visuals - Chams - Viewmodel"), XorStr("col_visible"));


		// visuals - misc or unique to one entity type
		setup_var(&var.visuals.b_fake, false, XorStr("Visuals - Misc"), XorStr("b_fake"));
		setup_var(&var.visuals.col_fake, color_var(Color(255, 0, 0, 127).ToImGUI()), XorStr("Visuals - Misc"), XorStr("col_fake"));

		setup_var(&var.visuals.b_verbose_resolver, false, XorStr("Visuals - Misc"), XorStr("b_verbose_resolver"));
		setup_var(&var.visuals.b_projectile_owner, false, XorStr("Visuals - Misc"), XorStr("b_projectile_owner"));
		setup_var(&var.visuals.b_projectile_timer, false, XorStr("Visuals - Misc"), XorStr("b_projectile_timer"));
		setup_var(&var.visuals.b_projectile_range, false, XorStr("Visuals - Misc"), XorStr("b_projectile_range"));

		setup_var(&var.visuals.nade_prediction.b_enabled, false, XorStr("Visuals - Nade Pred"), XorStr("b_enabled"));
		setup_var(&var.visuals.nade_prediction.col, color_var(Color::White().ToImGUI()), XorStr("Visuals - Nade Pred"), XorStr("col"));
		setup_var(&var.visuals.nade_prediction.col_visible, color_var(Color::White().ToImGUI()), XorStr("Visuals - Nade Pred"), XorStr("col_visible"));
		setup_var(&var.visuals.nade_prediction.col_hit, color_var(Color::Red().ToImGUI()), XorStr("Visuals - Nade Pred"), XorStr("col_hit"));

		setup_var(&var.visuals.b_c4timer, false, XorStr("Visuals - Misc"), XorStr("b_c4timer"));

		setup_var(&var.visuals.b_aimbot_feedback, false, XorStr("Visuals - Misc"), XorStr("b_aimbot_feedback"));
		setup_var(&var.visuals.b_aimbot_points, false, XorStr("Visuals - Misc"), XorStr("b_aimbot_points"));

		setup_var(&var.visuals.b_show_antiaim_angles, false, XorStr("Visuals - Misc"), XorStr("b_show_antiaim_angles"));
		setup_var(&var.visuals.b_show_antiaim_real, false, XorStr("Visuals - Misc"), XorStr("b_show_antiaim_real"));
		setup_var(&var.visuals.b_show_antiaim_fake, false, XorStr("Visuals - Misc"), XorStr("b_show_antiaim_fake"));
		setup_var(&var.visuals.b_show_antiaim_lby, false, XorStr("Visuals - Misc"), XorStr("b_show_antiaim_lby"));

		setup_var(&var.visuals.i_visualize_sound, 0, XorStr("Visuals - Misc"), XorStr("i_visualize_sound"));
		setup_var(&var.visuals.b_visualize_sound_enemy, false, XorStr("Visuals - Misc"), XorStr("b_visualize_sound_enemy"));
		setup_var(&var.visuals.b_visualize_sound_teammate, false, XorStr("Visuals - Misc"), XorStr("b_visualize_sound_teammate"));
		setup_var(&var.visuals.col_visualize_sound_enemy, color_var(Color::Red().ToImGUI()), XorStr("Visuals - Misc"), XorStr("col_visualize_sound_enemy"));
		setup_var(&var.visuals.col_visualize_sound_teammate, color_var(Color::Green().ToImGUI()), XorStr("Visuals - Misc"), XorStr("col_visualize_sound_teammate"));

		setup_var(&var.visuals.b_offscreen_esp, false, XorStr("Visuals - Misc"), XorStr("b_offscreen_esp"));
		setup_var(&var.visuals.f_offscreen_esp, 10.f, XorStr("Visuals - Misc"), XorStr("f_offscreen_esp"));
		setup_var(&var.visuals.col_offscreen_esp, color_var(Color::Red().ToImGUI()), XorStr("Visuals - Misc"), XorStr("col_offscreen_esp"));

		setup_var(&var.visuals.i_thirdperson_key, 0, XorStr("Visuals - Misc"), XorStr("i_thirdperson_key"));

		setup_var(&var.visuals.f_fov, 90.f, XorStr("Visuals - Misc"), XorStr("f_fov"));
		setup_var(&var.visuals.f_thirdperson_dist, 100.f, XorStr("Visuals - Misc"), XorStr("f_thirdperson_dist"));
		setup_var(&var.visuals.f_thirdperson_fov, 90.f, XorStr("Visuals - Misc"), XorStr("f_thirdperson_fov"));

		setup_var(&var.visuals.b_no_scope, false, XorStr("Visuals - Misc"), XorStr("b_no_scope"));
		setup_var(&var.visuals.b_no_zoom, false, XorStr("Visuals - Misc"), XorStr("b_no_zoom"));
		setup_var(&var.visuals.b_no_smoke, false, XorStr("Visuals - Misc"), XorStr("b_no_smoke"));
		setup_var(&var.visuals.b_no_flash, false, XorStr("Visuals - Misc"), XorStr("b_no_flash"));
		setup_var(&var.visuals.b_no_visual_recoil, false, XorStr("Visuals - Misc"), XorStr("b_no_visual_recoil"));

		setup_var(&var.visuals.b_scope_lines, false, XorStr("Visuals - Misc"), XorStr("b_scope_lines"));
		setup_var(&var.visuals.col_scope_lines, color_var(Color::Black().ToImGUI()), XorStr("Visuals - Misc"), XorStr("col_scope_lines"));

		setup_var(&var.visuals.f_blend_scope, 100.f, XorStr("Visuals - Misc"), XorStr("f_blend_scope"));

		setup_var(&var.visuals.i_autowall_xhair, 0, XorStr("Visuals - Misc"), XorStr("i_autowall_xhair"));

		setup_var(&var.visuals.b_indicators, false, XorStr("Visuals - Misc"), XorStr("b_indicators"));
		setup_var(&var.visuals.b_indicators_only_active, false, XorStr("Visuals - Misc"), XorStr("b_indicators_only_active"));
		setup_var(&var.visuals.b_indicator_desync, false, XorStr("Visuals - Misc"), XorStr("b_indicator_desync"));
		setup_var(&var.visuals.b_indicator_lc, false, XorStr("Visuals - Misc"), XorStr("b_indicator_lc"));
		setup_var(&var.visuals.b_indicator_priority, false, XorStr("Visuals - Misc"), XorStr("b_indicator_priority"));
		setup_var(&var.visuals.b_indicator_edge, false, XorStr("Visuals - Misc"), XorStr("b_indicator_edge"));
		setup_var(&var.visuals.b_indicator_fakeduck, false, XorStr("Visuals - Misc"), XorStr("b_indicator_fakeduck"));
		setup_var(&var.visuals.b_indicator_minwalk, false, XorStr("Visuals - Misc"), XorStr("b_indicator_minwalk"));
		setup_var(&var.visuals.b_indicator_choke, false, XorStr("Visuals - Misc"), XorStr("b_indicator_choke"));
		setup_var(&var.visuals.b_indicator_fake_latency, false, XorStr("Visuals - Misc"), XorStr("b_indicator_fake_latency"));
		setup_var(&var.visuals.b_indicator_double_tap, false, XorStr("Visuals - Misc"), XorStr("b_indicator_double_tap"));
		setup_var(&var.visuals.b_indicator_tickbase, false, XorStr("Visuals - Misc"), XorStr("b_indicator_tickbase"));
		setup_var(&var.visuals.b_indicator_lean_dir, false, XorStr("Visuals - Misc"), XorStr("b_indicator_lean_dir"));
		setup_var(&var.visuals.b_indicator_teleport, false, XorStr("Visuals - Misc"), XorStr("b_indicator_teleport"));
		setup_var(&var.visuals.b_indicator_stats, false, XorStr("Visuals - Misc"), XorStr("b_indicator_stats"));

		setup_var(&var.visuals.b_hit_sound, false, XorStr("Visuals - Misc"), XorStr("b_hit_sound"));
		setup_var(&var.visuals.b_hitmarker, false, XorStr("Visuals - Misc"), XorStr("b_hitmarker"));

		setup_var(&var.visuals.i_shot_logs, 0, XorStr("Visuals - Misc"), XorStr("i_shot_logs"));
		setup_var(&var.visuals.i_buy_logs, 0, XorStr("Visuals - Misc"), XorStr("i_buy_logs"));
		setup_var(&var.visuals.i_hurt_logs, 0, XorStr("Visuals - Misc"), XorStr("i_hurt_logs"));

		setup_var(&var.visuals.b_shot_log_lines, false, XorStr("Visuals - Misc"), XorStr("b_shot_log_lines"));
		setup_var(&var.visuals.f_shot_log_lines, 5.f, XorStr("Visuals - Misc"), XorStr("f_shot_log_lines"));

		setup_var(&var.visuals.b_stats, false, XorStr("Visuals - Misc"), XorStr("b_stats"));

		setup_var(&var.visuals.b_spread_circle, false, XorStr("Visuals - Misc"), XorStr("b_spread_circle"));
		setup_var(&var.visuals.col_spread_circle, color_var(Color(255, 255, 255, 50).ToImGUI()), XorStr("Visuals - Misc"), XorStr("col_spread_circle"));

		setup_var(&var.visuals.b_impacts, false, XorStr("Visuals - Misc"), XorStr("b_impacts"));
		setup_var(&var.visuals.b_hit_snapshot, false, XorStr("Visuals - Misc"), XorStr("b_hit_snapshot"));

		setup_var(&var.visuals.asuswall.b_enabled, false, XorStr("Visuals - Misc"), XorStr("b_enabled"));
		setup_var(&var.visuals.asuswall.col_world, color_var(Color::White().ToImGUI()), XorStr("Visuals - Misc"), XorStr("col_world"));
		setup_var(&var.visuals.asuswall.col_prop, color_var(Color::White().ToImGUI()), XorStr("Visuals - Misc"), XorStr("col_prop"));
		setup_var(&var.visuals.asuswall.col_sky, color_var(Color::White().ToImGUI()), XorStr("Visuals - Misc"), XorStr("col_sky"));

		setup_var(&var.visuals.skychanger.b_enabled, false, XorStr("Visual - SkyChanger"), XorStr("b_enabled"));
		setup_var(&var.visuals.skychanger.i_type, 0, XorStr("Visual - SkyChanger"), XorStr("i_type"));

		setup_var(&var.visuals.nade_prediction.b_enabled, false, XorStr("Visual - Nade Prediction"), XorStr("b_enabled"));
		setup_var(&var.visuals.nade_prediction.col, color_var(ImColor(1.f, 1.f, 1.f, 1.f), 0, 0.f), XorStr("Visual - Color - Nade Prediction"), XorStr("col_controller"));
		setup_var(&var.visuals.nade_prediction.col_visible, color_var(ImColor(1.f, 1.f, 1.f, 1.f), 0, 0.f), XorStr("Visual - Color - Nade Prediction"), XorStr("col_visible"));
		setup_var(&var.visuals.nade_prediction.col_hit, color_var(ImColor(1.f, 1.f, 1.f, 1.f), 0, 0.f), XorStr("Visual - Color - Nade Prediction"), XorStr("col_hit"));

		//encrypts(0)
	}

	// MISC
	{
		//decrypts(0)
		setup_var(&var.misc.b_bhop, false, XorStr("Misc"), XorStr("b_bhop"));
		setup_var(&var.misc.b_bhop_duck, false, XorStr("Misc"), XorStr("b_bhop_duck"));

		setup_var(&var.misc.b_autostrafe, false, XorStr("Misc"), XorStr("b_autostrafe"));
		setup_var(&var.misc.b_unlimited_duck, false, XorStr("Misc"), XorStr("b_unlimited_duck"));

		// no default keys plz
		setup_var(&var.misc.i_fakeduck_key, 0, XorStr("Misc"), XorStr("i_fakeduck_key"));
		setup_var(&var.misc.i_minwalk_key, 0, XorStr("Misc"), XorStr("i_minwalk_key"));
		setup_var(&var.misc.f_minwalk_speed, 36.f, XorStr("Misc"), XorStr("f_minwalk_speed"));

		setup_var(&var.misc.b_autobuy, false, XorStr("Misc"), XorStr("b_autobuy"));
		setup_var(&var.misc.b_autobuy_ai, false, XorStr("Misc"), XorStr("b_autobuy_ai"));
		setup_var(&var.misc.i_autobuy_t_primary, 0, XorStr("Misc"), XorStr("i_autobuy_t_primary"));
		setup_var(&var.misc.i_autobuy_ct_primary, 0, XorStr("Misc"), XorStr("i_autobuy_ct_primary"));
		setup_var(&var.misc.i_autobuy_t_secondary, 0, XorStr("Misc"), XorStr("i_autobuy_t_secondary"));
		setup_var(&var.misc.i_autobuy_ct_secondary, 0, XorStr("Misc"), XorStr("i_autobuy_ct_secondary"));
		setup_var(&var.misc.b_autobuy_armor, false, XorStr("Misc"), XorStr("b_autobuy_armor"));
		setup_var(&var.misc.b_autobuy_decoy, false, XorStr("Misc"), XorStr("b_autobuy_decoy"));
		setup_var(&var.misc.b_autobuy_fire, false, XorStr("Misc"), XorStr("b_autobuy_fire"));
		setup_var(&var.misc.b_autobuy_flash, false, XorStr("Misc"), XorStr("b_autobuy_flash"));
		setup_var(&var.misc.b_autobuy_frag, false, XorStr("Misc"), XorStr("b_autobuy_frag"));
		setup_var(&var.misc.b_autobuy_kit, false, XorStr("Misc"), XorStr("b_autobuy_kit"));
		setup_var(&var.misc.b_autobuy_smoke, false, XorStr("Misc"), XorStr("b_autobuy_smoke"));
		setup_var(&var.misc.b_autobuy_zeus, false, XorStr("Misc"), XorStr("b_autobuy_zeus"));

		setup_var(&var.misc.i_clantag, 0, XorStr("Misc"), XorStr("i_clantag"));

		setup_var(&var.misc.b_single_scope, false, XorStr("Misc"), XorStr("b_single_scope"));
		setup_var(&var.misc.b_water_mark, false, XorStr("Misc"), XorStr("b_water_mark"));

		setup_var(&var.misc.b_preserve_killfeed, false, XorStr("Misc"), XorStr("b_preserve_killfeed"));
		setup_var(&var.misc.b_include_assists_killfeed, false, XorStr("Misc"), XorStr("b_include_assists_killfeed"));
		setup_var(&var.misc.f_preserve_killfeed, 10.f, XorStr("Misc"), XorStr("f_preserve_killfeed"));

		setup_var(&var.misc.namechanger.b_enabled, false, XorStr("Misc - NameChanger"), XorStr("b_enabled"));
		setup_var(&var.misc.namechanger.i_mode, 0, XorStr("Misc - NameChanger"), XorStr("i_mode"));

		setup_var(&var.misc.clantag.b_enabled, bool_sw(false, 0, 0), XorStr("Misc - ClanTag"), XorStr("b_enabled"));
		setup_var(&var.misc.clantag.str_text, std::string(XorStr("mutiny.pw")), XorStr("Misc - ClanTag"), XorStr("str_text"));
		setup_var(&var.misc.clantag.i_anim, 0, XorStr("Misc - ClanTag"), XorStr("i_anim"));

		setup_var(&var.misc.thirdperson.b_enabled, bool_sw(false, 0, 0), XorStr("Misc - Thirdperson"), XorStr("b_enabled"));
		setup_var(&var.misc.thirdperson.b_disable_on_nade, false, XorStr("Misc - Thirdperson"), XorStr("b_disable_on_nade"));
		setup_var(&var.misc.thirdperson.f_distance, 100.f, XorStr("Misc - Thirdperson"), XorStr("f_distance"));

		setup_var(&var.misc.b_spectate_all, false, XorStr("Misc - Camera"), XorStr("b_spectate_all"));
		//encrypts(0)
	}

	// WAYPOINTS
	{
		//decrypts(0)
		setup_var(&var.waypoints.i_createwaypoint_key, 0, XorStr("Waypoints"), XorStr("i_createwaypoint_key"));
		setup_var(&var.waypoints.i_deletewaypoint_key, 0, XorStr("Waypoints"), XorStr("i_deletewaypoint_key"));
		setup_var(&var.waypoints.b_enable_creator, false, XorStr("Waypoints"), XorStr("b_enable_creator"));
		setup_var(&var.waypoints.b_draw_oneways, false, XorStr("Waypoints"), XorStr("b_draw_oneways"));
		setup_var(&var.waypoints.b_draw_campspots, false, XorStr("Waypoints"), XorStr("b_draw_campspots"));
		setup_var(&var.waypoints.b_enable_walkbot, false, XorStr("Waypoints"), XorStr("b_enable_walkbot"));
		setup_var(&var.waypoints.b_walkbot_ragehack, false, XorStr("Waypoints"), XorStr("b_walkbot_ragehack"));
		setup_var(&var.waypoints.b_enable_autowaypoint, false, XorStr("Waypoints"), XorStr("b_enable_autowaypoint"));
		setup_var(&var.waypoints.b_download_waypoints, false, XorStr("Waypoints"), XorStr("b_download_waypoints"));
		//encrypts(0)
	}

	// nit: NOTE: DO NOT TOUCH THESE LINES. IF YOU COMMENT/DELETE THESE TWO FOLLOWING LINES, EVERYTHING BREAKS. I DONT KNOW WHY BUT MURPHY'S LAW REALLY IS FUCKING MY ASS RIGHT NOW.
	// THERE IS A HIDDEN HEAP CORRUPTION WE NEED TO LOOK INTO.
	// LAST WORKING COMMIT BEFORE THIS HAPPENING: f587804

	//decrypts(0)
	setup_var(&var.visuals.asuswall.b_enabled, false, XorStr("Visual - AsusWall"), XorStr("b_enabled"));
	setup_var(&var.visuals.skychanger.b_enabled, false, XorStr("Visual - SkyChanger"), XorStr("b_enabled"));
	setup_var(&var.visuals.skychanger.i_type, 0, XorStr("Visual - SkyChanger"), XorStr("i_type"));
	setup_var(&var.visuals.nade_prediction.b_enabled, false, XorStr("Visual - Nade Prediction"), XorStr("b_enabled"));

	setup_var(&var.visuals.asuswall.col_world, color_var(ImColor(1.f, 1.f, 1.f, 1.f), 0, 0.f), XorStr("Visual - Color - AsusWall"), XorStr("col_world"));
	setup_var(&var.visuals.asuswall.col_prop, color_var(ImColor(1.f, 1.f, 1.f, 1.f), 0, 0.f), XorStr("Visual - Color - AsusWall"), XorStr("col_prop"));
	setup_var(&var.visuals.asuswall.col_sky, color_var(ImColor(1.f, 1.f, 1.f, 1.f), 0, 0.f), XorStr("Visual - Color - AsusWall"), XorStr("col_sky"));
	setup_var(&var.visuals.nade_prediction.col, color_var(ImColor(1.f, 1.f, 1.f, 1.f), 0, 0.f), XorStr("Visual - Color - Nade Prediction"), XorStr("col_controller"));
	setup_var(&var.visuals.nade_prediction.col_visible, color_var(ImColor(1.f, 1.f, 1.f, 1.f), 0, 0.f), XorStr("Visual - Color - Nade Prediction"), XorStr("col_visible"));
	setup_var(&var.visuals.nade_prediction.col_hit, color_var(ImColor(1.f, 1.f, 1.f, 1.f), 0, 0.f), XorStr("Visual - Color - Nade Prediction"), XorStr("col_hit"));
	//encrypts(0)
}

void config::refresh()
{
	auto &var = variable::get();
	if (std::get<2>(var.global.cfg_mthread))
	{
		auto str_value = std::get<0>(var.global.cfg_mthread);

		//decrypts(0)
		if (str_value == XorStr("create"))
			create(std::get<1>(var.global.cfg_mthread));
		else if (str_value == XorStr("load"))
			load(std::get<1>(var.global.cfg_mthread));
		else if (str_value == XorStr("save"))
			save(std::get<1>(var.global.cfg_mthread));
		else if (str_value == XorStr("remove"))
			remove(std::get<1>(var.global.cfg_mthread));

		//encrypts(0)

		std::get<2>(var.global.cfg_mthread) = false;
	}
}

bool config::create(std::string str_cfg)
{
	if (str_cfg.empty())
	{
		//decrypts(0)
		logger::add(LERROR, XorStr("[ Config ] Empty config name."));
		//encrypts(0)
		return false;
	}

	//decrypts(0)
	auto str_file = adr_util::string::format(XorStr("%s\\%s.%s"), str_dir.c_str(), adr_util::string::replace(str_cfg, " ", "_").c_str(), str_file_end.c_str());
	//encrypts(0)
	if (std::experimental::filesystem::v1::exists(str_file.c_str()))
	{
		//decrypts(0)
		logger::add(LERROR, XorStr("[ Config ] File already exists."));
		//encrypts(0)
		return false;
	}

	std::ofstream ofs_file;
	ofs_file.open(str_file.c_str(), std::ios::out | std::ios::trunc);
	//decrypts(0)
	ofs_file << XorStr("null");
	//encrypts(0)
	ofs_file.close();

	//decrypts(0)
	logger::add(LSUCCESS, XorStr("[ Config ] Created a new data file at %s."), str_file.c_str());
	//encrypts(0)

	save(str_cfg);
	return true;
}

bool config::load(std::string str_cfg)
{
	VMP_BEGINMUTILATION("config # load");
	if (str_cfg.empty())
	{
		//decrypts(0)
		logger::add(LERROR, XorStr("[ Config ] Empty config name."));
		//encrypts(0)
		return false;
	}

	//decrypts(0)
	auto str_file = adr_util::string::format(XorStr("%s\\%s.%s"), str_dir.c_str(), adr_util::string::replace(str_cfg, " ", "_").c_str(), str_file_end.c_str());
	//encrypts(0)
	if (!std::experimental::filesystem::v1::exists(str_file.c_str()))
	{
		//decrypts(0)
		logger::add(LERROR, XorStr("[ Config ] File does not exists."));
		//encrypts(0)
		return create(str_cfg);
	}

	std::ifstream ifs_file;
	ifs_file.open(str_file.c_str(), std::ios::in);
	std::string str_json(std::istreambuf_iterator<char>(ifs_file), {});

	//decrypts(0)
	std::string null = XorStr("null");
	//encrypts(0);
	if (str_json.empty() || str_json == null)
	{
		//decrypts(0)
		logger::add(LERROR, XorStr("[ Config ] File can not be loaded."));
		//encrypts(0)

		return false;
	}

	str_json = encrypt_decrypt(str_json); //crypt::get().aes256_decrypt(str_json, "dc99e138a186436b0d80406fd79a1b2a", "29c302523626df11");
	if (str_json.empty())
	{
		//decrypts(0)
		logger::add(LERROR, XorStr("[ Config ] File can not be decrypted."));
		//encrypts(0)
		return false;
	}

	auto js_config = json::parse(str_json);
	if (js_config.empty())
	{
		//decrypts(0)
		logger::add(LERROR, XorStr("[ Config ] File is not json alike."));
		//encrypts(0)
		return false;
	}
	//std::cout << js_config.dump(4) << std::endl;

	for (auto it = js_config.begin(); it != js_config.end(); ++it)
	{
		auto it_key = it.key();
		if (it_key.empty())
			continue;

		auto it_val = it.value();
		if (it_val.empty())
			continue;

		for (auto ito = it_val.begin(); ito != it_val.end(); ++ito)
		{
			auto ito_key = ito.key();
			if (ito_key.empty())
				continue;

			auto ito_val = ito.value();
			if (ito_val.empty())
				continue;

			auto item = find(it_key, ito_key);
			if (!item)
				continue;

			try
			{
				if (item->ti_type == std::type_index(typeid(int)))
					*(int*)item->p_option = ito_val.get<int>();
				else if (item->ti_type == std::type_index(typeid(bool)))
					*(bool*)item->p_option = ito_val.get<bool>();
				else if (item->ti_type == std::type_index(typeid(float)))
					*(float*)item->p_option = ito_val.get<float>();
				else if (item->ti_type == std::type_index(typeid(const char*)))
					*(const char**)item->p_option = ito_val.get<std::string>().c_str();
				else if (item->ti_type == std::type_index(typeid(std::string)))
					*(std::string*)item->p_option = ito_val.get<std::string>();
				else if (item->ti_type == std::type_index(typeid(bool_sw)))
				{
					auto& ptr = *(bool_sw*)item->p_option;

					//decrypts(0)
					ptr.b_state = ito_val[XorStr("state")].get<bool>();
					ptr.i_mode = ito_val[XorStr("mode")].get<int>();
					ptr.i_key = ito_val[XorStr("key")].get<int>();
					//encrypts(0)
				}
				else if (item->ti_type == std::type_index(typeid(color_var)))
				{
					auto& ptr = *(color_var*)item->p_option;

					//decrypts(0)
					ptr.col_color.Value.x = ito_val[XorStr("r")].get<float>();
					ptr.col_color.Value.y = ito_val[XorStr("g")].get<float>();
					ptr.col_color.Value.z = ito_val[XorStr("b")].get<float>();
					ptr.col_color.Value.w = ito_val[XorStr("a")].get<float>();
					ptr.f_rainbow_speed = ito_val[XorStr("speed")].get<float>();
					ptr.i_mode = ito_val[XorStr("mode")].get<int>();
					//encrypts(0)
				}
				else if (item->ti_type == std::type_index(typeid(health_color_var)))
				{
					auto& ptr = *(health_color_var*)item->p_option;

					//decrypts(0)
					ptr.col_color.Value.x = ito_val[XorStr("r")].get<float>();
					ptr.col_color.Value.y = ito_val[XorStr("g")].get<float>();
					ptr.col_color.Value.z = ito_val[XorStr("b")].get<float>();
					ptr.col_color.Value.w = ito_val[XorStr("a")].get<float>();
					ptr.f_rainbow_speed = ito_val[XorStr("speed")].get<float>();
					ptr.i_mode = ito_val[XorStr("mode")].get<int>();
					//encrypts(0)
				}
			}
			catch (json::exception& e)
			{
				//decrypts(0)
				logger::add(LERROR, XorStr("[ Config ] str_catg -> [ %s ] | str_name -> [ %s ] | e.what() -> [ %s ] | e.id() -> [ %d ]."), item->str_catg.c_str(), item->str_name.c_str(), e.what(), e.id);
				//encrypts(0)
			}
		}
	}

	auto& var = variable::get();

	// fixes for anyone using older configs
	if (var.visuals.i_thirdperson_key != 0)
	{
		var.misc.thirdperson.b_enabled.i_key = var.visuals.i_thirdperson_key;
		var.misc.thirdperson.b_enabled.i_mode = 2;
	}

	if (var.ragebot.baim_main.b_enabled)
	{
		var.ragebot.baim_main.b_force.b_state = true;
		var.ragebot.baim_main.b_force.i_mode = 0;

		// stop loading this the next time they save
		var.ragebot.baim_main.b_enabled = false;
	}

	if (var.ragebot.baim_main.b_cant_resolve)
	{
		// turn off 'cant resolve' and enable 'not guaranteed resolve' by default
		// for anyone that had it on in the past
		var.ragebot.baim_main.b_cant_resolve = false;
		var.ragebot.baim_main.b_not_body_hit_resolved = true;
	}

	// force enables

	// force forwardtrack and experimental resolver on
	var.ragebot.b_resolver_experimental = true;
	var.ragebot.f_resolver_experimental_leniency_exclude = 1.1f;
	var.ragebot.f_resolver_experimental_leniency_impact = 0.f;

	//force enable
	var.ragebot.b_use_alternative_multipoint = true;

	// force disables until further notice
	var.ragebot.exploits.b_hide_record = false;
	var.ragebot.b_make_all_misses_count = false;
	var.ragebot.b_strict_hitboxes = false;

	ifs_file.close();
	//decrypts(0)
	logger::add(LSUCCESS, XorStr("[ Config ] Loaded data from %s."), str_file.c_str());
	//encrypts(0)
	return true;
	VMP_END;
}

bool config::save(std::string str_cfg)
{
	VMP_BEGINMUTILATION("config # item type 02");
	if (str_cfg.empty())
	{
		//decrypts(0)
		logger::add(LERROR, XorStr("[ Config ] Empty config name."));
		//encrypts(0)
		return false;
	}

	//decrypts(0)
	auto str_file = adr_util::string::format(XorStr("%s\\%s.%s"), str_dir.c_str(), adr_util::string::replace(str_cfg, " ", "_").c_str(), str_file_end.c_str());
	//encrypts(0)
	if (!std::experimental::filesystem::v1::exists(str_file.c_str()))
	{
		//decrypts(0)
		logger::add(LWARN, XorStr("[ Config ] File does not exists! Creating..."));
		//encrypts(0)
	}

	json js_config;
	for (auto it = item.begin(); it != item.end(); ++it)
	{
		auto& config = *it;
		if (config.ti_type == std::type_index(typeid(int)))
			js_config[config.str_catg][config.str_name] = (int)*(int*)config.p_option;
		else if (config.ti_type == std::type_index(typeid(float)))
			js_config[config.str_catg][config.str_name] = (float)*(float*)config.p_option;
		else if (config.ti_type == std::type_index(typeid(bool)))
			js_config[config.str_catg][config.str_name] = (bool)*(bool*)config.p_option;
		else if (config.ti_type == std::type_index(typeid(const char*)))
			js_config[config.str_catg][config.str_name] = (const char*)*(const char**)config.p_option;
		else if (config.ti_type == std::type_index(typeid(std::string)))
			js_config[config.str_catg][config.str_name] = (std::string)*(std::string*)config.p_option;
		else if (config.ti_type == std::type_index(typeid(bool_sw)))
		{
			auto ptr = *(bool_sw*)config.p_option;
			//decrypts(0)
			js_config[config.str_catg][config.str_name][XorStr("state")] = ptr.b_state;
			js_config[config.str_catg][config.str_name][XorStr("mode")] = ptr.i_mode;
			js_config[config.str_catg][config.str_name][XorStr("key")] = ptr.i_key;
			//encrypts(0)
		}
		else if (config.ti_type == std::type_index(typeid(color_var)))
		{
			auto ptr = *(color_var*)config.p_option;
			//decrypts(0)
			js_config[config.str_catg][config.str_name][XorStr("r")] = ptr.col_color.Value.x;
			js_config[config.str_catg][config.str_name][XorStr("g")] = ptr.col_color.Value.y;
			js_config[config.str_catg][config.str_name][XorStr("b")] = ptr.col_color.Value.z;
			js_config[config.str_catg][config.str_name][XorStr("a")] = ptr.col_color.Value.w;
			js_config[config.str_catg][config.str_name][XorStr("mode")] = ptr.i_mode;
			js_config[config.str_catg][config.str_name][XorStr("speed")] = ptr.f_rainbow_speed;
			//encrypts(0)
		}
		else if (config.ti_type == std::type_index(typeid(health_color_var)))
		{
			auto ptr = *(health_color_var*)config.p_option;
			//decrypts(0)
			js_config[config.str_catg][config.str_name][XorStr("r")] = ptr.col_color.Value.x;
			js_config[config.str_catg][config.str_name][XorStr("g")] = ptr.col_color.Value.y;
			js_config[config.str_catg][config.str_name][XorStr("b")] = ptr.col_color.Value.z;
			js_config[config.str_catg][config.str_name][XorStr("a")] = ptr.col_color.Value.w;
			js_config[config.str_catg][config.str_name][XorStr("mode")] = ptr.i_mode;
			js_config[config.str_catg][config.str_name][XorStr("speed")] = ptr.f_rainbow_speed;
			//encrypts(0)
		}
	}

	//std::cout << js_config.dump(4) << std::endl;

	std::ofstream ofs_file;
	ofs_file.open(str_file.c_str(), std::ios::out | std::ios::trunc);
	ofs_file << encrypt_decrypt(js_config.dump()); // crypt::get().aes256_encrypt(js_config.dump(), "dc99e138a186436b0d80406fd79a1b2a", "29c302523626df11");
	ofs_file.close();

	//decrypts(0)
	logger::add(LSUCCESS, XorStr("[ Config ] Saved data at %s."), str_file.c_str());
	//encrypts(0)

	return true;
	VMP_END;
}

bool config::remove(std::string str_cfg) const
{
	if (str_cfg.empty())
	{
		//decrypts(0)
		logger::add(LERROR, XorStr("[ Config ] Empty config name."));
		//encrypts(0)
		return false;
	}

	//decrypts(0)
	std::string file = adr_util::string::format(XorStr("%s\\%s.%s"), str_dir.c_str(), str_cfg.c_str(), str_file_end.c_str());
	//encrypts(0)

	if (std::remove(file.data()) != 0)
	{
		//decrypts(0)
		logger::add(LERROR, XorStr("[ Config ] Failed to delete %s."), str_cfg.c_str());
		//encrypts(0)
		return false;
	}

	//decrypts(0)
	logger::add(LSUCCESS, XorStr("[ Config ] %s deleted."), str_cfg.c_str());
	//encrypts(0)
	return true;
}

std::vector<std::string> config::get_configs() const
{
	std::vector<std::string> names;
	//decrypts(0)
	auto search_path = adr_util::string::format(XorStr("%s\\*.%s"), str_dir.c_str(), str_file_end.c_str());
	//encrypts(0)

	WIN32_FIND_DATA fd;
	const auto h_find = FindFirstFile(search_path.c_str(), &fd);
	if (h_find != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
				names.push_back(adr_util::string::replace(std::string(fd.cFileName), "." + str_file_end, ""));
		}
		while (FindNextFile(h_find, &fd));
		FindClose(h_find);
	}
	return names;
}

const std::string config::get_config_directory() const
{
	return str_dir;
}
