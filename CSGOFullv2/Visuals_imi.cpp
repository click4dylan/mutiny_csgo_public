#include "precompiled.h"
#include "Visuals_imi.h"
#include "AntiAim.h"
#include "utlvector.h"
#include "ICollidable.h"
#include "LocalPlayer.h"
#include "Assistance.h"
#include "CParallelProcessor.h"
#include "Adriel/renderer.hpp"
#include "Adriel/adr_util.hpp"
#include "Adriel/adr_math.hpp"
#include "Adriel/ui.hpp"
#include "Aimbot_imi.h"
#include "ServerSounds.h"
#include "HUD.h"

#include "UsedConvars.h"
#include "AutoBone_imi.h"
#include "TickbaseExploits.h"
#include "misc.h"
#include "StatsTracker.h"

extern int serverticksallowed;
extern int clientticksallowed;

CVisuals g_Visuals;
Beams g_Beams;
CThreadFastMutex DrawMutex;

std::map< std::string, TYPE > ESPTypeNames =
{
	{ XorStrCT("esp_health"), HEALTH },
	{ XorStrCT("esp_armor"), ARMOR },
	{ XorStrCT("esp_ammo"), AMMO },
	{ XorStrCT("esp_defusing"), DEFUSING },
	{ XorStrCT("esp_name"), NAME },
	{ XorStrCT("esp_zoom"), ZOOM },
	{ XorStrCT("esp_antiaim"), ANTIAIM },
	{ XorStrCT("esp_weapon"), WEAPON },
	{ XorStrCT("esp_detonation"), DETONATION },
	{ XorStrCT("esp_lbytimer"), LBYTIME },
	{ XorStrCT("esp_ping"), PING },
	{ XorStrCT("esp_bomb"), BOMBCARRIER },
	{ XorStrCT("esp_reload"), RELOAD },
	{ XorStrCT("esp_money"), MONEY },
	{ XorStrCT("esp_haskit"), KIT },
	{ XorStrCT("esp_pinpull"), PINPULL },
	{ XorStrCT("esp_burn"), BURN },
	{ XorStrCT("esp_blind"), BLIND },
	{ XorStrCT("esp_vesthelm"), VESTHELM },
	{ XorStrCT("esp_fakeduck"), FAKEDUCK },
};

class GlowObjectManager
{
public:
	int RegisterGlowObject(CBaseEntity* pEntity, const float& flGlowRed, const float& flGlowGreen, const float& flGlowBlue, float flGlowAlpha, bool bRenderWhenOccluded, bool bRenderWhenUnoccluded, int nSplitScreenSlot)
	{
		int nIndex;
		if (m_nFirstFreeSlot == GlowObjectDefinition_t::END_OF_FREE_LIST)
		{
			nIndex = m_GlowObjectDefinitions.AddToTail();
		}
		else
		{
			nIndex = m_nFirstFreeSlot;
			m_nFirstFreeSlot = m_GlowObjectDefinitions[nIndex].m_nNextFreeSlot;
		}

		m_GlowObjectDefinitions[nIndex].m_pEntity = pEntity;
		m_GlowObjectDefinitions[nIndex].m_flGlowRed = flGlowRed;
		m_GlowObjectDefinitions[nIndex].m_flGlowGreen = flGlowGreen;
		m_GlowObjectDefinitions[nIndex].m_flGlowBlue = flGlowBlue;
		m_GlowObjectDefinitions[nIndex].m_flGlowAlpha = flGlowAlpha;
		m_GlowObjectDefinitions[nIndex].flUnk = 0.0f;
		m_GlowObjectDefinitions[nIndex].m_flBloomAmount = 1.0f;
		m_GlowObjectDefinitions[nIndex].localplayeriszeropoint3 = 0.0f;
		m_GlowObjectDefinitions[nIndex].m_bRenderWhenOccluded = bRenderWhenOccluded;
		m_GlowObjectDefinitions[nIndex].m_bRenderWhenUnoccluded = bRenderWhenUnoccluded;
		m_GlowObjectDefinitions[nIndex].m_bFullBloomRender = false;
		m_GlowObjectDefinitions[nIndex].m_iFullBloomStencilTestValue = 0;
		m_GlowObjectDefinitions[nIndex].m_iSplitScreenSlot = nSplitScreenSlot;
		m_GlowObjectDefinitions[nIndex].m_nNextFreeSlot = GlowObjectDefinition_t::ENTRY_IN_USE;

		return nIndex;
	}

	void UnregisterGlowObject(int nGlowObjectHandle)
	{
		m_GlowObjectDefinitions[nGlowObjectHandle].m_nNextFreeSlot = m_nFirstFreeSlot;
		m_GlowObjectDefinitions[nGlowObjectHandle].m_pEntity = NULL;
		m_nFirstFreeSlot = nGlowObjectHandle;
	}

	int HasGlowEffect(CBaseEntity* pEntity) const
	{
		for (int i = 0; i < m_GlowObjectDefinitions.Count(); ++i)
			if (!m_GlowObjectDefinitions[i].IsUnused() && m_GlowObjectDefinitions[i].m_pEntity == pEntity)
				return i;
		return NULL;
	}

	class GlowObjectDefinition_t
	{
	public:
		void set(float r, float g, float b, float a)
		{
			m_flGlowRed = r;
			m_flGlowGreen = g;
			m_flGlowBlue = b;
			m_flGlowAlpha = a;
			m_bRenderWhenOccluded = true;
			m_bRenderWhenUnoccluded = false;
			m_flBloomAmount = 1.0f;
			m_iGlowStyle = 0;
		}

		CBaseEntity* getEnt()
		{
			return m_pEntity;
		}

		bool IsUnused() const { return m_nNextFreeSlot != GlowObjectDefinition_t::ENTRY_IN_USE; }

	public:
		CBaseEntity* m_pEntity;
		float m_flGlowRed;
		float m_flGlowGreen;
		float m_flGlowBlue;
		float m_flGlowAlpha;

		char unknown[4];

		float flUnk;
		float m_flBloomAmount;
		float localplayeriszeropoint3;

		bool m_bRenderWhenOccluded;
		bool m_bRenderWhenUnoccluded;
		bool m_bFullBloomRender;

		char unknown1[1];

		int m_iFullBloomStencilTestValue;
		int m_iGlowStyle;
		int m_iSplitScreenSlot;

		// Linked list of free slots
		int m_nNextFreeSlot;

		// Special values for GlowObjectDefinition_t::m_nNextFreeSlot
		static const int END_OF_FREE_LIST = -1;
		static const int ENTRY_IN_USE = -2;
	};

	CUtlVector< GlowObjectDefinition_t > m_GlowObjectDefinitions;
	int m_nFirstFreeSlot;
};

std::string StringFromArgs(const char* fmt, ...)
{
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsprintf(buffer, fmt, args);
	va_end(args);
	return buffer;
}

std::string GetStrippedModelname(CBaseEntity* entity, bool* isDefuser = nullptr)
{
	auto Model = entity->GetModel();

	if (!Model)
		return "";

	auto ModelName = std::string(Interfaces::ModelInfoClient->GetModelName(Model));

	if (!ModelName.size())
		return "";

	//decrypts(0)
	auto mdl = ModelName.find(XorStr(".mdl"));
	//encrypts(0)

	if (mdl != std::string::npos)
		ModelName.erase(mdl, 4);

	//decrypts(0)
	auto dropped = ModelName.find(XorStr("_dropped"));
	//encrypts(0)

	if (dropped != std::string::npos)
		ModelName.erase(dropped, 8);

	//decrypts(0)
	auto v = ModelName.find(XorStr("v_"));
	//encrypts(0)

	if (v != std::string::npos)
		ModelName.erase(0, v + 2);

	//decrypts(0)
	auto w = ModelName.find(XorStr("w_"));
	//encrypts(0)

	if (w != std::string::npos)
		ModelName.erase(0, w + 2);

	//decrypts(0)
	auto eq = ModelName.find(XorStr("eq_"));
	//encrypts(0)

	if (eq != std::string::npos)
		ModelName.erase(0, eq + 3);

	//decrypts(0)
	auto pist = ModelName.find(XorStr("pist_"));
	//encrypts(0)

	if (pist != std::string::npos)
		ModelName.erase(0, pist + 5);

	//decrypts(0)
	auto smg = ModelName.find(XorStr("smg_"));
	//encrypts(0)

	if (smg != std::string::npos)
		ModelName.erase(0, smg + 44);

	//decrypts(0)
	auto mach = ModelName.find(XorStr("mach_"));
	//encrypts(0)

	if (mach != std::string::npos)
		ModelName.erase(0, mach + 5);

	//decrypts(0)
	auto rif = ModelName.find(XorStr("rif_"));
	//encrypts(0)

	if (rif != std::string::npos)
		ModelName.erase(0, rif + 4);

	//decrypts(0)
	auto snip = ModelName.find(XorStr("snip_"));
	//encrypts(0)

	if (snip != std::string::npos)
		ModelName.erase(0, snip + 5);

	//decrypts(0)
	auto shot = ModelName.find(XorStr("shot_"));
	//encrypts(0)

	if (shot != std::string::npos)
		ModelName.erase(0, shot + 5);

	for (size_t i = 0; i < ModelName.size(); ++i)
	{
		if (ModelName[i] == '_')
			ModelName[i] = ' ';
		else
			ModelName[i] = tolower(ModelName[i]);
	}

	//decrypts(0)
	if (ModelName[0] == '2' && ModelName[1] == '2' && ModelName[2] == '3') // usp
		ModelName = XorStr("USP-S");
	if (ModelName[0] == 'd' && ModelName[2] == 'c' && ModelName[4] == 'y') // decoy
		ModelName = XorStr("Decoy");
	if (ModelName[0] == 'f' && ModelName[2] == 'a' && ModelName[4] == 'g') // fraggrenade
		ModelName = XorStr("High Explosive");
	if (ModelName[0] == 'f' && ModelName[2] == 'a' && ModelName[4] == 'h') // flashbang
		ModelName = XorStr("Flashbang");
	if (ModelName[0] == 'i' && ModelName[2] == 'c' && ModelName[4] == 'n') // incendiarygrenade
		ModelName = XorStr("Incendiary");
	if (ModelName[0] == 'm' && ModelName[2] == 'l' && ModelName[4] == 't') // molotov
		ModelName = XorStr("Molotov");
	if (ModelName[0] == 's' && ModelName[2] == 'o' && ModelName[4] == 'e') // smokegrenade
		ModelName = XorStr("Smoke");
	if (ModelName[0] == 'h' && ModelName[2] == 'a' && ModelName[4] == 't') // healthshot
		ModelName = XorStr("Healthshot");
	if (ModelName[0] == 's' && ModelName[2] == 'n' && ModelName[4] == 'o') // sensorgrenade
		ModelName = XorStr("Sensor");
	if (ModelName[0] == 'i' && ModelName[2] == 'd') // ied
		ModelName = XorStr("C4");
	if (ModelName[0] == 'd' && ModelName[2] == 'f' && ModelName[4] == 's')
	{ // defuser
		ModelName = XorStr("Defuse Kit");
		if (isDefuser)
			*isDefuser = true;
	}
	//encrypts(0)

	return ModelName;
}

CVisuals::CVisuals()
{
	Clear();

	for (auto i = 0; i < static_cast<int>(arr_alpha.size()); i++)
	{
		if (arr_alpha[i] > 0.f)
			arr_alpha[i] = 0.f;
	}
}

void CVisuals::game_event(CGameEvent* p_event)
{
	if (!Interfaces::EngineClient->IsInGame())
		return;

	for (auto i = 0; i < static_cast<int>(arr_alpha.size()); i++)
	{
		if (arr_alpha[i] > 0.f)
			arr_alpha[i] = 0.f;
	}

	m_lastBestAimbotPos.Zero();
	m_lastBestDamage = 0;
	m_prioritize_body = false;
}

void CVisuals::Clear()
{
	
}

bool CVisuals::GenerateBoundingBox(CBaseEntity* Entity, Vector& origin, QAngle& angles, bool rotate, BoundingBox& out)
{
	auto local = Interfaces::EngineClient->GetLocalPlayer();
	if (!local)
		return false;
	CBaseEntity *LocalPlayer = Interfaces::ClientEntList->GetBaseEntity(local);
	if (!Interfaces::EngineClient->IsInGame() || !LocalPlayer || LocalPlayer->GetSpawnTime() == 0.0f)
		return false;

	//Vector vMin = Entity->GetCollideable()->OBBMins(), vMax = Entity->GetCollideable()->OBBMaxs();
	CPlayerrecord *_Record = g_LagCompensation.GetPlayerrecord(Entity);
	Vector vMin = _Record ? _Record->m_vecOBBMins : Entity->GetCollideable()->OBBMins(), vMax = _Record ? _Record->m_vecOBBMaxs : Entity->GetCollideable()->OBBMaxs();
	Vector lbf, lbb, ltb, ltf, rtb, rbb, rbf, rtf;

	Vector Bounds[] = {
		rotate ? VectorRotateR(Vector(vMin.x, vMin.y, vMin.z), angles) : Vector(vMin.x, vMin.y, vMin.z), // left bottom back corner
		rotate ? VectorRotateR(Vector(vMin.x, vMax.y, vMin.z), angles) : Vector(vMin.x, vMax.y, vMin.z), // left bottom front corner
		rotate ? VectorRotateR(Vector(vMax.x, vMax.y, vMin.z), angles) : Vector(vMax.x, vMax.y, vMin.z), // left top front corner
		rotate ? VectorRotateR(Vector(vMax.x, vMin.y, vMin.z), angles) : Vector(vMax.x, vMin.y, vMin.z), // left top back corner
		rotate ? VectorRotateR(Vector(vMax.x, vMax.y, vMax.z), angles) : Vector(vMax.x, vMax.y, vMax.z), // right top front corner
		rotate ? VectorRotateR(Vector(vMin.x, vMax.y, vMax.z), angles) : Vector(vMin.x, vMax.y, vMax.z), // right bottom front corner
		rotate ? VectorRotateR(Vector(vMin.x, vMin.y, vMax.z), angles) : Vector(vMin.x, vMin.y, vMax.z), // right bottom back corner
		rotate ? VectorRotateR(Vector(vMax.x, vMin.y, vMax.z), angles) : Vector(vMax.x, vMin.y, vMax.z) // right top back corner
	};

	if (!WorldToScreen(Bounds[0] + origin, lbb) || !WorldToScreen(Bounds[1] + origin, lbf)
		|| !WorldToScreen(Bounds[2] + origin, ltf) || !WorldToScreen(Bounds[3] + origin, ltb)
		|| !WorldToScreen(Bounds[4] + origin, rtf) || !WorldToScreen(Bounds[5] + origin, rbf)
		|| !WorldToScreen(Bounds[6] + origin, rbb) || !WorldToScreen(Bounds[7] + origin, rtb))
		return false;

	Vector vecArray[] = { lbf, rtb, lbb, rtf, rbf, rbb, ltb, ltf };
	float left = ltf.x, top = ltf.y, right = ltf.x, bottom = ltf.y;

	for (int idx = 1; idx < 8; idx++)
	{
		if (left > vecArray[idx].x)
			left = vecArray[idx].x;
		if (right < vecArray[idx].x)
			right = vecArray[idx].x;
		if (bottom < vecArray[idx].y)
			bottom = vecArray[idx].y;
		if (top > vecArray[idx].y)
			top = vecArray[idx].y;
	}

	out = BoundingBox(left, top, right - left, bottom - top);
	return true;
}

bool CVisuals::GenerateBoundingBox(CBaseEntity* Entity, Vector& vMin, Vector& vMax, BoundingBox& out)
{
	//Vector vMin = Entity->GetCollideable()->OBBMins(), vMax = Entity->GetCollideable()->OBBMaxs();
	CPlayerrecord *_Record = g_LagCompensation.GetPlayerrecord(Entity);
	Vector lbf, lbb, ltb, ltf, rtb, rbb, rbf, rtf;

	Vector Bounds[] = {
		Vector(vMin.x, vMin.y, vMin.z), // left bottom back corner
		Vector(vMin.x, vMax.y, vMin.z), // left bottom front corner
		Vector(vMax.x, vMax.y, vMin.z), // left top front corner
		Vector(vMax.x, vMin.y, vMin.z), // left top back corner
		Vector(vMax.x, vMax.y, vMax.z), // right top front corner
		Vector(vMin.x, vMax.y, vMax.z), // right bottom front corner
		Vector(vMin.x, vMin.y, vMax.z), // right bottom back corner
		Vector(vMax.x, vMin.y, vMax.z) // right top back corner
	};

	if (!WorldToScreen(Bounds[0], lbb) || !WorldToScreen(Bounds[1], lbf)
		|| !WorldToScreen(Bounds[2], ltf) || !WorldToScreen(Bounds[3], ltb)
		|| !WorldToScreen(Bounds[4], rtf) || !WorldToScreen(Bounds[5], rbf)
		|| !WorldToScreen(Bounds[6], rbb) || !WorldToScreen(Bounds[7], rtb))
		return false;

	Vector vecArray[] = { lbf, rtb, lbb, rtf, rbf, rbb, ltb, ltf };
	float left = ltf.x, top = ltf.y, right = ltf.x, bottom = ltf.y;

	for (int idx = 1; idx < 8; idx++)
	{
		if (left > vecArray[idx].x)
			left = vecArray[idx].x;
		if (right < vecArray[idx].x)
			right = vecArray[idx].x;
		if (bottom < vecArray[idx].y)
			bottom = vecArray[idx].y;
		if (top > vecArray[idx].y)
			top = vecArray[idx].y;
	}

	out = BoundingBox(left, top, right - left, bottom - top);

	return true;
}

bool CVisuals::GetBox(CBaseEntity * entity, const Vector &origin, BoundingBox &box, bool is_far_esp, bool use_valve)
{
	Vector mins = {}, maxs = {};
	if (!is_far_esp)
	{
		entity->ComputeHitboxSurroundingBox(&mins, &maxs, nullptr, use_valve);

		// adjust our mins/maxs to a player's model
		mins = { origin.x, origin.y, mins.z };
		maxs = { origin.x, origin.y, maxs.z + 8.f };
	}
	else
	{
		mins = { origin.x, origin.y, entity->ToPlayerRecord()->m_vecLastFarESPPredictedMins.z };
		maxs = { origin.x, origin.y, entity->ToPlayerRecord()->m_vecLastFarESPPredictedMaxs.z + 72.0f };
	}

	// grab the bottom and top of the box
	Vector screenMins = {}, screenMaxs = {};

	// check to see if either the mins or maxs are seen on the screen
	if (!WorldToScreen(mins, screenMins) || !WorldToScreen(maxs, screenMaxs))
		return false;

	// construct our boxes
	box.h = screenMins.y - screenMaxs.y;
	box.w = box.h * 0.5f;
	box.x = screenMins.x - (box.w * 0.5f);
	box.y = screenMins.y - box.h;

	return true;
}

