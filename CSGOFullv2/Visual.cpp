#include "precompiled.h"
#include <vector>
#include <array>
#include <locale>
#include "Draw.h"
#include "ICollidable.h"
#include "LocalPlayer.h"
#include "Globals.h"
#include "ConVar.h"
#include "Interfaces.h"
#include "INetchannelInfo.h"
#include "Targetting.h"
#include "AutoWall.h"
#include "ESP.h"
#include "OverrideView.h"
#include "Math.h"
#include "Utilities.h"

// Player Viusals 
// Make sure to use WorldToScreenCapped instead of WorldtoScreen!
Color AsusColor = Color(255, 255, 255, 150);
float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
Color NoSkyColor = Color(0, 0, 0, 255);
float r1 = 0.0f, g1 = 0.0f, b1 = 0.0f, a1 = 0.0f;
std::unordered_map<MaterialHandle_t, Color> skyboxMaterials;
std::unordered_map<MaterialHandle_t, Color> skyboxMaterials2;
std::unordered_map<MaterialHandle_t, Color> worldMaterials;
std::unordered_map<MaterialHandle_t, Color> worldMaterials2;
//
Visuals* PlayerVisuals = new Visuals;

std::vector<IMaterial*> m_worldMaterials;
std::vector<IMaterial*> m_staticPropMaterials;

ConVar* r_DrawSpecificStaticProp = nullptr;
char *rdrawspecificstaticpropstr = new char[25]{ 8, 37, 62, 8, 27, 13, 41, 10, 31, 25, 19, 28, 19, 25, 41, 14, 27, 14, 19, 25, 42, 8, 21, 10, 0 }; /*r_DrawSpecificStaticProp*/

std::string defaultSkyName;

#define MAX_BEAMS_ON_SCREEN 30
#define TIME_BEAMS_ON_SCREEN 3.0f;

void Visuals::BoxESP(float x, float y, float w, float h, Color clr)
{
    Interfaces::Surface->DrawSetColor(clr);
    Interfaces::Surface->DrawOutlinedRect(x - w, y, x + w, y + h);

    Interfaces::Surface->DrawSetColor(Color::Black());
    Interfaces::Surface->DrawOutlinedRect(x - w - 1, y - 1, x + w + 1, y + h + 1);
    Interfaces::Surface->DrawOutlinedRect(x - w + 1, y + 1, x + w - 1, y + h - 1);
}

char *cratestr = new char[6]{ 25, 8, 27, 14, 31, 0 }; /*crate*/
char *boxstr = new char[4]{ 24, 21, 2, 0 }; /*box*/
char *carstr = new char[4]{ 25, 27, 8, 0 }; /*car*/
char *rockstr = new char[5]{ 8, 21, 25, 17, 0 }; /*rock*/
char *stonestr = new char[6]{ 9, 14, 21, 20, 31, 0 }; /*stone*/
char *doorstr = new char[5]{ 30, 21, 21, 8, 0 }; /*door*/

void Visuals::ThreeDBox(CBaseEntity* pEntity, Color col, Vector origin)
{
	auto _collideable = pEntity->GetCollideable();
    Vector min = _collideable->OBBMins() + origin;
    Vector max = _collideable->OBBMaxs() + origin;

    Vector points[] = { Vector(min.x, min.y, min.z),
        Vector(min.x, max.y, min.z),
        Vector(max.x, max.y, min.z),
        Vector(max.x, min.y, min.z),
        Vector(min.x, min.y, max.z),
        Vector(min.x, max.y, max.z),
        Vector(max.x, max.y, max.z),
        Vector(max.x, min.y, max.z) };

    int edges[12][2] = { { 0, 1 },{ 1, 2 },{ 2, 3 },{ 3, 0 },
    { 4, 5 },{ 5, 6 },{ 6, 7 },{ 7, 4 },
    { 0, 4 },{ 1, 5 },{ 2, 6 },{ 3, 7 }, };

    for (auto it : edges)
    {
        Vector p1, p2;

        if (Interfaces::DebugOverlay->ScreenPosition(points[it[0]], p1) || Interfaces::DebugOverlay->ScreenPosition(points[it[1]], p2))
            return;

        DrawLine(p1.x, p1.y, p2.x, p2.y, col);

    }
}
void DrawName(Color cColor, int topx, int topy, HFont hFont, CBaseEntity* pEnt)
{
    player_info_t info;
    pEnt->GetPlayerInfo(&info);
    DrawString(hFont, topx, topy, cColor, FONT_CENTER, info.name);
}
void Visuals::ScreenDrawing()
{
    LocalPlayer.Get(&LocalPlayer);
    bool InGame = Interfaces::EngineClient->IsInGame() && LocalPlayer.Entity;
    if (InGame)
    {
        //TODO FINISH

    }
}

