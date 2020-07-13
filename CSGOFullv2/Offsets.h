#ifndef OFFSETS_H
#define OFFSETS_H
#pragma once

extern unsigned long RadarBaseAdr;
extern unsigned long GlowObjectManagerAdr;

#define localanglesoffset 0xC4

#define localoriginoffset 0xAC
#define m_nWaterType 0xE8
#define MAX_CSGO_POSE_PARAMS 20//24
#define MAX_CSGO_ANIM_LAYERS 13

#if 1
#define m_vecVehicleViewOrigin 0x3430 //F3  0F  7E  87  ??  ??  00  00  66  0F  D6  00 + 4
#define m_dwSplitScreenPlayerSlot 0x35F0 //8B  87  ??  ??  00  00  5F  8D  0C  40 + 2
#define m_dwGetClientVehicle (0x188 / 4) //8B  01  FF  90  ??  ??  00  00  85  C0  74  10  8B  CE + 4
#define setlocaloriginindex (0x3C4 / 4)

#define m_dwViewEntity 0x3318
#define m_rgflCoordinateFrame 0x440 //F3  0F  59  C4  F3  0F  58  D0  F3  0F  10  83  ??  ??  00  00 + 0xC
#define calcview_index (0x438/ 4)
#define weapon_shootposition_index 277
#define weapon_shootposition_base_index 652
#define m_bUseAnimationEyeOffset 0x3A05 //55  8B  EC  56  8B  75  08  57  8B  F9  56  8B  07  FF  90  ??  ??  00  00  80  BF  ??  ??  00  00  00   + 21 DEC
#define m_RefEHandle 0x28C //0x288
#define m_bStartedArming_ 0x3390
#define m_bCanMoveDuringFreezePeriod_ 0x38F8
#define m_flStepSoundTime 0x320C //F3  0F  5F  05  ??  ??  ??  ??  57  6A  00  C7  80  ??  ??  00  00  00  00  C8  43  + 13 DEC
#define m_flJumpTime 0x3000 //C7  80  ??  ??  ??  ??  FE  01  00  00  8B  46  04  + 2 DEC
#define m_flGravity 0xDC
#define m_flGroundAccelLinearFracLastTime_ 0xA2E0
#define m_iPlayerState_ 0x38A4
#define m_vecWaterJumpVel 0x33E8 //F3  0F  5C  C1  F3  0F  11  80  ??  ??  00  00  F3  0F  10  4D  90 + 8 DEC
#define m_dwVPhysicsObject 0x029C //8B  88  ??  ??  00  00  85  C9  74  0D  8B  01  FF  50  4C + 2 dec
#define m_StuckLast 0x2F98 //8D  46  01  89  81  ??  ??  00  00  B8 + 5 dec
#define m_dwPlayerMaxSpeed (0x434 / 4) //8B  80  ??  ??  00  00  FF  D0  8B  47  08 + 2 dec
#define m_iMoveState_ 0x38F4
#define m_flStamina_ 0x0A2C8
#define m_isurfacedata 0x35A0 //05  ??  00  00  00  C7  04  24  00  00  80  3F  FF  B1  ??  ??  00  00  50  FF  92  ??  ??  00  00  + 14 dec
#define m_bIsWalking_ 0x0389F
#define m_bDuckOverride_ 0x0A2D8
#define m_duckUntilOnGround 0x0A3A0 //80  B8  ??  ??  00  00  00  74  0D  F6  41  ??  04 + 2
#define m_bServerSideJumpAnimation 0x3A05 //80  B9  ??  ??  ??  ??  00  75  0F  8B  89  ??  ??  ??  ??  6A  00 + 2 dec
#define m_bSlowMovement 0x31B4 //80  B9  ??  ??  00  00  00  0F  85  ??  ??  ??  ??  8B  46  08 + 2 dec
#define m_bGameMovementOnGround 0x3208 //C6  81  ??  ??  00  00  00  8B  4E  04 + 2 dec
#define moveparent_ 0x144
#define m_dwEstimateAbsVelocity (0x234 / 4)
#define m_iEFlags 0xE4
#define m_SurfaceFriction 0x3210 //F3  0F  59  45  FC  F3  0F  59  80  ??  ??  00  00  F3  0F  5C  D0 + 9 dec
#define m_weaponMode 0x32DC
#define AllowThirdPersonOffset (0x0A4 / 4)
#define dw_m_BoneAccessor 0x2694
#define standardblendingrules_offset (0x320 / 4)
#define buildtransformations_offset (0x2E0 / 4)
#define doextraboneprocessing_offset (0x300 / 4)
#define LastBoneChangedTime_Offset (956 / 4)
#define IsViewModel_index (912 / 4) //8B  47  FC  8D  4F  FC  8B  80  ??  ??  00  00  FF  D0  0F  B6  15 + 8
#define LastSetupBonesFrameCount 0xA5C
#define m_iLastOcclusionCheckFrameCount 0xA30 //8B  B7  ??  ??  00  00  89  75  F8  39  70  04 //+ 2 framecount
#define m_iLastOcclusionCheckFlags 0xA28 //C7  87  ??  ??  00  00  00  00  00  00  85  F6  0F  84 //+ 2 occlusion flags
#define m_BoneSetupLock 9892
#define PlayFootstepSoundOffset (0x550 / 4)  //05  ??  00  00  00  C7  04  24  00  00  80  3F  FF  B1  ??  ??  00  00  50  FF  92  ??  ??  00  00 + 21 dec
#define next_message_time 0x110

