#pragma once
#include <Windows.h>
#include "NetVarManager.h"
#include "checksum_crc.h"
#include <array>

extern bool g_bIsExiting;

#ifdef GetProp
#	undef GetProp
#endif

class CHookedProp
{
public:
	CHookedProp()
	{
		m_Prop = 0;
		m_OriginalProxy = 0;
	}

	CHookedProp(RecvProp* Prop)
	{
		m_Prop = Prop;
		m_OriginalProxy = m_Prop->GetProxyFn();
	}

	~CHookedProp()
	{
		// Check against invalid memory region since when csgo exits normally unhooking a invalid proxy leads to a access violation
		if (!g_bIsExiting && !IsBadCodePtr((FARPROC)m_Prop))
			m_Prop->SetProxyFn(m_OriginalProxy);

		m_Prop = 0;
		m_OriginalProxy = 0;
	}

	RecvProp* GetProp()
	{
		return m_Prop;
	}

	RecvVarProxyFn GetOriginalProxy()
	{
		return m_OriginalProxy;
	}

private:
	RecvProp* m_Prop;
	RecvVarProxyFn m_OriginalProxy;
};

class CNetworkedVariables
{
public:
	CNetworkedVariables()
	{
		m_tables.clear();
		m_HookedProps.clear();
	}

	~CNetworkedVariables()
	{
		m_tables.clear();

		for (auto prop : m_HookedProps) // C++ does NOT call the destructor of a class so we have to so it manually -.-
		{
			delete prop;
		}

		m_HookedProps.clear();
	}

	int GetClassID(ClientClass* clientClass, const char* name);
	int GetClassID(ClientClass* clientClass, uint32_t hash);

	void Init();
	void InitClassIDs();
	int GetNetPropOffset(RecvTable* propTable, const char* propName, RecvVarProxyFn* origfn = 0, RecvVarProxyFn fn = 0);
	int GetNetPropOffset(RecvTable* propTable, CRC32_t propHash, RecvVarProxyFn* origfn = 0, RecvVarProxyFn fn = 0);
	int GetNetPropOffset(const char* tableName, const char* propName, RecvVarProxyFn* origfn = 0, RecvVarProxyFn fn = 0);
	int GetNetPropOffset(CRC32_t tableHash, CRC32_t propHash, RecvVarProxyFn* origfn = 0, RecvVarProxyFn fn = 0);

	RecvVarProxyFn GetOriginalProxy(const char* propName);
	RecvVarProxyFn GetOriginalProxy(CRC32_t propHash);

	std::vector< RecvTable* > m_tables;
	std::vector< CHookedProp* > m_HookedProps;

