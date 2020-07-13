#pragma once

#include <Windows.h>
#include "memalloc.h"
#include "RadianEuler.h"

#include <xmmintrin.h>
#include <emmintrin.h>

#include "ErrorCodes.h"
#include "checksum_crc.h"

#include <cmath>
#include "Adriel/ImGui/imgui.h"

#include <string>

/* DIRECT X */
#include <d3d9.h>
#include <d3dx9.h>
#pragma comment( lib, "d3d9.lib" )
#pragma comment( lib, "d3dx9.lib" )

//#define _DUMPHASHES
#ifndef _DEBUGCONSOLE
//#define _DEBUGCONSOLE
#endif

#ifndef _DEBUGCONSOLE
#undef _DUMPHASHES
#endif

#ifdef _DEBUGCONSOLE
#define DEBUGPRINT(x, ...) DebugPrint(x, ##__VA_ARGS__)
#else
#define DEBUGPRINT(...)
#endif

#define ASSIGNXIFNZERODO(x, y) x = y; if (x)
#define ASSIGNVARANDIFNZERODO(x, y) auto x = y; if (x)
#define ASSIGNVARANDIFZERODO(x, y) auto x = y; if (!x)
#define DELETEPOINTER(x) if(x) { delete x; x = 0; }
#define ZEROPOINTER(x) if(x) x = 0;

#define NETVARPROXY(funcname) void funcname (CRecvProxyData *pData, void *pStruct, void *pOut)
#define CAST(cast, address, add) reinterpret_cast<cast>((uint32_t)address + (uint32_t)add)
#define CAM_HULL_OFFSET 9.0f // the size of the bounding hull used for collision checking

#ifndef MAX_OSPATH
#define	MAX_OSPATH		260
#endif

#define CMD_MAXBACKUP 64

extern int GetMaxprocessableUserCmds();
#define MAX_USER_CMDS GetMaxprocessableUserCmds()
#define MAX_TICKS_TO_CHOKE 14

bool Voice_IsRecording();

enum FileSystemSeek_t
{
	FILESYSTEM_SEEK_HEAD = SEEK_SET,
	FILESYSTEM_SEEK_CURRENT = SEEK_CUR,
	FILESYSTEM_SEEK_TAIL = SEEK_END,
};

enum
{
	FILESYSTEM_INVALID_FIND_HANDLE = -1
};

#define FILESYSTEM_INVALID_HANDLE 0

// Swap two of anything.
template <class T>
__forceinline void V_swap(T& x, T& y)
{
	T temp = x;
	x = y;
	y = temp;
}


struct Vector2D;
class CClientState;
extern CClientState* g_ClientState;
#include "Offsets.h" //for CModifiableUserCmd
#define Bits2Bytes(b) ((b+7)>>3)

inline void SinCos(float radians, float *sine, float *cosine)
{
	_asm
	{
		fld		DWORD PTR[radians]
		fsincos

		mov edx, DWORD PTR[cosine]
		mov eax, DWORD PTR[sine]

		fstp DWORD PTR[edx]
		fstp DWORD PTR[eax]
	}
}

inline double minss(double a, double b)
{
	// Branchless SSE min.
	_mm_store_sd(&a, _mm_min_sd(_mm_set_sd(a), _mm_set_sd(b)));
	return a;
}

inline double maxss(double a, double b)
{
	// Branchless SSE max.
	_mm_store_sd(&a, _mm_max_sd(_mm_set_sd(a), _mm_set_sd(b)));
	return a;
}

inline void clampdouble(double &val, double minval, double maxval)
{
	// Branchless SSE clamp.
	// return minss( maxss(val,minval), maxval );

	_mm_store_sd(&val, _mm_min_sd(_mm_max_sd(_mm_set_sd(val), _mm_set_sd(minval)), _mm_set_sd(maxval)));
}

inline double clampdoubler(double val, double minval, double maxval)
{
	// Branchless SSE clamp.
	// return minss( maxss(val,minval), maxval );

	_mm_store_sd(&val, _mm_min_sd(_mm_max_sd(_mm_set_sd(val), _mm_set_sd(minval)), _mm_set_sd(maxval)));
	return val;
}

inline float minss(float a, float b)
{
	// Branchless SSE min.
	_mm_store_ss(&a, _mm_min_ss(_mm_set_ss(a), _mm_set_ss(b)));
	return a;
}

inline float maxss(float a, float b)
{
	// Branchless SSE max.
	_mm_store_ss(&a, _mm_max_ss(_mm_set_ss(a), _mm_set_ss(b)));
	return a;
}

inline void clampfloat(float &val, float minval, float maxval)
{
	// Branchless SSE clamp.
	// return minss( maxss(val,minval), maxval );

	_mm_store_ss(&val, _mm_min_ss(_mm_max_ss(_mm_set_ss(val), _mm_set_ss(minval)), _mm_set_ss(maxval)));
}

inline float clampfloatr(float val, float minval, float maxval)
{
	// Branchless SSE clamp.
	// return minss( maxss(val,minval), maxval );

	_mm_store_ss(&val, _mm_min_ss(_mm_max_ss(_mm_set_ss(val), _mm_set_ss(minval)), _mm_set_ss(maxval)));

	return val;
}

inline void NormalizeFloat(float &f)
{
	//f -= floorf(f / 360.0f + 0.5f) * 360.0f;

	f = fmodf(f, 360.0f);
	if (f > 180)
	{
		f -= 360;
	}
	if (f < -180)
	{
		f += 360;
	}
}

inline float NormalizeFloatr(float f)
{
	//f -= floorf(f / 360.0f + 0.5f) * 360.0f;
	f = fmodf(f, 360.0f);
	if (f > 180)
	{
		f -= 360;
	}
	if (f < -180)
	{
		f += 360;
	}
	return f;
}

inline float ClampXr(float x)
{
	NormalizeFloat(x);

	clampfloat(x, -89.0f, 89.0f);

	return x;
}

inline float ClampYr(float y)
{
	NormalizeFloat(y);

	return y;
}

inline void ClampX(float& x)
{
	NormalizeFloat(x);

	clampfloat(x, -89.0f, 89.0f);
}

inline void ClampY(float& y)
{
	NormalizeFloat(y);
}

inline int InternalCheckDeclareClass(const char *pClassName, const char *pClassNameMatch, void *pTestPtr, void *pBasePtr)
{
	// This makes sure that casting from ThisClass to BaseClass works right. You'll get a compiler error if it doesn't
	// work at all, and you'll get a runtime error if you use multiple inheritance.
	//Assert(pTestPtr == pBasePtr);

	// This is triggered by IMPLEMENT_SERVER_CLASS. It does DLLClassName::CheckDeclareClass( #DLLClassName ).
	// If they didn't do a DECLARE_CLASS in DLLClassName, then it'll be calling its base class's version
	// and the class names won't match.
	//Assert((void*)pClassName == (void*)pClassNameMatch);
	return 0;
}

