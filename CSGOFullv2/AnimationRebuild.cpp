#include "precompiled.h"
#include "C_CSGameRules.h"
#include "cx_strenc.h"
#include "LocalPlayer.h"

#define EYEPOS_FIXES 1

void __fastcall ModifyEyePositionServer(CCSGOPlayerAnimState* thisptr, DWORD EDX, Vector& vecEyePos, bool FixEyePosition)
{
	CBaseEntity *Entity = thisptr->pBaseEntity;
	if (Entity && (thisptr->m_bInHitGroundAnimation || thisptr->m_fDuckAmount != 0.0f  || !Entity->GetGroundEntity()))
	{
		Interfaces::MDLCache->BeginLock();
		//decrypts(0)
		int bone = Entity->LookupBone((char*)XorStr("head_0"));
		//encrypts(0)
		if (bone != -1)
		{
			Vector vecHeadPos;

			if (FixEyePosition && Entity->IsLocalPlayer(Entity))
			{
				vecEyePos = LocalPlayer.EyePosition_Forward;
				vecHeadPos = LocalPlayer.HeadPosition_Forward;

			}
			else
			{
				QAngle vecHeadAngles;
				Entity->GetBonePositionRebuilt(bone, vecHeadPos, vecHeadAngles);
			}

			float flHeadHeight = vecHeadPos.z + 1.7f;
			if (flHeadHeight < vecEyePos.z)
			{
				float tmp = clamp((fabsf(vecEyePos.z - flHeadHeight) - 4.0f) * (1.0f / 6.0f), 0.0f, 1.0f);
				vecEyePos.z += ((flHeadHeight - vecEyePos.z) * (((powf(tmp, 2) * -2.0) * tmp) + (3.0 * powf(tmp, 2))));
			}
		}
		Interfaces::MDLCache->EndLock();
	}
}

void __fastcall ModifyEyePosition(C_CSGOPlayerAnimState* thisptr, DWORD EDX, Vector& vecEyePos, bool FixEyePosition)
{
	CBaseEntity *Entity = thisptr->pBaseEntity;
	if (thisptr->pBaseEntity && !((Interfaces::EngineClient->IsHLTV() || Interfaces::EngineClient->IsPlayingDemo()) && Interfaces::EngineClient->GetEngineBuildNumber() <= 13546))
	{
		Interfaces::MDLCache->BeginLock();
		if (!thisptr->m_bInHitGroundAnimation && thisptr->m_fDuckAmount == 0.0f && thisptr->pBaseEntity->GetGroundEntity())
		{
			thisptr->m_bIsDucked = 0; //byte 0x328
			Interfaces::MDLCache->EndLock();
			return;
		}

		//decrypts(0)
		const int bone = Entity->LookupBone((char*)XorStr("head_0"));
		//encrypts(0)

		if (bone == -1)
		{
			Interfaces::MDLCache->EndLock();
			return;
		}

		//Vector vecHeadPos;
		//Entity->GetBonePosition(bone, vecHeadPos);

		Vector vecHeadPos2;

		if (FixEyePosition && Entity->IsLocalPlayer(Entity))
		{
			vecEyePos = LocalPlayer.EyePosition_Forward;
			vecHeadPos2 = LocalPlayer.HeadPosition_Forward;
		}
		else
		{
			QAngle vecHeadAngles;
			Entity->GetBonePositionRebuilt(bone, vecHeadPos2, vecHeadAngles);
		}

		float flHeadHeight = vecHeadPos2.z + 1.7f;

		if (vecEyePos.z > flHeadHeight)
		{
			float tmp = clamp((fabsf(vecEyePos.z - flHeadHeight) - 4.0f) * (1.0f / 6.0f), 0.0f, 1.0f);
			vecEyePos.z += ((flHeadHeight - vecEyePos.z) * ((powf(tmp, 2) * 3.0) - ((powf(tmp, 2) * 2.0) * tmp)));
		}

		if (thisptr->m_fDuckAmount < 1.0f)
		{
			thisptr->m_bIsDucked = false; //byte 0x328
		}
		else if (thisptr->m_bIsDucked == true)
		{
			Vector *absorigin = Entity->GetAbsOrigin();
			float duckviewz = (*g_pGameRules)->GetViewVectors()->m_vDuckView.z;
			float duckeyeposz = duckviewz + absorigin->z;
			if (duckeyeposz <= vecEyePos.z)
			{
				thisptr->m_flEyePosZ = duckeyeposz;
			}
			else
			{
				float tmp = clamp(duckeyeposz - vecEyePos.z, 0.0f, 3.0f);
				float delta = vecEyePos.z - tmp;
				float neweyeposz = vecEyePos.z;
				if (vecEyePos.z > thisptr->m_flEyePosZ)
					neweyeposz = thisptr->m_flEyePosZ;
				if (delta <= neweyeposz)
					neweyeposz = fminf(neweyeposz, vecEyePos.z);
				thisptr->m_flEyePosZ = neweyeposz;
				vecEyePos.z = neweyeposz;
			}
		}
		else
		{
			thisptr->m_flEyePosZ = vecEyePos.z;
			thisptr->m_bIsDucked = true;
		}
		Interfaces::MDLCache->EndLock();
	}
}

#if 0
void __fastcall ModifyEyePosition_Fixed(C_CSGOPlayerAnimState* thisptr, DWORD EDX, Vector& vecEyePos, bool FixEyePosition)
{
	Vector vecEyePos_ClientModified = vecEyePos;
	ModifyEyePosition(thisptr, EDX, vecEyePos_ClientModified, FixEyePosition);
	ModifyEyePositionServer(thisptr, EDX, vecEyePos, FixEyePosition);
}
#endif