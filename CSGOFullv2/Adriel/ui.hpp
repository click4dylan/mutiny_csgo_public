#pragma once
#include "stdafx.hpp"

class ui : public singleton<ui>
{
private:
	IDirect3DTexture9* dxt_logo {};
	bool			   b_visible;
	float			   f_scale;

	bool			   b_amd;

	enum Tabs_t: int
	{
		TAB_RAGE = 0,
		TAB_VISUALS,
		TAB_PLAYERS,
		TAB_MISC,
		TAB_WAYPOINTS,
		TAB_SKIN,
		TAB_CONFIG,
		TAB_INVALID,
	};

	// todo: nit; make this associated with a 'name', 'main bool in variable::get().visual.ui'
	struct TabControl_t
	{
		
	};

	std::vector<TabControl_t> tabs;	// so it can be in order via push_back.

	void config() const;
#ifdef INCLUDE_LEGIT
	void legit() const;
#endif

	void rage() const;

	void playerlist() const;
	void misc() const;
	void visual() const;
	void skin() const;

	void set_style() const;
	void create_fonts() const;

	void waypoints() const;

public:
	ui();
	~ui();

	void initialize(IDirect3DDevice9* p_device);
	void render();

	void show();
	void hide();
	void toggle();

	float get_scale() const;
	void update_objects(IDirect3DDevice9* p_device);

	bool is_visible() const { return b_visible; }
};