void CVisuals::DrawPlayerESP(CBaseEntity* Entity)
{
	CPlayerrecord* _playerRecord = g_LagCompensation.GetPlayerrecord(Entity);

	// playerrecord exists
	if (!_playerRecord || !_playerRecord->m_pEntity)
		return;

	auto _type = Entity->GetEntityType();

	// is valid
	if (Entity != _playerRecord->m_pEntity || _type != player)
		return;

	const bool enemy = Entity->IsEnemy(LocalPlayer.Entity);

	auto rec = _playerRecord->GetCurrentRecord();
	FarESPPlayer* _lastFarESPPacket = nullptr;
	bool _isUsingFarESP = false;
	if (_playerRecord->m_bDormant && !Entity->IsLocalPlayer())
	{
		if (_lastFarESPPacket = _playerRecord->GetFarESPPacket())
			_isUsingFarESP = true;
	}

	// is alive
	if (_isUsingFarESP)
	{
		if (_lastFarESPPacket->health <= 0)
			return;
	}
	else if (!Entity->GetAlive() || Entity->GetHealth() <= 0)
		return;

	// only draw ourselves in thirdperson if we have local player esp enabled
	if (Entity->IsLocalPlayer() && !Interfaces::Input->CAM_IsThirdPerson() && variable::get().visuals.pf_local_player.vf_main.b_enabled)
		return;

	// if we're not an enemy and we're not drawing teammates
	if (!Entity->IsLocalPlayer())
	{
		if (!enemy && !variable::get().visuals.pf_teammate.vf_main.b_enabled)
			return;

		// if we're an enemy and not drawing enemies
		if (enemy && !variable::get().visuals.pf_enemy.vf_main.b_enabled)
			return;
	}
	else
	{
		if (!variable::get().visuals.pf_local_player.vf_main.b_enabled)
			return;
	}

	// figure out attributes
	Color player_color = Color::White();
	int player_alpha = 255;

	bool draw_box = false;
	bool draw_name = false;
	bool draw_weapon = false;
	bool draw_ammo = false;
	bool draw_health = false;
	bool draw_armor = false;
	bool draw_money = false;
	bool draw_armorstatus = false;
	bool draw_bombcarrier = false;
	bool draw_haskit = false;
	bool draw_scoped = false;
	bool draw_pinpull = false;
	bool draw_blind = false;
	bool draw_burn = false;
	bool draw_bombint = false;
	bool entity_debug = false;
	bool draw_reload = false;
	bool draw_vuln = false;
	bool draw_resolver = false;
	bool draw_fakeduck = false;
	bool draw_aimbotdebug = false;
	float f_timeout = 0.f;

	bool draw_offscreen = false;
	float offscreen_size = 0.f;

	Color name_color{};
	Color box_color{};
	Color offscreen_color{};

	if (!Entity->IsLocalPlayer())
	{
		if (enemy)
		{
			draw_box = variable::get().visuals.pf_enemy.vf_main.b_box;
			draw_name = variable::get().visuals.pf_enemy.vf_main.b_name;
			draw_health = variable::get().visuals.pf_enemy.b_health;
			draw_armor = variable::get().visuals.pf_enemy.b_armor;
			draw_weapon = variable::get().visuals.pf_enemy.b_weapon;
			draw_ammo = variable::get().visuals.pf_enemy.vf_main.b_info;
			draw_money = variable::get().visuals.pf_enemy.b_money;
			draw_armorstatus = variable::get().visuals.pf_enemy.b_kevlar_helm;
			draw_bombcarrier = variable::get().visuals.pf_enemy.b_bomb_carrier;
			draw_haskit = variable::get().visuals.pf_enemy.b_has_kit;
			draw_scoped = variable::get().visuals.pf_enemy.b_scoped;
			draw_pinpull = variable::get().visuals.pf_enemy.b_pin_pull;
			draw_blind = variable::get().visuals.pf_enemy.b_blind;
			draw_burn = variable::get().visuals.pf_enemy.b_burn;
			draw_bombint = variable::get().visuals.pf_enemy.b_bomb_interaction;
			draw_reload = variable::get().visuals.pf_enemy.b_reload;
			draw_vuln = variable::get().visuals.pf_enemy.b_vuln;
			draw_resolver = variable::get().visuals.pf_enemy.b_resolver;
			draw_fakeduck = variable::get().visuals.pf_enemy.b_fakeduck;
			draw_aimbotdebug = variable::get().visuals.pf_enemy.b_aimbot_debug;
			draw_offscreen = variable::get().visuals.pf_enemy.b_offscreen_esp;
			f_timeout = variable::get().visuals.pf_enemy.vf_main.f_timeout;
			offscreen_size = variable::get().visuals.pf_enemy.f_offscreen_esp;

			entity_debug = variable::get().visuals.pf_enemy.b_entity_debug;
			player_color = variable::get().visuals.pf_enemy.vf_main.col_main.color();
			name_color = variable::get().visuals.pf_enemy.vf_main.col_name.color();
			box_color = variable::get().visuals.pf_enemy.vf_main.col_main.color();
			offscreen_color = variable::get().visuals.pf_enemy.col_offscreen_esp.color();
		}
		else if (!enemy)
		{
			draw_box = variable::get().visuals.pf_teammate.vf_main.b_box;
			draw_name = variable::get().visuals.pf_teammate.vf_main.b_name;
			draw_health = variable::get().visuals.pf_teammate.b_health;
			draw_armor = variable::get().visuals.pf_teammate.b_armor;
			draw_weapon = variable::get().visuals.pf_teammate.b_weapon;
			draw_ammo = variable::get().visuals.pf_teammate.vf_main.b_info;
			draw_money = variable::get().visuals.pf_teammate.b_money;
			draw_armorstatus = variable::get().visuals.pf_teammate.b_kevlar_helm;
			draw_bombcarrier = variable::get().visuals.pf_teammate.b_bomb_carrier;
			draw_haskit = variable::get().visuals.pf_teammate.b_has_kit;
			draw_scoped = variable::get().visuals.pf_teammate.b_scoped;
			draw_pinpull = variable::get().visuals.pf_teammate.b_pin_pull;
			draw_blind = variable::get().visuals.pf_teammate.b_blind;
			draw_burn = variable::get().visuals.pf_teammate.b_burn;
			draw_bombint = variable::get().visuals.pf_teammate.b_bomb_interaction;
			draw_reload = variable::get().visuals.pf_teammate.b_reload;
			draw_vuln = variable::get().visuals.pf_teammate.b_vuln;
			draw_fakeduck = variable::get().visuals.pf_teammate.b_fakeduck;
			draw_offscreen = variable::get().visuals.pf_teammate.b_offscreen_esp;
			f_timeout = variable::get().visuals.pf_teammate.vf_main.f_timeout;
			offscreen_size = variable::get().visuals.pf_teammate.f_offscreen_esp;
			draw_resolver = variable::get().visuals.pf_teammate.b_resolver;

			draw_aimbotdebug = false;

			entity_debug = variable::get().visuals.pf_teammate.b_entity_debug;
			player_color = variable::get().visuals.pf_teammate.vf_main.col_main.color();
			name_color = variable::get().visuals.pf_teammate.vf_main.col_name.color();
			box_color = variable::get().visuals.pf_teammate.vf_main.col_main.color();
			offscreen_color = variable::get().visuals.pf_teammate.col_offscreen_esp.color();
		}
	}
	else
	{
		draw_box = variable::get().visuals.pf_local_player.vf_main.b_box;
		draw_name = variable::get().visuals.pf_local_player.vf_main.b_name;
		draw_health = variable::get().visuals.pf_local_player.b_health;
		draw_armor = variable::get().visuals.pf_local_player.b_armor;
		draw_weapon = variable::get().visuals.pf_local_player.b_weapon;
		draw_ammo = variable::get().visuals.pf_local_player.vf_main.b_info;
		draw_money = variable::get().visuals.pf_local_player.b_money;
		draw_armorstatus = variable::get().visuals.pf_local_player.b_kevlar_helm;
		draw_bombcarrier = variable::get().visuals.pf_local_player.b_bomb_carrier;
		draw_haskit = variable::get().visuals.pf_local_player.b_has_kit;
		draw_scoped = variable::get().visuals.pf_local_player.b_scoped;
		draw_pinpull = variable::get().visuals.pf_local_player.b_pin_pull;
		draw_blind = variable::get().visuals.pf_local_player.b_blind;
		draw_burn = variable::get().visuals.pf_local_player.b_burn;
		draw_bombint = variable::get().visuals.pf_local_player.b_bomb_interaction;
		draw_reload = variable::get().visuals.pf_local_player.b_reload;
		draw_vuln = variable::get().visuals.pf_local_player.b_vuln;
		draw_fakeduck = variable::get().visuals.pf_local_player.b_fakeduck;
		draw_offscreen = false;
		offscreen_size = 0.f;

		draw_resolver = false;
		draw_aimbotdebug = false;

		entity_debug = variable::get().visuals.pf_local_player.b_entity_debug;
		player_color = variable::get().visuals.pf_local_player.vf_main.col_main.color();
		name_color = variable::get().visuals.pf_local_player.vf_main.col_name.color();
		box_color = variable::get().visuals.pf_local_player.vf_main.col_main.color();
		offscreen_color = Color(0, 0, 0, 0);
	}

	// this cant be const ...
	auto entindex = Entity->index; //EntIndex() ? from IClientNetworkable

									   // adjust dormancy states
	if (entindex <= MAX_PLAYERS)
	{
		if (!Entity->GetDormant())
		{
			if (m_customESP[entindex].m_bIsDormant)
				m_customESP[entindex].m_bWasDormant = true;

			m_customESP[entindex].m_bIsDormant = false;
		}
	}

	auto is_dormant = Entity->GetDormant();
	auto frequency = 1.25f * adr_math::my_super_clamp(f_timeout, 1.0f, 5.0f);
	auto step = frequency * Interfaces::Globals->absoluteframetime;

	//std::string victim_name = adr_util::sanitize_name(_playerRecord->m_PlayerInfo.name);
	//if (strstr(victim_name.c_str(), "Kace"))
	{
		//int test = 1;
	}

	if (!_isUsingFarESP)
	{
		auto& alpha = arr_alpha[entindex];
		if (!is_dormant)
		{
			if (alpha < 1.f)
				alpha += step * 6.5f;
		}
		else
		{
			if (alpha > 0.f)
				arr_alpha[entindex] -= step;
		}

		arr_alpha[entindex] = alpha > 1.f ? 1.f : alpha < 0.f ? 0.f : alpha;
		if (alpha <= 0.f)
			return;
	}

	player_alpha = _isUsingFarESP ? 255 : (int)(arr_alpha[entindex] * 255);
	player_color = Color(125, 125, 125, player_alpha);

	// grab basic information synced with far-esp packets
	Vector origin;
	QAngle angles;
	int health;
	int armor;
	uint8_t alive;

	if (!_isUsingFarESP)
	{
		origin = Entity->GetAbsOriginDirect();
		angles = Entity->GetAngleRotation();
		health = Entity->GetHealth();
		armor = Entity->GetArmor();
		alive = Entity->GetAlive();
	}
	else
	{
		origin = _playerRecord->m_vecLastFarESPPredictedPosition;
		angles = { 0.0f, _playerRecord->m_vecLastFarESPPredictedAbsAngles.y, 0.0f };
		health = _lastFarESPPacket->health;
		armor = _lastFarESPPacket->armor;
		alive = _lastFarESPPacket->health > 0;
	}

	m_customESP[entindex].m_bIsOffscreen = false;

	//Vector mins, maxs;
	//Entity->ComputeHitboxSurroundingBox(&mins, &maxs);

	BoundingBox BBox;

	// todo: nit; uncomment this when we save the local player's last team so when spectating, we don't consider everyone 'enemies'.
	//CBaseEntity* observer_target = LocalPlayer.Entity->GetObserverTarget();
	// don't show the offscreen arrow on the top of the screen if spectating a target
	//if (!LocalPlayer.IsAlive && observer_target && observer_target->entindex() != entindex)
	//	draw_offscreen = false;

	if (!GetBox(Entity, origin, BBox, _isUsingFarESP, true))
	{
		m_customESP[entindex].m_bIsOffscreen = true;

		if (draw_offscreen)
		{
			OffscreenESP(Entity, is_dormant ? player_color : offscreen_color, offscreen_size);
		}

		return;
	}

	//Vector screen_mins, screen_maxs;
	//if (!WorldToScreen(mins, screen_mins) || !WorldToScreen(maxs, screen_maxs))
	//{
	//	// we're offscreen so handle offscreen esp
	//	m_customESP[entindex].m_bIsOffscreen = true;
	//
	//	if (variable::get().visuals.b_offscreen_esp)
	//	{
	//		OffscreenESP(player_color, Entity);
	//	}
	//
	//	return;
	//}

	auto weapon = Entity->GetWeapon();
	auto animlayer = Entity->GetAnimOverlay(AIMSEQUENCE_LAYER1);
	auto cur_record = _playerRecord->GetCurrentRecord();

	// draw box
	if (draw_box)
	{
		Color color = Color(box_color.r(), box_color.g(), box_color.b(), player_alpha);
		render::get().add_custom_box(1, false, RECT{ (long)BBox.x, (long)BBox.y, (long)(BBox.w + BBox.x), (long)(BBox.h + BBox.y) }, nullptr, color, Color::Black(), Color(0, 0, 0, player_alpha));
	}

	if (draw_name)
	{
		auto& playerInfo = _playerRecord->m_PlayerInfo;

		// sanitize name
		std::string name = adr_util::sanitize_name(playerInfo.name);

		ImColor color = is_dormant ? player_color.ToImGUI() : ImColor(name_color.r(), name_color.g(), name_color.b(), player_alpha);
		if (!Entity->IsBot())
		{
			//decrypts(0)
			if (entity_debug)
				render::get().add_text(ImVec2(BBox.x + BBox.w * 0.5f, BBox.y - 9.f), color, CENTERED_X | CENTERED_Y | OUTLINE, SMALLEST_PIXEL_10, XorStr("%s [%d]"), name.data(), entindex);
			else
				render::get().add_text(ImVec2(BBox.x + BBox.w * 0.5f, BBox.y - 9.f), color, CENTERED_X | CENTERED_Y | OUTLINE, SMALLEST_PIXEL_10, XorStr("%s"), name.data());
			//encrypts(0)
		}
		else
		{
			//decrypts(0)
			if (entity_debug)
				render::get().add_text(ImVec2(BBox.x + BBox.w * 0.5f, BBox.y - 9.f), color, CENTERED_X | CENTERED_Y, SMALLEST_PIXEL_10, XorStr("[BOT] %s [%d]"), name.data(), entindex);
			else
				render::get().add_text(ImVec2(BBox.x + BBox.w * 0.5f, BBox.y - 9.f), color, CENTERED_X | CENTERED_Y, SMALLEST_PIXEL_10, XorStr("[BOT] %s"), name.data());
			//encrypts(0)
		}

		if (entity_debug && !is_dormant)
		{
			//decrypts(0)
			render::get().add_text(ImVec2(BBox.x + BBox.w * 0.5f, BBox.y - 20.f), Color::White().ToImGUI(), CENTERED_X | CENTERED_Y | DROP_SHADOW, BAKSHEESH_12, XorStr("[ %.1f %.1f %.1f ]"), origin.x, origin.y, origin.z);
			//encrypts(0)
		}
	}

	if (draw_weapon)
	{
		ImColor color = is_dormant ? player_color.ToImGUI() : ImColor(255, 255, 255, player_alpha);
		if (weapon)
		{
			// draw the name of the weapon
			//decrypts(0)
			render::get().add_text(ImVec2(BBox.x + BBox.w * 0.5f, BBox.y + BBox.h + 15.f + (draw_armor ? 8.f : 0.f)), color, CENTERED_X | CENTERED_Y | DROP_SHADOW, BAKSHEESH_12, XorStr("%s"), weapon->GetWeaponName().data());
			//encrypts(0)
		}
	}

	if (draw_ammo)
	{
		ImColor color = is_dormant ? player_color.ToImGUI() : ImColor(255, 255, 255, player_alpha);
		if (weapon)
		{
			// draw the ammo information [ current / max ] if we actually have ammo - this is to prevent drawing (-1/0) on knives/c4 etc
			if (!weapon->IsKnife() && !weapon->IsGrenade() && !weapon->IsUtility())
			{
				//decrypts(0)
				render::get().add_text(ImVec2(BBox.x + BBox.w * 0.5f, BBox.y + BBox.h + 25.f + (draw_armor ? 8.f : 0.f)), color, CENTERED_X | CENTERED_Y | DROP_SHADOW, BAKSHEESH_12, XorStr("%d / %d"), weapon->GetClipOne(), weapon->GetPrimaryReserveAmmoCount());
				//encrypts(0)
			}

			// i would sujest "weapon / ammo" and not:
			//		weapon
			//  ammo / max ammo
			// cuz literally max ammo is kinda useless and just more junk at screen
		}
	}

	if (draw_health)
	{
		health = min(100, health);
		Color health_color = adr_util::get_health_color(health);
		health_color[3] = player_alpha;

		health_color = is_dormant ? player_color : Color(health_color.r(), health_color.g(), health_color.b(), player_alpha);
		int bar_height = (int)std::round(health * BBox.h / 100.f);

		// ABS ONE
		//auto i_dim = 2;

		/*
		render::get().add_rect(ImVec2(ctx.bbox.left - i_dim - 4, ctx.bbox.top), ImVec2(ctx.bbox.left - i_dim, ctx.bbox.bottom), color_black.ToImGUI());
		render::get().add_rect_filled(ImVec2(ctx.bbox.left - i_dim - 3, ctx.bbox.top + 1), ImVec2(ctx.bbox.left - i_dim - 1, ctx.bbox.bottom - 1), color_out.ToImGUI());
		render::get().add_rect_filled(ImVec2(ctx.bbox.left - i_dim - 3, ctx.bbox.top + health + 1), ImVec2(ctx.bbox.left - i_dim - 1, ctx.bbox.bottom - 1), color_health.ToImGUI());
		*/

		//i_dim += 6;

		render::get().add_rect_filled(ImVec2(BBox.x - 7.f, BBox.y + BBox.h - bar_height + 1.f), 5.f, bar_height - 1.f, ImColor(0, 0, 0, player_alpha));
		render::get().add_rect_filled(ImVec2(BBox.x - 6.f, BBox.y + BBox.h - bar_height + 2.f), 3.f, bar_height - 3.f, health_color.ToImGUI());

		// p checks for cleaner screen
		//decrypts(0)
		if (health < 100)
			render::get().add_text(ImVec2(BBox.x - 8.f, BBox.y + BBox.h - bar_height - 1.f), health_color.ToImGUI(), RIGHT_ALIGN | DROP_SHADOW, BAKSHEESH_12, XorStr("%d"), health);
		//encrypts(0)
	}

	if (draw_armor)
	{
		armor = min(100, armor);
		int bar_width = (int)std::round(armor * (BBox.w + 7.f) / 100);

		Color color = is_dormant ? player_color : Color(0, 140, 255, player_alpha);

		render::get().add_rect_filled(ImVec2(BBox.x - 7.f, BBox.y + BBox.h + 2.f), bar_width, 5.f, ImColor(0, 0, 0, player_alpha));
		render::get().add_rect_filled(ImVec2(BBox.x - 6.f, BBox.y + BBox.h + 3.f), bar_width - 2.f, 3.f, color.ToImGUI());

		// p checks for cleaner screen
		//decrypts(0)
		if (armor < 100)
			render::get().add_text(ImVec2(BBox.x + bar_width - 2.f, BBox.y + BBox.h + 8.f), color.ToImGUI(), RIGHT_ALIGN | CENTERED_X | DROP_SHADOW, BAKSHEESH_12, XorStr("%d"), armor);
		//encrypts(0)
	}

	// add all the side-items
	std::vector< SideItem_t > side_items;

	if (draw_money)
	{
		//decrypts(0)
		std::string money = XorStr("$") + std::to_string(Entity->GetMoney());
		//encrypts(0)
		side_items.emplace_back(money, is_dormant ? player_color.ToImGUI() : ImColor(0, 255, 0, player_alpha));
	}

	if (draw_armorstatus) // we could use the p csgo icons /shrug ;-;
	{
		if (Entity->GetArmor() > 0 && Entity->HasHelmet())
		{
			//decrypts(0)
			side_items.emplace_back(XorStr("H+K"), is_dormant ? player_color.ToImGUI() : ImColor(255, 255, 255, player_alpha));
			//encrypts(0)
		}
		else if (Entity->HasHelmet())
		{
			side_items.emplace_back("H", is_dormant ? player_color.ToImGUI() : ImColor(255, 255, 255, player_alpha));
		}
		else
		{
			side_items.emplace_back("K", is_dormant ? player_color.ToImGUI() : ImColor(255, 255, 255, player_alpha));
		}
	}

	if (draw_bombcarrier)
	{
		if (Entity->HasC4())
		{
			//decrypts(0)
			side_items.emplace_back(XorStr("BOMB"), is_dormant ? player_color.ToImGUI() : ImColor(255, 165, 0, player_alpha));
			//encrypts(0)
		}
	}

	if (draw_haskit)
	{
		if (Entity->HasDefuseKit())
		{
			//decrypts(0)
			side_items.emplace_back(XorStr("KIT"), is_dormant ? player_color.ToImGUI() : ImColor(255, 165, 0, player_alpha));
			//encrypts(0)
		}
	}

	if (draw_scoped)
	{
		if (Entity->IsScoped())
		{
			//decrypts(0)
			side_items.emplace_back(XorStr("SCOPE"), is_dormant ? player_color.ToImGUI() : ImColor(0, 255, 255, player_alpha));
			//encrypts(0)
		}
	}

	if (draw_pinpull)
	{
		if (weapon && weapon->IsGrenade() && weapon->IsPinPulled())
		{
			//decrypts(0)
			side_items.emplace_back(XorStr("PIN PULL"), is_dormant ? player_color.ToImGUI() : ImColor(0, 255, 255, player_alpha));
			//encrypts(0)
		}
	}

	if (draw_blind)
	{
		if (Entity->FlashbangTime() > 0.001f)
		{
			//decrypts(0)
			side_items.emplace_back(XorStr("BLIND"), is_dormant ? player_color.ToImGUI() : ImColor(0, 255, 255, player_alpha));
			//encrypts(0)
		}
	}

	if (draw_burn)
	{
		if (Interfaces::Globals->curtime - Entity->m_fMolotovDamageTime() <= 0.2f)
		{
			//decrypts(0)
			side_items.emplace_back(XorStr("BURN"), is_dormant ? player_color.ToImGUI() : ImColor(255, 165, 0, player_alpha));
			//encrypts(0)
		}
	}

	if (draw_bombint)
	{
		if (Entity->IsDefusing())
		{
			//decrypts(0)
			side_items.emplace_back(XorStr("DEFUSE"), is_dormant ? player_color.ToImGUI() : ImColor(191, 85, 236, player_alpha));
			//encrypts(0)
		}
		else if (weapon && weapon->IsBomb() && weapon->StartedArming())
		{
			//decrypts(0)
			side_items.emplace_back(XorStr("PLANT"), is_dormant ? player_color.ToImGUI() : ImColor(191, 85, 236, player_alpha));
			//encrypts(0)
		}
	}

	if (draw_reload)
	{
		bool draw = false;
		if (weapon && weapon->IsReloading())
		{
			draw = true;
		}
		else
		{
			if (animlayer && animlayer->m_flWeight != 0.f && GetSequenceActivity(Entity, animlayer->_m_nSequence) == ACT_CSGO_RELOAD)
			{
				draw = true;
			}
		}

		if (draw)
		{
			//decrypts(0)
			side_items.emplace_back(XorStr("RELOAD"), is_dormant ? player_color.ToImGUI() : ImColor(0, 255, 255, player_alpha));
			//encrypts(0)
		}
	}

	if (draw_vuln)
	{
		if (animlayer && weapon)
		{
			if (weapon->IsReloading() || weapon->IsKnife() || weapon->IsGrenade() || weapon->IsBomb() || (animlayer->m_flWeight != 0.f && GetSequenceActivity(Entity, animlayer->_m_nSequence) == ACT_CSGO_RELOAD))
			{
				//decrypts(0)
				side_items.emplace_back(XorStr("VULN"), is_dormant ? player_color.ToImGUI() : ImColor(255, 0, 0, player_alpha));
				//encrypts(0)
			}
		}
	}

#if defined _DEBUG || defined INTERNAL_DEBUG
	if (draw_resolver)
#else
	if (draw_resolver && LocalPlayer.Entity->GetAlive())
#endif
	{
		// get the resolve type/info
		ImColor color;
		bool lbybroke = false;
		bool resolved = false;
		std::string info = GetResolveType(_playerRecord, cur_record, &lbybroke, &color);
		color.Value.w = player_alpha;

		if (!variable::get().visuals.b_verbose_resolver)
		{
			side_items.emplace_back("R", is_dormant ? player_color.ToImGUI() : color);
		}
		else
		{
			// draw resolve mode
			if (!info.empty())
			{
				side_items.emplace_back(info, is_dormant ? player_color.ToImGUI() : color);
			}

			// draw number of valid records
#if defined _DEBUG || defined INTERNAL_DEBUG
			side_items.emplace_back(std::to_string(_playerRecord->m_ValidRecordCount) + " VR", is_dormant ? player_color.ToImGUI() : ImColor(135, 206, 235));
#endif

			// draw number of ticks they're choking
			//decrypts(0)
			side_items.emplace_back(std::to_string(_playerRecord->m_iTicksChoked) + XorStr(" ticks"), is_dormant ? player_color.ToImGUI() : Color::White().ToImGUI());

#if defined _DEBUG || defined INTERNAL_DEBUG || defined TEST_BUILD
			// draw last 5 ticks choked backwards (last - last-5)
			std::string tick_history = {};
			if (_playerRecord->m_iTicksChokedHistory.size() > 5)
			{
				for (size_t i = _playerRecord->m_iTicksChokedHistory.size() - 1; i >= _playerRecord->m_iTicksChokedHistory.size() - 5; --i)
				{
					tick_history += std::to_string(_playerRecord->m_iTicksChokedHistory[i]); // test
					if (i != _playerRecord->m_iTicksChokedHistory.size() - 5)
						tick_history += "-";
				}

				side_items.emplace_back(tick_history, is_dormant ? player_color.ToImGUI() : Color::White().ToImGUI());
			}
#endif
			//encrypts(0)

			if (_playerRecord->m_bTickbaseShiftedBackwards)
			{
				//decrypts(0)
				side_items.emplace_back(XorStr("BWD TICKBASE"), is_dormant ? player_color.ToImGUI() : ImColor(255, 0, 0, player_alpha));
				//encrypts(0)
			}
			else if (_playerRecord->m_bTickbaseShiftedForwards)
			{
				//decrypts(0)
				side_items.emplace_back(XorStr("FWD TICKBASE"), is_dormant ? player_color.ToImGUI() : ImColor(255, 0, 0, player_alpha));
				//encrypts(0)
			}

			if (_playerRecord->m_flTimeSinceStartedPressingUseKey)
			{
				//decrypts(0)
				side_items.emplace_back(XorStr("E SPAMMING"), is_dormant ? player_color.ToImGUI() : ImColor(255, 0, 0, player_alpha));
				//encrypts(0)
			}

			// draw if lby broke
			if (lbybroke)
			{
				//decrypts(0)
				side_items.emplace_back(XorStr("LBY BROKEN"), is_dormant ? player_color.ToImGUI() : ImColor(255, 165, 0, player_alpha));
				//encrypts(0)
			}

			// draw shots missed | balance adjust misses | body hit resolver misses
			std::string str_missed{};
			if (_playerRecord->m_iShotsMissed > 0)
			{
				str_missed += std::to_string(_playerRecord->m_iShotsMissed);
			}
#if defined _DEBUG || defined INTERNAL_DEBUG
			if (_playerRecord->m_iShotsMissed_BalanceAdjust > 0)
			{
				str_missed += " | BA ";
				str_missed += std::to_string(_playerRecord->m_iShotsMissed_BalanceAdjust);
			}
#endif
			// draw body hit shots missed
			auto BodyResolveInfo = _playerRecord->GetBodyHitResolveInfo(cur_record);
			int misses = 0;
			if (BodyResolveInfo && (misses = BodyResolveInfo->m_iShotsMissed, misses > 0))
			{
				str_missed += " | BH ";
				str_missed += std::to_string(misses);
			}

			if (!str_missed.empty())
			{
				//decrypts(0)
				str_missed += XorStr(" MISSES");
				//encrypts(0)

				side_items.emplace_back(str_missed.data(), is_dormant ? player_color.ToImGUI() : ImColor(255, 0, 0, player_alpha));
			}
		}
	}

	if (draw_fakeduck)
	{
#if defined _DEBUG || defined INTERNAL_DEBUG
		render::get().add_text(ImVec2(BBox.x + BBox.w * 0.5f, BBox.y - 45.f), Color::White().ToImGUI(), CENTERED_X | CENTERED_Y | DROP_SHADOW, BAKSHEESH_12, "sv %d | cl %d", serverticksallowed, clientticksallowed);
#endif
		// if their duck amount is the same and choking max ticks
		if (_playerRecord->m_bIsFakeDucking)
		{
			//decrypts(0)
			side_items.emplace_back(XorStr("FAKEDUCK"), is_dormant ? player_color.ToImGUI() : ImColor(255, 0, 0, player_alpha));
			//encrypts(0)
		}
	}

	if (draw_aimbotdebug && LocalPlayer.Entity->GetAlive())
	{
#if defined _DEBUG || defined INTERNAL_DEBUG || defined TEST_BUILD
		// draw the best tick record weight
#if defined _DEBUG || defined INTERNAL_DEBUG
		if (_playerRecord->m_ValidRecordCount == 0 || _playerRecord->m_bTeleporting)
		{
			//decrypts(0)
			render::get().add_text(ImVec2(BBox.x + BBox.w * 0.5f, BBox.y - 30.f), Color::Red().ToImGUI(), CENTERED_X | CENTERED_Y | DROP_SHADOW, BAKSHEESH_12, XorStr("NO RECORD"));
			//encrypts(0)
		}
#endif
#endif

		// draw the baim reason/head reason from autobone
		//decrypts(0)
		render::get().add_text(ImVec2(BBox.x + BBox.w * 0.5f, BBox.y - 20.f), _playerRecord->m_iBaimReason <= BAIM_REASON_NONE ? ImColor(0, 255, 0, player_alpha) : ImColor(255, 255, 0, player_alpha), CENTERED_X | CENTERED_Y | DROP_SHADOW, BAKSHEESH_12, XorStr("%s"), _playerRecord->GetBaimReasonString().data());
		//encrypts(0)

		if (Entity->index == g_Ragebot.GetTarget())
		{
			//decrypts(0)
			side_items.emplace_back(XorStr("TARGET"), is_dormant ? player_color.ToImGUI() : ImColor(255, 0, 0, player_alpha));

			// draw if we're not firing due to strict hitboxes
#if 0
#if defined _DEBUG || defined INTERNAL_DEBUG
			if (variable::get().ragebot.b_strict_hitboxes && draw_strict && strict_hitgroup_one != -1 && strict_hitgroup_two != -1)
			{
				std::string strict = XorStr("STRICT [ "); strict += std::to_string(strict_hitgroup_one); strict += XorStr(" -> "); strict += std::to_string(strict_hitgroup_two); strict += " ]";
				side_items.emplace_back(strict.data(), is_dormant ? player_color.ToImGUI() : ImColor(255, 0, 0, player_alpha));
			}
#endif
#endif
			// draw if we're ignoring limbs
			if (variable::get().ragebot.b_ignore_limbs_if_moving && Entity->GetVelocity().Length() > variable::get().ragebot.f_ignore_limbs_if_moving)
			{
				side_items.emplace_back(XorStr("NO LIMBS"), is_dormant ? player_color.ToImGUI() : ImColor(255, 0, 0, player_alpha));
			}

#if defined _DEBUG || defined INTERNAL_DEBUG
			// draw hitchance/mindmg assistant
			//if (g_AutoBone.m_pBestHitbox->hitchance != 100.f && g_AutoBone.m_pBestHitbox->hitchance < variable::get().ragebot.f_hitchance)
			//{
			//	render::get().add_text(ImVec2(BBox.x + BBox.w * 0.5f, BBox.y - 26.f), Color::Cyan().ToImGUI(), CENTERED_X | CENTERED_Y | DROP_SHADOW, BAKSHEESH_12, XorStr("%2.1f HC < %2.1f MIN HC"), g_AutoBone.m_pBestHitbox->hitchance, variable::get().ragebot.f_hitchance);
			//}
			//
			//if (g_AutoBone.m_pBestHitbox->penetrated)
			//{
			//	if (g_AutoBone.m_pBestHitbox->damage > 0 && g_AutoBone.m_pBestHitbox->damage < variable::get().ragebot.i_mindmg_aw)
			//	{
			//		render::get().add_text(ImVec2(BBox.x + BBox.w * 0.5f, BBox.y - 32.f), Color::Red().ToImGUI(), CENTERED_X | CENTERED_Y | DROP_SHADOW, BAKSHEESH_12, XorStr("%2.1f DMG < %d AW MINDMG"), g_AutoBone.m_pBestHitbox->damage, variable::get().ragebot.i_mindmg_aw);
			//	}
			//}
			//else
			//{
			//	if (g_AutoBone.m_pBestHitbox->damage > 0 && g_AutoBone.m_pBestHitbox->damage < variable::get().ragebot.i_mindmg)
			//	{
			//		render::get().add_text(ImVec2(BBox.x + BBox.w * 0.5f, BBox.y - 32.f), Color::Red().ToImGUI(), CENTERED_X | CENTERED_Y | DROP_SHADOW, BAKSHEESH_12, XorStr("%2.1f DMG < %d MINDMG"), g_AutoBone.m_pBestHitbox->damage, variable::get().ragebot.i_mindmg);
			//	}
			//}
#endif

			// draw if we're prefering body/head on the target (skeet memes but also good debug)
			//render::get().add_text(ImVec2(BBox.x + BBox.w * 0.5f, BBox.y - 17.f), m_prioritize_body ? ImColor(255, 255, 0, player_alpha) : ImColor(0, 255, 0, player_alpha), CENTERED_X | CENTERED_Y | DROP_SHADOW, BAKSHEESH_12, m_prioritize_body ? XorStr("PREFER BODY") : XorStr("HEAD"));

			Vector hitpos;
			if (WorldToScreen(m_lastBestAimbotPos, hitpos) && m_lastBestDamage > 0)
			{
				auto dmg_color = adr_util::get_health_color(m_lastBestDamage);
				render::get().add_rect_filled(ImVec2(hitpos.x, hitpos.y), 2.f, 2.f, Color::Red().ToImGUI());
				render::get().add_text(ImVec2(hitpos.x, hitpos.y), Color::White().ToImGUI(), CENTERED_X | CENTERED_Y | DROP_SHADOW, BAKSHEESH_8, XorStr("HITPOS"));
				render::get().add_text(ImVec2(hitpos.x, hitpos.y + 10.f), m_lastBestDamage >= 100 ? Color::Green().ToImGUI() : dmg_color.ToImGUI(), CENTERED_X | CENTERED_Y | DROP_SHADOW, BAKSHEESH_8, XorStr("%d dmg"), m_lastBestDamage);
			}
			//encrypts(0)
		}

		// nit; todo: check if we can fire,
	}

	// nit; todo: mindmg and hitchance assistant

	// draw all the side items
	for (auto i = 0; i < side_items.size(); i++)
	{
		auto item = side_items[i];
		render::get().add_text(ImVec2(BBox.x + BBox.w + 2.f, BBox.y + (i * 11.f)), item.color, DROP_SHADOW, BAKSHEESH_12, item.info.data());
	}


	/*
	// get correct ESP build
	ESPBuild_s vecESP;
	vecESP.m_bEnabled = false;
	vecESP = *m_vecESP[PLAYER];

	// we have a valid entity + ESPBuild
	if (vecESP.m_bEnabled)
	{
	// init locals
	Color color		= Color::White();
	int playerAlpha = 255;

	bool visible = _playerRecord->m_bIsVisible;
	bool legit   = visible && !g_Assistance.LineGoesThroughSmoke(LocalPlayer.ShootPosition, Entity->GetEyePosition()); //ToDo: Spotted

	// don't draw unlegit enemies when using legit ESP
	if (g_Convars.ESP.esp_legit->GetBool() && !legit)
	return;

	// visibilitycheck and coloradjust
	if (visible && g_Convars.ESP.esp_visibility_check->GetBool())
	{
	if (!g_Convars.Colors.color_style->GetBool())
	color = Entity->GetTeam() != LocalPlayer.Entity->GetTeam() ? g_Convars.Colors.color_visible_enemy->GetColor() : g_Convars.Colors.color_visible_team->GetColor();
	else
	color = Entity->GetTeam() == 2 ? g_Convars.Colors.color_visible_enemy->GetColor() : g_Convars.Colors.color_visible_team->GetColor();
	}
	else
	{
	if (!g_Convars.Colors.color_style->GetBool())
	color = Entity->GetTeam() != LocalPlayer.Entity->GetTeam() ? g_Convars.Colors.color_invisible_enemy->GetColor() : g_Convars.Colors.color_invisible_team->GetColor();
	else
	color = Entity->GetTeam() == 2 ? g_Convars.Colors.color_invisible_enemy->GetColor() : g_Convars.Colors.color_invisible_team->GetColor();
	}

	// adjust alpha for dormant players
	if (m_customESP[Entity->index].m_bIsDormant && !_isUsingFarESP)
	color[3] = playerAlpha = 150;

	const int entindex = Entity->index;

	// actual plsyer
	if (entindex < 65)
	{
	if (!Entity->GetDormant())
	{
	if (m_customESP[entindex].m_bIsDormant)
	m_customESP[entindex].m_bWasDormant = true;

	m_customESP[entindex].m_bIsDormant = false;
	}
	else if (!_isUsingFarESP && (!m_customESP[entindex].m_bIsDormant || !g_Convars.ESP.esp_dormant->GetBool()))
	return;
	}

	Vector origin;
	QAngle angles;
	int health;
	int armor;
	uint8_t alive;
	if (!_isUsingFarESP)
	{
	origin = Entity->GetAbsOriginDirect();
	angles = Entity->GetAngleRotation();
	health = Entity->GetHealth();
	armor  = Entity->GetArmor();
	alive  = Entity->GetAlive();
	}
	else
	{
	origin = _lastFarESPPacket->absorigin;
	angles = _lastFarESPPacket->absangles;
	health = _lastFarESPPacket->health;
	armor  = _lastFarESPPacket->armor;
	alive  = _lastFarESPPacket->bAlive;
	}

	BoundingBox BBox;
	if (!GenerateBoundingBox(Entity, origin, angles, false, BBox))
	{
	m_customESP[entindex].m_bIsOffscreen = true;

	if (g_Convars.Visuals.hvh_esparrows->GetBool())
	{
	if (_isUsingFarESP || m_customESP[entindex].m_bIsDormant)
	color.SetColor(30, 30, 30, 150);

	OffscreenESP(color, Entity);
	}

	return;
	}

	m_customESP[entindex].m_bIsOffscreen = false;

	// static box calc
	int height = 70;
	if (Entity->GetFlags() & IN_DUCK)
	height = 58;

	float x = BBox.x, y = BBox.y;
	float w = BBox.w, h = BBox.h;
	Vector _bot, _top;

	// static box apply
	if (WorldToScreenCapped(origin, _bot) && WorldToScreenCapped(origin + Vector(0, 0, height), _top) && vecESP.m_iBoxstyle < 2)
	{
	h = _bot.y - _top.y;
	w = h / 2.f;
	x = _top.x - w / 2;
	y = _top.y;
	}

	if (vecESP.m_iBoxstyle)
	g_Draw.ESPBox(x, y, w, h, 1, vecESP.m_iCornerWidth, vecESP.m_iCornerHeight, color);

	CBaseCombatWeapon* Weapon = Entity->GetWeapon();

	// loop through all sides
	for (int side = LEFT; side <= BOTTOM; side++)
	{
	// init loop locals
	ESPSide_s Side		 = vecESP.m_Sides[side];
	int _tempBarDist = 0, _tempTextDist = 0;

	// loop through all sides elements
	for (size_t i = 0; i < Side.m_Elements.size(); i++)
	{
	// init loop locals
	Element_s _pItem = *Side.m_Elements[i];
	Element_s *_Item = &_pItem;
	sDraw _Draw;
	_Draw.m_clr = Color::White();

	// set _Draw according to element type
	switch ((int)_Item->m_Type)
	{
	case HEALTH:
	{
	_Draw.Set(StringFromArgs("%i HP", health), "", health, 100.f);
	break;
	}
	case ARMOR:
	{
	_Draw.Set(StringFromArgs("%i Armor", armor), "", armor, 100.f, _Item->m_Flags>TEXT ? Color(0, 192, 255, 255) : Color::White());
	break;
	}
	case AMMO:
	{
	if (Weapon)
	{
	// prepare string
	if (Weapon->GetCSWpnData()->iMaxClip1 > -1)
	_Draw.m_szText = StringFromArgs("%i / %i", Weapon->GetClipOne(), Weapon->GetCSWpnData()->iMaxClip1);

	// it is a bar
	if (_Item->m_Flags > TEXT)
	_Draw.Set("", "", Weapon->GetClipOne(), Weapon->GetCSWpnData()->iMaxClip1, Color(255, 100, 0, playerAlpha));
	}

	break;
	}
	case NAME:
	{
	player_info_t playerInfo;

	if (Interfaces::EngineClient->GetPlayerInfo(Entity->entindex(), &playerInfo))
	_Draw.m_szText = playerInfo.name;

	break;
	}
	case WEAPON:
	{
	if (Weapon)
	{
	WeaponInfo_t* pWeaponInfo = Weapon->GetCSWpnData();
	if (pWeaponInfo)
	{
	if (pWeaponInfo->szHudName[0]=='#' && pWeaponInfo->szHudName[12]=='_')
	{
	_Draw.m_szText = pWeaponInfo->szHudName+13;
	replaceAll(_Draw.m_szText, "_", " ");
	}
	}
	}
	break;
	}
	case DEFUSING:
	{
	// no bombinfo
	if (!g_Info.m_pC4)
	break;

	// is defusing
	if (g_Info.m_flDefuseTime>0.f && Entity->IsDefusing())
	{
	// can make it
	if (g_Info.m_flDefuseTime<=g_Info.m_pC4->GetBombTimer())
	_Draw.m_clr = Color(255, 0, 0, playerAlpha);
	// can't make it
	else
	_Draw.m_clr = Color(0, 255, 0);

	// prepare drawing
	_Draw.m_szText = StringFromArgs("Defusing (%.1fs)", g_Info.m_flDefuseTime);

	//Iif it's some kind of bar
	if (_Item->m_Flags>TEXT)
	_Draw.Set("", StringFromArgs("%.1fs", g_Info.m_flDefuseTime), g_Info.m_flDefuseTime, Entity->HasDefuseKit() ? 5.f : 10.f, _Draw.m_clr);
	}
	break;
	}
	case ZOOM:
	{
	if (_playerRecord->m_pEntity->IsScoped())
	_Draw.Set("SCOPED", "", 0, 0, Color(0, 255, 255, playerAlpha));
	break;
	}
	case ANTIAIM:
	{
	std::string _msg = StringFromArgs("m_iShotsMissed: %i", _playerRecord->m_iShotsMissed);
	_Draw.Set(_msg, "", 0, 0, Color(255, 0, 0, playerAlpha));
	//ToDo: this
	break;
	}
	case PING:
	{
	int ping = 0; //ToDo: get this from PlayerResource
	if (ping > 100)
	_Draw.Set(StringFromArgs("PING: %i ms", ping), "", 0, 0, Color(255, 150, 0, playerAlpha));
	break;
	}
	case LBYTIME:
	{
	if (_playerRecord->m_bMoving||!_playerRecord->m_bPredictLBY)
	_Draw.m_flMax = 0.f;
	else
	_Draw.m_flMax = 1.1f;

	_Draw.m_flValue = _playerRecord->m_next_lby_update_time - Interfaces::Globals->curtime;
	_Draw.m_clr		= Color(255, 0, 255, playerAlpha);						break;
	}
	case BOMBCARRIER:
	{
	if (_playerRecord->m_pEntity->HasC4())
	_Draw.Set("BOMB", "", 0.f, 0.f, Color(255, 165, 0, playerAlpha));
	break;
	}
	case RELOAD:
	{
	const auto layer = _playerRecord->m_pEntity->GetAnimOverlay(AIMSEQUENCE_LAYER1);
	if (layer && layer->m_flWeight != 0.f && GetSequenceActivity(_playerRecord->m_pEntity, layer->_m_nSequence) == ACT_CSGO_RELOAD)
	_Draw.Set("RELOAD", "", 0.f, 0.f, Color(0, 255, 255, playerAlpha));
	break;
	}
	case MONEY:
	{
	_Draw.Set(StringFromArgs("$%d", _playerRecord->m_pEntity->GetMoney()), "", 0.f, 0.f, Color(0, 255, 0, playerAlpha));
	break;
	}
	case KIT:
	{
	if(_playerRecord->m_pEntity->HasDefuseKit())
	_Draw.Set("KIT", "", 0.f, 0.f, Color(255, 165, 0, playerAlpha));
	break;
	}
	case PINPULL:
	{
	if (Weapon && Weapon->IsPinPulled())
	_Draw.Set("PIN PULL", "", 0.f, 0.f, Color(0, 255, 255, playerAlpha));
	break;
	}
	case BURN:
	{
	if (Interfaces::Globals->curtime - _playerRecord->m_pEntity->m_fMolotovDamageTime() <= 0.2f)
	_Draw.Set("BURN", "", 0.f, 0.f, Color(255, 165, 0, playerAlpha));
	break;
	}
	case BLIND:
	{
	if (_playerRecord->m_pEntity->FlashbangTime() > 0.001f)
	_Draw.Set("BLIND", "", 0.f, 0.f, Color(0, 255, 255, playerAlpha));
	break;
	}
	case VESTHELM:
	{
	if (_playerRecord->m_pEntity->GetArmor() > 0 && _playerRecord->m_pEntity->HasHelmet())
	_Draw.Set("H+K", "", 0.f, 0.f, Color(255, 255, 255, playerAlpha));
	else if (_playerRecord->m_pEntity->HasHelmet())
	_Draw.Set("H", "", 0.f, 0.f, Color(255, 255, 255, playerAlpha));
	else
	_Draw.Set("K", "", 0.f, 0.f, Color(255, 255, 255, playerAlpha));
	break;
	}
	case FAKEDUCK:
	{
	if (_playerRecord->m_last_duckamount > 0.f && _playerRecord->m_last_duckamount < 1.f)
	{
	if (_playerRecord->m_last_duckamount == _playerRecord->m_pEntity->GetDuckAmount())
	_Draw.Set("FD", "", 0.f, 0.f, Color(255, 0, 0, playerAlpha));
	}
	break;
	}
	default:
	break;
	}

	// if the item should not be drawn, continue
	if (!_Draw.ShouldDraw())
	continue;

	// calculate renderPos according to side
	switch (side)
	{
	case LEFT:
	if (_Item->m_Flags == TEXT)
	{
	_Item->m_RenderPos.x = x - Side.m_iBarCount * 6 - 5;
	_Item->m_RenderPos.y = y + _tempTextDist;
	_Draw.m_Align		 = CSurfaceDraw::TextAlign::RIGHT;
	_tempTextDist += 14;
	}
	else
	{
	_Item->m_RenderPos.x = x - _tempBarDist - 6;
	_Item->m_RenderPos.y = y;
	_tempBarDist += 6;
	}
	break;
	case RIGHT:
	if (_Item->m_Flags == TEXT)
	{
	_Item->m_RenderPos.x = x + w + Side.m_iBarCount * 6 + 5;
	_Item->m_RenderPos.y = y + _tempTextDist;
	_Draw.m_Align		 = CSurfaceDraw::TextAlign::LEFT;
	_tempTextDist += 14;
	}
	else
	{
	_Item->m_RenderPos.x = x + w + _tempBarDist + 1;
	_Item->m_RenderPos.y = y;
	_tempBarDist += 6;
	}
	break;
	case TOP:
	if (_Item->m_Flags == TEXT)
	{
	_Item->m_RenderPos.x = x + w / 2;
	_Item->m_RenderPos.y = y - 10 - Side.m_iBarCount * 6 - _tempTextDist;
	_Draw.m_Align		 = CSurfaceDraw::TextAlign::CENTER;
	_tempTextDist += 14;
	}
	else
	{
	_Item->m_RenderPos.x = x;
	_Item->m_RenderPos.y = y - 10 - _tempBarDist;
	_tempBarDist += 6;
	}
	break;
	case BOTTOM:
	if (_Item->m_Flags == TEXT)
	{
	_Item->m_RenderPos.x = x + w / 2;
	_Item->m_RenderPos.y = y + h + 10 + Side.m_iBarCount * 6 + _tempTextDist;
	_Draw.m_Align		 = CSurfaceDraw::TextAlign::CENTER;
	_tempTextDist += 14;
	}
	else
	{
	_Item->m_RenderPos.x = x;
	_Item->m_RenderPos.y = y + h + 5 + _tempBarDist;
	_tempBarDist += 6;
	}
	break;
	default:
	break;
	}

	// draw text
	if (_Item->m_Flags == TEXT)
	g_Draw.Text(_Item->m_RenderPos.x, _Item->m_RenderPos.y, _Draw.m_clr.r(), _Draw.m_clr.g(), _Draw.m_clr.b(), _Draw.m_clr.a(), (CSurfaceDraw::TextAlign)_Draw.m_Align, Fonts::Main, "%s", _Draw.m_szText.c_str()); //Draw Text
	//Draw Bars
	else if (_Item->m_Flags != NO_FLAG)
	{
	// healthbars are handled differently due to color fade
	if (_Item->m_Type == HEALTH)
	{
	// item is on left or right side
	if (side < TOP)
	g_Draw.Healthbar(_Item->m_RenderPos.x, _Item->m_RenderPos.y, 4, h, _Draw.m_flValue, _Item->m_Flags != BAR, playerAlpha);
	// item is on top or bottom side
	else
	g_Draw.HealthbarHorizontal(_Item->m_RenderPos.x, _Item->m_RenderPos.y, w, 4, _Draw.m_flValue, _Item->m_Flags != BAR, playerAlpha);
	}
	// normal Bars (Armor, Ammo, etc.)
	else
	{
	// we have a valid maximum
	if (_Draw.m_flMax > 0.f)
	{
	// item is on left or right side
	if (side < TOP)
	g_Draw.DrawBar(_Item->m_RenderPos.x, _Item->m_RenderPos.y, 4, h, _Draw.m_flValue, _Draw.m_flMax, _Draw.m_clr, _Item->m_Flags != BAR, _Draw.m_szCustom, playerAlpha);
	// item is on top or bottom side
	else
	g_Draw.DrawBarHorizontal(_Item->m_RenderPos.x, _Item->m_RenderPos.y, w, 4, _Draw.m_flValue, _Draw.m_flMax, _Draw.m_clr, _Item->m_Flags != BAR, _Draw.m_szCustom, playerAlpha);
	}
	}
	}
	}
	}
	}

	m_vecESP[DRAW] = nullptr;
	*/
}