/*void Visuals::SoundESP(int index)
{
    CBaseEntity* Entity = Interfaces::ClientEntList->GetBaseEntity(index);
    if (Entity && Entity->IsPlayer())
    {
        if (Entity != LocalPlayer.Entity && !Entity->GetDormant() && Entity->GetTeam() != LocalPlayer.Entity->GetTeam() && Entity->GetAlive())
        {
            Vector EnemyEyePos = Entity->GetEyePosition();
            Vector OurEyePos = LocalPlayer.Entity->GetEyePosition();
            float DistanceToUs = (EnemyEyePos - OurEyePos).Length();
            if (DistanceToUs < 4200.0f)
            {
                QAngle OurEyeAngles = LocalPlayer.Entity->GetEyeAngles();
                QAngle EnemyEyeAngles = Entity->GetEyeAngles();
                QAngle AngleFromEnemyToUs = CalcAngle(EnemyEyePos, OurEyePos);
                QAngle AngleFromUsToEnemy = CalcAngle(OurEyePos, EnemyEyePos);

                float DistFromEnemy = (EnemyEyePos - OurEyePos).Length();

                float FOVFromUsToEnemy = GetFovRegardlessOfDistance(OurEyeAngles, AngleFromUsToEnemy, DistFromEnemy);
                float FOVFromEnemyToUs = GetFovRegardlessOfDistance(EnemyEyeAngles, AngleFromEnemyToUs, DistFromEnemy);

                if (FOVFromUsToEnemy < 45.0f)
                {
                    bool EnemyIsFar = DistanceToUs > 1000.0f;
                    bool EnemyIsAimingAtUs = FOVFromEnemyToUs < 45.0f;

                    const char *SoundToPlay;
                    const char*             dangerfarsound = "C:\\Mutiny_pw\\Sounds\\df.wav";
                    const char*             dangerclosesound = "C:\\Mutiny_pw\\Sounds\\dc.wav";
                    const char*             farsound = "C:\\Mutiny_pw\\Sounds\\f.wav";
                    const char*             closesound = "C:\\Mutiny_pw\\Sounds\\c.wav";

                    if (EnemyIsAimingAtUs)
                    {
                        SoundToPlay = EnemyIsFar ? dangerfarsound : dangerclosesound;
                    }
                    else
                    {
                        SoundToPlay = EnemyIsFar ? farsound : closesound;
                    }

                    PlaySoundA(SoundToPlay, NULL, SND_FILENAME | SND_ASYNC);

                    Settings::NextTimeCanPlaySound = Interfaces::Globals->realtime + 0.5;
                }
            }
        }
    }

}*/

char *nosmokesigstr = new char[31]{ 59, 73, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 79, 77, 90, 90, 66, 56, 90, 90, 57, 56, 0 }; /*A3  ??  ??  ??  ??  57  8B  CB*/

extern float LocalFOV;

void Visuals::CornerBox(float x, float y, float w, float h, int borderPx, Color col)
{
    // TODO: To be Improved
    FillRGBA((x - (w / 2)) - 1, (y - h + borderPx) - 1, (w / 3) + 2, borderPx + 2, Color(0, 0, 0)); //top
    FillRGBA((x - (w / 2) + w - w / 3) - 1, (y - h + borderPx) - 1, w / 3, borderPx + 2, Color(0, 0, 0)); //top
    FillRGBA(x - (w / 2) - 1, (y - h + borderPx), borderPx + 2, (w / 3) + 1, Color(0, 0, 0)); //left 
    FillRGBA(x - (w / 2) - 1, ((y - h + borderPx) + h - w / 3) - 1, borderPx + 2, (w / 3) + 2, Color(0, 0, 0)); //left 
    FillRGBA(x - (w / 2), y - 1, (w / 3) + 1, borderPx + 2, Color(0, 0, 0)); //bottom 
    FillRGBA(x - (w / 2) + w - (w / 3 + 1), y - 1, (w / 3) + 2, borderPx + 2, Color(0, 0, 0)); //bottom 
    FillRGBA((x + w - borderPx) - (w / 2) - 1, (y - h + borderPx) - 1, borderPx + 2, w / 3 + 2, Color(0, 0, 0)); //right 
    FillRGBA((x + w - borderPx) - (w / 2) - 1, ((y - h + borderPx) + h - w / 3) - 1, borderPx + 2, (w / 3) + 2, Color(0, 0, 0)); //right 
                                                                                                                                               //Color
    FillRGBA(x - (w / 2), (y - h + borderPx), w / 3, borderPx, col); //top
    FillRGBA(x - (w / 2) + w - w / 3, (y - h + borderPx), w / 3, borderPx, col); //top
    FillRGBA(x - (w / 2), (y - h + borderPx), borderPx, w / 3, col); //left 
    FillRGBA(x - (w / 2), (y - h + borderPx) + h - w / 3, borderPx, w / 3, col); //left 
    FillRGBA(x - (w / 2), y, w / 3, borderPx, col); //bottom 
    FillRGBA(x - (w / 2) + w - w / 3, y, w / 3, borderPx, col); //bottom 
    FillRGBA((x + w - borderPx) - (w / 2), (y - h + borderPx), borderPx, w / 3, col); //right 
    FillRGBA((x + w - borderPx) - (w / 2), (y - h + borderPx) + h - w / 3, borderPx, w / 3, col); //right 
}

