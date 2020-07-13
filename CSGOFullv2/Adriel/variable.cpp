#include "variable.hpp"

variable::variable()
{
}

variable::~variable()
{
}

variable::struct_legitbot::struct_aim_cfg::struct_aim_cfg()
{
	//decrypts(0)
	hitboxes.push_back({ XorStr("HEAD"), HITBOX_HEAD, false });
	hitboxes.push_back({ XorStr("NECK"), HITBOX_LOWER_NECK, false });
	hitboxes.push_back({ XorStr("PELVIS"), HITBOX_PELVIS, false });
	hitboxes.push_back({ XorStr("BELLY"), HITBOX_BODY, false });
	hitboxes.push_back({ XorStr("THORAX"), HITBOX_THORAX, false });
	hitboxes.push_back({ XorStr("LOWER CHEST"), HITBOX_CHEST, false });
	hitboxes.push_back({ XorStr("UPPER CHEST"), HITBOX_UPPER_CHEST, false });
	hitboxes.push_back({ XorStr("RIGHT THIGH"), HITBOX_RIGHT_THIGH, false });
	hitboxes.push_back({ XorStr("RIGHT CALF"), HITBOX_RIGHT_CALF, false });
	hitboxes.push_back({ XorStr("RIGHT UPPER ARM"), HITBOX_RIGHT_UPPER_ARM, false });
	hitboxes.push_back({ XorStr("RIGHT FOREARM"), HITBOX_RIGHT_FOREARM, false });
	hitboxes.push_back({ XorStr("LEFT THIGH"), HITBOX_LEFT_THIGH, false });
	hitboxes.push_back({ XorStr("LEFT CALF"), HITBOX_LEFT_CALF, false });
	hitboxes.push_back({ XorStr("LEFT UPPER ARM"), HITBOX_LEFT_UPPER_ARM, false });
	hitboxes.push_back({ XorStr("LEFT FOREARM"), HITBOX_LEFT_FOREARM, false });
	//encrypts(0)
}

bool variable::struct_ragebot::baim::can_baim()
{
	return b_force.get() || b_lethal || b_cant_resolve || b_not_body_hit_resolved || b_airborne_target || b_airborne_local || b_moving_target;
}

bool variable::struct_ragebot::is_hitscanning(bool* has_body, bool* has_head)
{
	int active_count = 0;
	if (hitscan_head.b_enabled)
	{
		++active_count;
		*has_head = true;
	}
	if (hitscan_chest.b_enabled)
	{
		*has_body = true;
		++active_count;
	}
	if (hitscan_shoulders.b_enabled)
	{
		*has_body = true;
		++active_count;
	}
	if (hitscan_hands.b_enabled)
	{
		*has_body = true;
		++active_count;
	}
	/*
	if (hitscan_arms.b_enabled)
	{
		*has_body = true;
		++active_count;
	}
	*/
	if (hitscan_stomach.b_enabled)
	{
		*has_body = true;
		++active_count;
	}
	if (hitscan_legs.b_enabled)
	{
		*has_body = true;
		++active_count;
	}
	if (hitscan_feet.b_enabled)
	{
		*has_body = true;
		++active_count;
	}

	return active_count > 0;
}

bool variable::struct_ragebot::is_multipointing(std::vector<std::pair<int, float>>* active)
{
	int active_count = 0;
	if (hitscan_head.b_enabled && hitscan_head.b_multipoint)
	{
		++active_count;
		if (active != nullptr)
		{
			active->emplace_back(HITBOX_HEAD, hitscan_head.f_pointscale);
			active->emplace_back(HITBOX_LOWER_NECK, hitscan_head.f_pointscale);
		}
	}
	if (hitscan_chest.b_enabled && hitscan_chest.b_multipoint)
	{
		++active_count;
		if (active != nullptr)
		{
			active->emplace_back(HITBOX_THORAX, hitscan_chest.f_pointscale);
			active->emplace_back(HITBOX_CHEST, hitscan_chest.f_pointscale);
			active->emplace_back(HITBOX_UPPER_CHEST, hitscan_chest.f_pointscale);
		}
	}
	if (hitscan_arms.b_enabled && hitscan_arms.b_multipoint)
	{
		++active_count;
		if (active != nullptr)
		{
			active->emplace_back(HITBOX_LEFT_FOREARM, hitscan_arms.f_pointscale);
			active->emplace_back(HITBOX_RIGHT_FOREARM, hitscan_arms.f_pointscale);
			active->emplace_back(HITBOX_LEFT_UPPER_ARM, hitscan_arms.f_pointscale);
			active->emplace_back(HITBOX_RIGHT_UPPER_ARM, hitscan_arms.f_pointscale);
		}
	}
	if (hitscan_stomach.b_enabled && hitscan_stomach.b_multipoint)
	{
		++active_count;
		if (active != nullptr)
		{
			active->emplace_back(HITBOX_PELVIS, hitscan_stomach.f_pointscale);
			active->emplace_back(HITBOX_BODY, hitscan_stomach.f_pointscale);
		}
	}
	if (hitscan_legs.b_enabled && hitscan_legs.b_multipoint)
	{
		++active_count;
		if (active != nullptr)
		{
			active->emplace_back(HITBOX_LEFT_CALF, hitscan_legs.f_pointscale);
			active->emplace_back(HITBOX_RIGHT_CALF, hitscan_legs.f_pointscale);
			active->emplace_back(HITBOX_LEFT_THIGH, hitscan_legs.f_pointscale);
			active->emplace_back(HITBOX_RIGHT_THIGH, hitscan_legs.f_pointscale);
		}
	}
	if (hitscan_feet.b_enabled && hitscan_feet.b_multipoint)
	{
		++active_count;
		if (active != nullptr)
		{
			active->emplace_back(HITBOX_LEFT_FOOT, hitscan_feet.f_pointscale);
			active->emplace_back(HITBOX_RIGHT_FOOT, hitscan_feet.f_pointscale);
		}
	}

	return active_count != 0;
}