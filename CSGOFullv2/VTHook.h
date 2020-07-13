//Thanks to https://github.com/A5-/Gamerfood_CSGO
#ifndef VTHOOK_H
#define VTHOOK_H
#pragma once
#include <Windows.h>
#include <atomic>
#include "GameMemory.h"
#include "CViewSetup.h"
#include "IEngineTrace.h"
#include "C_TEMuzzleflash.h"
#include "TE_FireBullets.h"
#include "TE_EffectDispatch.h"
#include "imdlcache.h"
#include "IClientEntityList.h"
#include "shared_mem.h"
#include "CClientState.h"
#include "IVRenderView.h"
#include <intrin.h>
#include "string_encrypt_include.h"

struct DrawModelInfo_t
{
	studiohdr_t* m_StudioHdr;
	studiohwdata_t* m_pHardwareData;
	unsigned short m_Decals;
	int m_Skin;
	int m_Body;
	int m_HitboxSet;
	CBaseEntity* m_pClientEntity;
	int m_Lod;
	void* m_pColorMeshes;
	bool m_bStaticLighting;
	BYTE m_LightingState[440];
};

inline DWORD* GetVT(PDWORD* baseclass)
{
	return (DWORD*)ReadInt((DWORD)baseclass);
}

inline DWORD* GetVT(DWORD ptobaseclass)
{
	return (DWORD*)ReadInt(ReadInt(ptobaseclass));
}

//DWORD HookGameFunc(DWORD** ppClassBase, int IndexToHook, DWORD ReplacementFunction);
extern void InitKeyValues(KeyValues* pKeyValues, const char* name);
extern void LoadFromBuffer(KeyValues* pKeyValues, const char* resourceName, const char* pBuffer, void* pFileSystem = nullptr, const char* pPathID = nullptr, void* pfnEvaluateSymbolProc = nullptr);
class VTHook
{
public:
	VTHook()
	{
		memset(this, 0, sizeof(VTHook));
		m_SharedMemoryIndex = -1;
	}

	VTHook(PDWORD* ppdwClassBase, hook_types hooktype = hook_types::_UNSET)
	{
		m_HookType = hooktype;
		bInitialize(ppdwClassBase);
	}

	~VTHook()
	{
		if (m_ClassBase)
		{
			//*m_ClassBase = m_OldVT;
			WriteInt(ReadInt((DWORD)m_ClassBase), (int)m_OldVT);
		}

		if (m_SharedMemoryIndex != -1)
		{
			if (m_ClassBase && instance->instance != (PVOID)m_ClassBase)
			{
#ifdef _DEBUG
				//decrypts(0)
				MessageBoxA(NULL, XorStr("MVI_Error3"), "", MB_OK);
				//encrypts(0)
#endif
			}
			m_SharedMemoryIndex			   = -1;

			if (m_NewVT)
			{
				delete m_NewVT;
				m_NewVT = NULL;
			}
		}
	}

	int FindIndexFromAddress(DWORD adr)
	{
		for (DWORD i = 0; i < GetFunctionCount(); i++)
		{
			if (*(DWORD*)((DWORD)GetOldVT() + (sizeof(DWORD*) * i)) == adr)
			{
				return i;
			}
		}
		return -1;
	}

	PDWORD* GetClassBase() const
	{
		return m_ClassBase;
	}

	void ClearClassBase()
	{
		m_ClassBase = NULL;
	}

	PDWORD GetOldVT() const
	{
		return m_OldVT;
	}

	DWORD GetFunctionCount() const
	{
		return m_VTSize;
	}

	PDWORD GetNewVT()
	{
		return (PDWORD)((DWORD)m_NewVT + 4);
	}

	PDWORD GetCurrentVT()
	{
		return (PDWORD) * (DWORD*)((DWORD)m_ClassBase);
	}

	void /*bool*/ bInitialize(PDWORD* ppdwClassBase)
	{
#if 1
		m_ClassBase = ppdwClassBase; // Get Pointer to the class base and store in member variable of current object
			//m_OldVT = *ppdwClassBase; // Get Pointer the the Old Virtual Address Table
		m_OldVT  = (PDWORD)ReadInt((DWORD)ppdwClassBase);
		m_VTSize = GetVTCount(m_OldVT); //*ppdwClassBase); // Get the number of functions in the Virtual Address Table

		m_NewVT = new DWORD[m_VTSize + 1]; // Create A new virtual Address Table
			//memcpy(m_NewVT, m_OldVT, sizeof(DWORD) * m_VTSize);

		WriteByteArray((char*)((DWORD)m_NewVT + 4), (char*)m_OldVT, sizeof(DWORD) * m_VTSize); //Use driver to write instead of hack
			//*ppdwClassBase = m_NewVT; // replace the old virtual address table pointer with a pointer to the newly created virtual address table above
		m_NewVT[0] = (DWORD)ReadInt((DWORD)m_OldVT - 4);


		WriteInt(ReadInt((DWORD)&ppdwClassBase), (int)((DWORD)m_NewVT + 4));
		//return true;
#else
		m_ClassBase = ppdwClassBase; // Get Pointer to the class base and store in member variable of current object
			//m_OldVT = *ppdwClassBase; // Get Pointer the the Old Virtual Address Table
		m_OldVT  = (PDWORD)ReadInt((DWORD)ppdwClassBase);
		m_VTSize = GetVTCount(m_OldVT); //*ppdwClassBase); // Get the number of functions in the Virtual Address Table
		m_VTSize++; //for rtti
		m_NewVT = new DWORD[m_VTSize]; // Create A new virtual Address Table
			//memcpy(m_NewVT, m_OldVT, sizeof(DWORD) * m_VTSize);
		WriteByteArray((char*)((DWORD)m_NewVT + sizeof(DWORD)), (char*)m_OldVT, sizeof(DWORD) * (m_VTSize - 1)); //Use driver to write instead of hack
		m_NewVT[0] = ReadInt((DWORD)m_OldVT - sizeof(DWORD));
		//*ppdwClassBase = m_NewVT; // replace the old virtual address table pointer with a pointer to the newly created virtual address table above
		WriteInt(ReadInt((DWORD)&ppdwClassBase), (DWORD)m_NewVT + sizeof(DWORD));
		//return true;
#endif
	}
	void /*bool*/ bInitialize(PDWORD** pppdwClassBase) // fix for pp
	{
		//return bInitialize(*pppdwClassBase);
		bInitialize(*pppdwClassBase);
	}

