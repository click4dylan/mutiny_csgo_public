#pragma once
#include "CSGO_HX.h"

/*
struct WeaponInfo_t
{
float m_flArmorRatio;
float m_flPenetration;
float m_iDamage;
float m_flRange;
float m_flRangeModifier;
};
*/

extern DWORD WeaponDataPtrUnknown;
extern DWORD WeaponDataPtrUnknownCall;
extern DWORD GetCSWeaponDataAdr;

class CBaseEntity;

enum CSWeaponType
{
	WEAPONTYPE_KNIFE = 0,
	WEAPONTYPE_PISTOL,
	WEAPONTYPE_SUBMACHINEGUN,
	WEAPONTYPE_RIFLE,
	WEAPONTYPE_SHOTGUN,
	WEAPONTYPE_SNIPER_RIFLE,
	WEAPONTYPE_MACHINEGUN,
	WEAPONTYPE_C4,
	WEAPONTYPE_GRENADE,
	WEAPONTYPE_UNKNOWN
};

struct CHudTexture
{
	char	szShortName[64];	//0x0000
	char	szTextureFile[64];	//0x0040
	bool	bRenderUsingFont;	//0x0080
	bool	bPrecached;			//0x0081
	char	cCharacterInFont;	//0x0082
	BYTE	pad_0x0083;			//0x0083
	int		hFont;				//0x0084
	int		iTextureId;			//0x0088
	float	afTexCoords[4];		//0x008C
	int		iPosX[4];			//0x009C
}; //Size=0x00AC

class strike_weapon_definition;

class WeaponInfo_t
{
public:
	char pad_0000[4]; //0x0000
	char *ConsoleName; //0x0004
	char pad_0008[12]; //0x0008
	int iMaxClip1; //0x0014
	char pad_0018[12]; //0x0018
	int iMaxClip2; //0x0024
	char pad_0028[4]; //0x0028
	char *szWorldModel; //0x002C
	char *szViewModel; //0x0030
	char *szDropedModel; //0x0034
	char pad_0038[4]; //0x0038
	char *N00000984; //0x003C
	char pad_0040[56]; //0x0040
	char *szEmptySound; //0x0078
	char pad_007C[4]; //0x007C
	char *szBulletType; //0x0080
	char pad_0084[4]; //0x0084
	char *szHudName; //0x0088
	char *szWeaponName; //0x008C
	char pad_0090[60]; //0x0090
	int WeaponType; //0x00CC
	int iWeaponPrice; //0x00D0
	int iKillAward; //0x00D4
	char *szAnimationPrefex; //0x00D8
	float flCycleTime; //0x00DC
	float flCycleTimeAlt; //0x00E0
	float flTimeToIdle; //0x00E4
	float flIdleInterval; //0x00E8
	bool bFullAuto; //0x00EC
	char pad_00ED[3]; //0x00ED
	int iDamage; //0x00F0
	float flArmorRatio; //0x00F4
	int iBullets; //0x00F8
	float flPenetration; //0x00FC
	float flFlinchVelocityModifierLarge; //0x0100
	float flFlinchVelocityModifierSmall; //0x0104
	float flRange; //0x0108
	float flRangeModifier; //0x010C
	char pad_0110[28]; //0x0110
	int iCrosshairMinDistance; //0x012C
	float flMaxPlayerSpeed; //0x0130
	float flMaxPlayerSpeedAlt; //0x0134
	char pad_0138[4]; //0x0138
	float flSpread; //0x013C
	float flSpreadAlt; //0x0140
	float flInaccuracyCrouch; //0x0144
	float flInaccuracyCrouchAlt; //0x0148
	float flInaccuracyStand; //0x014C
	float flInaccuracyStandAlt; //0x0150
	float flInaccuracyJumpIntial; //0x0154
	float flInaccuracyJumpApex; //new april 16, 2020
	float flInaccuracyJump; //0x0158
	float flInaccuracyJumpAlt; //0x015C
	float flInaccuracyLand; //0x0160
	float flInaccuracyLandAlt; //0x0164
	float flInaccuracyLadder; //0x0168
	float flInaccuracyLadderAlt; //0x016C
	float flInaccuracyFire; //0x0170
	float flInaccuracyFireAlt; //0x0174
	float flInaccuracyMove; //0x0178
	float flInaccuracyMoveAlt; //0x017C
	float flInaccuracyReload; //0x0180
	int iRecoilSeed; //0x0184
	float flRecoilAngle; //0x0188
	float flRecoilAngleAlt; //0x018C
	float flRecoilVariance; //0x0190
	float flRecoilAngleVarianceAlt; //0x0194
	float flRecoilMagnitude; //0x0198
	float flRecoilMagnitudeAlt; //0x019C
	float flRecoilMagnatiudeVeriance; //0x01A0
	float flRecoilMagnatiudeVerianceAlt; //0x01A4
	float flRecoveryTimeCrouch; //0x01A8
	float flRecoveryTimeStand; //0x01AC
	float flRecoveryTimeCrouchFinal; //0x01B0
	float flRecoveryTimeStandFinal; //0x01B4
	int iRecoveryTransititionStartBullet; //0x01B8
	int iRecoveryTransititionEndBullet; //0x01BC
	bool bUnzoomAfterShot; //0x01C0
	char pad_01C1[31]; //0x01C1
	char *szWeaponClass; //0x01E0
	char pad_01E4[56]; //0x01E4
	float flInaccuracyPitchShift; //0x021C
	float flInaccuracySoundThreshold; //0x0220
	float flBotAudibleRange; //0x0224
	char pad_0228[12]; //0x0228
	bool bHasBurstMode; //0x0234
}; //Size: 0x0440