#define m_dwPhysicsSimulate (0x254 / 4)
#define m_nSimulationTick 0x2A8

#define __m_pStudioHdr 0x293C
#define m_pStudioHdr2 0x2938
#define m_Local_ 0x2FAC
#define m_pRagdoll 0x9F4 //83  B9  ??  ??  00  00  00  74  0D  80  B9  ??  ??  00  00  00  0F  85 + 2 dec
#define m_CachedBoneData 0x2900
#define m_iPrevBoneMask 0x268C
#define m_iAccumulatedBoneMask 0x2690
#define m_flLastBoneSetupTime 0x2914
#define m_pIK_offset 9824
#define m_EntClientFlags 104
//#define last_command_ack 0x4CB4

#define m_bIsToolRecording 0x28D
#define m_dwGetPredictable 0x2EA
#define m_iMostRecentModelBoneCounter 0x2680
#define m_hNetworkMoveParent 0x144
#define m_pMoveParent 0x300

#define ShouldInterpolateIndex (0x2B4 / 4)
#define EyeAnglesIndex (0x290 / 4)

#define m_dwWorldSpaceCenterOffset 0x138
#define m_dwGetAbsAnglesOffset 0x2C
#define m_dwEntIndexOffset 0x28

#define m_dwGetAlive (0x258 / 4)
#define dwClientTickCount 0x170 //subtract 8 from hooked clientstate functions
#define dwServerTickCount 0x16C //subtract 8 from hooked clientstate functions
#define m_flMaxSpeed_ 0x322C
#define m_iBurstShotsRemaining_ 0x3364
#define dwm_nAnimOverlay 0x2970
#define dwRenderAngles (deadflag + 0x4)
#define m_iObserverMode_ 0x334C
#define m_dwObserverModeVMT (0x480 / 4) //(0x478 / 4) //8B  4E  ??  8B  01  8B  80  ??  ??  00  00  FF  D0  83  F8  04 + 7 dec
#define SetHost_CurrentHost 0x18
#define m_dwGetCollisionAngles 0x24
#define m_flFriction_ 0x0140
#define m_dwShouldCollide 0x29C
#define m_dwGetSolid 0x144 //8B  8B  ??  ??  00  00  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  33  C9  8B  89  ??  ??  00  00  83  F9  FF  74  17 + 38 dec
//#define chokedcommands 0x4CB0
#define m_pCurrentCommand_ 0x3314 //C7  86  ??  ??  ??  ??  00  00  00  00  B9  ??  ??  ??  ??  A1
//#define lastoutgoingcommand 0x4CAC
#define m_OffsetEyePos 0x28C
#define m_OffsetEyeAngles 0x290
#define m_dwGetBaseAnimating 0x0B0
#define m_dwGetLastUserCommand 0x60
#define m_dwIsPlayer 0x260
#define m_dwSimulate (0x184 / 4)
#define m_dwPreDataUpdate (0x18 / 4)
#define m_dwPostDataUpdate (0x1C / 4)
#define m_dwGetNetChannel 0x50
#define m_dwGetNetChannel2 0
#define m_dwIsConnected 0x84
#define m_dwIsSpawned 0x88
#define m_bReloadVisuallyComplete  0x32FC
#define m_dwIsActive 0x8C
#define m_dwIsFakeClient 0x90
#define isHLTV 0x4CC4 //ClientState
//#define m_nSolidType 0x033A //FIXME
#define m_bInSimulation 0x4C90 //ClientState
#define m_bRedraw 0x3360
#define _m_nForceBone 0x267C
#define m_dwObserverTargetVMT (0x47C / 4) //8B  4E  ??  8B  01  8B  80  ??  ??  00  00  FF  D0  8B  F8 + 7 dec
#define m_dwSetLocalViewAngles 0x5A4
#define m_dwMaxPlayer 0x00000308
//#define m_dwEntityList 0x04ACF844
#define m_bDormant_ 0x000000ED//E9
#define m_iGlowIndex 0x00000A3F8//A330
//#define m_dwLocalPlayer 0x00AAC708
#define m_dwLocalPlayerIndex 0x00000178
//#define m_dwClientState 0x005CB514
#define m_dwViewAngles 0x00004D0C
#define m_iCrossHairID 0x0000AA70 //m_dwInCross = 0x2400;
#define m_iHealthServer 0x21C
#define m_vecOldOrigin (0x3A0 + 8)
#define m_vecOriginReal 0xAC //same as local origin
#define m_SurvivalModeOrigin 0x3A80
#define m_vecLocalOrigin m_vecOriginReal
#define m_dwBoneMatrix 0x00002698
#define m_dwRadarBasePointer 0x00000054
#define m_dwGetDefaultFOV (0x510 / 4) //8B  06  8B  CE  FF  90  ??  ??  00  00  8B  CF  66  0F  6E  C0  0F  5B  C0 + 6 dec
#define m_hPlayerAnimState 0x3894 //(m_bIsScoped - 8) //0x3874
#define m_hPlayerAnimStateServer 0x25D4 //this is base + 4 //0x29FC //0x29F8 is base not CSGO
#define m_flGoalFeetYawServer 0x70 //this is possibly wrong. it's what server sets absangles to //0x28 is base not CSGO
#define m_flCurrentFeetYawServer 0x68 //server sets lowerbodyyawtarget to this //0x2C is base not CSGO
#define m_flNextLowerBodyYawUpdateTimeServer 0x0FC
#define m_nMoveType 0x00000258 //m_bMoveType
#define m_nMoveCollide (m_nMoveType + 1)
#define m_nHitboxSetServer 0x39C
#define m_ArmorValueServer 0xEBC
#define ResetLatchedOffset (0x1BC / 4)
#define m_flLastDuckTime 0x3038 //F3  0F  10  80  ??  ??  00  00  F3  0F  58  45  FC  A1 + 4 dec
#define m_dwsurfaceProps 0x359C //0F  BF  52  40  89  91  ??  ??  00  00 + 6 dec
#define m_chTextureType 0x35A4 //8B  81  ??  ??  00  00  8A  40  ??  88  81  ??  ??  00  00  5D  C2  04  00 + 11 dec
#define m_flWaterJumpTime 0x31FC //F3  0F  10  80  ??  ??  00  00  0F  2E  C6  9F  F6  C4  44  0F  8A + 4 dec