	/*
	void ReHook()
	{
	if (m_ClassBase)
	{
	*m_ClassBase = m_NewVT;
	}
	}
	*/
	/*
	int iGetFuncCount()
	{
	return (int)m_VTSize;
	}
	*/
	/*
	DWORD GetFuncAddress(int Index)
	{
	if (Index >= 0 && Index <= (int)m_VTSize && m_OldVT != NULL)
	{
	return m_OldVT[Index];
	}
	return NULL;
	}
	*/

	/*
	PDWORD GetOldVT()
	{
	return m_OldVT;
	}
	*/

	DWORD HookFunction(DWORD dwNewFunc, unsigned int iIndex)
	{
		// Noel: Check if both the New Virtual Address Table and the the Old one have been allocated and exist,
		// also check if the function index into the New Virtual Address Table is within the bounds of the table
		if (m_NewVT && m_OldVT && iIndex <= m_VTSize && iIndex >= 0)
		{
			m_NewVT[iIndex + 1] = dwNewFunc; // Set the new function to be called in the new virtual address table
			return ReadInt((DWORD)m_OldVT + (sizeof(DWORD*) * iIndex));
			//return m_OldVT[iIndex]; // Return the old function to be called in the virtual address table
		}
		// Return null and fail if the new Virtual Address Table or the old one hasn't been allocated with = new,
		// or if the index chosen is outside the bounds of the table.
		return NULL;
	}

private:
	//Use standard C++ functions instead
	BOOL CodePointerIsInvalid(DWORD* testptr)
	{
#if 0
		bool Invalid = false;
		int test;
		try {
			test = ReadInt((DWORD)testptr);
}
		catch (...) {
			Invalid = true;
		}
		return Invalid;
#else

		return IsBadCodePtr((FARPROC)testptr);
#endif
	}
	DWORD GetVTCount(PDWORD pdwVMT)
	{
		DWORD dwIndex = NULL;

		for (dwIndex = 0; pdwVMT[dwIndex]; dwIndex++)
			if (!pdwVMT[dwIndex])
				break;

		/*
		for (dwIndex = 0; pdwVMT[dwIndex] != 0; dwIndex++)
		{
		//if (ShitPointer((DWORD*)pdwVMT[dwIndex]))
		//break;

		if (CodePointerIsInvalid((DWORD*)ReadInt((DWORD)pdwVMT + (sizeof(DWORD*) * dwIndex))))
		break;

		//Original code, uses windows api function

		//if (IsBadCodePtr((FARPROC)pdwVMT[dwIndex]))
		//{
		//break;
		//}
		}
		*/
		return dwIndex;
	}
	PDWORD* m_ClassBase;
	PDWORD m_NewVT, m_OldVT;
	DWORD m_VTSize;
	int m_SharedMemoryIndex;
	hook_types m_HookType;
};

extern VTHook* VPanel;
extern VTHook* ClientMode;
extern VTHook* Client;
extern VTHook* Trace;
extern VTHook* Prediction;
extern VTHook* Engine;
extern VTHook* TE_EffectDispatch;
extern VTHook* TE_MuzzleFlash;
extern VTHook* TE_FireBullets;
extern VTHook* HRenderView;
extern VTHook* EngineClientSound;
extern VTHook* DirectX;
extern VTHook* DemoPlayer;
extern VTHook* GameRules;
extern VTHook* HNetchan;
extern VTHook* HSurface;
extern VTHook* ClientEntityList;
extern VTHook* HClientState;
extern VTHook* StudioRenderHk;
extern VTHook* HTempEnts;
extern VTHook* HBeams;
extern VTHook* HMessageHandler;
extern VTHook* HGameEventListener;
extern VTHook* HLevelShutdownPreEntity;
extern VTHook* ModelRenderMo;
extern VTHook* ClientLeaf;
extern VTHook* BSPQuery;

void SetupVMTHooks();
extern float (*RandomFloat)(float from, float to);
extern int(*RandomInt)(int from, int to);
extern void (*RandomSeed)(unsigned int seed);
extern bool (*ThreadInMainThread)(void);
extern DWORD gpMemAllocVTable;
extern DWORD gpKeyValuesVTable;

CUserCmd* GetUserCmd(int slot, int sequence_number, bool DisableValidation = false); //MOVE ME
CUserCmd* GetUserCmdStruct(int slot); //MOVE ME
class CMoveData;