#define DECLARE_CLASS_GAMEROOT( className, baseClassName ) \
		typedef baseClassName BaseClass; \
		typedef className ThisClass; \
		template <typename T> friend int CheckDeclareClass_Access(T *, const char *pShouldBe); \
		static int CheckDeclareClass( const char *pShouldBe ) \
		{ \
			return InternalCheckDeclareClass( pShouldBe, #className, (ThisClass*)0xFFFFF, (BaseClass*)(ThisClass*)0xFFFFF ); \
}

#define DECLARE_CLASS_NOBASE( className )					typedef className ThisClass;
#define abstract_class class


//Todo: put me in area where compiler doesn't spew 1000 errors
inline void SSESqrt(float * __restrict pOut, float * __restrict pIn)
{
	_mm_store_ss(pOut, _mm_sqrt_ss(_mm_load_ss(pIn)));
	// compiles to movss, sqrtss, movss
}


#define DECL_ALIGN(x)			__declspec( align( x ) )

#define ALIGN4 DECL_ALIGN(4)
#define ALIGN8 DECL_ALIGN(8)
#define ALIGN16 DECL_ALIGN(16)
#define ALIGN32 DECL_ALIGN(32)
#define ALIGN128 DECL_ALIGN(128)

//extern float( *pfSqrt )( float x );

//#define FastSqrt(x)			(*pfSqrt)(x)


#define M_PI		3.14159265358979323846f
#define M_RADPI		57.295779513082f
#define M_PI_F		((float)(M_PI))	// Shouldn't collide with anything.
#define RAD2DEG( x  )  ( (float)(x) * (float)(180.f / M_PI_F) )
#define DEG2RAD( x  )  ( (float)(x) * (float)(M_PI_F / 180.f) )

#define IN_ATTACK				(1 << 0)
#define IN_JUMP					(1 << 1)
#define IN_DUCK					(1 << 2)
#define IN_FORWARD				(1 << 3)
#define IN_BACK					(1 << 4)
#define IN_USE					(1 << 5)
#define IN_CANCEL				(1 << 6)
#define IN_LEFT					(1 << 7)
#define IN_RIGHT				(1 << 8)
#define IN_MOVELEFT				(1 << 9)
#define IN_MOVERIGHT			(1 << 10)
#define IN_ATTACK2				(1 << 11)
#define IN_RUN					(1 << 12)
#define IN_RELOAD				(1 << 13)
#define IN_ALT1					(1 << 14)
#define IN_ALT2					(1 << 15)
#define IN_SCORE				(1 << 16)
#define IN_SPEED				(1 << 17)
#define IN_WALK					(1 << 18)
#define IN_ZOOM					(1 << 19)
#define IN_WEAPON1				(1 << 20)
#define IN_WEAPON2				(1 << 21)
#define IN_BULLRUSH				(1 << 22)

#define	FL_ONGROUND				(1 << 0)	
#define FL_DUCKING				(1 << 1)	
#define FL_ANIMDUCKING			(1 << 2)	// Player flag -- Player is in the process of crouching or uncrouching but could be in transition
#define	FL_WATERJUMP			(1 << 3)	
#define FL_ONTRAIN				(1 << 4) 
#define FL_INRAIN				(1 << 5)	
#define FL_FROZEN				(1 << 6) 
#define FL_ATCONTROLS			(1 << 7) 
#define	FL_CLIENT				(1 << 8)	
#define FL_FAKECLIENT			(1 << 9)	
#define	FL_INWATER				(1 << 10)
// NON-PLAYER SPECIFIC (i.e., not used by GameMovement or the client .dll ) -- Can still be applied to players, though
#define	FL_FLY					(1<<11)	// Changes the SV_Movestep() behavior to not need to be on ground
#define	FL_SWIM					(1<<12)	// Changes the SV_Movestep() behavior to not need to be on ground (but stay in water)
#define	FL_CONVEYOR				(1<<13)
#define	FL_NPC					(1<<14)
#define	FL_GODMODE				(1<<15)
#define	FL_NOTARGET				(1<<16)
#define	FL_AIMTARGET			(1<<17)	// set if the crosshair needs to aim onto the entity
#define	FL_PARTIALGROUND		(1<<18)	// not all corners are valid
#define FL_STATICPROP			(1<<19)	// Eetsa static prop!		
#define FL_GRAPHED				(1<<20) // worldgraph has this ent listed as something that blocks a connection
#define FL_GRENADE				(1<<21)
#define FL_STEPMOVEMENT			(1<<22)	// Changes the SV_Movestep() behavior to not do any processing
#define FL_DONTTOUCH			(1<<23)	// Doesn't generate touch functions, generates Untouch() for anything it was touching when this flag was set
#define FL_BASEVELOCITY			(1<<24)	// Base velocity has been applied this frame (used to convert base velocity into momentum)
#define FL_WORLDBRUSH			(1<<25)	// Not moveable/removeable brush entity (really part of the world, but represented as an entity for transparency or something)
#define FL_OBJECT				(1<<26) // Terrible name. This is an object that NPCs should see. Missiles, for example.
#define FL_KILLME				(1<<27)	// This entity is marked for death -- will be freed by game DLL
#define FL_ONFIRE				(1<<28)	// You know...
#define FL_DISSOLVING			(1<<29) // We're dissolving!
#define FL_TRANSRAGDOLL			(1<<30) // In the process of turning into a client side ragdoll.
#define FL_UNBLOCKABLE_BY_PLAYER (1<<31) // pusher that can't be blocked by the player

#define HIDEHUD_SCOPE			(1 << 11)

#define MAX_AREA_STATE_BYTES		32
#define MAX_AREA_PORTAL_STATE_BYTES 24

#define  Assert( _exp )										((void)0)
#define  AssertAligned( ptr )								((void)0)
#define  AssertOnce( _exp )									((void)0)
#define  AssertMsg( _exp, _msg )							((void)0)
#define  AssertMsgOnce( _exp, _msg )						((void)0)
#define  AssertFunc( _exp, _f )								((void)0)
#define  AssertEquals( _exp, _expectedValue )				((void)0)
#define  AssertFloatEquals( _exp, _expectedValue, _tol )	((void)0)
#define  Verify( _exp )										(_exp)
#define  VerifyEquals( _exp, _expectedValue )           	(_exp)

#define TEXTURE_GROUP_LIGHTMAP						"Lightmaps"
#define TEXTURE_GROUP_WORLD							"World textures"
#define TEXTURE_GROUP_MODEL							"Model textures"
#define TEXTURE_GROUP_VGUI							"VGUI textures"
#define TEXTURE_GROUP_PARTICLE						"Particle textures"
#define TEXTURE_GROUP_DECAL							"Decal textures"
#define TEXTURE_GROUP_SKYBOX						"SkyBox textures"
#define TEXTURE_GROUP_CLIENT_EFFECTS				"ClientEffect textures"
#define TEXTURE_GROUP_OTHER							"Other textures"
#define TEXTURE_GROUP_PRECACHED						"Precached"				// TODO: assign texture groups to the precached materials
#define TEXTURE_GROUP_CUBE_MAP						"CubeMap textures"
#define TEXTURE_GROUP_RENDER_TARGET					"RenderTargets"
#define TEXTURE_GROUP_UNACCOUNTED					"Unaccounted textures"	// Textures that weren't assigned a texture group.
//#define TEXTURE_GROUP_STATIC_VERTEX_BUFFER		"Static Vertex"
#define TEXTURE_GROUP_STATIC_INDEX_BUFFER			"Static Indices"
#define TEXTURE_GROUP_STATIC_VERTEX_BUFFER_DISP		"Displacement Verts"
#define TEXTURE_GROUP_STATIC_VERTEX_BUFFER_COLOR	"Lighting Verts"
#define TEXTURE_GROUP_STATIC_VERTEX_BUFFER_WORLD	"World Verts"
#define TEXTURE_GROUP_STATIC_VERTEX_BUFFER_MODELS	"Model Verts"
#define TEXTURE_GROUP_STATIC_VERTEX_BUFFER_OTHER	"Other Verts"
#define TEXTURE_GROUP_DYNAMIC_INDEX_BUFFER			"Dynamic Indices"
#define TEXTURE_GROUP_DYNAMIC_VERTEX_BUFFER			"Dynamic Verts"
#define TEXTURE_GROUP_DEPTH_BUFFER					"DepthBuffer"
#define TEXTURE_GROUP_VIEW_MODEL					"ViewModel"
#define TEXTURE_GROUP_PIXEL_SHADERS					"Pixel Shaders"
#define TEXTURE_GROUP_VERTEX_SHADERS				"Vertex Shaders"
#define TEXTURE_GROUP_RENDER_TARGET_SURFACE			"RenderTarget Surfaces"
#define TEXTURE_GROUP_MORPH_TARGETS					"Morph Targets"

#define	CONTENTS_EMPTY			0		// No contents

#define	CONTENTS_SOLID			0x1		// an eye is never valid in a solid
#define	CONTENTS_WINDOW			0x2		// translucent, but not watery (glass)
#define	CONTENTS_AUX			0x4
#define	CONTENTS_GRATE			0x8		// alpha-tested "grate" textures.  Bullets/sight pass through, but solids don't
#define	CONTENTS_SLIME			0x10
#define	CONTENTS_WATER			0x20
#define	CONTENTS_BLOCKLOS		0x40	// block AI line of sight
#define CONTENTS_OPAQUE			0x80	// things that cannot be seen through (may be non-solid though)
#define	LAST_VISIBLE_CONTENTS	CONTENTS_OPAQUE

#define ALL_VISIBLE_CONTENTS (LAST_VISIBLE_CONTENTS | (LAST_VISIBLE_CONTENTS-1))

#define CONTENTS_TESTFOGVOLUME	0x100
#define CONTENTS_UNUSED			0x200	

// unused 
// NOTE: If it's visible, grab from the top + update LAST_VISIBLE_CONTENTS
// if not visible, then grab from the bottom.
// CONTENTS_OPAQUE + SURF_NODRAW count as CONTENTS_OPAQUE (shadow-casting toolsblocklight textures)
#define CONTENTS_BLOCKLIGHT		0x400

#define CONTENTS_TEAM1			0x800	// per team contents used to differentiate collisions 
#define CONTENTS_TEAM2			0x1000	// between players and objects on different teams

// ignore CONTENTS_OPAQUE on surfaces that have SURF_NODRAW
#define CONTENTS_IGNORE_NODRAW_OPAQUE	0x2000

// hits entities which are MOVETYPE_PUSH (doors, plats, etc.)
#define CONTENTS_MOVEABLE		0x4000

// remaining contents are non-visible, and don't eat brushes
#define	CONTENTS_AREAPORTAL		0x8000

#define	CONTENTS_PLAYERCLIP		0x10000
#define	CONTENTS_MONSTERCLIP	0x20000

// currents can be added to any other contents, and may be mixed
#define	CONTENTS_CURRENT_0		0x40000
#define	CONTENTS_CURRENT_90		0x80000
#define	CONTENTS_CURRENT_180	0x100000
#define	CONTENTS_CURRENT_270	0x200000
#define	CONTENTS_CURRENT_UP		0x400000
#define	CONTENTS_CURRENT_DOWN	0x800000

#define	CONTENTS_ORIGIN			0x1000000	// removed before bsping an entity

#define	CONTENTS_MONSTER		0x2000000	// should never be on a brush, only in game
#define	CONTENTS_DEBRIS			0x4000000
#define	CONTENTS_DETAIL			0x8000000	// brushes to be added after vis leafs
#define	CONTENTS_TRANSLUCENT	0x10000000	// auto set if any surface has trans
#define	CONTENTS_LADDER			0x20000000
#define CONTENTS_HITBOX			0x40000000	// use accurate hitboxes on trace

// NOTE: These are stored in a short in the engine now.  Don't use more than 16 bits
#define	SURF_LIGHT		0x0001		// value will hold the light strength
#define	SURF_SKY2D		0x0002		// don't draw, indicates we should skylight + draw 2d sky but not draw the 3D skybox
#define	SURF_SKY		0x0004		// don't draw, but add to skybox
#define	SURF_WARP		0x0008		// turbulent water warp
#define	SURF_TRANS		0x0010
#define SURF_NOPORTAL	0x0020	// the surface can not have a portal placed on it
#define	SURF_TRIGGER	0x0040	// FIXME: This is an xbox hack to work around elimination of trigger surfaces, which breaks occluders
#define	SURF_NODRAW		0x0080	// don't bother referencing the texture

#define	SURF_HINT		0x0100	// make a primary bsp splitter

#define	SURF_SKIP		0x0200	// completely ignore, allowing non-closed brushes
#define SURF_NOLIGHT	0x0400	// Don't calculate light
#define SURF_BUMPLIGHT	0x0800	// calculate three lightmaps for the surface for bumpmapping
#define SURF_NOSHADOWS	0x1000	// Don't receive shadows
#define SURF_NODECALS	0x2000	// Don't receive decals
#define SURF_NOPAINT	SURF_NODECALS	// the surface can not have paint placed on it
#define SURF_NOCHOP		0x4000	// Don't subdivide patches on this surface 
#define SURF_HITBOX		0x8000	// surface is part of a hitbox

#define	MASK_ALL					(0xFFFFFFFF)
// everything that is normally solid
#define	MASK_SOLID					(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE)
// everything that blocks player movement
#define	MASK_PLAYERSOLID			(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE)
// blocks npc movement
#define	MASK_NPCSOLID				(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE)
// blocks fluid movement
#define	MASK_NPCFLUID				(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
// water physics in these contents
#define	MASK_WATER					(CONTENTS_WATER|CONTENTS_MOVEABLE|CONTENTS_SLIME)
// everything that blocks lighting
#define	MASK_OPAQUE					(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_OPAQUE)
// everything that blocks lighting, but with monsters added.
#define MASK_OPAQUE_AND_NPCS		(MASK_OPAQUE|CONTENTS_MONSTER)
// everything that blocks line of sight for AI
#define MASK_BLOCKLOS				(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_BLOCKLOS)
// everything that blocks line of sight for AI plus NPCs
#define MASK_BLOCKLOS_AND_NPCS		(MASK_BLOCKLOS|CONTENTS_MONSTER)
// everything that blocks line of sight for players
#define	MASK_VISIBLE					(MASK_OPAQUE|CONTENTS_IGNORE_NODRAW_OPAQUE)
// everything that blocks line of sight for players, but with monsters added.
#define MASK_VISIBLE_AND_NPCS		(MASK_OPAQUE_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE)
// bullets see these as solid
#define	MASK_SHOT					(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEBRIS|CONTENTS_GRATE|CONTENTS_HITBOX)
// bullets see these as solid, except monsters (world+brush only)
#define MASK_SHOT_BRUSHONLY			(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_DEBRIS)
// non-raycasted weapons see this as solid (includes grates)
#define MASK_SHOT_HULL				(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEBRIS|CONTENTS_GRATE)
// hits solids (not grates) and passes through everything else
#define MASK_SHOT_PORTAL			(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTER)
// everything normally solid, except monsters (world+brush only)
#define MASK_SOLID_BRUSHONLY		(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_GRATE)
// everything normally solid for player movement, except monsters (world+brush only)
#define MASK_PLAYERSOLID_BRUSHONLY	(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_PLAYERCLIP|CONTENTS_GRATE)
// everything normally solid for npc movement, except monsters (world+brush only)
#define MASK_NPCSOLID_BRUSHONLY		(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTERCLIP|CONTENTS_GRATE)
// just the world, used for route rebuilding
#define MASK_NPCWORLDSTATIC			(CONTENTS_SOLID|CONTENTS_WINDOW|CONTENTS_MONSTERCLIP|CONTENTS_GRATE)
// just the world, used for route rebuilding
#define MASK_NPCWORLDSTATIC_FLUID	(CONTENTS_SOLID|CONTENTS_WINDOW|CONTENTS_MONSTERCLIP)
// These are things that can split areaportals
#define MASK_SPLITAREAPORTAL		(CONTENTS_WATER|CONTENTS_SLIME)

// UNDONE: This is untested, any moving water
#define MASK_CURRENT				(CONTENTS_CURRENT_0|CONTENTS_CURRENT_90|CONTENTS_CURRENT_180|CONTENTS_CURRENT_270|CONTENTS_CURRENT_UP|CONTENTS_CURRENT_DOWN)

// everything that blocks corpse movement
// UNDONE: Not used yet / may be deleted
#define	MASK_DEADSOLID				(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_GRATE)

struct colorVec
{
	unsigned r, g, b, a;
};

enum
{
	PITCH = 0,	// up / down
	YAW,		// left / right
	ROLL		// fall over
};

enum FontRenderFlag_t
{
	FONT_LEFT = 0,
	FONT_RIGHT = 1,
	FONT_CENTER = 2
};

enum ItemDefinitionIndex : short
{
	WEAPON_NONE = 0,
	WEAPON_DEAGLE,
	WEAPON_ELITE,
	WEAPON_FIVESEVEN,
	WEAPON_GLOCK,
	WEAPON_AK47 = 7,
	WEAPON_AUG,
	WEAPON_AWP,
	WEAPON_FAMAS,
	WEAPON_G3SG1,
	WEAPON_GALILAR = 13,
	WEAPON_M249,
	WEAPON_M4A1 = 16,
	WEAPON_MAC10,
	WEAPON_P90 = 19,
	WEAPON_MP5SD = 23,
	WEAPON_UMP45,
	WEAPON_XM1014,
	WEAPON_BIZON,
	WEAPON_MAG7,
	WEAPON_NEGEV,
	WEAPON_SAWEDOFF,
	WEAPON_TEC9,
	WEAPON_TASER,
	WEAPON_HKP2000,
	WEAPON_MP7,
	WEAPON_MP9,
	WEAPON_NOVA,
	WEAPON_P250,
	WEAPON_SCAR20 = 38,
	WEAPON_SG556,
	WEAPON_SSG08,
	WEAPON_KNIFEGG,
	WEAPON_KNIFE,
	WEAPON_FLASHBANG,
	WEAPON_HEGRENADE,
	WEAPON_SMOKEGRENADE,
	WEAPON_MOLOTOV,
	WEAPON_DECOY,
	WEAPON_INCGRENADE,
	WEAPON_C4,
	WEAPON_HEALTHSHOT = 57,
	WEAPON_KNIFE_T = 59,
	WEAPON_M4A1_SILENCER,
	WEAPON_USP_SILENCER,
	WEAPON_CZ75A = 63,
	WEAPON_REVOLVER,
	WEAPON_TAGRENADE = 68,
	WEAPON_FISTS,
	WEAPON_BREACHCHARGE,
	WEAPON_TABLET = 72,
	WEAPON_MELEE = 74,
	WEAPON_AXE,
	WEAPON_HAMMER,
	WEAPON_SPANNER = 78,
	WEAPON_KNIFE_GHOST = 80,
	WEAPON_FIREBOMB,
	WEAPON_DIVERSION,
	WEAPON_FRAG_GRENADE,
	WEAPON_SNOWBALL,
	WEAPON_BUMPMINE,
	WEAPON_KNIFE_BAYONET = 500,
	WEAPON_KNIFE_FLIP = 505,
	WEAPON_KNIFE_GUT,
	WEAPON_KNIFE_KARAMBIT,
	WEAPON_KNIFE_M9_BAYONET,
	WEAPON_KNIFE_TACTICAL,
	WEAPON_KNIFE_FALCHION = 512,
	WEAPON_KNIFE_SURVIVAL_BOWIE = 514,
	WEAPON_KNIFE_BUTTERFLY,
	WEAPON_KNIFE_PUSH,
	WEAPON_KNIFE_URSUS = 519,
	WEAPON_KNIFE_GYPSY_JACKKNIFE,
	WEAPON_KNIFE_STILETTO = 522,
	WEAPON_KNIFE_WIDOWMAKER = 523,
	WEAPON_KNIFE_SKELETON = 525,
	GLOVE_STUDDED_BLOODHOUND = 5027,
	GLOVE_T_SIDE = 5028,
	GLOVE_CT_SIDE = 5029,
	GLOVE_SPORTY = 5030,
	GLOVE_SLICK = 5031,
	GLOVE_LEATHER_WRAP = 5032,
	GLOVE_MOTORCYCLE = 5033,
	GLOVE_SPECIALIST = 5034,
	GLOVE_HYDRA = 5035
};

constexpr int AchievementId[167]
{
	1001, 1002, 1004, 1005, 1006, 1007, 1008, 1009, 1010, 1011, 1012, 1013, 1014, 1015,
	2001, 2002, 2003, 2004, 2005, 3001, 3002, 3003, 3005, 3006, 3007, 3008, 3009, 3010,
	3011, 3012, 3013, 3014, 3015, 3016, 3017, 3018, 3019, 3020, 3021, 3022, 3023, 3024,
	3025, 3026, 3027, 3028, 3029, 3030, 3031, 3032, 3033, 3034, 3035, 3036, 3037, 3038,
	3039, 3040, 3044, 3046, 3047, 3048, 3049, 3050, 4001, 4003, 4005, 4006, 4007, 4008,
	4009, 4010, 4011, 4012, 4013, 4014, 4015, 4019, 4020, 4022, 4024, 4025, 4026, 4027,
	4030, 4031, 4032, 4033, 4035, 4036, 4037, 4038, 4039, 4040, 4041, 4042, 4043, 4044,
	4045, 4046, 4047, 4048, 5001, 5002, 5003, 5004, 5005, 5006, 5007, 5008, 5009, 5010,
	5011, 5012, 5013, 5014, 5015, 5016, 5017, 5018, 5019, 5020, 5021, 5022, 5023, 5024,
	5025, 5026, 5027, 5029, 5031, 5032, 5033, 5034, 5035, 5036, 5037, 5039, 5040, 5041,
	5042, 5044, 6004, 6006, 6007, 6010, 6011, 6012, 6013, 6018, 6019, 6020, 6021, 6022,
	6023, 6024, 6030, 6031, 6032, 6033, 6034, 6035, 6036, 6037, 6038, 6039, 6040
};

struct player_info_t
{
	char __pad0[0x8];

	int xuidlow;
	int xuidhigh;

	char name[128];
	int userid;
	char guid[33];

	char __pad1[0x17B];
};

class VectorByValue;
class Vector;

class VectorDouble
{
public:
	double x, y, z;

	VectorDouble(void);
	VectorDouble(double X, double Y, double Z);
	void Init(double ix = 0.0f, double iy = 0.0f, double iz = 0.0f);

	double operator[](int i) const;
	double& operator[](int i);
	// Base address...
	double* Base();
	double const* Base() const;
	inline void Zero();
	bool operator==(const VectorDouble& v) const;
	bool operator!=(const VectorDouble& v) const;
	__forceinline VectorDouble&	operator+=(const VectorDouble &v);
	__forceinline VectorDouble&	operator-=(const VectorDouble &v);
	__forceinline VectorDouble&	operator*=(const VectorDouble &v);
	__forceinline VectorDouble&	operator*=(double s);
	__forceinline VectorDouble&	operator/=(const VectorDouble &v);
	__forceinline VectorDouble&	operator/=(double s);
	__forceinline VectorDouble&	operator+=(double fl);
	__forceinline VectorDouble&	operator-=(double fl);
	inline double	Length() const;
	inline bool IsNaN()
	{
		return isnan(x) || isnan(y) || isnan(z);
	}
	__forceinline double LengthSqr(void) const
	{
		CHECK_VALID(*this);
		return (this->x*this->x + this->y*this->y + this->z*this->z);
	}
	bool IsZero(double tolerance = 0.01f) const
	{
		return (x > -tolerance && x < tolerance &&
			y > -tolerance && y < tolerance &&
			z > -tolerance && z < tolerance);
	}
	double	NormalizeInPlace();
	VectorDouble	Normalize();
	VectorDouble	NormalizeSqr();
	__forceinline double	DistToSqr(const VectorDouble &vOther) const;
	__forceinline double	Dist(const VectorDouble &vOther) const;
	double	Dot(const VectorDouble& vOther) const;
	double	Dot(const double* fOther) const;
	double	LengthToOtherX(const VectorDouble& vOther) const;
	double   LengthToOtherY(const VectorDouble& vOther) const;
	double   LengthToOtherZ(const VectorDouble& vOther) const;
	double	Length2D(void) const;
	double	Length2DSqr(void) const;
	void    MultAdd(VectorDouble& vOther, double fl);
	VectorDouble& operator=(const VectorDouble &vOther);
	operator VectorByValue &() { return *((VectorByValue *)(this)); }
	operator const VectorByValue &() const { return *((const VectorByValue *)(this)); }
	VectorDouble	operator-(void) const;

	VectorDouble	operator-(const VectorDouble& v) const;
	VectorDouble	operator+(const VectorDouble& v) const;
	VectorDouble	operator*(const VectorDouble& v) const;
	VectorDouble	operator/(const VectorDouble& v) const;
	VectorDouble	operator*(double fl) const;
	VectorDouble	operator/(double fl) const;

	VectorDouble(const VectorDouble& vOther);
};

class Vector
{
public:
	float x, y, z;
	Vector(void);
	Vector(float X, float Y, float Z);
	void Init(float ix = 0.0f, float iy = 0.0f, float iz = 0.0f);
	__inline bool IsValid() const
	{
		return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
	}
	float operator[](int i) const;
	float& operator[](int i);
	// Base address...
	float* Base();
	float const* Base() const;
	inline void Zero();
	bool operator==(const Vector& v) const;
	bool operator!=(const Vector& v) const;
	VectorDouble AsVectorDouble() { return VectorDouble(x, y, z); }
	__forceinline Vector&	operator+=(const Vector &v);
	__forceinline Vector&	operator-=(const Vector &v);
	__forceinline Vector&	operator*=(const Vector &v);
	__forceinline Vector&	operator*=(float s);
	__forceinline Vector&	operator/=(const Vector &v);
	__forceinline Vector&	operator/=(float s);
	__forceinline Vector&	operator+=(float fl);
	__forceinline Vector&	operator-=(float fl);
	inline float	Length() const;
	inline bool IsFinite() const
	{
		return isfinite(x) && isfinite(y) && isfinite(z);
	}
	inline bool IsNaN() const
	{
		return isnan(x) || isnan(y) || isnan(z);
	}
	__forceinline float LengthSqr(void) const
	{
		CHECK_VALID(*this);
		return (this->x*this->x + this->y*this->y + this->z*this->z);
	}
	bool IsZero(float tolerance = 0.01f) const
	{
		return (x > -tolerance && x < tolerance &&
			y > -tolerance && y < tolerance &&
			z > -tolerance && z < tolerance);
	}
	float	NormalizeInPlace();
	//Vector	Normalize();
	//Vector	NormalizeSqr();
	__forceinline float	DistToSqr(const Vector &vOther) const;
	__forceinline float	Dist(const Vector &vOther) const;
	__forceinline float	Dist2D(const Vector &vOther) const;
	float	Dot(const Vector& vOther) const;
	float	Dot(const float* fOther) const;
	float	LengthToOtherX(const Vector& vOther) const;
	float   LengthToOtherY(const Vector& vOther) const;
	float   LengthToOtherZ(const Vector& vOther) const;
	float	Length2D(void) const;
	float	Length2DSqr(void) const;
	void    MultAdd(Vector& vOther, float fl);
	Vector2D& AsVector2D();
	const Vector2D& AsVector2D() const;
	Vector Cross(const Vector& vOther) const;
	Vector& operator=(const Vector &vOther);
	operator VectorByValue &() { return *((VectorByValue *)(this)); }
	operator const VectorByValue &() const { return *((const VectorByValue *)(this)); }
	Vector	operator-(void) const;

	Vector	operator-(const Vector& v) const;
	Vector	operator+(const Vector& v) const;
	Vector	operator*(const Vector& v) const;
	Vector	operator/(const Vector& v) const;
	Vector	operator*(float fl) const;
	Vector	operator/(float fl) const;

	Vector(const Vector& vOther);
};

inline void VectorLerp(const Vector& src1, const Vector& src2, float t, Vector& dest)
{
	CHECK_VALID(src1);
	CHECK_VALID(src2);
	dest.x = src1.x + (src2.x - src1.x) * t;
	dest.y = src1.y + (src2.y - src1.y) * t;
	dest.z = src1.z + (src2.z - src1.z) * t;
}

//===============================================
inline void Vector::Init(float ix, float iy, float iz)
{
	x = ix; y = iy; z = iz;
	CHECK_VALID(*this);
}
//===============================================
inline Vector::Vector(float X, float Y, float Z)
{
	x = X; y = Y; z = Z;
	CHECK_VALID(*this);
}

inline Vector::Vector(const Vector &vOther)
{
	CHECK_VALID(vOther);
	x = vOther.x; y = vOther.y; z = vOther.z;
}

//===============================================
inline Vector::Vector(void) { }
//===============================================
inline void Vector::Zero()
{
	x = y = z = 0.0f;
}
//===============================================
inline void VectorClear(Vector& a)
{
	a.x = a.y = a.z = 0.0f;
}

inline Vector Vector::operator-(void) const
{
	Vector ret(-x, -y, -z);
	return ret;
}

//===============================================
inline Vector& Vector::operator=(const Vector &vOther)
{
	CHECK_VALID(vOther);
	x = vOther.x; y = vOther.y; z = vOther.z;
	return *this;
}

//===============================================
inline float& Vector::operator[](int i)
{
	//Assert((i >= 0) && (i < 3));
	return ((float*)this)[i];
}
//===============================================
inline float Vector::operator[](int i) const
{
	//Assert((i >= 0) && (i < 3));
	return ((float*)this)[i];
}
//===============================================
inline bool Vector::operator==(const Vector& src) const
{
	CHECK_VALID(src);
	CHECK_VALID(*this);
	return (src.x == x) && (src.y == y) && (src.z == z);
}
//===============================================
inline bool Vector::operator!=(const Vector& src) const
{
	CHECK_VALID(src);
	CHECK_VALID(*this);
	return (src.x != x) || (src.y != y) || (src.z != z);
}
//===============================================
__forceinline void VectorCopy(const Vector& src, Vector& dst)
{
	CHECK_VALID(src);
	dst.x = src.x;
	dst.y = src.y;
	dst.z = src.z;
}
//===============================================
__forceinline  Vector& Vector::operator+=(const Vector& v)
{
	CHECK_VALID(*this);
	CHECK_VALID(v);
	x += v.x; y += v.y; z += v.z;
	return *this;
}
//===============================================
__forceinline  Vector& Vector::operator-=(const Vector& v)
{
	CHECK_VALID(*this);
	CHECK_VALID(v);
	x -= v.x; y -= v.y; z -= v.z;
	return *this;
}
//===============================================
__forceinline  Vector& Vector::operator*=(float fl)
{
	x *= fl;
	y *= fl;
	z *= fl;
	CHECK_VALID(*this);
	return *this;
}
//===============================================
__forceinline  Vector& Vector::operator*=(const Vector& v)
{
	CHECK_VALID(v);
	x *= v.x;
	y *= v.y;
	z *= v.z;
	CHECK_VALID(*this);
	return *this;
}
//===============================================
__forceinline Vector&	Vector::operator+=(float fl)
{
	x += fl;
	y += fl;
	z += fl;
	CHECK_VALID(*this);
	return *this;
}
//===============================================
__forceinline Vector&	Vector::operator-=(float fl)
{
	x -= fl;
	y -= fl;
	z -= fl;
	CHECK_VALID(*this);
	return *this;
}
//===============================================
__forceinline  Vector& Vector::operator/=(float fl)
{
	//Assert(fl != 0.0f);
	float oofl = 1.0f / fl;
	x *= oofl;
	y *= oofl;
	z *= oofl;
	CHECK_VALID(*this);
	return *this;
}
//===============================================
__forceinline  Vector& Vector::operator/=(const Vector& v)
{
	CHECK_VALID(v);
	//Assert(v.x != 0.0f && v.y != 0.0f && v.z != 0.0f);
	x /= v.x;
	y /= v.y;
	z /= v.z;
	CHECK_VALID(*this);
	return *this;
}
//===============================================
inline float Vector::Length(void) const
{
	CHECK_VALID(*this);

	float root = 0.0f;

	float sqsr = x * x + y * y + z * z;

	__asm sqrtss xmm0, sqsr
	__asm movss root, xmm0

	return root;
}
//===============================================
inline float Vector::Length2D(void) const
{
	CHECK_VALID(*this);

	float root = 0.0f;

	float sqst = x * x + y * y;

	__asm
	{
		sqrtss xmm0, sqst
		movss root, xmm0
	}

	return root;
}

inline float Vector::LengthToOtherX(const Vector& vOther) const
{
	float dot = vOther.x * x;
	float sq;
	SSESqrt(&sq, &dot);
	return sq;
}

inline float Vector::LengthToOtherY(const Vector& vOther) const
{
	float dot = vOther.y * y;
	float sq;
	SSESqrt(&sq, &dot);
	return sq;
}

inline float Vector::LengthToOtherZ(const Vector& vOther) const
{
	float dot = vOther.z * z;
	float sq;
	SSESqrt(&sq, &dot);
	return sq;
}

//===============================================
inline float Vector::Length2DSqr(void) const
{
	return (x*x + y * y);
}

inline void Vector::MultAdd(Vector& vOther, float fl)
{
	x += fl * vOther.x;
	y += fl * vOther.y;
	z += fl * vOther.z;
}

//===============================================
inline Vector CrossProduct(const Vector& a, const Vector& b)
{
	return Vector(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x);
}

inline void CrossProduct(const Vector& a, const Vector& b, Vector& out)
{
	out = { a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x };
}
//===============================================
float Vector::DistToSqr(const Vector &vOther) const
{
	Vector delta;

	delta.x = x - vOther.x;
	delta.y = y - vOther.y;
	delta.z = z - vOther.z;

	return delta.LengthSqr();
}

float Vector::Dist(const Vector &vOther) const
{
	Vector delta;

	delta.x = x - vOther.x;
	delta.y = y - vOther.y;
	delta.z = z - vOther.z;

	return delta.Length();
}

float Vector::Dist2D(const Vector &vOther) const
{
	Vector delta;

	delta.x = x - vOther.x;
	delta.y = y - vOther.y;

	return delta.Length2D();
}

/*inline Vector Vector::Normalize()
{
Vector vector;
float length = this->Length();

if (length != 0) {
vector.x = x / length;
vector.y = y / length;
vector.z = z / length;
}
else
vector.x = vector.y = 0.0f; vector.z = 1.0f;

return vector;
}*/

inline Vector Vector::Cross(const Vector& vOther) const
{
	Vector res;
	CrossProduct(*this, vOther, res);
	return res;
}

/*inline Vector Vector::NormalizeSqr()
{
Vector vector;
float length = this->Length();

if (length != 0) {
vector.x = x / length;
vector.y = y / length;
vector.z = z / length;
}
else
vector.x = vector.y = 0.0f; vector.z = 1.0f;

return vector * vector;
}
*/
//===============================================
inline float Vector::NormalizeInPlace()
{
	Vector& v = *this;

	float iradius = 1.f / (this->Length() + 1.192092896e-07F); //FLT_EPSILON

	v.x *= iradius;
	v.y *= iradius;
	v.z *= iradius;
	return iradius;
}

//===============================================
inline float VectorNormalize(Vector& v)
{
#if 0
	//Assert(v.IsValid());
	float l = v.Length();
	if (l != 0.0f)
	{
		v /= l;
	}
	else
	{
		v.x = v.y = 0.0f; v.z = 1.0f;
	}
	return l;
#endif
	float radius = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);

	// FLT_EPSILON is added to the radius to eliminate the possibility of divide by zero.
	float iradius = 1.f / (radius + 1.92092896e-07F);

	v.x *= iradius;
	v.y *= iradius;
	v.z *= iradius;

	return radius;
}

