#include "asuswall.hpp"
#include "adr_math.hpp"

#include "../Interfaces.h"
#include "../UsedConvars.h"
#include "../NetworkedVariables.h"

asuswall::asuswall(): b_did_sky(false),
					  b_did_world(false),
					  r_drawspecificstaticprop(nullptr),
					  fog_colorskybox(nullptr),
					  fog_override(nullptr)
{
}

asuswall::~asuswall()
{
}

void asuswall::sky(const char* str_sky)
{
	StaticOffsets.GetOffsetValueByType<void(__fastcall *)(const char*)>(_SetSky)(str_sky);
}

void asuswall::do_sky( const MaterialHandle_t i, IMaterial* p_material, bool save_backup )
{
	//decrypts(0)
	if ( strstr( p_material->GetTextureGroupName(), XorStr("Sky")))
	{
		if ( save_backup )
			mat_sky.push_back( material_backup( i, p_material ) );

		auto col = variable::get().visuals.asuswall.col_sky.color();
		p_material->AlphaModulate(col.a() / 255.f);
		p_material->ColorModulate( col.r() / 255.f, col.g() / 255.f, col.b() / 255.f );
	}
	//encrypts(0)

	//decrypts(0)
	if ( strstr( p_material->GetTextureGroupName(), XorStr("StaticProp") ) && strstr( p_material->GetName(), XorStr("sky") ) )
	{
		if ( save_backup )
			mat_sky.push_back( material_backup( i, p_material ) );

		p_material->AlphaModulate( 0.f );
	}
	//encrypts(0)
}

void asuswall::apply_sky()
{
	static auto old_type = -1;
	if (old_type != variable::get().visuals.skychanger.i_type)
	{
		old_type = variable::get().visuals.skychanger.i_type;
		b_did_sky = false;
	}

	static auto old_col_sky = Color::White();
	if ( old_col_sky != variable::get().visuals.asuswall.col_sky.color() )
	{
		old_col_sky = variable::get().visuals.asuswall.col_sky.color();
		b_did_sky = false;
	}

	if (b_did_sky)
		return;

	fog_colorskybox->SetValue(1);
	fog_override->SetValue(1);

	//decrypts(0)
	static std::vector<const char*> sky_list =
	{
		XorStr("cs_baggage_skybox_"),
		XorStr("cs_tibet"),
		XorStr("embassy"),
		XorStr("italy"),
		XorStr("jungle"),
		XorStr("nukeblank"),
		XorStr("office"),
		XorStr("sky_cs15_daylight01_hdr"),
		XorStr("sky_cs15_daylight02_hdr"),
		XorStr("sky_cs15_daylight03_hdr"),
		XorStr("sky_cs15_daylight04_hdr"),
		XorStr("sky_csgo_cloudy01"),
		XorStr("sky_csgo_night02"),
		XorStr("sky_csgo_night02b"),
		XorStr("sky_dust"),
		XorStr("sky_venice"),
		XorStr("vertigo"),
		XorStr("vietnam"),
	};
	//encrypts(0)


	sky( sky_list[ variable::get().visuals.skychanger.i_type ] );

	for ( auto& mat : mat_sky )
	{
		if ( mat.material->GetReferenceCount() <= 0 )
			continue;

		mat.restore();
		//mat.material->Refresh();
	}
	mat_sky.clear();

	for ( auto i = Interfaces::MatSystem->FirstMaterial(); i != Interfaces::MatSystem->InvalidMaterial(); i = Interfaces::MatSystem->NextMaterial( i ) )
	{
		const auto p_material = Interfaces::MatSystem->GetMaterial( i );
		if ( !p_material || p_material->IsErrorMaterial() )
			continue;

		if ( p_material->GetReferenceCount() <= 0 )
			continue;

		do_sky( i, p_material, true );
	}

	b_did_sky = true;
}

void asuswall::remove_sky()
{
	if (!b_did_sky)
		return;

	for ( auto& mat : mat_sky )
	{
		if ( mat.material->GetReferenceCount() <= 0 )
			continue;

		mat.restore();
		//mat.material->Refresh();
	}
	mat_sky.clear();

	//decrypts(0)
	sky( Interfaces::Cvar->FindVar( XorStr("sv_skyname") )->GetString() );
	//encrypts(0)
	
	fog_colorskybox->SetValue(old_fog_colorskybox);
	fog_override->SetValue(old_fog_override);

	b_did_sky = false;
}