void Parallel_DrawEntityESP(CEntInfo& entinfo)
{
	IClientUnknown* pHandle = (IClientUnknown*)entinfo.m_pEntity;
	CBaseEntity* Entity;
	if (pHandle && (Entity = pHandle->GetBaseEntity(), Entity != nullptr))
	{
		g_Visuals.DrawEntityESP(Entity);
	}
}

void CVisuals::DrawEntityESP(CBaseEntity* Entity)
{
#ifdef IMI_MENU
	ClassID iClassID = (ClassID)Entity->GetClientClass()->m_ClassID;

	if (Entity->IsPlayer() || iClassID == _CCSPlayer)
	{
		DrawPlayerESP(Entity);
		return;
	}

	//Get EntityType
	eEntityType _type = Entity->GetEntityType();

	ESPBuild_s vecESP;
	vecESP.m_bEnabled = false;

	//Get correct ESP Build via EntityType
	switch (_type)
	{
	case plantedc4:
		g_Info.m_pC4 = Entity;
		vecESP = *m_vecESP[BOMB];
		break;
	case chicken:
		vecESP = *m_vecESP[CHICKEN];
		break;
	case projectile:
		vecESP = *m_vecESP[NADE];
		break;
	case weapon:
		vecESP = *m_vecESP[WEAP];
		break;
	default:
		return;
	}

	if (vecESP.m_bEnabled)
	{
		//Init locals
		Color color = vecESP.m_clrBox;

		BoundingBox BBox;
		if (!GenerateBoundingBox(Entity, Entity->GetAbsOriginDirect(), Entity->GetAngleRotation(), true, BBox))
			return;

		float x = BBox.x, y = BBox.y;
		float w = BBox.w, h = BBox.h;

		if (vecESP.m_iBoxstyle)
			g_Draw.ESPBox(x, y, w, h, 1, vecESP.m_iCornerWidth, vecESP.m_iCornerHeight, color);

		CBaseCombatWeapon* Weapon = _type == weapon ? (CBaseCombatWeapon*)Entity : nullptr;

		//Loop through all sides
		for (int side = LEFT; side <= BOTTOM; side++)
		{
			//Init loop locals
			auto Side = vecESP.m_Sides[side];
			int _tempBarDist = 0, _tempTextDist = 0;

			//Loop through all sides elements
			for (size_t i = 0; i < Side.m_Elements.size(); i++)
			{
				//Init loop locals
				auto _pItem = *Side.m_Elements[i];
				auto _Item = &_pItem;
				sDraw _Draw;
				_Draw.m_clr = color;

				//Set _Draw according to element type
				switch ((int)_Item->m_Type)
				{
				case AMMO:
					if (Weapon)
					{
						//Prepare String
						if (Weapon->GetCSWpnData()->iMaxClip1 > -1)
							_Draw.m_szText = StringFromArgs("%i / %i", Weapon->GetClipOne(), Weapon->GetCSWpnData()->iMaxClip1);

						//It is a bar
						if (_Item->m_Flags > TEXT)
							_Draw.Set("", "", Weapon->GetClipOne(), Weapon->GetCSWpnData()->iMaxClip1, Color(255, 100, 0, 255));
					}
					break;
				case NAME:
				{
					switch (_type)
					{
					case plantedc4:
						_Draw.Set("C4", "", 0, 0, Color(255, 100, 0, 255));
						break;
					case chicken:
						_Draw.Set("IZ CHICKEN LEL");
						break;
					case projectile:
						_Draw.Set(GetStrippedModelname(Entity));
						break;
					default:
						break;
					}
				}
				break;
				case WEAPON:
					if (Weapon)
					{
						WeaponInfo_t* pWeaponInfo = Weapon->GetCSWpnData();
						if (pWeaponInfo->szHudName[0] == '#' && pWeaponInfo->szHudName[12] == '_')
						{
							_Draw.m_szText = pWeaponInfo->szHudName + 13;
							replaceAll(_Draw.m_szText, "_", " ");
						}
					}
					break;
				case DETONATION:
					//No Bombinfo
					if (!g_Info.m_pC4)
						break;

					if (_type == plantedc4 && g_Info.m_pC4->GetBombTimer() > 0.f)
					{
						//Adjust color and text
						_Draw.m_clr = Color(250, 100, 0);
						_Draw.m_szText = StringFromArgs("Explosion in %.1fs", g_Info.m_pC4->GetBombTimer());

						//It is a bar
						if (_Item->m_Flags > FLAGS::TEXT)
							//Adjust other needed values
							_Draw.Set("", StringFromArgs("%.1fs", g_Info.m_pC4->GetBombTimer()), g_Info.m_pC4->GetBombTimer(), 45.f, _Draw.m_clr); //ToDo: implement cvar for bombtime
					}
					break;
				default:
					break;
				}

				//If the item should not be drawn, continue
				if (!_Draw.ShouldDraw())
					continue;

				//Calculate RenderPos according to side
				switch (side)
				{
				case LEFT:
					if (_Item->m_Flags == TEXT)
					{
						_Item->m_RenderPos.x = x - Side.m_iBarCount * 6 - 5;
						_Item->m_RenderPos.y = y + _tempTextDist;
						_Draw.m_Align = CSurfaceDraw::TextAlign::RIGHT;
						_tempTextDist += 14;
					}
					else
					{
						_Item->m_RenderPos.x = x - _tempBarDist - 6;
						_Item->m_RenderPos.y = y;
						_tempBarDist += 6;
					}
					break;
				case RIGHT:
					if (_Item->m_Flags == TEXT)
					{
						_Item->m_RenderPos.x = x + w + Side.m_iBarCount * 6 + 5;
						_Item->m_RenderPos.y = y + _tempTextDist;
						_Draw.m_Align = CSurfaceDraw::TextAlign::LEFT;
						_tempTextDist += 14;
					}
					else
					{
						_Item->m_RenderPos.x = x + w + _tempBarDist + 1;
						_Item->m_RenderPos.y = y;
						_tempBarDist += 6;
					}
					break;
				case TOP:
					if (_Item->m_Flags == TEXT)
					{
						_Item->m_RenderPos.x = x + w / 2;
						_Item->m_RenderPos.y = y - 10 - Side.m_iBarCount * 6 - _tempTextDist;
						_Draw.m_Align = CSurfaceDraw::TextAlign::CENTER;
						_tempTextDist += 14;
					}
					else
					{
						_Item->m_RenderPos.x = x;
						_Item->m_RenderPos.y = y - 10 - _tempBarDist;
						_tempBarDist += 6;
					}
					break;
				case BOTTOM:
					if (_Item->m_Flags == TEXT)
					{
						_Item->m_RenderPos.x = x + w / 2;
						_Item->m_RenderPos.y = y + h + 10 + Side.m_iBarCount * 6 + _tempTextDist;
						_Draw.m_Align = CSurfaceDraw::TextAlign::CENTER;
						_tempTextDist += 14;
					}
					else
					{
						_Item->m_RenderPos.x = x;
						_Item->m_RenderPos.y = y + h + 5 + _tempBarDist;
						_tempBarDist += 6;
					}
					break;
				default:
					break;
				}

				//Draw Text
				if (_Item->m_Flags == TEXT)
					g_Draw.Text(_Item->m_RenderPos.x, _Item->m_RenderPos.y, _Draw.m_clr.r(), _Draw.m_clr.g(), _Draw.m_clr.b(), _Draw.m_clr.a(), (CSurfaceDraw::TextAlign)_Draw.m_Align, Fonts::Main, "%s", _Draw.m_szText.c_str()); //Draw Text
																																																									//Draw Bars
				else if (_Item->m_Flags != NO_FLAG)
				{
					//We have a valid maximum
					if (_Draw.m_flMax > 0.f)
					{
						//Item is on left or right side
						if (side < TOP)
							g_Draw.DrawBar(_Item->m_RenderPos.x, _Item->m_RenderPos.y, 4, h, _Draw.m_flValue, _Draw.m_flMax, _Draw.m_clr, _Item->m_Flags != BAR, _Draw.m_szCustom, 255);
						//Item is on top or bottom side
						else
							g_Draw.DrawBarHorizontal(_Item->m_RenderPos.x, _Item->m_RenderPos.y, w, 4, _Draw.m_flValue, _Draw.m_flMax, _Draw.m_clr, _Item->m_Flags != BAR, _Draw.m_szCustom, 255);
					}
				}
			}
		}
	}

	m_vecESP[DRAW] = nullptr;
#else
	if (Entity->GetClientClass())
	{
		// check for planted c4
		auto classname = Entity->GetClientClass()->GetName();
		//decrypts(0)
		bool is_planted_c4 = !strcmp(classname, XorStr("CPlantedC4"));
		//encrypts(0)
		if (variable::get().visuals.b_c4timer && is_planted_c4)
		{
			if (Entity->GetDormant())
				return;

			DrawPlantedBomb(Entity);
			return;
		}

		Vector screen;
		if (!WorldToScreen(Entity->WorldSpaceCenter(), screen))
			return;


		// dropped weapons
		if (Entity->IsWeapon())
		{
			if (Entity->GetOwner() != nullptr || Entity->GetDormant())
				return;

			if (variable::get().visuals.vf_weapon.b_enabled)
			{
				DrawDroppedItem(Entity, screen);
				return;
			}
		}

		// dropped bomb
		//decrypts(0)
		bool is_bomb = !strcmp(classname, XorStr("CC4"));
		//encrypts(0)
		if (variable::get().visuals.vf_bomb.b_enabled && is_bomb && Entity->GetOwner() == nullptr)
		{
			BoundingBox BBox;
			if (!GenerateBoundingBox(Entity, Entity->GetAbsOriginDirect(), Entity->GetAngleRotation(), true, BBox))
				return;

			// draw box
			if (variable::get().visuals.vf_bomb.b_box)
			{
				//render::get().add_rect(ImVec2(BBox.x, BBox.y), BBox.w, BBox.h, ImColor(0, 0, 0, 255));
				//render::get().add_rect(ImVec2(BBox.x - 1.f, BBox.y - 1.f), BBox.w + 2, BBox.h + 2, variable::get().visuals.vf_bomb.col_main.color().ToImGUI());
				//render::get().add_rect(ImVec2(BBox.x + 1.f, BBox.y - 1.f), BBox.w - 2, BBox.h - 2, variable::get().visuals.vf_bomb.col_main.color().ToImGUI());

				render::get().add_custom_box(1, false, RECT{ (long)BBox.x, (long)BBox.y, (long)(BBox.w + BBox.x), (long)(BBox.h + BBox.y) }, nullptr, variable::get().visuals.vf_bomb.col_main.color(), Color::Black(), Color::Black());
			}

			if (variable::get().visuals.vf_bomb.b_name)
			{
				//decrypts(0)
				render::get().add_text(ImVec2(screen.x, screen.y), variable::get().visuals.vf_bomb.col_name.color().ToImGUI(), CENTERED_X | CENTERED_Y | DROP_SHADOW, BAKSHEESH_14, XorStr("C4"));
				//encrypts(0)
			}

			return;
		}

		// thrown projectiles
		//decrypts(0)
		bool is_proj = strstr(classname, XorStr("Projectile")) || !strcmp(classname, XorStr("CInferno"));
		//encrypts(0)

		if (variable::get().visuals.vf_projectile.b_enabled && is_proj)
		{
			if (!Entity->GetOwner() || Entity->GetDormant())
				return;

			DrawProjectile(Entity, screen);
		}
	}

#endif
}