inline Vector VectorNormalizeReturn(Vector& v)
{
	float radius = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);

	// FLT_EPSILON is added to the radius to eliminate the possibility of divide by zero.
	float iradius = 1.f / (radius + FLT_EPSILON);
	return (v * iradius);
}

inline float VectorNormalizeSqr(Vector& v)
{
	//Assert(v.IsValid());
	float l = v.Length();
	if (l != 0.0f)
	{
		v /= l;
	}
	else
	{
		v.x = v.y = 0.0f; v.z = 1.0f;
	}
	return l * l;
}
//===============================================
FORCEINLINE float VectorNormalize(float * v)
{
	return VectorNormalize(*(reinterpret_cast<Vector *>(v)));
}

//===============================================
inline Vector Vector::operator+(const Vector& v) const
{
	Vector res;
	res.x = x + v.x;
	res.y = y + v.y;
	res.z = z + v.z;
	return res;
}

//===============================================
inline Vector Vector::operator-(const Vector& v) const
{
	Vector res;
	res.x = x - v.x;
	res.y = y - v.y;
	res.z = z - v.z;
	return res;
}
//===============================================
inline Vector Vector::operator*(float fl) const
{
	Vector res;
	res.x = x * fl;
	res.y = y * fl;
	res.z = z * fl;
	return res;
}
//===============================================
inline Vector Vector::operator*(const Vector& v) const
{
	Vector res;
	res.x = x * v.x;
	res.y = y * v.y;
	res.z = z * v.z;
	return res;
}
//===============================================
inline Vector Vector::operator/(float fl) const
{
	Vector res;
	res.x = x / fl;
	res.y = y / fl;
	res.z = z / fl;
	return res;
}
//===============================================
inline Vector Vector::operator/(const Vector& v) const
{
	Vector res;
	res.x = x / v.x;
	res.y = y / v.y;
	res.z = z / v.z;
	return res;
}
inline float Vector::Dot(const Vector& vOther) const
{
	const Vector& a = *this;

	return(a.x*vOther.x + a.y*vOther.y + a.z*vOther.z);
}

inline float Vector::Dot(const float* fOther) const
{
	const Vector& a = *this;

	return(a.x*fOther[0] + a.y*fOther[1] + a.z*fOther[2]);
}

inline float* Vector::Base()
{
	return (float*)this;
}

inline float const* Vector::Base() const
{
	return (float const*)this;
}

inline void VectorLerp(const VectorDouble& src1, const VectorDouble& src2, double t, VectorDouble& dest)
{
	CHECK_VALID(src1);
	CHECK_VALID(src2);
	dest.x = src1.x + (src2.x - src1.x) * t;
	dest.y = src1.y + (src2.y - src1.y) * t;
	dest.z = src1.z + (src2.z - src1.z) * t;
}

//===============================================
inline void VectorDouble::Init(double ix, double iy, double iz)
{
	x = ix; y = iy; z = iz;
	CHECK_VALID(*this);
}
//===============================================
inline VectorDouble::VectorDouble(double X, double Y, double Z)
{
	x = X; y = Y; z = Z;
	CHECK_VALID(*this);
}

inline VectorDouble::VectorDouble(const VectorDouble &vOther)
{
	CHECK_VALID(vOther);
	x = vOther.x; y = vOther.y; z = vOther.z;
}

//===============================================
inline VectorDouble::VectorDouble(void) { }
//===============================================
inline void VectorDouble::Zero()
{
	x = y = z = 0.0;
}
//===============================================
inline void VectorClear(VectorDouble& a)
{
	a.x = a.y = a.z = 0.0;
}

inline VectorDouble VectorDouble::operator-(void) const
{
	VectorDouble ret(-x, -y, -z);
	return ret;
}

//===============================================
inline VectorDouble& VectorDouble::operator=(const VectorDouble &vOther)
{
	CHECK_VALID(vOther);
	x = vOther.x; y = vOther.y; z = vOther.z;
	return *this;
}

//===============================================
inline double& VectorDouble::operator[](int i)
{
	//Assert((i >= 0) && (i < 3));
	return ((double*)this)[i];
}
//===============================================
inline double VectorDouble::operator[](int i) const
{
	//Assert((i >= 0) && (i < 3));
	return ((double*)this)[i];
}
//===============================================
inline bool VectorDouble::operator==(const VectorDouble& src) const
{
	CHECK_VALID(src);
	CHECK_VALID(*this);
	return (src.x == x) && (src.y == y) && (src.z == z);
}
//===============================================
inline bool VectorDouble::operator!=(const VectorDouble& src) const
{
	CHECK_VALID(src);
	CHECK_VALID(*this);
	return (src.x != x) || (src.y != y) || (src.z != z);
}
//===============================================
__forceinline void VectorCopy(const VectorDouble& src, VectorDouble& dst)
{
	CHECK_VALID(src);
	dst.x = src.x;
	dst.y = src.y;
	dst.z = src.z;
}
//===============================================
__forceinline  VectorDouble& VectorDouble::operator+=(const VectorDouble& v)
{
	CHECK_VALID(*this);
	CHECK_VALID(v);
	x += v.x; y += v.y; z += v.z;
	return *this;
}
//===============================================
__forceinline  VectorDouble& VectorDouble::operator-=(const VectorDouble& v)
{
	CHECK_VALID(*this);
	CHECK_VALID(v);
	x -= v.x; y -= v.y; z -= v.z;
	return *this;
}
//===============================================
__forceinline  VectorDouble& VectorDouble::operator*=(double fl)
{
	x *= fl;
	y *= fl;
	z *= fl;
	CHECK_VALID(*this);
	return *this;
}
//===============================================
__forceinline  VectorDouble& VectorDouble::operator*=(const VectorDouble& v)
{
	CHECK_VALID(v);
	x *= v.x;
	y *= v.y;
	z *= v.z;
	CHECK_VALID(*this);
	return *this;
}
//===============================================
__forceinline VectorDouble&	VectorDouble::operator+=(double fl)
{
	x += fl;
	y += fl;
	z += fl;
	CHECK_VALID(*this);
	return *this;
}
//===============================================
__forceinline VectorDouble&	VectorDouble::operator-=(double fl)
{
	x -= fl;
	y -= fl;
	z -= fl;
	CHECK_VALID(*this);
	return *this;
}
//===============================================
__forceinline  VectorDouble& VectorDouble::operator/=(double fl)
{
	//Assert(fl != 0.0);
	double oofl = 1.0 / fl;
	x *= oofl;
	y *= oofl;
	z *= oofl;
	CHECK_VALID(*this);
	return *this;
}
//===============================================
__forceinline  VectorDouble& VectorDouble::operator/=(const VectorDouble& v)
{
	CHECK_VALID(v);
	//Assert(v.x != 0.0 && v.y != 0.0 && v.z != 0.0);
	x /= v.x;
	y /= v.y;
	z /= v.z;
	CHECK_VALID(*this);
	return *this;
}
//===============================================
inline double VectorDouble::Length(void) const
{
	CHECK_VALID(*this);

	return sqrt(x*x + y * y + z * z);
}
//===============================================
inline double VectorDouble::Length2D(void) const
{
	CHECK_VALID(*this);
	return sqrt(x*x + y * y);
}

inline double VectorDouble::LengthToOtherX(const VectorDouble& vOther) const
{
	double dot = vOther.x * x;
	return sqrt(dot);
}

inline double VectorDouble::LengthToOtherY(const VectorDouble& vOther) const
{
	double dot = vOther.y * y;
	return sqrt(dot);
}

inline double VectorDouble::LengthToOtherZ(const VectorDouble& vOther) const
{
	double dot = vOther.z * z;
	return sqrt(dot);
}

//===============================================
inline double VectorDouble::Length2DSqr(void) const
{
	return (x*x + y * y);
}

inline void VectorDouble::MultAdd(VectorDouble& vOther, double fl)
{
	x += fl * vOther.x;
	y += fl * vOther.y;
	z += fl * vOther.z;
}

//===============================================
inline VectorDouble CrossProduct(const VectorDouble& a, const VectorDouble& b)
{
	return VectorDouble(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x);
}

inline void CrossProduct(const VectorDouble& a, const VectorDouble& b, VectorDouble& out)
{
	out = { a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x };
}
//===============================================
double VectorDouble::DistToSqr(const VectorDouble &vOther) const
{
	VectorDouble delta;

	delta.x = x - vOther.x;
	delta.y = y - vOther.y;
	delta.z = z - vOther.z;

	return delta.LengthSqr();
}

double VectorDouble::Dist(const VectorDouble &vOther) const
{
	VectorDouble delta;

	delta.x = x - vOther.x;
	delta.y = y - vOther.y;
	delta.z = z - vOther.z;

	return delta.Length();
}

/*inline VectorDouble VectorDouble::Normalize()
{
VectorDouble vector;
double length = this->Length();

if (length != 0) {
vector.x = x / length;
vector.y = y / length;
vector.z = z / length;
}
else
vector.x = vector.y = 0.0; vector.z = 1.0;

return vector;
}

inline VectorDouble VectorDouble::NormalizeSqr()
{
VectorDouble vector;
double length = this->Length();

if (length != 0) {
vector.x = x / length;
vector.y = y / length;
vector.z = z / length;
}
else
vector.x = vector.y = 0.0; vector.z = 1.0;

return vector * vector;
}

//===============================================
inline double VectorDouble::NormalizeInPlace()
{
VectorDouble& v = *this;

double iradius = 1. / (this->Length() + 1.192092896e-07); //FLT_EPSILON

v.x *= iradius;
v.y *= iradius;
v.z *= iradius;
return iradius;
}
//===============================================
*/
inline double VectorNormalize(VectorDouble& v)
{
#if 0
	//Assert(v.IsValid());
	float l = v.Length();
	if (l != 0.0f)
	{
		v /= l;
	}
	else
	{
		v.x = v.y = 0.0f; v.z = 1.0f;
	}
	return l;
#endif
	double radius = sqrt(v.x*v.x + v.y*v.y + v.z*v.z);

	// FLT_EPSILON is added to the radius to eliminate the possibility of divide by zero.
	double iradius = 1. / (radius + 1.92092896e-07);

	v.x *= iradius;
	v.y *= iradius;
	v.z *= iradius;

	return radius;
}

inline double VectorNormalizeGetRadius(VectorDouble& v)
{
	double radius = sqrt(v.x*v.x + v.y*v.y + v.z*v.z);

	// FLT_EPSILON is added to the radius to eliminate the possibility of divide by zero.
	return 1. / (radius + 1.92092896e-07);
}

inline double VectorNormalizeSqr(VectorDouble& v)
{
	//Assert(v.IsValid());
	double l = v.Length();
	if (l != 0.0)
	{
		v /= l;
	}
	else
	{
		v.x = v.y = 0.0; v.z = 1.0;
	}
	return l * l;
}
//===============================================
FORCEINLINE double VectorNormalize(double * v)
{
	return VectorNormalize(*(reinterpret_cast<VectorDouble *>(v)));
}

//===============================================
inline VectorDouble VectorDouble::operator+(const VectorDouble& v) const
{
	VectorDouble res;
	res.x = x + v.x;
	res.y = y + v.y;
	res.z = z + v.z;
	return res;
}

//===============================================
inline VectorDouble VectorDouble::operator-(const VectorDouble& v) const
{
	VectorDouble res;
	res.x = x - v.x;
	res.y = y - v.y;
	res.z = z - v.z;
	return res;
}
//===============================================
inline VectorDouble VectorDouble::operator*(double fl) const
{
	VectorDouble res;
	res.x = x * fl;
	res.y = y * fl;
	res.z = z * fl;
	return res;
}
//===============================================
inline VectorDouble VectorDouble::operator*(const VectorDouble& v) const
{
	VectorDouble res;
	res.x = x * v.x;
	res.y = y * v.y;
	res.z = z * v.z;
	return res;
}
//===============================================
inline VectorDouble VectorDouble::operator/(double fl) const
{
	VectorDouble res;
	res.x = x / fl;
	res.y = y / fl;
	res.z = z / fl;
	return res;
}
//===============================================
inline VectorDouble VectorDouble::operator/(const VectorDouble& v) const
{
	VectorDouble res;
	res.x = x / v.x;
	res.y = y / v.y;
	res.z = z / v.z;
	return res;
}
inline double VectorDouble::Dot(const VectorDouble& vOther) const
{
	const VectorDouble& a = *this;

	return(a.x*vOther.x + a.y*vOther.y + a.z*vOther.z);
}

inline double VectorDouble::Dot(const double* fOther) const
{
	const VectorDouble& a = *this;

	return(a.x*fOther[0] + a.y*fOther[1] + a.z*fOther[2]);
}

inline double* VectorDouble::Base()
{
	return (double*)this;
}

inline double const* VectorDouble::Base() const
{
	return (double const*)this;
}

FORCEINLINE void VectorAdd(const Vector& a, const Vector& b, Vector& c)
{
	CHECK_VALID(a);
	CHECK_VALID(b);
	c.x = a.x + b.x;
	c.y = a.y + b.y;
	c.z = a.z + b.z;
}

FORCEINLINE void VectorSubtract(const Vector& a, const Vector& b, Vector& c)
{
	CHECK_VALID(a);
	CHECK_VALID(b);
	c.x = a.x - b.x;
	c.y = a.y - b.y;
	c.z = a.z - b.z;
}

FORCEINLINE void VectorMultiply(const Vector& a, float b, Vector& c)
{
	CHECK_VALID(a);
	//Assert(IsFinite(b));
	c.x = a.x * b;
	c.y = a.y * b;
	c.z = a.z * b;
}

FORCEINLINE void VectorMultiply(const Vector& a, const Vector& b, Vector& c)
{
	CHECK_VALID(a);
	CHECK_VALID(b);
	c.x = a.x * b.x;
	c.y = a.y * b.y;
	c.z = a.z * b.z;
}

// for backwards compatability
inline void VectorScale(const Vector& in, float scale, Vector& result)
{
	VectorMultiply(in, scale, result);
}


FORCEINLINE void VectorDivide(const Vector& a, float b, Vector& c)
{
	CHECK_VALID(a);
	//Assert(b != 0.0f);
	float oob = 1.0f / b;
	c.x = a.x * oob;
	c.y = a.y * oob;
	c.z = a.z * oob;
}

FORCEINLINE void VectorDivide(const Vector& a, const Vector& b, Vector& c)
{
	CHECK_VALID(a);
	CHECK_VALID(b);
	//Assert((b.x != 0.0f) && (b.y != 0.0f) && (b.z != 0.0f));
	c.x = a.x / b.x;
	c.y = a.y / b.y;
	c.z = a.z / b.z;
}

class QAngleByValue;
class QAngle
{
public:
	float x, y, z;

	QAngle(void);
	QAngle(float X, float Y, float Z);

	operator QAngleByValue &() { return *((QAngleByValue *)(this)); }
	operator const QAngleByValue &() const { return *((const QAngleByValue *)(this)); }

	void Init(float ix = 0.0f, float iy = 0.0f, float iz = 0.0f);
	void Random(float minVal, float maxVal);

	__inline bool IsValid() const
	{
		return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
	}

	void Invalidate();
	void NormalizeAngle();

	bool IsZero()
	{
		CHECK_VALID(*this);
		if (this->x == 0.f && this->y == 0.f && this->z == 0.f)
			return true;

		return false;
	}

	float operator[](int i) const;
	float& operator[](int i);

	float* Base();
	float const* Base() const;

	bool operator==(const QAngle& v) const;
	bool operator!=(const QAngle& v) const;

	QAngle& operator+=(const QAngle &v);
	QAngle& operator-=(const QAngle &v);
	QAngle& operator*=(float s);
	QAngle& operator/=(float s);

	float   Length() const;
	float   LengthSqr() const;

	QAngle& operator=(const QAngle& src);

	QAngle  operator-(void) const;

	QAngle  operator+(const QAngle& v) const;
	QAngle  operator-(const QAngle& v) const;
	QAngle  operator*(float fl) const;
	QAngle  operator/(float fl) const;

	void ClampX();
	void ClampY();
	void Clamp();
	QAngle Mod(float N);

	inline bool QAngle::IsNaN()
	{
		return isnan(x) || isnan(y) || isnan(z);
	}

