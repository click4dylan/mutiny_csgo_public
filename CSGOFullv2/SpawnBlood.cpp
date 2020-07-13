#include "precompiled.h"

///////// THIS FUNCTION IS NOT USED ANYMORE - ToDo: true?



#include "VTHook.h"
#include "SpawnBlood.h"
#include "Interfaces.h"
#include "LocalPlayer.h"
#include "VMProtectDefs.h"
#include "INetchannelInfo.h"
#include "GameMemory.h"
#include "Targetting.h"

void OnSpawnBlood(C_TEEffectDispatch* thisptr, DataUpdateType_t updateType)
{
	CBaseEntity* pEntity = thisptr->m_EffectData.m_hEntity != -1 ? Interfaces::ClientEntList->GetBaseEntityFromHandle(thisptr->m_EffectData.m_hEntity) : nullptr;
	if (pEntity && pEntity->IsPlayer())
	{
		CPlayerrecord* pCPlayer = pEntity->ToPlayerRecord();
		if (pCPlayer && pCPlayer->Impact.m_flLastBloodProcessRealTime != Interfaces::Globals->realtime)
		{
			pCPlayer->Impact.m_vecDirBlood = thisptr->m_EffectData.m_vNormal;
			pCPlayer->Impact.m_vecBloodOrigin = thisptr->m_EffectData.m_vOrigin;
			pCPlayer->Impact.m_flLastBloodProcessRealTime = Interfaces::Globals->realtime;
			pCPlayer->Impact.m_iLastBloodProcessTickCount = Interfaces::Globals->tickcount;
		}
	}
	oTE_EffectDispatch_PostDataUpdate(thisptr, updateType);
}

//char *baimbloodresolveheadposstr3 = new char[26]{ 56, 21, 30, 3, 90, 59, 19, 23, 90, 40, 31, 9, 21, 22, 12, 31, 30, 90, 50, 31, 27, 30, 42, 21, 9, 0 }; /*Body Aim Resolved HeadPos*/

