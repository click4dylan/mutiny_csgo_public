#include "precompiled.h"
#include "BaseEntity.h"
#include "GameMemory.h"
#include "LocalPlayer.h"
#include "UsedConvars.h"
#include "cx_strenc.h"
#include "VMProtectDefs.h"
#include "C_CSGameTypes.h"

#include "Adriel/stdafx.hpp"

extern void __fastcall ModifyEyePosition_Fixed(C_CSGOPlayerAnimState* thisptr, DWORD EDX, Vector& vecEyePos, bool FixEyePosition);
extern void __fastcall ModifyEyePositionServer(CCSGOPlayerAnimState* thisptr, DWORD EDX, Vector& vecEyePos, bool FixEyePosition);
//char *basecalcviewsigstr = new char[179]{ 79, 79, 90, 90, 66, 56, 90, 90, 63, 57, 90, 90, 79, 76, 90, 90, 66, 56, 90, 90, 60, 75, 90, 90, 79, 77, 90, 90, 66, 56, 90, 90, 66, 63, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 66, 73, 90, 90, 60, 67, 90, 90, 60, 60, 90, 90, 77, 78, 90, 90, 73, 63, 90, 90, 74, 60, 90, 90, 56, 77, 90, 90, 57, 75, 90, 90, 57, 75, 90, 90, 63, 74, 90, 90, 74, 78, 90, 90, 74, 79, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 57, 75, 90, 90, 63, 67, 90, 90, 75, 74, 90, 90, 73, 67, 90, 90, 78, 66, 90, 90, 74, 78, 90, 90, 77, 79, 90, 90, 72, 56, 90, 90, 66, 56, 90, 90, 74, 66, 90, 90, 66, 79, 90, 90, 57, 67, 90, 90, 77, 78, 90, 90, 72, 79, 90, 90, 66, 56, 90, 90, 74, 75, 0 }; /*55  8B  EC  56  8B  F1  57  8B  8E  ??  ??  ??  ??  83  F9  FF  74  3E  0F  B7  C1  C1  E0  04  05  ??  ??  ??  ??  C1  E9  10  39  48  04  75  2B  8B  08  85  C9  74  25  8B  01*/
//DWORD BaseCalcView = NULL;