	struct sOffsets
	{
		uint32_t m_lifeState,
			m_fFlags,
			m_fEffects,
			m_iHealth,
			m_aimPunchAngle,
			m_aimPunchAngleVel,
			m_viewPunchAngle,
			m_iTeamNum,
			m_nTickBase,
			m_iClip1,
			m_iMaxClip1,
			m_flSimulationTime,
			m_flOldSimulationTime,
			m_flCycle,
			m_flPlaybackRate,
			m_flPoseParameter,
			m_flOldPoseParameter,
			m_nSequence,
			m_flNextAttack,
			m_flNextPrimaryAttack,
			m_flNextSecondaryAttack,
			m_fLastShotTime,
			m_vecVelocity,
			m_vecBaseVelocity,
			m_vecViewOffset,
			m_vecOrigin,
			m_hViewModel,
			m_nModelIndex,
			m_angEyeAngles,
			m_bIsDefusing,
			m_bHasDefuser,
			m_hActiveWeapon,
			m_nHitboxSet,
			m_clrRender,
			m_hOwnerEntity,
			m_vecMins,
			m_vecMaxs,
			m_angRotation,
			m_bSpotted,
			m_bIsScoped,
			m_iFOV,
			m_iShotsFired,
			m_bClientSideAnimation,
			m_zoomLevel,
			m_flRecoilIndex,
			m_hMyWeapons,
			m_hMyWearables,
			m_bInitialized,
			m_hOuter,
			m_iAccountID,
			m_OriginalOwnerXuidLow,
			m_OriginalOwnerXuidHigh,
			m_nFallbackPaintKit,
			m_flFallbackWear,
			m_nFallbackStatTrak,
			m_nFallbackSeed,
			m_iItemIDHigh,
			m_iItemIDLow,
			m_iEntityQuality,
			m_szCustomName,
			m_iItemDefinitionIndex,
			m_iViewModelIndex,
			m_iWorldModelIndex,
			m_flFlashDuration,
			m_iCompetitiveRanking,
			m_iCompetitiveWins,
			m_flC4Blow,
			m_bBombTicking,
			m_bBombDefused,
			m_flDefuseCountDown,
			m_hCombatWeaponParent,
			BaseWeaponWorldModel_ModelIndex,
			PredictedViewModel_Weapon,
			PredictedViewModel_Sequence,
			m_bHasHelmet,
			m_ArmorValue,
			m_bHasHeavyArmor,
			m_Collision,
			m_CollisionGroup,
			m_flLowerBodyYawTarget,
			m_bDidSmokeEffect,
			m_flFlashMaxAlpha,
			CurrentCommand,
			EntityGlowIndex,
			WritableBones,
			m_bPinPulled,
			m_fThrowTime,
			m_flPostponeFireReadyTime,
			m_fAccuracyPenalty,
			m_hObserverTarget,
			m_bGunGameImmunity,
			m_bShouldGlow,
			MuzzleFlashParity,
			m_flDuckAmount,
			m_vecForce,
			m_vecRagdollVelocity,
			m_bIsBroken,
			m_bClientSideRagdoll,
			m_hVehicle,
			m_hGroundEntity,
			m_flStepSize,
			m_bAllowAutoMovement,
			m_nWaterLevel,
			m_flFallVelocity,
			m_flVelocityModifier,
			m_flDuckSpeed,
			deadflag,
			m_flThirdpersonRecoil,
			m_hViewEntity,
			m_flLaggedMovementValue,
			m_vecLadderNormal,
			m_iObserverMode,
			moveparent,
			m_flFriction,
			m_flMaxSpeed,
			m_bIsPlayerGhost,
			m_bCanMoveDuringFreezePeriod,
			m_bIsGrabbingHostage,
			m_flGroundAccelLinearFracLastTime,
			m_iPlayerState,
			m_iMoveState,
			m_flStamina,
			m_bIsWalking,
			m_bDuckOverride,
			m_Local,
			m_WeaponMode,
			m_iClip2,
			m_iBurstShotsRemaining,
			m_fNextBurstShot,
			m_bStartedArming,
			m_SurvivalRules,
			m_bFreezePeriod,
			m_nIsAutoMounting,
			m_vecAutomoveTargetEnd,
			m_flAutoMoveTargetTime,
			m_flAutoMoveStartTime,
			m_iBlockingUseActionInProgress,
			m_flHealthShotBoostExpirationTime,
			m_flTimeOfLastInjury,
			m_vecAngVelocity,
			m_flElasticity,
			m_flMaxFallVelocity,
			m_bIsSpawnRappelling,
			m_fMolotovDamageTime,
			m_szClan,
			m_iPrimaryReserveAmmoCount,
			m_bWaitForNoAttack,
			m_bKilledByTaser,
			m_nViewModelIndex,
			m_hBombDefuser,
			m_hWeaponWorldModel,
			m_bStrafing,
			m_fireCount,
			m_nRelativeDirectionOfLastInjury,
			m_LastHitGroup,
			m_nForceBone,
			m_vphysicsCollisionState,
			m_bIsValveDS,
			m_nExplodeEffectTickBegin,
			m_vecSpecifiedSurroundingMins,
			m_vecSpecifiedSurroundingMaxs,
			m_bFireIsBurning,
			m_fireXDelta,
			m_fireYDelta,
			m_fireZDelta;
	} Offsets;

