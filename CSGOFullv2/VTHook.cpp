#include "precompiled.h"
#include "VTHook.h"
//#include "cx_strenc.h"
#include "IClientModeShared.h"
#include "IBaseClientDLL.h"
#include "Interfaces.h"
#include "AutoWall.h"
#include "Reporting.h"
#include "SetClanTag.h"
#include "ICvar.h"
#include "Events.h"
#include "EncryptString.h"
#include "DirectX.h"
#include "LocalPlayer.h"
#include <intrin.h> //VS2017 requires this for _ReturnAddress
#include "ClientSideAnimationList.h"
#include "VMProtectDefs.h"
#include "IVPanel.h"
#include "Draw.h"
#include <Psapi.h>
#include "GameVersion.h"
#include "CPlayerResource.h"
#include "tempents.h"
#include "beams.h"
#include "Gametypes.h"
#include "C_CSGameRules.h"
#include "C_CSGameTypes.h"
#include "shared_mem.h"
#include "NetworkedVariables.h"
#include "Spycam.h"
#include "CParallelProcessor.h"
#include "memalloc.h"
#include "Netvars.h"
#include "ReadPacketEntities.h"
#include "SteamAPI.h"
#include "isteamgamecoordinator.h"

extern void PlaceJMP(BYTE *bt_DetourAddress, DWORD dw_FunctionAddress, DWORD dw_Size);

DWORD SpoofSteamIDJmpBackAdr;

int __stdcall RunSpoofSteamID(uint64_t* dest)
{
	return 0;
	std::ifstream in("g:\\steamid_out.txt", std::ifstream::binary);
	if (in.is_open())
	{
		uint64_t newsteamid;
		in.read((char*)&newsteamid, sizeof(uint64_t));
		*dest = newsteamid;
		return 1;
	}
	return 0;
}

void __stdcall DumpSteamID(uint64_t* in)
{
	return;
	std::ofstream out("g:\\steamid_out.txt", std::ofstream::binary | std::ofstream::trunc);
	if (out.is_open())
	{
		uint64_t oldsteamid = *in;

		out.write((char*)&oldsteamid, sizeof(uint64_t));
	}
}

__declspec(naked) void SpoofSteamID()
{
	//dump steamid
	__asm
	{
		push edx
		mov ecx, [edi + 4]
		mov eax, [ecx]
		call[eax + 8]

		lea edx, [esp + 0x10]
		push edx
		call DumpSteamID
	}

	__asm
	{
		lea edx, [esp + 0x10]
		push edx
		call RunSpoofSteamID
		test eax, eax
		je DontSpoof
		jmp SpoofSteamIDJmpBackAdr
		DontSpoof:
		lea edx, [esp + 0x10]
		push edx
		mov ecx, [edi + 4]
		mov eax, [ecx]
		call [eax + 8]
		jmp SpoofSteamIDJmpBackAdr
	}
}

DWORD AdrOfSetLastTimeStampInterpolate = NULL;
DWORD AdrOfIsPausedExtrapolate = NULL;
DWORD AdrOfSetLastTimeStampFSN = NULL;
DWORD AdrOfs_bInterpolate = NULL;
DWORD AdrOfInvalidateBoneCache = NULL;
DWORD AdrOf_m_iDidCheckForOcclusion = NULL;
DWORD AdrOf_m_nWritableBones = NULL;
DWORD AdrOf_m_dwOcclusionArray = NULL;
DWORD AdrOf_StandardFilterRulesCallOne = NULL;
DWORD g_pClientLeafSystem = NULL;
DWORD AdrOf_StandardFilterRulesMemoryTwo = NULL;
DWORD AdrOf_SetAbsOrigin = NULL;
DWORD AdrOf_SetAbsAngles = NULL;
DWORD AdrOf_SetAbsVelocity = NULL;
DWORD AdrOf_DataCacheSetPoseParmaeter = NULL;
bool *s_bOverridePostProcessingDisable = NULL;
DWORD OffsetOf_UpdateClientSideAnimation = NULL;
DWORD OffsetOf_PlayerSpawnTime = NULL;
DWORD OffsetOf_SetDormant = NULL;
DWORD AdrOf_InvalidatePhysicsRecursive = NULL;
DWORD AdrOf_Frametime1 = NULL;
DWORD AdrOf_Frametime2 = NULL;
DWORD AdrOf_Frametime3 = NULL;
DWORD IsEntityBreakable_FirstCall_Arg1 = NULL;
DWORD IsEntityBreakable_FirstCall_Arg2 = NULL;
DWORD IsEntityBreakable_SecondCall_Arg1 = NULL;
DWORD IsEntityBreakable_SecondCall_Arg2 = NULL;
DWORD AdrOf_IsEntityBreakableCall = NULL;
unsigned long GlowObjectManagerAdr = NULL;
unsigned long RadarBaseAdr = NULL;
DWORD DemoPlayerVTable = NULL;
DWORD DemoPlayerCreateMoveReturnAdr = NULL;
DWORD SendDatagramCL_MoveReturnAdr = NULL;
float* host_interval_per_tick;
DWORD SequencesAvailableVMT = NULL;
svtable** sv = nullptr;
int* hoststate = nullptr;
double *net_time;
void *RecvTable_Decode_Address;
DWORD AdrOf_CalcAbsoluteVelocity;
DWORD AdrOf_CalcAbsolutePosition;
DWORD AdrOf_SetGroundEntity;
DWORD AdrOf_SequenceDuration;

GetLastTimeStampFn oGetLastTimeStamp;
LookupPoseParameterFn LookupPoseParameterGame;
GetPoseParameterRangeFn GetPoseParameterRangeGame;
SetPoseParameterFn SetPoseParameterGame;
SetupBonesFn oSetupBones;
GetStringForKeyActivityListFn GetStringForKeyActivityList;
PredictionUpdateFn oPredictionUpdate;
PredictionUpdateHLTVFn oPredictionUpdateHLTVFn;
InPredictionFn oInPrediction;
pSeqdescFn opSeqdesc;
CL_FireEventsFn CL_FireEvents;
ProcessOnDataChangedEventsFn ProcessOnDataChangedEvents = nullptr;
ResetAnimationStateFn oResetAnimState;
GetVCollideFn oGetVCollide;
IMemAlloc *g_pMemAlloc = nullptr;
bool g_bIsExiting = false;