template < typename T >
T GetVFunc(void* pTable, int Index)
{
	auto table = *reinterpret_cast< uint32_t** >(pTable);

	if (IsBadCodePtr((FARPROC)table) != 0)
		return 0;

	return reinterpret_cast< T >(table[Index]);
}

enum EGCResults;

using OverrideViewFN = void(__thiscall*)(void* ecx, CViewSetup*);
extern OverrideViewFN oOverrideView;
using CreateMoveFn = bool(__thiscall*)(void* ecx, float, CUserCmd*);
extern CreateMoveFn oCreateMove; //bool(__stdcall *oCreateMove)(float, CUserCmd*);
using ViewRenderFn = void(__fastcall*)(void*, void*, vrect_t*);
extern ViewRenderFn oViewRender;
using FrameStageNotifyFn = void(__thiscall*)(void*, ClientFrameStage_t);
extern FrameStageNotifyFn oFrameStageNotify;
using WriteUsercmdDeltaToBufferFn = bool(__thiscall*)(void*, int, void*, int, int, bool);
extern WriteUsercmdDeltaToBufferFn oWriteUsercmdDeltaToBuffer;
typedef void(__thiscall* DrawModelExecuteFn)(void*, void*, void*, const ModelRenderInfo_t&, matrix3x4_t*);
extern DrawModelExecuteFn oDrawModelExecute;
using AllowThirdPersonFn = BOOLEAN(__thiscall*)(void*);
extern AllowThirdPersonFn oAllowThirdPerson;
using CAM_ToFirstPersonFn = void(__thiscall*)(void*);
extern CAM_ToFirstPersonFn oCAM_ToFirstPerson;
using IsHLTVFn = bool(__thiscall*)(void*);
extern IsHLTVFn oIsHLTV;
using PlayerFallingDamageFn = void(__thiscall*)(void* ecx);
extern PlayerFallingDamageFn oPlayerFallingDamage;
using ProcessMovementFn = void (__thiscall*)(void* pGameMovement, CBaseEntity* basePlayer, CMoveData* moveData);
extern ProcessMovementFn oProcessMovement;
using GetVCollideFn = vcollide_t*(__thiscall*)(void*, int);
extern GetVCollideFn oGetVCollide;
using SendMessageGameCoordinatorFn = EGCResults(__thiscall*) (void* ecx, uint32_t unMsgType, const void *pubData, uint32_t cubData);
extern SendMessageGameCoordinatorFn oSendMessageSteamGameCoordinator;
using RetrieveMessageGameCoordinatorFn = EGCResults(__thiscall*) (void* ecx, uint32_t *punMsgType, void *pubDest, uint32_t cubDest, uint32_t *pcubMsgSize);
extern RetrieveMessageGameCoordinatorFn oRetrieveMessageSteamGameCoordinator;
using SetReservationCookieFn = void(__thiscall*)(void*, unsigned long long);
extern SetReservationCookieFn oSetReservationCookie;
extern unsigned long long g_ServerReservationCookie;

using SceneEndFn = void(__fastcall*)(IVRenderView*, void*);
extern SceneEndFn oSceneEnd;