	enum ClassID : int
	{
		_CTestTraceline = 223,
		_CTEWorldDecal = 224,
		_CTESpriteSpray = 221,
		_CTESprite = 220,
		_CTESparks = 219,
		_CTESmoke = 218,
		_CTEShowLine = 216,
		_CTEProjectedDecal = 213,
		_CFEPlayerDecal = 71,
		_CTEPlayerDecal = 212,
		_CTEPhysicsProp = 209,
		_CTEParticleSystem = 208,
		_CTEMuzzleFlash = 207,
		_CTELargeFunnel = 205,
		_CTEKillPlayerAttachments = 204,
		_CTEImpact = 203,
		_CTEGlowSprite = 202,
		_CTEShatterSurface = 215,
		_CTEFootprintDecal = 199,
		_CTEFizz = 198,
		_CTEExplosion = 196,
		_CTEEnergySplash = 195,
		_CTEEffectDispatch = 194,
		_CTEDynamicLight = 193,
		_CTEDecal = 191,
		_CTEClientProjectile = 190,
		_CTEBubbleTrail = 189,
		_CTEBubbles = 188,
		_CTEBSPDecal = 187,
		_CTEBreakModel = 186,
		_CTEBloodStream = 185,
		_CTEBloodSprite = 184,
		_CTEBeamSpline = 183,
		_CTEBeamRingPoint = 182,
		_CTEBeamRing = 181,
		_CTEBeamPoints = 180,
		_CTEBeamLaser = 179,
		_CTEBeamFollow = 178,
		_CTEBeamEnts = 177,
		_CTEBeamEntPoint = 176,
		_CTEBaseBeam = 175,
		_CTEArmorRicochet = 174,
		_CTEMetalSparks = 206,
		_CSteamJet = 167,
		_CSmokeStack = 157,
		_DustTrail = 275,
		_CFireTrail = 74,
		_SporeTrail = 281,
		_SporeExplosion = 280,
		_RocketTrail = 278,
		_SmokeTrail = 279,
		_CPropVehicleDriveable = 144,
		_ParticleSmokeGrenade = 277,
		_CParticleFire = 116,
		_MovieExplosion = 276,
		_CTEGaussExplosion = 201,
		_CEnvQuadraticBeam = 66,
		_CEmbers = 55,
		_CEnvWind = 70,
		_CPrecipitation = 137,
		_CPrecipitationBlocker = 138,
		_CBaseTempEntity = 18,
		_NextBotCombatCharacter = 0,
		_CEconWearable = 54,
		_CBaseAttributableItem = 4,
		_CEconEntity = 53,
		_CWeaponXM1014 = 272,
		_CWeaponTaser = 267,
		_CTablet = 171,
		_CSnowball = 158,
		_CSmokeGrenade = 155,
		_CWeaponShield = 265,
		_CWeaponSG552 = 263,
		_CSensorGrenade = 151,
		_CWeaponSawedoff = 259,
		_CWeaponNOVA = 255,
		_CIncendiaryGrenade = 99,
		_CMolotovGrenade = 112,
		_CMelee = 111,
		_CWeaponM3 = 247,
		_CKnifeGG = 108,
		_CKnife = 107,
		_CHEGrenade = 96,
		_CFlashbang = 77,
		_CFists = 76,
		_CWeaponElite = 238,
		_CDecoyGrenade = 47,
		_CDEagle = 46,
		_CWeaponUSP = 271,
		_CWeaponM249 = 246,
		_CWeaponUMP45 = 270,
		_CWeaponTMP = 269,
		_CWeaponTec9 = 268,
		_CWeaponSSG08 = 266,
		_CWeaponSG556 = 264,
		_CWeaponSG550 = 262,
		_CWeaponScout = 261,
		_CWeaponSCAR20 = 260,
		_CSCAR17 = 149,
		_CWeaponP90 = 258,
		_CWeaponP250 = 257,
		_CWeaponP228 = 256,
		_CWeaponNegev = 254,
		_CWeaponMP9 = 253,
		_CWeaponMP7 = 252,
		_CWeaponMP5Navy = 251,
		_CWeaponMag7 = 250,
		_CWeaponMAC10 = 249,
		_CWeaponM4A1 = 248,
		_CWeaponHKP2000 = 245,
		_CWeaponGlock = 244,
		_CWeaponGalilAR = 243,
		_CWeaponGalil = 242,
		_CWeaponG3SG1 = 241,
		_CWeaponFiveSeven = 240,
		_CWeaponFamas = 239,
		_CWeaponBizon = 234,
		_CWeaponAWP = 232,
		_CWeaponAug = 231,
		_CAK47 = 1,
		_CWeaponCSBaseGun = 236,
		_CWeaponCSBase = 235,
		_CC4 = 34,
		_CBumpMine = 32,
		_CBumpMineProjectile = 33,
		_CBreachCharge = 28,
		_CBreachChargeProjectile = 29,
		_CWeaponBaseItem = 233,
		_CBaseCSGrenade = 8,
		_CSnowballProjectile = 160,
		_CSnowballPile = 159,
		_CSmokeGrenadeProjectile = 156,
		_CSensorGrenadeProjectile = 152,
		_CMolotovProjectile = 113,
		_CItem_Healthshot = 104,
		_CItemDogtags = 106,
		_CDecoyProjectile = 48,
		_CPhysPropRadarJammer = 126,
		_CPhysPropWeaponUpgrade = 127,
		_CPhysPropAmmoBox = 124,
		_CPhysPropLootCrate = 125,
		_CItemCash = 105,
		_CEnvGasCanister = 63,
		_CDronegun = 50,
		_CParadropChopper = 115,
		_CSurvivalSpawnChopper = 170,
		_CBRC4Target = 27,
		_CInfoMapRegion = 102,
		_CFireCrackerBlast = 72,
		_CInferno = 100,
		_CChicken = 36,
		_CDrone = 49,
		_CFootstepControl = 79,
		_CCSGameRulesProxy = 39,
		_CWeaponCubemap = 0,
		_CWeaponCycler = 237,
		_CTEPlantBomb = 210,
		_CTEFireBullets = 197,
		_CTERadioIcon = 214,
		_CPlantedC4 = 128,
		_CCSTeam = 43,
		_CCSPlayerResource = 41,
		_CCSPlayer = 40,
		_CPlayerPing = 130,
		_CCSRagdoll = 42,
		_CTEPlayerAnimEvent = 211,
		_CHostage = 97,
		_CHostageCarriableProp = 98,
		_CBaseCSGrenadeProjectile = 9,
		_CHandleTest = 95,
		_CTeamplayRoundBasedRulesProxy = 173,
		_CSpriteTrail = 165,
		_CSpriteOriented = 164,
		_CSprite = 163,
		_CRagdollPropAttached = 147,
		_CRagdollProp = 146,
		_CPropCounter = 141,
		_CPredictedViewModel = 139,
		_CPoseController = 135,
		_CGrassBurn = 94,
		_CGameRulesProxy = 93,
		_CInfoLadderDismount = 101,
		_CFuncLadder = 85,
		_CTEFoundryHelpers = 200,
		_CEnvDetailController = 61,
		_CDangerZone = 44,
		_CDangerZoneController = 45,
		_CWorldVguiText = 274,
		_CWorld = 273,
		_CWaterLODControl = 230,
		_CWaterBullet = 229,
		_CVoteController = 228,
		_CVGuiScreen = 227,
		_CPropJeep = 143,
		_CPropVehicleChoreoGeneric = 0,
		_CTriggerSoundOperator = 226,
		_CBaseVPhysicsTrigger = 22,
		_CTriggerPlayerMovement = 225,
		_CBaseTrigger = 20,
		_CTest_ProxyToggle_Networkable = 222,
		_CTesla = 217,
		_CBaseTeamObjectiveResource = 17,
		_CTeam = 172,
		_CSunlightShadowControl = 169,
		_CSun = 168,
		_CParticlePerformanceMonitor = 117,
		_CSpotlightEnd = 162,
		_CSpatialEntity = 161,
		_CSlideshowDisplay = 154,
		_CShadowControl = 153,
		_CSceneEntity = 150,
		_CRopeKeyframe = 148,
		_CRagdollManager = 145,
		_CPhysicsPropMultiplayer = 122,
		_CPhysBoxMultiplayer = 120,
		_CPropDoorRotating = 142,
		_CBasePropDoor = 16,
		_CDynamicProp = 52,
		_CProp_Hallucination = 140,
		_CPostProcessController = 136,
		_CPointWorldText = 134,
		_CPointCommentaryNode = 133,
		_CPointCamera = 132,
		_CPlayerResource = 131,
		_CPlasma = 129,
		_CPhysMagnet = 123,
		_CPhysicsProp = 121,
		_CStatueProp = 166,
		_CPhysBox = 119,
		_CParticleSystem = 118,
		_CMovieDisplay = 114,
		_CMaterialModifyControl = 110,
		_CLightGlow = 109,
		_CItemAssaultSuitUseable = 0,
		_CItem = 0,
		_CInfoOverlayAccessor = 103,
		_CFuncTrackTrain = 92,
		_CFuncSmokeVolume = 91,
		_CFuncRotating = 90,
		_CFuncReflectiveGlass = 89,
		_CFuncOccluder = 88,
		_CFuncMoveLinear = 87,
		_CFuncMonitor = 86,
		_CFunc_LOD = 81,
		_CTEDust = 192,
		_CFunc_Dust = 80,
		_CFuncConveyor = 84,
		_CFuncBrush = 83,
		_CBreakableSurface = 31,
		_CFuncAreaPortalWindow = 82,
		_CFish = 75,
		_CFireSmoke = 73,
		_CEnvTonemapController = 69,
		_CEnvScreenEffect = 67,
		_CEnvScreenOverlay = 68,
		_CEnvProjectedTexture = 65,
		_CEnvParticleScript = 64,
		_CFogController = 78,
		_CEnvDOFController = 62,
		_CCascadeLight = 35,
		_CEnvAmbientLight = 60,
		_CEntityParticleTrail = 59,
		_CEntityFreezing = 58,
		_CEntityFlame = 57,
		_CEntityDissolve = 56,
		_CDynamicLight = 51,
		_CColorCorrectionVolume = 38,
		_CColorCorrection = 37,
		_CBreakableProp = 30,
		_CBeamSpotlight = 25,
		_CBaseButton = 5,
		_CBaseToggle = 19,
		_CBasePlayer = 15,
		_CBaseFlex = 12,
		_CBaseEntity = 11,
		_CBaseDoor = 10,
		_CBaseCombatCharacter = 6,
		_CBaseAnimatingOverlay = 3,
		_CBoneFollower = 26,
		_CBaseAnimating = 2,
		_CAI_BaseNPC = 0,
		_CBeam = 24,
		_CBaseViewModel = 21,
		_CBaseParticleEntity = 14,
		_CBaseGrenade = 13,
		_CBaseCombatWeapon = 7,
		_CBaseWeaponWorldModel = 23
	};
};

