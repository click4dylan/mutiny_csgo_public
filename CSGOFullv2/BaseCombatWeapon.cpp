#include "precompiled.h"
#include "BaseCombatWeapon.h"
#include "GameMemory.h"
#include "VTHook.h"
#include "VMProtectDefs.h"
#include "cx_strenc.h"
#include "GlobalInfo.h"
#include "UsedConvars.h"

#include "EncryptString.h"

DWORD WeaponDataPtrUnknown = NULL;
DWORD WeaponDataPtrUnknownCall = NULL;
DWORD GetCSWeaponDataAdr = NULL;

void* CBaseCombatWeapon::GetIronSightController()
{
	//if (IronSightControllerOffset == NULL)
	//{
	//	IronSightControllerOffset = FindMemoryPattern(ClientHandle, (char*)"8B  98  ??  ??  00  00  85  DB  74  46  80  3B  00", strlen((char*)"8B  98  ??  ??  00  00  85  DB  74  46  80  3B  00"));
	//
	//	if (!IronSightControllerOffset)
	//	{
	//		THROW_ERROR(ERR_CANT_FIND_IRONSIGHT_CONTROLLER_OFFSET);
	//		exit(EXIT_SUCCESS);
	//	}
	//	IronSightControllerOffset = *(DWORD*)(IronSightControllerOffset + 2);
	//}

	return (void*)*(DWORD*)((DWORD)this + StaticOffsets.GetOffsetValue(_IronSightController));
}

float CBaseCombatWeapon::GetNextPrimaryAttack()
{
	return *(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flNextPrimaryAttack);//*(float*)((DWORD)this + m_flNextPrimaryAttack);
}

float CBaseCombatWeapon::GetNextSecondaryAttack()
{
	return *(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flNextSecondaryAttack);
}

float CBaseCombatWeapon::GetPostPoneFireReadyTime()
{
	return *(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flPostponeFireReadyTime);
}

void CBaseCombatWeapon::SetPostPoneFireReadyTime(float time)
{
	*(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flPostponeFireReadyTime) = time;
}

float CBaseCombatWeapon::GetLastShotTime()
{
	return *(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_fLastShotTime);
}

float CBaseCombatWeapon::GetAccuracyPenalty()
{
	return *(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_fAccuracyPenalty);//*(float*)((DWORD)this + m_fAccuracyPenalty);
}

void CBaseCombatWeapon::SetAccuracyPenalty(float penalty)
{
	*(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_fAccuracyPenalty) = penalty;
}

int CBaseCombatWeapon::GetXUIDLow()
{
	return *(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_OriginalOwnerXuidLow);//*(int*)((DWORD)this + m_OriginalOwnerXuidLow);
}

int CBaseCombatWeapon::GetXUIDHigh()
{
	return *(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_OriginalOwnerXuidHigh);// *(int*)((DWORD)this + m_OriginalOwnerXuidHigh);
}

int CBaseCombatWeapon::GetEntityQuality()
{
	return *(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_iEntityQuality);//*(int*)((DWORD)this + m_iEntityQuality);
}

int CBaseCombatWeapon::GetAccountID()
{
	return *(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_iAccountID);// *(int*)((DWORD)this + m_iAccountID);
}

int CBaseCombatWeapon::GetItemIDHigh()
{
	return *(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_iItemIDHigh);//*(int*)((DWORD)this + m_iItemIDHigh);
}

int CBaseCombatWeapon::GetItemDefinitionIndex()
{
	return *(short*)((DWORD)this + g_NetworkedVariables.Offsets.m_iItemDefinitionIndex);// *(int*)((DWORD)this + m_iItemDefinitionIndex);
}

float CBaseCombatWeapon::GetRecoilIndex()
{
	return *(float*)((DWORD) this + g_NetworkedVariables.Offsets.m_flRecoilIndex);
}

void CBaseCombatWeapon::SetRecoilIndex(float index)
{
	*(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flRecoilIndex) = index;
}

int CBaseCombatWeapon::GetFallbackPaintKit()
{
	return *(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_nFallbackPaintKit);;// *(int*)((DWORD)this + m_nFallbackPaintKit);
}

int CBaseCombatWeapon::GetFallbackStatTrak()
{
	return *(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_nFallbackStatTrak);// *(int*)((DWORD)this + m_nFallbackStatTrak);
}

