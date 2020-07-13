#ifndef GAMEMEMORY_H
#define GAMEMEMORY_H
#pragma once

#include "HitboxDefines.h"
#include "Offsets.h"
//#include "cx_strenc.h"
#include "misc.h"
#include "CSGO_HX.h"
#include "BaseEntity.h"
#include "BaseCombatWeapon.h"
#include "EncryptString.h"
#include "Animation.h"
#include <map>
#include "Math.h"
#include "NetworkedVariables.h"

#define FLAGS_LBYUPDATE (1 << 0)
#define FLAGS_FIRING (1 << 0)

int __cdecl HashString2 (char * guidstr);
uint32_t HashString_MUTINY(const char * s);

extern int ActualNumPlayers;

#define gamecurtime Interfaces::Globals->curtime

#define TICK_INTERVAL			(Interfaces::Globals->interval_per_tick)

#define CAST(cast, address, add) reinterpret_cast<cast>((uint32_t)address + (uint32_t)add)

#define TIME_TO_TICKS( dt )		( (int)( 0.5f + (float)(dt) / TICK_INTERVAL ) )
#define TICKS_TO_TIME( t )		( TICK_INTERVAL *( t ) )
#define ROUND_TO_TICKS( t )		( TICK_INTERVAL * TIME_TO_TICKS( t ) )
#define TICK_NEVER_THINK		(-1)

#define	LIFE_ALIVE				0 // alive
#define	LIFE_DYING				1 // playing death animation or still falling off of a ledge waiting to hit ground
#define	LIFE_DEAD				2 // dead. lying still.

enum TEAMS : DWORD {
	TEAM_NONE = 0,
	TEAM_GOTV,
	TEAM_T,
	TEAM_CT
};

#define INVALID_PLAYER INT_MAX

struct LocalPlayerStoredNetvars
{
	Vector origin;
	Vector networkorigin;
	Vector viewoffset;
	Vector LocalEyePos;
	Vector LocalShootPos;
};

struct SavedHitboxPos
{
	int index = -1;
	bool forwardtracked;
	bool penetrated;
	bool bsaved;
	bool bismultipoint;
	Vector origin;
	float damage;
	float hitchance;
	int actual_hitbox_due_to_penetration;
	int actual_hitgroup_due_to_penetration;
	Vector localplayer_shootpos;
	Vector localplayer_origin;
	QAngle localplayer_eyeangles;
};

float GetGlobalTickInterval();
int GetGlobalTickCount();
float GetGlobalCurTime();
float GetGlobalRealTime();
int GetExactTick(float SimulationTime);
int GetExactServerTick();
int GetServerTick();
float GetServerTime();

void ClearAllPlayers();

#define MAX_PLAYER_NAME_LENGTH	32
#define MAX_TRAIL_LENGTH	30
typedef struct MapPlayer_s {
	int		index;		// player's index
	int		userid;		// user ID on server
	int		icon;		// players texture icon ID
	Color   color;		// players team color
	wchar_t	name[MAX_PLAYER_NAME_LENGTH];
	int		team;		// N,T,CT
	int		health;		// 0..100, 7 bit
	Vector	position;	// current x,y pos
	QAngle	angle;		// view origin 0..360
	Vector2D trail[MAX_TRAIL_LENGTH];	// save 1 footstep each second for 1 minute
} MapPlayer_t;

using ServerRankRevealAllFn = bool(__cdecl*)(float*);
extern ServerRankRevealAllFn ServerRankRevealAllEx;

void ServerRankRevealAll();

extern unsigned char(__cdecl *ReadByte) (uintptr_t);
extern int(__cdecl *ReadInt) (uintptr_t);
extern float(__cdecl *ReadFloat) (uintptr_t);
extern double(__cdecl *ReadDouble) (uintptr_t);
extern short(__cdecl *ReadShort) (uintptr_t);
extern void(__cdecl *WriteByte) (uintptr_t, unsigned char);
extern void(__cdecl *WriteInt) (uintptr_t, int);
extern void(__cdecl *WriteFloat) (uintptr_t, float);
extern void(__cdecl *WriteDouble) (uintptr_t, double);
extern void(__cdecl *WriteShort) (uintptr_t, short);

inline void ReadByteArray(uintptr_t adr, char* dest, int size)
{
	for (int i = 0; i < size; i++)
	{
		dest[i] = ReadByte(adr + i);
	}
}

inline void WriteByteArray(char* dest, char* source, int sourcelength)
{
	for (int i = 0; i < sourcelength; i++)
	{
		WriteByte((uintptr_t)(dest + i), source[i]);
	}
}

