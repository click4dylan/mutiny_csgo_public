#include "precompiled.h"
#include "BaseEntity.h"
#include "GameMemory.h"
#include "LocalPlayer.h"
//#include "Convars.h"
#include "CPlayerrecord.h"

void __declspec(safebuffers) __fastcall HookedBuildTransformations(CBaseEntity* ent, DWORD edx, CStudioHdr* hdr, Vector* pos, Quaternion* q, const matrix3x4_t& cameraTransform, int boneMask, byte* boneComputed)
{
	// backup bone flags.
	int numbones = hdr->numbones();
	int *flags = (int*)stackalloc(numbones * sizeof(int));
	int **pflags = (int**)stackalloc(numbones * sizeof(int));

	if (!LocalPlayer.RunningFakeAngleBones)
	{
		for (int i = 0; i < hdr->numbones(); i++)
		{
			int *pointertoflags = &hdr->m_boneFlags[i];
			pflags[i] = pointertoflags;
			flags[i] = *pointertoflags;

			//override bone flags to stop jigglebones
			*pointertoflags &= ~BONE_ALWAYS_PROCEDURAL;
		}
	}

	DWORD func = NULL;
	auto rec = ent->ToPlayerRecord();
	if (rec && rec->Hooks.m_bHookedBaseEntity && (func = rec->Hooks.BaseEntity->GetOriginalHookedSub11()))
	{
		//call original
		((void(__thiscall*)(CBaseEntity* ent, CStudioHdr* hdr, Vector* pos, Quaternion* q, const matrix3x4_t& cameraTransform, int boneMask, byte* boneComputed))
			func)(ent, hdr, pos, q, cameraTransform, boneMask, boneComputed);
	}

	if (!LocalPlayer.RunningFakeAngleBones)
	{
		//restore bone flags
		for (int i = 0; i < numbones; i++)
		{
			*pflags[i] = flags[i];
		}
	}
}

void __fastcall HookedStandardBlendingRules(CBaseEntity* ent, DWORD edx, CStudioHdr* hdr, Vector pos[], QuaternionAligned q[], float currentTime, int boneMask)
{
	//Normally we would force BONE_USED_BY_HITBOX but since SetupBones is rebuilt, do nothing for now

	//Remove lean_root flags because the server doesn't use it
	int backup_flags;
	int bone = ent->LookupBone("lean_root");

	if (bone != -1)
	{
		backup_flags = hdr->m_boneFlags[bone];
		hdr->m_boneFlags[bone] = 0;
	}


	DWORD func = NULL;
	auto rec = ent->ToPlayerRecord();
	if (rec && rec->Hooks.m_bHookedBaseEntity && (func = rec->Hooks.BaseEntity->GetOriginalHookedSub12()))
	{
		((void(__thiscall*)(CBaseEntity* ent, CStudioHdr* hdr, Vector pos[], QuaternionAligned q[], float currentTime, int boneMask))
			func)(ent, hdr, pos, q, currentTime, boneMask);
	}

	if (bone != -1)
		hdr->m_boneFlags[bone] = backup_flags;
}

void __fastcall HookedAccumulateLayers(CBaseEntity* ent, DWORD edx, void* boneSetup, Vector pos[], Quaternion q[], float currentTime)
{
	C_CSGOPlayerAnimState *animstate = ent->GetPlayerAnimState();
	auto old = animstate->m_bIsReset;
	//if (!LocalPlayer.RunningFakeAngleBones)
		animstate->m_bIsReset = 1; //state + 4

	DWORD func = NULL;
	auto rec = ent->ToPlayerRecord();
	if (rec && rec->Hooks.m_bHookedBaseEntity && (func = rec->Hooks.BaseEntity->GetOriginalHookedSub13()))
	{

		((void(__thiscall*)(CBaseEntity* ent, void* boneSetup, Vector pos[], Quaternion q[], float currentTime))
			func)(ent, boneSetup, pos, q, currentTime);
	}

	animstate->m_bIsReset = old;
}

