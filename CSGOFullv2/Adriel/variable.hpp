#pragma once
#include "stdafx.hpp"
#include "custom_def.hpp"
#include "../HitboxDefines.h"

struct Waypoint;

class variable : public singleton<variable>
{
public:
	variable();
	~variable();

	bool_sw b_ok;

	struct struct_legitbot
	{
		struct struct_aim
		{
			bool_sw b_enabled;
			bool b_lag_compensation {};
			bool b_per_weapon {};
		} aim;

		struct struct_trigger
		{
			bool_sw b_enabled;
		} trigger;

		struct struct_trigger_cfg
		{
			int i_delay;
			float f_hitchance;

			bool b_auto_scope;
			bool b_check_smoke;
			bool b_check_flash;
			bool b_check_scope;
			bool b_autowall;

			bool b_filter_head;
			bool b_filter_chest;
			bool b_filter_stomach;
			bool b_filter_arms;
			bool b_filter_legs;
		};
		std::array<struct_trigger_cfg, 7> trigger_cfg {};

		struct struct_aim_cfg
		{

			struct_aim_cfg();

			float f_fov{};
			bool b_incross{};
			bool b_dynamic{};

			int i_retarget_time{};

			std::vector<std::tuple<std::string, int, bool>> hitboxes;

			bool b_aim_time{};
			int i_speed_type{};
			float f_speed{};
			bool b_randomize_speed{};

			float f_y_rcs{};
			float f_x_rcs{};
			bool b_randomize_rcs{};

			float f_hitchance{};

			bool b_autowall{};
			bool b_auto_pistol{};

			int i_slow_moving{};
			int i_activation_shots{};

			bool b_check_scope{};
			bool b_check_smoke{};
			bool b_check_flash{};

			bool b_skip_incross{};
			bool b_first_shot_assist{};
			bool b_lock{};
			bool b_auto_shoot{};
			bool b_auto_scope{};
			bool b_silentaim{};
			bool b_auto_stop{};
			bool b_aim_step{};
			bool b_wait_for_in_attack{};

			bool b_standalone_rcs{};

			float f_y_standalone_rcs{};
			float f_x_standalone_rcs{};
			float f_standalone_rcs_speed{};

			//int i_standalone_rcs_activation_shots; // that's really not needed lol

			bool b_standalone_rcs_randomize_speed{};
			bool b_standalone_rcs_randomize_rcs{};
			bool b_standalone_rcs_after_kill{};
		};
		std::array<struct_aim_cfg, 73> aim_cfg;
	} legitbot;

	struct struct_skinchanger
	{
		bool b_enabled = false;

		struct struct_weapon
		{
			int i_item_definition_index;
			int i_fallback_paint_kit;
			int i_fallback_seed;
			int i_fallback_stat_trak;
			int i_entity_quality;
			float f_fallback_wear;

			std::string str_name;
			std::array<std::pair<int, float>, 5> sticker;
		};

		std::unordered_map<int, struct_weapon> weapon_ct;
		std::unordered_map<int, struct_weapon> weapon_tr;
	} skinchanger;

	struct struct_global
	{
		bool b_reseting = false;

		std::tuple<std::string, std::string, bool> cfg_mthread = { "", "", false };
	} global;

	struct struct_ui
	{
		bool b_init = false;
		bool b_stream_proof = false;
		bool b_unload = false;
		bool b_warn = false;
		bool b_only_one = false;
		bool b_use_tooltips = false;
		bool b_allow_manual_edits = false;
		bool b_allow_untrusted = false;

		bool b_visual = false; // nit; todo: set this to false when done with visual tab
		bool b_legit = false;
		bool b_rage = false;
		bool b_playerlist = false;
		bool b_misc = false;
		bool b_waypoints = false;
		bool b_skin = false;
		bool b_config = false;

		color_var col_controller;
		color_var col_background;
		color_var col_text;

		float f_menu_time = 0.f;
	} ui;

	struct struct_waypoints
	{
		struct struct_SelectedWorldPosition
		{
			Vector m_vecOrigin;
			Vector m_vecDir;

			void Reset()
			{
				m_vecOrigin.Init();
				m_vecDir.Init();
			};
			
			struct_SelectedWorldPosition()
			{
				Reset();
			};
			
		} SelectedWorldPosition;