void CVisuals::DrawPlantedBomb(CBaseEntity* Entity)
{
	auto get_armour_health = [](float fl_damage, int armor_value) -> float
	{
		const auto fl_cur_damage = fl_damage;
		if (fl_cur_damage == 0.0f || armor_value == 0)
			return fl_cur_damage;

		const auto fl_armor_ratio = 0.5f;
		const auto fl_armor_bonus = 0.5f;

		auto fl_new = fl_cur_damage * fl_armor_ratio;
		auto fl_armor = (fl_cur_damage - fl_new) * fl_armor_bonus;

		if (fl_armor > armor_value)
		{
			fl_armor = armor_value * (1.0f / fl_armor_bonus);
			fl_new = fl_cur_damage - fl_armor;
		}

		return fl_new;
	};

	const float defuse_countdown = *(float*)((DWORD)Entity + g_NetworkedVariables.Offsets.m_flDefuseCountDown);
	const float time_left = max(0.f, Entity->GetBombTimer());
	const float defuse_time_left = max(0.f, defuse_countdown - TICKS_TO_TIME(LocalPlayer.Entity->GetTickBase()));
	const bool bomb_ticking = *(bool*)((DWORD)Entity + g_NetworkedVariables.Offsets.m_bBombTicking);

	Vector screen{};
	CBaseEntity *defuser = (CBaseEntity*)Interfaces::ClientEntList->GetClientEntityFromHandle(*(EHANDLE*)((DWORD)Entity + g_NetworkedVariables.Offsets.m_hBombDefuser));

	// only draw if we're about to blow up
	if (time_left > 0.01f && bomb_ticking)
	{
		auto viewport = render::get().get_viewport();
		if (!defuser)
		{
			//decrypts(0)
			if (!Entity->GetBombDefused())
				render::get().add_text(ImVec2(viewport.Width / 2, 100.f), adr_util::get_health_color(adr_math::my_super_clamp(static_cast<int>(time_left), 0, 100)).ToImGUI(), CENTERED_X | CENTERED_Y | DROP_SHADOW, BAKSHEESH_14, XorStr("C4 is planted! %2.1f sec left"), time_left);
			//encrypts(0)
		}
		else
		{
			//decrypts(0)
			render::get().add_text(ImVec2(viewport.Width / 2, 100.f), adr_util::get_health_color(adr_math::my_super_clamp(static_cast<int>(defuse_time_left), 0, 100)).ToImGUI(), CENTERED_X | CENTERED_Y | DROP_SHADOW, BAKSHEESH_14, XorStr("C4 is being defused! %2.1f sec left to defuse - %2.1f sec left total"), defuse_time_left, time_left);
			//encrypts(0)
		}

		auto src = Entity->GetAbsOriginDirect();
		if (!src.IsZero())
		{
			auto f_dist = LocalPlayer.Entity->GetEyePosition().Dist(src) * 0.01905f;
			auto f_rad = (f_dist / 0.01905f - 75.68f) / 789.2f;
			auto i_damage = max(static_cast<int>(ceilf(get_armour_health(450.7f * exp(-f_rad * f_rad), LocalPlayer.Entity->GetArmor()))), 0);

			auto i_health_left = LocalPlayer.Entity->GetHealth() - i_damage + 1;
			if (i_health_left < 0)
				i_health_left = 0;

			//decrypts(0)
			if (LocalPlayer.Entity->GetAlive() && !Entity->GetBombDefused())
				render::get().add_text(ImVec2(viewport.Width / 2, 116.f), adr_util::get_health_color(i_health_left).ToImGUI(), CENTERED_X | CENTERED_Y | DROP_SHADOW, BAKSHEESH_14, XorStr("Remaining HP: ~ %d HP"), i_health_left);
			//encrypts(0)
		}

		// check to see if the c4 is on the screen, if so we're gonna draw a summary of what we draw up top; on the bomb itself
		if (WorldToScreen(Entity->WorldSpaceCenter(), screen))
		{
			//decrypts(0)
			render::get().add_text(ImVec2(screen.x, screen.y), adr_util::get_health_color(adr_math::my_super_clamp(static_cast<int>(time_left), 0, 100)).ToImGUI(), CENTERED_X | CENTERED_Y | DROP_SHADOW, BAKSHEESH_12, XorStr("%2.1f secs"), time_left);
			if (defuser)
			{
				render::get().add_text(ImVec2(screen.x, screen.y + 15.f), adr_util::get_health_color(adr_math::my_super_clamp(static_cast<int>(defuse_time_left), 0, 100)).ToImGUI(), CENTERED_X | CENTERED_Y | DROP_SHADOW, BAKSHEESH_12, XorStr("%2.1f secs"), defuse_time_left);
			}
			//encrypts(0)
		}
	}
}