//C_CSPlayer::CalcView
void __fastcall HookedCalcView(CBaseEntity* _Entity, DWORD edx, Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov)
{
	//((void(__thiscall*)(CBaseEntity*, Vector &, QAngle &, float &, float &, float &))AllPlayers[_Entity->index].PersistentData.HookedBaseEntity->GetOriginalHookedSub9())(_Entity, eyeOrigin, eyeAngles, zNear, zFar, fov);

	//static DWORD m_bThirdPersonOffset;
	//if (!BaseCalcView)
	//{
	//	DecStr(basecalcviewsigstr, 178);
	//	BaseCalcView = FindMemoryPattern(ClientHandle, basecalcviewsigstr, 178);
	//	EncStr(basecalcviewsigstr, 178);
	//	delete[]basecalcviewsigstr;
	//	//m_bThirdPersonOffset = FindMemoryPattern(ClientHandle, (char*)charenc("80  BE  ??  ??  ??  ??  ??  74  ??  8B  06  8B  CE  8B  80  ??  ??  ??  ??  FF  D0  84  C0  75  ??  38"), strlen(charenc("80  BE  ??  ??  ??  ??  ??  74  ??  8B  06  8B  CE  8B  80  ??  ??  ??  ??  FF  D0  84  C0  75  ??  38")));
	//	if (!BaseCalcView /*|| !m_bThirdPersonOffset*/)
	//	{
	//		THROW_ERROR(ERR_CANT_FIND_BASE_CALCVIEW_SIGNATURE);
	//		exit(EXIT_SUCCESS);
	//	}
	//	//m_bThirdPersonOffset = (*(DWORD*)((DWORD)m_bThirdPersonOffset + 2));
	//}

	//Call baseclass
	StaticOffsets.GetOffsetValueByType<void(__thiscall*)(CBaseEntity*, Vector&, QAngle&, float&, float&, float&)>(_BaseCalcView)(_Entity, eyeOrigin, eyeAngles, zNear, zFar, fov);
	//((void(__thiscall*)(CBaseEntity*, Vector &, QAngle &, float &, float &, float &))BaseCalcView)(_Entity, eyeOrigin, eyeAngles, zNear, zFar, fov);

	//Stop view lag when antiaiming or fakelagging
	if (_Entity->IsLocalPlayer() && !variable::get().ragebot.b_antiaim)
	{
		// Something is still overriding the local player's abs angles other than RunCommand, so just restore it here..
		QAngle _FixedAngles = { 0.0f, LocalPlayer.m_goalfeetyaw, 0.0f };
		_Entity->SetAbsAngles(_FixedAngles);
		_Entity->SetLocalAngles(_FixedAngles);

		if (_Entity->GetShouldUseAnimationEyeOffset() && _Entity->GetPlayerAnimState())
		{
			if (/**(bool*)((DWORD)_Entity + m_bThirdPersonOffset)*/ _Entity->GetPredictable() && _Entity->GetAliveVMT()) //cl_camera_height_restriction_debug)
			{
				CPlayerrecord *record = _Entity->ToPlayerRecord();
				if (record && record->m_pAnimStateServer[ResolveSides::NONE] && record->m_pAnimStateServer[ResolveSides::NONE]->pBaseEntity == _Entity)
					ModifyEyePositionServer(_Entity->ToPlayerRecord()->m_pAnimStateServer[ResolveSides::NONE], 0, eyeOrigin, false);
				//ModifyEyePosition_Fixed(_Entity->GetPlayerAnimState(), 0, eyeOrigin, false);
			}
			else
			{
				if (_Entity->GetObserverMode() == OBS_MODE_IN_EYE)
				{
					C_BaseEntity *LocalPlayer = Interfaces::ClientEntList->GetBaseEntity(Interfaces::EngineClient->GetLocalPlayer());
					if (LocalPlayer && LocalPlayer->GetAliveVMT())
					{
						CPlayerrecord *record = _Entity->ToPlayerRecord();
						if (record && record->m_pAnimStateServer[ResolveSides::NONE] && record->m_pAnimStateServer[ResolveSides::NONE]->pBaseEntity == _Entity)
							ModifyEyePositionServer(_Entity->ToPlayerRecord()->m_pAnimStateServer[ResolveSides::NONE], 0, eyeOrigin, false);
						//ModifyEyePosition_Fixed(LocalPlayer->GetPlayerAnimState(), 0, eyeOrigin, false);
					}
				}
			}
		}
	}
	
	if (LocalPlayer.IsFakeDucking)
	{
		if (Interfaces::Input->m_fCameraInThirdPerson) // fix the up and down bob while fake ducking
			eyeOrigin.z = LocalPlayer.Entity->GetAbsOrigin()->z + 64.f;
	}

	CBaseCombatWeapon *weapon = _Entity->GetActiveCSWeapon();
	if (weapon)
	{
		void* controller = weapon->GetIronSightController();
		if (controller && *(BYTE*)(DWORD)controller)
		{
			float flNewFOV = (float)_Entity->GetDefaultFOV();
			//static DWORD IsInIronSightFunc = NULL;
			//if (!IsInIronSightFunc)
			//{
			//	IsInIronSightFunc = FindMemoryPattern(ClientHandle, (char*)XorStr("53  56  8B  F1  57  8B  4E  3C  85  C9  0F  84  ??  ??  ??  ??  8B  81"), strlen((char*)XorStr("53  56  8B  F1  57  8B  4E  3C  85  C9  0F  84  ??  ??  ??  ??  8B  81")));
			//	if (!IsInIronSightFunc)
			//	{
			//		THROW_ERROR(ERR_CANT_FIND_ISINIRONSIGHT_SIGNATURE);
			//		exit(EXIT_SUCCESS);
			//	}
			//}
			bool bIsInIronSight = StaticOffsets.GetOffsetValueByType<bool(__thiscall*)(void*)>(_IsInIronsight)(controller); //((bool(__thiscall*)(void*))IsInIronSightFunc)(controller);
			if (bIsInIronSight)
			{
				flNewFOV += (float)((float)(*(float *)((DWORD)controller + 52) - flNewFOV) * *(float *)((DWORD)controller + 36));
			}
			fov = flNewFOV;
		}
	}

	//Removed in operation Nov 18, 2019
#if 0
	if (_Entity->GetUnknownSurvivalBool())
	{
		_Entity->SetUnknownSurvivalBool(0);
		if (_Entity == GetHudPlayer())
		{
			*StaticOffsets.GetOffsetValueByType<int*>(_UselessCalcViewSurvivalBool) = 0;
			Interfaces::Input->CAM_ToFirstPerson();
			_Entity->ThirdPersonSwitch(0);
		}
	}
#endif


	auto gamerules = GetGamerules();
	if (gamerules)
	{
		auto gametypes = GetGameTypes();
		if ((gametypes->GetCurrentGameType() == 6 && !gametypes->GetCurrentGameMode()) || mp_coopmission_dz.GetVar()->GetBool())
		{
			auto customrules = gamerules->GetCustomizedGameRules();
			if (customrules)
			{
				SurvivalCalcView(customrules, _Entity, eyeOrigin, eyeAngles);
			}
		}
	}

	//added in operation Nov 18, 2019
	_Entity->DelayUnscope(fov);

	if (_Entity->IsAutoMounting() > 0)
	{
		auto vectors = gamerules->GetViewVectors();
		auto currentautomoveorigin = vectors->m_vDuckView + _Entity->GetAutoMoveOrigin();
		auto automovetargetend = vectors->m_vDuckHullMax + _Entity->GetAutomoveTargetEnd();

		float time = (Interfaces::Globals->curtime - (_Entity->GetAutomoveTargetTime() - (_Entity->GetAutomoveTargetTime() - _Entity->GetAutomoveStartTime())))
			/ (_Entity->GetAutomoveTargetTime() - _Entity->GetAutomoveStartTime());
		time = clamp(time, 0.0f, 1.0f);
		float fuck = powf(time, 0.6214906f);
		fuck = fminf(fuck + 0.5f, 1.0f);
		eyeOrigin = (automovetargetend - currentautomoveorigin) * fuck + currentautomoveorigin;
	}

	LocalPlayer.LastCameraPosition = eyeOrigin;
}