int* s_nTraceFilterCount;
void *s_TraceFilter;

DWORD* ClientCount = nullptr;
DWORD ClientList = NULL;

//FIXME: MOVE ME
//FIXME: MOVE ME
float __stdcall Hooks::GetLastTimeStamp(void)
{
	return oGetLastTimeStamp();
}



uintptr_t ppMoveHelperClient;
char GameRulesShouldCollideOffset;
VTHook*	VPanel = nullptr;
VTHook*	ClientMode = nullptr;
VTHook*	Client = nullptr;
VTHook* Trace = nullptr;
VTHook*	Prediction = nullptr;
VTHook* Engine = nullptr;
VTHook* EngineClientSound = nullptr;
VTHook* TE_FireBullets = nullptr;
VTHook* TE_MuzzleFlash = nullptr;
VTHook* TE_EffectDispatch = nullptr;
VTHook* ModelRenderMo = nullptr;
VTHook* DemoPlayer = nullptr;
VTHook* HNetchan = nullptr;
VTHook* HSurface = nullptr;
VTHook* GameRules = nullptr;
VTHook* ClientEntityList = nullptr;
VTHook* HClientState = nullptr;
VTHook* StudioRenderHk = nullptr;
VTHook* HTempEnts = nullptr;
VTHook* HBeams = nullptr;
VTHook* HMessageHandler = nullptr;
VTHook* HGameEventListener = nullptr;
VTHook* Input = nullptr;
VTHook* MoveHelperClient   = nullptr;
VTHook* ClientLeaf = nullptr;
VTHook* BSPQuery = nullptr;
VTHook* HRenderView = nullptr;
VTHook* DirectX = nullptr;
VTHook* GameMovement = nullptr;
VTHook* ModelInfoClient = nullptr;
VTHook* ISteamGameCoordinator = nullptr;
//VTHook*			Surface;
//VTHook*			D3D9;

float(*RandomFloat) (float from, float to);
int(*RandomInt) (int from, int to);
void(*RandomSeed) (unsigned int seed);
bool(*ThreadInMainThread) (void);
DWORD gpMemAllocVTable;
DWORD gpKeyValuesVTable;
UINT(__cdecl* MD5_PseudoRandom)(UINT);