void CVisuals::DrawDroppedItem(CBaseEntity* Entity, const Vector& screen)
{
	CBaseCombatWeapon* weapon = reinterpret_cast<CBaseCombatWeapon*>(Entity);
	WeaponInfo_t* data = weapon->GetCSWpnData();
	if (!data)
		return;

	BoundingBox BBox;
	if (!GenerateBoundingBox(Entity, Entity->GetAbsOriginDirect(), Entity->GetAngleRotation(), true, BBox))
		return;

	// draw box
	if (variable::get().visuals.vf_weapon.b_box)
	{
		//render::get().add_rect(ImVec2(BBox.x, BBox.y), BBox.w, BBox.h, ImColor(0, 0, 0, 255));
		//render::get().add_rect(ImVec2(BBox.x - 1.f, BBox.y - 1.f), BBox.w + 2, BBox.h + 2, variable::get().visuals.vf_weapon.col_main.color().ToImGUI());
		//render::get().add_rect(ImVec2(BBox.x + 1.f, BBox.y - 1.f), BBox.w - 2, BBox.h - 2, variable::get().visuals.vf_weapon.col_main.color().ToImGUI());

		render::get().add_custom_box(1, false, RECT{ (long)BBox.x, (long)BBox.y, (long)(BBox.w + BBox.x), (long)(BBox.h + BBox.y) }, nullptr, variable::get().visuals.vf_weapon.col_main.color(), Color::Black(), Color::Black());
	}

	// draw name and info
	// is this a valid weapon with ammo
	if (data->WeaponType != WEAPONTYPE_GRENADE && data->WeaponType != WEAPONTYPE_KNIFE && weapon->GetItemDefinitionIndex() != WEAPON_TASER)
	{
		//decrypts(0)
		if (variable::get().visuals.vf_weapon.b_name)
		{
			render::get().add_text(ImVec2(screen.x, screen.y), variable::get().visuals.vf_weapon.col_name.color().ToImGUI(), CENTERED_X | CENTERED_Y | DROP_SHADOW, BAKSHEESH_14, XorStr("%s"), weapon->GetWeaponName().data());
		}
		if (variable::get().visuals.vf_weapon.b_info)
		{
			render::get().add_text(ImVec2(screen.x, screen.y + 10.f), variable::get().visuals.vf_weapon.col_name.color().ToImGUI(), CENTERED_X | CENTERED_Y | DROP_SHADOW, BAKSHEESH_14, XorStr("%d / %d"), weapon->GetClipOne(), weapon->GetPrimaryReserveAmmoCount());
		}
		//encrypts(0)
	}
	else
	{
		//decrypts(0)
		if (variable::get().visuals.vf_weapon.b_name)
		{
			render::get().add_text(ImVec2(screen.x, screen.y), variable::get().visuals.vf_weapon.col_name.color().ToImGUI(), CENTERED_X | CENTERED_Y | DROP_SHADOW, BAKSHEESH_14, XorStr("%s"), weapon->GetWeaponName().data());
		}
		//encrypts(0)
	}
}

void CVisuals::DrawProjectile(CBaseEntity* Entity, const Vector& screen)
{
	auto owner = Entity->GetOwner();
	if (!owner)
		return;

	auto owner_name = adr_util::sanitize_name((char*)owner->GetName().data());

	BoundingBox BBox;
	if (!GenerateBoundingBox(Entity, Entity->GetAbsOriginDirect(), Entity->GetAngleRotation(), true, BBox))
		return;

	// draw box
	if (variable::get().visuals.vf_projectile.b_box && Entity->GetExplodeEffectTickBegin() == 0)
	{
		//render::get().add_rect(ImVec2(BBox.x, BBox.y), BBox.w, BBox.h, ImColor(0, 0, 0, 255));
		//render::get().add_rect(ImVec2(BBox.x - 1.f, BBox.y - 1.f), BBox.w + 2, BBox.h + 2, variable::get().visuals.vf_projectile.col_main.color().ToImGUI());
		//render::get().add_rect(ImVec2(BBox.x + 1.f, BBox.y - 1.f), BBox.w - 2, BBox.h - 2, variable::get().visuals.vf_projectile.col_main.color().ToImGUI());

		render::get().add_custom_box(1, false, RECT{ (long)BBox.x, (long)BBox.y, (long)(BBox.w + BBox.x), (long)(BBox.h + BBox.y) }, nullptr, variable::get().visuals.vf_projectile.col_main.color(), Color::Black(), Color::Black());
	}

	if (variable::get().visuals.b_projectile_owner && Entity->GetExplodeEffectTickBegin() == 0)
	{
		//decrypts(0)
		render::get().add_text(ImVec2(screen.x, screen.y + 10.f), Color::White().ToImGUI(), CENTERED_X | CENTERED_Y | DROP_SHADOW, BAKSHEESH_14, XorStr("%s"), owner_name.data());
		//encrypts(0)
	}

	const model_t *model = Entity->GetModel();

	if (!model)
		return;

	std::string model_name = model->name;

	ImColor color = Color::White().ToImGUI();
	std::string proj_name{};

	const char* classname = Entity->GetClientClass()->GetName();

	// check the class id to figure out what to draw for the name
	// cbasecsgrenadeprojectile shares the same id for flashbang/frag so we need to check the model name
	//decrypts(0)
	if (!strcmp(classname, XorStr("CBaseCSGrenadeProjectile")))
	{
		if (model_name.find(XorStr("bang")) != std::string::npos)
		{
			color = Color::Green().ToImGUI();
			proj_name = XorStr("Flash");
		}
		else if (model_name.find(XorStr("frag")) != std::string::npos)
		{
			color = Color::Red().ToImGUI();
			proj_name = XorStr("Frag");
		}
	}
	// same with cmolotovprojectile for incendiary/molotov
	else if (!strcmp(classname, XorStr("CMolotovProjectile")))
	{
		if (model_name.find(XorStr("ince")) != std::string::npos)
		{
			color = Color::Orange().ToImGUI();
			proj_name = XorStr("Incendiary");
		}
		else if (model_name.find(XorStr("molo")) != std::string::npos)
		{
			color = Color::Orange().ToImGUI();
			proj_name = XorStr("Molotov");
		}
	}
	else
	{
		if (!strcmp(classname, XorStr("CDecoyProjectile")))
		{
			color = Color::White().ToImGUI();
			proj_name = XorStr("Decoy");
		}
		else if (!strcmp(classname, XorStr("CSmokeGrenadeProjectile")))
		{
			color = Color::White().ToImGUI();
			proj_name = XorStr("Smoke");
		}
		else if (!strcmp(classname, XorStr("CSensorGrenadeProjectile")))
		{
			color = Color::White().ToImGUI();
			proj_name = XorStr("TAG");
		}
	}
	//encrypts(0)
	// fix the bug that Valve introduced as frag grenades/decoys to still draw after exploding, fucking smh.
	if (!proj_name.empty() && variable::get().visuals.vf_projectile.b_name && Entity->GetExplodeEffectTickBegin() == 0)
	{
		render::get().add_text(ImVec2(screen.x, screen.y), color, CENTERED_X | CENTERED_Y | DROP_SHADOW, BAKSHEESH_14, proj_name.data());
	}
}

void CVisuals::DrawGlow(CBaseEntity* Entity, int r, int g, int b, int a)
{
	static GlowObjectManager* glow = *(GlowObjectManager**)GetGlowObjectManager();

	GlowObjectManager::GlowObjectDefinition_t* m_pGlowEntity = nullptr;

	for (int i = 0; i < glow->m_GlowObjectDefinitions.Count(); ++i)
	{
		auto& _ent = glow->m_GlowObjectDefinitions[i];

		if (_ent.getEnt() == Entity)
		{
			m_pGlowEntity = &_ent;
			break;
		}
	}

	if (m_pGlowEntity)
		m_pGlowEntity->set(float(r) / 255.0f, float(g) / 255.0f, float(b) / 255.0f, float(a) / 255.0f);
}

void CVisuals::DrawGlow(CBaseEntity* Entity, Color clr, int a)
{
	DrawGlow(Entity, clr.r(), clr.g(), clr.b(), a);
}

void CVisuals::DrawGlow(CBaseEntity* Entity, Color clr)
{
	DrawGlow(Entity, clr.r(), clr.g(), clr.b(), clr.a());
}

void CVisuals::OffscreenESP(CBaseEntity* ent, Color color, float size)
{
	Vector entPos = ent->WorldSpaceCenter();

	Vector dirToTarget = (entPos - LocalPlayer.Entity->GetAbsOriginDirect());
	VectorNormalizeFast(dirToTarget);

	const float radius = 360.f;

	int width, height;
	Interfaces::EngineClient->GetScreenSize(width, height);

	QAngle angles = LocalPlayer.CurrentEyeAngles;
	//Interfaces::EngineClient->GetViewAngles(angles);

	Vector forward, right, up;
	up = Vector(0.f, 0.f, 1.f);

	AngleVectors(angles, &forward);
	forward.z = 0.f;
	VectorNormalizeFast(forward);

	CrossProduct(up, forward, right);

	float front = DotProduct(dirToTarget, forward);
	float side = DotProduct(dirToTarget, right);

	float xPos = radius * -side;
	float yPos = radius * -front;

	float rot = RAD2DEG(atan2(xPos, yPos) + M_PI);

	float yawRad = DEG2RAD(-rot);

	xPos = (int)((width * 0.5f) + (radius * sin(yawRad)));
	yPos = (int)((height * 0.5f) - (radius * cos(yawRad)));

	Vector vec;
	vec.x = angles.x;
	vec.y = angles.y;
	vec.z = angles.z;

	Vertex_t tris[3];

	tris[0].m_Position = { xPos, yPos };
	tris[1].m_Position = { xPos - size, yPos + size * 2.f };
	tris[2].m_Position = { xPos + size, yPos + size * 2.f };

	Vector2D offscreenPos(xPos, yPos);

	tris[0] = RotateVertex(offscreenPos, tris[0].m_Position, -rot);
	tris[1] = RotateVertex(offscreenPos, tris[1].m_Position, -rot);
	tris[2] = RotateVertex(offscreenPos, tris[2].m_Position, -rot);

	int idx = ent->index;

	// draw an opaque one
#ifdef IMI_MENU
	Interfaces::Surface->DrawSetColor(color);
	Interfaces::Surface->DrawTexturedPolygon(3, tris);
#else
	ImVec2 point_a = { tris[0].m_Position.x, tris[0].m_Position.y };
	ImVec2 point_b = { tris[1].m_Position.x, tris[1].m_Position.y };
	ImVec2 point_c = { tris[2].m_Position.x, tris[2].m_Position.y };

	// nice effect
	static auto alpha = 0.f;
	static auto plus_or_minus = false;

	if (alpha <= 0.f || alpha >= 255.f)
		plus_or_minus = !plus_or_minus;

	alpha += plus_or_minus ? (255.f / 2.f * Interfaces::Globals->frametime) : -(255.f / 2.f * Interfaces::Globals->frametime);
	alpha = adr_math::my_super_clamp(alpha, 0.f, 255.f);

	color.SetAlpha(alpha);
	render::get().add_triangle_filled(point_a, point_b, point_c, color.ToImGUI());
#endif
}

void CVisuals::DrawGlow()
{
	if (!variable::get().visuals.b_enabled)
		return;

	auto apply_glow = [](GlowObjectManager::GlowObjectDefinition_t *obj, const ImColor &color, const float opacity, const bool full_bloom, const int style, const float blend, const bool should_glow = true, const bool exception = false)
	{
		if (!should_glow)
		{
			obj->m_flGlowAlpha = 0.f;
			return;
		}

		obj->m_iGlowStyle = style;
		obj->m_bRenderWhenOccluded = true;
		obj->m_bRenderWhenUnoccluded = false;
		obj->m_bFullBloomRender = full_bloom;
		obj->m_flGlowRed = color.Value.x;
		obj->m_flGlowGreen = color.Value.y;
		obj->m_flGlowBlue = color.Value.z;
		obj->m_flGlowAlpha = opacity * (blend / 100.f);

		// nit; this fucking offset is the same offset for plantedc4 'num of entities to destroy' in DestroyDefuserRopes. 
		//If you set it to true, it will cause a null pointer deref
		if (!exception)
			*(bool*)((DWORD)obj->m_pEntity + g_NetworkedVariables.Offsets.m_bShouldGlow) = should_glow;
	};

	const auto glow_mgr = *(GlowObjectManager**)GetGlowObjectManager();

	if (!LocalPlayer.Entity || !glow_mgr || glow_mgr->m_GlowObjectDefinitions.Count() < 1)
		return;

	const bool glow_local_player = variable::get().visuals.pf_local_player.vf_main.b_enabled && variable::get().visuals.pf_local_player.vf_main.glow.b_enabled;
	const bool glow_enemy = variable::get().visuals.pf_enemy.vf_main.glow.b_enabled && variable::get().visuals.pf_enemy.vf_main.b_enabled;
	const bool glow_teammate = variable::get().visuals.pf_teammate.vf_main.glow.b_enabled && variable::get().visuals.pf_teammate.vf_main.b_enabled;
	const bool glow_weapons = variable::get().visuals.vf_weapon.glow.b_enabled && variable::get().visuals.vf_weapon.b_enabled;
	const bool glow_bomb = variable::get().visuals.vf_bomb.glow.b_enabled && variable::get().visuals.vf_bomb.b_enabled;
	const bool glow_projectiles = variable::get().visuals.vf_projectile.glow.b_enabled && variable::get().visuals.vf_projectile.b_enabled;

	float opacity = 1.f;
	ImColor color = Color::White().ToImGUI();
	int style = 0;
	bool full_bloom = false;
	float blend = 1.f;

	for (auto i = 0; i < glow_mgr->m_GlowObjectDefinitions.Count(); ++i)
	{
		GlowObjectManager::GlowObjectDefinition_t *obj = &glow_mgr->m_GlowObjectDefinitions[i];

		if (!obj || !obj->m_pEntity || !obj->m_pEntity->GetClientClass() || obj->m_pEntity->GetDormant() || obj->IsUnused())
			continue;

		const int class_id = obj->m_pEntity->GetClientClass()->m_ClassID;

		if (!obj->m_pEntity->IsLocalPlayer())
		{
			bool enemy = obj->m_pEntity->IsEnemy(LocalPlayer.Entity);

			// enable m_bCanUseFastPath to allow C_BaseAnimating::GetClientModelRenderable() to fail and force DME to run
			if (obj->m_pEntity->CanUseFastPath())
				obj->m_pEntity->SetCanUseFastPath(false);

			if (obj->m_pEntity->GetOwner() == nullptr && obj->m_pEntity->IsWeapon())
			{
				if (glow_weapons)
				{
					color = variable::get().visuals.vf_weapon.glow.col_color.color().ToImGUI();
					style = variable::get().visuals.vf_weapon.glow.i_type;
					full_bloom = variable::get().visuals.vf_weapon.glow.b_fullbloom;
					blend = variable::get().visuals.vf_weapon.glow.f_blend;

					apply_glow(obj, color, opacity, full_bloom, style, blend);
					continue;
				}
				else
				{
					// disable glow since the engine never writes to weapons
					apply_glow(obj, color, opacity, full_bloom, style, blend, false);
					continue;
				}
			}
			else if (class_id == _CC4 || class_id == _CPlantedC4)
			{
				if (glow_bomb)
				{
					color = variable::get().visuals.vf_bomb.glow.col_color.color().ToImGUI();
					style = variable::get().visuals.vf_bomb.glow.i_type;
					full_bloom = variable::get().visuals.vf_bomb.glow.b_fullbloom;
					blend = variable::get().visuals.vf_bomb.glow.f_blend;

					// nit; DO NOT SET SHOULDGLOW
					apply_glow(obj, color, opacity, full_bloom, style, blend, true, true);
					continue;
				}
				else
				{
					// disable glow since the engine never writes to bomb unless planted
					apply_glow(obj, color, opacity, full_bloom, style, blend, false, true);
					continue;
				}
			}
			else if (class_id == _CInferno || obj->m_pEntity->IsProjectile())
			{
				if (glow_projectiles)
				{
					color = variable::get().visuals.vf_projectile.glow.col_color.color().ToImGUI();
					style = variable::get().visuals.vf_projectile.glow.i_type;
					full_bloom = variable::get().visuals.vf_projectile.glow.b_fullbloom;
					blend = variable::get().visuals.vf_projectile.glow.f_blend;

					apply_glow(obj, color, opacity, full_bloom, style, blend);
					continue;
				}
				else
				{
					// disable glow since the engine never writes to projectiles
					apply_glow(obj, color, opacity, full_bloom, style, blend, false);
					continue;
				}
			}
			else if (class_id == _CCSPlayer && !obj->m_bRenderWhenOccluded)
			{
				if (glow_enemy && enemy)
				{
					color = variable::get().visuals.pf_enemy.vf_main.glow.col_color.color().ToImGUI();
					style = variable::get().visuals.pf_enemy.vf_main.glow.i_type;
					full_bloom = variable::get().visuals.pf_enemy.vf_main.glow.b_fullbloom;
					blend = variable::get().visuals.pf_enemy.vf_main.glow.f_blend;

					apply_glow(obj, color, opacity, full_bloom, style, blend);
					continue;
				}
				else if (glow_teammate && !enemy)
				{
					color = variable::get().visuals.pf_teammate.vf_main.glow.col_color.color().ToImGUI();
					style = variable::get().visuals.pf_teammate.vf_main.glow.i_type;
					full_bloom = variable::get().visuals.pf_teammate.vf_main.glow.b_fullbloom;
					blend = variable::get().visuals.pf_teammate.vf_main.glow.f_blend;

					apply_glow(obj, color, opacity, full_bloom, style, blend);
					continue;
				}
			}
		}
		else
		{
			if (glow_local_player && variable::get().misc.thirdperson.b_enabled.get() && !obj->m_bRenderWhenOccluded)
			{
				color = variable::get().visuals.pf_local_player.vf_main.glow.col_color.color().ToImGUI();
				style = variable::get().visuals.pf_local_player.vf_main.glow.i_type;
				full_bloom = variable::get().visuals.pf_local_player.vf_main.glow.b_fullbloom;
				blend = variable::get().visuals.pf_local_player.vf_main.glow.f_blend;

				apply_glow(obj, color, opacity, full_bloom, style, blend);
				continue;
			}
		}
	}
}

