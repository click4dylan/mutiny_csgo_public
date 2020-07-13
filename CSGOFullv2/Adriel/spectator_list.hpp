#pragma once
#include "stdafx.hpp"

class spectator_list : public singleton<spectator_list>
{
private:
	static std::list<int> get_observervators();

public:
	spectator_list();
	~spectator_list();

	void render() const;
};