void asuswall::do_world(const MaterialHandle_t i, IMaterial* p_material, bool save_backup)
{
	//decrypts(0)
	if (strstr(p_material->GetTextureGroupName(), XorStr("World")))
	{
		if (save_backup)
			mat_world.push_back(material_backup(i, p_material));

		auto col = variable::get().visuals.asuswall.col_world.color();
		p_material->AlphaModulate(col.a() / 255.f);
		p_material->ColorModulate(col.r() / 255.f, col.g() / 255.f, col.b() / 255.f);

		//p_material->SetMaterialVarFlag(MATERIAL_VAR_TRANSLUCENT, false); //this should be tested
	}
	else if (strstr(p_material->GetTextureGroupName(), XorStr("StaticProp")))
	{
		if (save_backup)
			mat_world.push_back(material_backup(i, p_material));

		auto col = variable::get().visuals.asuswall.col_prop.color();
		p_material->AlphaModulate(col.a() / 255.f);
		p_material->ColorModulate(col.r() / 255.f, col.g() / 255.f, col.b() / 255.f);

		//p_material->SetMaterialVarFlag(MATERIAL_VAR_TRANSLUCENT, false); //this should be tested
	}
	else if (strstr(p_material->GetTextureGroupName(), XorStr("Particle")))
	{
		if (save_backup)
			mat_world.push_back(material_backup(i, p_material));

		p_material->AlphaModulate(0.f);
		p_material->ColorModulate(0.f, 0.f, 0.f);

		p_material->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, true);
	}
	//encrypts(0)
}

void asuswall::apply_world()
{
	static auto old_col_world = Color::White();
	static auto old_col_prop = Color::White();

	if (old_col_world != variable::get().visuals.asuswall.col_world.color() || old_col_prop != variable::get().visuals.asuswall.col_prop.color())
	{
		old_col_world = variable::get().visuals.asuswall.col_world.color();
		old_col_prop = variable::get().visuals.asuswall.col_prop.color();

		b_did_world = false;
	}
	
	if (b_did_world)
		return;

	r_drawspecificstaticprop->SetValue(0);

	for (auto& mat : mat_world)
	{
		if (mat.material->GetReferenceCount() <= 0)
			continue;

		mat.restore();
		//mat.material->Refresh();
	}
	mat_world.clear();

	for (auto i = Interfaces::MatSystem->FirstMaterial(); i != Interfaces::MatSystem->InvalidMaterial(); i = Interfaces::MatSystem->NextMaterial(i))
	{
		const auto p_material = Interfaces::MatSystem->GetMaterial(i);
		if (!p_material || p_material->IsErrorMaterial())
			continue;

		if (p_material->GetReferenceCount() <= 0)
			continue;

		do_world(i, p_material, true);
	}

	b_did_world = true;
}

void asuswall::remove_world()
{
	if (!b_did_world)
		return;

	for (auto& mat : mat_world)
	{
		if (mat.material->GetReferenceCount() <= 0)
			continue;

		mat.restore();
		//mat.material->Refresh();
	}
	mat_world.clear();

	r_drawspecificstaticprop->SetValue(old_r_drawspecificstaticprop);

	b_did_world = false;
}

void asuswall::game_event(CGameEvent* p_event)
{
	if (!Interfaces::EngineClient->IsInGame())
		return;

	if (variable::get().visuals.asuswall.b_enabled)
	{
		remove_world();
		apply_world();
	}

	if (variable::get().visuals.skychanger.b_enabled)
	{
		remove_sky();
		apply_sky();
	}
}

void asuswall::fsn(const ClientFrameStage_t stage)
{
	if (!r_drawspecificstaticprop)
	{
		//decrypts(0)
		r_drawspecificstaticprop = Interfaces::Cvar->FindVar(XorStr("r_drawspecificstaticprop"));
		//encrypts(0)
		r_drawspecificstaticprop->nFlags &= ~(FCVAR_HIDDEN | FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT);
		old_r_drawspecificstaticprop = r_drawspecificstaticprop->GetInt();
	}

	if (!fog_colorskybox)
	{
		//decrypts(0)
		fog_colorskybox = Interfaces::Cvar->FindVar(XorStr("fog_colorskybox"));
		//encrypts(0)
		fog_colorskybox->nFlags &= ~(FCVAR_HIDDEN | FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT);
		old_fog_colorskybox = fog_colorskybox->GetString();
	}

	if (!fog_override)
	{
		//decrypts(0)
		fog_override = Interfaces::Cvar->FindVar(XorStr("fog_override"));
		//encrypts(0)
		fog_override->nFlags &= ~(FCVAR_HIDDEN | FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT);
		old_fog_override = fog_override->GetInt();
	}

	if (stage == ClientFrameStage_t::FRAME_NET_UPDATE_POSTDATAUPDATE_START)
	{
		if (variable::get().visuals.skychanger.b_enabled)
			apply_sky();
		else
			remove_sky();

		if (variable::get().visuals.asuswall.b_enabled)
			apply_world();
		else
			remove_world();
	}
}

asuswall::material_backup::material_backup()
{
}

inline asuswall::material_backup::material_backup(MaterialHandle_t h, IMaterial * p)
{
	handle = h;
	material = p;

	material->GetColorModulation(&color[0], &color[1], &color[2]);
	alpha = material->GetAlphaModulation();

	translucent = material->GetMaterialVarFlag(MATERIAL_VAR_TRANSLUCENT);
	nodraw = material->GetMaterialVarFlag(MATERIAL_VAR_NO_DRAW);
}

inline void asuswall::material_backup::restore()
{
	material->ColorModulate(color[0], color[1], color[2]);
	material->AlphaModulate(alpha);

	material->SetMaterialVarFlag(MATERIAL_VAR_TRANSLUCENT, translucent);
	material->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, nodraw);
}
