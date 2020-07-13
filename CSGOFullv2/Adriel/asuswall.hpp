#pragma once
#include "stdafx.hpp"
#include "../CMaterialSystem.h"

class asuswall : public singleton<asuswall>
{
private:
	class material_backup
	{
	public:
		MaterialHandle_t handle {};
		IMaterial* material {};

		float color[3] {};
		float alpha {};

		bool translucent {};
		bool nodraw {};

		material_backup();
		material_backup(MaterialHandle_t h, IMaterial* p);

		void restore();
	};

private:
	std::vector<material_backup> mat_world {};
	std::vector<material_backup> mat_sky {};

	bool b_did_sky;
	bool b_did_world;

private:
	ConVar* r_drawspecificstaticprop;
	ConVar* fog_colorskybox;
	ConVar* fog_override;

	int old_r_drawspecificstaticprop;
	const char* old_fog_colorskybox;
	int old_fog_override;

private:
	static void sky(const char* str_sky);

private:
	void do_sky(const MaterialHandle_t i, IMaterial * p_material, bool save_backup);
	void do_world(MaterialHandle_t i, IMaterial* p_material, bool save_backup);

	void apply_sky();
	void apply_world();

public:
	asuswall();
	~asuswall();

	void remove_world();
	void remove_sky();

	void game_event(CGameEvent* p_event);
	void fsn(ClientFrameStage_t stage);	
};