#include "StatsTracker.h"

StatsTracker g_StatsTracker;

void StatsTracker::clear()
{
	m_head_hits = 0;
	m_body_hits = 0;
	m_arms_hits = 0;
	m_lower_hits = 0;

	m_head_misses = 0;
	m_body_misses = 0;
	m_arms_misses = 0;
	m_lower_misses = 0;

	m_headshots = 0;
	m_baims = 0;

	m_shotbt_headshots = 0;
	m_shotbt_baims = 0;
}