using TraceRayFn = void(__thiscall*)(void*, Ray_t const&, unsigned int, CTraceFilter*, trace_t&);
extern TraceRayFn oTraceRay;
using RunCommandFn = void(__thiscall*)(void*, CBaseEntity*, CUserCmd*, void*);
extern RunCommandFn oRunCommand;
using GetLastTimeStampFn = float(__thiscall*)(void);
extern GetLastTimeStampFn oGetLastTimeStamp;
using TE_FireBullets_PostDataUpdateFn = void(__thiscall*)(C_TEFireBullets*, DataUpdateType_t);
extern TE_FireBullets_PostDataUpdateFn oTE_FireBullets_PostDataUpdate;
using TE_EffectDispatch_PostDataUpdateFn = void(__thiscall*)(C_TEEffectDispatch*, DataUpdateType_t);
extern TE_EffectDispatch_PostDataUpdateFn oTE_EffectDispatch_PostDataUpdate;
using TE_MuzzleFlash_PostDataUpdateFn = void(__thiscall*)(C_TEMuzzleFlash*, DataUpdateType_t);
extern TE_MuzzleFlash_PostDataUpdateFn oTE_MuzzleFlash_PostDataUpdate;
using LookupPoseParameterFn = int(__stdcall*)(CStudioHdr* me, const char* str);
extern LookupPoseParameterFn LookupPoseParameterGame;
using GetPoseParameterRangeFn = bool(__thiscall*)(CBaseEntity* me, int index, float& minValue, float& maxValue);
extern GetPoseParameterRangeFn GetPoseParameterRangeGame;
using SetPoseParameterFn = float(__thiscall*)(CBaseEntity* me, /*CStudioHdr*/ void* pStudioHdr, int iParameter, float flValue);
extern SetPoseParameterFn SetPoseParameterGame;
using IN_KeyEventFn = int(__fastcall*)(void*, void*, int, ButtonCode_t, const char*);
extern IN_KeyEventFn oIN_KeyEvent;
using GetViewModelFOVFn = float(__stdcall*)();
extern GetViewModelFOVFn oGetViewModelFOV;
using UpdateClientSideAnimationFn = void(__thiscall*)(CBaseEntity* me);
extern UpdateClientSideAnimationFn oUpdateClientSideAnimation;
using ResetAnimationStateFn = void(__thiscall*)(C_CSGOPlayerAnimState*);
extern ResetAnimationStateFn oResetAnimState;
using LockStudioHdrFn = void(__thiscall*)(CBaseEntity*);
extern LockStudioHdrFn oLockStudioHdr;
using GetFirstSequenceAnimTagFn = void(__thiscall*)(CBaseEntity*, int, int, int*);
extern GetFirstSequenceAnimTagFn oGetFirstSequenceAnimTag;
using SurpressLadderChecksFn = void(__thiscall*)(CBaseEntity*, Vector*, Vector*);
extern SurpressLadderChecksFn oSurpressLadderChecks;
using SetPunchVMTFn = void(__thiscall*)(CBaseEntity*, QAngle&);
extern SetPunchVMTFn oSetPunchVMT;
using IsInAVehicleFn = bool(__thiscall*)(CBaseEntity*);
extern IsInAVehicleFn oIsInAVehicle;
using IsCarryingHostageFn = bool(__thiscall*)(CBaseEntity*, int unknown);
extern IsCarryingHostageFn oIsCarryingHostage;
using EyeVectorsFn = void(__thiscall*)(CBaseEntity*, Vector*, Vector*, Vector*);
extern EyeVectorsFn oEyeVectors;
using CreateStuckTableFn = void (*)(void);
extern CreateStuckTableFn oCreateStuckTable;
extern Vector** rgv3tStuckTable;
using VoiceStatusFn = void(__thiscall*)(void*, int, int, bool);
extern VoiceStatusFn oVoiceStatus;
class CGameEvent;
using FireGameEventFn = void(__thiscall*)(void*, CGameEvent*, bool);
extern FireGameEventFn oFireEvent;
#if 0
using SetViewAnglesFn = void(__stdcall *) (QAngle &va);
extern SetViewAnglesFn oSetViewAngles;
#endif
using GetNetChannelInfoFn = INetChannelInfo*(__thiscall*)(void*);
extern GetNetChannelInfoFn oGetNetChannelInfo;
extern DWORD GetNetChannelInfo_cvarcheck_retaddr;
;
using IsPausedFn = bool(__stdcall*)();
extern IsPausedFn oIsPaused;
using IsPlayingBackFn = bool(__thiscall*)(DWORD demoplayerptr);
extern IsPlayingBackFn oIsDemoPlayingBack;
using SendDatagramFn = int(__thiscall*)(void*, void*);
extern SendDatagramFn oSendDatagram;
using CanPacketFn = bool(__thiscall*)(void*);
extern CanPacketFn oCanPacket;
using SendNetMsgFn = bool(__thiscall*)(void*, void*, bool, bool);
extern SendNetMsgFn oSendNetMsg;
using EmitSoundFn = void(__fastcall*)(void* ecx, void* edx, void* filter, int iEntIndex, int iChannel, const char* pSoundEntry, unsigned int nSoundEntryHash, const char* pSample, float flVolume, float flAttenuation, int nSeed, int iFlags, int iPitch, const Vector* pOrigin, const Vector* pDirection, Vector* pUtlVecOrigins, bool bUpdatePositions, float soundtime, int speakerentity, int unknown);
extern EmitSoundFn oEmitSound;
using PlaySoundFn = void(__thiscall*)(void*, const char*);
extern PlaySoundFn oPlaySound;
using PanoramaAutoAcceptFn = void(__thiscall*)();
extern PanoramaAutoAcceptFn PanoramaAutoAccept;
using PaintTraverseFn = void(__thiscall*)(void*, unsigned int, bool, bool);
extern PaintTraverseFn oPaintTraverse;
using OnScreenSizeChangedFn = void(__thiscall*)(void*, int, int);
extern OnScreenSizeChangedFn oOnScreenSizeChanged;
using UnlockCursorFn = void(__thiscall*)(ISurface*);
extern UnlockCursorFn oUnlockCursor;
using LockCursorFn = void(__thiscall*)(ISurface*);
extern LockCursorFn oLockCursor;
struct mstudioseqdesc_t;
using pSeqdescFn = mstudioseqdesc_t*(__thiscall*)(studiohdr_t*, int);
extern pSeqdescFn opSeqdesc;
using CL_MoveFn = void (*)(void);
extern CL_MoveFn CL_Move;
using WriteUsercmdFn = void(__fastcall*)(void*, CUserCmd*, CUserCmd*);
extern WriteUsercmdFn oWriteUserCmd;
using ProcessTempEntitiesFn = bool(__thiscall*)(void*, void*);
extern ProcessTempEntitiesFn oProcessTempEntities;
using CL_FireEventsFn = void (*)(void);
extern CL_FireEventsFn CL_FireEvents;
using ProcessPacketFn = void(__thiscall*)(void*, void*, bool);
extern ProcessPacketFn oProcessPacket;
using ProcessOnDataChangedEventsFn = void (*)(void);
extern ProcessOnDataChangedEventsFn ProcessOnDataChangedEvents;
using PacketStartFn = void(__thiscall*)(void*, int, int);
extern PacketStartFn oPacketStart;
using PacketEndFn = void(__thiscall*)(void*);
extern PacketEndFn oPacketEnd;
using LevelShutdownPreEntityFn = void(__thiscall*)(void*);
extern LevelShutdownPreEntityFn oGameStats_LevelShutdownPreEntity;
using LevelInitPreEntityFn = void(__thiscall*)(void*);
extern LevelInitPreEntityFn oGameStats_LevelInitPreEntity;
using GameStatsInitFn = void(__thiscall*)(void*);
extern GameStatsInitFn oGameStatsInit;
using ModelRenderGlowFn = void(__thiscall*)(void*, DWORD);
extern ModelRenderGlowFn oModelRenderGlow;
extern void RestoreVACCedPointers();
//using DoPostScreenSpaceEffectsFn = bool(__fastcall*)(CViewSetup*, void*);
using DoPostScreenSpaceEffectsFn = bool(__thiscall*)(void*, CViewSetup*);
extern DoPostScreenSpaceEffectsFn oDoPostScreenSpaceEffects;
class bf_write;
using Net_SendPacketFn				= int(__fastcall*)(void* netchan, unsigned char *data, size_t length, bf_write *pVoicePayload, bool bUseCompression);
using CCLCMsg_Move_Deconstructor_Fn = void(__thiscall*)(void*);
extern CCLCMsg_Move_Deconstructor_Fn CCLCMsg_Move_Deconstructor;