#if 0
void ResolveFromImpact(CPlayerrecord* pCPlayer, CShotrecord* shotrecord, C_TEEffectDispatch* impact)
{
	auto Entity = pCPlayer->m_pInflictedEntity;
	CTickrecord *tickrecord = shotrecord->m_Tickrecord;

	//Now make sure the tick record associated with the shot record still exists
	if (!pCPlayer->FindTickRecord(tickrecord))
		return;

	QAngle absangles = { 0.0f, 0.0f, 0.0f };
	float firstyawfound;

	matrix3x4_t *original_matrix = tickrecord->m_CachedBoneMatrix;
	matrix3x4_t OriginalMatrixInverted, EndMatrix;
	int numbones = tickrecord->m_iCachedBonesCount;

	//we aren't receiving the actual hitgroup yet because this is being called before player_hurt
	int IdealHitGroup = MTargetting.HitboxToHitgroup(shotrecord->m_iActualHitbox); //shotrecord->m_iActualHitgroup; 
	int IdealHitBox = impact->m_EffectData.m_nHitBox;
	CStudioHdr *modelptr = Entity->GetModelPtr();
	studiohdr_t *hdr = modelptr->_m_pStudioHdr;
	mstudiohitboxset_t* const set = hdr->pHitboxSet(Entity->GetHitboxSet());
	//if (IdealHitBox >= set->numhitboxes)
	//	return;

	Vector impactorigin = tickrecord->m_NetOrigin + impact->m_EffectData.m_vOrigin;
	Interfaces::DebugOverlay->AddBoxOverlay(impactorigin, Vector(-0.2f, -0.2f, -0.2f), Vector(0.2f, 0.2f, 0.2f), QAngle(0.0f, 0.0f, 0.0f), 255, 0, 0, 255, 5.0f);

	Vector shotorigin = tickrecord->m_NetOrigin + impact->m_EffectData.m_vStart;
	Interfaces::DebugOverlay->AddBoxOverlay(shotorigin, Vector(-0.2f, -0.2f, -0.2f), Vector(0.2f, 0.2f, 0.2f), QAngle(0.0f, 0.0f, 0.0f), 0, 0, 255, 255, 5.0f);

	Vector vecDirOutwardFromImpactTowardsShotOrigin = shotorigin - impactorigin;
	VectorNormalizeFast(vecDirOutwardFromImpactTowardsShotOrigin);


	Interfaces::DebugOverlay->AddLineOverlay(shotorigin, shotorigin + -vecDirOutwardFromImpactTowardsShotOrigin * 256.0f, 0, 255, 0, false, 5.0f);

	Vector tracestart = shotorigin; //impactorigin + (vecDirOutwardFromImpactTowardsShotOrigin * 1.25f);
	Vector traceend = impactorigin + -(vecDirOutwardFromImpactTowardsShotOrigin * 0.5f);
	Vector traceexclude = impactorigin + (vecDirOutwardFromImpactTowardsShotOrigin * 1.0f);

	bool FoundRealHeadPosition = false; //Real head should be the exact spot the head is
	bool FoundGeneralAreaOfHead = false; //General head is an area where the head will most likely generally be. It's not an exact spot

	matrix3x4_t testmatrix[MAXSTUDIOBONES];
	matrix3x4_t firstmatrixfound[MAXSTUDIOBONES];
	matrix3x4_t *pbestmatrixfound = nullptr;

	std::vector<CSphere>m_cSpheres;
	std::vector<COBB>m_cOBBs;

	Ray_t starttoendray, impacttoexcluderay;
	starttoendray.Init(tracestart, traceend);
	impacttoexcluderay.Init(traceend, traceexclude);

	for (int i = 0; i < 360; i++)
	{
		//cast int to float to prevent accuracy loss from the for loop
		absangles.y = (float)i;

		//Transform the old matrix to the current player position for aesthetic reasons
		MatrixInvert(tickrecord->m_EntityToWorldTransform, OriginalMatrixInverted);
		MatrixCopy(tickrecord->m_EntityToWorldTransform, EndMatrix);

		AngleMatrix(absangles, EndMatrix);

		//Get a positional matrix from the current position
		PositionMatrix(tickrecord->m_NetOrigin, EndMatrix);

		//Get a relative transform
		matrix3x4_t TransformedMatrix;
		ConcatTransforms(EndMatrix, OriginalMatrixInverted, TransformedMatrix);

		for (int i = 0; i < numbones; i++)
		{
			//Now concat the original matrix with the rotated one
			ConcatTransforms(TransformedMatrix, original_matrix[i], testmatrix[i]);
												//old matrix		dest new matrix
		}

		m_cSpheres.clear();
		m_cOBBs.clear();

#if 1
		for (int i = 0; i < HITBOX_MAX; i++)
		{
			mstudiobbox_t *pbox = set->pHitbox(i);
			if (pbox->radius != -1.0f)
			{
				Vector vMin, vMax;
				VectorTransformZ(pbox->bbmin, testmatrix[pbox->bone], vMin);
				VectorTransformZ(pbox->bbmax, testmatrix[pbox->bone], vMax);
				SetupCapsule(vMin, vMax, pbox->radius, i, pbox->group, hdr->pBone(pbox->bone)->physicsbone, m_cSpheres);
			}
			else
			{
				m_cOBBs.push_back(COBB(pbox->bbmin, pbox->bbmax, pbox->angles, &testmatrix[pbox->bone], i, pbox->group, hdr->pBone(pbox->bone)->physicsbone));
			}
		}
#else
		mstudiobbox_t *pbox = set->pHitbox(IdealHitBox);
		if (pbox->radius != -1.0f)
		{
			Vector vMin, vMax;
			VectorTransformZ(pbox->bbmin, tmpmatrix[pbox->bone], vMin);
			VectorTransformZ(pbox->bbmax, tmpmatrix[pbox->bone], vMax);
			SetupCapsule(vMin, vMax, pbox->radius, IdealHitBox, pbox->group, m_cSpheres);
		}
		else
		{
			m_cOBBs.push_back(COBB(pbox->bbmin, pbox->bbmax, &tmpmatrix[pbox->bone], IdealHitBox, pbox->group));
		}
#endif

		trace_t tr;
		TRACE_HITBOX(Entity, starttoendray, tr, m_cSpheres, m_cOBBs);

		if (tr.m_pEnt && (tr.hitgroup == IdealHitGroup || (tr.hitgroup == HITBOX_HEAD && IdealHitGroup == HITGROUP_NECK)))
		{
			//Entity->DrawHitboxesFromCache(ColorRGBA(255, 0, 0, 255), 1.6f, tmpmatrix);

			//Make sure if we trace slightly out of the impact origin, we don't still hit the same thing, otherwise the angle isn't correct
			TRACE_HITBOX(Entity, impacttoexcluderay, tr, m_cSpheres, m_cOBBs);

			if (!tr.m_pEnt)
			{
				FoundRealHeadPosition = true;
				pbestmatrixfound = testmatrix;
				break;
			}

			if (!FoundGeneralAreaOfHead)
			{
				firstyawfound = absangles.y;
				memcpy(firstmatrixfound, testmatrix, sizeof(matrix3x4_t) * MAXSTUDIOBONES);
				pbestmatrixfound = firstmatrixfound;
				FoundGeneralAreaOfHead = true;
			}
		}
	}

	if (pbestmatrixfound)
	{
		const float drawsecs = 2.5f;
		Vector HeadPos = Entity->GetBonePositionCachedOnly(HITBOX_HEAD, pbestmatrixfound);
		DecStr(baimbloodresolveheadposstr3, 25);
		Interfaces::DebugOverlay->AddTextOverlay(Vector(HeadPos.x, HeadPos.y, HeadPos.z + 1.0f), drawsecs, baimbloodresolveheadposstr3);
		EncStr(baimbloodresolveheadposstr3, 25);
		int redamount = FoundRealHeadPosition ? 0 : 255; //Draw as yellow if we didn't find the exact head position
		Interfaces::DebugOverlay->AddBoxOverlay(HeadPos, Vector(-2, -2, -2), Vector(2, 2, 2), QAngle(0, 0, 0), redamount, 255, 0, 255, drawsecs);

		//pCPlayer->Impact.m_bIsBodyHitResolved = true;
		//pCPlayer->Impact.m_flResolvedYaw = absangles.y;
		pCPlayer->Impact.m_flLastBodyHitResolveTime = Interfaces::Globals->realtime;

		Entity->DrawHitboxesFromCache(ColorRGBA(redamount, 255, 0.0f, 75.0f), drawsecs, pbestmatrixfound);
	}
}
#endif