float CBaseCombatWeapon::GetFallbackWear()
{
	return *(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flFallbackWear);// *(float*)((DWORD)this + m_flFallbackWear);
}
char* CBaseCombatWeapon::GetGunIcon()
{
	int WeaponId = GetItemDefinitionIndex();
	switch (WeaponId)
	{
	case WEAPONTYPE_KNIFE:
	case WEAPON_KNIFE_BAYONET:
	case WEAPON_KNIFE_BUTTERFLY:
	case WEAPON_KNIFE_FALCHION:
	case WEAPON_KNIFE_FLIP:
	case WEAPON_KNIFE_GUT:
	case WEAPON_KNIFE_KARAMBIT:
	case WEAPON_KNIFE_M9_BAYONET:
	case WEAPON_KNIFE_T:
	case WEAPON_KNIFE_TACTICAL:
	case WEAPON_KNIFE_PUSH:
		return "J";
	case WEAPON_DEAGLE:
		return "i";
	case WEAPON_ELITE:
		return "p";
	case WEAPON_FIVESEVEN:
		return "s";
	case WEAPON_GLOCK:
		return "h";
	case WEAPON_HKP2000:
		return "G";
	case WEAPON_P250:
		return "k";
	case WEAPON_USP_SILENCER:
		return "N";
	case WEAPON_TEC9:
		return "V";
	case WEAPON_REVOLVER:
		return "H";
	case WEAPON_MAC10:
		return "U";
	case WEAPON_UMP45:
		return "B";
	case WEAPON_BIZON:
		return "t";
	case WEAPON_MP7:
		return "P";
	case WEAPON_MP9:
		return "A";
	case WEAPON_P90:
		return "F";
	case WEAPON_GALILAR:
		return "g";
	case WEAPON_FAMAS:
		return "a";
	case WEAPON_M4A1_SILENCER:
		return "T";
	case WEAPON_M4A1:
		return "R";
	case WEAPON_AUG:
		return "w";
	case WEAPON_SG556:
		return "L";
	case WEAPON_AK47:
		return "q";
	case WEAPON_G3SG1:
		return "f";
	case WEAPON_SCAR20:
		return "K";
	case WEAPON_AWP:
		return "e";
	case WEAPON_SSG08:
		return "X";
	case WEAPON_XM1014:
		return "M";
	case WEAPON_SAWEDOFF:
		return "J";
	case WEAPON_MAG7:
		return "I";
	case WEAPON_NOVA:
		return "D";
	case WEAPON_NEGEV:
		return "S";
	case WEAPON_M249:
		return "Y";
	case WEAPON_TASER:
		return "C";
	case WEAPON_FLASHBANG:
		return "d";
	case WEAPON_HEGRENADE:
		return "j";
	case WEAPON_SMOKEGRENADE:
		return "Z";
	case WEAPON_MOLOTOV:
		return "O";
	case WEAPON_DECOY:
		return "o";
	case WEAPON_INCGRENADE:
		return "l";
	case WEAPON_C4:
		return "y";
	case WEAPON_CZ75A:
		return "u";
	default:
		return "  ";
	}
};

WeaponInfo_t* CBaseCombatWeapon::GetCSWpnData()
{
	class CCSWeaponSystem
	{
	public:
		virtual ~CCSWeaponSystem() = 0;
		virtual void pad04() = 0;
		virtual WeaponInfo_t* GetWeaponInfo(int weaponId) = 0;
	};

	auto weaponSystem = reinterpret_cast<CCSWeaponSystem*>(StaticOffsets.GetOffsetValue(_GetWeaponSystem));

	if (!weaponSystem)
		return nullptr;

	return weaponSystem->GetWeaponInfo(this->GetItemDefinitionIndex());
}

BOOL CBaseCombatWeapon::IsEmpty()
{
	return *(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_iClip1) == 0;//*(int*)((DWORD)this + m_iClip1);
}

BOOLEAN CBaseCombatWeapon::IsReloading()
{
	return *StaticOffsets.GetOffsetValueByType<BOOLEAN*>(_m_bInReload, this);// 0x3235); //*(bool*)((DWORD)this + 0x3235);
}

int CBaseCombatWeapon::GetClipOne()
{
	return *(int*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_iClip1);
}

int CBaseCombatWeapon::GetClipTwo()
{
	return *(int*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_iClip2);
}