#ifdef HOOK_LAG_COMPENSATION
using FrameUpdatePostEntityThinkFn = void(__fastcall*)(void*);
extern FrameUpdatePostEntityThinkFn oLagCompFrameUpdatePostEntityThink;
using StartLagCompensationFn = void(__thiscall*)(void*, void*, int, const Vector&, const QAngle&, float);
extern StartLagCompensationFn oStartLagCompensation;
#endif
using SetupBonesFn = bool(__thiscall*)(void*, matrix3x4_t*, int, int, float);
extern SetupBonesFn oSetupBones;
using SequencesAvailableCallFn = int(__thiscall*)(studiohdr_t*, int a2);
extern SequencesAvailableCallFn SequencesAvailableCall;
using GetShotgunSpreadFn = void(__thiscall*)(CBaseCombatWeapon*, int, int, int, float*, float*);

//float SetPoseParameter(CBaseEntity* me, void*pStudioHDR, int iParameter, float flValue, float *flValueResult);

typedef void(__thiscall* RenderViewFn)(void*, CViewSetup&, CViewSetup&, int, int);
extern RenderViewFn oRenderView;

using ShouldDrawLocalPlayerFn = bool(__fastcall*)(void*, void*, CBaseEntity*);
extern ShouldDrawLocalPlayerFn oShouldDrawLocalPlayer;

using LevelInitPostEntityFn = void(__thiscall*)(void*);
extern LevelInitPostEntityFn oLevelInitPostEntity;
using LevelInitPreEntityFn2 = void(__thiscall*)(void*, const char*);
extern LevelInitPreEntityFn2 oLevelInitPreEntityHLClient;
using ShutdownFn = void(__thiscall*)(void*);
extern ShutdownFn oShutdown;
using ShutdownNetchanFn = void(__fastcall*)(void*, void*, int, const char*);
extern ShutdownNetchanFn oShutdownNetchan;
using LevelShutdownFn = void(__thiscall*)(void*);
extern LevelShutdownFn oLevelShutdown;
using CL_SendMoveFn = void(__cdecl*)(void);
extern CL_SendMoveFn CL_SendMove;

using DrawModelFn = void(__thiscall*)(void*, void*, const DrawModelInfo_t&, matrix3x4a_t*, float*, float*, const Vector&, int);
extern DrawModelFn oDrawModel;

using AddRenderableFn = int(__thiscall*)(void*, IClientRenderable* p_renderable, int, RenderableTranslucencyType_t, int, int);
extern AddRenderableFn oAddRenderable;

using GroupStudioHdrFn = const studiohdr_t*(__thiscall*)(CStudioHdr*, DWORD);

using ListLeavesInBox = int(__thiscall*)(void*, const Vector& mins, const Vector& maxs, unsigned short* list, int list_max);
extern ListLeavesInBox oListLeavesInBox;

class CPrediction;
enum EGCResults;