#define m_bHasHelmetServer 0x14F4
#define m_bIsGrabbingHostage_ 0x38A9
#define m_bInReload 0x3275 //(m_bIsDefusing - 0x65F) //F6  41  24  01  74  2E  80  BA  ??  ??  00  00  00  75  25 + 8 dec
#define m_aimPunchAngleVMT (0x540 / 4) //8B  01  8D  54  24  14  52  FF  90  ??  ??  00  00 + 9 dec
#define m_angEyeAnglesServer 0x29B0
#define m_vecAbsAngles 0xC4
#define m_vecAbsVelocity_ 0x94
#define m_vecDuckingOrigin 0x2FA4 //F3  0F  10  89  ??  ??  00  00  F3  0F  10  81  ??  ??  00  00  F3  0F  5C  40  04  F3  0F  5C  08 + 4 dec
#define m_vecAbsOrigin 0xA0
#define m_vecLocalAnglesOffset 0xD0

#define m_szLastPlaceName 0x00003588
#define m_iWeaponID 0x000032EC
#define m_iClip2_ 0x3238
#define m_vecLocalAngles (0xC8 + 8)
#define m_vecOldAngRotation (0x3AC + 8)
#define m_flAnimTime 0x025C
#define m_flAnimTimeOld (m_flAnimTime + 4)
#define m_flSimulationTimeServer 0x6C
#define m_nCreationTick (0x998 + 8)
#define m_flProxyRandomValue (0x0D8 + 8)