		Waypoint *m_pSelectedWaypoint = nullptr;

		void ResetSelectedInformation()
		{
			SelectedWorldPosition.Reset();
			m_pSelectedWaypoint = nullptr;
		}

		bool b_enable_creator = false;
		bool b_enable_autowaypoint = false;
		bool b_draw_oneways = false;
		bool b_draw_campspots = false;
		bool b_enable_walkbot = false;
		bool b_walkbot_ragehack = false;
		bool b_download_waypoints = false;
		int i_createwaypoint_key = (int)VK_XBUTTON1;
		int i_deletewaypoint_key = (int)VK_XBUTTON1;

		struct struct_editor
		{
			bool b_jump = false;
			bool b_duck = false;
			bool b_fakeduck = false;
			bool b_campspot = false;
			bool b_oneway = false;
			bool b_oneway_victim = false;
			bool b_bombsite = false;
			bool b_safezone = false;
			bool b_ctspawn = false;
			bool b_tspawn = false;
			bool b_ladder_top = false;
			bool b_ladder_step = false;
			bool b_ladder_bottom = false;
			bool b_use = false;
			bool b_path = false;
			bool b_breakable = false;
		} editor;
	} waypoints;

	struct struct_ragebot
	{
		bool b_enabled = true;
		bool b_resolver = true;
		int i_flipenemy_key = 0;
		bool b_resolver_experimental = true;
		bool b_resolver_moving = true;
		bool b_resolver_nov6build = true;
		bool_sw b_resolver_nojitter = bool_sw(false, 0, 0);
		float f_resolver_experimental_leniency_exclude = 1.0f;
		float f_resolver_experimental_leniency_impact = 0.0f;
		bool b_multithreaded = true;
		bool b_silent = true;
		bool b_autofire = true;
		bool b_autoscope = true;
		bool b_autostop = false;
		bool b_ignore_limbs_if_moving = true;
		float f_ignore_limbs_if_moving = 0.1f;
		bool b_scan_through_teammates = false;
		bool b_make_all_misses_count = false;
		bool b_strict_hitboxes = false;
		int i_sort = 0;
		int i_targets_per_tick = 2;

		bool_sw b_forwardtrack;

		struct hitscan
		{
			// are we targeting this hitbox
			bool b_enabled;
			// are we doing multipoint on this hitbox
			bool b_multipoint;
			// what is the pointscale for this hitbox
			float f_pointscale;
			// what is the priority of this hitbox (highest = most prioritized)
			int i_priority;
		};

		bool b_advanced_multipoint = false;

		// todo: nit; weapon configs for these
		hitscan hitscan_head{};
		//hitscan hitscan_neck {};	todo: nit; possibly add this later
		hitscan hitscan_chest {};
		hitscan hitscan_shoulders{};
		hitscan hitscan_stomach {}; 
		hitscan hitscan_arms {};
		hitscan hitscan_hands{};
		hitscan hitscan_legs {};
		hitscan hitscan_feet {};

		int i_mindmg = 0;
		int i_mindmg_aw = 0;

		float f_hitchance = 0.f; // head hitchance
		float f_body_hitchance = 0.f;
		float f_doubletap_hitchance = 0.f;

		struct baim
		{
			// outdated - are we forcing baim
			bool b_enabled;
			// are we forcing baim
			bool_sw b_force;
			// are we forcing baim when damage is lethal
			bool b_lethal = false;
			// are we forcing baim when we can't resolve them
			bool b_cant_resolve = false;
			// are we forcing baim when they are not body hit resolved
			bool b_not_body_hit_resolved = false;
			// are we forcing baim when targets are airborne
			bool b_airborne_target = false;
			// are we forcing baim when we're airborne
			bool b_airborne_local = false;
			// are we forcing baim when targets are moving
			bool b_moving_target = false;
			// are we forcing baim after x amount of misses
			bool b_after_misses = false;
			// the number of misses we're waiting to force baim
			int i_after_misses = 1;
			// the min speed at which we force a baim on a target
			float f_moving_target = 1.f;
			//forcing baim if health is = or under baim health value
			bool b_after_health = false;
			//health value for bodyaim
			float body_aim_health = 10.f;

			bool can_baim();
		};

		baim baim_main {};

