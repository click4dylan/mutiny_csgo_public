#pragma once

#include <vector>
#include <map>
#include <Windows.h>

//#include "ICvar.h"
//#include "Interfaces.h"
//#include "ISurface.h"
//#include "IVModelInfo.h"
//#include "IVModelRender.h"
//#include "IMaterialSystem.h"
//#include "IMaterial.h"
//#include "IRenderView.h"
//#include "IVDebugOverlay.h"
//#include "IStudioRender.h"

//SDK
#include "IBaseClientDLL.h"
#include "CClientEntityList.h"
#include "IClientModeShared.h"
#include "IEngineClient.h"
#include "IEngineTrace.h"
#include "IGlobalVarsBase.h"
#include "IInputSystem.h"
#include "IVPanel.h"
#include "IPrediction.h"
#include "VPhysics.h"
#include "IGameMovement.h"
#include "IModelInfoClient.h"
#include "ICvar.h"
#include "TE_FireBullets.h"
#include "ivdebugoverlay.h"
#include "IVRenderView.h"
#include "CMaterialSystem.h"
#include "IVModelRender.h"
#include "IInput.h"
#include "IViewRenderBeams.h"
#include "C_CSGameRules.h"

//Others
#include "SDK/UserCmd.h"
#include "SDK/Defines.h"
#include "SDK/Structs.h"
#include "SDK/StudioHitbox.h"

#define TICKS_TO_TIME(t) (float(t) * Interfaces::GlobalVars->interval_per_tick)
#define TIME_TO_TICKS(dt) int( 0.5f + float(dt) / Interfaces::GlobalVars->interval_per_tick )

class IGameEventManager2;
class IMDLCache;
class CMaterialSystem;
class IVModelRender;
class CGameMovement;
class CMoveHelperClient;
class IGameEvent;
class IViewRenderBeams;
class C_TEMuzzleFlash;

class CMenu;

extern CMenu* g_Menu;

namespace Interfaces
{
	//extern IVModelRender*			ModelRender;
	//extern IVModelInfo*			ModelInfo;
	//extern IMaterialSystem*		MaterialSystem;
	//extern IMaterial*				Material;
	//extern IStudioRender*			StudioRender;

	extern IBaseClientDll*			BaseClient;
	extern IClientModeShared*		ClientMode;
	extern CClientEntityList*		ClientEntityList;
	extern ICVar*					Cvar;
	extern IInputSystem*			InputSystem;
	extern IEngineClient*			EngineClient;
	extern IEngineTrace*			EngineTrace;
	extern IGlobalVarsBase*			GlobalVars;
	extern IVPanel*					VPanel;
	extern IVRenderView*			RenderView;
	extern CPrediction*				Prediction;
	extern IPhysicsSurfaceProps*	Physprops;
	extern IVModelRender*			ModelRender;
	extern CGameMovement*			GameMovement;
	extern IVModelInfoClient*		ModelInfoClient;
	extern IVDebugOverlay*			DebugOverlay;
	extern CMaterialSystem*			MaterialSystem;
	extern IInput*					Input;
	extern IGameEventManager2*		GameEventManager;
	extern C_TEFireBullets*			TE_FireBullets;
	extern ISurface*				Surface;
	extern C_TEEffectDispatch*		TE_EffectDispatch;
	extern IMDLCache*				MDLCache;
	extern void*					IEngineSoundClient;
	extern CMoveHelperClient		**MoveHelperClient;
	extern IViewRenderBeams*		Beams;
	extern C_TEMuzzleFlash*			TE_MuzzleFlash;
	extern C_CSGameRules			**GameRules;
}

namespace Global
{

}