//WARNING: No checks for evenly dividable sizes!
inline void ReadIntArray(uintptr_t adr, char* dest, int size)
{
	for (int offset = 0; offset < size; offset += 4)
	{
		//Get the rest of the bytes
		*(int*)(dest + offset) = ReadInt(adr + offset);
	}

}

/*
__forceinline void Read2DVector(uintptr_t adr, char* dest)
{
	*(char*)dest = ReadDouble(adr);
}

__forceinline void Read2DVector(uintptr_t adr, Vector2D& dest)
{
	*(char*)&dest = ReadDouble(adr);
}
__forceinline void Read2DVector(uintptr_t adr, float* dest)
{
	*dest = ReadDouble(adr);
}
*/

uintptr_t FindMemoryPattern(uintptr_t start, uintptr_t end, std::string strpattern, bool double_wide = true);
uintptr_t FindMemoryPattern(HANDLE ModuleHandle, std::string strpattern, bool double_wide = true);
uintptr_t FindMemoryPattern(HANDLE ModuleHandle, char* strpattern, size_t length, bool double_wide = true);
uintptr_t FindMemoryPattern_(HANDLE ModuleHandle, char* strpattern, bool double_wide = true);
uintptr_t FindMemoryPattern(uintptr_t start, uintptr_t end, const char *strpattern, size_t length, bool double_wide = true);
uint32_t FindPattern(HANDLE Module, std::string Pattern, uint32_t StartAddress, bool UseStartAddressAsStartPoint, bool SubtractBaseAddressFromStart, bool Dereference, uint32_t AddBeforeDereference, int DereferenceTimes, uint32_t *rawadr = 0, int sizeofvalue = 4);

inline void ReadVector(uintptr_t adr, Vector& dest)
{
	dest.x = ReadFloat(adr);
	dest.y = ReadFloat(adr + sizeof(float));
	dest.z = ReadFloat(adr + (sizeof(float) * 2));
}

inline void ReadAngle(uintptr_t adr, QAngle& dest)
{
	dest.x = ReadFloat(adr);
	dest.y = ReadFloat(adr + sizeof(float));
	dest.z = ReadFloat(adr + (sizeof(float) * 2));
}

inline void WriteAngle(uintptr_t adr, QAngle ang)
{
	WriteFloat(adr, ang.x);
	WriteFloat(adr + sizeof(float), ang.y);
	WriteFloat(adr + (sizeof(float) * 2), ang.z);
}

inline void WriteVector(uintptr_t adr, Vector ang)
{
	WriteFloat(adr, ang.x);
	WriteFloat(adr + sizeof(float), ang.y);
	WriteFloat(adr + (sizeof(float) * 2), ang.z);
}

extern HANDLE EngineHandle;
extern HANDLE ClientHandle;
extern HMODULE ThisDLLHandle;
extern HANDLE MatchmakingHandle;
extern HANDLE VPhysicsHandle;
extern HANDLE VSTDLIBHandle;
extern HANDLE SHADERAPIDX9Handle;
extern HANDLE DatacacheHandle;
extern HANDLE Tier0Handle;
extern HANDLE MaterialSystemHandle;
extern HANDLE VGUIMatSurfaceHandle;
extern HANDLE VGUI2Handle;
extern HANDLE StudioRenderHandle;
extern HANDLE FileSystemStdioHandle;
extern HANDLE ServerHandle;
class CBaseEntity;

inline BOOLEAN IsInSimulation(DWORD ClientState)
{
	return (BOOLEAN)ReadByte(ClientState + m_bInSimulation);
}

inline BOOLEAN IsHLTV(DWORD ClientState)
{
	return (BOOLEAN)ReadByte(ClientState + isHLTV);
}

#if 0
struct Bones {
	matrix3x4_t bones[128];
};

Vector getBonePosition(DWORD p, int boneId) {
	Bones bones = mem.Read<Bones>(p + 0x2698);

	Vector pos;
	pos.x = bones.bones[boneId][0][3];
	pos.y = bones.bones[boneId][1][3];
	pos.z = bones.bones[boneId][2][3];
	return pos;
}
#endif

inline void GetBonePos(DWORD BoneMatrix, int BoneID, Vector& BonePos)
{
	BonePos.x = ReadFloat((BoneMatrix + (0x30 * BoneID) + 0xC));//x
	BonePos.y = ReadFloat((BoneMatrix + (0x30 * BoneID) + 0x1C));//y
	BonePos.z = ReadFloat((BoneMatrix + (0x30 * BoneID) + 0x2C));//z
}