		struct fakelag
		{
			// fakelag enabled?
			bool b_enabled;
			// disrupt enabled?
			bool b_disrupt;

			// the current mode ( 0 = static | 1 = step | 2 = adaptive | 3 = pingpong | 4 = latency seed | 5 = random )
			int i_mode;

			// flags when to trigger fakelag
			int i_trigger_flags;
			// flags on when to disable fakelag
			int i_disable_flags;

			// static amount of ticks
			int i_static_ticks;
			// minimum amount of ticks
			int i_min_ticks;
			// maximum amount of ticks
			int i_max_ticks;
			// chance to disrupt
			float f_disrupt_chance;
		};

		struct hvh
		{
			int i_pitch = 0;
			int i_yaw = 0;
			
			float f_custom_pitch = 89.f;
			float f_yaw_offset = 0.f;
			float f_desync_amt = 120.f;
			float f_desync_amt_fast = 120.f; // obsolete due to minwalk and moving mode
			float f_lby_delta = 160.f;
			float f_jitter = 1.f;
			float f_jitter_fast = 1.f; // obsolete due to minwalk and moving mode
			float f_spin_speed = 1.f;
			float f_sway_speed = 5.0f;

			bool b_desync = 0;
			bool b_jitter = false;
			bool b_apply_freestanding = false;
			bool b_apply_walldtc = false;
			bool b_apply_sway = false;
			bool b_sway_wait = false;
			bool b_break_lby_by_flicking = false;
			float f_sway_min = 0.01f;
			float f_sway_max = 0.25f;

			int i_desync_style = 0;

			fakelag fakelag;
		};

		struct exploits
		{
			bool b_hide_shots = false;
			bool b_hide_record = false;
			bool b_disable_fakelag_while_multitapping = true;

			int i_ticks_to_wait = 15;

			bool_sw b_multi_tap = bool_sw(false, 0, 0);
			bool_sw b_nasa_walk = bool_sw(false, 0, 0);

		} exploits;

		bool b_antiaim = false;

		hvh standing {};
		hvh moving {};
		hvh minwalking{};
		hvh in_air {};

		float f_freestanding_walldist = 25;
		bool b_freestand_dormant = false;

		int i_fakelag_ticks = 1;		// obsolete due to hvh modes
		int i_fakelag_mode = 0;			// obsolete due to hvh modes
		bool b_fakelag_peek = false;
		bool b_fakelag_land = false;	// obsolete due to hvh modes

		bool b_auto_backwards = false;
		int i_auto_backwards = 1;

		int i_manual_aa_left_key = (int)VK_LEFT;
		int i_manual_aa_right_key = (int)VK_RIGHT;
		int i_manual_aa_back_key = (int)VK_DOWN;
		int i_manual_aa_lean_dir = (int)VK_MENU;

		int i_override_key = (int)VK_XBUTTON2;

		float f_fake_latency = 1.f;

		int i_double_tap = 1;		   // obsolete

		bool b_lagcomp_use_simtime = false;
		bool b_use_alternative_multipoint = true;
		bool b_safe_point = true;
		bool b_safe_point_head = false;

		bool is_hitscanning(bool* has_body, bool* has_head);

		bool is_multipointing(std::vector<std::pair<int, float>>* active = nullptr);

	} ragebot;

	struct struct_visuals
	{
		// enable visuals entirely
		bool b_enabled = true;
		bool b_screenshot_safe = false;

		struct cham_options
		{
			// enable chams on this entity
			bool b_enabled = true;

			// enable xqz
			bool b_xqz = true;

			// the material to use
			int i_mat_visible = 0;
			int i_mat_invisible = 0;
			int i_mat_desync_type = 0;

			// visible color
			color_var col_visible;
			// invisible color
			color_var col_invisible;
		};

		struct glow_options
		{
			// enable glow on this entity
			bool b_enabled = true;

			// fullbloom
			bool b_fullbloom = false;

			// type of glow
			int i_type = 0;

			// blend amount
			float f_blend = 100.f;

			// color
			color_var col_color;
		};

		struct visual_filter
		{
			// enable visuals on this filter
			bool b_enabled = true;
			// render the filter
			bool b_render = true;
			// enable box drawing
			bool b_box = false;
			// enable drawing name of entity
			bool b_name = true;
			// enable extra info
			bool b_info = false;
			// enable dormant esp for entity
			float f_timeout = 0.f;
			// colors for the entity's name
			color_var col_name;

