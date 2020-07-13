#pragma once

class StatsTracker
{
public:
	StatsTracker() = default;
	~StatsTracker() = default;

	void clear();

	int m_head_hits = 0;
	int m_head_misses = 0;

	int m_body_hits = 0;
	int m_body_misses = 0;

	int m_arms_hits = 0;
	int m_arms_misses = 0;

	int m_lower_hits = 0;
	int m_lower_misses = 0;

	int m_headshots = 0;
	int m_shotbt_headshots = 0;

	int m_baims = 0;
	int m_shotbt_baims = 0;
};

extern StatsTracker g_StatsTracker;