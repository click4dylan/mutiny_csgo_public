#include "Interfaces.h"
#include "LocalPlayer.h"
#include <unordered_map>

struct HitMarker
{
	float expireTime;
	int damage;
};


struct BulletImpact
{
    CBaseEntity* target;
    Vector endPos;
    float time;

    BulletImpact() : target(nullptr), endPos(0.f, 0.f, 0.f), time(0.f) {}
};

// Max Visual Clients
#define VISUAL_MAXCLINETS 64
//Visual Class

class Visuals
{
public:

	// Player Visuals
	void PlayerVisuals();
	void WorldVisuals();
	//void SoundESP(int index);

	//Screen Drawing
	void ScreenDrawing();

	// Stuff
	int VisualScreenWidth;
	int VisualScreenHeight;

	//Bullet Impacts
	std::deque<BulletImpact> bulletImpacts;
	std::vector<HitMarker> hitMarkers;

	void ThreeDBox(CBaseEntity* pEntity, Color col, Vector origin);
	void BoxESP(float x, float y, float w, float h, Color clr);
	void DrawAngleLines(Color cColor, CBaseEntity* pEnt);
	void CornerBox(float x, float y, float w, float h, int borderPx, Color col);
	void WeaponESP(Color cColor, int x, int y, HFont hFont, CBaseEntity* pEnt);
	void DrawHealth(Color cColor, int topx, int topy, HFont hFont, CBaseEntity* pEnt, int health);;
};
extern std::vector<IMaterial*> m_worldMaterials;
extern std::vector<IMaterial*> m_staticPropMaterials;
extern Visuals* PlayerVisuals;
extern IMaterial* visible_tex;
extern IMaterial* hidden_tex;
extern IMaterial* visible_flat;
extern IMaterial* hidden_flat;
extern HFont ESPFONT;
extern HFont ESPNumberFont;
extern HFont WeaponIcon;
extern ConVar* r_DrawSpecificStaticProp;
extern std::string defaultSkyName;
extern void SetupFonts();
extern void SetupTextures();
extern void DrawString(HFont font, int x, int y, Color color, DWORD alignment, const char* msg, ...);
extern void DrawPixel(int x, int y, Color col);
extern void GetTextSize(unsigned long& Font, int& w, int& h, const char* strText, ...);
extern void DrawStringOutlined(unsigned long& Font, Vector2D pos, Color c, unsigned int flags, const char* strText, ...);
extern void DrawOutlinedRect(int x, int y, int w, int h, Color col);
extern void DrawLine(int x0, int y0, int x1, int y1, Color col);
extern void DrawRect(int x, int y, int w, int h, Color col);
extern void DrawFilledRect(int x, int y, int w, int h, Color col);
extern void DrawCircle(float x, float y, float r, float s, Color color);
extern void FillRGBA(int x, int y, int w, int h, Color col);