void Visuals::WeaponESP(Color cColor, int x, int y, HFont hFont, CBaseEntity* pEnt)
{
    auto pWeapon = pEnt->GetWeapon();

    if (pWeapon == nullptr)
        return;

    char *weaponname = nullptr;

    bool IsRevolver = false;

	CPlayerrecord * _playerRecord = pEnt->ToPlayerRecord();
	if (!_playerRecord || !_playerRecord->m_bHasWeaponInfo)
		return;

	WeaponInfo_t *pWeaponInfo = &_playerRecord->m_WeaponInfo;//pWeapon->GetCSWpnData();
    if (pWeaponInfo)
    {
        if (pWeaponInfo->szHudName[0] == '#' && pWeaponInfo->szHudName[12] == '_')
            weaponname = pWeaponInfo->szHudName + 13;
    }

    char info[128];
    sprintf_s<128>(info, "%s", weaponname);

    if (pWeapon->GetClipOne() >= 0 && pWeaponInfo->iMaxClip1 >= 0)
    {
        char addinfo[128];
        sprintf_s<128>(addinfo, " [%i / %i]", pWeapon->GetClipOne(), pWeapon->GetClipTwo());

        strcat_s(info, addinfo);
    }

    DrawString(hFont, x, y, cColor, FONT_CENTER, info);
}

void HealthBarBattery(Vector bot, Vector top, float health)
{
    float height = (bot.y - top.y);
    float offset = (height / 4.f) + 5;
    float flBoxes = std::ceil(health / 10.f);
    float flX = top.x - 7 - height / 4.f; float flY = top.y - 1;
    float flHeight = height / 10.f;
    float flMultiplier = 12 / 360.f; flMultiplier *= flBoxes - 1;
    Color ColHealth = Color::FromHSB(flMultiplier, 1, 1);

    DrawRect(flX, flY, 4, height + 2, Color(80, 80, 80, 125));
    DrawOutlinedRect(flX, flY, 4, height + 2, Color::Black());
    DrawRect(flX + 1, flY, 2, flHeight * flBoxes + 1, ColHealth);

    for (int i = 0; i < 10; i++)
        DrawLine(flX, flY + i * flHeight, flX + 4, flY + i * flHeight, Color::Black());

}
char *health1str = new char[11]{ 50, 31, 27, 22, 14, 18, 64, 90, 95, 19, 0 }; /*Health: %i*/

void Visuals::DrawHealth(Color cColor, int topx, int topy, HFont hFont, CBaseEntity* pEnt, int health)
{
    /*Concat Strings*/
    DecStr(health1str, 10);
    DrawString(hFont, topx, topy, cColor, FONT_CENTER, health1str, health);
    EncStr(health1str, 10);
}

void DrawRingBeam(const Vector& centerPos, ColorRGBAFloat color)
{
	return;
#if 0
    BeamInfo_t beamInfo;

    DecStr(spritespurplelaser1vmtstr, 24);

    // there is a new check that checks if modelindex < 0, return nullptr.
    beamInfo.m_pszModelName = spritespurplelaser1vmtstr;
    beamInfo.modelindex = Interfaces::ModelInfoClient->GetModelIndex(spritespurplelaser1vmtstr);
    beamInfo.m_nHaloIndex = 0;
    beamInfo.m_flHaloScale = 0.f;
    beamInfo.m_flLife = 3.f;        // speed of the radius expanding
    beamInfo.m_flWidth = 10.f;      // thickness of the ring at the start
    beamInfo.m_flEndWidth = 1.f;    // thickness of the ring at the end
    beamInfo.m_flFadeLength = 1.f;  // interval of fading away (with FBEAM_FADEOUT)
    beamInfo.m_flAmplitude = 0.f;   // if this is on, no ring is drawn
    beamInfo.m_flBrightness = 255.f;
    beamInfo.m_flSpeed = 0.f;       // doesn't affect ring but we need to set it to 0 lmao
    beamInfo.m_nStartFrame = 0;
    beamInfo.m_flFrameRate = 2.f;
    beamInfo.m_flRed = clamp(color.r, 0.f, 255.f);
    beamInfo.m_flGreen = clamp(color.g, 0.f, 255.f);
    beamInfo.m_flBlue = clamp(color.b, 0.f, 255.f);
    beamInfo.m_vecCenter = centerPos;
    beamInfo.m_flStartRadius = 0.f;
    beamInfo.m_flEndRadius = 1500.f;
    beamInfo.m_nFlags = FBEAM_ONLYNOISEONCE | FBEAM_FADEOUT;

    Beam_t* newBeam = Interfaces::Beams->CreateBeamRingPoint(beamInfo);

    if (newBeam != nullptr)
    {
        Interfaces::Beams->DrawBeam(newBeam);
    }

    EncStr(spritespurplelaser1vmtstr, 24);
#endif
}