enum StaticOffsetName
{
	INVALID_STATIC_OFFSET = -1,

	_IsPlayer,
	_StriderMuzzleEffect,
	_GunshipMuzzleEffect,
	_CS_Bloodspray,
	_Tesla,
	_ViewRender,
	_ParseEventDelta,
	_GetPlayerResource,
	_CalcPlayerView,
	_InvalidatePhysicsRecursive,
	_GetShotgunSpread,
	_NoClipEnabled,
	_CanUseFastPath,
	_SetAbsAngles,
	_SetAbsAnglesDirect,
	_AbsVelocityDirect,
	_SetAbsOrigin,
	_AbsOriginDirect,
	_OverridePostProcessingDisable,
	_LastOcclusionCheckTime,
	_InvalidateBoneCache,
	_EntityGlowIndex, // Outdated
	_UpdateClientSideAnimation,
	_LastOutgoingCommand,
	_DeltaTick,
	_pClientState,
	_FinishDrawing,
	_StartDrawing,
	_UTIL_TraceLineIgnoreTwoEntities,
	_UTIL_ClipTraceToPlayers,
	_UTIL_ClipTraceToPlayers2,
	_IsEntityBreakable,
	_IsBaseCombatWeaponIndex,
	_LobbyScreen_Scaleform,
	_ChangeTeammateColor,
	_IsReady,
	_AcceptMatch,
	_GetWeaponDataIndex,
	_LineGoesThroughSmoke,
	_RevealRanks,
	_Cone,
	_Spread,
	_UpdateAccuracyPenalty,
	_GameResources,
	_GlowObjectManager,
	_KeyValues_LoadFromBuffer,
	_ChangeClantag,
	_KeyValues_Constructor,
	_EyeAnglesVMT,
	_LocalAngles,
	//_DeadFlag,
	//_RenderAngles,
	_CalcAbsoluteVelocity,
	_CalcAbsolutePosition,
	_OldOrigin,
	_LocalOrigin,
	_WorldSpaceCenter,
	_MoveType,
	_MoveCollide,
	_m_iEFlags,
	_InThreadedBoneSetup,
	_m_BoneSetupLock,
	_m_bIsToolRecording,
	_m_EntClientFlags,
	_GetPredictable,
	_m_pIK_offset,
	_m_flLastBoneSetupTime,
	_LastBoneChangedTime_Offset,
	_m_iPrevBoneMask,
	_m_iAccumulatedBoneMask,
	_LastSetupBonesFrameCount,
	_GetSolid,
	_m_iMostRecentModelBoneCounter,
	_g_iModelBoneCounter,
	_PhysicsSolidMaskForEntityVMT,
	_SetDormant,
	_LockStudioHdr,
	_GetPunchAngleVMT,
	_SetPunchAngleVMT,
	_SpawnTime,
	_m_dwObserverTargetVMT,
	_ReevaluateAnimLod,
	_AllowBoneAccessForViewModels,
	_AllowBoneAccessForNormalModels,
	_IsViewModel,
	_BoneAccessor,
	_m_pStudioHdr,
	_MarkForThreadedBoneSetup,
	_GetClassnameVMT,
	_SequencesAvailableVMT,
	_SequencesAvailableCall,
	_CreateIK,
	_Teleported,
	_IKInit,
	_ShouldSkipAnimFrame,
	_StandardBlendingRules,
	_BuildTransformations,
	_m_pRagdoll,
	_UpdateIKLocks,
	_CalculateIKLocks,
	_ControlMouth,
	_Wrap_SolveDependencies,
	_UpdateTargets,
	_AttachmentHelper,
	_UnknownOffset1,
	_DoExtraBoneProcessing,
	_m_CachedBoneData,
	_SplitScreenPlayerSlot,
	_g_VecRenderOrigin,
	_CacheVehicleView,
	_Weapon_ShootPosition,
	_Weapon_ShootPosition_Base,
	_GetClientVehicle,
	_IsInAVehicle,
	_VehicleViewOrigin,
	_GetObserverModeVMT,
	_m_bUseAnimationEyeOffset,
	_BaseCalcView,
	_m_bThirdPerson,
	_GetAliveVMT,
	_LookupBone,
	_m_rgflCoordinateFrame,
	_GameRules,
	_C_CSPlayer_CalcView,
	_PlayFootstepSound,
	_Simulate,
	_PhysicsSimulate,
	_ShouldInterpolate,
	_LookupPoseParameter,
	_GetPoseParameterRange,
	_m_flEncodedController,
	_SetGroundEntity,
	_SurfaceFriction,
	_m_chTextureType,
	_m_flWaterJumpTime,
	_m_nWaterType,
	_m_hPlayerAnimState,
	_GetCompetitiveMatchID,
	_IsPaused,
	_host_tickcount,
	_predictionrandomseed,
	_predictionplayer,
	_MD5PseudoRandom,
	_IsPausedExtrapolateReturnAddress,
	_g_pClientLeafSystem,
	_StandardFilterRulesCallOne,
	_SetAbsVelocity,
	_UpdateClientSideAnimationFunction,
	_frametime1,
	_frametime2,
	_frametime3,
	_ClientSideAnimationList,
	_EnableInvalidateBoneCache,
	_RadarBase,
	_IsEntityBreakable_FirstCall_Arg1,
	_IsEntityBreakable_FirstCall_Arg2,
	_IsEntityBreakable_SecondCall_Arg1,
	_IsEntityBreakable_SecondCall_Arg2,
	_IsEntityBreakable_ActualCall,
	_WeaponScriptPointer,
	_WeaponScriptPointerCall,
	_GetWeaponSystem,
	_Input,
	_oSetupBones,
	_host_interval_per_tick,
	_GetSequenceName,
	_GetSequenceActivity,
	_GetSequenceActivityNameForModel,
	_ActivityListNameForIndex,
	_pSeqDesc,
	_svtable,
	_hoststate,
	_CL_Move,
	_WriteUserCmd,
	_pCommands,
	_SetupVelocityReturnAddress,
	_CL_FireEvents,
	_AbsRecomputationEnabled,
	_AbsQueriesValid,
	_PlayerResource,
	_ResetAnimationState,
	_ProcessOnDataChangedEvents,
	_pTempEnts,
	_pBeams,
	_Net_Time,
	_Receivetable_Decode,
	_SequenceDuration,
	_GetFirstSequenceAnimTag,
	_GameTypes,
	_SurpressLadderChecks,
	_s_nTraceFilterCount,
	_s_TraceFilter,
	_IsCarryingHostage,
	_EyeVectors,
	_CreateStuckTable,
	_rgv3tStuckTable,
	_LevelShutdownPreEntity,
	_LevelInitPreEntity,
	_RenderViewTrap1,
	_RenderViewTrap2,
	_CHLClientTrap2,
	_EngineClientTrap2,
	_LevelShutdownCHLClientTrap,
	_LevelShutdownEngineClientTrap,
	_LocalPlayerEntityTrap,
	_ModelRenderGlow,
	_ModelRenderGlowTrap,
	_ModelRenderGlowTrap2,
	_PredictionUpdateHLTVCall,
	_GetBonePosition,
	_GetNetChannelInfoCvarCheck,
	_IClientEntityList,
	_RenderBeams,
	_DirectXPrePointer,
	_TE_EffectDispatch,
	_MoveHelperClient,
	_FormatViewModelAttachment,
	_GroupStudioHdr,
	_Attachments,
	_GetAbsAnglesVMT,
	_ShouldCollide,
	_IsTransparent,
	_EstimateAbsVelocity,
	_GetBaseAnimating,
	_AnimOverlay,
	_GetAbsOriginVMT,
	_GetPlayerMaxSpeedVMT,
	_PlayClientJumpSound,
	_PlayClientUnknownSound,
	_DoAnimationEvent1, //this is actually the BaseAnimState vtable
	_DoAnimationEvent2,
	_ResetLatched,
	_GetDefaultFOV,
	_UpdateStepSound,
	_GetActiveCSWeapon,
	_IsTaunting,
	_IsInThirdPersonTaunt,
	_IsBotPlayerResourceOffset,
	_StepSoundTime,
	_m_StuckLast,
	_m_VPhysicsObject,
	_m_vecWaterJumpVel,
	_m_flSwimSoundTime,
	_m_SurfaceProps,
	_m_iSurfaceData,
	_m_flTimeNotOnLadder,
	_m_vecDuckingOrigin,
	_m_duckUntilOnGround,
	_m_bServerSideJumpAnimation,
	_m_bSlowMovement,
	_m_bHasWalkMovedSinceLastJump,
	_m_pPredictionPlayer,
	_m_pPlayerCommand,
	_m_pCurrentCommand,
	_FreeEntityBaselines,
	_GetWeaponMaxSpeedVMT,
	_GetWeaponMaxSpeed2VMT,
	_WeaponOnJumpVMT,
	_WeaponGetZoomLevelVMT,
	_WeaponGetNumZoomLevelsVMT,
	_m_bInReload,
	_ParallelProcess,
	_ParallelQueueVT1,
	_ParallelQueueVT2,
	_DoExecute,
	_AbortJob,
	_ShotgunSpread,
	_GetMaxHealthVMT,
	_TakeDamage,
	_SurvivalCalcView,
	_SurvivalModeOrigin,
	_UnknownSurvivalBool,
	_GetHudPlayer,
	_UselessCalcViewSurvivalBool,
	_ThirdPersonSwitchVMT,
	_CanWaterJumpVMT,
	_GetEncumberance,
	_RenderHandle,
	_TE_FireBullets,
	_UserCmd_GetChecksum,
	_CL_SendMove,
	_SplitScreenMgr,
	_CL_SendMove_DefaultMemory,
	_CCLCMsg_Move_vtable1,
	_CCLCMsg_Move_vtable2,
	_CCLCMsg_Move_UnknownCall,
	_CCLCMsg_Move_Deconstructor_relative,
	_UnknownAnimationFloat,
	_UnknownAnimationCall,
	_PlayerFilterRules,
	_GetViewOffsetVMT,
	_GetSolidFlagsVMT,
	_PhysicsCheckWaterTransition,
	_PhysicsPushEntity,
	_GetUnknownEntity,
	_DesiredCollisionGroup,
	_EyePositionVMT,
	_MaintainSequenceTransitionsReturnAddress,
	_AccumulateLayersVMT,
	_boneFlagsOffset,
	_ClientEntityListArray,
	_HasC4,
	_FlashbangTime,
	_CacheSequences,
	_SetupLean,
	_SetupAimMatrix,
	_SetupWeaponAction,
	_SetupMovement,
	_SetSequenceVMT,
	_WeaponHasBurstVMT,
	_m_Activity,
	_IsFullAutoVMT,
	_GetCycleTimeVMT,
	_GetMaxClip1VMT,
	_GetReserveAmmoCount,
	_m_bFireOnEmpty,
	_m_bIsCustomPlayer,
	_UpdateViewModelAddonsSub,
	_UpdateViewModelAddonsVT,
	_ViewModelLabelHandle,
	_ViewModelStatTrackHandle,
	_RemoveViewModelStickers,
	_RemoveViewModelArmModels,
	_RemoveEntity,
	_OnLatchInterpolatedVariables,
	_FrameAdvance,
	_HandleTaserAnimation,
	_GetViewModel,
	_IsBaseCombatWeapon,
	_WeaponGetZoomFOVVMT,
	_FileSystemStringVMT,
	_SetSky,
	_FindElement,
	_Hud,
	_ClearDeathNotices,
	_IsInIronsight,
	_SetupVelocity,
	_UpdatePartition,
	_CAM_ToThirdPerson,
	_CAM_ToFirstPerson,
	_IronSightController,
	_OnLand,
	_NetSetConVar_Constructor,
	_NetSetConVar_Init,
	_NetSetConVar_Destructor,
	_NetIsMultiplayer,
	_CNetMsg_Tick_Constructor,
	_CNetMsg_Tick_Destructor,
	_CNetMsg_Tick_Setup,
	_CNetMsg_Tick_MiscAdr,
	_NET_SendPacketSig,
	_ViewMatrixPtr,
	_GetWeaponMaxSpeed3VMT,
	_LookupSequence,
	_GetWeaponMoveAnimation,
	_GetSequenceLinearMotion,
	_UpdateLayerOrderPreset,
	_UnknownSetupMovementFloat,
	_GetLayerIdealWeightFromSeqCycle,
	_GetSequenceCycleRate,
	_GetLayerSequenceCycleRate,
	_GetAnySequenceAnimTag,
	_GetPoseParameter,
	_FindKey,
	_GetInt,
	_CActivityToSequenceMapping_Reinitialize,
	_GetActivityListVersion,
	_g_nActivityListVersion,
	_IndexModelSequences,
	_g_ActivityModifiersTable,
	_m_afButtonLast,
	_m_nButtons,
	_m_afButtonPressed,
	_m_afButtonReleased,
	_symbolsInitialized,
	_s_pSymbolTable,
	_AddString,
	_FindString,
	_FindFieldByName,
	_GetPredDescMap,
	_GetPredictedFrame,
	_g_Predictables,
	_touchStamp,
	_doLocalPlayerPrePrediction,
	_PhysicsCheckForEntityUntouch,
	_PhysicsTouchTriggers,
	_MoveToLastReceivedPosition,
	_UnknownEntityPredictionBool,
	_StorePredictionResults,
	_m_nFinalPredictedTick,
	_m_pFirstPredictedFrame,
	_VPhysicsCompensateForPredictionErrorsVMT,
	_ComputeFirstCommandToExecute,
	_physenv,
	_g_BoneAccessStack,
	_g_BoneAcessBase,
	_RestoreOriginalEntityState,
	_RunSimulation,
	_RestoreEntityToPredictedFrame,
	_ShiftIntermediateDataForward,
	_ShiftFirstPredictedIntermediateDataForward,
	_SaveData,
	_ListLeavesInBox_ReturnAddrBytes,
	_IsVoiceRecording_Ptr,
	_Interpolate,
	_InterpolationVarMap,
	_DelayUnscope,
	_SelectWeightedSequence,
	_InitPose,
	_AccumulatePose,
	_CalcAutoplaySequences,
	_CalcBoneAdj,
	_IKConstruct,
	_IKDestruct,
	_ParticleProp,
	_SUB_Remove,
	_Spawn,
	_Precache,
	MAX_STATIC_OFFSETS
};