inline DWORD GetRadar()
{
	//return ReadInt(ReadInt((DWORD)ClientHandle + m_dwRadarBase) + m_dwRadarBasePointer);
	return ReadInt(*(DWORD*)RadarBaseAdr + m_dwRadarBasePointer);
}

//BROKEN
inline void GetMapPlayer_T(DWORD RadarAdr, int index, MapPlayer_t *dest)
{
	ReadByteArray(RadarAdr + sizeof(MapPlayer_t) * index, (char*)dest, sizeof(MapPlayer_t));
}

wchar_t* GetPlayerNameAddress(DWORD RadarAdr, int index);

void GetPlayerName(DWORD RadarAdr, int index, wchar_t* dest);

//TODO: BROKEN
inline int GetMaxPlayers(DWORD ClientState)
{
	return (int)ReadByte(ClientState + m_dwMaxPlayer);
}

#if 0
inline CBaseEntity* GetBaseEntity(int index)
{
	return (CBaseEntity*)ReadInt((DWORD)ClientHandle + m_dwEntityList + (index * 16));
}
#endif

inline DWORD GetGlowObjectManager()
{
	return GlowObjectManagerAdr;
	//return ((DWORD)ClientHandle + m_dwGlowObject);
}

inline DWORD GetGlowObject()
{
	return ReadInt(GetGlowObjectManager());
}

/*
//This doesn't get encrypted for some reason

static std::string Ranks[] =
{
	strenc("-"),
	strenc("Silver I"),
	strenc("Silver II"),
	strenc("Silver III"),
	strenc("Silver IV"),
	strenc("Silver Elite"),
	strenc("Silver Elite Master"),

	strenc("Gold Nova I"),
	strenc("Gold Nova II"),
	strenc("Gold Nova III"),
	strenc("Gold Nova Master"),
	strenc("Master Guardian I"),
	strenc("Master Guardian II"),

	strenc("Master Guardian Elite"),
	strenc("Distinguished Master Guardian"),
	strenc("Legendary Eagle"),
	strenc("Legendary Eagle Master"),
	strenc("Supreme Master First Class"),
	strenc("The Global Elite")
};
*/

inline int GetCompetitiveRank(DWORD CSPlayerResource, DWORD index)
{
	return ReadInt(CSPlayerResource + g_NetworkedVariables.Offsets.m_iCompetitiveRanking + index * 4);
}

extern char *silver1str;
extern char *silver2str;
extern char *silver3str;
extern char *silver4str;
extern char *silver5str;
extern char *silver6str;
extern char *nova1str;
extern char *nova2str;
extern char *nova3str;
extern char *nova4str;
extern char *mg1str;
extern char *mg2str;
extern char *mg3str;
extern char *mg4str;
extern char *lestr;
extern char *lemstr;
extern char *supremestr;
extern char *globalstr;

inline void GetCompetitiveRankString(char* dest, int destlength, int rank)
{
#if 0
	//strcpy(dest, Ranks[rank].c_str());
	switch (rank) {
		case 0:
			strcpy(dest, "-");
			break;
		case 1:
			DecStr(silver1str);
			strcpy(dest, silver1str);
			EncStr(silver1str);
			break;
		case 2:
			DecStr(silver2str);
			strcpy(dest, silver2str);
			EncStr(silver2str);
			break;
		case 3:
			DecStr(silver3str);
			strcpy(dest, silver3str);
			EncStr(silver3str);
			break;
		case 4:
			DecStr(silver4str);
			strcpy(dest, silver4str);
			EncStr(silver4str);
			break;
		case 5:
			DecStr(silver5str);
			strcpy(dest, silver5str);
			EncStr(silver5str);
			break;
		case 6:
			DecStr(silver6str);
			strcpy(dest, silver6str);
			EncStr(silver6str);
			break;
		case 7:
			DecStr(nova1str);
			strcpy(dest, nova1str);
			EncStr(nova1str);
			break;
		case 8:
			DecStr(nova2str);
			strcpy(dest, nova2str);
			EncStr(nova2str);
			break;
		case 9:
			DecStr(nova3str);
			strcpy(dest, nova3str);
			EncStr(nova3str);
			break;
		case 10:
			DecStr(nova4str);
			strcpy(dest, nova4str);
			EncStr(nova4str);
			break;
		case 11:
			DecStr(mg1str);
			strcpy(dest, mg1str);
			EncStr(mg1str);
			break;
		case 12:
			DecStr(mg2str);
			strcpy(dest, mg2str);
			EncStr(mg2str);
			break;
		case 13:
			DecStr(mg3str);
			strcpy(dest, mg3str);
			EncStr(mg3str);
			break;
		case 14:
			DecStr(mg4str);
			strcpy(dest, mg4str);
			EncStr(mg4str);
			break;
		case 15:
			DecStr(lestr);
			strcpy(dest, lestr);
			EncStr(lestr);
			break;
		case 16:
			DecStr(lemstr);
			strcpy(dest, lemstr);
			EncStr(lemstr);
			break;
		case 17:
			DecStr(supremestr);
			strcpy(dest, supremestr);
			EncStr(supremestr);
			break;
		case 18:
			DecStr(globalstr);
			strcpy(dest, globalstr);
			EncStr(globalstr);
			break;
	}
#endif
}