int CBaseCombatWeapon::GetPrimaryReserveAmmoCount()
{
	return *(int*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_iPrimaryReserveAmmoCount);
}

void CBaseCombatWeapon::UpdateAccuracyPenalty()
{
	GetVFunc<void(__thiscall*)(void*)>(this, StaticOffsets.GetOffsetValue(_UpdateAccuracyPenalty))(this);
}

float CBaseCombatWeapon::GetInaccuracy() //GetAccuracyPenalty
{
	return GetVFunc<float(__thiscall*)(void*)>(this, StaticOffsets.GetOffsetValue(_Cone))(this);
}

float CBaseCombatWeapon::GetWeaponSpread()
{
	return GetVFunc<float(__thiscall*)(void*)>(this, StaticOffsets.GetOffsetValue(_Spread))(this);
}

bool CBaseCombatWeapon::IsGun()
{
	int id = GetItemDefinitionIndex();
	return !IsKnife(id) && !IsGrenade(id) && !IsBomb(id) && id != WEAPON_TASER;
#if 0
	int id = this->GetWeaponID();

	switch (id)
	{
	case WEAPON_DEAGLE:
	case WEAPON_ELITE:
	case WEAPON_FIVESEVEN:
	case WEAPON_GLOCK:
	case WEAPON_AK47:
	case WEAPON_AUG:
	case WEAPON_AWP:
	case WEAPON_FAMAS:
	case WEAPON_G3SG1:
	case WEAPON_GALILAR:
	case WEAPON_M249:
	case WEAPON_M4A1:
	case WEAPON_MAC10:
	case WEAPON_P90:
	case WEAPON_UMP45:
	case WEAPON_XM1014:
	case WEAPON_BIZON:
	case WEAPON_MAG7:
	case WEAPON_NEGEV:
	case WEAPON_SAWEDOFF:
	case WEAPON_TEC9:
		return true;
	case WEAPON_TASER:
		return false;
	case WEAPON_HKP2000:
	case WEAPON_MP7:
	case WEAPON_MP9:
	case WEAPON_NOVA:
	case WEAPON_P250:
	case WEAPON_SCAR20:
	case WEAPON_SG556:
	case WEAPON_SSG08:
		return true;
	case WEAPON_KNIFE:
	case WEAPON_FLASHBANG:
	case WEAPON_HEGRENADE:
	case WEAPON_SMOKEGRENADE:
	case WEAPON_MOLOTOV:
	case WEAPON_DECOY:
	case WEAPON_INCGRENADE:
	case WEAPON_C4:
	case WEAPON_KNIFE_T:
		return false;
	case WEAPON_M4A1_SILENCER:
	case WEAPON_USP_SILENCER:
	case WEAPON_CZ75A:
	case WEAPON_REVOLVER:
		return true;
	default:
		return false;
	}
#endif
}