class CModule;

class StaticOffset
{
public:
	StaticOffsetName GetName() const { return Name; }
	StaticOffsetName GetIndex() const { return Name; }
	uint32_t GetOffset() const;
	template <class T, typename D> T GetOffsetByType(D addfrom = 0) { return (T)(GetOffset() + (uint32_t)addfrom); }
	template <class T> T GetOffsetByType() { return (T)GetOffset(); }
	template <class T, typename D> T GetVFuncByType(D vTable) { return (*(T**)(void*)vTable)[GetOffset()]; }
	bool HasBeenFound() const { return IsFound; }
	bool IsHardCoded() const { return DoNotSigScan; }
	size_t GetSignatureLength() const { return SignatureLength; }
	char* GetSignature() const { return Signature; }
	HANDLE GetHandle() const { return Handle; }
	bool ShouldDereference() const { return DereferenceTimes > 0; }
	bool ShouldDivideAfterDereference() const { return DivideAfterDereference != 0; }
	bool ShouldAddAtEnd() const { return AddAtEnd != 0; }
	int GetDereferenceTimes() const { return DereferenceTimes; }
	int GetAddBeforeDereference() const { return AddBeforeDereference; }
	int GetDivideAfterDereference() const { return DivideAfterDereference; }
	int GetAddAtEnd() const { return AddAtEnd; }
	int GetSizeOfValue() const { return SizeOfValue; }
	uint32_t GetRawAddress() const { return RawAddress ^ GetDecryptionKey(); }
	bool ShouldUseRawHandle() const { return UsesRawHandle; }
	uint32_t GetDecryptionKey() const { return DecryptionKey; }
	void GenerateDecryptionKey() { DecryptionKey = (uint32_t)__rdtsc(); }