inline int GetCompetitiveWins(DWORD CSPlayerResource, DWORD index)
{
	return ReadInt(CSPlayerResource + g_NetworkedVariables.Offsets.m_iCompetitiveWins + index * 4);
}

inline void SpotPlayer(CBaseEntity* Entity)
{
	WriteByte((DWORD)Entity + g_NetworkedVariables.Offsets.m_bSpotted, 1);
}

extern RECT rc; //screen rectangle
extern HWND tWnd; //target game window
extern HWND hWnd; //hack window

inline bool PointIsInsideBox(Vector2D point, Vector2D topleft, int width, int height)
{
	Vector2D bottomright = { topleft.x + width, topleft.y + height };
	if ((point.x >= topleft.x && point.x <= bottomright.x)
		&& (point.y >= topleft.y && point.y <= bottomright.y))
		return true;
	return false;
}

inline bool PointIsInsideBox(Vector point, Vector topleft, int width, int height)
{
	Vector2D bottomright = { topleft.x + width, topleft.y + height };
	if ((point.x >= topleft.x && point.x <= bottomright.x)
		&& (point.y >= topleft.y && point.y <= bottomright.y))
		return true;
	return false;
}

inline bool PointIsInsideBox(Vector point, Vector2D topleft, int width, int height)
{
	Vector2D bottomright = { topleft.x + width, topleft.y + height };
	if ((point.x >= topleft.x && point.x <= bottomright.x)
		&& (point.y >= topleft.y && point.y <= bottomright.y))
		return true;
	return false;
}

inline bool PointIsInsideBox(Vector2D point, Vector2D topleft, Vector2D bottomright)
{
	if ((point.x >= topleft.x && point.x <= bottomright.x)
		&& (point.y >= topleft.y && point.y <= bottomright.y))
		return true;
	return false;
}

inline bool PointIsInsideBox(Vector point, Vector topleft, Vector bottomright)
{
	if ((point.x >= topleft.x && point.x <= bottomright.x)
		&& (point.y >= topleft.y && point.y <= bottomright.y))
		return true;
	return false;
}

inline bool PointIsInsideBox(Vector point, Vector2D topleft, Vector2D bottomright)
{
	if ((point.x >= topleft.x && point.x <= bottomright.x)
		&& (point.y >= topleft.y && point.y <= bottomright.y))
		return true;
	return false;
}

//Dylan added below
inline float DistanceBetweenPoints(Vector2D points1, Vector2D points2)
{
	float dx = points1.x - points2.x;
	float dy = points1.y - points2.y;
	float sqin = dx*dx + dy*dy;
	float sqout;
	SSESqrt(&sqout, &sqin);
	return sqout;//fabs(sqrtf(dx*dx + dy*dy));
}

inline float DistanceBetweenPoints(Vector points1, Vector points2)
{
	float dx = points1.x - points2.x;
	float dy = points1.y - points2.y;
	float sqin = dx*dx + dy*dy;
	float sqout;
	SSESqrt(&sqout, &sqin);
	return sqout; //fabs(sqrtf(dx*dx + dy*dy));
}

typedef struct
{
	float flMatrix[4][4];
}WorldToScreenMatrix_t;

BOOL WorldToScreenCapped(Vector& from, Vector& to);
BOOL WorldToScreen(Vector& from, Vector& to);

struct rgba_t
{
	float r;
	float g;
	float b;
	float a;

	bool operator==(const rgba_t& other) const
	{
		return r == other.r && g == other.g && b == other.b && a == other.a;
	}

	bool operator!=(const rgba_t& other) const
	{
		return !((*this) == other);
	}
};

struct GlowObject_t
{
	uint32_t pEntity;
	rgba_t rgba;
	uint8_t unk1[16];
	bool m_bRenderWhenOccluded;
	bool m_bRenderWhenUnoccluded;
	bool m_bFullBloom;
	uint8_t unk2[0xE];
};

void ScreenToWorld(int mousex, int mousey, float fov,
	const Vector& vecRenderOrigin,
	const QAngle& vecRenderAngles,
	Vector& vecPickingRay);

#endif