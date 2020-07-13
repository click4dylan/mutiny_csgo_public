#include "precompiled.h"
#include "Overlay.h"
#include "CSGO_HX.h"
#include "GameMemory.h"
#ifdef LICENSED
#include "Licensing2.h"
#endif
#include "CreateMove.h"
#include "Reporting.h" //For competitive match id
#include "SetClanTag.h"
#define PRIu64       "llu" //#include <inttypes.h> //For competitive match id 
#include <ctype.h>
#include "EncryptString.h"
#include "VTHook.h"
#include "ConVar.h"
#include "HitboxDefines.h"
#include "Targetting.h"
#include "Draw.h"

HINSTANCE hInstance; //Set by dllmain.cpp