	inline QAngle QAngle::Normalize()
	{
		QAngle vector;
		float length = this->Length();

		if (length != 0) {
			vector.x = x / length;
			vector.y = y / length;
			vector.z = z / length;
		}
		else
			vector.x = vector.y = 0.0f; vector.z = 1.0f;

		return vector;
	}

	QAngle Normalized()
	{
		if (this->x != this->x)
			this->x = 0;
		if (this->y != this->y)
			this->y = 0;
		if (this->z != this->z)
			this->z = 0;

		if (this->x > 89.f)
			this->x = 89.f;
		if (this->x < -89.f)
			this->x = -89.f;

		while (this->y > 180)
			this->y -= 360;
		while (this->y <= -180)
			this->y += 360;

		if (this->y > 180.f)
			this->y = 180.f;
		if (this->y < -180.f)
			this->y = -180.f;

		this->z = 0;

		return *this;
	}
};

//-----------------------------------------------------------------------------
// constructors
//-----------------------------------------------------------------------------
inline QAngle::QAngle(void)
{
#ifdef _DEBUG
#ifdef VECTOR_PARANOIA
	// Initialize to NAN to catch errors
	x = y = z = float_NAN;
#endif
#endif
}

inline QAngle::QAngle(float X, float Y, float Z)
{
	x = X; y = Y; z = Z;
	CHECK_VALID(*this);
}

//-----------------------------------------------------------------------------
// initialization
//-----------------------------------------------------------------------------
inline void QAngle::Init(float ix, float iy, float iz)
{
	x = ix; y = iy; z = iz;
	CHECK_VALID(*this);
}

inline void QAngle::Random(float minVal, float maxVal)
{
	x = minVal + ((float)rand() / RAND_MAX) * (maxVal - minVal);
	y = minVal + ((float)rand() / RAND_MAX) * (maxVal - minVal);
	z = minVal + ((float)rand() / RAND_MAX) * (maxVal - minVal);
	CHECK_VALID(*this);
}

//-----------------------------------------------------------------------------
// assignment
//-----------------------------------------------------------------------------
inline QAngle& QAngle::operator=(const QAngle &vOther)
{
	CHECK_VALID(vOther);
	x = vOther.x; y = vOther.y; z = vOther.z;
	return *this;
}

//-----------------------------------------------------------------------------
// comparison
//-----------------------------------------------------------------------------
inline bool QAngle::operator==(const QAngle& src) const
{
	CHECK_VALID(src);
	CHECK_VALID(*this);
	return (src.x == x) && (src.y == y) && (src.z == z);
}

inline bool QAngle::operator!=(const QAngle& src) const
{
	CHECK_VALID(src);
	CHECK_VALID(*this);
	return (src.x != x) || (src.y != y) || (src.z != z);
}

//-----------------------------------------------------------------------------
// standard math operations
//-----------------------------------------------------------------------------
inline QAngle& QAngle::operator+=(const QAngle& v)
{
	CHECK_VALID(*this);
	CHECK_VALID(v);
	x += v.x; y += v.y; z += v.z;
	return *this;
}

inline QAngle& QAngle::operator-=(const QAngle& v)
{
	CHECK_VALID(*this);
	CHECK_VALID(v);
	x -= v.x; y -= v.y; z -= v.z;
	return *this;
}

inline QAngle& QAngle::operator*=(float fl)
{
	x *= fl;
	y *= fl;
	z *= fl;
	CHECK_VALID(*this);
	return *this;
}

inline QAngle& QAngle::operator/=(float fl)
{
	//Assert(fl != 0.0f);
	float oofl = 1.0f / fl;
	x *= oofl;
	y *= oofl;
	z *= oofl;
	CHECK_VALID(*this);
	return *this;
}

//-----------------------------------------------------------------------------
// Base address...
//-----------------------------------------------------------------------------
inline float* QAngle::Base()
{
	return (float*)this;
}

inline float const* QAngle::Base() const
{
	return (float const*)this;
}

//-----------------------------------------------------------------------------
// Array access
//-----------------------------------------------------------------------------
inline float& QAngle::operator[](int i)
{
	//Assert((i >= 0) && (i < 3));
	return ((float*)this)[i];
}

inline float QAngle::operator[](int i) const
{
	//Assert((i >= 0) && (i < 3));
	return ((float*)this)[i];
}

//-----------------------------------------------------------------------------
// length
//-----------------------------------------------------------------------------
inline float QAngle::Length() const
{
	CHECK_VALID(*this);
	float sqin = LengthSqr();
	float sqout;
	SSESqrt(&sqout, &sqin);
	return sqout; //(float)sqrt(LengthSqr());
}


inline float QAngle::LengthSqr() const
{
	CHECK_VALID(*this);
	return x * x + y * y;
}

inline QAngle QAngle::operator-(void) const
{
	return QAngle(-x, -y, -z);
}

inline QAngle QAngle::operator+(const QAngle& v) const
{
	QAngle res;
	res.x = x + v.x;
	res.y = y + v.y;
	res.z = z + v.z;
	return res;
}

inline QAngle QAngle::operator-(const QAngle& v) const
{
	QAngle res;
	res.x = x - v.x;
	res.y = y - v.y;
	res.z = z - v.z;
	return res;
}

inline QAngle QAngle::operator*(float fl) const
{
	QAngle res;
	res.x = x * fl;
	res.y = y * fl;
	res.z = z * fl;
	return res;
}

inline QAngle QAngle::operator/(float fl) const
{
	QAngle res;
	res.x = x / fl;
	res.y = y / fl;
	res.z = z / fl;
	return res;
}

inline void QAngle::ClampX()
{
	//this->x = fmodf(this->x, 360.0f);

	NormalizeFloat(x);

	clampfloat(x, -89.0f, 89.0f);
}

inline void QAngle::ClampY()
{
	//this->y = fmodf(this->y, 360.0f);

	NormalizeFloat(y);

	//return *this;
}

inline void QAngle::Clamp()
{
	//this->x = fmodf(this->x, 360.0f);
	//this->y = fmodf(this->y, 360.0f);

#ifdef _DEBUG
	if (isnan(this->x) || isnan(this->y) || isnan(this->z))
	{
		THROW_ERROR(ERR_CLAMP_HAS_NAN);
		exit(EXIT_SUCCESS);
	}
#endif

	NormalizeFloat(x);
	NormalizeFloat(y);
	clampfloat(x, -89.0f, 89.0f);
	z = 0.0f;

#ifdef _DEBUG
	if (isnan(this->x) || isnan(this->y) || isnan(this->z))
	{
		THROW_ERROR(ERR_CLAMP_HAS_NAN);
		exit(EXIT_SUCCESS);
	}
#endif
}


inline void QAngle::NormalizeAngle()
{
	x -= floorf(x / 360.0f + 0.5f) * 360.0f;
	y -= floorf(y / 360.0f + 0.5f) * 360.0f;
	z -= floorf(z / 360.0f + 0.5f) * 360.0f;
}

inline QAngle QAngle::Mod(float N)
{
	CHECK_VALID(*this);
	this->x = fmodf(x, N);
	this->y = fmodf(y, N);
	this->z = fmodf(z, N);

	return *this;
}

class CUserCmd
{
public:
	virtual ~CUserCmd() { }; //0x0
	int		command_number; //0x4
	int		tick_count; //0x8
	QAngle	viewangles; //0xC
	Vector	aimdirection; //0x18
	float	forwardmove; //0x24
	float	sidemove; //0x28
	float	upmove; //0x2C
	int		buttons; //0x30
	unsigned char    impulse; //0x34
	int		weaponselect; //0x35
	int		weaponsubtype; //0x39
	int		random_seed; //0x3D
	short	mousedx; //0x41
	short	mousedy; //0x43
	bool	hasbeenpredicted; //0x45
	QAngle  headangles;
	Vector  headorigin;
	//char	pad_0x4C[0x18]; //0x46
	void Reset()
	{
		command_number = 0;
		tick_count = 0;
		viewangles.Init();
		aimdirection.Init();
		forwardmove = 0.0f;
		sidemove = 0.0f;
		upmove = 0.0f;
		buttons = 0;
		impulse = 0;
		weaponselect = 0;
		weaponsubtype = 0;
		random_seed = 0;
		mousedx = 0;
		mousedy = 0;

		hasbeenpredicted = false;
#if defined( HL2_DLL ) || defined( HL2_CLIENT_DLL )
		entitygroundcontact.RemoveAll();
#endif
		headangles.Init();
		headorigin.Init();
	}
	CRC32_t GetChecksum();
};

class ModifiableUserCmd
{
public:
	CUserCmd  cmd_backup; //Original command before modification
	CUserCmd* cmd;
	DWORD* FramePointer;
	bool bFinalTick;
	bool bOriginalSendPacket;
	bool bSendPacket; //Should we send a cmd this tick
	bool bShouldAutoScope;
	bool bShouldAutoStop;
private:
	bool* pbSendPacket;
public:
	int PressingAimbotKey;
	int PressingTriggerbotKey;
	int PressingJumpButton;
	bool bManuallyFiring;

	ModifiableUserCmd() : cmd_backup(), cmd(nullptr), FramePointer(nullptr), bFinalTick(false),
		bOriginalSendPacket(false),
		bSendPacket(false),
		bShouldAutoScope(false),
		bShouldAutoStop(false),
		pbSendPacket(nullptr),
		PressingAimbotKey(0),
		PressingTriggerbotKey(0),
		PressingJumpButton(0),
		bManuallyFiring(false)
	{
	}

	explicit ModifiableUserCmd(CUserCmd* newcmd) : cmd_backup(), cmd(newcmd), FramePointer(nullptr),
		bFinalTick(false),
		bOriginalSendPacket(false),
		bSendPacket(false),
		bShouldAutoScope(false),
		bShouldAutoStop(false),
		pbSendPacket(nullptr),
		PressingAimbotKey(0), PressingTriggerbotKey(0), PressingJumpButton(0),
		bManuallyFiring(false)
	{
	}

	~ModifiableUserCmd() {}

	void Reset(CUserCmd* newcmd);
	bool IsUserCmdAndPlayerNotMoving();

	BOOL IsManuallyAttacking() const { return cmd_backup.buttons & IN_ATTACK; }
	BOOL IsManuallySecondaryAttacking() const { return cmd_backup.buttons & IN_ATTACK2; }
	BOOL IsAttacking() const { return cmd->buttons & IN_ATTACK; }
	BOOL IsSecondaryAttacking() const { return cmd->buttons & IN_ATTACK2; }
	BOOL IsUsing() const { return cmd->buttons & IN_USE; }
	BOOL IsJumping() const { return cmd->buttons & IN_JUMP; }
	BOOL IsDucking() const { return cmd->buttons & IN_DUCK; }
	void OverrideSendPacket() { *pbSendPacket = bSendPacket; }
};

// Swap two of anything.
template <class T>
FORCEINLINE void _swap(T& x, T& y)
{
	T temp = x;
	x = y;
	y = temp;
}

struct vrect_t
{
	int				x, y, width, height;
	vrect_t			*pnext;
};

struct VMatrix
{
	float m[4][4];

	inline float* operator[](int i)
	{
		return m[i];
	}

	inline const float* operator[](int i) const
	{
		return m[i];
	}
};

inline void MatrixCopy(const VMatrix& src, VMatrix& dst)
{
	if (&src != &dst)
	{
		memcpy(dst.m, src.m, 16 * sizeof(float));
	}
}

struct matrix3x3_t
{
	float m_flMatVal[3][3];
};

struct matrix3x4_t
{
	matrix3x4_t() {}
	matrix3x4_t(
		float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23)
	{
		m_flMatVal[0][0] = m00;	m_flMatVal[0][1] = m01; m_flMatVal[0][2] = m02; m_flMatVal[0][3] = m03;
		m_flMatVal[1][0] = m10;	m_flMatVal[1][1] = m11; m_flMatVal[1][2] = m12; m_flMatVal[1][3] = m13;
		m_flMatVal[2][0] = m20;	m_flMatVal[2][1] = m21; m_flMatVal[2][2] = m22; m_flMatVal[2][3] = m23;
	}

	//-----------------------------------------------------------------------------
	// Creates a matrix where the X axis = forward
	// the Y axis = left, and the Z axis = up
	//-----------------------------------------------------------------------------
	void Init(const Vector& xAxis, const Vector& yAxis, const Vector& zAxis, const Vector &vecOrigin)
	{
		m_flMatVal[0][0] = xAxis.x; m_flMatVal[0][1] = yAxis.x; m_flMatVal[0][2] = zAxis.x; m_flMatVal[0][3] = vecOrigin.x;
		m_flMatVal[1][0] = xAxis.y; m_flMatVal[1][1] = yAxis.y; m_flMatVal[1][2] = zAxis.y; m_flMatVal[1][3] = vecOrigin.y;
		m_flMatVal[2][0] = xAxis.z; m_flMatVal[2][1] = yAxis.z; m_flMatVal[2][2] = zAxis.z; m_flMatVal[2][3] = vecOrigin.z;
	}

	//-----------------------------------------------------------------------------
	// Creates a matrix where the X axis = forward
	// the Y axis = left, and the Z axis = up
	//-----------------------------------------------------------------------------
	matrix3x4_t(const Vector& xAxis, const Vector& yAxis, const Vector& zAxis, const Vector &vecOrigin)
	{
		Init(xAxis, yAxis, zAxis, vecOrigin);
	}

	inline Vector GetOrigin()
	{
		return{ m_flMatVal[0][3], m_flMatVal[1][3], m_flMatVal[2][3] };
	}

	inline void SetOrigin(Vector const & p)
	{
		m_flMatVal[0][3] = p.x;
		m_flMatVal[1][3] = p.y;
		m_flMatVal[2][3] = p.z;
	}

	inline void Invalidate(void)
	{
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				m_flMatVal[i][j] = VEC_T_NAN;
			}
		}
	}

	float *operator[](int i) { /*Assert((i >= 0) && (i < 3));*/ return m_flMatVal[i]; }
	const float *operator[](int i) const { /*Assert((i >= 0) && (i < 3));*/ return m_flMatVal[i]; }
	float *Base() { return &m_flMatVal[0][0]; }
	const float *Base() const { return &m_flMatVal[0][0]; }

	float m_flMatVal[3][4];
};

inline void MatrixCopy(const matrix3x4_t& in, matrix3x4_t& out)
{
	memcpy(out.Base(), in.Base(), sizeof(float) * 3 * 4);
}

struct ALIGN16 matrix3x4a_t : public matrix3x4_t
{
public:
	/*
	matrix3x4a_t() { if (((size_t)Base()) % 16 != 0) { Error( "matrix3x4a_t missaligned" ); } }
	*/
	matrix3x4a_t& operator=(const matrix3x4_t& src) { memcpy(Base(), src.Base(), sizeof(float) * 3 * 4); return *this; };
};

void MatrixInvert(const matrix3x4_t &in, matrix3x4_t &out);

struct mstudiobbox_t
{
	int		bone;
	int		group; // intersection group //4
	Vector	bbmin; // bounding box  //8
	Vector	bbmax; //20
	int		hitboxnameindex; // offset to the name of the hitbox. //32
	QAngle angles; //36
	float	radius; //48 //diameter
	QAngle anglesunknown;
	float anglepad;
	//int		pad2[4];

	char* getHitboxName()
	{
		if (hitboxnameindex == 0)
			return "";

		return ((char*)this) + hitboxnameindex;
	}
};

//Useful for IMGUI menu color options. Just call FinishEditing after imgui color is processed
class ColorRGBFloatP
{
public:
	float r;
	float g;
	float b;
	float* originalr;
	float* originalg;
	float* originalb;
	ColorRGBFloatP(float& red, float& green, float& blue)
	{
		originalr = &red;
		originalg = &green;
		originalb = &blue;
		r = red;
		g = green;
		b = blue;
	}
	~ColorRGBFloatP() {}
	float* AsFloatPtr() { return &r; }
	float operator[](int i) const
	{
		return ((float*)this)[i];
	}
	void FinishEditing()
	{
		*originalr = r;
		*originalg = g;
		*originalb = b;
	}
};

class ColorRGBFloat
{
public:
	float r;
	float g;
	float b;
	ColorRGBFloat(float red, float green, float blue)
	{
		r = red;
		g = green;
		b = blue;
	}
	~ColorRGBFloat() {}
	float* AsFloatPtr() { return &r; }
	float operator[](int i) const
	{
		return ((float*)this)[i];
	}
};

class ColorRGBAFloat
{
public:
	float a;
	float r;
	float g;
	float b;
	ColorRGBAFloat() {};
	ColorRGBAFloat(float red, float green, float blue, float alpha)
	{
		a = alpha;
		r = red;
		g = green;
		b = blue;
	}
	~ColorRGBAFloat() {}
};

class ColorRGBA
{
public:
	unsigned char a;
	unsigned char r;
	unsigned char g;
	unsigned char b;
	ColorRGBA(int red, int green, int blue, int alpha)
	{
		a = (unsigned char)alpha;
		r = (unsigned char)red;
		g = (unsigned char)green;
		b = (unsigned char)blue;
	}
	~ColorRGBA() {}
};

struct ColorRGB
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
	ColorRGB(int red, int green, int blue)
	{
		r = (unsigned char)red;
		g = (unsigned char)green;
		b = (unsigned char)blue;
	}
	~ColorRGB() {}
};


class Color
{
public:
	Color()
	{
		*((int *)this) = 0;
	}
	Color(int color32)
	{
		*((int *)this) = color32;
	}
	Color(int _r, int _g, int _b)
	{
		SetColor(_r, _g, _b, 255);
	}
	Color(int _r, int _g, int _b, int _a)
	{
		SetColor(_r, _g, _b, _a);
	}

	void SetColor(int _r, int _g, int _b, int _a = 255)
	{
		_color[0] = (unsigned char)_r;
		_color[1] = (unsigned char)_g;
		_color[2] = (unsigned char)_b;
		_color[3] = (unsigned char)_a;
	}

	void GetColor(int &_r, int &_g, int &_b, int &_a) const
	{
		_r = _color[0];
		_g = _color[1];
		_b = _color[2];
		_a = _color[3];
	}

	void SetRawColor(int color32)
	{
		*((int*)this) = color32;
	}

	int GetRawColor() const
	{
		return *((int*)this);
	}

	int GetD3DColor() const
	{
		return ((int)((((_color[3]) & 0xff) << 24) | (((_color[0]) & 0xff) << 16) | (((_color[1]) & 0xff) << 8) | ((_color[2]) & 0xff)));
	}

	inline int r() const { return _color[0]; }
	inline int g() const { return _color[1]; }
	inline int b() const { return _color[2]; }
	inline int a() const { return _color[3]; }

	inline float rBase() const { return _color[0] / 255.0f; }
	inline float gBase() const { return _color[1] / 255.0f; }
	inline float bBase() const { return _color[2] / 255.0f; }
	inline float aBase() const { return _color[3] / 255.0f; }

	unsigned char &operator[](int index)
	{
		return _color[index];
	}

	const unsigned char &operator[](int index) const
	{
		return _color[index];
	}

	bool operator == (const Color &rhs) const
	{
		return (*((int *)this) == *((int *)&rhs));
	}

	bool operator != (const Color &rhs) const
	{
		return !(operator==(rhs));
	}

	Color &operator=(const Color &rhs)
	{
		SetRawColor(rhs.GetRawColor());
		return *this;
	}

	D3DCOLOR direct() const
	{
		return ((int)((((_color[3]) & 0xff) << 24) | (((_color[0]) & 0xff) << 16) | (((_color[1]) & 0xff) << 8) | ((_color[2]) & 0xff)));
	}