	void SetFound(bool found) { IsFound = found; }
	void SetOffset(uint32_t offset) { Offset = offset; }
	void SetRawAddress(uint32_t adr) { RawAddress = adr; }

	void ClearIdentifiableData()
	{
		const uint32_t _Filler = (uint32_t)__rdtsc();
		Name = INVALID_STATIC_OFFSET;
		SignatureLength = 0;

#if defined MUTINY_FRAMEWORK && !defined (DEVELOPER)
		if (!IsHardCoded() && Signature != nullptr)
		{
			memset(Signature, _Filler, strlen(Signature) + 1);
			delete[] Signature;
		}
#endif
		Signature = nullptr;
		DereferenceTimes = _Filler;
		DivideAfterDereference = _Filler;
		AddAtEnd = _Filler;
		AddBeforeDereference = _Filler;
		SizeOfValue = _Filler;
		RawAddress = _Filler;
		Handle = (HANDLE)_Filler;
		UsesRawHandle = false;
		IsFound = false;
	}
	StaticOffset::StaticOffset()
	{
		Name = INVALID_STATIC_OFFSET;
		IsFound = false;
		DecryptionKey = UINT32_MAX;
	};
	StaticOffset::StaticOffset(StaticOffsetName name, size_t signaturelength, char* signature, HANDLE dllhandle, int dereferencetimes = 0, int addbeforedereference = 0, int divideafterdereference = 0, int addatend = 0, int sizeofvalue = 4, uint32_t decryptionkey = UINT32_MAX)
	{
		Name = name;
		Offset = 0;
		IsFound = false;
		DoNotSigScan = false;
		UsesRawHandle = false;
		SignatureLength = signaturelength;
		Signature = signature;
		Handle = dllhandle;
		DereferenceTimes = dereferencetimes;
		AddBeforeDereference = addbeforedereference;
		DivideAfterDereference = divideafterdereference;
		AddAtEnd = addatend;
		SizeOfValue = sizeofvalue;
		RawAddress = 0;
		DecryptionKey = decryptionkey;
		if (decryptionkey == UINT32_MAX)
			GenerateDecryptionKey();
	}
	StaticOffset::StaticOffset(StaticOffsetName name, uint32_t offset, uint32_t decryptionkey = UINT32_MAX)
	{
		Name = name;
		Offset = offset;
		IsFound = true;
		DoNotSigScan = true;
		UsesRawHandle = false;
		SignatureLength = 0;
		Signature = 0;
		DecryptionKey = decryptionkey;
		if (decryptionkey == UINT32_MAX)
			GenerateDecryptionKey();
	}
	StaticOffset::StaticOffset(StaticOffsetName name, uint32_t rawadr, HANDLE handle, int dereferencetimes, int addbeforedereference, int divideafterdereference, int addatend, int sizeofvalue, uint32_t decryptionkey = UINT32_MAX)
	{
		Name = name;
		Offset = 0;
		IsFound = true;
		DoNotSigScan = false;
		UsesRawHandle = true;
		RawAddress = rawadr;
		Handle = handle;
		DereferenceTimes = dereferencetimes;
		AddBeforeDereference = addbeforedereference;
		DivideAfterDereference = divideafterdereference;
		AddAtEnd = addatend;
		SizeOfValue = sizeofvalue;
		DecryptionKey = decryptionkey;
		if (decryptionkey == UINT32_MAX)
			GenerateDecryptionKey();
	}
private:
	StaticOffsetName Name;
public:
	uint32_t Offset;
	uint32_t DecryptionKey;
private:
	bool IsFound;
	bool DoNotSigScan;
	bool UsesRawHandle;
	size_t SignatureLength;
	char* Signature;
	HANDLE Handle;

