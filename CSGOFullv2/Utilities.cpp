#include "precompiled.h"
#include "Utilities.h"
#include "misc.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include "LocalPlayer.h"
#include "CParallelProcessor.h"
#include "AutoWall.h"
#include "UsedConvars.h"

#include "Adriel/stdafx.hpp"

inline uint32_t fnv1a(unsigned char oneByte, uint32_t hash) {
	return (oneByte ^ hash) * 0x01000193;
}

uint32_t fnv1a(const void* data, size_t numBytes, uint32_t hash)
{
	const unsigned char* ptr = (const unsigned char*)data;
	while (numBytes--) hash = fnv1a(*ptr++, hash);
	return hash;
}

uint32_t fnv1a(const char* text, uint32_t hash)
{
	while (*text) hash = fnv1a((unsigned char)*text++, hash);
	return hash;
}

uint32_t fnv1a(const std::string& text, uint32_t hash) {
	return fnv1a(text.data(), text.length(), hash);
}

std::string GetHexFromColor(Color _clr)
{
	std::string result = "";
	std::stringstream sstream;
	sstream << std::setfill('0') << std::setw(2) << std::hex << _clr.r();
	sstream << std::setfill('0') << std::setw(2) << std::hex << _clr.g();
	sstream << std::setfill('0') << std::setw(2) << std::hex << _clr.b();
	result = sstream.str();

	return result;
}

Color GetColorFromHex(std::string _hex)
{
	std::string _final = "";

	if (_hex.length() == 3)
	{
		for (int i = 0; i < 3; i++)
			_final.append(2u, _hex[i]);
	}
	else
		_final = _hex;

	int num = std::stoi(_final, 0, 16);
	int r = num / 0x10000;
	int g = (num / 0x100) % 0x100;
	int b = num % 0x100;
	return Color(r, g, b);
}

std::vector<std::string> split(const std::string &s, char delim)
{
	std::vector<std::string> elems;
	std::stringstream ss(s);
	std::string number;

	while (std::getline(ss, number, delim))
		elems.push_back(number);

	return elems;
}

bool replace(std::string& str, const std::string& from, const std::string& to)
{
	size_t start_pos = str.find(from);

	if (start_pos == std::string::npos)
		return false;

	str.replace(start_pos, from.length(), to);
	return true;
}

int MapReverseKey(std::map<int, std::string> _map, std::string _find)
{
	for (auto it = _map.begin(); it != _map.end(); it++)
	{
		std::string _findToUpper = it->second;

		std::transform(_findToUpper.begin(), _findToUpper.end(), _findToUpper.begin(), ::toupper);
		std::transform(_find.begin(), _find.end(), _find.begin(), ::toupper);

		if (_findToUpper == _find)
			return it->first;
	}

	return -1337;
}

bool IsPointInBox(int _x1, int _y1, int _x2, int _y2, int _w, int _h, bool debug)
{
	if (_x1 >= _x2 &&
		_x1 <= _x2 + _w &&
		_y1 >= _y2 &&
		_y1 <= _y2 + _h)
	{
		//Draw debug info if wanted
		if (debug)
			g_Draw.Border(_x2 - 1, _y2 - 1, _w + 2, _h + 2, 2, 0, 255, 0, 255);

		return true;
	}

	//Draw debug info if wanted
	if (debug)
	{
		g_Draw.Border(_x2 - 1, _y2 - 1, _w + 2, _h + 2, 2, 255, 0, 0, 255);
		g_Draw.Border(_x1 - 2, _y1 - 2, 4, 4, 2, 0, 255, 0, 255);
	}

	return false;
}

void replaceAll(std::string &s, const std::string &search, const std::string &replace)
{
	for (size_t pos = 0; ; pos += replace.length()) {
		// Locate the substring to replace
		pos = s.find(search, pos);
		if (pos == std::string::npos) break;
		// Replace by erasing and inserting
		s.erase(pos, search.length());
		s.insert(pos, replace);
	}
}

Vector m_vForward;
Vector m_vRight;
Vector m_vUp;
CBaseEntity* m_pEntity;
CBaseCombatWeapon* m_pWeapon;
int NumSeedsNeeded = 0;
int NumMissesMax = 0;
std::atomic<int> NumSeedsHit = 0;
std::atomic<int> NumSeedsMissed = 0;
//std::atomic<int> NumHitchanceThreadsFinished = 0;
float _Range;
float _RecoilIndex;
float _Inaccuracy;
float _WeaponSpread;
int TargetHitgroup;
bool _CanOptimizeTrace;
Vector* _HitboxWorldMins, *_HitboxWorldMaxs;

struct HitchanceThread_t
{
	float A, B, C, D;
};

HitchanceThread_t HitchanceJobs[256];

