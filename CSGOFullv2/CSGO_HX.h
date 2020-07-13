// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the CSGO_HX_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// CSGO_HX_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifndef CSGO_HX
#define CSGO_H
#pragma once

#ifdef CSGO_HX_EXPORTS
#define DLLEXP __declspec(dllexport)
#else
#define DLLEXP __declspec(dllimport)
#endif

#include <Windows.h>
#include "misc.h"
#include <string>
#include "BaseEntity.h"
#include "ErrorCodes.h"
#include "Profiler.h"
#include <mutex>
#if _HAS_CXX17
#include <algorithm>
#endif

#define NETVARPROXY(funcname) void funcname (CRecvProxyData *pData, void *pStruct, void *pOut)

#define MAX_PLAYERS 64

#if _HAS_CXX17
using std::clamp;
#else
#define clamp(val, min, max) (((val) > (max)) ? (max) : (((val) < (min)) ? (min) : (val)))
#endif

typedef unsigned char uint8_t;
extern Vector vecZero;
extern QAngle angZero;

#define AIMBOT_KEY 0x43
#define TRIGGERBOT_KEY VK_XBUTTON2

/* Glow Object structure in csgo */
/*
struct glow_t
{
	DWORD dwBase;
	float r;
	float g;
	float b;
	float a;
	uint8_t unk1[16];
	bool m_bRenderWhenOccluded;
	bool m_bRenderWhenUnoccluded;
	bool m_bFullBloom;
	uint8_t unk2[10];
};
*/

extern RecvVarProxyFn OriginalSmokeProxy;
extern NETVARPROXY(SmokeProxy);

void InitTime();
HANDLE FindHandle(std::string name);

extern bool CrashingServer;
extern bool GotCSGORect;
extern std::mutex GotCSGORectMutex;
double QPCTime();
void GetCSGORect();

extern float Time;
extern Vector CenterOfScreen;
extern bool AllocedConsole;
void AllocateConsole();

extern DWORD WINAPI CheatInit(LPVOID lParam);

#define THINK_SPEED (1.0f / 60.0f)

#endif