namespace Hooks
{
extern bool __fastcall CreateMove(void* ecx, void* edx, float flInputSampleTime, CUserCmd* cmd);
extern void __fastcall OverrideView(void* ecx, void* edx, CViewSetup* setup);
extern void __fastcall View_Render(void* ecx, void* edx, vrect_t* rect);
extern int __fastcall new_IN_KeyEvent(void* ecx, void* edx, int eventcode, ButtonCode_t keynum, const char* pszCurrentBinding);
extern void __fastcall FrameStageNotify(void* ecx, void*, ClientFrameStage_t stage);
extern bool __fastcall WriteUsercmdDeltaToBuffer(void* ecx, void* edx, int slot, void* buf, int from, int to, bool isnewcommand);
extern void __fastcall DrawModelExecute(void* thisptr, int edx, void* ctx, void* state, const ModelRenderInfo_t& pInfo, matrix3x4_t* pCustomBoneToWorld);
extern void __fastcall DrawModel(void*, void*, void*, const DrawModelInfo_t&, matrix3x4a_t*, float*, float*, Vector&, int);
extern void __fastcall TraceRay(void* ths, void*, Ray_t const& ray, unsigned int mask, CTraceFilter* filter, trace_t& trace);
extern void __fastcall RunCommand(void* thisptr, void* edx, CBaseEntity* pEntity, CUserCmd* pUserCmd, void* moveHelper);
extern void __fastcall PredictionUpdate(CPrediction* pred, DWORD edx, int startframe, bool validframe, int incoming_acknowledged, int outgoing_command);
extern bool __fastcall new_InPrediction(void* ecx, void* edx);
extern float __stdcall GetLastTimeStamp(void);
extern void __stdcall TE_FireBullets_PostDataUpdate(DataUpdateType_t updateType);
extern void __fastcall TE_MuzzleFlash_PostDataUpdate(C_TEMuzzleFlash* thisptr, void* edx, DataUpdateType_t updateType);
extern void __fastcall TE_EffectDispatch_PostDataUpdate(C_TEEffectDispatch*, void* edx, DataUpdateType_t updateType);
extern void __fastcall RenderView(void* thisptr, void* edx, CViewSetup& setup, CViewSetup& hudViewSetup, int nClearFlags, int whatToDraw);
extern bool __fastcall ShouldDrawLocalPlayer(void* ecx, void* edx, CBaseEntity* pPlayer);
//extern void __stdcall SetViewAngles(QAngle &va);
extern bool __stdcall IsPaused();
extern BOOLEAN __fastcall AllowThirdPerson(void* gamerulespointer);
extern bool __fastcall DemoIsPlayingBack(void* thisptr);
extern int __fastcall SendDatagram(void* netchan, void*, void* datagram);
extern bool __fastcall CanPacket(void* netchan);
extern void __fastcall EmitSound(void* ecx, void* edx, void* filter, int iEntIndex, int iChannel, const char* pSoundEntry, unsigned int nSoundEntryHash, const char* pSample, float flVolume, float flAttenuation, int nSeed, int iFlags, int iPitch, const Vector* pOrigin, const Vector* pDirection, Vector* pUtlVecOrigins, bool bUpdatePositions, float soundtime, int speakerentity, int unknown);
extern void __fastcall HookedPlaySound(void* ecx, void* edx, const char* filename);
extern void __fastcall PaintTraverse(void* thisptr, void* edx, unsigned int panel, bool forceRepaint, bool allowForce);
extern void __fastcall hkOnScreenSizeChanged(void* _this, void* edx, int nOldWidth, int nOldHeight);
extern void __fastcall OnAddEntity(void* thisptr, DWORD EDX, IHandleEntity* pEnt, CBaseHandle handle);
extern void __fastcall OnRemoveEntity(void* thisptr, DWORD EDX, IHandleEntity* pEnt, CBaseHandle handle);
extern bool __fastcall IsHLTV(void* thisptr);
extern bool __fastcall ProcessTempEntities(void* clientstate, void* edx, void* msg);
extern void __fastcall ProcessPacket(void* netchan, void* edx, void* packet, bool bHasHeader);
extern bool __fastcall SendNetMsg(void* netchan, void* edx, void* msg, bool bForceReliable, bool bVoice);
extern void __fastcall HookedTempEntsUpdate(void* tempentsptr);
extern void __fastcall HookedUpdateTempEntBeams(void* beamsptr);
extern void __fastcall PacketStart(void* messagehandler, DWORD edx, int incoming_sequence, int outgoing_acknowledged);
extern void __fastcall PacketEnd(void* messagehandler);
extern void __stdcall LockCursor();
extern void __stdcall UnlockCursor();
extern void __fastcall VoiceStatus(void* ECX, void* EDX, int entindex, int iSsSlot, bool bTalking);
extern void __fastcall FireEvent(void* ECX, void* EDX, CGameEvent* event, bool bServerOnly);
extern void __fastcall GameStats_LevelShutdownPreEntity(void* ECX);
extern void __fastcall GameStats_LevelInitPreEntity(void* ECX);
extern void __fastcall GameStats_Init(void* ECX);
extern void __fastcall ModelRenderGlow(void* ECX, void* EDX, DWORD unknown);
extern void HookLevelShutdownAndInitPreEntity();
extern void HookModelRenderGlow();
extern void __fastcall LevelInitPostEntity(void* pclient);
extern void __fastcall LevelInitPreEntity(void* pclient, void* edx, const char* mapname);
extern void __fastcall LevelShutdown(void* pclient);
extern void __fastcall Shutdown(void* pclient);
extern void __fastcall ShutdownNetchan(void* netchan, void* edx, int code, const char* reason);
extern bool __fastcall DoPostScreenSpaceEffects(void* ecx, void* edx, CViewSetup* setup);
extern INetChannelInfo* __fastcall GetNetChannelInfo(void* eng);
extern void __fastcall Hooked_SceneEnd(IVRenderView* ecx, void* edx);
extern void __fastcall Hooked_CAM_ToFirstPerson(void* ecx, void*);
extern void __fastcall PlayerFallingDamage(void* ecx);
extern int __fastcall AddRenderable(void* ecx, void* edx, IClientRenderable* p_renderable, int unk1, RenderableTranslucencyType_t n_type, int unk2, int unk3);
extern int __fastcall ListLeavesInBox(void* ecx, void* edx, Vector& mins, Vector& maxs, unsigned short* list, int list_max);
extern void __fastcall Hooked_DoAnimationEvent(void* animstate, void* edx, PlayerAnimEvent_t event, int data);
extern void __fastcall CSPlayerAnimState_Release(void* animstate);
extern void __fastcall ProcessMovement(void* ecx, DWORD edx, CBaseEntity* basePlayer, CMoveData* moveData);
extern vcollide_t* __fastcall GetVCollide(void* ecx, DWORD edx, int index);
extern EGCResults __fastcall SendMessageGameCoordinator(void* ecx, DWORD edx, uint32_t unMsgType, const void *pubData, uint32_t cubData);
extern EGCResults __fastcall RetrieveMessageGameCoordinator(void* ecx, DWORD edx, uint32_t *punMsgType, void *pubDest, uint32_t cubDest, uint32_t *pcubMsgSize);
extern void __fastcall SetReservationCookie(void* clientstate, void* edx, unsigned long long cookie);
#ifdef HOOK_LAG_COMPENSATION
extern void __fastcall LagComp_FrameUpdatePostEntityThink(void* thisptr);
extern void __fastcall StartLagCompensation(void* thisptr, int edx, void* player, int lagCompensationType, const Vector& weaponPos, const QAngle& weaponAngles, float weaponRange);
#endif
} // namespace Hooks