void CVisuals::DrawESP()
{
	if (Interfaces::EngineClient->IsConnected())
	{
#if 0
		auto startindex = 1;
		ParallelProcess(Interfaces::ClientEntList->GetEntityInfo(startindex), Interfaces::ClientEntList->GetNumEntitiesStartingFromIndex(startindex), Parallel_DrawEntityESP);
#else
		for (auto i = 1; i <= MAX_PLAYERS; i++)
		{
			//IClientUnknown* pHandle = reinterpret_cast<IClientUnknown*>(Interfaces::ClientEntList->GetEntityInfo(i)->m_pEntity);
			//if (pHandle)
				//DrawPlayerESP(pHandle->GetBaseEntity());
			CBaseEntity* pBaseEntity = Interfaces::ClientEntList->GetBaseEntity(i);
			if (pBaseEntity)
				DrawPlayerESP(pBaseEntity);
		}

		for (auto i = MAX_PLAYERS + 1; i <= Interfaces::ClientEntList->GetHighestEntityIndex(); i++)
		{
			//IClientUnknown* pHandle = reinterpret_cast<IClientUnknown*>(Interfaces::ClientEntList->GetEntityInfo(i)->m_pEntity);
			//if (pHandle)
			CBaseEntity* pBaseEntity = Interfaces::ClientEntList->GetBaseEntity(i);
			if (pBaseEntity)
				DrawEntityESP(pBaseEntity);
		}
#endif
	}
}

void CVisuals::DrawHitmarker() const
{
#ifdef IMI_MENU
	if (!g_Convars.Visuals.misc_hitmarker->GetBool())
		return;

	const auto lineSize = 8;
	if (_iHurtTime + 250 >= (int)GetTickCount())
	{
		const int screenCenterX = g_Info.ScreenSize.Width / 2;
		const int screenCenterY = g_Info.ScreenSize.Height / 2;

		g_Draw.DrawLine(screenCenterX - lineSize, screenCenterY - lineSize, screenCenterX - (lineSize / 4), screenCenterY - (lineSize / 4), Color(200, 200, 200, 255));
		g_Draw.DrawLine(screenCenterX - lineSize, screenCenterY + lineSize, screenCenterX - (lineSize / 4), screenCenterY + (lineSize / 4), Color(200, 200, 200, 255));
		g_Draw.DrawLine(screenCenterX + lineSize, screenCenterY + lineSize, screenCenterX + (lineSize / 4), screenCenterY + (lineSize / 4), Color(200, 200, 200, 255));
		g_Draw.DrawLine(screenCenterX + lineSize, screenCenterY - lineSize, screenCenterX + (lineSize / 4), screenCenterY - (lineSize / 4), Color(200, 200, 200, 255));
	}
#else
	if (!variable::get().visuals.b_hitmarker)
		return;

	const auto size = 8;
	if (_iHurtTime + 250 >= (int)GetTickCount())
	{
		const ImVec2 center = ImVec2(render::get().get_viewport().Width / 2, render::get().get_viewport().Height / 2);

		render::get().add_line(ImVec2(center.x - size, center.y - size), ImVec2(center.x - (size / 4), center.y - (size / 4)), Color::White().ToImGUI());
		render::get().add_line(ImVec2(center.x - size, center.y + size), ImVec2(center.x - (size / 4), center.y + (size / 4)), Color::White().ToImGUI());
		render::get().add_line(ImVec2(center.x + size, center.y + size), ImVec2(center.x + (size / 4), center.y + (size / 4)), Color::White().ToImGUI());
		render::get().add_line(ImVec2(center.x + size, center.y - size), ImVec2(center.x + (size / 4), center.y - (size / 4)), Color::White().ToImGUI());
	}
#endif
}

void CVisuals::DrawIndicators()
{
#ifdef IMI_MENU
	// we're alive, draw stuff
	if (LocalPlayer.IsAlive)
	{
		int y = 60;

		if (g_Convars.AntiAim.antiaim_enable->GetBool() && g_Convars.AntiAim.antiaim_desync->GetBool())
		{
#ifndef IMI_MENU
#ifdef _DEBUG
			render::get().add_text(ImVec2())
				g_renderer.draw_text(color_green(), font_baksheesh12, ImVec2(50.f, g_client.m_screenHeight * 0.5f - 80.f), Renderer::LEFT_ALIGN, "unchoked: %f", real);
			g_renderer.draw_text(color_red(), font_baksheesh12, ImVec2(50.f, g_client.m_screenHeight * 0.5f - 60.f), Renderer::LEFT_ALIGN, "choked: %f", fake);
			/*char txt[64];
			sprintf(txt, "Real: %.2f", g_Info.m_flRealYaw);
			g_Draw.Text(10, g_Info.ScreenSize.Height - y, 255, 0, 0, 255, CSurfaceDraw::TextAlign::LEFT, Fonts::LBY, txt);

			y += 20;

			sprintf(txt, "Fake: %.2f", g_Info.m_flFakeYaw);
			g_Draw.Text(10, g_Info.ScreenSize.Height - y, 0, 255, 0, 255, CSurfaceDraw::TextAlign::LEFT, Fonts::LBY, txt);

			y += 20;

			sprintf(txt, "LBY: %.2f", LocalPlayer.LowerBodyYaw);
			g_Draw.Text(10, g_Info.ScreenSize.Height - y, 0, 0, 255, 255, CSurfaceDraw::TextAlign::LEFT, Fonts::LBY, txt);

			y += 20;

			//clear the formatted string
			memset(txt, 0, 64);*/
#endif
#endif

			if (LocalPlayer.m_bDesynced)
			{
				const float flBoxes = std::ceil(util::clamp(LocalPlayer.m_flDesynced, 0.f, 60.f) / 6.f);
				float flMultiplier = 12 / 360.f;
				flMultiplier *= flBoxes - 1;
				Color ColHealth = Color::FromHSB(flMultiplier, 1, 1);

#ifdef _DEBUG
				float newdelta = AngleNormalize(AngleDiff(LocalPlayer.real_playerbackup.OriginalAbsAngles.y, LocalPlayer.fake_playerbackup.OriginalAbsAngles.y));
#define NUM_DELTAS_TO_STORE 3
				static float deltas[NUM_DELTAS_TO_STORE]{};
				deltas[Interfaces::Globals->tickcount % NUM_DELTAS_TO_STORE] = newdelta;
				float avg = 0.0f;
				for (auto dt : deltas)
					avg += dt;
				avg /= NUM_DELTAS_TO_STORE;

				static float lastavg = avg;
				//if (fabsf(avg - lastavg) < 0.5f)
				//	avg = lastavg;
				//lastavg = avg;

				g_Draw.Text(10, g_Info.ScreenSize.Height - y, ColHealth.r(), ColHealth.g(), ColHealth.b(), 255, CSurfaceDraw::TextAlign::LEFT, Fonts::LBY, "DESYNC %.1f", newdelta);
#else
				g_Draw.Text(10, g_Info.ScreenSize.Height - y, ColHealth.r(), ColHealth.g(), ColHealth.b(), 255, CSurfaceDraw::TextAlign::LEFT, Fonts::LBY, "DESYNC");
#endif
			}
			else
				g_Draw.Text(10, g_Info.ScreenSize.Height - y, 255, 0, 0, 255, CSurfaceDraw::TextAlign::LEFT, Fonts::LBY, "DESYNC");

			y += 20;

#ifdef _DEBUG
			g_Draw.Text(10, g_Info.ScreenSize.Height - y, 255, 255, 255, 255, CSurfaceDraw::TextAlign::LEFT, Fonts::LBY, "LBY %.1f", LocalPlayer.Entity->GetLowerBodyYaw());
			y += 20;
#endif
		}
		if (g_Convars.HVH.hvh_fakelatency->GetBool())
		{
			const float flBoxes = std::ceil(g_Convars.HVH.hvh_fakelatency_amount->GetInt() / 80.f);
			float flMultiplier = 12 / 360.f;
			flMultiplier *= flBoxes - 1;
			Color ColHealth = Color::FromHSB(0.3 - flMultiplier, 1, 1);

			g_Draw.Text(10, g_Info.ScreenSize.Height - y, ColHealth.r(), ColHealth.g(), ColHealth.b(), 255, CSurfaceDraw::TextAlign::LEFT, Fonts::LBY, "PING");

			y += 20;
		}
		if (g_Convars.HVH.hvh_fakelag->GetBool())
		{
			if (LocalPlayer.Entity->GetVelocity().Length() > 250.f)
			{
				if (LocalPlayer.Current_Origin.DistToSqr(LocalPlayer.Previous_Origin) > 4096.f)
					g_Draw.Text(10, g_Info.ScreenSize.Height - y, 0, 255, 0, 255, CSurfaceDraw::TextAlign::LEFT, Fonts::LBY, "LC");
				else
					g_Draw.Text(10, g_Info.ScreenSize.Height - y, 255, 0, 0, 255, CSurfaceDraw::TextAlign::LEFT, Fonts::LBY, "LC");
			}
		}
	}
#else

	auto &var = variable::get();

	if (!LocalPlayer.IsAlive)
	{
		DrawAntiMedia();
		return;
	}

	bool only_active = var.visuals.b_indicators_only_active;

	std::vector< SideItem_t > indicators;

	// don't draw these if the chat is open
	//decrypts(0)
	const auto hud_chat = (CCSGO_HudChat*)Interfaces::Hud->FindElement(XorStr("CCSGO_HudChat"));
	//encrypts(0)
	if (hud_chat && hud_chat->m_is_open)
	{
		DrawAntiMedia();
		return;
	}

#if defined _DEBUG || defined INTERNAL_DEBUG
	if (LocalPlayer.Entity)
	{
		auto animstate = LocalPlayer.Entity->GetPlayerAnimState();
		float move_yaw = LocalPlayer.Entity->GetPoseParameterUnscaled(7);
		if (animstate)
		{
			//indicators.emplace_back(std::to_string(animstate->m_flGoalFeetYaw), Color::White().ToImGUI());
			//indicators.emplace_back(std::to_string(move_yaw), Color::Cyan().ToImGUI());

			indicators.emplace_back(std::to_string(g_Info.m_flUnchokedYaw), Color::White().ToImGUI());
			indicators.emplace_back(std::to_string(g_Info.m_flChokedYaw), Color::Cyan().ToImGUI());

			//if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
			//	printf("unchoked: %f | choked: %f\n", g_Info.m_flUnchokedYaw, g_Info.m_flChokedYaw);

			//if(animstate->m_flSpeed > MINIMUM_PLAYERSPEED_CONSIDERED_MOVING)
			//	printf("actual gfy: %2.1f | prev gfy: %2.1f\n", animstate->m_flGoalFeetYaw, g_Visuals.test_gfy);
		}

		indicators.emplace_back(std::to_string(weapon_spread), Color::White().ToImGUI());
	}
#endif

	// forwardtrack indicator
#if defined _DEBUG || defined INTERNAL_DEBUG

	//decrypts(0)
	indicators.emplace_back(XorStr("FORWARDTRACK"), g_Info.UsingForwardTrack ? Color::Green().ToImGUI() : Color::Red().ToImGUI());
	//encrypts(0)
#endif

	// desync indicator
	if (var.visuals.b_indicator_desync)
	{
		// todo: check for antiaim and desync is enabled
		if (var.ragebot.b_antiaim && LocalPlayer.Config_IsDesyncing())
		{
			if (!only_active || (only_active && LocalPlayer.m_bDesynced))
			{
				const float step = std::ceil(adr_math::my_super_clamp(LocalPlayer.m_flDesynced, 0.f, 60.f) / 6.f);
				float hue = 12 / 360.f;
				hue *= step - 1;
				Color desync_color = Color::FromHSB(hue, 1, 1);

				//decrypts(0)
				std::string info = XorStr("DESYNC");
				//encrypts(0)
				indicators.emplace_back(info, desync_color.ToImGUI());
			}
		}
	}

	// 'HEAD' indicator
	if (var.visuals.b_indicator_priority)
	{
		if (var.ragebot.b_enabled)
		{
			if (!only_active || (only_active && var.ragebot.baim_main.b_force.get() || !m_prioritize_body))
			{
				if (var.ragebot.baim_main.b_force.get())
				{
					//decrypts(0)
					indicators.emplace_back(XorStr("FORCE BAIM"), Color::Green().ToImGUI());
					//encrypts(0)
				}
				else
				{
					//decrypts(0)
					indicators.emplace_back(XorStr("HEAD"), !m_prioritize_body ? Color::Green().ToImGUI() : Color::Red().ToImGUI());
					//encrypts(0)
				}
			}
		}
	}

	// lag comp indicator
	if (var.visuals.b_indicator_lc)
	{
		if (LocalPlayer.Config_IsFakelagging())
		{
			bool breaking_lc = LocalPlayer.Entity->GetVelocity().Length() > 250.f && LocalPlayer.Current_Origin.DistToSqr(LocalPlayer.Previous_Origin) > 4096.f;
			if (!only_active || (only_active && breaking_lc))
			{
				//decrypts(0)
				indicators.emplace_back(XorStr("LC"), breaking_lc ? Color::Green().ToImGUI() : Color::Red().ToImGUI());
				//encrypts(0)
			}
		}
	}

	// choke indicator
	if (var.visuals.b_indicator_choke)
	{
		const int ticks = last_ticks_choked;
		if (!only_active || (only_active && ticks > 1))
		{
			//decrypts(0)
			std::string info = XorStr("CHOKE: ");
			//encrypts(0)
			info += std::to_string(ticks);
			indicators.emplace_back(info, Color::White().ToImGUI());
		}
	}

	// lean dir indicator
	if (var.visuals.b_indicator_lean_dir)
	{
		if (var.ragebot.b_antiaim && LocalPlayer.Config_IsDesyncing())
		{
			//decrypts(0)

			bool dir = LocalPlayer.GetDesyncStyle();

			std::string info = XorStr("LEAN - ");
			info += dir ? XorStr("LEFT") : XorStr("RIGHT");

			if (g_AntiAim.m_bManualSwapDesyncDir)
				info += XorStr(" (SWAPPED)");

			indicators.emplace_back(info, Color::Grey().ToImGUI());

			//encrypts(0)			
		}
	}

	// edge/freestanding indicator
	if (var.visuals.b_indicator_edge)
	{
		if (var.ragebot.b_antiaim)
		{
			if (g_AntiAim.m_iFreestandType == FREESTAND_NONE)
			{
				// manual aa
				//decrypts(0)
				if (g_AntiAim.m_iManualType == MANUAL_LEFT)
				{
					indicators.emplace_back(XorStr("MANUAL - LEFT"), ImColor(191, 85, 236));
				}
				else if (g_AntiAim.m_iManualType == MANUAL_RIGHT)
				{
					indicators.emplace_back(XorStr("MANUAL - RIGHT"), ImColor(191, 85, 236));
				}
				else if (g_AntiAim.m_iManualType == MANUAL_BACK)
				{
					indicators.emplace_back(XorStr("MANUAL - BACK"), ImColor(191, 85, 236));
				}
				//encrypts(0)
			}
			else
			{
				//decrypts(0)
				if (g_AntiAim.m_iFreestandType == FREESTAND_LEFT)
				{
					indicators.emplace_back(XorStr("FREESTAND - LEFT"), ImColor(191, 85, 236));
				}
				else if (g_AntiAim.m_iFreestandType == FREESTAND_RIGHT)
				{
					indicators.emplace_back(XorStr("FREESTAND - RIGHT"), ImColor(191, 85, 236));
				}
				else if (g_AntiAim.m_iFreestandType == FREESTAND_BACK)
				{
					indicators.emplace_back(XorStr("FREESTAND - BACK"), ImColor(191, 85, 236));
				}
				//encrypts(0)
			}
		}
	}

	// min-walk indicator
	if (var.visuals.b_indicator_minwalk)
	{
		if (!only_active || (only_active && LocalPlayer.bFakeWalking))
		{
			//decrypts(0)
			indicators.emplace_back(XorStr("MINWALK"), LocalPlayer.bFakeWalking ? Color::Green().ToImGUI() : Color::Red().ToImGUI());
			//encrypts(0)
		}
	}

	// fake-duck indicator
	if (var.visuals.b_indicator_fakeduck)
	{
		if (!only_active || (only_active && LocalPlayer.IsFakeDucking))
		{
			//decrypts(0)
			indicators.emplace_back(XorStr("FAKEDUCK"), LocalPlayer.IsFakeDucking ? Color::Green().ToImGUI() : Color::Red().ToImGUI());
			//encrypts(0)
		}
	}

	// fake-latency indicator
	if (var.visuals.b_indicator_fake_latency && var.ragebot.f_fake_latency > 0.f)
	{
		const float step = std::ceil(var.ragebot.f_fake_latency / 80.f);
		float hue = 12 / 360.f;
		hue *= step - 1;
		Color ping_color = Color::FromHSB(0.3 - hue, 1, 1);

		//decrypts(0)
		indicators.emplace_back(XorStr("FAKELAT"), ping_color.ToImGUI());
		//encrypts(0)
	}

	// tickbase shift indicator
	if (LocalPlayer.IsAllowedUntrusted() && var.visuals.b_indicator_tickbase)
	{
		if (var.ragebot.exploits.b_hide_record || var.ragebot.exploits.b_hide_shots || var.ragebot.exploits.b_multi_tap.get() || var.ragebot.exploits.b_nasa_walk.get())
		{
			bool tickbase_ready = g_Tickbase.m_bReadyToShiftTickbase && !LocalPlayer.WaitForTickbaseBeforeFiring;

			std::stringstream ss;
#if defined _DEBUG || defined INTERNAL_DEBUG
			ss << XorStr("TICKBASE: ") << g_Tickbase.m_iTicksAllowedForProcessing << XorStr(" A | ") << g_Tickbase.m_iNumFakeCommandsToSend << XorStr(" S");
#else
			//decrypts(0)
			ss << XorStr("SHIFT");
			//encrypts(0)
#endif
			if (!only_active || (only_active && tickbase_ready))
			{
				indicators.emplace_back(ss.str().data(), tickbase_ready ? Color::Green().ToImGUI() : Color::Red().ToImGUI());
			}
		}
	}

	// double tap indicator
	if (LocalPlayer.IsAllowedUntrusted() && var.visuals.b_indicator_double_tap && var.ragebot.exploits.b_multi_tap.get())
	{
		bool is_multi_tapping = (g_Tickbase.m_bReadyToShiftTickbase && Interfaces::Globals->realtime >= g_Tickbase.m_flDelayTickbaseShiftUntilThisTime);
		if (!only_active || only_active && is_multi_tapping)
		{
			//decrypts(0)
			indicators.emplace_back(XorStr("MULTITAP"), is_multi_tapping ? Color::Green().ToImGUI() : Color::Red().ToImGUI());
			//encrypts(0)
		}
	}
	// teleport indicator
	if (LocalPlayer.IsAllowedUntrusted() && var.visuals.b_indicator_teleport && var.ragebot.exploits.b_nasa_walk.get())
	{
		bool is_able_to_teleport = (g_Tickbase.m_bReadyToShiftTickbase && Interfaces::Globals->realtime >= g_Tickbase.m_flDelayTickbaseShiftUntilThisTime && g_Tickbase.m_iTicksAllowedForProcessing >= g_Tickbase.m_iNumFakeCommandsToSend);
		if (!only_active || only_active && is_able_to_teleport)
		{
			//decrypts(0)
			indicators.emplace_back(XorStr("TELEPORT"), is_able_to_teleport ? Color::Green().ToImGUI() : Color::Red().ToImGUI());
			//encrypts(0)
		}
	}

	auto viewport = render::get().get_viewport();

	// fix resolution dissipation
	float indicator_y = 0.f;
	if (viewport.Height > 1080)
		indicator_y = INDICATOR_Y_1080;
	else if (viewport.Height >= 900)
		indicator_y = INDICATOR_Y_900;
	else
		indicator_y = INDICATOR_Y;

	// render the indicators
	if (var.visuals.b_indicators)
	{
		for (auto i = 0; i < indicators.size(); ++i)
		{
			auto indicator = indicators[i];
			render::get().add_text(ImVec2(30.f, viewport.Height - (indicator_y + (i * 17.f))), indicator.color, DROP_SHADOW, BAKSHEESH_18, indicator.info.data());
		}
	}

	// draw antimedia
	DrawAntiMedia();

#endif
}