	int DereferenceTimes;
	int AddBeforeDereference;
	int DivideAfterDereference;
	int AddAtEnd;
	int SizeOfValue;
	uint32_t RawAddress;
};

class CStaticOffsets
{
public:
	enum API : char
	{
		API_VERSION = 4
	};

	enum CachedStaticOffsetsHandle
	{
		CACHE_CLIENTDLL = 0,
		CACHE_ENGINEDLL,
		CACHE_VGUIMATSURFACE,
		CACHE_SHADERAPIDX9,
		CACHE_SERVERDLL,
		CACHE_UNKNOWN
	};

	CStaticOffsets();
	void AddAllOffsets();
	void UpdateAllOffsets();
	void HideStaticOffsets();
	bool ReadOffsetsFromFile();
	void DumpOffsetsToFile();
	StaticOffset &GetOffset(StaticOffsetName name);
	uint32_t GetOffsetValue(StaticOffsetName name);
	template <class T, typename D>
	T GetOffsetValueByType(const StaticOffsetName name, D addfrom)
	{
		return (T)GetOffset(name).GetOffsetByType<T>(addfrom);
	}

	template <class T>
	T GetOffsetValueByType(const StaticOffsetName name)
	{
		return (T)GetOffset(name).GetOffsetByType<T>();
	}

	template <class T, typename D>
	T GetVFuncByType(const StaticOffsetName name, D vTable)
	{
		return (T)GetOffset(name).GetVFuncByType<T, D>(vTable);
	}

private:
	void AddOffset(const StaticOffset& fulloffset);
	CachedStaticOffsetsHandle HandleToCachedHandle(HANDLE handle);
	HANDLE HandleToCachedHandle(CachedStaticOffsetsHandle dll);
public:
	std::array<StaticOffset, MAX_STATIC_OFFSETS> Offsets;
private:
	bool m_bAlreadyAdded;
};


extern CNetworkedVariables g_NetworkedVariables;
extern CStaticOffsets StaticOffsets;