void __cdecl ParallelHitchance(HitchanceThread_t & args) 
{
	if (NumSeedsHit >= NumSeedsNeeded || (NumMissesMax && NumSeedsMissed >= NumMissesMax))
	{
		//++NumHitchanceThreadsFinished;
		return;
	}

	float randfloats[4] = { args.A, args.B, args.C, args.D };

	Vector spread = m_pWeapon->CalculateSpread(0, _Inaccuracy, _WeaponSpread, false, randfloats);

	Vector dir = m_vForward + (m_vRight * spread.x) + (m_vUp * spread.y);
	VectorNormalizeFast(dir);

	CGameTrace trace;
	trace_t boxtrace;
	Ray_t ray;
	ray.Init(LocalPlayer.ShootPosition, LocalPlayer.ShootPosition + (dir * _Range));

	// can't trust bones on far esp targets
	//Do a trace to the surrounding bounding box to see if we hit it, for optimization
	if (!_CanOptimizeTrace || IntersectRayWithBox(ray, *_HitboxWorldMins, *_HitboxWorldMaxs, 0.0f, &boxtrace))
	{
		CPlayerrecord* _playerRecord = &m_PlayerRecords[m_pEntity->index];
		Interfaces::EngineTrace->ClipRayToEntity(ray, MASK_SHOT, (IHandleEntity*)m_pEntity, &trace);

		Autowall_Output_t output;
		bool scan_through_teammates = variable::get().ragebot.b_scan_through_teammates;

		if (trace.m_pEnt == m_pEntity && (trace.hitgroup == TargetHitgroup || TargetHitgroup == HITGROUP_GENERIC) && Autowall(LocalPlayer.ShootPosition, trace.endpos, output, !scan_through_teammates, true, m_pEntity, trace.hitbox) == m_pEntity)
			++NumSeedsHit;
		else
			++NumSeedsMissed;
	}
	else
	{
		//CPlayerrecord* _playerRecord = &m_PlayerRecords[m_pEntity->index];
		//Interfaces::DebugOverlay->AddBoxOverlay(_playerRecord->m_TargetRecord->m_AbsOrigin, _playerRecord->m_TargetRecord->m_HitboxWorldMins - _playerRecord->m_TargetRecord->m_AbsOrigin, _playerRecord->m_TargetRecord->m_HitboxWorldMaxs - _playerRecord->m_TargetRecord->m_AbsOrigin, _playerRecord->m_TargetRecord->m_AbsAngles, 255, 0, 0, 200, TICKS_TO_TIME(2));
		//Interfaces::DebugOverlay->AddLineOverlayAlpha(LocalPlayer.ShootPosition, ray.m_Start + ray.m_Delta, 0, 255, 0, 40, true, TICKS_TO_TIME(2));
		
		++NumSeedsMissed;
	}
	//++NumHitchanceThreadsFinished;
}
#include "WeaponController.h"
#include "UsedConvars.h"
bool WeaponIsAtMaxAccuracy(WeaponInfo_t *_WeaponData, float _Inaccuracy)
{
	return false;

	if (weapon_accuracy_nospread.GetVar()->GetInt() > 0)
		return true;

	const auto round_accuracy = [](const float accuracy) { return floorf(accuracy * 1000.f) / 1000.f; };

	//revolver
	if (LocalPlayer.WeaponVars.IsRevolver)
	{
		if (!g_WeaponController.RevolverWillFire())
			return true;

		if (LocalPlayer.Entity->GetDuckAmount() > 0.0f)
			return round_accuracy(_Inaccuracy) <= round_accuracy(0.002948f);
		return round_accuracy(_Inaccuracy) <= round_accuracy(0.007714f);
	}

	//Don't bother running hitchance if the gun won't shoot
	if (g_WeaponController.WeaponDidFire(CurrentUserCmd.cmd->buttons | IN_ATTACK) != 1)
		return true;

	bool scoped = LocalPlayer.CurrentWeapon && LocalPlayer.WeaponVars.IsScopedWeapon && LocalPlayer.CurrentWeapon->GetZoomLevel() > 0;

	if (LocalPlayer.Entity->GetDuckAmount() > 0.0f)
		return round_accuracy(_Inaccuracy) <= round_accuracy(scoped ? _WeaponData->flInaccuracyCrouchAlt : _WeaponData->flInaccuracyCrouch);

	return round_accuracy(_Inaccuracy) <= round_accuracy(scoped ? _WeaponData->flInaccuracyStandAlt : _WeaponData->flInaccuracyStand);
}