	float* Base()
	{
		float clr[3];

		clr[0] = _color[0] / 255.0f;
		clr[1] = _color[1] / 255.0f;
		clr[2] = _color[2] / 255.0f;

		return &clr[0];
	}

	ImColor ToImGUI()
	{
		return ImColor(_color[0], _color[1], _color[2], _color[3]);
	}

	void SetAlpha(int _a)
	{
		_color[3] = static_cast<unsigned char>(_a);
	}

	//Compatibility
	inline unsigned char* base() {
		return &_color[0];
	}

	float* BaseAlpha()
	{
		float clr[4];

		clr[0] = _color[0] / 255.0f;
		clr[1] = _color[1] / 255.0f;
		clr[2] = _color[2] / 255.0f;
		clr[3] = _color[3] / 255.0f;

		return &clr[0];
	}

	static float Hue(const Color color)
	{
		float R = 1.f * color.r() / 255;
		float G = 1.f * color.g() / 255;
		float B = 1.f * color.b() / 255;

		float mx = max(R, max(G, B));
		float mn = min(R, min(G, B));
		if (mx == mn)
			return 0.f;

		float delta = mx - mn;

		float hue = 0.f;
		if (mx == R)
			hue = (G - B) / delta;
		else if (mx == G)
			hue = 2.f + (B - R) / delta;
		else
			hue = 4.f + (R - G) / delta;

		hue *= 60.f;
		if (hue < 0.f)
			hue += 360.f;

		return hue / 360.f;
	}
	static float Saturation(const Color color)
	{
		float R = 1.f * color.r() / 255;
		float G = 1.f * color.g() / 255;
		float B = 1.f * color.b() / 255;

		float mx = max(R, max(G, B));
		float mn = min(R, min(G, B));

		float delta = mx - mn;

		if (mx == 0.f)
			return delta;

		return delta / mx;
	}
	static float Brightness(const Color color)
	{
		float R = 1.f * color.r() / 255;
		float G = 1.f * color.g() / 255;
		float B = 1.f * color.b() / 255;

		return max(R, max(G, B));
	}

	float Hue() const
	{
		return Hue(*this);
	}
	float Saturation() const
	{
		return Saturation(*this);
	}
	float Brightness() const
	{
		return Brightness(*this);
	}

	static Color FromHSB(float hue, float saturation, float brightness)
	{
		float h = hue == 1.0f ? 0 : hue * 6.0f;
		float f = h - (int)h;
		float p = brightness * (1.0f - saturation);
		float q = brightness * (1.0f - saturation * f);
		float t = brightness * (1.0f - (saturation * (1.0f - f)));

		if (h < 1)
		{
			return Color(
				(unsigned char)(brightness * 255),
				(unsigned char)(t * 255),
				(unsigned char)(p * 255)
			);
		}
		else if (h < 2)
		{
			return Color(
				(unsigned char)(q * 255),
				(unsigned char)(brightness * 255),
				(unsigned char)(p * 255)
			);
		}
		else if (h < 3)
		{
			return Color(
				(unsigned char)(p * 255),
				(unsigned char)(brightness * 255),
				(unsigned char)(t * 255)
			);
		}
		else if (h < 4)
		{
			return Color(
				(unsigned char)(p * 255),
				(unsigned char)(q * 255),
				(unsigned char)(brightness * 255)
			);
		}
		else if (h < 5)
		{
			return Color(
				(unsigned char)(t * 255),
				(unsigned char)(p * 255),
				(unsigned char)(brightness * 255)
			);
		}
		else
		{
			return Color(
				(unsigned char)(brightness * 255),
				(unsigned char)(p * 255),
				(unsigned char)(q * 255)
			);
		}
	}

	static Color Red() { return Color(255, 0, 0); }
	static Color Green() { return Color(0, 255, 0); }
	static Color Yellow() { return Color(255, 255, 0); }
	static Color Blue() { return Color(0, 0, 255); }
	static Color LightBlue() { return Color(100, 100, 255); }
	static Color Grey() { return Color(128, 128, 128); }
	static Color DarkGrey() { return Color(45, 45, 45); }
	static Color Black() { return Color(0, 0, 0); }
	static Color White() { return Color(255, 255, 255); }
	static Color Purple() { return Color(220, 0, 220); }
	static Color Cyan() { return Color(0, 255, 255); }
	static Color Magenta() { return Color(255, 0, 255); }
	static Color Orange() { return Color(255, 165, 0); }

	//Menu
	static Color Background() { return Color(55, 55, 55); }
	static Color FrameBorder() { return Color(80, 80, 80); }
	static Color MainText() { return Color(230, 230, 230); }
	static Color HeaderText() { return Color(49, 124, 230); }
	static Color CurrentTab() { return Color(55, 55, 55); }
	static Color Tabs() { return Color(23, 23, 23); }
	static Color Highlight() { return Color(49, 124, 230); }
	static Color ElementBorder() { return Color(0, 0, 0); }
	static Color SliderScroll() { return Color(78, 143, 230); }


private:
	unsigned char _color[4];
};
#if 0
class CViewSetup
{
public:
	int			x, x_old;
	int			y, y_old;
	int			width, width_old;
	int			height, height_old;
	bool		m_bOrtho;
	float		m_OrthoLeft;
	float		m_OrthoTop;
	float		m_OrthoRight;
	float		m_OrthoBottom;
	bool		m_bCustomViewMatrix;
	matrix3x4_t	m_matCustomViewMatrix;
	char		pad_0x68[0x48];
	float		fov;
	float		fovViewmodel;
	Vector		origin;
	QAngle		angles;
	float		zNear;
	float		zFar;
	float		zNearViewmodel;
	float		zFarViewmodel;
	float		m_flAspectRatio;
	float		m_flNearBlurDepth;
	float		m_flNearFocusDepth;
	float		m_flFarFocusDepth;
	float		m_flFarBlurDepth;
	float		m_flNearBlurRadius;
	float		m_flFarBlurRadius;
	int			m_nDoFQuality;
	int			m_nMotionBlurMode;
	float		m_flShutterTime;
	Vector		m_vShutterOpenPosition;
	QAngle		m_shutterOpenAngles;
	Vector		m_vShutterClosePosition;
	QAngle		m_shutterCloseAngles;
	float		m_flOffCenterTop;
	float		m_flOffCenterBottom;
	float		m_flOffCenterLeft;
	float		m_flOffCenterRight;
	int			m_EdgeBlur;
};
#endif

enum FontDrawType_t
{
	FONT_DRAW_DEFAULT = 0,
	FONT_DRAW_NONADDITIVE,
	FONT_DRAW_ADDITIVE,
	FONT_DRAW_TYPE_COUNT = 2,
};

typedef __declspec(align(16)) union
{
	float f[4];
	__m128 v;
} m128;

__forceinline __m128 sqrt_ps(const __m128 squared)
{
	return _mm_sqrt_ps(squared);
}

typedef unsigned long HFont;

struct Vector2D
{
public:
	float x, y;

	Vector2D() {}
	Vector2D(float x_, float y_) { x = x_; y = y_; }
	void NormalizeInPlace();
	Vector AsVector() { return Vector(x, y, 0.0f); }

	Vector2D operator+(const Vector2D& v) const
	{
		return Vector2D(x + v.x, y + v.y);
	}

	Vector2D operator/(const float v) const
	{
		return Vector2D(x / v, y / v);
	}

	Vector2D operator-(const Vector2D& v) const
	{
		return Vector2D(x - v.x, y - v.y);
	}

	float length() const
	{
		m128 tmp;
		tmp.f[0] = x * x + y * y;
		const auto calc = sqrt_ps(tmp.v);
		return reinterpret_cast<const m128*>(&calc)->f[0];
	}
};

inline Vector2D& Vector::AsVector2D()
{
	return *(Vector2D*)this;
}

inline const Vector2D& Vector::AsVector2D() const
{
	return *(const Vector2D*)this;
}

typedef std::pair<Vector2D, Vector2D> rectangle;
struct FontVertex_t
{
	Vector2D m_Position;
	Vector2D m_TexCoord;

	FontVertex_t() {}
	FontVertex_t(const Vector2D &pos, const Vector2D &coord = Vector2D(0, 0))
	{
		m_Position = pos;
		m_TexCoord = coord;
	}
	void Init(const Vector2D &pos, const Vector2D &coord = Vector2D(0, 0))
	{
		m_Position = pos;
		m_TexCoord = coord;
	}
};

typedef FontVertex_t Vertex_t;

struct surfacephysicsparams_t
{
	// vphysics physical properties
	float			friction;
	float			elasticity;				// collision elasticity - used to compute coefficient of restitution
	float			density;				// physical density (in kg / m^3)
	float			thickness;				// material thickness if not solid (sheet materials) in inches
	float			dampening;
};

struct surfaceaudioparams_t// +16
{
	// sounds / audio data
	float			reflectivity;		// like elasticity, but how much sound should be reflected by this surface
	float			hardnessFactor;	// like elasticity, but only affects impact sound choices
	float			roughnessFactor;	// like friction, but only affects scrape sound choices

										// audio thresholds
	float			roughThreshold;	// surface roughness > this causes "rough" scrapes, < this causes "smooth" scrapes
	float			hardThreshold;	// surface hardness > this causes "hard" impacts, < this causes "soft" impacts
	float			hardVelocityThreshold;	// collision velocity > this causes "hard" impacts, < this causes "soft" impacts
											// NOTE: Hard impacts must meet both hardnessFactor AND velocity thresholds
	float    highPitchOcclusion;       //a value betweeen 0 and 100 where 0 is not occluded at all and 100 is silent (except for any additional reflected sound)
	float    midPitchOcclusion;
	float    lowPitchOcclusion;
};

// ???????????
// im sorry shark but this one is wrong - updating.

struct surfacesoundnames_t// +36
{
	unsigned short	walkStepLeft;
	unsigned short	walkStepRight;
	unsigned short	runStepLeft;
	unsigned short	runStepRight;
	unsigned short	impactSoft;
	unsigned short	impactHard;
	unsigned short	scrapeSmooth;
	unsigned short	scrapeRough;
	unsigned short	bulletImpact;
	unsigned short	rolling;
	unsigned short	breakSound;
	unsigned short	strainSound;
};

// this and the other class is the same...
struct surfacesoundhandles_t
{
	unsigned short	walkStepLeft;
	unsigned short	walkStepRight;
	unsigned short	runStepLeft;
	unsigned short	runStepRight;
	unsigned short	impactSoft;
	unsigned short	impactHard;
	unsigned short	scrapeSmooth;
	unsigned short	scrapeRough;
	unsigned short	bulletImpact;
	unsigned short	rolling;
	unsigned short	breakSound;
	unsigned short	strainSound;
};

// this looks like switched...
// from abs ->
/*
*	float    maxSpeedFactor;
*	float    jumpFactor;
*	char    pad00[0x4];
*	// then its the same...
*
*/
struct surfacegameprops_t
{
public:
	float    maxSpeedFactor; //0x50
	float    jumpFactor; //0x54
	float    flPenetrationModifier;
	float    flDamageModifier;
	unsigned short    gamematerial;
	unsigned char climbable;
	char    pad01[0x3];

};//Size=0x0019

struct surfacedata_t
{
	surfacephysicsparams_t	physics;
	surfaceaudioparams_t	audio;
	surfacesoundnames_t		sounds;
	surfacegameprops_t		game;
	//surfacesoundhandles_t	soundhandles;
};

enum ClientFrameStage_t
{
	FRAME_UNDEFINED = -1,
	FRAME_START,
	FRAME_NET_UPDATE_START,
	FRAME_NET_UPDATE_POSTDATAUPDATE_START,
	FRAME_NET_UPDATE_POSTDATAUPDATE_END,
	FRAME_NET_UPDATE_END,
	FRAME_RENDER_START,
	FRAME_RENDER_END
};

#define MAX_SPLITSCREEN_CLIENT_BITS 2
// this should == MAX_JOYSTICKS in InputEnums.h
#define MAX_SPLITSCREEN_CLIENTS	( 1 << MAX_SPLITSCREEN_CLIENT_BITS ) // 4

enum
{
	MAX_JOYSTICKS = MAX_SPLITSCREEN_CLIENTS,
	MOUSE_BUTTON_COUNT = 5,
};

enum JoystickAxis_t
{
	JOY_AXIS_X = 0,
	JOY_AXIS_Y,
	JOY_AXIS_Z,
	JOY_AXIS_R,
	JOY_AXIS_U,
	JOY_AXIS_V,
	MAX_JOYSTICK_AXES,
};

enum
{
	JOYSTICK_MAX_BUTTON_COUNT = 32,
	JOYSTICK_POV_BUTTON_COUNT = 4,
	JOYSTICK_AXIS_BUTTON_COUNT = MAX_JOYSTICK_AXES * 2,
};

#define JOYSTICK_BUTTON_INTERNAL( _joystick, _button ) ( JOYSTICK_FIRST_BUTTON + ((_joystick) * JOYSTICK_MAX_BUTTON_COUNT) + (_button) )
#define JOYSTICK_POV_BUTTON_INTERNAL( _joystick, _button ) ( JOYSTICK_FIRST_POV_BUTTON + ((_joystick) * JOYSTICK_POV_BUTTON_COUNT) + (_button) )
#define JOYSTICK_AXIS_BUTTON_INTERNAL( _joystick, _button ) ( JOYSTICK_FIRST_AXIS_BUTTON + ((_joystick) * JOYSTICK_AXIS_BUTTON_COUNT) + (_button) )

#define JOYSTICK_BUTTON( _joystick, _button ) ( (ButtonCode_t)JOYSTICK_BUTTON_INTERNAL( _joystick, _button ) )
#define JOYSTICK_POV_BUTTON( _joystick, _button ) ( (ButtonCode_t)JOYSTICK_POV_BUTTON_INTERNAL( _joystick, _button ) )
#define JOYSTICK_AXIS_BUTTON( _joystick, _button ) ( (ButtonCode_t)JOYSTICK_AXIS_BUTTON_INTERNAL( _joystick, _button ) )

enum ButtonCode_t
{
	BUTTON_CODE_INVALID = -1,
	BUTTON_CODE_NONE = 0,

	KEY_FIRST = 0,

	KEY_NONE = KEY_FIRST,
	KEY_0,
	KEY_1,
	KEY_2,
	KEY_3,
	KEY_4,
	KEY_5,
	KEY_6,
	KEY_7,
	KEY_8,
	KEY_9,
	KEY_A,
	KEY_B,
	KEY_C,
	KEY_D,
	KEY_E,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_I,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_M,
	KEY_N,
	KEY_O,
	KEY_P,
	KEY_Q,
	KEY_R,
	KEY_S,
	KEY_T,
	KEY_U,
	KEY_V,
	KEY_W,
	KEY_X,
	KEY_Y,
	KEY_Z,
	KEY_PAD_0,
	KEY_PAD_1,
	KEY_PAD_2,
	KEY_PAD_3,
	KEY_PAD_4,
	KEY_PAD_5,
	KEY_PAD_6,
	KEY_PAD_7,
	KEY_PAD_8,
	KEY_PAD_9,
	KEY_PAD_DIVIDE,
	KEY_PAD_MULTIPLY,
	KEY_PAD_MINUS,
	KEY_PAD_PLUS,
	KEY_PAD_ENTER,
	KEY_PAD_DECIMAL,
	KEY_LBRACKET,
	KEY_RBRACKET,
	KEY_SEMICOLON,
	KEY_APOSTROPHE,
	KEY_BACKQUOTE,
	KEY_COMMA,
	KEY_PERIOD,
	KEY_SLASH,
	KEY_BACKSLASH,
	KEY_MINUS,
	KEY_EQUAL,
	KEY_ENTER,
	KEY_SPACE,
	KEY_BACKSPACE,
	KEY_TAB,
	KEY_CAPSLOCK,
	KEY_NUMLOCK,
	KEY_ESCAPE,
	KEY_SCROLLLOCK,
	KEY_INSERT,
	KEY_DELETE,
	KEY_HOME,
	KEY_END,
	KEY_PAGEUP,
	KEY_PAGEDOWN,
	KEY_BREAK,
	KEY_LSHIFT,
	KEY_RSHIFT,
	KEY_LALT,
	KEY_RALT,
	KEY_LCONTROL,
	KEY_RCONTROL,
	KEY_LWIN,
	KEY_RWIN,
	KEY_APP,
	KEY_UP,
	KEY_LEFT,
	KEY_DOWN,
	KEY_RIGHT,
	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	KEY_F6,
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_F11,
	KEY_F12,
	KEY_CAPSLOCKTOGGLE,
	KEY_NUMLOCKTOGGLE,
	KEY_SCROLLLOCKTOGGLE,

	KEY_LAST = KEY_SCROLLLOCKTOGGLE,
	KEY_COUNT = KEY_LAST - KEY_FIRST + 1,

	// Mouse
	MOUSE_FIRST = KEY_LAST + 1,

	MOUSE_LEFT = MOUSE_FIRST,
	MOUSE_RIGHT,
	MOUSE_MIDDLE,
	MOUSE_4,
	MOUSE_5,
	MOUSE_WHEEL_UP,		// A fake button which is 'pressed' and 'released' when the wheel is moved up 
	MOUSE_WHEEL_DOWN,	// A fake button which is 'pressed' and 'released' when the wheel is moved down

	MOUSE_LAST = MOUSE_WHEEL_DOWN,
	MOUSE_COUNT = MOUSE_LAST - MOUSE_FIRST + 1,

	// Joystick
	JOYSTICK_FIRST = MOUSE_LAST + 1,

	JOYSTICK_FIRST_BUTTON = JOYSTICK_FIRST,
	JOYSTICK_LAST_BUTTON = JOYSTICK_BUTTON_INTERNAL(MAX_JOYSTICKS - 1, JOYSTICK_MAX_BUTTON_COUNT - 1),
	JOYSTICK_FIRST_POV_BUTTON,
	JOYSTICK_LAST_POV_BUTTON = JOYSTICK_POV_BUTTON_INTERNAL(MAX_JOYSTICKS - 1, JOYSTICK_POV_BUTTON_COUNT - 1),
	JOYSTICK_FIRST_AXIS_BUTTON,
	JOYSTICK_LAST_AXIS_BUTTON = JOYSTICK_AXIS_BUTTON_INTERNAL(MAX_JOYSTICKS - 1, JOYSTICK_AXIS_BUTTON_COUNT - 1),

	JOYSTICK_LAST = JOYSTICK_LAST_AXIS_BUTTON,

	BUTTON_CODE_LAST,
	BUTTON_CODE_COUNT = BUTTON_CODE_LAST - KEY_FIRST + 1,

	// Helpers for XBox 360
	KEY_XBUTTON_UP = JOYSTICK_FIRST_POV_BUTTON,	// POV buttons
	KEY_XBUTTON_RIGHT,
	KEY_XBUTTON_DOWN,
	KEY_XBUTTON_LEFT,

	KEY_XBUTTON_A = JOYSTICK_FIRST_BUTTON,		// Buttons
	KEY_XBUTTON_B,
	KEY_XBUTTON_X,
	KEY_XBUTTON_Y,
	KEY_XBUTTON_LEFT_SHOULDER,
	KEY_XBUTTON_RIGHT_SHOULDER,
	KEY_XBUTTON_BACK,
	KEY_XBUTTON_START,
	KEY_XBUTTON_STICK1,
	KEY_XBUTTON_STICK2,
	KEY_XBUTTON_INACTIVE_START,