void CVisuals::DrawAntiMedia()
{
	// anti-media indicators
	std::vector< SideItem_t > antimedia_indicators;

	// todo; nit add manual fire, resolver override, bad server

	if (variable::get().ragebot.b_enabled && !variable::get().ragebot.b_resolver)
	{
		//decrypts(0)
		antimedia_indicators.emplace_back(XorStr("NO RESOLVER"), Color::Red().ToImGUI());
		//encrypts(0)
	}

	// low fps indicator
	if (ImGui::GetIO().Framerate < (1.f / Interfaces::Globals->interval_per_tick))
	{
		//decrypts(0)
		antimedia_indicators.emplace_back(XorStr("FPS < TICKRATE"), Color::Red().ToImGUI());
		//encrypts(0)
	}

	// low hc indicator
	// todo; nit - make a map of < short, float > ( item id, hc ) of low values
	if (variable::get().ragebot.b_enabled && !variable::get().legitbot.aim.b_enabled.b_state)
	{
		// no hitboxes enabled for ragebot
		bool has_body = false;
		bool has_head = false;
		if (!variable::get().ragebot.is_hitscanning(&has_body, &has_head))
		{
			//decrypts(0)
			antimedia_indicators.emplace_back(XorStr("NO HITBOXES"), Color::Red().ToImGUI());
			//encrypts(0)
		}
		else
		{
			// check to see if baim is enabled and no baim-based hitboxes are enabled
			if (!has_body && variable::get().ragebot.baim_main.can_baim())
			{
				//decrypts(0)
				antimedia_indicators.emplace_back(XorStr("NO BODY HITBOXES"), Color::Red().ToImGUI());
				//encrypts(0)
			}
		}

		if (weapon_accuracy_nospread.GetVar()->GetInt() < 1)
		{
			if (variable::get().ragebot.f_hitchance < 20.f)
			{
				//decrypts(0)
				antimedia_indicators.emplace_back(XorStr("LOW HITCHANCE"), Color::Red().ToImGUI());
				//encrypts(0)
			}
		}

		// low mindmg indicator
		// todo; nit - make a map of < short, float > ( item id, mindmg ) of low values
		if (variable::get().ragebot.i_mindmg < 10)
		{
			//decrypts(0)
			antimedia_indicators.emplace_back(XorStr("LOW MINDMG"), Color::Red().ToImGUI());
			//encrypts(0)
		}
		if (variable::get().ragebot.i_mindmg_aw < 10)
		{
			//decrypts(0)
			antimedia_indicators.emplace_back(XorStr("LOW AW MINDMG"), Color::Red().ToImGUI());
			//encrypts(0)
		}
	}

	auto viewport = render::get().get_viewport();

	// fix resolution dissipation
	float indicator_y = 0.f;
	if (viewport.Height > 1080)
		indicator_y = INDICATOR_Y_1080;
	else if (viewport.Height >= 900)
		indicator_y = INDICATOR_Y_900;
	else
		indicator_y = INDICATOR_Y;

	// render the anti-media indicators
	for (auto i = 0; i < antimedia_indicators.size(); ++i)
	{
		auto indicator = antimedia_indicators[i];
		render::get().add_text(ImVec2(viewport.Width - 150.f, viewport.Height - (indicator_y + (i * 17.f))), indicator.color, DROP_SHADOW, BAKSHEESH_18, indicator.info.data());
	}
}

void CVisuals::DrawBulletTracers()
{
#if 0
	// we want to draw bullettracers
	if (false)
	{
		// init locals
		Color color;

		// loop through all tracers
		for (size_t i = 0; i < m_BulletTracers.size(); ++i)
		{
			// get bullettracer data
			auto data = m_BulletTracers[i];

			// build beam
			BeamInfo_t beam;
			//decrypts(0)
			beam.m_pszModelName = XorStr("sprites/physbeam.vmt");
			//encrypts(0)
			beam.modelindex = Interfaces::ModelInfoClient->GetModelIndex(beam.m_pszModelName);
			beam.m_bRenderable = true;
			beam.m_flBrightness = 255.f;
			beam.m_vecStart = data.m_vecStart;
			beam.m_vecEnd = data.m_vecStop;
			beam.m_nSegments = 2;

			// set beam color
			if (data.m_iTeamID == 2)
				color.SetColor(255, 0, 0);
			else
				color.SetColor(0, 192, 255);

			// set color, life and size
			beam.m_flRed = color.r();
			beam.m_flGreen = color.g();
			beam.m_flBlue = color.b();
			beam.m_flWidth = 2.f;
			beam.m_flEndWidth = 2.f;
			beam.m_flLife = 2.f;

			// create beam
			auto beamDraw = Interfaces::Beams->CreateBeamPoints(beam);

			// draw beam
			if (beamDraw)
				Interfaces::Beams->DrawBeam(beamDraw);
	}
}

	// clear bullet tracers
	m_BulletTracers.clear();
#endif
}

void CVisuals::DrawSpreadCircle()
{
#ifdef IMI_MENU
	// feature disabled
	if (!g_Convars.Visuals.visuals_spreadcircle->GetBool())
		return;

	auto _weapon = LocalPlayer.Entity->GetWeapon();

	// alive and well
	if (LocalPlayer.Entity && LocalPlayer.IsAlive && _weapon && _weapon->IsGun())
	{
		// get accuracy
		const auto accuracy = (_weapon->GetWeaponSpread() + _weapon->GetInaccuracy()) * 500;

		// draw circle
		g_Draw.DrawCircle(g_Info.ScreenSize.Width / 2, g_Info.ScreenSize.Height / 2, accuracy, g_Convars.Colors.color_spreadcircle->GetColor(), g_Convars.Visuals.visuals_spreadcircle_alpha->GetInt());
	}
#else
	if (!variable::get().visuals.b_spread_circle || !LocalPlayer.Entity)
		return;

	auto weapon = LocalPlayer.Entity->GetWeapon();

	if (LocalPlayer.IsAlive && weapon && !weapon->IsKnife() && !weapon->IsGrenade() && !weapon->IsBomb())
	{
		weapon_spread = weapon->GetWeaponSpread() + weapon->GetInaccuracy();
		const auto acc = weapon->GetWeaponSpread() + weapon->GetInaccuracy() * 500.f;
		render::get().add_circle_filled(ImVec2(render::get().get_viewport().Width / 2, render::get().get_viewport().Height / 2), acc, variable::get().visuals.col_spread_circle.color().ToImGUI(), 30);
	}
#endif
}

void CVisuals::DrawScopeLines()
{
	if (!variable::get().visuals.b_scope_lines)
		return;

	if (LocalPlayer.Entity && LocalPlayer.IsAlive && LocalPlayer.Entity->IsScoped())
	{
		auto viewport = render::get().get_viewport();
		render::get().add_line(ImVec2(viewport.Width / 2, 0), ImVec2(viewport.Width / 2, viewport.Height), variable::get().visuals.col_scope_lines.color().ToImGUI());
		render::get().add_line(ImVec2(0, viewport.Height / 2), ImVec2(viewport.Width, viewport.Height / 2), variable::get().visuals.col_scope_lines.color().ToImGUI());
	}
}

void CVisuals::DrawAutowallCrosshair()
{
	if (variable::get().visuals.i_autowall_xhair <= 0 || !LocalPlayer.WeaponVars.IsGun)
		return;

	ImColor color = m_autowallHit ? Color::Green().ToImGUI() : Color::Red().ToImGUI();
	color.Value.w = 127.5f;

	auto viewport = render::get().get_viewport();

	// draw box
	render::get().add_rect_filled(ImVec2((viewport.Width / 2), (viewport.Height / 2)), 2.f, 2.f, color);

#if defined _DEBUG || defined INTERNAL_DEBUG
	if (variable::get().visuals.i_autowall_xhair == 2)
	{
		std::stringstream dmg;
		for (auto i = 0; i < m_autowallDamagePerWall.size(); ++i)
		{
			dmg << i << " : " << m_autowallDamagePerWall[i] << " DMG";
			if (i != m_autowallDamagePerWall.size() - 1)
				dmg << " | ";
		}

		render::get().add_text(ImVec2(viewport.Width / 2, (viewport.Height / 2) - 10.f), Color::White().ToImGUI(), CENTERED_X | CENTERED_Y | DROP_SHADOW, BAKSHEESH_12, dmg.str().data());

		if (m_autowallHitPlayer)
		{
			//decrypts(0)
			render::get().add_text(ImVec2(viewport.Width / 2, (viewport.Height / 2) + 10.f), Color::White().ToImGUI(), CENTERED_X | CENTERED_Y | DROP_SHADOW, BAKSHEESH_12, XorStr("hitgroup: %d | hitbox: %d"), m_autowallHitgroup, m_autowallHitbox);
			//encrypts(0)
		}
	}
#else
	// draw damage
	if (variable::get().visuals.i_autowall_xhair == 2 && m_autowallHit && m_autowallDamage > 0)
	{
		std::stringstream dmg;
		Color dmg_color;

		//decrypts(0)
		dmg << m_autowallDamage << XorStr(" DMG");
		//encrypts(0)
		dmg_color = adr_util::get_health_color(m_autowallDamage);

		render::get().add_text(ImVec2(viewport.Width / 2, (viewport.Height / 2) - 10.f), dmg_color.ToImGUI(), CENTERED_X | CENTERED_Y | DROP_SHADOW, BAKSHEESH_12, dmg.str().data());
	}
#endif
}

void CAM_ToThirdPerson()
{
	StaticOffsets.GetOffsetValueByType<void(*)(void)>(_CAM_ToThirdPerson)();
}

void CAM_ToFirstPerson()
{
	StaticOffsets.GetOffsetValueByType<void(*)(void)>(_CAM_ToFirstPerson)();
}

void CVisuals::SpectateAll()
{
	//decrypts(0)
	static ConVar* mp_forcecamera = Interfaces::Cvar->FindVar(XorStr("mp_forcecamera"));
	//encrypts(0)
	mp_forcecamera->nFlags &= ~(FCVAR_PROTECTED | FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_NOT_CONNECTED | FCVAR_SERVER_CAN_EXECUTE | FCVAR_CLIENTCMD_CAN_EXECUTE | FCVAR_ARCHIVE);

	if (variable::get().misc.thirdperson.b_enabled.get() && !LocalPlayer.IsAlive)
	{
		if (last_forcecam != 0)
		{
			mp_forcecamera->SetValue(0);
			last_forcecam = 0;
			return;
		}
	}

	if (!variable::get().misc.b_spectate_all)
	{
		if (last_forcecam != 1)
		{
			mp_forcecamera->SetValue(1);
			last_forcecam = 1;
		}
		return;
	}

	if (last_forcecam != 0)
	{
		mp_forcecamera->SetValue(0);
		last_forcecam = 0;
	}
}

float GetCorrectCameraDistance(CViewSetup* pSetup, const float config_dist)
{
	const auto f_factor = clamp(config_dist, 25.f, 200.f);
	auto inverse_angles = pSetup->angles;

	//SDK::Interfaces::Engine()->GetViewAngles();
	inverse_angles.x *= -1.f;
	inverse_angles.y += 180.f;

	Vector forward;
	AngleVectors(inverse_angles, &forward);

	CTraceFilterWorldOnly filter;
	trace_t trace;
	Ray_t ray;

	ray.Init(LocalPlayer.Entity->GetEyePosition(), LocalPlayer.Entity->GetEyePosition() + forward * f_factor);
	Interfaces::EngineTrace->TraceRay(ray, MASK_ALL, &filter, &trace);

	return f_factor * trace.fraction - 10.f;
}

void CVisuals::ThirdPerson(CViewSetup *pSetup)
{
	if (ui::get().is_visible() || !LocalPlayer.Entity)
		return;
#if 0
	//decrypts(0)
	static ConVar* sv_allow_third_person = Interfaces::Cvar->FindVar(XorStr("sv_allow_thirdperson"));
	static ConVar* sv_cheats = Interfaces::Cvar->FindVar(XorStr("sv_cheats"));
	static ConVar* cam_idealdist = Interfaces::Cvar->FindVar(XorStr("cam_idealdist"));
	static ConVar* cam_collision = Interfaces::Cvar->FindVar(XorStr("cam_collision"));
	static ConVar* cam_idealpitch = Interfaces::Cvar->FindVar(XorStr("cam_idealpitch"));
	static ConVar* cam_idealyaw = Interfaces::Cvar->FindVar(XorStr("cam_idealyaw"));
	static ConVar* cam_snapto = Interfaces::Cvar->FindVar(XorStr("cam_snapto"));
	static ConVar* cam_ideallag = Interfaces::Cvar->FindVar(XorStr("cam_ideallag"));

	sv_allow_third_person->nFlags &= ~(FCVAR_PROTECTED | FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_NOT_CONNECTED | FCVAR_SERVER_CAN_EXECUTE | FCVAR_CLIENTCMD_CAN_EXECUTE);
	sv_allow_third_person->SetValue(1);

	cam_idealdist->nFlags &= ~(FCVAR_PROTECTED | FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_NOT_CONNECTED | FCVAR_SERVER_CAN_EXECUTE | FCVAR_CLIENTCMD_CAN_EXECUTE | FCVAR_ARCHIVE);
	cam_collision->nFlags &= ~(FCVAR_SERVER_CAN_EXECUTE | FCVAR_ARCHIVE);
	cam_collision->SetValue(1);
	cam_idealpitch->nFlags &= ~(FCVAR_SERVER_CAN_EXECUTE | FCVAR_ARCHIVE);
	cam_idealpitch->SetValue(0.f);
	cam_idealyaw->nFlags &= ~(FCVAR_SERVER_CAN_EXECUTE | FCVAR_ARCHIVE);
	cam_idealyaw->SetValue(0.f);
	cam_snapto->nFlags &= ~(FCVAR_SERVER_CAN_EXECUTE | FCVAR_ARCHIVE);
	cam_snapto->SetValue(0);
	cam_ideallag->nFlags &= ~(FCVAR_SERVER_CAN_EXECUTE | FCVAR_ARCHIVE);
	cam_ideallag->SetValue(0.f);
	//encrypts(0)

	QAngle angles;
	Interfaces::EngineClient->GetViewAngles(angles);

	//if (LocalPlayer.Entity)
	//{
	//	if (LocalPlayer.IsAlive)
	//		Interfaces::EngineClient->GetViewAngles(angles);
	//	else
	//	{
	//		if(LocalPlayer.Entity->GetObserverTarget())
	//			angles = LocalPlayer.Entity->GetObserverTarget()->GetEyeAngles();
	//	}
	//}

	const int bThirdPerson = Interfaces::Input->CAM_IsThirdPerson();

	// we're playing
	if (LocalPlayer.IsAlive)
	{
		if (variable::get().misc.thirdperson.b_enabled.get())
		{
			// todo: nit; fix this logic later, this is good enough, I hate gotos, but everything is initialized before it, so fuck it.
			if (LocalPlayer.WeaponVars.IsGrenade && variable::get().misc.thirdperson.b_disable_on_nade)
			{
				if (bThirdPerson)
					CAM_ToFirstPerson();

				return;
			}

			if (!bThirdPerson)
				CAM_ToThirdPerson();

			//Interfaces::Input->m_fCameraInThirdPerson = true;

			cam_idealdist->SetValue(variable::get().misc.thirdperson.f_distance);

#if 0
			Interfaces::Input->m_vecCameraOffset = Vector(angles.x, angles.y, variable::get().misc.thirdperson.f_distance);

			Vector camOffset = Interfaces::Input->m_vecCameraOffset;

			Vector camForward;
			AngleVectors(QAngle(camOffset.x, camOffset.y, 0.f), &camForward, nullptr, nullptr);

			Vector CAM_HULL_MIN(-CAM_HULL_OFFSET, -CAM_HULL_OFFSET, -CAM_HULL_OFFSET);
			Vector CAM_HULL_MAX(CAM_HULL_OFFSET, CAM_HULL_OFFSET, CAM_HULL_OFFSET);

			CGameTrace trace;
			CTraceFilterWorldOnly traceFilter;
			Ray_t ray;

			Vector position = LocalPlayer.EyePosition;

			ray.Init(position, position - (camForward * camOffset.z), CAM_HULL_MIN, CAM_HULL_MAX);

			Interfaces::EngineTrace->TraceRay(ray, MASK_SOLID, &traceFilter, &trace);

			if (trace.fraction < 1.0)
				camOffset.z *= trace.fraction;

			Interfaces::Input->m_vecCameraOffset = camOffset;
#endif
		}
		else
		{
			if (bThirdPerson)
				CAM_ToFirstPerson();

			//Interfaces::Input->m_fCameraInThirdPerson = false;
		}
	}
	else
	{
		CAM_ToFirstPerson();
	}

	//Stop wireframe bug in third person
	//*s_bOverridePostProcessingDisable = Interfaces::Input->m_fCameraInThirdPerson ? variable::get().visuals.b_no_scope : 0;
#endif

	auto &var = variable::get();

	if (variable::get().misc.thirdperson.b_enabled.get())
	{
		if (LocalPlayer.Entity->GetAlive())
		{
			if (var.misc.thirdperson.b_disable_on_nade && LocalPlayer.WeaponVars.IsGrenade)
			{
				if (Interfaces::Input->m_fCameraInThirdPerson)
					Interfaces::Input->m_fCameraInThirdPerson = false;
			}
			else
			{
				if (!Interfaces::Input->m_fCameraInThirdPerson)
					Interfaces::Input->m_fCameraInThirdPerson = true;

				Interfaces::Input->m_vecCameraOffset.z = GetCorrectCameraDistance(pSetup, var.misc.thirdperson.f_distance);
			}

			if (m_tp_wasDeadInThirdperson)
				m_tp_wasDeadInThirdperson = false;
		}
		else
		{
			if (Interfaces::Input->m_fCameraInThirdPerson)
				Interfaces::Input->m_fCameraInThirdPerson = false;

			if (!m_tp_wasDeadInThirdperson)
			{
				LocalPlayer.Entity->SetObserverMode(OBS_MODE_CHASE);
				m_tp_wasDeadInThirdperson = true;
			}
		}
	}
	else
	{
		if (Interfaces::Input->m_fCameraInThirdPerson)
			Interfaces::Input->m_fCameraInThirdPerson = false;

		if (m_tp_wasDeadInThirdperson)
		{
			LocalPlayer.Entity->SetObserverMode(OBS_MODE_IN_EYE);
			m_tp_wasDeadInThirdperson = false;
		}

		auto i_current_mode = LocalPlayer.Entity->GetObserverMode();
		if (m_tp_oldState != i_current_mode)
			m_tp_oldState = i_current_mode;
	}
}

void CVisuals::FOVChanger(CViewSetup* pSetup)
{
	// invalid y0
	if (!pSetup || !Interfaces::EngineClient->IsInGame() || !Interfaces::EngineClient->IsConnected())
		return;

	// reset to default
	//pSetup->fov = 90.f;

#ifdef IMI_MENU
	// fovchanger is enabled
	if (g_Convars.Visuals.misc_fovchanger->GetBool())
		// add value to current FOV
		pSetup->fov += g_Convars.Visuals.misc_fovchanger_value->GetInt();
#else

	LocalPlayer.Get(&LocalPlayer);

	pSetup->fov = variable::get().misc.thirdperson.b_enabled.b_state ? variable::get().visuals.f_thirdperson_fov : variable::get().visuals.f_fov;

	// remove zoom
	if (!variable::get().visuals.b_no_zoom)
	{
		CBaseCombatWeapon* Weapon;
		if (LocalPlayer.Entity && (Weapon = LocalPlayer.Entity->GetWeapon()))
		{
			if (LocalPlayer.Entity->IsScoped())
			{
				const int zoom_fov_1 = Weapon->GetZoomFOV(1);
				const int zoom_fov_2 = Weapon->GetZoomFOV(2);

				switch (Weapon->GetZoomLevel())
				{
				case 1:
				{
					pSetup->fov = static_cast<float>(zoom_fov_1);
					break;
				}
				case 2:
				{
					pSetup->fov = static_cast<float>(zoom_fov_2);
					break;
				}
				default:
					break;
				}
			}
		}
	}

#endif
}

void CVisuals::DrawAntiAim()
{
#ifdef IMI_MENU
	if (!g_Convars.AntiAim.antiaim_enable->GetBool() || !g_Convars.Visuals.antiaim_arrows->GetBool() || !LocalPlayer.IsAlive)
		return;
	if (Interfaces::Input->CAM_IsThirdPerson())
		g_Draw.angle_direction_thirdperson();
	else
		g_Draw.angle_direction_firstperson();
#else
	if (!variable::get().ragebot.b_enabled || !variable::get().ragebot.b_antiaim || !LocalPlayer.Config_IsDesyncing() || !variable::get().visuals.b_show_antiaim_angles || !LocalPlayer.IsAlive)
		return;

	if (Interfaces::Input->CAM_IsThirdPerson())
	{
		Vector start = *LocalPlayer.Entity->GetAbsOrigin();
		Vector end, forward, start_screen, end_screen;

		// real angle
		if (variable::get().visuals.b_show_antiaim_real)
		{
			AngleVectors(QAngle(0.f, g_Info.m_flUnchokedYaw, 0.f), &forward);
			end = start + (forward * 50.f);

			if (!WorldToScreen(start, start_screen) || !WorldToScreen(end, end_screen))
				return;

			render::get().add_line(ImVec2(start_screen.x, start_screen.y), ImVec2(end_screen.x, end_screen.y), Color::Red().ToImGUI());
			//decrypts(0)
			render::get().add_text(ImVec2(end_screen.x, end_screen.y), Color::Red().ToImGUI(), CENTERED_X | CENTERED_Y | DROP_SHADOW, BAKSHEESH_12, XorStr("real"));
			//encrypts(0)
		}
		// fake angle
		if (variable::get().visuals.b_show_antiaim_fake)
		{
			AngleVectors(QAngle(0.f, g_Info.m_flChokedYaw, 0.f), &forward);
			end = start + (forward * 50.f);

			if (!WorldToScreen(start, start_screen) || !WorldToScreen(end, end_screen))
				return;

			render::get().add_line(ImVec2(start_screen.x, start_screen.y), ImVec2(end_screen.x, end_screen.y), Color::Green().ToImGUI());
			//decrypts(0)
			render::get().add_text(ImVec2(end_screen.x, end_screen.y), Color::Green().ToImGUI(), CENTERED_X | CENTERED_Y | DROP_SHADOW, BAKSHEESH_12, XorStr("fake"));
			//encrypts(0)
		}
		// lby angle
		if (variable::get().visuals.b_show_antiaim_lby)
		{
			AngleVectors(QAngle(0.f, LocalPlayer.LowerBodyYaw, 0.f), &forward);
			end = start + (forward * 50.f);

			if (!WorldToScreen(start, start_screen) || !WorldToScreen(end, end_screen))
				return;

			render::get().add_line(ImVec2(start_screen.x, start_screen.y), ImVec2(end_screen.x, end_screen.y), Color::Yellow().ToImGUI());
			//decrypts(0)
			render::get().add_text(ImVec2(end_screen.x, end_screen.y), Color::Yellow().ToImGUI(), CENTERED_X | CENTERED_Y | DROP_SHADOW, BAKSHEESH_12, XorStr("lby"));
			//encrypts(0)
		}
	}
#endif
}