std::string CBaseCombatWeapon::GetWeaponName()
{
static std::unordered_map< int, std::string > weapon_names = 
{
	{ WEAPON_DEAGLE,					XorStrCT("Deagle")},
	{ WEAPON_ELITE,						XorStrCT("Dual Berettas")},
	{ WEAPON_FIVESEVEN,					XorStrCT("Five-Seven")},
	{ WEAPON_GLOCK,						XorStrCT("Glock")},
	{ WEAPON_AK47,						XorStrCT("AK-47")},
	{ WEAPON_AUG,						XorStrCT("AUG")},
	{ WEAPON_AWP,						XorStrCT("AWP")},
	{ WEAPON_FAMAS,						XorStrCT("FAMAS")},
	{ WEAPON_G3SG1,						XorStrCT("G3SG1")},
	{ WEAPON_GALILAR,					XorStrCT("Galil")},
	{ WEAPON_M249,						XorStrCT("M249")},
	{ WEAPON_M4A1,						XorStrCT("M4A4")},
	{ WEAPON_MAC10,						XorStrCT("MAC-10")},
	{ WEAPON_P90,						XorStrCT("P90")},
	{ WEAPON_MP5SD,						XorStrCT("MP5-SD")},
	{ WEAPON_UMP45,						XorStrCT("UMP-45")},
	{ WEAPON_XM1014,					XorStrCT("XM1014")},
	{ WEAPON_BIZON,						XorStrCT("PP-Bizon")},
	{ WEAPON_MAG7,						XorStrCT("MAG-7")},
	{ WEAPON_NEGEV,						XorStrCT("Negev")},
	{ WEAPON_SAWEDOFF,					XorStrCT("Sawed-Off")},
	{ WEAPON_TEC9,						XorStrCT("Tec-9")},
	{ WEAPON_TASER,						XorStrCT("Zeus")},
	{ WEAPON_HKP2000,					XorStrCT("P2000")},
	{ WEAPON_MP7,						XorStrCT("MP7")},
	{ WEAPON_MP9,						XorStrCT("MP9")},
	{ WEAPON_NOVA,						XorStrCT("Nova")},
	{ WEAPON_P250,						XorStrCT("P250")},
	{ WEAPON_SCAR20,					XorStrCT("SCAR-20")},
	{ WEAPON_SG556,						XorStrCT("SG 553")},
	{ WEAPON_SSG08,						XorStrCT("Scout")},
	{ WEAPON_REVOLVER,					XorStrCT("R8")},
	{ WEAPON_KNIFE,						XorStrCT("Knife")},
	{ WEAPON_KNIFE_T,					XorStrCT("Knife")},
	{ WEAPON_KNIFE,						XorStrCT("Knife")},
	{ WEAPON_KNIFE_BAYONET,				XorStrCT("Bayonet")},	// u8"\u22C6 Bayonet"
	{ WEAPON_KNIFE_FLIP,				XorStrCT("Flip Knife")},
	{ WEAPON_KNIFE_GUT,					XorStrCT("Gut Knife")},
	{ WEAPON_KNIFE_KARAMBIT,			XorStrCT("Karambit")},
	{ WEAPON_KNIFE_M9_BAYONET,			XorStrCT("M9 Bayonet")},
	{ WEAPON_KNIFE_FALCHION,			XorStrCT("Falchion")},
	{ WEAPON_KNIFE_TACTICAL,			XorStrCT("Huntsman Knife")},
	{ WEAPON_KNIFE_SURVIVAL_BOWIE,		XorStrCT("Bowie Knife")},
	{ WEAPON_KNIFE_BUTTERFLY,			XorStrCT("Butterfly Knife")},
	{ WEAPON_KNIFE_PUSH,				XorStrCT("Shadow Daggers")},
	{ WEAPON_KNIFE_URSUS,				XorStrCT("Ursus Knife")},
	{ WEAPON_KNIFE_GYPSY_JACKKNIFE,		XorStrCT("Navaja Knife")},
	{ WEAPON_KNIFE_STILETTO,			XorStrCT("Stiletto Knife")},
	{ WEAPON_KNIFE_WIDOWMAKER,			XorStrCT("Talon Knife")},
	{ WEAPON_FLASHBANG,					XorStrCT("Flashbang")},
	{ WEAPON_HEGRENADE,					XorStrCT("Frag Grenade")},
	{ WEAPON_SMOKEGRENADE,				XorStrCT("Smoke Grenade")},
	{ WEAPON_MOLOTOV,					XorStrCT("Molotov")},
	{ WEAPON_DECOY,						XorStrCT("Decoy Grenade")},
	{ WEAPON_INCGRENADE,				XorStrCT("Incendiary Grenade")},
	{ WEAPON_C4,						XorStrCT("C4")},
	{ WEAPON_TAGRENADE,					XorStrCT("T.A.G Grenade")},
	{ WEAPON_HEALTHSHOT,				XorStrCT("Medishot")},
	{ WEAPON_M4A1_SILENCER,				XorStrCT("M4A1-S")},
	{ WEAPON_USP_SILENCER,				XorStrCT("USP-S")},
	{ WEAPON_CZ75A,						XorStrCT("CZ-75")},
	{ WEAPON_FISTS,						XorStrCT("Bare Hands")},
	{ WEAPON_BREACHCHARGE,				XorStrCT("Breach Charge")},
	{ WEAPON_TABLET,					XorStrCT("Tablet")},
	{ WEAPON_MELEE,						XorStrCT("Melee")},
	{ WEAPON_AXE,						XorStrCT("Axe")},
	{ WEAPON_HAMMER,					XorStrCT("Hammer")},
	{ WEAPON_TABLET,					XorStrCT("Wrench")},
	{ WEAPON_KNIFE_GHOST,				XorStrCT("Spectral Shiv")},
	{ WEAPON_FIREBOMB,					XorStrCT("Firebomb")},
	{ WEAPON_TABLET,					XorStrCT("Diversion Device")},
	{ WEAPON_FRAG_GRENADE,				XorStrCT("Frag Grenade")},
};

	return weapon_names[GetItemDefinitionIndex()];
#if 0

	int id = this->GetWeaponID();

	switch (id)
	{
	case WEAPON_DEAGLE:
		return strenc("Desert Eagle");
	case WEAPON_ELITE:
		return strenc("Dual Berettas");
	case WEAPON_FIVESEVEN:
		return strenc("Five-SeveN");
	case WEAPON_GLOCK:
		return strenc("Glock-18");
	case WEAPON_AK47:
		return strenc("AK-47");
	case WEAPON_AUG:
		return strenc("AUG");
	case WEAPON_AWP:
		return strenc("AWP");
	case WEAPON_FAMAS:
		return strenc("FAMAS");
	case WEAPON_G3SG1:
		return strenc("G3SG1");
	case WEAPON_GALILAR:
		return strenc("Galil");
	case WEAPON_M249:
		return strenc("M249");
	case WEAPON_M4A1:
		return strenc("M4A1");
	case WEAPON_MAC10:
		return strenc("MAC-10");
	case WEAPON_P90:
		return strenc("P90");
	case WEAPON_UMP45:
		return strenc("UMP-45");
	case WEAPON_XM1014:
		return strenc("XM1014");
	case WEAPON_BIZON:
		return strenc("PP-Bizon");
	case WEAPON_MAG7:
		return strenc("MAG-7");
	case WEAPON_NEGEV:
		return strenc("Negev");
	case WEAPON_SAWEDOFF:
		return strenc("Sawed-Off");
	case WEAPON_TEC9:
		return strenc("Tec-9");
	case WEAPON_TASER:
		return strenc("Taser");
	case WEAPON_HKP2000:
		return strenc("P2000");
	case WEAPON_MP7:
		return strenc("MP7");
	case WEAPON_MP9:
		return strenc("MP9");
	case WEAPON_NOVA:
		return strenc("Nova");
	case WEAPON_P250:
		return strenc("P250");
	case WEAPON_SCAR20:
		return strenc("SCAR-20");
	case WEAPON_SG556:
		return strenc("SG 553");
	case WEAPON_SSG08:
		return strenc("SSG 08");
	case WEAPON_KNIFE:
		return strenc("Knife");
	case WEAPON_FLASHBANG:
		return strenc("Flashbang");
	case WEAPON_HEGRENADE:
		return strenc("HE Grenade");
	case WEAPON_SMOKEGRENADE:
		return strenc("Smoke Grenade");
	case WEAPON_MOLOTOV:
		return strenc("Molotov");
	case WEAPON_DECOY:
		return strenc("Decoy");
	case WEAPON_INCGRENADE:
		return strenc("Incendiary Grenade");
	case WEAPON_C4:
		return strenc("C4");
	case WEAPON_KNIFE_T:
		return strenc("Knife");
	case WEAPON_M4A1_SILENCER:
		return strenc("M4A1-S");
	case WEAPON_USP_SILENCER:
		return strenc("USP-S");
	case WEAPON_CZ75A:
		return strenc("CZ75-Auto");
	case WEAPON_REVOLVER:
		return strenc("R8 Revolver");
	default:
		return strenc("Knife");
	}

	return "";
#endif
}