			//colors of entity's ammo bar
			color_var col_ammo;

			// the main color for the entity
			color_var col_main;
			// chams for this entity
			cham_options chams;
			// glow for this entity
			glow_options glow;
		};

		struct player_filter
		{
			// esp-flags
			bool b_weapon = true;
			bool b_health = true;
			bool b_armor = false;
			bool b_money = false;
			bool b_kevlar_helm = false;
			bool b_bomb_carrier = true;
			bool b_has_kit = true;
			bool b_scoped = true;
			bool b_pin_pull = false;
			bool b_blind = true;
			bool b_burn = true;
			bool b_bomb_interaction = true;		// planting and defusing
			bool b_reload = true;
			bool b_vuln = false;
			bool b_resolver = true;
			bool b_assistant_hc = false;
			bool b_assistant_mindmg = false;
			bool b_aimbot_debug = false;
			bool b_entity_debug = false;		// draw id, position
			bool b_fakeduck = true;

			bool b_backtrack = true;
			color_var col_backtrack;

			bool b_faresp = false;
			int i_faresp = 20;		// number of packets sent out

			bool b_offscreen_esp = false;
			float f_offscreen_esp = 15.f;
			color_var col_offscreen_esp;

			bool b_smooth_animation = false;

			visual_filter vf_main;
		};

		// todo: nit; replace pf_local_player etc with these
		enum PlayerFilter_t
		{
			PLAYER_FILTER_LOCAL,
			PLAYER_FILTER_ENEMY,
			PLAYER_FILTER_TEAMMATE,
			PLAYER_FILTER_MAX,
		};

		enum VisualFilter_t
		{
			VISUAL_FILTER_BOMB,
			VISUAL_FILTER_PROJECTILE,
			VISUAL_FILTER_WEAPON,
			VISUAL_FILTER_RAGDOLL,
			VISUAL_FILTER_HANDS,
			VISUAL_FILTER_ARMS,
			VISUAL_FILTER_VIEWMODEL
		};

		// -- one tab control - first column --
		//std::array<player_filter, PF_MAX> player_filters;

		player_filter pf_local_player;
		player_filter pf_enemy;
		player_filter pf_teammate;

		visual_filter vf_bomb;			// planted c4 and dropped c4
		visual_filter vf_projectile;	// thrown and dropped projectiles
		visual_filter vf_weapon;		// dropped weapons
		visual_filter vf_ragdolls;		// dead entities

		visual_filter vf_arms;			// viewmodel arms
		visual_filter vf_viewweapon;		// viewmodel weapon

		// -- unique variables that these filters don't need to share --

		// local player specific
		bool b_fake = true;
		color_var col_fake = color_var(Color(255, 0, 0, 165).ToImGUI());

		bool b_verbose_resolver = false;

		// show the projectile owner name
		bool b_projectile_owner = false;
		// show the projectile time ( smoke, molly/incendiary )
		bool b_projectile_timer = false;
		// show the range of projectile
		bool b_projectile_range = false;

		// show the planted bomb timer
		bool b_c4timer = true;

		// -- misc visuals - second column --

		bool b_aimbot_feedback = false;
		bool b_aimbot_points = false;

		// antiaim angles
		bool b_show_antiaim_angles = true;

		bool b_show_antiaim_real = true;
		bool b_show_antiaim_fake = true;
		bool b_show_antiaim_lby = false;

		// visualize sound
		int i_visualize_sound = 0;
		bool b_visualize_sound_enemy = true;
		bool b_visualize_sound_teammate = true;
		color_var col_visualize_sound_enemy = color_var(Color::Magenta().ToImGUI());
		color_var col_visualize_sound_teammate = color_var(Color::Cyan().ToImGUI());

		// offscreen esp - obsolete
		bool b_offscreen_esp = true;
		float f_offscreen_esp = 15.f;
		color_var col_offscreen_esp = color_var(Color::Red().ToImGUI());

		// thirdperson
		int i_thirdperson_key = (int)'C';

		// fov/camera modification
		float f_fov = 90.f;
		float f_thirdperson_fov = 100.f;
		float f_thirdperson_dist = 120.f;