#define m_nBody 0x0A20
#define m_flEncodedController 0x0A48
#define m_flPoseParameterServer 0x3D8
#define __m_flPlaybackRate 0x0A18
#define m_bDucked 0x3034
#define m_bDucking 0x3035 //80  B9  ??  ??  00  00  00  74  0D  F6  81  ??  ??  00  00  02 + 2 dec
#define m_bInDuckJump 0x303C
#define m_nDuckTimeMsecs 0x2FF8
#define m_nDuckJumpTimeMsecs 0x2FFC
#define m_flSwimSoundTime 0x3200 //C7  80  ??  ??  00  00  00  00  7A  44  8B  46  08 + 2 dec
#define m_nJumpTimeMsecs 0x3000
#define m_nNextThinkTick 0x00F8
#define m_flLaggedMovementValue_ 0x3568
#define m_vecLadderNormal_ 0x3214
#define m_flTimeNotOnLadder_ 0x3204 //0F  2F  81  ??  ??  ??  ??  0F  82  ??  ??  ??  ??  F3  0F  10  0D + 3 dec
#endif

// entity effects
enum
{
	EF_BONEMERGE = 0x001,	// Performs bone merge on client side
	EF_BRIGHTLIGHT = 0x002,	// DLIGHT centered at entity origin
	EF_DIMLIGHT = 0x004,	// player flashlight
	EF_NOINTERP = 0x008,	// don't interpolate the next frame
	EF_NOSHADOW = 0x010,	// Don't cast no shadow
	EF_NODRAW = 0x020,	// don't draw entity
	EF_NORECEIVESHADOW = 0x040,	// Don't receive no shadow
	EF_BONEMERGE_FASTCULL = 0x080,	// For use with EF_BONEMERGE. If this is set, then it places this ent's origin at its
									// parent and uses the parent's bbox + the max extents of the aiment.
									// Otherwise, it sets up the parent's bones every frame to figure out where to place
									// the aiment, which is inefficient because it'll setup the parent's bones even if
									// the parent is not in the PVS.
									EF_ITEM_BLINK = 0x100,	// blink an item so that the user notices it.
									EF_PARENT_ANIMATES = 0x200,	// always assume that the parent entity is animating
#ifdef NFS_DLL
									EF_POWERUP_SPEED = 0x400,
									EF_POWERUP_DAMAGE = 0x800,
									EF_POWERUP_AMMO = 0x1000,
									EF_POWERUP_ARMOR = 0x2000,
									EF_POWERUP_BATTERY = 0x4000,
									EF_ROCKET = 0x8000, //dlight that glows orange-red
									EF_MAX_BITS = 15
#else
									EF_MAX_BITS = 10
#endif
};

#endif