	KEY_XSTICK1_RIGHT = JOYSTICK_FIRST_AXIS_BUTTON,	// XAXIS POSITIVE
	KEY_XSTICK1_LEFT,							// XAXIS NEGATIVE
	KEY_XSTICK1_DOWN,							// YAXIS POSITIVE
	KEY_XSTICK1_UP,								// YAXIS NEGATIVE
	KEY_XBUTTON_LTRIGGER,						// ZAXIS POSITIVE
	KEY_XBUTTON_RTRIGGER,						// ZAXIS NEGATIVE
	KEY_XSTICK2_RIGHT,							// UAXIS POSITIVE
	KEY_XSTICK2_LEFT,							// UAXIS NEGATIVE
	KEY_XSTICK2_DOWN,							// VAXIS POSITIVE
	KEY_XSTICK2_UP,								// VAXIS NEGATIVE
};

#define MAXSTUDIOSKINS		32		// total textures
#define MAXSTUDIOBONES		256 //128		// total bones actually used
#define MAXSTUDIOFLEXDESC	1024	// maximum number of low level flexes (actual morph targets)
#define MAXSTUDIOFLEXCTRL	96		// maximum number of flexcontrollers (input sliders)
#define MAXSTUDIOPOSEPARAM	24
#define MAXSTUDIOBONECTRLS	4
#define MAXSTUDIOANIMBLOCKS 256

#define BONE_CALCULATE_MASK			0x1F
#define BONE_PHYSICALLY_SIMULATED	0x01	// bone is physically simulated when physics are active
#define BONE_PHYSICS_PROCEDURAL		0x02	// procedural when physics is active
#define BONE_ALWAYS_PROCEDURAL		0x04	// bone is always procedurally animated
#define BONE_SCREEN_ALIGN_SPHERE	0x08	// bone aligns to the screen, not constrained in motion.
#define BONE_SCREEN_ALIGN_CYLINDER	0x10	// bone aligns to the screen, constrained by it's own axis.

#define BONE_USED_MASK				0x0007FF00
#define BONE_USED_BY_ANYTHING		0x0007FF00
#define BONE_USED_BY_HITBOX			0x00000100	// bone (or child) is used by a hit box
#define BONE_USED_BY_ATTACHMENT		0x00000200	// bone (or child) is used by an attachment point
#define BONE_USED_BY_VERTEX_MASK	0x0003FC00
#define BONE_USED_BY_VERTEX_LOD0	0x00000400	// bone (or child) is used by the toplevel model via skinned vertex
#define BONE_USED_BY_VERTEX_LOD1	0x00000800	
#define BONE_USED_BY_VERTEX_LOD2	0x00001000  
#define BONE_USED_BY_VERTEX_LOD3	0x00002000
#define BONE_USED_BY_VERTEX_LOD4	0x00004000
#define BONE_USED_BY_VERTEX_LOD5	0x00008000
#define BONE_USED_BY_VERTEX_LOD6	0x00010000
#define BONE_USED_BY_VERTEX_LOD7	0x00020000
#define BONE_USED_BY_BONE_MERGE		0x00040000	// bone is available for bone merge to occur against it

#define BONE_USED_BY_VERTEX_AT_LOD(lod) ( BONE_USED_BY_VERTEX_LOD0 << (lod) )
#define BONE_USED_BY_ANYTHING_AT_LOD(lod) ( ( BONE_USED_BY_ANYTHING & ~BONE_USED_BY_VERTEX_MASK ) | BONE_USED_BY_VERTEX_AT_LOD(lod) )

#define MAX_NUM_LODS 8

#define BONE_TYPE_MASK				0x00F00000
#define BONE_FIXED_ALIGNMENT		0x00100000	// bone can't spin 360 degrees, all interpolation is normalized around a fixed orientation

#define BONE_HAS_SAVEFRAME_POS		0x00200000	// Vector48
#define BONE_HAS_SAVEFRAME_ROT64	0x00400000	// Quaternion64
#define BONE_HAS_SAVEFRAME_ROT32	0x00800000	// Quaternion32

#define	HITGROUP_GENERIC	0
#define	HITGROUP_HEAD		1
#define	HITGROUP_CHEST		2
#define	HITGROUP_STOMACH	3
#define HITGROUP_LEFTARM	4	
#define HITGROUP_RIGHTARM	5
#define HITGROUP_LEFTLEG	6
#define HITGROUP_RIGHTLEG	7
#define HITGROUP_NECK		8
#define HITGROUP_GEAR		10			// alerts NPC, but doesn't do damage or bleed (1/100th damage)

extern std::string get_hitgroup_name(int id);

struct model_t
{
	char        name[255];
	char pad[17]; //255
	int type; //0x110
	char pad2[36]; //0x114
	unsigned short studio; //0x138 //MDLHandle_t
						   //there is more
};
typedef unsigned short ModelInstanceHandle_t;

struct ModelRenderInfo_t
{
	Vector origin;
	QAngle angles;
	float uniformscale;
	void *pRenderable;
	const model_t *pModel;
	const matrix3x4_t *pModelToWorld;
	const matrix3x4_t *pLightingOffset;
	const Vector *pLightingOrigin;
	int flags;
	int entity_index;
	int skin;
	int body;
	int hitboxset;
	ModelInstanceHandle_t instance;
	ModelRenderInfo_t()
	{
		pModelToWorld = NULL;
		pLightingOffset = NULL;
		pLightingOrigin = NULL;
	}
};

enum OverrideType_t
{
	OVERRIDE_NORMAL = 0,
	OVERRIDE_BUILD_SHADOWS,
	OVERRIDE_DEPTH_WRITE,
};

enum MaterialPropertyTypes_t
{
	MATERIAL_PROPERTY_NEEDS_LIGHTMAP = 0,					// bool
	MATERIAL_PROPERTY_OPACITY,								// int (enum MaterialPropertyOpacityTypes_t)
	MATERIAL_PROPERTY_REFLECTIVITY,							// vec3_t
	MATERIAL_PROPERTY_NEEDS_BUMPED_LIGHTMAPS				// bool
};

enum ImageFormat
{
	IMAGE_FORMAT_UNKNOWN = -1,
	IMAGE_FORMAT_RGBA8888 = 0,
	IMAGE_FORMAT_ABGR8888,
	IMAGE_FORMAT_RGB888,
	IMAGE_FORMAT_BGR888,
	IMAGE_FORMAT_RGB565,
	IMAGE_FORMAT_I8,
	IMAGE_FORMAT_IA88,
	IMAGE_FORMAT_P8,
	IMAGE_FORMAT_A8,
	IMAGE_FORMAT_RGB888_BLUESCREEN,
	IMAGE_FORMAT_BGR888_BLUESCREEN,
	IMAGE_FORMAT_ARGB8888,
	IMAGE_FORMAT_BGRA8888,
	IMAGE_FORMAT_DXT1,
	IMAGE_FORMAT_DXT3,
	IMAGE_FORMAT_DXT5,
	IMAGE_FORMAT_BGRX8888,
	IMAGE_FORMAT_BGR565,
	IMAGE_FORMAT_BGRX5551,
	IMAGE_FORMAT_BGRA4444,
	IMAGE_FORMAT_DXT1_ONEBITALPHA,
	IMAGE_FORMAT_BGRA5551,
	IMAGE_FORMAT_UV88,
	IMAGE_FORMAT_UVWQ8888,
	IMAGE_FORMAT_RGBA16161616F,
	IMAGE_FORMAT_RGBA16161616,
	IMAGE_FORMAT_UVLX8888,
	IMAGE_FORMAT_R32F,			// Single-channel 32-bit floating point
	IMAGE_FORMAT_RGB323232F,	// NOTE: D3D9 does not have this format
	IMAGE_FORMAT_RGBA32323232F,
	IMAGE_FORMAT_RG1616F,
	IMAGE_FORMAT_RG3232F,
	IMAGE_FORMAT_RGBX8888,

	IMAGE_FORMAT_NULL,			// Dummy format which takes no video memory

								// Compressed normal map formats
								IMAGE_FORMAT_ATI2N,			// One-surface ATI2N / DXN format
								IMAGE_FORMAT_ATI1N,			// Two-surface ATI1N format

								IMAGE_FORMAT_RGBA1010102,	// 10 bit-per component render targets
								IMAGE_FORMAT_BGRA1010102,
								IMAGE_FORMAT_R16F,			// 16 bit FP format

															// Depth-stencil texture formats
															IMAGE_FORMAT_D16,
															IMAGE_FORMAT_D15S1,
															IMAGE_FORMAT_D32,
															IMAGE_FORMAT_D24S8,
															IMAGE_FORMAT_LINEAR_D24S8,
															IMAGE_FORMAT_D24X8,
															IMAGE_FORMAT_D24X4S4,
															IMAGE_FORMAT_D24FS8,
															IMAGE_FORMAT_D16_SHADOW,	// Specific formats for shadow mapping
															IMAGE_FORMAT_D24X8_SHADOW,	// Specific formats for shadow mapping

																						// supporting these specific formats as non-tiled for procedural cpu access (360-specific)
																						IMAGE_FORMAT_LINEAR_BGRX8888,
																						IMAGE_FORMAT_LINEAR_RGBA8888,
																						IMAGE_FORMAT_LINEAR_ABGR8888,
																						IMAGE_FORMAT_LINEAR_ARGB8888,
																						IMAGE_FORMAT_LINEAR_BGRA8888,
																						IMAGE_FORMAT_LINEAR_RGB888,
																						IMAGE_FORMAT_LINEAR_BGR888,
																						IMAGE_FORMAT_LINEAR_BGRX5551,
																						IMAGE_FORMAT_LINEAR_I8,
																						IMAGE_FORMAT_LINEAR_RGBA16161616,

																						IMAGE_FORMAT_LE_BGRX8888,
																						IMAGE_FORMAT_LE_BGRA8888,

																						NUM_IMAGE_FORMATS
};

class CBaseHandle;

// An IHandleEntity-derived class can go into an entity list and use ehandles.
class IHandleEntity
{
public:
	virtual ~IHandleEntity() {}
	virtual void SetRefEHandle(const CBaseHandle &handle) = 0;
	virtual const CBaseHandle& GetRefEHandle() const = 0;
};

#define	DAMAGE_NO				0
#define DAMAGE_EVENTS_ONLY		1		// Call damage functions, but don't modify health
#define	DAMAGE_YES				2
#define	DAMAGE_AIM				3

enum Collision_Group_t
{
	COLLISION_GROUP_NONE = 0,
	COLLISION_GROUP_DEBRIS,			// Collides with nothing but world and static stuff
	COLLISION_GROUP_DEBRIS_TRIGGER, // Same as debris, but hits triggers
	COLLISION_GROUP_INTERACTIVE_DEBRIS,	// Collides with everything except other interactive debris or debris
	COLLISION_GROUP_INTERACTIVE,	// Collides with everything except interactive debris or debris
	COLLISION_GROUP_PLAYER,
	COLLISION_GROUP_BREAKABLE_GLASS,
	COLLISION_GROUP_VEHICLE,
	COLLISION_GROUP_PLAYER_MOVEMENT,  // For HL2, same as Collision_Group_Player, for
									  // TF2, this filters out other players and CBaseObjects
									  COLLISION_GROUP_NPC,			// Generic NPC group
									  COLLISION_GROUP_IN_VEHICLE,		// for any entity inside a vehicle
									  COLLISION_GROUP_WEAPON,			// for any weapons that need collision detection
									  COLLISION_GROUP_VEHICLE_CLIP,	// vehicle clip brush to restrict vehicle movement
									  COLLISION_GROUP_PROJECTILE,		// Projectiles!
									  COLLISION_GROUP_DOOR_BLOCKER,	// Blocks entities not permitted to get near moving doors
									  COLLISION_GROUP_PASSABLE_DOOR,	// Doors that the player shouldn't collide with
									  COLLISION_GROUP_DISSOLVING,		// Things that are dissolving are in this group
									  COLLISION_GROUP_PUSHAWAY,		// Nonsolid on client and server, pushaway in player code

									  COLLISION_GROUP_NPC_ACTOR,		// Used so NPCs in scripts ignore the player.
									  COLLISION_GROUP_NPC_SCRIPTED,	// USed for NPCs in scripts that should not collide with each other
									  COLLISION_GROUP_PZ_CLIP,



									  COLLISION_GROUP_DEBRIS_BLOCK_PROJECTILE, // Only collides with bullets

									  LAST_SHARED_COLLISION_GROUP
};

struct string_t
{
public:
	bool operator!() const { return (pszValue == NULL); }
	bool operator==(const string_t &rhs) const { return (pszValue == rhs.pszValue); }
	bool operator!=(const string_t &rhs) const { return (pszValue != rhs.pszValue); }
	bool operator<(const string_t &rhs) const { return ((void *)pszValue < (void *)rhs.pszValue); }

	const char *ToCStr() const { return (pszValue) ? pszValue : ""; }

protected:
	const char *pszValue;
};

#define PHYSICS_MULTIPLAYER_AUTODETECT	0	// use multiplayer physics mode as defined in model prop data
#define PHYSICS_MULTIPLAYER_SOLID		1	// soild, pushes player away 
#define PHYSICS_MULTIPLAYER_NON_SOLID	2	// nonsolid, but pushed by player
#define PHYSICS_MULTIPLAYER_CLIENTSIDE	3	// Clientside only, nonsolid 	

class IMultiplayerPhysics
{
public:
	virtual int		GetMultiplayerPhysicsMode() = 0;
	virtual float	GetMass() = 0;
	virtual bool	IsAsleep() = 0;
};

enum propdata_interactions_t
{
	PROPINTER_PHYSGUN_WORLD_STICK,		// "onworldimpact"	"stick"
	PROPINTER_PHYSGUN_FIRST_BREAK,		// "onfirstimpact"	"break"
	PROPINTER_PHYSGUN_FIRST_PAINT,		// "onfirstimpact"	"paintsplat"
	PROPINTER_PHYSGUN_FIRST_IMPALE,		// "onfirstimpact"	"impale"
	PROPINTER_PHYSGUN_LAUNCH_SPIN_NONE,	// "onlaunch"		"spin_none"
	PROPINTER_PHYSGUN_LAUNCH_SPIN_Z,	// "onlaunch"		"spin_zaxis"
	PROPINTER_PHYSGUN_BREAK_EXPLODE,	// "onbreak"		"explode_fire"
	PROPINTER_PHYSGUN_BREAK_EXPLODE_ICE,	// "onbreak"	"explode_ice"
	PROPINTER_PHYSGUN_DAMAGE_NONE,		// "damage"			"none"

	PROPINTER_FIRE_FLAMMABLE,			// "flammable"			"yes"
	PROPINTER_FIRE_EXPLOSIVE_RESIST,	// "explosive_resist"	"yes"
	PROPINTER_FIRE_IGNITE_HALFHEALTH,	// "ignite"				"halfhealth"

	PROPINTER_PHYSGUN_CREATE_FLARE,		// "onpickup"		"create_flare"

	PROPINTER_PHYSGUN_ALLOW_OVERHEAD,	// "allow_overhead"	"yes"

	PROPINTER_WORLD_BLOODSPLAT,			// "onworldimpact", "bloodsplat"

	PROPINTER_PHYSGUN_NOTIFY_CHILDREN,	// "onfirstimpact" cause attached flechettes to explode
	PROPINTER_MELEE_IMMUNE,				// "melee_immune"	"yes"

										// If we get more than 32 of these, we'll need a different system

										PROPINTER_NUM_INTERACTIONS,
};

enum mp_break_t
{
	MULTIPLAYER_BREAK_DEFAULT,
	MULTIPLAYER_BREAK_SERVERSIDE,
	MULTIPLAYER_BREAK_CLIENTSIDE,
	MULTIPLAYER_BREAK_BOTH
};

class IBreakableWithPropData
{
public:
	// Damage modifiers
	virtual void		SetDmgModBullet(float flDmgMod) = 0;
	virtual void		SetDmgModClub(float flDmgMod) = 0;
	virtual void		SetDmgModExplosive(float flDmgMod) = 0;
	virtual float		GetDmgModBullet(void) = 0;
	virtual float		GetDmgModClub(void) = 0;
	virtual float		GetDmgModExplosive(void) = 0;
	virtual float		GetDmgModFire(void) = 0;

	// Explosive
	virtual void		SetExplosiveRadius(float flRadius) = 0;
	virtual void		SetExplosiveDamage(float flDamage) = 0;
	virtual float		GetExplosiveRadius(void) = 0;
	virtual float		GetExplosiveDamage(void) = 0;

	// Physics damage tables
	virtual void		SetPhysicsDamageTable(string_t iszTableName) = 0;
	virtual string_t	GetPhysicsDamageTable(void) = 0;

	// Breakable chunks
	virtual void		SetBreakableModel(string_t iszModel) = 0;
	virtual string_t 	GetBreakableModel(void) = 0;
	virtual void		SetBreakableSkin(int iSkin) = 0;
	virtual int			GetBreakableSkin(void) = 0;
	virtual void		SetBreakableCount(int iCount) = 0;
	virtual int			GetBreakableCount(void) = 0;
	virtual void		SetMaxBreakableSize(int iSize) = 0;
	virtual int			GetMaxBreakableSize(void) = 0;

	// LOS blocking
	virtual void		SetPropDataBlocksLOS(bool bBlocksLOS) = 0;
	virtual void		SetPropDataIsAIWalkable(bool bBlocksLOS) = 0;

	// Interactions
	virtual void		SetInteraction(propdata_interactions_t Interaction) = 0;
	virtual bool		HasInteraction(propdata_interactions_t Interaction) = 0;

	// Multiplayer physics mode
	virtual void		SetPhysicsMode(int iMode) = 0;
	virtual int			GetPhysicsMode() = 0;

	// Multiplayer breakable spawn behavior
	virtual void		SetMultiplayerBreakMode(mp_break_t mode) = 0;
	virtual mp_break_t	GetMultiplayerBreakMode(void) const = 0;

	// Used for debugging
	virtual void		SetBasePropData(string_t iszBase) = 0;
	virtual string_t	GetBasePropData(void) = 0;
};
#define FW_DONTCARE         0
#define FW_THIN             100
#define FW_EXTRALIGHT       200
#define FW_LIGHT            300
#define FW_NORMAL           400
#define FW_MEDIUM           500
#define FW_SEMIBOLD         600
#define FW_BOLD             700
#define FW_EXTRABOLD        800
#define FW_HEAVY            900

enum FontFlags_t
{
	FONTFLAG_NONE,
	FONTFLAG_ITALIC = 0x001,
	FONTFLAG_UNDERLINE = 0x002,
	FONTFLAG_STRIKEOUT = 0x004,
	FONTFLAG_SYMBOL = 0x008,
	FONTFLAG_ANTIALIAS = 0x010,
	FONTFLAG_GAUSSIANBLUR = 0x020,
	FONTFLAG_ROTARY = 0x040,
	FONTFLAG_DROPSHADOW = 0x080,
	FONTFLAG_ADDITIVE = 0x100,
	FONTFLAG_OUTLINE = 0x200,
	FONTFLAG_CUSTOM = 0x400,
	FONTFLAG_BITMAP = 0x800,
};

struct IntRect
{
	int x0;
	int y0;
	int w;
	int h;
};