void CVisuals::RunAutowallCrosshair()
{
	// autowall crosshair
	if (variable::get().visuals.i_autowall_xhair > 0 && LocalPlayer.CurrentWeapon->GetCSWpnData())
	{

		QAngle va;
		Interfaces::EngineClient->GetViewAngles(va);
		Vector dir;
		AngleVectors(va, &dir);
		Autowall_Output_t output;
		Autowall(LocalPlayer.ShootPosition, LocalPlayer.ShootPosition + (dir * LocalPlayer.CurrentWeapon->GetCSWpnData()->flRange), output, true);
		g_Visuals.m_autowallHit = output.penetrated_wall;
		g_Visuals.m_autowallHitPlayer = output.entity_hit != nullptr;
		g_Visuals.m_autowallDamage = static_cast<int>(output.damage_dealt);
		for (auto i = 0; i < output.damage_per_wall.size(); ++i)
		{
			g_Visuals.m_autowallDamagePerWall[i] = output.damage_per_wall[i];
		}

		if (output.entity_hit != nullptr)
		{
			g_Visuals.m_autowallHitbox = output.hitbox_hit;
			g_Visuals.m_autowallHitgroup = output.hitgroup_hit;
		}
		else
		{
			g_Visuals.m_autowallHitbox = -1;
			g_Visuals.m_autowallHitgroup = -1;
		}
	}
}

void CVisuals::GrabMaterials()
{
	// clear material list
	m_WorldMats.clear();
	m_staticPropMats.clear();

	// loop through all materials
	for (MaterialHandle_t i = Interfaces::MatSystem->FirstMaterial(); i != Interfaces::MatSystem->InvalidMaterial(); i = Interfaces::MatSystem->NextMaterial(i))
	{
		// get current material
		IMaterial* mat = Interfaces::MatSystem->GetMaterial(i);

		// skip invalid materials
		if (!mat)
			continue;

		// add world materials to vector
		//decrypts(0)
		if (strstr(mat->GetTextureGroupName(), XorStr("World")))
			m_WorldMats.emplace_back(mat);
		// add static prop materials to vector
		else if (strstr(mat->GetTextureGroupName(), XorStr("StaticProp")))
			m_staticPropMats.emplace_back(mat);
		//encrypts(0)
	}
}

void CVisuals::HandleMaterialModulation()
{
	// spoof cvar
	//decrypts(0)
	static ConVar* r_DrawSpecificStaticProp = Interfaces::Cvar->FindVar(XorStr("r_DrawSpecificStaticProp"));
	//encrypts(0)

	// set cvar if needed
	//if (g_Convars.Visuals.world_asus_walls->GetBool())
	//	r_DrawSpecificStaticProp->SetValue(0);
	//else
	r_DrawSpecificStaticProp->SetValue(-1);

	// init needed colors
	Color night_props = Color(127, 127, 127);
	Color night_world = Color(43, 41, 46);
	Color color_default = Color(255, 255, 255);

	// loop through all world mats
	for (auto& world : m_WorldMats)
	{
		// skip invalid
		if (!world)
			continue;

		// set nightmode colors
#if 0
		if (g_Convars.Visuals.world_nightmode->GetBool())
			world->ColorModulate(night_world.rBase(), night_world.gBase(), night_world.bBase());
		// set default colors
		else
#endif
			world->ColorModulate(color_default.rBase(), color_default.gBase(), color_default.bBase());

		// we want asus walls
#if 0
		if (g_Convars.Visuals.world_asus_walls->GetBool())
		{
			// lower alpha if needed
			if (!g_Convars.Visuals.world_asus_props_only->GetBool())
				world->AlphaModulate(g_Convars.Visuals.world_asus_alpha->GetFloat() / 100.f);
			// set to default otherwise
			else
				world->AlphaModulate(1.f);
		}
		// we don't want asus walls
		else
#endif
			world->AlphaModulate(1.f);
	}

	for (auto& prop : m_staticPropMats)
	{
		// skip invalid
		if (!prop)
			continue;

		// set nightmode colors
#if 0
		if (g_Convars.Visuals.world_nightmode->GetBool())
			prop->ColorModulate(night_props.rBase(), night_props.gBase(), night_props.bBase());
		// set default colors
		else
#endif
			prop->ColorModulate(color_default.rBase(), color_default.gBase(), color_default.bBase());

		// lower alpha if needed
#if 0
		if (g_Convars.Visuals.world_asus_walls->GetBool())
			prop->AlphaModulate(g_Convars.Visuals.world_asus_alpha->GetFloat() / 100.f);
		// set to default otherwise
		else
#endif
			prop->AlphaModulate(1.f);
	}

	m_bMaterialsNeedUpdate = false;
}

void CVisuals::FindMaterials()
{
	//decrypts(0)
	for (auto materialHandle = Interfaces::MatSystem->FirstMaterial();
		materialHandle != Interfaces::MatSystem->InvalidMaterial();
		materialHandle = Interfaces::MatSystem->NextMaterial(materialHandle))
	{
		auto ma = Interfaces::MatSystem->GetMaterial(materialHandle);
		auto hash = ma->GetName();

		//DEBUGPRINT("Material: %s, 0x%x", ma->GetName(), hash);

		if (hash == XorStr("debug/debugambientcube"))
		{
			g_Info.LitMaterial = ma;
			continue;
		}

		if (hash == XorStr("debug/debugtranslucentsinglecolor"))
		{
			g_Info.UnlitMaterial = ma;
			continue;
		}

		if (g_Info.LitMaterial && g_Info.UnlitMaterial)
			break;
	}
	//encrypts(0)

	//decrypts(0)
	g_Info.GlassMaterial = Interfaces::MatSystem->FindMaterial(XorStr("models/inventory_items/cologne_prediction/cologne_prediction_glass"), XorStr("Other textures"));
	//encrypts(0)

	//decrypts(0)

	auto mat = Interfaces::MatSystem->FindMaterial(XorStr("dev/blurfilterx_nohdr"), XorStr("Other textures"));

	if (mat)
		BlurMaterials.push_back(mat);
	mat = Interfaces::MatSystem->FindMaterial(XorStr("dev/blurfiltery_nohdr"), XorStr("Other textures"));

	if (mat)
		BlurMaterials.push_back(mat);

	mat = Interfaces::MatSystem->FindMaterial(XorStr("dev/scope_bluroverlay"), XorStr("Other textures"));

	if (mat)
		BlurMaterials.push_back(mat);

	mat = Interfaces::MatSystem->FindMaterial(XorStr("particle/vistasmokev1/vistasmokev1_fire"), XorStr("Other textures"));
	if (mat)
		SmokeMaterials.push_back(mat);
	mat = Interfaces::MatSystem->FindMaterial(XorStr("particle/vistasmokev1/vistasmokev1_smokegrenade"), XorStr("Other textures"));
	if (mat)
		SmokeMaterials.push_back(mat);
	mat = Interfaces::MatSystem->FindMaterial(XorStr("particle/vistasmokev1/vistasmokev1_emods"), XorStr("Other textures"));
	if (mat)
		SmokeMaterials.push_back(mat);
	mat = Interfaces::MatSystem->FindMaterial(XorStr("particle/vistasmokev1/vistasmokev1_emods_impactdust"), XorStr("Other textures"));
	if (mat)
		SmokeMaterials.push_back(mat);

	mat = Interfaces::MatSystem->FindMaterial(XorStr("debug/debugdrawflat"), XorStr("Model textures"));

	if (mat)
		MutinyMaterials[FLAT] = mat;

	mat = Interfaces::MatSystem->FindMaterial(XorStr("debug/debugambientcube"), XorStr("Model textures"));

	if (mat)
		MutinyMaterials[SHADED] = mat;

	mat = Interfaces::MatSystem->FindMaterial(XorStr("models/inventory_items/dogtags/dogtags_outline"), XorStr("Model textures"));

	if (mat)
		MutinyMaterials[PULSE] = mat;

	mat = Interfaces::MatSystem->FindMaterial(XorStr("models/weapons/v_models/arms/glove_motorcycle/glove_motorcycle_right"), XorStr("Model textures"));

	if (mat)
		MutinyMaterials[METALLIC] = mat;

	mat = Interfaces::MatSystem->FindMaterial(XorStr("models/inventory_items/cologne_prediction/cologne_prediction_glass"), XorStr("Other textures"));

	if (mat)
		MutinyMaterials[GLASS] = mat;


	//encrypts(0)
	for (const auto &mat : MutinyMaterials)
	{
		if (!mat)
			continue;

		mat->SetMaterialVarFlag(MATERIAL_VAR_NOFOG, true);
		mat->SetMaterialVarFlag(MATERIAL_VAR_ADDITIVE, false);
		mat->SetMaterialVarFlag(MATERIAL_VAR_VERTEXCOLOR, false);
		mat->SetMaterialVarFlag(MATERIAL_VAR_VERTEXALPHA, false);
		mat->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, false);
		mat->SetMaterialVarFlag(MATERIAL_VAR_WIREFRAME, false);

		mat->IncrementReferenceCount();
	}
}


std::string CVisuals::GetResolveSide(int side)
{
	switch (side)
	{
	case INVALID_RESOLVE_SIDE:
	{
		//decrypts(0)
		std::string s = XorStr("INVALID");
		//encrypts(0)
		return s;
	}
	case NEGATIVE_60:
	{
#if defined _DEBUG || defined STAFF || defined INTERNAL_DEBUG
		return "DESYNCED [-60]";
#else
		//decrypts(0)
		std::string s = XorStr("DESYNCED [1]");
		//encrypts(0)
		return s;
#endif
	}
	case POSITIVE_60:
	{
#if defined _DEBUG || defined STAFF || defined INTERNAL_DEBUG
		return "DESYNCED [+60]";
#else
		//decrypts(0)
		std::string s = XorStr("DESYNCED [2]");
		//encrypts(0)
		return s;
#endif
	}
	case NEGATIVE_35:
	{
#if defined _DEBUG || defined STAFF || defined INTERNAL_DEBUG
		return "DESYNCED [-35]";
#else
		//decrypts(0)
		std::string s = XorStr("DESYNCED [3]");
		//encrypts(0)
		return s;
#endif
	}
	case POSITIVE_35:
	{
#if defined _DEBUG || defined STAFF || defined INTERNAL_DEBUG
		return "DESYNCED [+35]";
#else
		//decrypts(0)
		std::string s = XorStr("DESYNCED [4]");
		//encrypts(0)
		return s;
#endif
	}
	case NONE:
	{
#if defined _DEBUG || defined STAFF || defined INTERNAL_DEBUG
		return "DESYNCED [NONE]";
#else
		//decrypts(0)
		std::string s = XorStr("DESYNCED [5]");
		//encrypts(0)
		return s;
#endif
	}
	default:
	{
		//decrypts(0)
		std::string s = XorStr("UNKNOWN");
		//encrypts(0)
		return s;
	}
	}
	return{};
}

std::string CVisuals::GetResolveType(CPlayerrecord *playerRecord, CTickrecord *currentRecord, bool* lbybroke, ImColor* color)
{
	std::string info{};

	if (playerRecord->m_bLegit && !playerRecord->m_bForceNotLegit)
	{
		//decrypts(0)
		info = XorStr("LEGIT");
		//encrypts(0)

		if (color != nullptr)
			*color = ImColor(0, 255, 0);
	}
	else if (currentRecord)
	{
		// check for broken lby
		if (playerRecord->m_bIsBreakingLBYWithLargeDelta || playerRecord->m_bIsConsistentlyBalanceAdjusting)
		{
			if (lbybroke != nullptr)
				*lbybroke = true;
		}

		switch (currentRecord->m_iResolveMode)
		{
		case RESOLVE_MODE_MANUAL:
		{
			// get correct side based on choked/unchoked
			auto side = currentRecord->m_iResolveSide;

			if (color != nullptr)
				*color = side == INVALID_RESOLVE_SIDE ? ImColor(255, 0, 0) : currentRecord->m_iTicksChoked == 0 ? ImColor(255, 0, 255) : ImColor(191, 85, 236);

			info = GetResolveSide(side);

			break;
		}
		case RESOLVE_MODE_AUTOMATIC:
		case RESOLVE_MODE_BRUTE_FORCE:
		{
			// old resolve-side resolver
			if (currentRecord)
			{
				// get correct side based on choked/unchoked
				auto side = currentRecord->m_iResolveSide;

				if (color != nullptr)
					*color = side == INVALID_RESOLVE_SIDE ? ImColor(255, 0, 0) : currentRecord->m_iTicksChoked == 0 ? ImColor(255, 0, 255) : ImColor(191, 85, 236);

				info = GetResolveSide(side);

				if (currentRecord->m_bUsedBodyHitResolveDelta)
				{
					//decrypts(0)
					info += XorStr(" | DETECTED");
					//encrypts(0)
				}
				else
				{
					if (currentRecord->m_iResolveMode == RESOLVE_MODE_AUTOMATIC)
					{
						//decrypts(0)
						info += XorStr(" | AUTO");
						//encrypts(0)
					}
					else
					{
						//decrypts(0)
						info += XorStr(" | BRUTEFORCE");
						//encrypts(0)
					}
				}

				//#if defined _DEBUG || defined INTERNAL_DEBUG || defined STAFF
				if (currentRecord->m_bIsUsingBalanceAdjustResolver)
				{
					//decrypts(0)
					info += XorStr(" | BALANCE");
					//encrypts(0)
				}
				if (currentRecord->m_bIsUsingFreestandResolver)
				{
					//decrypts(0)
					info += XorStr(" | FREESTAND");
					//encrypts(0)
				}
				if (currentRecord->m_bIsUsingMovingResolver)
				{
					//decrypts(0)
					info += XorStr(" | LASTMOVE");
					//encrypts(0)
				}
				//#endif
			}
			break;
		}
		default:
		{
			//decrypts(0)
			info = XorStr("NONE");
			//encrypts(0)

			if (color != nullptr)
				*color = ImColor(255, 0, 0);
			break;
		}
		}
	}

	return info;
}

std::string CVisuals::GetResolveType(ShotResult shotresult)
{
	std::vector<std::string> resolve_type;
#ifdef _DEBUG
	//printf("[missed] body hit: %d | stance: %d | rs: %d\n", shotresult.m_bIsBodyHitResolved, shotresult.m_iBodyHitResolveStance, shotresult.m_ResolveSide_WhenChoked);
#endif

	if (shotresult.m_bLegit && !shotresult.m_bForceNotLegit)
	{
		//decrypts(0)
		resolve_type.push_back(XorStr("LEGIT"));
		//encrypts(0)
	}
	else
	{
		switch (shotresult.m_ResolveMode)
		{
		case RESOLVE_MODE_MANUAL:
		case RESOLVE_MODE_AUTOMATIC:
		case RESOLVE_MODE_BRUTE_FORCE:
		{
			resolve_type.push_back(GetResolveSide(shotresult.m_ResolveSide));
			break;
		}
		default:
		{
			//decrypts(0)
			resolve_type.push_back(XorStr("NONE"));
			//encrypts(0)
			break;
		}
		}
	}

	if (shotresult.m_bEnemyFiredBullet)
	{
		//decrypts(0)
		resolve_type.push_back(XorStr("SHOT"));
		//encrypts(0)
	}

	if (shotresult.m_bEnemyIsNotChoked)
	{
		//decrypts(0)
		resolve_type.push_back(XorStr("REAL"));
		//encrypts(0)
	}

#if defined DEBUG_SHOTS || defined _DEBUG || defined INTERNAL_DEBUG || defined STAFF
	if (shotresult.m_bForwardTracked)
	{
		//decrypts(0)
		resolve_type.push_back(XorStr("FT"));
		//encrypts(0)
	}

	if (shotresult.m_bDoesNotFoundTowardsStats)
	{
		//decrypts(0)
		resolve_type.push_back(XorStr("NO STATS"));
		//encrypts(0)
	}

	if (shotresult.m_bCountedBodyHitMiss)
	{
		//decrypts(0)
		resolve_type.push_back(XorStr("COUNTED BH MISS"));
		//encrypts(0)
	}
#endif

	std::string result;
	for (int i = 0; i < resolve_type.size(); ++i)
	{
		result += resolve_type[i];
		if (i < resolve_type.size() - 1)
		{
			result += " | ";
		}
	}

	return result;
}

void Beams::render_beam(const Vector &start_pos, const Vector &end_pos, ImColor color)
{
	const float life_time = 5.f;
	const float width = 10.f;
	const float speed = 20.f;

	BeamInfo_t beamInfo;

	beamInfo.m_nType = TE_BEAMPOINTS;
	//decrypts(0)
	beamInfo.m_pszModelName = XorStr("sprites/purplelaser1.vmt");
	//encrypts(0)
	beamInfo.modelindex = -1;
	beamInfo.m_flHaloScale = 0.f;
	beamInfo.m_flLife = life_time;
	beamInfo.m_flWidth = width;
	beamInfo.m_flEndWidth = width;
	beamInfo.m_flAmplitude = 0.f;
	beamInfo.m_flFadeLength = 2.5f;
	beamInfo.m_flBrightness = 255.f;
	beamInfo.m_flSpeed = speed;
	beamInfo.m_nStartFrame = 0;
	beamInfo.m_flFrameRate = 0.f;
	beamInfo.m_flRed = color.Value.x * 255.f;
	beamInfo.m_flGreen = color.Value.y * 255.f;
	beamInfo.m_flBlue = color.Value.z * 255.f;
	beamInfo.m_nSegments = 2;
	beamInfo.m_bRenderable = true;
	beamInfo.m_vecStart = LocalPlayer.EyePosition + Vector(1.0f, 0.f, 0.f);
	beamInfo.m_vecEnd = end_pos;

	Beam_t *new_beam = Interfaces::Beams->CreateBeamPoints(beamInfo);
	if (new_beam)
		Interfaces::Beams->DrawBeam(new_beam);
}

void Beams::render_beam_ring(const Vector &origin, ImColor color)
{
	BeamInfo_t beamInfo;

	// nit; there's a new check that requires modelindex >= 0 otherwise, return nullptr
	//decrypts(0)
	beamInfo.m_pszModelName = XorStr("sprites/purplelaser1.vmt");
	beamInfo.modelindex = Interfaces::ModelInfoClient->GetModelIndex(XorStr("sprites/purplelaser1.vmt"));
	//encrypts(0)
	beamInfo.m_nHaloIndex = 0;
	beamInfo.m_flHaloScale = 0.f;
	beamInfo.m_flLife = 3.f;                  // speed of the radius expanding
	beamInfo.m_flWidth = 10.f;                 // thickness of the ring at the start
	beamInfo.m_flEndWidth = 1.f;                  // thickness of the ring at the end
	beamInfo.m_flFadeLength = 1.f;                  // interval of the beam fading out (FBEAM_FADEOUT)
	beamInfo.m_flAmplitude = 0.f;                  // this needs to be set to 0, anything else, the ringpoint doesn't draw
	beamInfo.m_flBrightness = 255.f;
	beamInfo.m_flSpeed = 0.f;                  // needs to be set to 0
	beamInfo.m_nStartFrame = 0;
	beamInfo.m_flFrameRate = 2.f;
	beamInfo.m_flRed = color.Value.x * 255.f;
	beamInfo.m_flGreen = color.Value.y * 255.f;
	beamInfo.m_flBlue = color.Value.z * 255.f;
	beamInfo.m_vecCenter = origin;
	beamInfo.m_flStartRadius = 0.f;
	beamInfo.m_flEndRadius = 1500.f;
	beamInfo.m_nFlags = FBEAM_ONLYNOISEONCE | FBEAM_FADEOUT;

	Beam_t *new_beam = Interfaces::Beams->CreateBeamRingPoint(beamInfo);
	if (new_beam)
		Interfaces::Beams->DrawBeam(new_beam);
}