bool CBaseCombatWeapon::IsInThrow()
{
	bool bPinPulled = *CAST(bool*, this, g_NetworkedVariables.Offsets.m_bPinPulled);
	float flThrowTime = *CAST(float*, this, g_NetworkedVariables.Offsets.m_fThrowTime);

	return !(bPinPulled || fabsf(Interfaces::Globals->curtime - flThrowTime) > .1f);
}

int	CBaseCombatWeapon::GetZoomLevel()
{
	return *(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_zoomLevel);
}

int CBaseCombatWeapon::GetZoomFOV(int level)
{
	return StaticOffsets.GetVFuncByType<int(__thiscall*)(CBaseCombatWeapon*, int)>(_WeaponGetZoomFOVVMT, this)(this, level);
}

bool CBaseCombatWeapon::IsPistol(int OptionalItemDefinitionIndex)
{
	int id = OptionalItemDefinitionIndex ? OptionalItemDefinitionIndex : this->GetItemDefinitionIndex();

	switch (id)
	{
	case WEAPON_GLOCK:
	case WEAPON_P250:
	case WEAPON_USP_SILENCER:
	case WEAPON_DEAGLE:
	case WEAPON_CZ75A:
	case WEAPON_TEC9:
	case WEAPON_ELITE:
	case WEAPON_FIVESEVEN:
	case WEAPON_HKP2000:
	case WEAPON_REVOLVER:
		return true;
	}
	return false;
}