class ISurface
{
public:
	void		DrawSetColor(int r, int g, int b, int a);
	void		DrawSetColor(Color col);
	void		DrawRect(int x0, int y0, int x1, int y1);
	void		DrawFilledRect(int x0, int y0, int x1, int y1);
	void		DrawOutlinedRect(int x0, int y0, int x1, int y1);
	void		DrawLine(int x0, int y0, int x1, int y1);
	void		DrawPolyLine(int *px, int *py, int numPoints);
	void		DrawSetTextFont(HFont font);
	void		DrawSetTextColor(int r, int g, int b, int a);
	void		DrawSetTextColor(Color col);
	void		DrawSetTextPos(int x, int y);
	void		DrawPrintText(const wchar_t *text, int textLen, FontDrawType_t drawType = FONT_DRAW_DEFAULT);
	void		DrawSetTextureRGBA(int id, const unsigned char *rgba, int wide, int tall);
	void		DrawSetTexture(int id);
	int			CreateNewTextureID(bool procedural = false);
	HFont		Create_Font();
	bool		SetFontGlyphSet(HFont font, const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags, int nRangeMin = 0, int nRangeMax = 0);
	void		GetTextSize(HFont font, const wchar_t *text, int &wide, int &tall);
	void		DrawOutlinedCircle(int x, int y, int radius, int segments);
	void		DrawTexturedPolygon(int n, Vertex_t *pVertice, bool bClipVertices = true);
	int         GetFontTall(HFont font);
	void		Play_Sound(const char* pSample);
};
#define FCVAR_NONE				0 

// Command to ConVars and ConCommands
// ConVar Systems
#define FCVAR_UNREGISTERED		(1<<0)	// If this is set, don't add to linked list, etc.
#define FCVAR_DEVELOPMENTONLY	(1<<1)	// Hidden in released products. Flag is removed automatically if ALLOW_DEVELOPMENT_CVARS is defined.
#define FCVAR_GAMEDLL			(1<<2)	// defined by the game DLL
#define FCVAR_CLIENTDLL			(1<<3)  // defined by the client DLL
#define FCVAR_HIDDEN			(1<<4)	// Hidden. Doesn't appear in find or autocomplete. Like DEVELOPMENTONLY, but can't be compiled out.

// ConVar only
#define FCVAR_PROTECTED			(1<<5)  // It's a server cvar, but we don't send the data since it's a password, etc.  Sends 1 if it's not bland/zero, 0 otherwise as value
#define FCVAR_SPONLY			(1<<6)  // This cvar cannot be changed by clients connected to a multiplayer server.
#define	FCVAR_ARCHIVE			(1<<7)	// set to cause it to be saved to vars.rc
#define	FCVAR_NOTIFY			(1<<8)	// notifies players when changed
#define	FCVAR_USERINFO			(1<<9)	// changes the client's info string
#define FCVAR_CHEAT				(1<<14) // Only useable in singleplayer / debug / multiplayer & sv_cheats

#define FCVAR_PRINTABLEONLY		(1<<10)  // This cvar's string cannot contain unprintable characters ( e.g., used for player name etc ).
#define FCVAR_UNLOGGED			(1<<11)  // If this is a FCVAR_SERVER, don't log changes to the log file / console if we are creating a log
#define FCVAR_NEVER_AS_STRING	(1<<12)  // never try to print that cvar

// It's a ConVar that's shared between the client and the server.
// At signon, the values of all such ConVars are sent from the server to the client (skipped for local
//  client, of course )
// If a change is requested it must come from the console (i.e., no remote client changes)
// If a value is changed while a server is active, it's replicated to all connected clients
#define FCVAR_REPLICATED		(1<<13)	// server setting enforced on clients, TODO rename to FCAR_SERVER at some time
#define FCVAR_DEMO				(1<<16)  // record this cvar when starting a demo file
#define FCVAR_DONTRECORD		(1<<17)  // don't record these command in demofiles
#define FCVAR_RELOAD_MATERIALS	(1<<20)	// If this cvar changes, it forces a material reload
#define FCVAR_RELOAD_TEXTURES	(1<<21)	// If this cvar changes, if forces a texture reload

#define FCVAR_NOT_CONNECTED		(1<<22)	// cvar cannot be changed by a client that is connected to a server
#define FCVAR_MATERIAL_SYSTEM_THREAD (1<<23)	// Indicates this cvar is read from the material system thread
#define FCVAR_ARCHIVE_XBOX		(1<<24) // cvar written to config.cfg on the Xbox

#define FCVAR_ACCESSIBLE_FROM_THREADS	(1<<25)	// used as a debugging tool necessary to check material system thread convars

#define FCVAR_SERVER_CAN_EXECUTE	(1<<28)// the server is allowed to execute this command on clients via ClientCommand/NET_StringCmd/CBaseClientState::ProcessStringCmd.
#define FCVAR_SERVER_CANNOT_QUERY	(1<<29)// If this is set, then the server is not allowed to query this cvar's value (via IServerPluginHelpers::StartQueryCvarValue).
#define FCVAR_CLIENTCMD_CAN_EXECUTE	(1<<30)	// IVEngineClient::ClientCmd is allowed to execute this command. 
// Note: IVEngineClient::ClientCmd_Unrestricted can run any client command.

// #define FCVAR_AVAILABLE			(1<<15)
// #define FCVAR_AVAILABLE			(1<<18)
// #define FCVAR_AVAILABLE			(1<<19)
// #define FCVAR_AVAILABLE			(1<<20)
// #define FCVAR_AVAILABLE			(1<<21)
// #define FCVAR_AVAILABLE			(1<<23)
// #define FCVAR_AVAILABLE			(1<<26)
// #define FCVAR_AVAILABLE			(1<<27)
// #define FCVAR_AVAILABLE			(1<<31)

#define FCVAR_MATERIAL_THREAD_MASK ( FCVAR_RELOAD_MATERIALS | FCVAR_RELOAD_TEXTURES | FCVAR_MATERIAL_SYSTEM_THREAD )	

struct FileHandle_t;
class CUtlBuffer;

typedef bool(*GetSymbolProc_t)(const char *pKey);

class KeyValues
{
public:
	KeyValues(const char *setName);
	void CallConstructor(const char *setName);

	//
	// AutoDelete class to automatically free the keyvalues.
	// Simply construct it with the keyvalues you allocated and it will free them when falls out of scope.
	// When you decide that keyvalues shouldn't be deleted call Assign(NULL) on it.
	// If you constructed AutoDelete(NULL) you can later assign the keyvalues to be deleted with Assign(pKeyValues).
	//
	class AutoDelete
	{
	public:
		explicit inline AutoDelete(KeyValues *pKeyValues) : m_pKeyValues(pKeyValues) {}
		explicit inline AutoDelete(const char *pchKVName) : m_pKeyValues(new KeyValues(pchKVName)) {}
		inline ~AutoDelete(void) { if (m_pKeyValues) m_pKeyValues->deleteThis(); }
		inline void Assign(KeyValues *pKeyValues) { m_pKeyValues = pKeyValues; }
		KeyValues *operator->() { return m_pKeyValues; }
		operator KeyValues *() { return m_pKeyValues; }
	private:
		AutoDelete(AutoDelete const &x); // forbid
		AutoDelete & operator= (AutoDelete const &x); // forbid
	protected:
		KeyValues *m_pKeyValues;
	};

	//
	// AutoDeleteInline is useful when you want to hold your keyvalues object inside
	// and delete it right after using.
	// You can also pass temporary KeyValues object as an argument to a function by wrapping it into KeyValues::AutoDeleteInline
	// instance:   call_my_function( KeyValues::AutoDeleteInline( new KeyValues( "test" ) ) )
	//
	class AutoDeleteInline : public AutoDelete
	{
	public:
		explicit inline AutoDeleteInline(KeyValues *pKeyValues) : AutoDelete(pKeyValues) {}
		inline operator KeyValues *() const { return m_pKeyValues; }
		inline KeyValues * Get() const { return m_pKeyValues; }
	};

	// Quick setup constructors
	KeyValues(const char *setName, const char *firstKey, const char *firstValue);
	KeyValues(const char *setName, const char *firstKey, const wchar_t *firstValue);
	KeyValues(const char *setName, const char *firstKey, int firstValue);
	KeyValues(const char *setName, const char *firstKey, const char *firstValue, const char *secondKey, const char *secondValue);
	KeyValues(const char *setName, const char *firstKey, int firstValue, const char *secondKey, int secondValue);

	// Section name
	const char *GetName() const;
	void SetName(const char *setName);

	// gets the name as a unique int
	int GetNameSymbol() const;
	int GetNameSymbolCaseSensitive() const;

	// File access. Set UsesEscapeSequences true, if resource file/buffer uses Escape Sequences (eg \n, \t)
	void UsesEscapeSequences(bool state); // default false
	bool LoadFromFile(void *filesystem, const char *resourceName, const char *pathID = NULL, GetSymbolProc_t pfnEvaluateSymbolProc = NULL);
	bool SaveToFile(void *filesystem, const char *resourceName, const char *pathID = NULL);

	// Read from a buffer...  Note that the buffer must be null terminated
	bool LoadFromBuffer(char const *resourceName, const char *pBuffer, void* pFileSystem = NULL, const char *pPathID = NULL, GetSymbolProc_t pfnEvaluateSymbolProc = NULL);

	// Read from a utlbuffer...
	bool LoadFromBuffer(char const *resourceName, CUtlBuffer &buf, void* pFileSystem = NULL, const char *pPathID = NULL, GetSymbolProc_t pfnEvaluateSymbolProc = NULL);

	// Find a keyValue, create it if it is not found.
	// Set bCreate to true to create the key if it doesn't already exist (which ensures a valid pointer will be returned)
	KeyValues *FindKey(const char *keyName, bool bCreate = false);
	KeyValues *FindKey(int keySymbol) const;
	KeyValues *CreateNewKey();		// creates a new key, with an autogenerated name.  name is guaranteed to be an integer, of value 1 higher than the highest other integer key name
	void AddSubKey(KeyValues *pSubkey);	// Adds a subkey. Make sure the subkey isn't a child of some other keyvalues
	void RemoveSubKey(KeyValues *subKey);	// removes a subkey from the list, DOES NOT DELETE IT
	void InsertSubKey(int nIndex, KeyValues *pSubKey); // Inserts the given sub-key before the Nth child location
	bool ContainsSubKey(KeyValues *pSubKey); // Returns true if this key values contains the specified sub key, false otherwise.
	void SwapSubKey(KeyValues *pExistingSubKey, KeyValues *pNewSubKey);	// Swaps an existing subkey for a new one, DOES NOT DELETE THE OLD ONE but takes ownership of the new one
	void ElideSubKey(KeyValues *pSubKey);	// Removes a subkey but inserts all of its children in its place, in-order (flattens a tree, like firing a manager!)

											// Key iteration.
											//
											// NOTE: GetFirstSubKey/GetNextKey will iterate keys AND values. Use the functions 
											// below if you want to iterate over just the keys or just the values.
											//
	KeyValues *GetFirstSubKey();	// returns the first subkey in the list
	KeyValues *GetNextKey();		// returns the next subkey
	void SetNextKey(KeyValues * pDat);

	//
	// These functions can be used to treat it like a true key/values tree instead of 
	// confusing values with keys.
	//
	// So if you wanted to iterate all subkeys, then all values, it would look like this:
	//     for ( KeyValues *pKey = pRoot->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey() )
	//     {
	//		   Msg( "Key name: %s\n", pKey->GetName() );
	//     }
	//     for ( KeyValues *pValue = pRoot->GetFirstValue(); pKey; pKey = pKey->GetNextValue() )
	//     {
	//         Msg( "Int value: %d\n", pValue->GetInt() );  // Assuming pValue->GetDataType() == TYPE_INT...
	//     }
	KeyValues* GetFirstTrueSubKey();
	KeyValues* GetNextTrueSubKey();

	KeyValues* GetFirstValue();	// When you get a value back, you can use GetX and pass in NULL to get the value.
	KeyValues* GetNextValue();


	// Data access
	int   GetInt(const char *keyName = NULL, int defaultValue = 0);
	uint64 GetUint64(const char *keyName = NULL, uint64 defaultValue = 0);
	float GetFloat(const char *keyName = NULL, float defaultValue = 0.0f);
	const char *GetString(const char *keyName = NULL, const char *defaultValue = "");
	const wchar_t *GetWString(const char *keyName = NULL, const wchar_t *defaultValue = L"");
	void *GetPtr(const char *keyName = NULL, void *defaultValue = (void*)0);
	Color GetColor(const char *keyName = NULL, const Color &defaultColor = Color(0, 0, 0, 0));
	bool GetBool(const char *keyName = NULL, bool defaultValue = false) { return GetInt(keyName, defaultValue ? 1 : 0) ? true : false; }
	bool  IsEmpty(const char *keyName = NULL);

	// Data access
	int   GetInt(int keySymbol, int defaultValue = 0);
	uint64 GetUint64(int keySymbol, uint64 defaultValue = 0);
	float GetFloat(int keySymbol, float defaultValue = 0.0f);
	const char *GetString(int keySymbol, const char *defaultValue = "");
	const wchar_t *GetWString(int keySymbol, const wchar_t *defaultValue = L"");
	void *GetPtr(int keySymbol, void *defaultValue = (void*)0);
	Color GetColor(int keySymbol /* default value is all black */);
	bool GetBool(int keySymbol, bool defaultValue = false) { return GetInt(keySymbol, defaultValue ? 1 : 0) ? true : false; }
	bool  IsEmpty(int keySymbol);

	// Key writing
	void SetWString(const char *keyName, const wchar_t *value);
	void SetString(const char *keyName, const char *value);
	void SetInt(const char *keyName, int value);
	void SetUint64(const char *keyName, uint64 value);
	void SetFloat(const char *keyName, float value);
	void SetPtr(const char *keyName, void *value);
	void SetColor(const char *keyName, Color value);
	void SetBool(const char *keyName, bool value) { SetInt(keyName, value ? 1 : 0); }

	// Memory allocation (optimized)
	void *operator new(size_t iAllocSize);
	void *operator new(size_t iAllocSize, int nBlockUse, const char *pFileName, int nLine);
	void operator delete(void *pMem);
	void operator delete(void *pMem, int nBlockUse, const char *pFileName, int nLine);

	KeyValues& operator=(KeyValues& src);

	// Adds a chain... if we don't find stuff in this keyvalue, we'll look
	// in the one we're chained to.
	void ChainKeyValue(KeyValues* pChain);

	void RecursiveSaveToFile(CUtlBuffer& buf, int indentLevel);

	bool WriteAsBinary(CUtlBuffer &buffer);
	bool ReadAsBinary(CUtlBuffer &buffer);

	// Allocate & create a new copy of the keys
	KeyValues *MakeCopy(void) const;

	// Make a new copy of all subkeys, add them all to the passed-in keyvalues
	void CopySubkeys(KeyValues *pParent) const;

	// Clear out all subkeys, and the current value
	void Clear(void);

	// Data type
	enum types_t
	{
		TYPE_NONE = 0,
		TYPE_STRING,
		TYPE_INT,
		TYPE_FLOAT,
		TYPE_PTR,
		TYPE_WSTRING,
		TYPE_COLOR,
		TYPE_UINT64,
		TYPE_COMPILED_INT_BYTE,			// hack to collapse 1 byte ints in the compiled format
		TYPE_COMPILED_INT_0,			// hack to collapse 0 in the compiled format
		TYPE_COMPILED_INT_1,			// hack to collapse 1 in the compiled format
		TYPE_NUMTYPES,
	};
	types_t GetDataType(const char *keyName = NULL);

	// Virtual deletion function - ensures that KeyValues object is deleted from correct heap
	void deleteThis();

	void SetStringValue(char const *strValue);

	// unpack a key values list into a structure
	void UnpackIntoStructure(struct KeyValuesUnpackStructure const *pUnpackTable, void *pDest);

	// Process conditional keys for widescreen support.
	bool ProcessResolutionKeys(const char *pResString);

	// Dump keyvalues recursively into a dump context
	bool Dump(void *pDump, int nIndentLevel = 0);

	// Merge operations describing how two keyvalues can be combined
	enum MergeKeyValuesOp_t
	{
		MERGE_KV_ALL,
		MERGE_KV_UPDATE,	// update values are copied into storage, adding new keys to storage or updating existing ones
		MERGE_KV_DELETE,	// update values specify keys that get deleted from storage
		MERGE_KV_BORROW,	// update values only update existing keys in storage, keys in update that do not exist in storage are discarded
	};
	void MergeFrom(KeyValues *kvMerge, MergeKeyValuesOp_t eOp = MERGE_KV_ALL);

	// Assign keyvalues from a string
	static KeyValues * FromString(char const *szName, char const *szStringVal, char const **ppEndOfParse = NULL);

private:
	KeyValues(KeyValues&);	// prevent copy constructor being used

							// prevent delete being called except through deleteThis()
	~KeyValues();
	void CallDestructor();

	KeyValues* CreateKey(const char *keyName);

	void RecursiveCopyKeyValues(KeyValues& src);
	void RemoveEverything();
	//	void RecursiveSaveToFile( IBaseFileSystem *filesystem, CUtlBuffer &buffer, int indentLevel );
	//	void WriteConvertedString( CUtlBuffer &buffer, const char *pszString );

	// NOTE: If both filesystem and pBuf are non-null, it'll save to both of them.
	// If filesystem is null, it'll ignore f.
	void RecursiveSaveToFile(void *filesystem, FileHandle_t f, CUtlBuffer *pBuf, int indentLevel);
	void WriteConvertedString(void *filesystem, FileHandle_t f, CUtlBuffer *pBuf, const char *pszString);

	void RecursiveLoadFromBuffer(char const *resourceName, CUtlBuffer &buf, GetSymbolProc_t pfnEvaluateSymbolProc);

	// for handling #include "filename"
	void AppendIncludedKeys(/*CUtlVector< KeyValues * >&*/void* includedKeys);
	void ParseIncludedKeys(char const *resourceName, const char *filetoinclude,
		void* pFileSystem, const char *pPathID, /*CUtlVector< KeyValues * >&*/void* includedKeys, GetSymbolProc_t pfnEvaluateSymbolProc);

	// For handling #base "filename"
	void MergeBaseKeys(/*CUtlVector< KeyValues * >&*/ void* baseKeys);
	void RecursiveMergeKeyValues(KeyValues *baseKV);

	// NOTE: If both filesystem and pBuf are non-null, it'll save to both of them.
	// If filesystem is null, it'll ignore f.
	void InternalWrite(void *filesystem, FileHandle_t f, CUtlBuffer *pBuf, const void *pData, int len);

	void Init();
	const char * ReadToken(CUtlBuffer &buf, bool &wasQuoted, bool &wasConditional);
	void WriteIndents(void *filesystem, FileHandle_t f, CUtlBuffer *pBuf, int indentLevel);

	void FreeAllocatedValue();
	void AllocateValueBlock(int size);

	bool ReadAsBinaryPooledFormat(CUtlBuffer &buf, void *pFileSystem, unsigned int poolKey, GetSymbolProc_t pfnEvaluateSymbolProc);

	bool EvaluateConditional(const char *pExpressionString, GetSymbolProc_t pfnEvaluateSymbolProc);

public:

	uint32 m_iKeyName : 24;	// keyname is a symbol defined in KeyValuesSystem
	uint32 m_iKeyNameCaseSensitive1 : 8;	// 1st part of case sensitive symbol defined in KeyValueSystem

											// These are needed out of the union because the API returns string pointers
	char *m_sValue;
	wchar_t *m_wsValue;

	// we don't delete these
	union
	{
		int m_iValue;
		float m_flValue;
		void *m_pValue;
		unsigned char m_Color[4];
	};

	char	   m_iDataType;
	char	   m_bHasEscapeSequences; // true, if while parsing this KeyValue, Escape Sequences are used (default false)
	uint16	   m_iKeyNameCaseSensitive2;	// 2nd part of case sensitive symbol defined in KeyValueSystem;

	KeyValues *m_pPeer;	// pointer to next key in list
	KeyValues *m_pSub;	// pointer to Start of a new sub key list
	KeyValues *m_pChain;// Search here if it's not in our list
};