void OnSpawnImpact(C_TEEffectDispatch* thisptr, DataUpdateType_t updateType)
{
	CBaseEntity* pEntity = thisptr->m_EffectData.m_hEntity != -1 ? Interfaces::ClientEntList->GetBaseEntityFromHandle(thisptr->m_EffectData.m_hEntity) : nullptr;
	if (pEntity && pEntity->IsPlayer() && pEntity != LocalPlayer.Entity)
	{
		CPlayerrecord *pCPlayer = pEntity->ToPlayerRecord();

		//Server sends us two impact events for the same shot..
		if (pCPlayer->Impact.m_flLastImpactProcessRealTime != Interfaces::Globals->realtime)
		{
			//float flLatency = Interfaces::EngineClient->GetNetChannelInfo()->GetLatency(FLOW_OUTGOING);
			//float flTolerance = clamp(flLatency, 0.1f, 0.2f);

			//The estimated time we would have shot
			//const float flTimeShot = Interfaces::Globals->realtime - (flLatency + TICKS_TO_TIME(1));

			//The record we shot
			CShotrecord *shotrecord = nullptr;
			CPlayerrecord *_localrecord = LocalPlayer.Entity->ToPlayerRecord();


			const auto &ShotRecords = _localrecord->ShotRecords;

			for (auto& record : ShotRecords)
			{
				//If record is too old, then don't even bother with it
				if (fabsf(Interfaces::Globals->realtime - record->m_flRealTime) >= 1.0f)
					continue;

				if (record->m_pInflictedEntity == pEntity && !record->m_bTEImpactEffectAcked)
				{
					record->m_iActualHitbox = thisptr->m_EffectData.m_nHitBox;
					record->m_bTEImpactEffectAcked = true;

					shotrecord = record;

					//Interfaces::DebugOverlay->AddBoxOverlay(thisptr->m_EffectData.m_vStart + record->record.absorigin, Vector(-0.5f, -0.5f, -0.5f), Vector(0.5f, 0.5f, 0.5f), angZero, 0, 0, 255, 255, 3.5f);
					break;
				}
			}

			if (!shotrecord)
			{
				//check for manual fire
				if (fabsf(Interfaces::Globals->realtime - LocalPlayer.ManualFireShotInfo.m_flLastManualShotRealTime) < 1.0f)
					LocalPlayer.ManualFireShotInfo.m_iActualHitbox = thisptr->m_EffectData.m_nHitBox;
				else
				{
					g_QueuedImpactEvents.push_back(ImpactEventQueue_t(pEntity, Interfaces::Globals->realtime, thisptr->m_EffectData.m_nHitBox));
					if (g_QueuedImpactEvents.size() > 5)
						g_QueuedImpactEvents.pop_front();
					for (auto& ev = g_QueuedImpactEvents.begin(); ev != g_QueuedImpactEvents.end();)
					{
						if (Interfaces::Globals->realtime - ev->m_flRealTime > 1.0f)
							ev = g_QueuedImpactEvents.erase(ev);
						else
							++ev;
					}
				}
			}
			else
			{
				//ResolveFromImpact(pCPlayer, shotrecord, thisptr);
			}

			pCPlayer->Impact.m_flLastImpactProcessRealTime = Interfaces::Globals->realtime;
		}
	}
	oTE_EffectDispatch_PostDataUpdate(thisptr, updateType);
}