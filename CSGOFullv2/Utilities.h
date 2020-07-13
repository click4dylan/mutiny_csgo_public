#pragma once
#include <inttypes.h>
#include <vector>
#include <string>
#include <map>

class Color;
struct typedescription_t;
struct datamap_t;
class QAngle;
class CBaseEntity;
class Vector;
class IMaterial;
class ITexture;


typedef enum _AMMOTYPE
{
	BULLET_PLAYER_INVALID,
	BULLET_PLAYER_50AE, // deagle, revolver
	BULLET_PLAYER_762MM, // ak, scar20, aug, ssg08, g3sg1, 
	BULLET_PLAYER_556MM, // sg556, m4a1, famas, galil
	BULLET_PLAYER_556MM_SMALL, // m4a1s
	BULLET_PLAYER_556MM_BOX, // negev, m249
	BULLET_PLAYER_338MAG, // awp, 
	BULLET_PLAYER_9MM, // tec-9, mp7, mp9, glock, bizon, dualies
	BULLET_PLAYER_BUCKSHOT, // sawed-off, xm1014, nova, mag7
	BULLET_PLAYER_45ACP = 9, // ump, mac10, 
	BULLET_PLAYER_357SIG, // p2000, 
	BULLET_PLAYER_357SIG_SMALL, // usp
	BULLET_PLAYER_357SIG_P250, // p250, cz75
	BULLET_PLAYER_57MM, // p90, fiveseven, 
						// BULLET_PLAYER_357SIG_MIN, // unused???
} AmmoType;

uint32_t fnv1a(const void* data, size_t numBytes, uint32_t hash = 0x811C9DC5);
uint32_t fnv1a(const char* text, uint32_t hash = 0x811C9DC5);
uint32_t fnv1a(const std::string& text, uint32_t hash = 0x811C9DC5);

std::vector<std::string> split(const std::string &s, char delim);
bool replace(std::string& str, const std::string& from, const std::string& to);

std::string GetHexFromColor(Color _clr);
Color GetColorFromHex(std::string _hex);

int MapReverseKey(std::map<int, std::string> _map, std::string _find);

bool IsPointInBox(int _x1, int _y1, int _x2, int _y2, int _w, int _h, bool debug = false);

void replaceAll(std::string &s, const std::string &search, const std::string &replace);
bool Hitchance(CBaseEntity* player, QAngle& viewangles, float percent, int _TargetHitgroup = 0, float *calcedHitchance = nullptr);
void GenerateTexture(std::string name, IMaterial* *mat, ITexture* *tex);

void ForceMaterial(bool ignoreZ, Color clr, IMaterial* material);
void ForceMaterial(bool ignoreZ, int r, int g, int b, int a, IMaterial* material);

void ForceMaterialAlpha(bool ignoreZ, float a, IMaterial* material);
void ForceMaterialAlpha(int alpha, IMaterial* material);

bool IsAMD();

int IntFromChars(const std::string& str, int start);

typedescription_t* __fastcall FindFlatFieldByName(const char* name, datamap_t* map);