inline void *KeyValues::GetPtr(int keySymbol, void *defaultValue)
{
	KeyValues *dat = FindKey(keySymbol);
	return dat ? dat->GetPtr((const char *)NULL, defaultValue) : defaultValue;
}

void AngleQuaternion(const QAngle &angles, Quaternion &outQuat);

void QuaternionSlerp(const Quaternion &p, const Quaternion &q, float t, Quaternion &qt);

void QuaternionSlerpNoAlign(const Quaternion &p, const Quaternion &q, float t, Quaternion &qt);

void QuaternionAlign(const Quaternion &p, const Quaternion &q, Quaternion &qt);

void QuaternionMatrix(const Quaternion &q, matrix3x4_t& matrix);

void QuaternionMatrix(const Quaternion &q, const Vector &pos, matrix3x4_t &matrix);

void MatrixGetColumn(const matrix3x4_t& in, int column, Vector &out);

void MatrixAngles(const matrix3x4_t & matrix, float *angles); // !!!!

void MatrixAngles(const matrix3x4_t &mat, RadianEuler &angles, Vector &position);

void MatrixAngles(const matrix3x4_t &mat, Quaternion &q, Vector &position);

inline void MatrixPosition(const matrix3x4_t &matrix, Vector &position)
{
	MatrixGetColumn(matrix, 3, position);
}

inline void MatrixAngles(const matrix3x4_t &matrix, QAngle &angles)
{
	MatrixAngles(matrix, &angles.x);
}

inline void MatrixAngles(const matrix3x4_t &matrix, QAngle &angles, Vector &position)
{
	MatrixAngles(matrix, angles);
	MatrixPosition(matrix, position);
}

inline void MatrixAngles(const matrix3x4_t &matrix, RadianEuler &angles)
{
	MatrixAngles(matrix, &angles.x);

	angles.Init(DEG2RAD(angles.z), DEG2RAD(angles.x), DEG2RAD(angles.y));
}

inline void AngleMatrix(const QAngle &angles, matrix3x4_t& matrix)
{
#ifdef _VPROF_MATHLIB
	VPROF_BUDGET("AngleMatrix", "Mathlib");
#endif
	//Assert(s_bMathlibInitialized);

	float sr, sp, sy, cr, cp, cy;

#ifdef _X360
	fltx4 radians, scale, sine, cosine;
	radians = LoadUnaligned3SIMD(angles.Base());
	scale = ReplicateX4(M_PI_F / 180.f);
	radians = MulSIMD(radians, scale);
	SinCos3SIMD(sine, cosine, radians);

	sp = SubFloat(sine, 0);	sy = SubFloat(sine, 1);	sr = SubFloat(sine, 2);
	cp = SubFloat(cosine, 0);	cy = SubFloat(cosine, 1);	cr = SubFloat(cosine, 2);
#else
	SinCos(DEG2RAD(angles[YAW]), &sy, &cy);
	SinCos(DEG2RAD(angles[PITCH]), &sp, &cp);
	SinCos(DEG2RAD(angles[ROLL]), &sr, &cr);
#endif

	// matrix = (YAW * PITCH) * ROLL
	matrix[0][0] = cp * cy;
	matrix[1][0] = cp * sy;
	matrix[2][0] = -sp;

	float crcy = cr * cy;
	float crsy = cr * sy;
	float srcy = sr * cy;
	float srsy = sr * sy;
	matrix[0][1] = sp * srcy - crsy;
	matrix[1][1] = sp * srsy + crcy;
	matrix[2][1] = sr * cp;

	matrix[0][2] = (sp*crcy + srsy);
	matrix[1][2] = (sp*crsy - srcy);
	matrix[2][2] = cr * cp;

	matrix[0][3] = 0.0f;
	matrix[1][3] = 0.0f;
	matrix[2][3] = 0.0f;
}

inline void MatrixSetColumn(const Vector &in, int column, matrix3x4_t& out)
{
	out[0][column] = in.x;
	out[1][column] = in.y;
	out[2][column] = in.z;
}

inline void AngleMatrix(const QAngle &angles, const Vector &position, matrix3x4_t& matrix)
{
	AngleMatrix(angles, matrix);
	MatrixSetColumn(position, 3, matrix);
}

inline void PositionMatrix(const Vector &position, matrix3x4_t &mat)
{
	MatrixSetColumn(position, 3, mat);
}

inline void SetIdentityMatrix(matrix3x4_t& matrix)
{
	memset(matrix.Base(), 0, sizeof(float) * 3 * 4);
	matrix[0][0] = 1.0;
	matrix[1][1] = 1.0;
	matrix[2][2] = 1.0;
}


// In win32 try to use the intrinsic fabs so the optimizer can do it's thing inline in the code
#pragma intrinsic( fabs )
// Also, alias float make positive to use fabs, too
// NOTE:  Is there a perf issue with double<->float conversion?
inline float FloatMakePositive(float f)
{
	return (float)fabs(f);
}


//-----------------------------------------------------------------------------
// Quaternion equality with tolerance
//-----------------------------------------------------------------------------
inline bool QuaternionsAreEqual(const Quaternion& src1, const Quaternion& src2, float tolerance)
{
	if (FloatMakePositive(src1.x - src2.x) > tolerance)
		return false;
	if (FloatMakePositive(src1.y - src2.y) > tolerance)
		return false;
	if (FloatMakePositive(src1.z - src2.z) > tolerance)
		return false;
	return (FloatMakePositive(src1.w - src2.w) <= tolerance);
}

template <class T>
__forceinline T Lerp(float flPercent, T const &A, T const &B)
{
	return A + (B - A) * flPercent;
}

void QuaternionAngles(const Quaternion &q, QAngle &angles);

template<> __forceinline QAngle Lerp<QAngle>(float flPercent, const QAngle& q1, const QAngle& q2)
{
	// Avoid precision errors
	if (q1 == q2)
		return q1;

	Quaternion src, dest;

	// Convert to quaternions
	AngleQuaternion(q1, src);
	AngleQuaternion(q2, dest);

	Quaternion result;

	// Slerp
	QuaternionSlerp(src, dest, flPercent, result);

	// Convert to euler
	QAngle output;
	QuaternionAngles(result, output);
	return output;
}

inline RadianEuler::RadianEuler(QAngle const &angles)
{
	Init(
		angles.z * 3.14159265358979323846f / 180.f,
		angles.x * 3.14159265358979323846f / 180.f,
		angles.y * 3.14159265358979323846f / 180.f);
}

inline QAngle RadianEuler::ToQAngle(void) const
{
	return QAngle(
		y * 180.f / 3.14159265358979323846f,
		z * 180.f / 3.14159265358979323846f,
		x * 180.f / 3.14159265358979323846f);
}

inline QAngle CalcAngleNew(Vector src, Vector dst)
{
	QAngle angles;
	Vector delta = src - dst;
	angles.x = (asinf(delta.z / delta.Length()) * M_RADPI);
	angles.y = (atanf(delta.y / delta.x) * M_RADPI);
	angles.z = 0.0f;
	if (delta.x >= 0.0) { angles.y += 180.0f; }

	return angles;
}

//Source: http://oldschooldotnet.blogspot.com/2011/04/calculating-average-of-two-angles-two.html
inline float GetAverageBearing(float bearingA, float bearingB)
{
	if (bearingA > bearingB)
	{

		float temp = bearingA;
		bearingA = bearingB;

		bearingB = temp;
	}

	if (bearingB - bearingA > 180.0f) bearingB -= 360.0f;

	float finalBearing = (bearingB + bearingA) / 2;

	if (finalBearing < 0.0f) finalBearing += 360.0f;

	return finalBearing;
}

inline void NormalizeAngles(QAngle& angles)
{
	NormalizeFloat(angles.x);
	NormalizeFloat(angles.y);
	NormalizeFloat(angles.z);
}

inline QAngle NormalizeAngles_r(QAngle angles)
{
	NormalizeFloat(angles.x);
	NormalizeFloat(angles.y);
	NormalizeFloat(angles.z);
	return angles;
}

inline void NormalizeAngle(float& angle)
{
	NormalizeFloat(angle);
}
#if 0
void NormalizeAngle(QAngle& Angle)
{
	while (Angle.x <= -D3DX_PI)
		Angle.x += 2 * D3DX_PI;
	while (Angle.x > D3DX_PI)
		Angle.x -= 2 * D3DX_PI;
	while (Angle.y <= -D3DX_PI)
		Angle.y += 2 * D3DX_PI;
	while (Angle.y > D3DX_PI)
		Angle.y -= 2 * D3DX_PI;
}
#endif

inline void NormalizeAnglesNoClamp(QAngle & angle)
{
	NormalizeFloat(angle.x);
	NormalizeFloat(angle.y);
	NormalizeFloat(angle.z);
}

inline void ClampXYZLegacy(QAngle &angle)
{
	angle.x = fmodf(angle.x, 360.0f);
	angle.y = fmodf(angle.x, 360.0f);
	angle.z = 0.0f;

	clampfloat(angle.x, -89.0f, 89.0f);
}

void ConcatTransforms(const matrix3x4_t& in1, const matrix3x4_t& in2, matrix3x4_t& out);

void VectorRotate(const float *in1, const matrix3x4_t & in2, float *out);
void VectorRotate(const Vector &in1, const QAngle &in2, Vector &out);
Vector VectorRotateR(const Vector &in1, const QAngle &in2);
void VectorRotate(const Vector &in1, const Quaternion &in2, Vector &out);
void VectorIRotate(const float *in1, const matrix3x4_t & in2, float *out);

inline void VectorRotate(const Vector& in1, const matrix3x4_t &in2, Vector &out)
{
	VectorRotate(&in1.x, in2, &out.x);
}

inline void VectorIRotate(const Vector& in1, const matrix3x4_t &in2, Vector &out)
{
	VectorIRotate(&in1.x, in2, &out.x);
}

extern void VectorMA(const Vector& start, float scale, const Vector& direction, Vector& dest);

inline void VectorMA(const QAngle &start, float scale, const QAngle &direction, QAngle &dest)
{
	CHECK_VALID(start);
	CHECK_VALID(direction);
	dest.x = start.x + scale * direction.x;
	dest.y = start.y + scale * direction.y;
	dest.z = start.z + scale * direction.z;
}


enum MoveCollide_t
{
	MOVECOLLIDE_DEFAULT = 0,

	// These ones only work for MOVETYPE_FLY + MOVETYPE_FLYGRAVITY
	MOVECOLLIDE_FLY_BOUNCE,	// bounces, reflects, based on elasticity of surface and object - applies friction (adjust velocity)
	MOVECOLLIDE_FLY_CUSTOM,	// Touch() will modify the velocity however it likes
	MOVECOLLIDE_FLY_SLIDE,  // slides along surfaces (no bounce) - applies friciton (adjusts velocity)

	MOVECOLLIDE_COUNT,		// Number of different movecollides

							// When adding new movecollide types, make sure this is correct
							MOVECOLLIDE_MAX_BITS = 3
};

#define MAXSTUDIOBONECTRLS 4

inline float AngleNormalize(float angle)
{
	angle = fmodf(angle, 360.0f);
	if (angle > 180)
	{
		angle -= 360;
	}
	if (angle < -180)
	{
		angle += 360;
	}
	return angle;
}

//--------------------------------------------------------------------------------------------------------------
// ensure that 0 <= angle <= 360
inline float AngleNormalizePositive(float angle)
{
	angle = fmodf(angle, 360.0f);

	if (angle < 0.0f)
	{
		angle += 360.0f;
	}

	return angle;
}

inline float anglemod(float a)
{
	a = (360.f / 65536) * ((int)(a*(65536.f / 360.0f)) & 65535);
	return a;
}

// BUGBUG: Why doesn't this call angle diff?!?!?
extern float ApproachAngle(float target, float value, float speed);

extern float Approach(float target, float value, float speed);

extern float AngleDiff(float destAngle, float srcAngle);

extern float Bias(float x, float biasAmt);

// Each mod defines these for itself.
class CViewVectors
{
public:
	CViewVectors() {}

	CViewVectors(
		Vector vView,
		Vector vHullMin,
		Vector vHullMax,
		Vector vDuckHullMin,
		Vector vDuckHullMax,
		Vector vDuckView,
		Vector vObsHullMin,
		Vector vObsHullMax,
		Vector vDeadViewHeight)
	{
		m_vView = vView;
		m_vHullMin = vHullMin;
		m_vHullMax = vHullMax;
		m_vDuckHullMin = vDuckHullMin;
		m_vDuckHullMax = vDuckHullMax;
		m_vDuckView = vDuckView;
		m_vObsHullMin = vObsHullMin;
		m_vObsHullMax = vObsHullMax;
		m_vDeadViewHeight = vDeadViewHeight;
	}

	// Height above entity position where the viewer's eye is.
	Vector m_vView; //0

	Vector m_vHullMin; //12
	Vector m_vHullMax; //24

	Vector m_vDuckHullMin; //36
	Vector m_vDuckHullMax; //48
	Vector m_vDuckView; //60

	Vector m_vObsHullMin; //72
	Vector m_vObsHullMax; //84

	Vector m_vDeadViewHeight; //96
};

//-----------------------------------------------------------------------------
// Allows us to specifically pass the vector by value when we need to
//-----------------------------------------------------------------------------
class VectorByValue : public Vector
{
public:
	// Construction/destruction:
	VectorByValue(void) : Vector() {}
	VectorByValue(float X, float Y, float Z) : Vector(X, Y, Z) {}
	VectorByValue(const VectorByValue& vOther) { *this = vOther; }
};

void AddPointToBounds(const Vector& v, Vector& mins, Vector& maxs);

class __declspec(align(16)) VectorAligned : public Vector
{
public:
	VectorAligned() {}

	VectorAligned(const Vector &vec)
	{
		this->x = vec.x;
		this->y = vec.y;
		this->z = vec.z;
	}

	float w;
};

void VectorCopy(RadianEuler const& src, RadianEuler &dst);

//-----------------------------------------------------------------------------
// Copy
//-----------------------------------------------------------------------------
inline void VectorCopy(const QAngle& src, QAngle& dst)
{
	CHECK_VALID(src);
	dst.x = src.x;
	dst.y = src.y;
	dst.z = src.z;
}

FORCEINLINE float DotProduct(const float *v1, const float *v2)
{
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}
FORCEINLINE void VectorSubtract(const float *a, const float *b, float *c)
{
	c[0] = a[0] - b[0];
	c[1] = a[1] - b[1];
	c[2] = a[2] - b[2];
}
FORCEINLINE void VectorAdd(const float *a, const float *b, float *c)
{
	c[0] = a[0] + b[0];
	c[1] = a[1] + b[1];
	c[2] = a[2] + b[2];
}
FORCEINLINE void VectorCopy(const float *a, float *b)
{
	b[0] = a[0];
	b[1] = a[1];
	b[2] = a[2];
}
FORCEINLINE void VectorClear(float *a)
{
	a[0] = a[1] = a[2] = 0;
}

FORCEINLINE float VectorMaximum(const float *v)
{
	return max(v[0], max(v[1], v[2]));
}

FORCEINLINE float VectorMaximum(const Vector& v)
{
	return max(v.x, max(v.y, v.z));
}

FORCEINLINE void VectorScale(const float* in, float scale, float* out)
{
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
}


// Cannot be forceinline as they have overloads:
inline void VectorFill(float *a, float b)
{
	a[0] = a[1] = a[2] = b;
}

inline void VectorNegate(float *a)
{
	a[0] = -a[0];
	a[1] = -a[1];
	a[2] = -a[2];
}

struct cplane_t
{
	Vector	normal;
	float	dist;
	unsigned char	type;
	unsigned char	signbits;
	unsigned char	pad[2];
}; //20 bytes

   //-----------------------------------------------------------------------------
   // Transform a plane
   //-----------------------------------------------------------------------------
void MatrixTransformPlane(const matrix3x4_t &src, const cplane_t &inPlane, cplane_t &outPlane);

void MatrixITransformPlane(const matrix3x4_t &src, const cplane_t &inPlane, cplane_t &outPlane);


inline float DotProductAbs(const Vector &v0, const Vector &v1)
{
	return FloatMakePositive(v0.x*v1.x) + FloatMakePositive(v0.y*v1.y) + FloatMakePositive(v0.z*v1.z);
}

inline float DotProductAbs(const Vector &v0, const float *v1)
{
	return FloatMakePositive(v0.x * v1[0]) + FloatMakePositive(v0.y * v1[1]) + FloatMakePositive(v0.z * v1[2]);
}

//-----------------------------------------------------------------------------
// Rotates a AABB into another space; which will inherently grow the box. 
// (same as TransformAABB, but doesn't take the translation into account)
//-----------------------------------------------------------------------------
void RotateAABB(const matrix3x4_t &transform, const Vector &vecMinsIn, const Vector &vecMaxsIn, Vector &vecMinsOut, Vector &vecMaxsOut);

//-----------------------------------------------------------------------------
// Transforms a AABB into another space; which will inherently grow the box.
//-----------------------------------------------------------------------------
void TransformAABB(const matrix3x4_t &in1, const Vector &vecMinsIn, const Vector &vecMaxsIn, Vector &vecMinsOut, Vector &vecMaxsOut);

void QuaternionBlend(const Quaternion &p, const Quaternion &q, float t, Quaternion &qt);
void QuaternionBlendNoAlign(const Quaternion &p, const Quaternion &q, float t, Quaternion &qt);

//from raxer
__forceinline float interpolate(const float from, const float to, const float percent)
{
	return to * percent + from * (1.f - percent);
}

__forceinline Vector interpolate(const Vector& from, const Vector& to, const float percent)
{
	return to * percent + from * (1.f - percent);
}

__forceinline float looping_interpolate(float from, float to, const float percent)
{
	if (fabs(to - from) >= .5f)
	{
		if (from < to)
			from += 1.f;
		else
			to += 1.f;
	}

	auto s = interpolate(from, to, percent);

	s = s - (int32_t)(s);
	if (s < .0f)
		s = s + 1.f;

	return s;
}

#define _CS_PLAYER_SPEED_DUCK_MODIFIER 0.340f
#define _CS_PLAYER_SPEED_WALK_MODIFIER 0.520f
#define _CS_PLAYER_SPEED_RUN 260.f
#define _CS_PLAYER_SPEED_STOPPED 1.f
#define _CS_PLAYER_SPEED_OBSERVER 900.f
#define _CS_PLAYER_HEAVYARMOR_FLINCH_MODIFIER 0.5f
#define _CS_PLAYER_SPEED_HAS_HOSTAGE 200.f

#define _CS_PLAYER_MAXSPEED_MODIFIER (1.0f / 250.0f) //0.004

#define MIN_ROUTABLE_PAYLOAD		16

// NOTE:  Bits 5, 6, and 7 are used to specify the # of padding bits at the end of the packet!!!
#define ENCODE_PAD_BITS( x ) ( ( x << 5 ) & 0xff )
#define DECODE_PAD_BITS( x ) ( ( x >> 5 ) & 0xff )

// each channel packet has 1 byte of FLAG bits
#define PACKET_FLAG_RELIABLE			(1<<0)
#define PACKET_FLAG_COMPRESSED			(1<<1)
#define PACKET_FLAG_ENCRYPTED			(1<<2)
#define PACKET_FLAG_SPLIT				(1<<3)
#define PACKET_FLAG_CHOKED				(1<<4)

#define PAD_NUMBER(number, boundary) ( ((number) + ((boundary)-1)) / (boundary) ) * (boundary)

#define NET_MAX_PAYLOAD 524284