class CBaseCombatWeapon
{
public:
	char				__pad[0x64];
	int					index;
	void*				GetIronSightController();
	float				GetNextPrimaryAttack();
	float				GetNextSecondaryAttack();
	float				GetPostPoneFireReadyTime();
	void				SetPostPoneFireReadyTime(float time);
	float				GetLastShotTime();
	float				GetAccuracyPenalty();
	void				SetAccuracyPenalty(float penalty);
	int					GetXUIDLow();
	int					GetXUIDHigh();
	int					GetEntityQuality();
	int					GetAccountID();
	int					GetItemIDHigh();
	int 				GetItemDefinitionIndex();
	float				GetRecoilIndex();
	void				SetRecoilIndex(float index);
	int					GetFallbackPaintKit();
	int					GetFallbackStatTrak();
	float				GetFallbackWear();
	BOOL				IsEmpty();
	BOOLEAN				IsReloading();
	int					GetClipOne();
	int					GetClipTwo();
	int					GetPrimaryReserveAmmoCount();
	void				UpdateAccuracyPenalty();
	float				GetWeaponSpread();
	float				GetInaccuracy();
	WeaponInfo_t*		GetCSWpnData();

	char*				GetGunIcon();
	bool				IsGun();
	std::string			GetWeaponName();
	bool				IsInThrow();
	int					GetZoomLevel();
	int					GetZoomFOV(int level);
	bool				IsPistol(int OptionalItemDefinitionIndex = 0);
	bool				IsShotgun(int OptionalItemDefinitionIndex = 0);
	bool				IsKnife(int OptionalItemDefinitionIndex = 0);
	bool				IsGlove(int OptionalItemDefinitionIndex = 0);
	bool				IsSniper(bool IncludeAutoSnipers, int OptionalItemDefinitionIndex = 0);
	bool				IsGrenade(int OptionalItemDefinitionIndex = 0);
	bool				IsBomb(int OptionalItemDefinitionIndex = 0);
	char				GetMode();
	int					GetBurstShotsRemaining();
	float				GetNextBurstShotTime();
	bool				IsUtility(int OptionalItemDefinitionIndex = 0);
	float				GetMaxSpeed();
	float				GetMaxSpeed2();
	float				GetMaxSpeed3();
	void				OnJump(float flUpVelocity);
	void				OnLand(float flUpVelocity);
	int					GetZoomLevelVMT();
	int					GetNumZoomLevels();
	bool				StartedArming();
	bool				WeaponHasBurst();
	int					GetActivity();
	float				GetCycleTime(int secondary);
	bool				IsFullAuto();
	int					GetMaxClip1();
	int					GetReserveAmmoCount(int mode);
	bool				GetFireOnEmpty();
	void				SetFireOnEmpty(bool fireonempty);
	bool				IsPinPulled();
	Vector				CalculateSpread(int iSeed, float flInaccuracy, float flSpread, bool bRevolverRightClick = false, float *randfloats = nullptr);
	EHANDLE				GetWorldModelHandle();
	int					GetWeaponType();
	datamap_t*			GetPredDescMap();
	bool				ShouldUseDoubleTapHitchance();
};