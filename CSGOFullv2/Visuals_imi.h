#pragma once
#include "Includes.h"

#define INDICATOR_Y_1080 450.f
#define INDICATOR_Y_900  300.f
#define INDICATOR_Y      240.f

enum ResolveSides : int;

struct ShotResult;

class CustomPlayerInfo
{
public:
	CustomPlayerInfo()
	{
		m_bIsDormant = false; //set from DormantESP
		m_bWasDormant = false; //set from DormantESP
		m_bIsOffscreen = false; //set from ESP drawing
		m_bIsVisible = false; //set from ESP drawing
		m_bIsMoving = false; //set from DoSpyCam through Entity/Playerrecord
		m_bInAir = false; //set from DoSpyCam through Entity/Playerrecord
		m_flDistance = 0.f; //set from DoSpyCam through Entity/Playerrecord
		m_bWillPeek = false;
		m_flLastDamageTime = 0.f; //set from player_hurt
		m_flLastKillDistance = 0.f; //set from player_death
		m_flLastKillTime = 0.f; //set from player_death
	}

	void SetInfoForEntity(CBaseEntity*);

	bool m_bIsDormant = false;
	bool m_bWasDormant = false;
	bool m_bIsOffscreen = false;
	bool m_bIsMoving = false;
	bool m_bInAir = false;
	bool m_bWillPeek = false;
	bool m_bIsVisible = false;
	float m_flLastDamageTime = 0.f;
	float m_flLastKillDistance = 0.f;
	float m_flLastKillTime = 0.f;
	float m_flDistance = 0.f;
};

enum TYPE
{
	NO_TYPE,
	HEALTH,
	ARMOR,
	AMMO,
	NAME,
	WEAPON,
	DEFUSING,
	ZOOM,
	ANTIAIM,
	DETONATION,
	PING,
	LBYTIME,
	BOMBCARRIER,
	RELOAD,
	MONEY,
	KIT,
	PINPULL,
	BURN,
	BLIND,
	VESTHELM,
	FAKEDUCK,
};

enum FLAGS
{
	NO_FLAG,
	TEXT,
	BAR,
	BARTEXT
};

enum SIDE
{
	NO_SIDE = -1,
	LEFT,
	RIGHT,
	TOP,
	BOTTOM
};

extern std::map<std::string, TYPE> ESPTypeNames;

enum ENTTYPE
{
	DRAW = 0,
	PLAYER,
	WEAP,
	BOMB,
	NADE,
	CHICKEN,
	MAXTYPE
};

struct sDraw
{
	std::string m_szText = "", m_szCustom = "";
	float m_flValue = 0.f;
	float m_flMax = 0.f;
	Color m_clr = Color(0, 0, 0, 0);
	POINT m_RenderPosition;
	int m_Align = 0;

	void Set(std::string _text = "", std::string _custom = "", float _value = 0.f, float _max = 0.f, Color _clr = Color::White())
	{
		m_szText = _text;
		m_szCustom = _custom;
		m_flValue = _value;
		m_flMax = _max;
		m_clr = _clr;
	}

	bool ShouldDraw() const
	{
		return !(m_szText.empty() && m_flMax == 0.f && m_flValue == 0.f);
	}
};

struct BulletTrace_s {
	Vector m_vecStart;
	Vector m_vecStop;
	int m_iTeamID;
	int m_tickCount;
};

struct SideItem_t
{
	std::string info;
	ImColor color;

	SideItem_t(std::string _info, ImColor _color) : info(_info), color(_color)
	{
	}
};

class Beams
{
public:
	Beams() = default;
	~Beams() = default;

	void render_beam(const Vector &start_pos, const Vector &end_pos, ImColor color);

	void render_beam_ring(const Vector &origin, ImColor color);
};

enum MaterialType_t
{
	FLAT = 0,
	SHADED = 1,
	PULSE = 2,
	METALLIC = 3,
	GLASS = 4,
	MAX_MATERIALS,
};

class CVisuals
{
protected:
	struct BoundingBox
	{
		float x = 0, y = 0, w = 0, h = 0;
		BoundingBox() { }
		BoundingBox(float _x, float _y, float _w, float _h)
		{
			x = _x;
			y = _y;
			w = _w;
			h = _h;
		}
	};
	bool GenerateBoundingBox(CBaseEntity* Entity, Vector& origin, QAngle& angles, bool rotate, BoundingBox& out);
	bool GenerateBoundingBox(CBaseEntity* Entity, Vector& mins, Vector& maxs, BoundingBox& out);
	bool GetBox(CBaseEntity *entity, const Vector &origin, BoundingBox &box, bool is_far_esp, bool use_valve = false);

	void DrawPlayerESP(CBaseEntity* Entity);
	void DrawGlow(CBaseEntity* Entity, int r, int g, int b, int a);
	void DrawGlow(CBaseEntity* Entity, Color clr, int a);
	void DrawGlow(CBaseEntity* Entity, Color clr);

	void OffscreenESP(CBaseEntity* ent, Color color, float size);

	std::vector<IMaterial*> m_WorldMats;
	std::vector<IMaterial*> m_staticPropMats;

	int m_tp_oldState = -1;
	bool m_tp_wasDeadInThirdperson = false;

public:
	bool m_bMaterialsNeedUpdate = true;

	std::vector<BulletTrace_s> m_BulletTracers;
	std::array<CustomPlayerInfo, 65> m_customESP;
	std::array<float, 65> arr_alpha{};

	int _iHurtTime = 0;

	Vector m_lastBestAimbotPos{};
	int m_lastBestDamage = -1;
	float m_lastHitchance = 0.f;

	int m_autowallDamage = -1;
	std::array<float, 4> m_autowallDamagePerWall;
	bool m_autowallHit = false;
	bool m_autowallHitPlayer = false;
	int m_autowallHitgroup = 0;
	int m_autowallHitbox = 0;

	bool m_prioritize_body = false;

	int last_ticks_choked = 0;

	int last_forcecam = 0;

	bool draw_strict = false;
	int strict_hitgroup_one = -1;
	int strict_hitgroup_two = -1;

	float weapon_spread = -1.f;

	std::vector< IMaterial *> BlurMaterials{};
	std::vector< IMaterial *> SmokeMaterials{};
	std::array<IMaterial*, MAX_MATERIALS> MutinyMaterials{};

	CVisuals();

	void game_event(CGameEvent* p_event);

	void Clear();
	void DrawESP();
	void DrawEntityESP(CBaseEntity*);
	void DrawPlantedBomb(CBaseEntity*);
	void DrawDroppedItem(CBaseEntity*, const Vector&);
	void DrawProjectile(CBaseEntity*, const Vector&);
	void DrawGlow();
	void DrawHitmarker() const;
	void DrawIndicators();
	void DrawAntiMedia();
	void DrawBulletTracers();
	void DrawSpreadCircle();
	void DrawScopeLines();
	void RunAutowallCrosshair();
	void ThirdPerson(CViewSetup *pSetup);
	void SpectateAll();
	void FOVChanger(CViewSetup* pSetup);
	void DrawAntiAim();
	void DrawAutowallCrosshair();

	void GrabMaterials();

	void HandleMaterialModulation();

	void FindMaterials();

	std::string GetResolveType(CPlayerrecord *playerRecord, CTickrecord *currentRecord, bool *lbybroke = nullptr, ImColor* color = nullptr);
	std::string GetResolveType(ShotResult result);
	std::string GetResolveSide(int side);
};

extern CVisuals g_Visuals;
extern Beams g_Beams;