bool Hitchance(CBaseEntity* player, QAngle& viewangles, float percent, int _TargetHitgroup, float* calculatedHitchance)
{
#if 0
	g_SpreadMutex.lock();
	g_Info.m_Spread.clear();
	g_SpreadMutex.unlock();
#endif

	if (g_WeaponController.WeaponDidFire(CurrentUserCmd.cmd->buttons | IN_ATTACK) != 1)
	{
		if (calculatedHitchance != nullptr)
			*calculatedHitchance = 100.f;
		return true;
	}

	float scale = weapon_recoil_scale.GetVar()->GetFloat();

	NormalizeAngles(viewangles);
	AngleVectors(viewangles + (LocalPlayer.Entity->GetPunch() * scale), &m_vForward, &m_vRight, &m_vUp);

	m_pEntity = player;
	auto _Weapon = LocalPlayer.CurrentWeapon;
	m_pWeapon = _Weapon;
	auto _WeaponData = _Weapon->GetCSWpnData();
	_Range = _WeaponData->flRange;

	auto _playerRecord = g_LagCompensation.GetPlayerrecord(player);
	auto _tickRecord = _playerRecord->m_TargetRecord;
	_CanOptimizeTrace = !_playerRecord->m_bIsUsingFarESP && !_playerRecord->m_bIsUsingServerSide && _tickRecord && _tickRecord->m_bCachedBones;

	// can't trust bones on far esp targets
	if (_CanOptimizeTrace)
	{
		_HitboxWorldMins = &_tickRecord->m_HitboxWorldMins;
		_HitboxWorldMaxs = &_tickRecord->m_HitboxWorldMaxs;
		// test

		//Do a trace to the surrounding bounding box to see if we hit it, for optimization
		//This is never going to be false if it's called from the ragebot because the ragebot only calls hitchance if we can trace to the player
#if 0
		trace_t boxtrace;
		Ray_t ray;
		Vector m_vForwardNormalized = m_vForward;
		VectorNormalizeFast(m_vForwardNormalized);
		Vector m_End = (LocalPlayer.ShootPosition + m_vForwardNormalized * _WeaponData->flRange);
		Vector delta = m_End - LocalPlayer.ShootPosition;

		if (!IntersectRayWithBox(LocalPlayer.ShootPosition, delta, *_HitboxWorldMins, *_HitboxWorldMaxs, 0.0f, &boxtrace))
		{
			Interfaces::DebugOverlay->AddBoxOverlay(_tickRecord->m_AbsOrigin, _tickRecord->m_HitboxWorldMins - _tickRecord->m_AbsOrigin, _tickRecord->m_HitboxWorldMaxs - _tickRecord->m_AbsOrigin, _tickRecord->m_AbsAngles, 255, 0, 0, 200, 5.0f);
			Interfaces::DebugOverlay->AddLineOverlay(LocalPlayer.ShootPosition, m_End, 255, 0, 0, true, 5.0f);
			if(calculatedHitchance != nullptr)
				*calculatedHitchance = 0.f;
		
			return false;
		}
#endif
	}
	
	_Inaccuracy = _Weapon->GetInaccuracy();
	_WeaponSpread = _Weapon->GetWeaponSpread();
	_RecoilIndex = _Weapon->GetRecoilIndex();
	TargetHitgroup = _TargetHitgroup;

	if (WeaponIsAtMaxAccuracy(_WeaponData, _Inaccuracy))
	{
		if (calculatedHitchance != nullptr)
			*calculatedHitchance = 99.99f;
		return true;
	}
	
	NumSeedsHit = 0;
	NumSeedsMissed = 0;
	NumSeedsNeeded = 256 * (percent / 100.f);
	NumMissesMax = 256 - NumSeedsNeeded;
	//NumHitchanceThreadsFinished = 0;

#if 0
	float _RoundedCurrentSpread = floorf(_WeaponSpread * 1000.0f) / 1000.0f;
	float _RoundedDownMinSpread;

	auto mode = _Weapon->GetMode();// && !_Weapon->IsSniper(true);
	_RoundedDownMinSpread = _WeaponData->flSpreadAlt; //mode == 0 ? _WeaponData->flSpread : _WeaponData->flSpreadAlt;
	_RoundedDownMinSpread = floorf(_RoundedDownMinSpread * 1000.0f) / 1000.0f;
	/*
	float flBaseInaccuracy = mode == 0 ? _WeaponData->flInaccuracyStand : _WeaponData->flInaccuracyStandAlt;
	if (LocalPlayer.Entity->GetDuckAmount() > 0.0f)
		flBaseInaccuracy = mode == 0 ? _WeaponData->flInaccuracyCrouch : _WeaponData->flInaccuracyCrouchAlt;
	if (!LocalPlayer.Entity->GetGroundEntity())
		flBaseInaccuracy += mode == 0 ? _WeaponData->flInaccuracyJump : _WeaponData->flInaccuracyJumpAlt;
	if (LocalPlayer.Entity->GetMoveType() == MOVETYPE_LADDER)
		flBaseInaccuracy += mode == 0 ? _WeaponData->flInaccuracyLadder : _WeaponData->flInaccuracyLadderAlt; //FIXME?
	if (LocalPlayer.Entity->GetPlayerAnimState()->m_bInHitGroundAnimation)
		flBaseInaccuracy += mode == 0 ? _WeaponData->flInaccuracyLand : _WeaponData->flInaccuracyLandAlt; //FIXME?
	if (LocalPlayer.Entity->GetVelocity().Length2D() > 0.0f)
		flBaseInaccuracy += mode == 0 ? _WeaponData->flInaccuracyMove : _WeaponData->flInaccuracyMoveAlt; //FIXME?
	if (_Weapon->GetClipOne() <= 0)
		flBaseInaccuracy += _WeaponData->flInaccuracyReload;

	_RoundedDownMinSpread = floorf(flBaseInaccuracy * 1000.0f) / 1000.0f;
	*/

	if (_RoundedCurrentSpread == _RoundedDownMinSpread)
		return true;
#endif

	START_PROFILING

	for (int i = 0; i < 256;)
	{
		RandomSeed((i & 0xFF) + 1);
		HitchanceThread_t &job = HitchanceJobs[i++];

		job.A = RandomFloat(0.f, 1.f);
		job.B = RandomFloat(0.f, 2.f * M_PI_F);

		if (weapon_accuracy_shotgun_spread_patterns.GetVar() && weapon_accuracy_shotgun_spread_patterns.GetVar()->GetInt() > 0)
		{
			auto info = m_pWeapon->GetCSWpnData();
			StaticOffsets.GetOffsetValueByType<GetShotgunSpreadFn>(_ShotgunSpread)(m_pWeapon, m_pWeapon->GetItemDefinitionIndex(), 0, 0 + info->iBullets * _RecoilIndex, &job.D, &job.C);
		}
		else
		{
			job.C = RandomFloat(0.f, 1.f);
			job.D = RandomFloat(0.f, 2.f * M_PI_F);
		}
	}

	//for (int i = 0; i < 256; i++)
	//	ParallelHitchance(HitchanceJobs[i]);

	ParallelProcessChunks(HitchanceJobs, 256, &ParallelHitchance, 256 / 8);

	END_PROFILING

	//ParallelProcess is not truly parallel, so this while is useless for now
	//while (NumHitchanceThreadsFinished != 256) {}

	if (calculatedHitchance != nullptr)
	{
		*calculatedHitchance = static_cast<float>(((float)NumSeedsHit / 256.f) * 100.f);
		clamp(*calculatedHitchance, 0.f, 100.f);
	}

	return NumSeedsHit >= NumSeedsNeeded;
}