bool __fastcall HookedSetupBones(CBaseAnimating* pBaseAnimating, DWORD EDX, matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime)
{
	CBaseEntity* Entity = (CBaseEntity*)((DWORD)pBaseAnimating - 4);
	LocalPlayer.Get(&LocalPlayer);

	if (Entity->IsPlayer())
	{
		CPlayerrecord *_playerRecord = &m_PlayerRecords[Entity->index];

		if (Entity == LocalPlayer.Entity)
		{
			if (LocalPlayer.IsAlive)
			{
				if (pBoneToWorldOut)
				{
					if (boneMask == BONE_USED_BY_VERTEX_LOD0 && !LocalPlayer.IsFakeLaggingOnPeek)
					{
						matrix3x4_t * original_matrix_Real = LocalPlayer.RealAngleMatrix;
						matrix3x4_t OriginalMatrixInverted_Real, EndMatrix_Real;
						//Transform the old matrix to the current player position for aesthetic reasons
						MatrixInvert(LocalPlayer.RealAngleEntityToWorldTransform, OriginalMatrixInverted_Real);

						MatrixCopy(LocalPlayer.RealAngleEntityToWorldTransform, EndMatrix_Real);

						//Set the angles for the new matrix
						//AngleMatrix(QAngle(0.0f, LocalPlayer.LowerBodyYaw, 0.0f), EndMatrix);

						//Get a positional matrix from the current position
						PositionMatrix(Entity->GetAbsOriginDirect(), EndMatrix_Real);

						//Get a relative transform
						matrix3x4_t TransformedMatrix_Real, TransformedMatrix_Fake;
						ConcatTransforms(EndMatrix_Real, OriginalMatrixInverted_Real, TransformedMatrix_Real);

						for (int i = 0; i < nMaxBones; i++)
						{
							//Now concat the original matrix with the rotated one
							ConcatTransforms(TransformedMatrix_Real, original_matrix_Real[i], pBoneToWorldOut[i]);
																	//old matrix			  dest new matrix
						}
					}
					else
					{
						memcpy(pBoneToWorldOut, LocalPlayer.RealAngleMatrix, nMaxBones * sizeof(matrix3x4_t));
					}
				}
				return true;
			}
			else
			{
				Entity->GetLocalAngles().x = 0.0f;
				Entity->GetAngleRotation().x = 0.0f;
				Entity->GetAbsAnglesDirect().x = 0.0f;
				AllowSetupBonesToUpdateAttachments = false;
				//AllowSetupBonesToUpdateAttachments = true;

				DWORD func = NULL;
				bool ret = false;
				if (_playerRecord && _playerRecord->Hooks.m_bHookedClientRenderable && (func = _playerRecord->Hooks.ClientRenderable->GetOriginalHookedSub1()))
				{
					ret = ((bool(__thiscall*)(CBaseAnimating*, matrix3x4_t*, int, int, float))func)(pBaseAnimating, pBoneToWorldOut, nMaxBones, boneMask, currentTime);
				}
				AllowSetupBonesToUpdateAttachments = false;
				return ret;
			}
		}
		else
		{
			//if (!g_Convars.Compatibility.disable_all->GetBool())
			{
				CUtlVectorSimple* CachedBoneData = Entity->GetCachedBoneData();
				CThreadFastMutex* pBoneSetupLock = Entity->GetBoneSetupLock();

				if (*g_bInThreadedBoneSetup)
				{
					if (!pBoneSetupLock->TryLock())
					{
						// someone else is handling
#ifdef _DEBUG
						DebugBreak();
#endif
						return false;
					}
					// else, we have the lock
				}
				else
				{
					pBoneSetupLock->Lock();
				}

				int tmpMask = boneMask;
				tmpMask |= 0x80000; // HACK HACK - this is a temp fix until we have accessors for bones to find out where problems are.

				int nLOD = 0;
				int nMask = BONE_USED_BY_VERTEX_LOD0;
				for (; nLOD < MAX_NUM_LODS; ++nLOD, nMask <<= 1)
				{
					if (tmpMask & nMask)
						break;
				}
				for (; nLOD < MAX_NUM_LODS; ++nLOD, nMask <<= 1)
				{
					tmpMask |= nMask;
				}

#if 1
				if (CachedBoneData->count == 0)
				{
					//possible cause for fps drops
					//Entity->InvalidateBoneCache();
					//Entity->SetLastOcclusionCheckFlags(0);
					//Entity->SetLastOcclusionCheckFrameCount(Interfaces::Globals->framecount);
					pBoneSetupLock->Unlock();
					Entity->AddEffect(EF_NOINTERP);
					bool ret = Entity->SetupBones(pBoneToWorldOut, nMaxBones, boneMask, currentTime);
					Entity->RemoveEffect(EF_NOINTERP);
					return ret;
				}
#endif

				//CachedBoneData->count = pCPlayer->iCachedBonesCount;
				//memcpy(CachedBoneData->Base(), pCPlayer->CachedBoneMatrixes, sizeof(matrix3x4_t) * pCPlayer->iCachedBonesCount);

				if (!pBoneToWorldOut || nMaxBones == -1)
				{
					pBoneSetupLock->Unlock();
					return true;
				}

				if ((unsigned int)nMaxBones >= CachedBoneData->count)
				{
					memcpy((void*)pBoneToWorldOut, CachedBoneData->Base(), sizeof(matrix3x4_t) * CachedBoneData->Count());
					pBoneSetupLock->Unlock();
					return true;
				}
				else
				{
#ifdef _DEBUG
					printf("HookedSetupBones: invalid bone array size(%d - needs %d)\n", nMaxBones, CachedBoneData->count);
					//DebugBreak();
#endif
				}

				pBoneSetupLock->Unlock();
				return false;
			}
		}
		DWORD func = NULL;
		if (_playerRecord && _playerRecord->Hooks.m_bHookedClientRenderable && (func = _playerRecord->Hooks.ClientRenderable->GetOriginalHookedSub1()))
			return ((bool(__thiscall*)(CBaseAnimating*, matrix3x4_t*, int, int, float))func)(pBaseAnimating, pBoneToWorldOut, nMaxBones, boneMask, currentTime);

		return false;
	}
#if 0
	else if (Entity->GetOwner() == LocalPlayer.Entity || Entity->GetMoveParent() == LocalPlayer.Entity)
	{
		if (pBoneToWorldOut)
		{
			if (oSetupBones(pBaseAnimating, pBoneToWorldOut, nMaxBones, boneMask, currentTime))
			{
				//must be a weapon the player is holding
				auto parent = Entity->GetMoveParent();

				matrix3x4_t OriginalMatrixInverted, EndMatrix;
				auto cache = Entity->GetCachedBoneData();
				int numbones = cache->Count();

				//Transform the old matrix to the current player position for aesthetic reasons
				MatrixInvert(Entity->EntityToWorldTransform(), OriginalMatrixInverted);
				MatrixCopy(Entity->EntityToWorldTransform(), EndMatrix);

				//Get relative position from parent position
				Vector relative_position = Entity->GetAbsOriginDirect() - LocalPlayer.LastAnimatedOrigin;

				//Get a positional matrix from the current position of the parent + the relative position of the following entity
				PositionMatrix(parent->GetAbsOriginDirect() + relative_position, EndMatrix);

				//Get a relative transform
				matrix3x4_t TransformedMatrix;
				ConcatTransforms(EndMatrix, OriginalMatrixInverted, TransformedMatrix);

				for (int i = 0; i < numbones; i++)
				{
					//Now concat the original matrix with the rotated one
					ConcatTransforms(TransformedMatrix, pBoneToWorldOut[i], pBoneToWorldOut[i]);
					//old matrix		dest new matrix
				}

				return true;
			}
			return false;
		}
	}
#endif

	return oSetupBones(pBaseAnimating, pBoneToWorldOut, nMaxBones, boneMask, currentTime);
}

bool __fastcall HookedTestHitboxes(CBaseEntity* me, DWORD edx, const Ray_t& ray, uint32_t mask, CGameTrace& trace)
{
	auto rec = me->ToPlayerRecord();
	DWORD func;
	if (rec && rec->Hooks.m_bHookedBaseEntity && (func = rec->Hooks.BaseEntity->GetOriginalHookedSub14()))
	{
		bool result = ((bool(__thiscall*)(CBaseEntity* , const Ray_t& , uint32_t , CGameTrace& )) func)(me, ray, mask, trace);

		return result;
	}
	return false;
}