extern uintptr_t ppMoveHelperClient;
extern char GameRulesShouldCollideOffset;
extern UINT(__cdecl* MD5_PseudoRandom)(UINT);
extern DWORD AdrOfSetLastTimeStampInterpolate;
extern DWORD AdrOfIsPausedExtrapolate;
extern DWORD AdrOfSetLastTimeStampFSN;
extern DWORD AdrOfs_bInterpolate;
extern DWORD AdrOfInvalidateBoneCache;
extern DWORD AdrOf_m_iDidCheckForOcclusion;
extern DWORD AdrOf_m_nWritableBones;
extern DWORD AdrOf_m_dwOcclusionArray;
extern DWORD AdrOf_StandardFilterRulesCallOne;
extern DWORD g_pClientLeafSystem;
extern DWORD AdrOf_StandardFilterRulesMemoryTwo;
extern DWORD AdrOf_SetAbsOrigin;
extern DWORD AdrOf_SetAbsAngles;
extern DWORD AdrOf_SetAbsVelocity;
extern DWORD AdrOf_DataCacheSetPoseParmaeter;
extern bool* s_bOverridePostProcessingDisable;
extern DWORD OffsetOf_UpdateClientSideAnimation;
extern DWORD OffsetOf_PlayerSpawnTime;
extern DWORD OffsetOf_SetDormant;
extern DWORD AdrOf_InvalidatePhysicsRecursive;
extern DWORD AdrOf_Frametime1;
extern DWORD AdrOf_Frametime2;
extern DWORD AdrOf_Frametime3;
extern DWORD* ClientCount;
extern DWORD ClientList;
extern DWORD AdrOf_NET_SendPacket;
extern DWORD AdrOf_CNET_SendData;
extern DWORD AdrOf_CNET_SendDatagram;
extern unsigned long GlowObjectManagerAdr;
extern unsigned long RadarBaseAdr;
extern DWORD DemoPlayerVTable;
extern DWORD DemoPlayerCreateMoveReturnAdr;
extern DWORD SendDatagramCL_MoveReturnAdr;
extern DWORD SequencesAvailableVMT;
extern DWORD IsEntityBreakable_FirstCall_Arg1;
extern DWORD IsEntityBreakable_FirstCall_Arg2;
extern DWORD IsEntityBreakable_SecondCall_Arg1;
extern DWORD IsEntityBreakable_SecondCall_Arg2;
extern DWORD AdrOf_IsEntityBreakableCall;
extern float* host_interval_per_tick;
extern CUserCmd* m_pCommands; //usercommands
extern DWORD AdrOf_SetupVelocityReturnAddress;
extern bool* s_bAbsRecomputationEnabled;
extern bool* s_bAbsQueriesValid;
extern void* RecvTable_Decode_Address;
extern DWORD AdrOf_CalcAbsoluteVelocity;
extern DWORD AdrOf_CalcAbsolutePosition;
extern DWORD AdrOf_SetGroundEntity;
extern DWORD AdrOf_SequenceDuration;
extern int* s_nTraceFilterCount;
extern void* s_TraceFilter; //MAX_NESTING 8
extern DWORD ModelRenderGlowVMT;
extern DWORD LevelShutdownPreEntityVMT;
extern DWORD LevelInitPreEntityVMT;
extern DWORD pLevelShutdownVMTWhenWeHooked;
extern DWORD* CHLClientTrap;
extern DWORD* CHLClientTrap2;
extern DWORD* LocalPlayerEntityTrap;
extern DWORD* EngineClientTrap;
extern DWORD* EngineClientTrap2;
extern DWORD* ModelRenderTrap;
extern DWORD* ModelRenderTrap2;