bool CBaseCombatWeapon::IsShotgun(int OptionalItemDefinitionIndex)
{
	int id = OptionalItemDefinitionIndex ? OptionalItemDefinitionIndex : this->GetItemDefinitionIndex();
	if (id == WEAPON_MAG7 || id == WEAPON_SAWEDOFF || id == WEAPON_XM1014 || id == WEAPON_NOVA)
		return true;
	return false;
}

bool CBaseCombatWeapon::IsKnife(int OptionalItemDefinitionIndex)
{
	int id = OptionalItemDefinitionIndex ? OptionalItemDefinitionIndex : this->GetItemDefinitionIndex();
	return id >= WEAPON_KNIFE_BAYONET && id < GLOVE_STUDDED_BLOODHOUND || id == WEAPON_KNIFE_T || id == WEAPON_KNIFE || id == WEAPON_KNIFEGG || id == WEAPON_FISTS || index == WEAPON_KNIFE_GHOST || index == WEAPON_KNIFE_SKELETON || index == WEAPON_SPANNER || index == WEAPON_HAMMER || index == WEAPON_AXE || index == WEAPON_MELEE;
}

bool CBaseCombatWeapon::IsGlove(int OptionalItemDefinitionIndex)
{
	int id = OptionalItemDefinitionIndex ? OptionalItemDefinitionIndex : this->GetItemDefinitionIndex();
	return id >= GLOVE_STUDDED_BLOODHOUND && id <= GLOVE_HYDRA;
}

bool CBaseCombatWeapon::IsSniper(bool IncludeAutoSnipers, int OptionalItemDefinitionIndex)
{
	int id = OptionalItemDefinitionIndex ? OptionalItemDefinitionIndex : this->GetItemDefinitionIndex();

	bool is_sniper = (id == WEAPON_SSG08 || id == WEAPON_AWP);
	return IncludeAutoSnipers ? (id == WEAPON_G3SG1 || id == WEAPON_SCAR20 || is_sniper) : is_sniper;
}

char CBaseCombatWeapon::GetMode()
{
	return *(char*)((DWORD)this + g_NetworkedVariables.Offsets.m_WeaponMode);
}

int	CBaseCombatWeapon::GetBurstShotsRemaining()
{ 
	return *(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_iBurstShotsRemaining);
}

float CBaseCombatWeapon::GetNextBurstShotTime()
{
	return *(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_fNextBurstShot);
}

bool CBaseCombatWeapon::IsUtility(int OptionalItemDefinitionIndex)
{
	int id = OptionalItemDefinitionIndex ? OptionalItemDefinitionIndex : this->GetItemDefinitionIndex();

	switch (id)
	{
	case WEAPON_TASER:
	case WEAPON_DIVERSION:
	case WEAPON_TABLET:
	case WEAPON_HEALTHSHOT:
	case WEAPON_BREACHCHARGE:
	case WEAPON_C4:
		return true;
	default:
		return false;
	}
}

bool CBaseCombatWeapon::IsGrenade(int OptionalItemDefinitionIndex)
{
	int id = OptionalItemDefinitionIndex ? OptionalItemDefinitionIndex : this->GetItemDefinitionIndex();

	switch (id)
	{
	case WEAPON_FLASHBANG:
	case WEAPON_HEGRENADE:
	case WEAPON_INCGRENADE:
	case WEAPON_MOLOTOV:
	case WEAPON_SMOKEGRENADE:
	case WEAPON_DECOY:
	case WEAPON_FRAG_GRENADE:
	case WEAPON_FIREBOMB:
	case WEAPON_TAGRENADE:
	case WEAPON_SNOWBALL:
	case WEAPON_BUMPMINE:
		return true;
	}
	return false;
}