void InitKeyValues(KeyValues* pKeyValues, const char* name)
{
	static DWORD initKeyValueAddr = 0;
	if (!initKeyValueAddr)
	{
		initKeyValueAddr = FindMemoryPattern(ClientHandle, "68  ??  ??  ??  ??  8B  C8  E8  ??  ??  ??  ??  89  45  FC  EB  07  C7  45  ??  ??  ??  ??  ??  8B  03  56", 106) + 7;
		if (!initKeyValueAddr)
			initKeyValueAddr = 2;
	}

	if (initKeyValueAddr == 2)
		return;

	static DWORD dwFunction = 0;

	if (!dwFunction)
		dwFunction = initKeyValueAddr + *reinterpret_cast<PDWORD_PTR>(initKeyValueAddr + 1) + 5;

	if (!dwFunction)
	{
		return;
	}
	__asm
	{
		push name
		mov ecx, pKeyValues
		call dwFunction
	}
}
void LoadFromBuffer(KeyValues* pKeyValues, const char* resourceName, const char* pBuffer, void* pFileSystem, const char* pPathID, void* pfnEvaluateSymbolProc)
{
	static  DWORD dwFunction = 0;

	if (!dwFunction)
	{
		dwFunction = FindMemoryPattern(ClientHandle, "55  8B  EC  83  EC  48  53  56  57  8B  F9  89  7D  F4", 54);

		if (!dwFunction)
			dwFunction = 2;
	}

	if (dwFunction == 2)
	{
		return;
	}

	__asm
	{
		push pfnEvaluateSymbolProc
		push pPathID
		push pFileSystem
		push pBuffer
		push resourceName
		mov ecx, pKeyValues
		call dwFunction
	}
}
void SetupVMTHooks()
{


	//Setup interfaces from gamedlls
	//decrypts(0)
	while (!Interfaces::Client)
		Interfaces::Client = CaptureInterface<IBaseClientDll>(ClientHandle, XorStr("VClient018"));
	while (!Interfaces::ClientMode)
		Interfaces::ClientMode = **(IClientModeShared***)((*(DWORD**)Interfaces::Client)[10] + 0x5);
	while (!Interfaces::EngineClient)
		Interfaces::EngineClient = CaptureInterface<IEngineClient>(EngineHandle, XorStr("VEngineClient014"));
	while (!Interfaces::Globals)
		Interfaces::Globals = **(IGlobalVarsBase***)((*(DWORD**)Interfaces::Client)[0] + 0x1B);
	while (!Interfaces::EngineTrace)
		Interfaces::EngineTrace = CaptureInterface<IEngineTrace>(EngineHandle, XorStr("EngineTraceClient004"));
	while (!Interfaces::RenderView)
		Interfaces::RenderView = CaptureInterface<IVRenderView>(EngineHandle, XorStr("VEngineRenderView014"));
	while (!Interfaces::FileSystem)
		Interfaces::FileSystem = CaptureInterface<IFileSystem>(FileSystemStdioHandle, XorStr("VFileSystem017"));
	while (!Interfaces::ClientLeafSystem)
		Interfaces::ClientLeafSystem = CaptureInterface<IClientLeafSystem>(ClientHandle, XorStr("ClientLeafSystem002"));
	//encrypts(0)

	int clientversion = Interfaces::EngineClient->GetClientVersion();
	int buildversion = Interfaces::EngineClient->GetEngineBuildNumber();
	if (clientversion != CLIENT_VERSION || buildversion != ENGINE_BUILD_VERSION)
	{
#ifdef _DEBUG
		
#else
		//THROW_ERROR(ERR_OUTDATED_CSGO_VERSION);
		//exit(EXIT_SUCCESS);
#endif
	}

	const char *LevelName = Interfaces::EngineClient->GetLevelName();
	if (Interfaces::EngineClient->IsConnected() || Interfaces::EngineClient->IsInGame() || (LevelName && *(char*)LevelName != 0x0))
	{
		//decrypts(0)
		Interfaces::EngineClient->ClientCmd_Unrestricted(XorStr("disconnect"), nullptr);
		//encrypts(0)
	}

	//decrypts(0)
	HMODULE InputSystemHandle = GetModuleHandleA(XorStr("inputsystem.dll"));
	Interfaces::InputSystem = CaptureInterface<IInputSystem>(InputSystemHandle, XorStr("InputSystemVersion001"));

	VGUIMatSurfaceHandle = (HANDLE)GetModuleHandleA(XorStr("vguimatsurface.dll"));
	Interfaces::Surface = CaptureInterface<ISurface>(VGUIMatSurfaceHandle, XorStr("VGUI_Surface031"));
	//encrypts(0)

	g_NetworkedVariables.Init();
	StaticOffsets.AddAllOffsets();
	StaticOffsets.UpdateAllOffsets();
	HookEyeAnglesProxy();

	Interfaces::MoveHelperClient = StaticOffsets.GetOffsetValueByType<CMoveHelperClient**>(_MoveHelperClient);
	g_ClientState = **reinterpret_cast<CClientState***>((*reinterpret_cast<uintptr_t**>(Interfaces::EngineClient))[12] + 0x10); //StaticOffsets.GetOffsetValueByType<CClientState*>(_pClientState);
	Helper_GetLastCompetitiveMatchId = StaticOffsets.GetOffsetValueByType<uint64_t(__stdcall *)(void)>(_GetCompetitiveMatchID);
	pSetClanTag = StaticOffsets.GetOffsetValueByType<void(__fastcall*)(const char*, const char*)>(_ChangeClantag);
	IsPaused = StaticOffsets.GetOffsetValueByType<BOOLEAN(__thiscall*)(DWORD)>(_IsPaused);
	host_tickcount = StaticOffsets.GetOffsetValue(_host_tickcount);
	m_pPredictionRandomSeed = StaticOffsets.GetOffsetValueByType<DWORD*>(_predictionrandomseed);
	g_pGameRules = StaticOffsets.GetOffsetValueByType<C_CSGameRules**>(_GameRules);
	Interfaces::GameRules = g_pGameRules;
	g_pGameTypes = StaticOffsets.GetOffsetValueByType<IGameTypes**>(_GameTypes);
	Interfaces::GameTypes = g_pGameTypes;
	MD5_PseudoRandom = StaticOffsets.GetOffsetValueByType<UINT(__cdecl*)(UINT)>(_MD5PseudoRandom);
	AdrOfIsPausedExtrapolate = StaticOffsets.GetOffsetValue(_IsPausedExtrapolateReturnAddress);
	AdrOfInvalidateBoneCache = StaticOffsets.GetOffsetValue(_InvalidateBoneCache);
	g_pClientLeafSystem = StaticOffsets.GetOffsetValue(_g_pClientLeafSystem);
	AdrOf_StandardFilterRulesCallOne = StaticOffsets.GetOffsetValue(_StandardFilterRulesCallOne);
	ServerRankRevealAllEx = StaticOffsets.GetOffsetValueByType<ServerRankRevealAllFn>(_RevealRanks);
	AdrOf_SetAbsOrigin = StaticOffsets.GetOffsetValue(_SetAbsOrigin);
	AdrOf_SetAbsAngles = StaticOffsets.GetOffsetValue(_SetAbsAngles);
	AdrOf_SetAbsVelocity = StaticOffsets.GetOffsetValue(_SetAbsVelocity);
	LookupPoseParameterGame = StaticOffsets.GetOffsetValueByType<LookupPoseParameterFn>(_LookupPoseParameter);
	GetPoseParameterRangeGame = StaticOffsets.GetOffsetValueByType<GetPoseParameterRangeFn>(_GetPoseParameterRange);
	s_bOverridePostProcessingDisable = StaticOffsets.GetOffsetValueByType<bool*>(_OverridePostProcessingDisable);
	OffsetOf_UpdateClientSideAnimation = StaticOffsets.GetOffsetValue(_UpdateClientSideAnimation);
	oUpdateClientSideAnimation = StaticOffsets.GetOffsetValueByType<UpdateClientSideAnimationFn>(_UpdateClientSideAnimationFunction);
	AdrOf_InvalidatePhysicsRecursive = StaticOffsets.GetOffsetValue(_InvalidatePhysicsRecursive);
	AdrOf_Frametime1 = StaticOffsets.GetOffsetValue(_frametime1);
	AdrOf_Frametime2 = StaticOffsets.GetOffsetValue(_frametime2);
	AdrOf_Frametime3 = StaticOffsets.GetOffsetValue(_frametime3);
	g_ClientSideAnimationList = StaticOffsets.GetOffsetValueByType<CUtlVectorSimple*>(_ClientSideAnimationList);
	s_bEnableInvalidateBoneCache = StaticOffsets.GetOffsetValueByType<bool*>(_EnableInvalidateBoneCache);
	bAllowBoneAccessForNormalModels = StaticOffsets.GetOffsetValueByType<bool*>(_AllowBoneAccessForNormalModels);
	bAllowBoneAccessForViewModels = StaticOffsets.GetOffsetValueByType<bool*>(_AllowBoneAccessForViewModels);
	g_iModelBoneCounter = StaticOffsets.GetOffsetValueByType<unsigned long*>(_g_iModelBoneCounter);
	GlowObjectManagerAdr = StaticOffsets.GetOffsetValueByType<unsigned long>(_GlowObjectManager);
	RadarBaseAdr = StaticOffsets.GetOffsetValue(_RadarBase);
	IsEntityBreakable_FirstCall_Arg1 = StaticOffsets.GetOffsetValue(_IsEntityBreakable_FirstCall_Arg1);
	IsEntityBreakable_FirstCall_Arg2 = StaticOffsets.GetOffsetValue(_IsEntityBreakable_FirstCall_Arg2);
	IsEntityBreakable_SecondCall_Arg1 = StaticOffsets.GetOffsetValue(_IsEntityBreakable_SecondCall_Arg1);
	IsEntityBreakable_SecondCall_Arg2 = StaticOffsets.GetOffsetValue(_IsEntityBreakable_SecondCall_Arg2);
	AdrOf_IsEntityBreakableCall = StaticOffsets.GetOffsetValue(_IsEntityBreakable_ActualCall);
	WeaponDataPtrUnknown = StaticOffsets.GetOffsetValue(_WeaponScriptPointer);
	WeaponDataPtrUnknownCall = StaticOffsets.GetOffsetValue(_WeaponScriptPointerCall);
	GetCSWeaponDataAdr = StaticOffsets.GetOffsetValue(_GetWeaponSystem);
	pUTIL_ClipTraceToPlayers = (void(__fastcall*)(const Vector&, const Vector&, unsigned int, ITraceFilter *, trace_t *))StaticOffsets.GetOffsetValue(_UTIL_ClipTraceToPlayers);
	pUTIL_ClipTraceToPlayers2 = (void(__fastcall*)(const Vector&, const Vector&, unsigned int, ITraceFilter *, trace_t *, float))StaticOffsets.GetOffsetValue(_UTIL_ClipTraceToPlayers);
	Interfaces::Input = StaticOffsets.GetOffsetValueByType<CInput*>(_Input);
	oSetupBones = StaticOffsets.GetOffsetValueByType<SetupBonesFn>(_oSetupBones);
	IKInit = StaticOffsets.GetOffsetValueByType<IKInitFn>(_IKInit);
	UpdateTargets = StaticOffsets.GetOffsetValueByType<UpdateTargetsFn>(_UpdateTargets);
	SolveDependencies = StaticOffsets.GetOffsetValueByType<SolveDependenciesFn>(_Wrap_SolveDependencies);
	AttachmentHelper = StaticOffsets.GetOffsetValueByType<AttachmentHelperFn>(_AttachmentHelper);
	ConstructIK = StaticOffsets.GetOffsetValueByType<ConstructIKFn>(_CreateIK);
	Teleported = StaticOffsets.GetOffsetValueByType<TeleportedFn>(_Teleported);
	ShouldSkipAnimationFrame = StaticOffsets.GetOffsetValueByType<ShouldSkipAnimationFrameFn>(_ShouldSkipAnimFrame);
	ShouldSkipAnimationFrameIsPlayerReturnAdr = (DWORD)ShouldSkipAnimationFrame + 0xD;
	g_bInThreadedBoneSetup = StaticOffsets.GetOffsetValueByType<bool*>(_InThreadedBoneSetup);
	MarkForThreadedBoneSetupCall = StaticOffsets.GetOffsetValueByType<MarkForThreadedBoneSetupFn>(_MarkForThreadedBoneSetup);
	SequencesAvailableVMT = StaticOffsets.GetOffsetValue(_SequencesAvailableVMT);
	SequencesAvailableCall = (SequencesAvailableCallFn)StaticOffsets.GetOffsetValueByType<SequencesAvailableCallFn>(_SequencesAvailableCall);
	ReevaluateAnimLod = StaticOffsets.GetOffsetValueByType<ReevaluateAnimLodFn>(_ReevaluateAnimLod);
	host_interval_per_tick = StaticOffsets.GetOffsetValueByType<float*>(_host_interval_per_tick);
	GetSequenceName = (GetSequenceNameFn)StaticOffsets.GetOffsetValueByType<GetSequenceNameFn>(_GetSequenceName);
	GetSequenceActivity = (GetSequenceActivityFn)StaticOffsets.GetOffsetValueByType<GetSequenceActivityFn>(_GetSequenceActivity);
	GetSequenceActivityNameForModel = (GetSequenceActivityNameForModelFn)StaticOffsets.GetOffsetValueByType<GetSequenceActivityNameForModelFn>(_GetSequenceActivityNameForModel);

#if 0
	DecStr(getstringforkeyactivitylistsig, 34);
	adr = FindMemoryPattern(ClientHandle, getstringforkeyactivitylistsig, 34);
	EncStr(getstringforkeyactivitylistsig, 34);
	delete[]getstringforkeyactivitylistsig;

	if (!adr)
	{
		THROW_ERROR(ERR_CANT_FIND_GET_STRING_FOR_KEY_ACTIVITYLIST_SIG);
		exit(EXIT_SUCCESS);
	}

	GetStringForKeyActivityList = (GetStringForKeyActivityListFn)adr;
#endif

	ActivityList_NameForIndex = StaticOffsets.GetOffsetValueByType<ActivityList_NameForIndexFn>(_ActivityListNameForIndex);
	opSeqdesc = StaticOffsets.GetOffsetValueByType<pSeqdescFn>(_pSeqDesc);
	sv = StaticOffsets.GetOffsetValueByType<svtable**>(_svtable);
	hoststate = StaticOffsets.GetOffsetValueByType<int*>(_hoststate);
	CL_Move = StaticOffsets.GetOffsetValueByType<CL_MoveFn>(_CL_Move);
	oWriteUserCmd = StaticOffsets.GetOffsetValueByType<WriteUsercmdFn>(_WriteUserCmd);
	m_pCommands = StaticOffsets.GetOffsetValueByType<CUserCmd*>(_pCommands);
	AdrOf_SetupVelocityReturnAddress = StaticOffsets.GetOffsetValue(_SetupVelocityReturnAddress);
	CL_FireEvents = StaticOffsets.GetOffsetValueByType<CL_FireEventsFn>(_CL_FireEvents);
	s_bAbsRecomputationEnabled = StaticOffsets.GetOffsetValueByType<bool*>(_AbsRecomputationEnabled);
	s_bAbsQueriesValid = StaticOffsets.GetOffsetValueByType<bool*>(_AbsQueriesValid);
	g_PR = StaticOffsets.GetOffsetValueByType<CPlayerResource*>(_PlayerResource);
	OffsetOf_PlayerSpawnTime = StaticOffsets.GetOffsetValue(_SpawnTime);
	OffsetOf_SetDormant = StaticOffsets.GetOffsetValue(_SetDormant);
	oResetAnimState = StaticOffsets.GetOffsetValueByType<ResetAnimationStateFn>(_ResetAnimationState);
	ProcessOnDataChangedEvents = StaticOffsets.GetOffsetValueByType<ProcessOnDataChangedEventsFn>(_ProcessOnDataChangedEvents);
	tempents = StaticOffsets.GetOffsetValueByType<CTempEnts*>(_pTempEnts);
	beams = StaticOffsets.GetOffsetValueByType<CBeams*>(_pBeams);
	net_time = StaticOffsets.GetOffsetValueByType<double*>(_Net_Time);
	RecvTable_Decode_Address = StaticOffsets.GetOffsetValueByType<void*>(_Receivetable_Decode);
	AdrOf_CalcAbsoluteVelocity = StaticOffsets.GetOffsetValue(_CalcAbsoluteVelocity);
	AdrOf_CalcAbsolutePosition = StaticOffsets.GetOffsetValue(_CalcAbsolutePosition);
	AdrOf_SetGroundEntity = StaticOffsets.GetOffsetValue(_SetGroundEntity);
	AdrOf_SequenceDuration = StaticOffsets.GetOffsetValue(_SequenceDuration);
	oLockStudioHdr = StaticOffsets.GetOffsetValueByType<LockStudioHdrFn>(_LockStudioHdr);
	oGetFirstSequenceAnimTag = StaticOffsets.GetOffsetValueByType<GetFirstSequenceAnimTagFn>(_GetFirstSequenceAnimTag);
	gametypes = StaticOffsets.GetOffsetValueByType<GameTypes**>(_GameTypes);
	oSurpressLadderChecks = StaticOffsets.GetOffsetValueByType<SurpressLadderChecksFn>(_SurpressLadderChecks);
	oSetPunchVMT = StaticOffsets.GetOffsetValueByType<SetPunchVMTFn>(_SetPunchAngleVMT);
	oIsInAVehicle = StaticOffsets.GetOffsetValueByType<IsInAVehicleFn>(_IsInAVehicle);
	s_nTraceFilterCount = StaticOffsets.GetOffsetValueByType<int*>(_s_nTraceFilterCount);
	s_TraceFilter = StaticOffsets.GetOffsetValueByType<void*>(_s_TraceFilter);
	oIsCarryingHostage = StaticOffsets.GetOffsetValueByType<IsCarryingHostageFn>(_IsCarryingHostage);
	oEyeVectors = StaticOffsets.GetOffsetValueByType<EyeVectorsFn>(_EyeVectors);
	oCreateStuckTable = StaticOffsets.GetOffsetValueByType<CreateStuckTableFn>(_CreateStuckTable);
	rgv3tStuckTable = StaticOffsets.GetOffsetValueByType<Vector**>(_rgv3tStuckTable);
	LevelShutdownPreEntityVMT = StaticOffsets.GetOffsetValue(_LevelShutdownPreEntity);
	LevelInitPreEntityVMT = StaticOffsets.GetOffsetValue(_LevelInitPreEntity);
	CHLClientTrap2 = StaticOffsets.GetOffsetValueByType<DWORD*>(_CHLClientTrap2);
	EngineClientTrap2 = StaticOffsets.GetOffsetValueByType<DWORD*>(_EngineClientTrap2);
	CHLClientTrap = StaticOffsets.GetOffsetValueByType<DWORD*>(_LevelShutdownCHLClientTrap);
	EngineClientTrap = StaticOffsets.GetOffsetValueByType<DWORD*>(_LevelShutdownEngineClientTrap);
	LocalPlayerEntityTrap = StaticOffsets.GetOffsetValueByType<DWORD*>(_LocalPlayerEntityTrap);
	ModelRenderGlowVMT = StaticOffsets.GetOffsetValue(_ModelRenderGlow); //NOTE: FIX HookModelRenderGlow VFUNC INDEX IF THIS CHANGES
	ModelRenderTrap = StaticOffsets.GetOffsetValueByType<DWORD*>(_ModelRenderGlowTrap);
	ModelRenderTrap2 = StaticOffsets.GetOffsetValueByType<DWORD*>(_ModelRenderGlowTrap2);
	ParallelProcess_original = StaticOffsets.GetOffsetValueByType<ParallelProcessFn>(_ParallelProcess);

#ifdef DYLAN_VAC
	Hooks::HookLevelShutdownAndInitPreEntity();
	//Hooks::HookModelRenderGlow();
#endif

	oPredictionUpdateHLTVFn = StaticOffsets.GetOffsetValueByType<PredictionUpdateHLTVFn>(_PredictionUpdateHLTVCall);
	oGetBonePosition = StaticOffsets.GetOffsetValueByType<GetBonePositionFn>(_GetBonePosition);
	oLookupBone = StaticOffsets.GetOffsetValueByType<LookupBoneFn>(_LookupBone);
	GetNetChannelInfo_cvarcheck_retaddr = StaticOffsets.GetOffsetValue(_GetNetChannelInfoCvarCheck);

	////////////////////////////////
	//ADD NEW ONES ABOVE HERE
	////////////////////////////////

#ifndef SERVER_SIDE_ONLY
	ClientEntityList = new VTHook(StaticOffsets.GetOffsetValueByType<DWORD**>(_IClientEntityList), hook_types::_ClientEntityList);
	oOnAddEntity = (OnCreateEntityFn)ClientEntityList->HookFunction((DWORD)Hooks::OnAddEntity, IClientEntityList::ONENTITYCREATED);
	oOnRemoveEntity = (OnDeleteEntityFn)ClientEntityList->HookFunction((DWORD)Hooks::OnRemoveEntity, IClientEntityList::ONENTITYDELETED);
#endif

	Interfaces::Beams = StaticOffsets.GetOffsetValueByType<IViewRenderBeams*>(_RenderBeams);

	Interfaces::Hud = StaticOffsets.GetOffsetValueByType<CHud*>(_Hud);

	//decrypts(0)
	Interfaces::ClientEntList = CaptureInterface<CClientEntityList>(ClientHandle, XorStr("VClientEntityList003"));
	auto entptroffset = StaticOffsets.GetOffsetValue(_ClientEntityListArray);
	entptroffset += entptroffset;
	m_EntPtrArray = (CEntInfo*)((DWORD)(entptroffset * 8) + (DWORD)Interfaces::ClientEntList);

	Interfaces::Physprops = CaptureInterface<IPhysicsSurfaceProps>(VPhysicsHandle, XorStr("VPhysicsSurfaceProps001"));

	Interfaces::ModelRender = CaptureInterface<IVModelRender>(EngineHandle, XorStr("VEngineModel016"));

	Interfaces::StudioRender = CaptureInterface<CStudioRenderContext>(StudioRenderHandle, XorStr("VStudioRender026"));

	Interfaces::MatSystem = CaptureInterface<CMaterialSystem>(MaterialSystemHandle, XorStr("VMaterialSystem080"));

	Interfaces::Prediction = CaptureInterface<CPrediction>(ClientHandle, XorStr("VClientPrediction001"));

	Interfaces::GameMovement = CaptureInterface<CGameMovement>(ClientHandle, XorStr("GameMovement001"));

	Interfaces::ModelInfoClient = CaptureInterface<IVModelInfoClient>(EngineHandle, XorStr("VModelInfoClient004"));

	Interfaces::Cvar = CaptureInterface<ICVar>(VSTDLIBHandle, XorStr("VEngineCvar007"));

	Interfaces::GameEventManager = CaptureInterface<IGameEventManager2>(EngineHandle, XorStr("GAMEEVENTSMANAGER002"));

	Interfaces::DebugOverlay = CaptureInterface<IVDebugOverlay>(EngineHandle, XorStr("VDebugOverlay004"));

	Interfaces::MDLCache = CaptureInterface<IMDLCache>(DatacacheHandle, XorStr("MDLCache004"));

	Interfaces::EngineSound = CaptureInterface<IEngineSound>(EngineHandle, XorStr("IEngineSoundClient003"));

	Interfaces::PhysicsCollision = CaptureInterface<IPhysicsCollision>(VPhysicsHandle, XorStr("VPhysicsCollision007"));

	VGUI2Handle = (HANDLE)GetModuleHandleA(XorStr("vgui2.dll"));
	Interfaces::VPanel = CaptureInterface<IVPanel>(VGUI2Handle, XorStr("VGUI_Panel009"));

	HMODULE vstdlibhandle = GetModuleHandle(XorStr("vstdlib.dll"));

	auto randomfloatproc = GetProcAddress(vstdlibhandle, XorStr("RandomFloat"));
	//encrypts(0)

	RandomFloat = (float (*)(float, float))randomfloatproc;
	if (!RandomFloat)
	{
		//THROW_ERROR(ERR_CANT_FIND_RANDOMFLOAT);
		exit(EXIT_SUCCESS);
	}

	//decrypts(0)
	auto randomseedproc = GetProcAddress(vstdlibhandle, XorStr("RandomSeed"));
	//encrypts(0)

	RandomSeed = (void (*)(unsigned int))randomseedproc;
	if (!RandomSeed)
	{
		//THROW_ERROR(ERR_CANT_FIND_RANDOMSEED);
		exit(EXIT_SUCCESS);
	}

	//decrypts(0)
	auto randomintproc = GetProcAddress(vstdlibhandle, XorStr("RandomInt"));
	//encrypts(0)

	RandomInt = (int(*)(int, int))randomintproc;
	if (!RandomInt)
	{
		//THROW_ERROR(ERR_CANT_FIND_RANDOMSEED);
		exit(EXIT_SUCCESS);
	}

	//decrypts(0)
	auto keyvaluesproc = GetProcAddress(vstdlibhandle, XorStr("KeyValuesSystem"));
	//encrypts(0)

	gpKeyValuesVTable = (DWORD)keyvaluesproc;
	if (!gpKeyValuesVTable)
	{
		//THROW_ERROR(ERR_CANT_FIND_RANDOMSEED);
		exit(EXIT_SUCCESS);
	}

	//decrypts(0)
	g_pThreadPool = *(IThreadPool**)GetProcAddress(vstdlibhandle, XorStr("g_pThreadPool"));

	auto threadinmainthreadproc = GetProcAddress((HMODULE)Tier0Handle, XorStr("ThreadInMainThread"));
	ThreadInMainThread = (bool (*)(void))threadinmainthreadproc;

	gpMemAllocVTable = (DWORD)GetProcAddress((HMODULE)Tier0Handle, XorStr("g_pMemAlloc"));
	g_pMemAlloc = *(IMemAlloc**)gpMemAllocVTable;

	Interfaces::TE_EffectDispatch = StaticOffsets.GetOffsetValueByType<C_TEEffectDispatch*>(_TE_EffectDispatch);

	auto cl_modelfastpath = Interfaces::Cvar->FindVar(XorStr("cl_modelfastpath"));
	//encrypts(0)

	if (cl_modelfastpath)
	{
		cl_modelfastpath->nFlags &= ~(FCVAR_HIDDEN | FCVAR_DEVELOPMENTONLY);
		cl_modelfastpath->SetValue(1);
	}

	//Hook virtual method tables

#ifndef SERVER_SIDE_ONLY

//#ifndef USE_REBUILT_HANDLE_BULLET_PENETRATION
	//Trace = new VTHook((DWORD**)Interfaces::EngineTrace);
	//oTraceRay = (TraceRayFn)Trace->HookFunction((DWORD)Hooks::TraceRay, 5);
//#endif
	ClientMode = new VTHook((DWORD**)Interfaces::ClientMode, hook_types::_ClientMode);
	Client = new VTHook((DWORD**)Interfaces::Client, hook_types::_Client);
	Prediction = new VTHook((DWORD**)Interfaces::Prediction, hook_types::_Prediction);
	Engine = new VTHook((DWORD**)Interfaces::EngineClient, hook_types::_Engine);

	TE_EffectDispatch = new VTHook((DWORD**)Interfaces::TE_EffectDispatch, hook_types::_EffectDispatch);
	EngineClientSound = new VTHook((DWORD**)Interfaces::EngineSound, hook_types::_EngineClientSound);
	HSurface = new VTHook((DWORD**)Interfaces::Surface, hook_types::_Surface);
	VPanel = new VTHook((DWORD**)Interfaces::VPanel, hook_types::_VPanel);
	ModelRenderMo = new VTHook(reinterpret_cast<DWORD**>(Interfaces::ModelRender), hook_types::_ModelRender);
	StudioRenderHk = new VTHook(reinterpret_cast<DWORD**>(Interfaces::StudioRender), hook_types::_StudioRender);
	HClientState = new VTHook((DWORD**)((DWORD)g_ClientState + 8), hook_types::_ClientState);
	HTempEnts = new VTHook((DWORD**)(tempents), hook_types::_TempEnts);
	HBeams = new VTHook((DWORD**)(beams), hook_types::_Beams);
	HGameEventListener = new VTHook((DWORD**)(Interfaces::GameEventManager), hook_types::_GameEventListener);
	HRenderView = new VTHook((DWORD**)Interfaces::RenderView, hook_types::_RenderView);
	Input = new VTHook((DWORD**)Interfaces::Input, hook_types::_IInput);
	MoveHelperClient = new VTHook((DWORD**)*Interfaces::MoveHelperClient, hook_types::_MoveHelperClientHook);
	GameMovement = new VTHook((DWORD**)Interfaces::GameMovement, hook_types::_GameMovement);
	ModelInfoClient = new VTHook((DWORD**)Interfaces::ModelInfoClient, hook_types::_ModelInfoClient);
	ISteamGameCoordinator = new VTHook((DWORD**)GetSteamGameCoordinator(), hook_types::_SteamGameCoordinator);

	TE_FireBullets = new VTHook((DWORD**)Interfaces::TE_FireBullets);

	ClientLeaf = new VTHook((DWORD**)Interfaces::ClientLeafSystem, hook_types::_ClientLeafSystem);
	BSPQuery = new VTHook((DWORD**)Interfaces::EngineClient->GetBSPTreeQuery(), hook_types::_BSPQuery);
	//HRenderView = new VTHook((DWORD**)adrsig);
	//Hooks::Surface = new VTHook((DWORD**)Interfaces::Surface);
	//Hooks::D3D9 = new VTHook((DWORD**)offsets.d3d9Device);

	oTE_FireBullets_PostDataUpdate = (TE_FireBullets_PostDataUpdateFn)TE_FireBullets->HookFunction((DWORD)Hooks::TE_FireBullets_PostDataUpdate, 7);
	//oTE_MuzzleFlash_PostDataUpdate = (TE_MuzzleFlash_PostDataUpdateFn)TE_MuzzleFlash->HookFunction((DWORD)Hooks::TE_MuzzleFlash_PostDataUpdate, 7);
	oTE_EffectDispatch_PostDataUpdate = (TE_EffectDispatch_PostDataUpdateFn)TE_EffectDispatch->HookFunction((DWORD)Hooks::TE_EffectDispatch_PostDataUpdate, 7); // Can get done easier I guess
	
#ifdef DYLAN_VAC
	oVoiceStatus = (VoiceStatusFn)Client->HookFunction((DWORD)Hooks::VoiceStatus, (0x88 / 4)); //0x84 / 4
#endif
	oFireEvent = (FireGameEventFn)HGameEventListener->HookFunction((DWORD)Hooks::FireEvent, (0x20 / 4));


	oOverrideView = (OverrideViewFN)ClientMode->HookFunction((DWORD)Hooks::OverrideView, 18);
	oCreateMove = (CreateMoveFn)ClientMode->HookFunction((DWORD)Hooks::CreateMove, 24);
	oDoPostScreenSpaceEffects = (DoPostScreenSpaceEffectsFn)ClientMode->HookFunction((DWORD)Hooks::DoPostScreenSpaceEffects, 44);
	oRunCommand = (RunCommandFn)Prediction->HookFunction((DWORD)Hooks::RunCommand, 19);
	#ifdef UK_FIX
	oPredictionUpdate = (PredictionUpdateFn)Prediction->HookFunction((DWORD)Hooks::PredictionUpdate, (0x0C / 4));
	#else
	oPredictionUpdate = (PredictionUpdateFn)Prediction->HookFunction((DWORD)Hooks::PredictionUpdate, (0x0C / 4));
	#endif
	oInPrediction = (InPredictionFn)Prediction->HookFunction((DWORD)Hooks::new_InPrediction, 14);
	//oGetLastTimeStamp = (GetLastTimeStampFn)EngineClient->HookFunction((DWORD)Hooks::GetLastTimeStamp, 14);
	oIsPaused = (IsPausedFn)Engine->HookFunction((DWORD)Hooks::IsPaused, 90);
	oGetNetChannelInfo = (GetNetChannelInfoFn)Engine->HookFunction((DWORD)Hooks::GetNetChannelInfo, 78);
	oEmitSound = (EmitSoundFn)EngineClientSound->HookFunction((DWORD)Hooks::EmitSound, 5);
	oOnScreenSizeChanged = (OnScreenSizeChangedFn)HSurface->HookFunction((DWORD)Hooks::hkOnScreenSizeChanged, 116);
	oUnlockCursor = (UnlockCursorFn)HSurface->HookFunction((DWORD)Hooks::UnlockCursor, 66);
	oLockCursor = (LockCursorFn)HSurface->HookFunction((DWORD)Hooks::LockCursor, 67);
	oDrawModelExecute = (DrawModelExecuteFn)ModelRenderMo->HookFunction((DWORD)Hooks::DrawModelExecute, 21);
	oDrawModel = (DrawModelFn)StudioRenderHk->HookFunction((DWORD)Hooks::DrawModel, 29); //not now
	oPaintTraverse = (PaintTraverseFn)VPanel->HookFunction((DWORD)Hooks::PaintTraverse, 41);
	//oViewRender = (ViewRenderFn)Client->HookFunction((DWORD)Hooks::View_Render, 27);
	oFrameStageNotify = (FrameStageNotifyFn)Client->HookFunction((DWORD)Hooks::FrameStageNotify, 37); //36
	oShutdown = (ShutdownFn)Client->HookFunction((DWORD)Hooks::Shutdown, 4);
	oLevelInitPreEntityHLClient = (LevelInitPreEntityFn2)Client->HookFunction((DWORD)Hooks::LevelInitPreEntity, 5);
	oLevelInitPostEntity = (LevelInitPostEntityFn)Client->HookFunction((DWORD)Hooks::LevelInitPostEntity, 6);
	oLevelShutdown = (LevelShutdownFn)Client->HookFunction((DWORD)Hooks::LevelShutdown, 7);
	oWriteUsercmdDeltaToBuffer = (WriteUsercmdDeltaToBufferFn)Client->HookFunction((DWORD)Hooks::WriteUsercmdDeltaToBuffer, 24);
	oIsHLTV = (IsHLTVFn)Engine->HookFunction((DWORD)Hooks::IsHLTV, 93); //93
	oCAM_ToFirstPerson = (CAM_ToFirstPersonFn)Input->HookFunction((DWORD)Hooks::Hooked_CAM_ToFirstPerson, 36);
	oPlayerFallingDamage = (PlayerFallingDamageFn)MoveHelperClient->HookFunction((DWORD)Hooks::PlayerFallingDamage, 9);
	oProcessMovement = (ProcessMovementFn)GameMovement->HookFunction((DWORD)Hooks::ProcessMovement, 1);
	oGetVCollide = (GetVCollideFn)ModelInfoClient->HookFunction((DWORD)Hooks::GetVCollide, 6);
	oSendMessageSteamGameCoordinator = (SendMessageGameCoordinatorFn)ISteamGameCoordinator->HookFunction((DWORD)Hooks::SendMessageGameCoordinator, 0);
	oRetrieveMessageSteamGameCoordinator = (RetrieveMessageGameCoordinatorFn)ISteamGameCoordinator->HookFunction((DWORD)Hooks::RetrieveMessageGameCoordinator, 2);

	// ProcessTempEntities
	// DWORD searchaddress = FindMemoryPattern(EngineHandle, "55  8B  EC  83  E4  F8  83  EC  4C  A1  ??  ??  ??  ??  80  B8  ??  ??  00  00  00", strlen("55  8B  EC  83  E4  F8  83  EC  4C  A1  ??  ??  ??  ??  80  B8  ??  ??  00  00  00"));
	// int indexfound = HClientState->FindIndexFromAddress(searchaddress);
	oProcessTempEntities = (ProcessTempEntitiesFn)HClientState->HookFunction((DWORD)Hooks::ProcessTempEntities, 36);
	oTempEntsUpdate = (TempEntsUpdateFn)HTempEnts->HookFunction((DWORD)Hooks::HookedTempEntsUpdate, 5);
	oUpdateTempEntBeams = (UpdateTempEntBeamsFn)HBeams->HookFunction((DWORD)Hooks::HookedUpdateTempEntBeams, 3);
	oSetReservationCookie = (SetReservationCookieFn)HClientState->HookFunction((DWORD)Hooks::SetReservationCookie, 63);
	//oReadPacketEntities = (ReadPacketEntitiesFn)HClientState->HookFunction((DWORD)Hooked_ReadPacketEntities, 64);

	//oShouldDrawLocalPlayer = (ShouldDrawLocalPlayerFn)ClientMode->HookFunction((DWORD)Hooks::ShouldDrawLocalPlayer.Entity, 15);
	//oRenderView = (RenderViewFn)HRenderView->HookFunction((DWORD)Hooks::RenderView, 6);
	//oSetViewAngles = (SetViewAnglesFn)EngineClient->HookFunction((DWORD)Hooks::SetViewAngles, 19);

	DirectX = new VTHook(*StaticOffsets.GetOffsetValueByType<DWORD***>(_DirectXPrePointer), hook_types::_DirectX);
	oEndScene = (EndSceneFn)DirectX->HookFunction((DWORD)Hooked_EndScene, 42);
	oReset = (ResetFn)DirectX->HookFunction((DWORD)Hooked_Reset, 16);
	oSceneEnd = (SceneEndFn)HRenderView->HookFunction((DWORD)Hooks::Hooked_SceneEnd, 9);
	oAddRenderable = (AddRenderableFn)ClientLeaf->HookFunction((DWORD)Hooks::AddRenderable, 7);
	oListLeavesInBox = (ListLeavesInBox)BSPQuery->HookFunction((DWORD)Hooks::ListLeavesInBox, 6);

	CreateEventListeners(); //Create Game Event Listeners

	DWORD loladr = FindMemoryPattern(EngineHandle, "52  8B  01  FF  50  08  8B  74  24  10  8D  4C  24  18");
	if (loladr)
	{
		//PlaceJMP((BYTE*)loladr, (DWORD)&SpoofSteamID, 6);
		SpoofSteamIDJmpBackAdr = loladr + 6;
	}
#endif
}