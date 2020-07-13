#pragma once

class SetUpVelocityResults_t {
public:
	void ClearResults() {
		m_use_output_as_input = false;
		m_ransetupvelocity = false;
		m_lby_timer_updated = false;
		m_lby_updated = false;
		m_979_triggered = false;
	}
	SetUpVelocityResults_t() {
		ClearResults();
		m_bodyyaw = 0.0f;
		m_absangles = { 0.0f,0.0f,0.0f };
		m_curfeetyaw = 0.0f;
		m_goalfeetyaw = 0.0f;
		m_last_animation_update_time = 0.0f;
		m_next_lby_update_time = 0.0f;
		m_predicted_lowerbodyyaw = 0.0f;
		m_duckamount = 0.0f;
		m_velocity = { 0.0f,0.0f, 0.0f };
	}
	SetUpVelocityResults_t(C_CSGOPlayerAnimState *input_animstate, float input_next_lby_update_time = 0, float input_predicted_lby = 0)
	{
		m_curfeetyaw = input_animstate->m_flCurrentFeetYaw;
		m_goalfeetyaw = input_animstate->m_flGoalFeetYaw;
		m_velocity = input_animstate->m_vVelocity;
		m_duckamount = input_animstate->m_fDuckAmount;
		m_next_lby_update_time = input_next_lby_update_time;
		m_predicted_lowerbodyyaw = input_predicted_lby;
		m_use_output_as_input = true;
	}
	bool m_use_output_as_input;
	bool m_ransetupvelocity;
	bool m_lby_timer_updated;
	bool m_lby_updated;
	bool m_979_triggered;
	float m_bodyyaw;
	QAngle m_absangles;

	float m_curfeetyaw;
	float m_goalfeetyaw;
	float m_last_animation_update_time;
	float m_next_lby_update_time;
	float m_predicted_lowerbodyyaw;
	float m_duckamount;
	Vector m_velocity;
};