bool CBaseCombatWeapon::IsBomb(int OptionalItemDefinitionIndex)
{
	int id = OptionalItemDefinitionIndex ? OptionalItemDefinitionIndex : this->GetItemDefinitionIndex();

	return id == WEAPON_C4;
}

float CBaseCombatWeapon::GetMaxSpeed()
{
	//8B  07  8B  CF  8B  80  ??  ??  00  00  FF  D0  D8  0D  ??  ??  ??  ??  F2  0F  10  05 + 6 dec
	typedef float(__thiscall* OriginalFn)(CBaseCombatWeapon*);
	return StaticOffsets.GetVFuncByType<OriginalFn>(_GetWeaponMaxSpeedVMT, this)(this);
}

float CBaseCombatWeapon::GetMaxSpeed2()
{
	//8B  80  ??  ??  00  00  FF  D0  D8  4C  24  24  D9  5C  24  24 + 2 dec
	typedef float(__thiscall* OriginalFn)(CBaseCombatWeapon*);
	return StaticOffsets.GetVFuncByType<OriginalFn>(_GetWeaponMaxSpeed2VMT, this)(this);
}

//NOTE: USE THIS ONE 
float CBaseCombatWeapon::GetMaxSpeed3()
{
	typedef float(__thiscall* OriginalFn)(CBaseCombatWeapon*);
	return StaticOffsets.GetVFuncByType<OriginalFn>(_GetWeaponMaxSpeed3VMT, this)(this);
}

void CBaseCombatWeapon::OnJump(float flUpVelocity)
{
	//8B  C8  F3  0F  11  04  24  FF  92  ??  ??  00  00  8B  E5  5D  C2  04  00 + 9 dec
	typedef void(__thiscall* OriginalFn)(CBaseCombatWeapon*, float);
	StaticOffsets.GetVFuncByType<OriginalFn>(_WeaponOnJumpVMT, this)(this, flUpVelocity);
}

void CBaseCombatWeapon::OnLand(float flUpVelocity)
{
	typedef void(__thiscall* OriginalFn)(CBaseCombatWeapon*, float);
	static DWORD index = StaticOffsets.GetOffsetValue(_WeaponOnJumpVMT) + 1; //TODO: fixme, get actual sig
	GetVFunc<OriginalFn>(this, index)(this, flUpVelocity);
}

int CBaseCombatWeapon::GetZoomLevelVMT()
{
	//0F  84  ??  ??  ??  ??  85  FF  0F  84  ??  ??  ??  ??  8B  07  8B  CF  8B  80  ??  ??  00  00 + 20 dec
	typedef int(__thiscall* OriginalFn)(CBaseCombatWeapon*);
	return StaticOffsets.GetVFuncByType<OriginalFn>(_WeaponGetZoomLevelVMT, this)(this);
}

int CBaseCombatWeapon::GetNumZoomLevels()
{
	//8B  80  ??  ??  00  00  FF  D0  83  F8  01  7E  2D  8B  07 + 2 dec
	typedef int(__thiscall* OriginalFn)(CBaseCombatWeapon*);
	return StaticOffsets.GetVFuncByType<OriginalFn>(_WeaponGetNumZoomLevelsVMT, this)(this);
}

bool CBaseCombatWeapon::StartedArming() 
{ 
	return *(bool*)((DWORD)this + g_NetworkedVariables.Offsets.m_bStartedArming);
}

bool CBaseCombatWeapon::WeaponHasBurst()
{
	switch (GetItemDefinitionIndex())
	{
		case WEAPON_FAMAS:
		case WEAPON_GLOCK:
			return true;
	};
	return false;
	//return StaticOffsets.GetVFuncByType<bool(__thiscall*)(CBaseCombatWeapon*)>(_WeaponHasBurstVMT, this)(this);
}

int CBaseCombatWeapon::GetActivity()
{
	return *StaticOffsets.GetOffsetValueByType<int*>(_m_Activity, this);
}

float CBaseCombatWeapon::GetCycleTime(int secondary)
{
	return StaticOffsets.GetVFuncByType<float(__thiscall*)(CBaseCombatWeapon*, int)>(_GetCycleTimeVMT, this)(this, secondary);
}

bool CBaseCombatWeapon::IsFullAuto()
{
	return StaticOffsets.GetVFuncByType<bool(__thiscall*)(CBaseCombatWeapon*)>(_IsFullAutoVMT, this)(this);
}