		// removals
		bool b_no_smoke = true;
		bool b_no_visual_recoil = true;
		bool b_no_flash = true;
		bool b_no_zoom = false;

		// scope options
		bool b_no_scope = true;
		bool b_scope_lines = false;
		color_var col_scope_lines = color_var(Color::Black().ToImGUI(), 0, 0.f);

		// cham zoom blend for local player
		float f_blend_scope = 100.f;

		// autowall crosshair
		int i_autowall_xhair = 0;

		// indicators
		bool b_indicators = true;
		bool b_indicators_only_active = true;
		bool b_indicator_desync = true;
		bool b_indicator_lc = true;
		bool b_indicator_priority = true;
		bool b_indicator_edge = true;
		bool b_indicator_fakeduck = true;
		bool b_indicator_minwalk = true;
		bool b_indicator_choke = false;
		bool b_indicator_fake_latency = false;
		bool b_indicator_double_tap = false;
		bool b_indicator_tickbase = false;
		bool b_indicator_lean_dir = false;
		bool b_indicator_teleport = false;
		bool b_indicator_stats = false;

		// hit indicators
		bool b_hit_sound = false;
		bool b_hitmarker = false;

		// logs
		int i_shot_logs = 0;			// 1 - Console | 2 - Chat | 3 - Console & Chat
		int i_buy_logs = 0;
		int i_hurt_logs = 0;

		bool b_shot_log_lines = false;
		float f_shot_log_lines = 5.f;

		// stats
		bool b_stats = false;

		bool b_spread_circle = false;
		color_var col_spread_circle = color_var(Color::White().ToImGUI());

		// impacts
		bool b_impacts = false;
		bool b_hit_snapshot = false;

		// ADRIEL ... VAR
		struct struct_asuswall
		{
			bool b_enabled;

			color_var col_world;
			color_var col_prop;
			color_var col_sky;
		} asuswall;

		struct struct_skychanger
		{
			bool b_enabled = false;
			int i_type = 0;
		} skychanger;

		struct struct_nade_prediction
		{
			bool b_enabled;

			color_var col;
			color_var col_visible;	// visible
			color_var col_hit;		// impact pos
		} nade_prediction;

		struct struct_spectator_list
		{
			bool_sw b_enabled;
			float f_alpha;
		} spectator_list;
	} visuals;

	struct struct_misc
	{
		bool b_bhop = true;
		bool b_bhop_duck = false;
		bool b_autostrafe = true;
		bool b_unlimited_duck = true;

		int i_fakeduck_key = (int)VK_XBUTTON1;
		int i_minwalk_key = (int)VK_MENU;
		float f_minwalk_speed = 36.f;

		// autobuy
		bool b_autobuy = false;
		bool b_autobuy_ai = false;
		int i_autobuy_t_primary = 0;
		int i_autobuy_t_secondary = 0;
		int i_autobuy_ct_primary = 0;
		int i_autobuy_ct_secondary = 0;
		bool b_autobuy_frag = 0;
		bool b_autobuy_smoke = 0;
		bool b_autobuy_flash = 0;
		bool b_autobuy_fire = 0;
		bool b_autobuy_decoy = 0;
		bool b_autobuy_armor = 0;
		bool b_autobuy_zeus = 0;
		bool b_autobuy_kit = 0;

		// clantag
		int i_clantag = 0;

		bool b_single_scope = false;
		bool b_water_mark = false;

		// killfeed
		bool b_preserve_killfeed = false;
		bool b_include_assists_killfeed = false;
		float f_preserve_killfeed = 10.f;

		// spectate all
		bool b_spectate_all = false;

		struct struct_clantagchanger
		{
			bool_sw b_enabled;
			std::string str_text;
			int i_anim;
		} clantag;

		struct struct_disconnectreason
		{
			std::string str_text;
		} disconnect_reason;

		struct struct_namechanger
		{
			bool b_enabled;
			int i_mode;
		} namechanger;

		struct struct_thirdperson
		{
			bool_sw b_enabled;
			bool b_disable_on_nade;
			float f_distance;
		} thirdperson;

	} misc;

	struct struct_user
	{
		std::string str_hash = "";
		std::string str_user = "";
	} user;
}; //variable::get().ui.b_unload;