#pragma once

#include "Includes.h"

class Fakelag
{
public:
	Fakelag() = default;
	~Fakelag() = default;

	void run();
	void get_visual_progress();

	float get_visual_choke();


private:

	int mode_adaptive();
	int mode_step();
	int mode_pingpong();
	int mode_latency_seed(int& limit);

	// step related vars
	int m_step_limit = 1;

	// ping pong related vars
	int m_pingpong_limit = 1;
	bool m_pingpong_increase = true;

	// latency related vars
	float m_latencyseed_lastavg = 100.f;

	float visual_choke = 0.f;
};

extern Fakelag g_Fakelag;