using IKInitFn = void(__thiscall*)(DWORD, CStudioHdr*, QAngle&, Vector&, float, int, int);
extern IKInitFn IKInit;
using UpdateTargetsFn = void(__thiscall*)(DWORD, Vector*, Quaternion*, matrix3x4_t*, byte*);
extern UpdateTargetsFn UpdateTargets;
using SolveDependenciesFn = void(__thiscall*)(DWORD, Vector*, Quaternion*, matrix3x4_t*, byte*);
extern SolveDependenciesFn SolveDependencies;
using AttachmentHelperFn = void(__thiscall*)(CBaseEntity*, CStudioHdr*);
extern AttachmentHelperFn AttachmentHelper;
using ConstructIKFn = DWORD(__thiscall*)(void*);
extern ConstructIKFn ConstructIK;
using TeleportedFn = bool(__thiscall*)(CBaseEntity*);
extern TeleportedFn Teleported;
using UnknownSetupBonesFn		 = DWORD(__thiscall*)(DWORD);
using ShouldSkipAnimationFrameFn = bool(__fastcall*)(CBaseEntity*);
extern ShouldSkipAnimationFrameFn ShouldSkipAnimationFrame;
extern DWORD ShouldSkipAnimationFrameIsPlayerReturnAdr;
using MDLCacheCriticalSectionCallFn = int(__thiscall*)(CBaseEntity*);
extern MDLCacheCriticalSectionCallFn MDLCacheCriticalSectionCall;
using GetSequenceNameFn = const char*(__thiscall*)(CBaseEntity*, int sequence);
extern GetSequenceNameFn GetSequenceName;
using GetSequenceActivityFn = Activities(__thiscall*)(CBaseEntity*, int sequence);
extern GetSequenceActivityFn GetSequenceActivity;
using GetSequenceActivityNameForModelFn = const char*(__fastcall*)(CStudioHdr*, int sequence);
extern GetSequenceActivityNameForModelFn GetSequenceActivityNameForModel;
using GetStringForKeyActivityListFn = const char*(__stdcall*)(unsigned short key);
extern GetStringForKeyActivityListFn GetStringForKeyActivityList;
using ActivityList_NameForIndexFn = const char*(__fastcall*)(int activityIndex);
extern ActivityList_NameForIndexFn ActivityList_NameForIndex;
using MarkForThreadedBoneSetupFn = void(__thiscall*)(CBaseEntity*);
extern MarkForThreadedBoneSetupFn MarkForThreadedBoneSetupCall;
using ReevaluateAnimLodFn = void(__thiscall*)(CBaseEntity*, CBaseEntity*);
extern ReevaluateAnimLodFn ReevaluateAnimLod;
using PredictionUpdateFn = void(__thiscall*)(void*, int, bool, int, int);
extern PredictionUpdateFn oPredictionUpdate;

using InPredictionFn = bool(__fastcall*)(void*, void*);
extern InPredictionFn oInPrediction;

using PredictionUpdateHLTVFn = void(__cdecl*)(void);
extern PredictionUpdateHLTVFn oPredictionUpdateHLTVFn;
using GetBonePositionFn = void(__thiscall*)(CBaseEntity*, int, Vector*);
extern GetBonePositionFn oGetBonePosition;
using LookupBoneFn = int(__thiscall*)(CBaseEntity*, char*);
extern LookupBoneFn oLookupBone;
typedef void(__thiscall* PreDataUpdateFn)(CBaseEntity* me, DataUpdateType_t updateType);
typedef void(__thiscall* PostDataUpdateFn)(CBaseEntity* me, DataUpdateType_t updateType);

extern PreDataUpdateFn oPreDataUpdate;
extern PostDataUpdateFn oPostDataUpdate;

extern std::list<CBaseEntity*> g_Infernos;

class svtable
{
public:
	virtual void unk0();
	virtual void unk1();
	virtual void unk2();
	virtual void unk3();
	virtual BOOLEAN IsActive(); /*{
		typedef BOOLEAN(__thiscall* OriginalFn)(svtable*);
		return GetVFunc<OriginalFn>(this, 4)(this);
	}
	*/
};

extern svtable** sv;
extern int* hoststate;
extern double* net_time;
inline bool IsHostingServer() { return *hoststate >= 2 || (*sv)->IsActive(); }

inline DWORD GetServerClientEntity(int i)
{
	DWORD EAX = *(DWORD*)ClientList;
	DWORD EBX = *(DWORD*)((i * 4) + EAX);

#if 1
	if (!EBX)
		return NULL;

	return (EBX + 4);
#endif

#if 0
	DWORD val = *(DWORD*)(*(DWORD*)ClientList + (4 * i));
	if (!val)
		return NULL;

	return (val + 4);
#endif
}

//CBaseEntity::Instance(edict_t*)
inline CBaseEntity* ServerClientToEntity(DWORD cl)
{
	cl -= 4;

	DWORD edict = *(DWORD*)(cl + 0x494); //0x48C

	DWORD ecx = *(DWORD*)(edict + 0x0C);
	if (ecx)
	{
		DWORD EAX = *(DWORD*)ecx;
		return ((CBaseEntity * (__thiscall*)(DWORD))(*(DWORD*)(EAX + 0x14)))(ecx);
	}

	return nullptr;
}

inline int GetServerClientCount()
{
	return *ClientCount;
}

inline int GetClientCount()
{
	return 64;
	//return ActualNumPlayers;
}

inline void* MALLOC(size_t size)
{
	using MemAllocFn = void*(__thiscall*)(void* thisptr, size_t);
	void* thisptr	= (void*)*(DWORD*)(gpMemAllocVTable);

	return GetVFunc< MemAllocFn >(thisptr, 1)(thisptr, size);
}

inline void FREE(void* ptr)
{
	using MemAllocFn = void*(__thiscall*)(void* thisptr, void*);
	void* thisptr = (void*)*(DWORD*)(gpMemAllocVTable);

	GetVFunc< MemAllocFn >(thisptr, 3)(thisptr, ptr);
}

inline void FREE_SPECIAL(void* ptr)
{
	using MemAllocFn = void*(__thiscall*)(void* thisptr, void*);
	void* thisptr = (void*)*(DWORD*)(gpMemAllocVTable);

	GetVFunc< MemAllocFn >(thisptr, 5)(thisptr, ptr);
}

#endif