int CBaseCombatWeapon::GetMaxClip1()
{
	return StaticOffsets.GetVFuncByType<int(__thiscall*)(CBaseCombatWeapon*)>(_GetMaxClip1VMT, this)(this);
}

int CBaseCombatWeapon::GetReserveAmmoCount(int mode)
{
	return StaticOffsets.GetOffsetValueByType<int(__thiscall*)(CBaseCombatWeapon*, int, CBaseCombatWeapon*)>(_GetReserveAmmoCount)(this, mode, this);
}

bool CBaseCombatWeapon::GetFireOnEmpty()
{
	return *StaticOffsets.GetOffsetValueByType<bool*>(_m_bFireOnEmpty, this);
}

void CBaseCombatWeapon::SetFireOnEmpty(bool fireonempty)
{
	*StaticOffsets.GetOffsetValueByType<bool*>(_m_bFireOnEmpty, this) = fireonempty;
}

bool CBaseCombatWeapon::IsPinPulled()
{
	return *(bool*)((DWORD)this + g_NetworkedVariables.Offsets.m_bPinPulled);
}

Vector CBaseCombatWeapon::CalculateSpread(int iSeed, float flInaccuracy, float flSpread, bool bRevolverRightClick, float* randfloats)
{
	auto info = GetCSWpnData();
	if (!info || info->iBullets == 0)
		return vecZero;

	int id = GetItemDefinitionIndex();
	float recoil_index = GetRecoilIndex();

	float r1 = 0.f;
	float r2 = 0.f;
	float r3 = 0.f, r4 = 0.f;

	if (randfloats)
	{
		r1 = randfloats[0];
		r2 = randfloats[1];
		r3 = randfloats[2];
		r4 = randfloats[3];
	}
	else
	{
		RandomSeed((iSeed & 0xFF) + 1);
		r1 = RandomFloat(0.f, 1.f);
		r2 = RandomFloat(0.f, (M_PI * 2.0f));

		if (weapon_accuracy_shotgun_spread_patterns.GetVar() && weapon_accuracy_shotgun_spread_patterns.GetVar()->GetInt() > 0)
		{
			StaticOffsets.GetOffsetValueByType<GetShotgunSpreadFn>(_ShotgunSpread)(this, id, 0, 0 + info->iBullets * recoil_index, &r4, &r3);
		}
		else
		{
			r3 = RandomFloat(0.f, 1.f);
			r4 = RandomFloat(0.f, (M_PI * 2.0f));
		}
	}

	if (id == WEAPON_REVOLVER && bRevolverRightClick) 
	{
		r3 = 1.f - (r1 * r1);
		r4 = 1.f - (r3 * r3);
	}
	else if (id == WEAPON_NEGEV && recoil_index < 3.f) 
	{
		for (int i = 3; i > recoil_index; --i) 
		{
			r1 *= r1;
			r3 *= r3;
		}

		r1 = 1.f - r1;
		r3 = 1.f - r3;
	}

	// grab cosine/sine values used
	float c1 = std::cos(r2);
	float c2 = std::cos(r4);

	float s1 = std::sin(r2);
	float s2 = std::sin(r4);

	// calculate spread vector
	return { (c1 * (r1 * flInaccuracy)) + (c2 * (r3 * flSpread)), (s1 * (r1 * flInaccuracy)) + (s2 * (r3 * flSpread)), 0.0f };
}

EHANDLE CBaseCombatWeapon::GetWorldModelHandle()
{
	return *(EHANDLE*)((DWORD)this + g_NetworkedVariables.Offsets.m_hWeaponWorldModel);
}

int CBaseCombatWeapon::GetWeaponType()
{
	auto data = GetCSWpnData();
	return data->WeaponType;
}

datamap_t* CBaseCombatWeapon::GetPredDescMap()
{
	// todo: nit; make a signature for this somehow
	return GetVFunc<datamap_t*(__thiscall*)(CBaseCombatWeapon*)>(this, 18)(this);
}

bool CBaseCombatWeapon::ShouldUseDoubleTapHitchance()
{
	auto index = GetItemDefinitionIndex();
	switch (index)
	{
		case WEAPON_SSG08:
		case WEAPON_AWP:
		case WEAPON_REVOLVER:
			return false;
	}
	return true;
}