void GenerateTexture(std::string name, IMaterial* *mat, ITexture* *tex)
{
	// init texture
	//decrypts(0)
	std::string type = XorStr("UnlitGeneric");
	std::string matdata = "\"" + type + ("\"\n{\n\t\"") + XorStr("$basetexture\" \"") + name + ("\"\n}\n");
	std::string matName = (name + XorStr(".vmt"));
	//encrypts(0)

	// create keyvalues
	auto keyValues = new KeyValues(type.c_str());
	keyValues->LoadFromBuffer(matName.c_str(), matdata.c_str());

	// create texture
	if (keyValues)
	{
		*mat = Interfaces::MatSystem->CreateMaterial(matName.c_str(), keyValues);
		Interfaces::MatSystem->ForceBeginRenderTargetAllocation();
		*tex = Interfaces::MatSystem->CreateFullFrameRenderTarget(name.c_str());
		Interfaces::MatSystem->ForceEndRenderTargetAllocation();
	}
}

bool IsAMD() {

	enum {
		EAX,
		EBX,
		ECX,
		EDX,
	};

	int regs[4] = { 0 };
	char vendor[13];
	ZeroMemory(&vendor, 13);
	__cpuid(regs, 0);

	memcpy(vendor, &regs[EBX], 4);
	memcpy(vendor + 4, &regs[EDX], 4);
	memcpy(vendor + 8, &regs[ECX], 4);

	//decrypts(0)
	bool ret = strstr(vendor, XorStr("AMD"));
	//encrypts(0)
	return ret;
}

int IntFromChars(const std::string& str, int start)
{
	return *(int*)(str.data() + start);
}

typedescription_t* __fastcall FindFlatFieldByName(const char* name, datamap_t* map)
{
	return StaticOffsets.GetOffsetValueByType<typedescription_t*(__fastcall*)(const char*, datamap_t*)>(_FindFieldByName)(name, map);
}