#include "Adriel/config.hpp"
#include "Adriel/adr_util.hpp"
#include "LocalPlayer.h"
#include "UsedConvars.h"
#include "Events.h"
#include "CreateMove.h"
#include "Eventlog.h"

//FIXME: TODO: separate the two things this function does into two separate function - fill in the tempentity impact results, and body hit resolver
void FillImpactTempEntResultsAndResolveFromPlayerHurt(CBaseEntity* EntityHit, CPlayerrecord* pCPlayer, CShotrecord* shotrecord, bool CalledFromQueuedEventProcessor)
{
	if (!pCPlayer)
		return;

	auto& var = variable::get();

	bool _DoNotResolve = (!var.ragebot.b_enabled || !var.ragebot.b_resolver_experimental || !var.ragebot.b_resolver || shotrecord->m_bForwardTracked)
		|| pCPlayer->m_bLegit || pCPlayer->m_bIsBot || !pCPlayer->m_bAllowBodyHitResolver;

	// don't resolve teammates via body hit
	if (LocalPlayer.Entity && EntityHit && Interfaces::ClientEntList->EntityExists(EntityHit))
	{
		bool is_enemy = false;
		if (EntityHit->IsPlayer())
		{
			if (LocalPlayer.Entity->GetTeam() != EntityHit->GetTeam())
				is_enemy = true;

			if (mp_teammates_are_enemies.GetVar()->GetInt() == 1)
				is_enemy = true;
		}

		if (EntityHit == LocalPlayer.Entity || !is_enemy)
		{
			if (!_DoNotResolve)
			{
				if (EntityHit == LocalPlayer.Entity)
				{
#if defined DEBUG_BH || defined _DEBUG || defined INTERNAL_DEBUG
					g_Eventlog.AddEventElement(0x1, "[BH] skip, player was you");
					g_Eventlog.OutputEvent(variable::get().visuals.i_shot_logs);
#endif
				}
				else if (!EntityHit->IsEnemy(LocalPlayer.Entity))
				{
#if defined DEBUG_BH || defined _DEBUG || defined INTERNAL_DEBUG
					g_Eventlog.AddEventElement(0x1, "[BH] skip, player was a teammate");
					g_Eventlog.OutputEvent(variable::get().visuals.i_shot_logs);
#endif
				}
			}
			return;
		}
	}

	if (!shotrecord)
	{
		//ResolveFromPlayerHurt_ManualShot(pCPlayer, shotrecord);
		return;
	}

#if 1
	//Don't recursively add queues!
	if (!CalledFromQueuedEventProcessor)
	{
		g_QueuedImpactResolveEvents.emplace_back(EntityHit, pCPlayer, *shotrecord);
		return;
	}
#else
	if (!pCPlayer->LastShot.m_bReceivedExactImpactPos)
	{
		//we didn't receive an exact impact position yet, queue this in case we do in the future

		//Don't recursively add queues!
		if (!CalledFromQueuedEventProcessor)
			g_QueuedImpactResolveEvents.emplace_back(EntityHit, pCPlayer, *shotrecord);

		return;
	}
#endif

	START_PROFILING

		auto Entity = EntityHit;
	matrix3x4_t *original_matrix;
	//we aren't receiving the actual hitgroup yet because this is being called before player_hurt
	int IdealHitGroup, IdealHitBox;
	float body_yaw, original_yaw, original_eye_yaw, original_lby, speed;
	CTickrecord *tickrecord = nullptr;
	PlayerBackup_t* backupstate = nullptr;
	bool _isInflictor = shotrecord->m_bIsInflictor;

	CPlayerrecord *srcPlayer = _isInflictor ? shotrecord->m_pInflictedEntity->ToPlayerRecord() : pCPlayer;
	if (_isInflictor)
	{
		//don't allow body hit resolves from legit players because of server angle lerping
		if (srcPlayer->m_bLegit)
		{
			END_PROFILING
				return;
		}
	}

#if defined DEBUG_BH || defined _DEBUG || defined INTERNAL_DEBUG
	if (!_DoNotResolve)
	{
		g_Eventlog.AddEventElement(0x1, "[BH] started");
		g_Eventlog.OutputEvent(variable::get().visuals.i_shot_logs);
	}
#endif

	CPlayerrecord::s_Shot &LastShot = _isInflictor ? srcPlayer->LastShotAsEnemy : srcPlayer->LastShot;
	if (!LastShot.m_bReceivedExactImpactPos)
	{
#if defined DEBUG_BH || defined _DEBUG || defined INTERNAL_DEBUG
		if (!_DoNotResolve)
		{
			g_Eventlog.AddEventElement(0x7, "[BH] never received exact impact pos");
			g_Eventlog.OutputEvent(variable::get().visuals.i_shot_logs);
		}
#endif
		END_PROFILING
			return;
	}

	const Vector& impactorigin = LastShot.m_vecMainImpactPos;
	//Now make sure the tick record associated with the shot record still exists
	if (!_isInflictor)
	{
		tickrecord = shotrecord->m_Tickrecord;
		if (!tickrecord || !pCPlayer->FindTickRecord(tickrecord))
		{
#if defined DEBUG_BH || defined _DEBUG || defined INTERNAL_DEBUG
			if (!_DoNotResolve)
			{
				g_Eventlog.AddEventElement(0x7, "[BH] failed to find enemy tick record");
				g_Eventlog.OutputEvent(variable::get().visuals.i_shot_logs);
			}
#endif
			END_PROFILING
				return;
		}
	}
	else
	{
		tickrecord = shotrecord->m_Tickrecord;
		if (!tickrecord || !shotrecord->m_pInflictorEntity->ToPlayerRecord()->FindTickRecord(tickrecord))
		{
#if defined DEBUG_BH || defined _DEBUG || defined INTERNAL_DEBUG
			if (!_DoNotResolve)
			{
				g_Eventlog.AddEventElement(0x7, "[BH] failed to find inflictor tick record");
				g_Eventlog.OutputEvent(variable::get().visuals.i_shot_logs);
			}
#endif
			END_PROFILING
				return;
		}

		CTickrecord *closestrecord = nullptr;
		//find the closest record of the enemy they shot at to the tick the inflictor shot from
		int bestdelta;
		for (auto& tick : pCPlayer->m_Tickrecords)
		{
			int dt = abs(tick->m_iServerTick - tickrecord->m_iServerTick);
			if (abs(tick->m_iServerTick - tickrecord->m_iServerTick) < 15)
			{
				if (!closestrecord || dt < bestdelta)
				{
					closestrecord = tick;
					bestdelta = dt;
					if (dt == 0)
						break;
				}
			}
		}

		if (!closestrecord)
		{
#if defined DEBUG_BH || defined _DEBUG || defined INTERNAL_DEBUG
			if (!_DoNotResolve)
			{
				g_Eventlog.AddEventElement(0x7, "[BH] failed to find inflicted player tickrecord");
				g_Eventlog.OutputEvent(variable::get().visuals.i_shot_logs);
			}
#endif
			END_PROFILING
				return;
		}
		//fix up the correct data so the rest of the function works
		tickrecord = closestrecord;
	}

#ifdef _DEBUG
	Interfaces::DebugOverlay->AddBoxOverlay(impactorigin, Vector(-1.5, -1.5, -1.5), Vector(1.5, 1.5, 1.5), angZero, 0, 255, 0, 255, 3.0f);
#endif
	original_matrix = tickrecord->m_PlayerBackup.CachedBoneMatrices;
	Vector localeyepos = shotrecord->m_vecLocalEyePos;
	if (_isInflictor)
	{
		if (!shotrecord->m_bTEImpactEffectAcked)
		{
			for (auto& ev = g_QueuedImpactEvents.begin(); ev != g_QueuedImpactEvents.end(); ++ev)
			{
				if (ev->m_EntityHit == EntityHit && Interfaces::Globals->realtime == ev->m_flRealTime)
				{
					shotrecord->m_bTEImpactEffectAcked = true;
					shotrecord->m_iActualHitbox = ev->m_iActualHitbox;
					g_QueuedImpactEvents.erase(ev);
					break;
				}
			}
		}
		if (pCPlayer->Impact.m_flLastBloodProcessRealTime == Interfaces::Globals->realtime
			&& (pCPlayer->Impact.m_vecBloodOrigin - impactorigin).Length() < 10.0f)
		{
			//so you don't have to trace as far, the trace start pos is closer to the blood impact event
			localeyepos = impactorigin + pCPlayer->Impact.m_vecDirBlood * 200.0f;
		}
	}

	if (_DoNotResolve)
	{
		END_PROFILING
			return;
	}

	IdealHitBox = shotrecord->m_bTEImpactEffectAcked ? shotrecord->m_iActualHitbox : HITBOX_MAX;
	IdealHitGroup = shotrecord->m_iActualHitgroup;
	Vector& networkorigin = tickrecord->m_NetOrigin;
	body_yaw = tickrecord->m_flPoseParams[11];
	original_yaw = tickrecord->m_AbsAngles.y;
	original_eye_yaw = AngleNormalize(tickrecord->m_EyeYaw);
	original_lby = tickrecord->m_flLowerBodyYaw;
	speed = tickrecord->m_Velocity.Length();

	backupstate = new PlayerBackup_t(Entity);
	if (!backupstate)
	{
		END_PROFILING
			return;
	}

#if defined DEBUG_BH || defined _DEBUG || defined INTERNAL_DEBUG
	std::string victim_name = adr_util::sanitize_name((char*)EntityHit->GetName().data());
	g_Eventlog.AddEventElement(0x1, "[BH] Attempting to resolve " + victim_name);
	g_Eventlog.OutputEvent(variable::get().visuals.i_shot_logs);
#endif

	if (tickrecord->m_bFiredBullet)
	{
#if defined DEBUG_BH || defined _DEBUG || defined INTERNAL_DEBUG
		g_Eventlog.AddEventElement(0x7, "[BH] was skipped, enemy was firing");
		g_Eventlog.OutputEvent(variable::get().visuals.i_shot_logs);
#endif
		delete backupstate;
		END_PROFILING
			return;
	}

	QAngle absangles = { 0.0f, 0.0f, 0.0f };
	float firstyawfound;
	float original_eye_yaw_positive = AngleNormalizePositive(original_eye_yaw);

	CStudioHdr *modelptr = Entity->GetModelPtr();
	studiohdr_t *hdr = modelptr->_m_pStudioHdr;
	mstudiohitboxset_t* const set = hdr->pHitboxSet(Entity->GetHitboxSet());
	//mstudiobbox_t *thorax = set->pHitbox(HITBOX_THORAX);
	//QAngle original_thorax_angles;
	//MatrixAngles(original_fake_matrix[thorax->bone], original_thorax_angles);
	//QAngle fake_head_angle = CalcAngle(networkorigin, Entity->GetBonePositionCachedOnly(HITBOX_HEAD, original_fake_matrix));

	//Interfaces::DebugOverlay->AddBoxOverlay(impactorigin, Vector(-0.2f, -0.2f, -0.2f), Vector(0.2f, 0.2f, 0.2f), QAngle(0.0f, 0.0f, 0.0f), 255, 0, 0, 255, 5.0f);
	//Interfaces::DebugOverlay->AddBoxOverlay(localeyepos, Vector(-0.2f, -0.2f, -0.2f), Vector(0.2f, 0.2f, 0.2f), QAngle(0.0f, 0.0f, 0.0f), 0, 0, 255, 255, 5.0f);

	Vector vecDirOutwardFromImpactTowardsShotOrigin = localeyepos - impactorigin;
	VectorNormalizeFast(vecDirOutwardFromImpactTowardsShotOrigin);

	Vector traceend = impactorigin;// +(-vecDirOutwardFromImpactTowardsShotOrigin * 1.0f);
	float lenience = variable::get().ragebot.f_resolver_experimental_leniency_impact;
	if (lenience > 0.0f)
		traceend += -vecDirOutwardFromImpactTowardsShotOrigin * lenience;
	const Vector traceexclude = impactorigin + (vecDirOutwardFromImpactTowardsShotOrigin * variable::get().ragebot.f_resolver_experimental_leniency_exclude);
	const Vector pseudoup = { 0.0f, 0.0f, 1.0f };

	bool FoundRealHeadPosition = false; //Real head should be the exact spot the head is
	bool FoundGeneralAreaOfHead = false; //General head is an area where the head will most likely generally be. It's not an exact spot

	static matrix3x4_t testmatrix[MAXSTUDIOBONES];
	static matrix3x4_t bestmatrix[MAXSTUDIOBONES];
	matrix3x4_t *pbestmatrixfound = nullptr;
	float best_head_angle_diff = 0.f;
	float general_head_angle_diff = 0.f;

	std::vector<CSphere>m_cSpheres;
	std::vector<COBB>m_cOBBs;

	const Ray_t starttoendray((Vector)localeyepos, (Vector)traceend);
	const Ray_t impacttoexcluderay((Vector)localeyepos, (Vector)traceexclude);

#ifdef _DEBUG
	if (!Entity->GetAlive())
	{
		//Interfaces::DebugOverlay->AddLineOverlay(localeyepos, traceend, 0, 255, 0, true, 3.0f);
	}
#endif

	CTickrecord *oldtickrecord = nullptr;
	for (auto& tick : pCPlayer->m_Tickrecords)
	{
		if (tick->m_iServerTick < tickrecord->m_iServerTick)
		{
			oldtickrecord = tick;
			break;
		}
	}

	if (!oldtickrecord)
	{
#if defined DEBUG_BH || defined _DEBUG || defined INTERNAL_DEBUG
		g_Eventlog.AddEventElement(0x7, "[BH] couldn't find oldtickrecord");
		g_Eventlog.OutputEvent(variable::get().visuals.i_shot_logs);
#endif
		delete backupstate;
		END_PROFILING
			return;
	}

	CCSGOPlayerAnimState* animstate = tickrecord->m_pAnimStateServer[tickrecord->m_iResolveSide];
	if (!animstate)
	{
#if defined DEBUG_BH || defined _DEBUG || defined INTERNAL_DEBUG
		g_Eventlog.AddEventElement(0x7, "[BH] couldn't find tickrecord->m_pAnimStateServer");
		g_Eventlog.OutputEvent(variable::get().visuals.i_shot_logs);
#endif
		delete backupstate;
		END_PROFILING
			return;
	}

	float flDuckSpeedClamp, flRunningSpeedClamp, flLerped, flSpeed, flMaxYaw, flMinYaw, flLadderCycle;
	float flBestDist = 65535.0f;
	float flBestDist_General = 65535.0f;
	bool bOnLadder;
	bool bAlive = Entity->GetAlive();
	Vector &vVelocity = animstate->m_vVelocity;
	float velAngle = (atan2(-vVelocity.y, -vVelocity.x) * 180.0f) * (1.0f / M_PI);
	if (velAngle < 0.0f)
		velAngle += 360.0f;

	flDuckSpeedClamp = clamp(animstate->m_flDuckingSpeed, 0.0f, 1.0f);
	flRunningSpeedClamp = clamp(animstate->m_flRunningSpeed, 0.0f, 1.0f);
	flLerped = ((flDuckSpeedClamp - flRunningSpeedClamp) * animstate->m_fDuckAmount) + flRunningSpeedClamp;
	flSpeed = animstate->m_flSpeed;
	flMaxYaw = animstate->m_flMaxYaw;
	flMinYaw = animstate->m_flMinYaw;
	flLadderCycle = animstate->m_flLadderCycle;
	bOnLadder = animstate->m_bOnLadder;

	float flBiasMove = Bias(flLerped, 0.18f);
	float m_flCurrentMoveDirGoalFeetDelta, m_flGoalMoveDirGoalFeetDelta, _m_flCurrentMoveDirGoalFeetDelta, _m_flGoalMoveDirGoalFeetDelta;

	CCSGOPlayerAnimState* oldanimstate = oldtickrecord->m_pAnimStateServer[oldtickrecord->m_iResolveSide];

	if (!oldanimstate)
	{
#if defined DEBUG_BH || defined _DEBUG || defined INTERNAL_DEBUG
		g_Eventlog.AddEventElement(0x7, "[BH] couldn't find oldtickrecord->m_pAnimStateServer");
		g_Eventlog.OutputEvent(variable::get().visuals.i_shot_logs);
#endif
		delete backupstate;
		END_PROFILING
			return;
	}

	_m_flCurrentMoveDirGoalFeetDelta = oldanimstate->m_flCurrentMoveDirGoalFeetDelta;
	_m_flGoalMoveDirGoalFeetDelta = oldanimstate->m_flGoalMoveDirGoalFeetDelta;

	//Backtrack the player to the record we shot at

	if (Entity != tickrecord->m_PlayerBackup.Entity)
	{
#if defined DEBUG_BH || defined _DEBUG || defined INTERNAL_DEBUG
		g_Eventlog.AddEventElement(0x7, "[BH] entity didn't match");
		g_Eventlog.OutputEvent(variable::get().visuals.i_shot_logs);
#endif
		//entity is no longer valid
		delete backupstate;
		END_PROFILING
			return;
	}
	if (Entity->GetRefEHandle().ToUnsignedLong() != tickrecord->m_PlayerBackup.RefEHandle)
	{
#if defined DEBUG_BH || defined _DEBUG || defined INTERNAL_DEBUG
		g_Eventlog.AddEventElement(0x7, "[BH] ehandle didn't match");
		g_Eventlog.OutputEvent(variable::get().visuals.i_shot_logs);
#endif
		//entity is no longer valid
		delete backupstate;
		END_PROFILING
			return;
	}

	int numtimes = 0;
restart:
	tickrecord->m_PlayerBackup.RestoreData();

	bool bStrafing = Entity->IsStrafing();
	bool bRunLadderPose = bOnLadder || flLadderCycle > 0.0f;
	bool bMoving = flSpeed > 0.0f;
	QAngle angles;
	Vector LadderNormal = Entity->GetVecLadderNormal();
	VectorAngles(animstate->m_vecSetupLeanVelocityInterpolated, pseudoup, angles);
	float BestAbsYaw;
	int HitgroupHit;
	float layer7_weight = Entity->GetAnimOverlayDirect(7)->m_flWeight;
	float maxdesyncdelta = tickrecord->m_flAbsMaxDesyncDelta;//fminf(tickrecord->m_flAbsMaxDesyncDelta + 2.0f, 58.0f);

	if (maxdesyncdelta == 0.0f)
		maxdesyncdelta = Entity->GetMaxDesyncDelta(tickrecord);

	const int nTimes = (maxdesyncdelta - -maxdesyncdelta) / 0.5f + 1;

	for (int iter = 0; iter < nTimes; ++iter)
	{
		float i = -maxdesyncdelta + iter * 0.5f;
		//cast int to float to prevent accuracy loss from the for loop
		absangles.y = AngleNormalizePositive(original_eye_yaw + (float)i);

		m_flCurrentMoveDirGoalFeetDelta = _m_flCurrentMoveDirGoalFeetDelta;
		m_flGoalMoveDirGoalFeetDelta = _m_flGoalMoveDirGoalFeetDelta;

		if (bMoving)
			m_flGoalMoveDirGoalFeetDelta = AngleNormalize(AngleDiff(velAngle, absangles.y));

		float m_flFeetVelDirDelta = AngleNormalize(AngleDiff(m_flGoalMoveDirGoalFeetDelta, m_flCurrentMoveDirGoalFeetDelta));
		m_flCurrentMoveDirGoalFeetDelta = layer7_weight >= 1.0f ? m_flGoalMoveDirGoalFeetDelta : AngleNormalize(((flBiasMove + 0.1f) * m_flFeetVelDirDelta) + m_flCurrentMoveDirGoalFeetDelta);
		Entity->SetPoseParameter(7, m_flCurrentMoveDirGoalFeetDelta); //move_yaw

		float new_body_yaw_pose;
		float eye_goalfeet_delta = AngleDiff(original_eye_yaw, absangles.y);
		if (eye_goalfeet_delta < 0.0f)
			new_body_yaw_pose = (eye_goalfeet_delta / flMinYaw) * -58.0f;
		else
			new_body_yaw_pose = (eye_goalfeet_delta / flMaxYaw) * 58.0f;

		Entity->SetAbsAnglesDirect(absangles); //fix the abs angles to the goalfeetyaw
		Entity->SetLocalAnglesDirect(absangles);
		Entity->SetEFlags(Entity->GetEFlags() & ~EFL_DIRTY_ABSTRANSFORM);
		Entity->SetPoseParameter(11, new_body_yaw_pose); //fix the body yaw pose parameter

		if (bStrafing)
			Entity->SetPoseParameter(14, AngleNormalize(m_flCurrentMoveDirGoalFeetDelta));

		if (bRunLadderPose)
		{
			QAngle ladderAngles;
			VectorAngles(LadderNormal, ladderAngles);
			Entity->SetPoseParameter(4, AngleDiff(ladderAngles.y, absangles.y));
		}

		Entity->SetPoseParameter(2, AngleNormalize(absangles.y - angles.y)); //lean_yaw

		AllowSetupBonesToUpdateAttachments = false;
		Entity->InvalidateBoneCache();
		Entity->SetLastOcclusionCheckFlags(0);
		Entity->SetLastOcclusionCheckFrameCount(Interfaces::Globals->framecount);
		Entity->ReevaluateAnimLOD();
		Entity->SetupBonesRebuilt(testmatrix, MAXSTUDIOBONES, BONE_USED_BY_HITBOX, Interfaces::Globals->curtime);

		trace_t tr;
		Vector exitpos;
		Interfaces::EngineTrace->ClipRayToEntity(starttoendray, MASK_SHOT, (IHandleEntity*)Entity, &tr);

		if (tr.m_pEnt == Entity && tr.physicsbone == Entity->GetForceBone() && tr.hitgroup == IdealHitGroup && (IdealHitBox == HITBOX_MAX || tr.hitbox == IdealHitBox))
		{
			//float distancefromexittostart = (exitpos - localeyepos).Length();
			//float distancefromimpacttostart = (impactorigin - localeyepos).Length();
			//if the exit is further away from the impact, of course
			//if (distancefromexittostart > distancefromimpacttostart)
			{
				//Interfaces::DebugOverlay->AddBoxOverlay(tr.endpos, Vector(-0.5, -0.5, -0.5), Vector(0.5, 0.5, 0.5), angZero, 255, 255, 0, 255, 5.0f);

				//ghetto fix until i trace out further to fix hands/legs, etc
				Vector delta = impactorigin - tr.endpos;
				float dist = delta.Length();
				///if (delta.Length() <= 2.0f)
				{
					//Entity->DrawHitboxesFromCache(ColorRGBA(255, 0, 0, 255), 1.6f, tmpmatrix);
					int DestHitbox = tr.hitbox;
					int DestHitgroup = tr.hitgroup;
					//Vector DestEndPos = tr.endpos;

					//Make sure if we trace slightly out of the impact origin, we don't still hit the same thing, otherwise the angle isn't correct
					Interfaces::EngineTrace->ClipRayToEntity(impacttoexcluderay, MASK_SHOT, (IHandleEntity*)Entity, &tr);

					if ((!tr.m_pEnt || tr.hitbox != DestHitbox) && dist < flBestDist)
					{
						FoundRealHeadPosition = true;
						memcpy(bestmatrix, testmatrix, sizeof(matrix3x4_t) * MAXSTUDIOBONES);
						//best_head_angle_diff = angle_diff;
						flBestDist = dist;
						BestAbsYaw = absangles.y;
						HitgroupHit = DestHitgroup;
						//break;
					}

					if (!FoundRealHeadPosition && dist < flBestDist_General && dist < 6.0f)
					{
						firstyawfound = absangles.y;
						memcpy(bestmatrix, testmatrix, sizeof(matrix3x4_t) * MAXSTUDIOBONES);
						BestAbsYaw = absangles.y;
						HitgroupHit = DestHitgroup;
						FoundGeneralAreaOfHead = true;
						//general_head_angle_diff = angle_diff;
						flBestDist_General = dist;
					}
				}
			}
		}
	}

	if (FoundGeneralAreaOfHead || FoundRealHeadPosition)
	{
		if (!FoundRealHeadPosition)
		{
			absangles.y = firstyawfound;
			//best_head_angle_diff = general_head_angle_diff;
		}
		else
		{
			absangles.y = BestAbsYaw;
		}

		const float drawsecs = 2.5f;
		int redamount = FoundRealHeadPosition ? 0 : 255; //Draw as yellow if we didn't find the exact head position

		float diff = AngleDiff(absangles.y, original_eye_yaw);
		bool left = diff < 0.0f;
		float absdiff = fabsf(diff);
		//is the player desyncing at the max possible for the given movement speed
		float deltatomaxdesyncdelta = fabsf(absdiff - tickrecord->m_flAbsMaxDesyncDelta);
		bool isnearmaxdesyncdelta = deltatomaxdesyncdelta <= 10.0f;

		//Only force new resolve info if the delta we calculated at animate time was significantly different than the result we found here
		if (fabsf(AngleDiff(absangles.y, tickrecord->m_AbsAngles.y)) > 8.0f)
		{
			//find eyeangles that match the desync amount

			ResolveSides side = ResolveSides::INVALID_RESOLVE_SIDE;
			s_Impact_Info* SavedInfo = nullptr;

#if defined DEBUG_BH || defined _DEBUG || defined INTERNAL_DEBUG
			const char *name = __FUNCTION__;
#else
			const char *name = nullptr;
#endif
			if (diff > 0.0f)
			{
				SavedInfo = pCPlayer->GetBodyHitResolveInfo(tickrecord, true);
				//Force the resolver to use the body hit side!
				if (pCPlayer->m_iResolveMode != RESOLVE_MODE_MANUAL)
#ifdef NO_35
					pCPlayer->SetResolveSide(ResolveSides::POSITIVE_60, name);
#else
					pCPlayer->SetResolveSide(isnearmaxdesyncdelta ? ResolveSides::POSITIVE_60 : ResolveSides::POSITIVE_35, name);
#endif
			}
			else
			{
				SavedInfo = pCPlayer->GetBodyHitResolveInfo(tickrecord, true);
				//Force the resolver to use the body hit side!
				if (pCPlayer->m_iResolveMode != RESOLVE_MODE_MANUAL)
#ifdef NO_35
					pCPlayer->SetResolveSide(ResolveSides::NEGATIVE_60, name);
#else
					pCPlayer->SetResolveSide(isnearmaxdesyncdelta ? ResolveSides::NEGATIVE_60 : ResolveSides::NEGATIVE_35, name);
#endif
			}


#ifdef NO_35
			SavedInfo->m_bIsBodyHitResolved = false;
			SavedInfo->m_bIsNearMaxDesyncDelta = true;
#else
			SavedInfo->m_bIsBodyHitResolved = true; //set to true to actually make the cheat use the delta when resolving
			SavedInfo->m_bIsNearMaxDesyncDelta = isnearmaxdesyncdelta;
#endif
			SavedInfo->m_bRealYawIsLeft = left;
			SavedInfo->m_flDesyncDelta = deltatomaxdesyncdelta > 5.0f ? diff : 60.0f; //If the player is desyncing at the max desync delta then we don't want to always animate a lower value
			SavedInfo->m_vecResolvedOrigin = networkorigin;
			SavedInfo->m_NetworkedEyeYaw = original_eye_yaw;
			SavedInfo->m_NetworkedLBY = original_lby;
			pCPlayer->Impact.ResolveStances[pCPlayer->GetBodyHitStance(tickrecord)].m_iShotsMissed = 0;
			SavedInfo->m_flLastResolveTime = Interfaces::Globals->realtime;
			pCPlayer->Impact.m_flLastBodyHitResolveTime = Interfaces::Globals->realtime;
			pCPlayer->m_iShotsMissed = 0;
			SavedInfo->m_bResetSetGoalFeetYaw = true;
		}

#if defined DEBUG_BH || defined _DEBUG || defined INTERNAL_DEBUG
		if (!FoundRealHeadPosition)
		{
			std::string ev = "[BH] Approximately resolved " + victim_name + " to " + std::to_string(diff) + " desync amt and " + std::to_string(absangles.y) + " goalfeetyaw";
			if (tickrecord->m_Velocity.Length() > 15.0f)
				ev += " and target was moving";
			ev += " and max desync was " + std::to_string(maxdesyncdelta);
			g_Eventlog.AddEventElement(0x9, ev);
			g_Eventlog.OutputEvent(variable::get().visuals.i_shot_logs);
		}
		else
		{
			std::string ev = "[BH] Successfully resolved " + victim_name + " to " + std::to_string(diff) + " desync amt and " + std::to_string(absangles.y) + " goalfeetyaw";
			if (tickrecord->m_Velocity.Length() > 15.0f)
				ev += " and target was moving";
			ev += " and max desync was " + std::to_string(maxdesyncdelta);
			g_Eventlog.AddEventElement(0x6, ev);
			g_Eventlog.OutputEvent(variable::get().visuals.i_shot_logs);
		}
#endif

#if defined DEBUG_BH || defined _DEBUG || defined INTERNAL_DEBUG
		//if (g_Convars.Visuals.visuals_draw_hit->GetBool())
		//Interfaces::DebugOverlay->AddTextOverlay(pCPlayer->m_pEntity->GetNetworkOrigin() + Vector(0, 0, 64), 4.0f, "delta: %f absyaw: %f body_yaw: %f eyeyaw: %f targettime: %f desthitbox: %i hitgrouphit %i", diff, absangles.y, Entity->GetOldPoseParameterUnscaled(11), tickrecord->m_EyeYaw, TICKS_TO_TIME(tickrecord->m_iTickcount) - g_LagCompensation.GetLerpTime(), IdealHitBox, HitgroupHit);
		Entity->DrawHitboxesFromCache(ColorRGBA(redamount, 255, 0.0f, 115.0f), drawsecs, bestmatrix);
		//Interfaces::DebugOverlay->AddBoxOverlay(Entity->GetBonePositionCachedOnly(HITBOX_LEFT_FOREARM, bestmatrix), Vector(-2, -2, -2), Vector(2, 2, 2), angZero, 255, 0, 0, 255, 4.0f);
#endif
	}
	else
	{
		//for debugging
		//if (numtimes++ == 0)
		//{
		//	goto restart;
		//}
#if defined DEBUG_BH ||defined _DEBUG || defined INTERNAL_DEBUG
		g_Eventlog.AddEventElement(0x7, "[BH] failed to resolve " + victim_name);
		g_Eventlog.OutputEvent(variable::get().visuals.i_shot_logs);
#endif
	}

	backupstate->RestoreData();
	delete backupstate;
	END_PROFILING
}

void ResolveFromPlayerHurt_ManualShot(CPlayerrecord* pCPlayer, CShotrecord* shotrecord)
{
#if 0
	auto Entity = pCPlayer->m_pInflictedEntity;
	matrix3x4_t entitytoworldtransform;
	matrix3x4_t entitytoworldtransform_fake;
	matrix3x4_t* original_matrix;
	matrix3x4_t* original_fake_matrix;
	//we aren't receiving the actual hitgroup yet because this is being called before player_hurt
	int IdealHitGroup, IdealHitBox, numbones;
	Vector localeyepos, impactorigin, networkorigin;
	float body_yaw, body_yaw_fake, original_yaw, original_yaw_fake, original_eye_yaw, speed;
	CTickrecord* tickrecord = nullptr;
	PlayerBackup_t* backupstate = nullptr;

	//FIXME: MAKE THIS WORK AGAIN
	//this must be a manual shot
	auto& ShotInfo = LocalPlayer.ManualFireShotInfo;
	if (!ShotInfo.m_bCachedFakeBones)
		return;
	int index = Entity->index;
	impactorigin = ShotInfo.m_vecMainImpactPos;
	original_matrix = &ShotInfo.BoneMatrixes[0][index];
	original_fake_matrix = &ShotInfo.BoneMatrixesFake[0][index];
	numbones = ShotInfo.m_iNumBones[index];
	localeyepos = ShotInfo.m_vecLocalEyePosition;
	IdealHitGroup = ShotInfo.m_iActualHitgroup;
	IdealHitBox = ShotInfo.m_iActualHitbox;
	entitytoworldtransform = ShotInfo.m_EntityToWorldTransform[index];
	entitytoworldtransform_fake = ShotInfo.m_EntityToWorldTransformFake[index];
	networkorigin = ShotInfo.m_NetOrigin[index];
	body_yaw = ShotInfo.m_flBodyYaw[index];
	body_yaw_fake = ShotInfo.m_flBodyYawFake[index];;
	original_yaw = ShotInfo.m_flAbsYaw[index];
	original_yaw_fake = ShotInfo.m_flAbsYawFake[index];
	original_eye_yaw = ShotInfo.m_flNetEyeYaw[index];
	speed = ShotInfo.m_flSpeed[index];

	auto Entity = pCPlayer->m_pInflictedEntity;
	matrix3x4_t entitytoworldtransform;
	matrix3x4_t entitytoworldtransform_fake;
	matrix3x4_t* original_matrix;
	matrix3x4_t* original_fake_matrix;
	//we aren't receiving the actual hitgroup yet because this is being called before player_hurt
	int IdealHitGroup, IdealHitBox, numbones;
	Vector localeyepos, impactorigin, networkorigin;
	float body_yaw, body_yaw_fake, original_yaw, original_yaw_fake, original_eye_yaw, speed;
	CTickrecord* tickrecord = nullptr;
	PlayerBackup_t* backupstate = nullptr;

	if (!shotrecord)
	{
		//FIXME: MAKE THIS WORK AGAIN
		//this must be a manual shot
		auto& ShotInfo = LocalPlayer.ManualFireShotInfo;
		if (!ShotInfo.m_bCachedFakeBones)
			return;
		int index = Entity->index;
		impactorigin = ShotInfo.m_vecMainImpactPos;
		original_matrix = &ShotInfo.BoneMatrixes[0][index];
		original_fake_matrix = &ShotInfo.BoneMatrixesFake[0][index];
		numbones = ShotInfo.m_iNumBones[index];
		localeyepos = ShotInfo.m_vecLocalEyePosition;
		IdealHitGroup = ShotInfo.m_iActualHitgroup;
		IdealHitBox = ShotInfo.m_iActualHitbox;
		entitytoworldtransform = ShotInfo.m_EntityToWorldTransform[index];
		entitytoworldtransform_fake = ShotInfo.m_EntityToWorldTransformFake[index];
		networkorigin = ShotInfo.m_NetOrigin[index];
		body_yaw = ShotInfo.m_flBodyYaw[index];
		body_yaw_fake = ShotInfo.m_flBodyYawFake[index];;
		original_yaw = ShotInfo.m_flAbsYaw[index];
		original_yaw_fake = ShotInfo.m_flAbsYawFake[index];
		original_eye_yaw = ShotInfo.m_flNetEyeYaw[index];
		speed = ShotInfo.m_flSpeed[index];
	}
	else
	{
		tickrecord = shotrecord->m_Tickrecord;
		//Now make sure the tick record associated with the shot record still exists
		if (!pCPlayer->FindTickRecord(tickrecord) || !tickrecord->m_bCachedFakeBones)
			return;

		impactorigin = pCPlayer->m_vecMainImpactPos;
		original_matrix = tickrecord->m_CachedBoneMatrix;
		original_fake_matrix = tickrecord->m_CachedBoneMatrix_Fake;
		numbones = tickrecord->m_iCachedBonesCount;
		localeyepos = shotrecord->m_vecLocalEyePos;
		IdealHitGroup = shotrecord->m_iActualHitgroup;
		IdealHitBox = shotrecord->m_bTEImpactEffectAcked ? shotrecord->m_iActualHitbox : MTargetting.HitgroupToHitbox(IdealHitGroup);
		entitytoworldtransform = tickrecord->m_EntityToWorldTransform;
		entitytoworldtransform_fake = tickrecord->m_EntityToWorldTransform_Fake;
		networkorigin = tickrecord->m_NetOrigin;
		body_yaw = tickrecord->m_flPoseParams[11];
		body_yaw_fake = tickrecord->m_flBodyYaw_Fake;
		original_yaw = tickrecord->m_AbsAngles.y;
		original_yaw_fake = tickrecord->m_angAbsAngles_Fake.y;
		original_eye_yaw = tickrecord->m_EyeYaw;
		speed = tickrecord->m_Velocity.Length();

		backupstate = new PlayerBackup_t(Entity);
	}

	if (tickrecord && tickrecord->m_bFiredBullet)
		return;

	QAngle absangles = { 0.0f, 0.0f, 0.0f };
	float firstyawfound;
	float original_eye_yaw_positive = AngleNormalizePositive(original_eye_yaw);

	matrix3x4_t OriginalMatrixInverted, EndMatrix;
	mstudiohitboxset_t* set = Entity->GetModelPtr()->_m_pStudioHdr->pHitboxSet(Entity->GetHitboxSet());
	mstudiobbox_t* thorax = set->pHitbox(HITBOX_THORAX);
	//mstudiobbox_t *head = set->pHitbox(HITBOX_HEAD);
	QAngle original_thorax_angles;
	MatrixAngles(original_fake_matrix[thorax->bone], original_thorax_angles);
	QAngle fake_head_angle = CalcAngle(networkorigin, Entity->GetBonePositionCachedOnly(HITBOX_HEAD, original_fake_matrix));

	//if (IdealHitBox >= set->numhitboxes)
	//	return;

	//Interfaces::DebugOverlay->AddBoxOverlay(impactorigin, Vector(-0.2f, -0.2f, -0.2f), Vector(0.2f, 0.2f, 0.2f), QAngle(0.0f, 0.0f, 0.0f), 255, 0, 0, 255, 5.0f);

	Vector shotorigin = localeyepos;
	//Interfaces::DebugOverlay->AddBoxOverlay(shotorigin, Vector(-0.2f, -0.2f, -0.2f), Vector(0.2f, 0.2f, 0.2f), QAngle(0.0f, 0.0f, 0.0f), 0, 0, 255, 255, 5.0f);

	Vector vecDirOutwardFromImpactTowardsShotOrigin = shotorigin - impactorigin;
	VectorNormalizeFast(vecDirOutwardFromImpactTowardsShotOrigin);


	//Interfaces::DebugOverlay->AddLineOverlay(shotorigin, shotorigin + -vecDirOutwardFromImpactTowardsShotOrigin * 256.0f, 0, 255, 0, false, 5.0f);

	Vector tracestart = shotorigin; //impactorigin + (vecDirOutwardFromImpactTowardsShotOrigin * 1.25f);
	Vector traceend = impactorigin + -(vecDirOutwardFromImpactTowardsShotOrigin * 0.5f);
	Vector traceexclude = impactorigin + (vecDirOutwardFromImpactTowardsShotOrigin * 1.0f);

	bool FoundRealHeadPosition = false; //Real head should be the exact spot the head is
	bool FoundGeneralAreaOfHead = false; //General head is an area where the head will most likely generally be. It's not an exact spot

	matrix3x4_t testmatrix[MAXSTUDIOBONES];
	matrix3x4_t firstmatrixfound[MAXSTUDIOBONES];
	matrix3x4_t* pbestmatrixfound = nullptr;
	float best_head_angle_diff;
	float general_head_angle_diff;

	std::vector<CSphere>m_cSpheres;
	std::vector<COBB>m_cOBBs;

	Ray_t starttoendray, impacttoexcluderay;
	starttoendray.Init(tracestart, traceend);
	impacttoexcluderay.Init(impactorigin, traceexclude);

	CTickrecord* oldtickrecord = nullptr;
	for (auto& tick : pCPlayer->m_Tickrecords)
	{
		if (tick->m_iServerTick < tickrecord->m_iServerTick)
		{
			oldtickrecord = tick;
			break;
		}
	}

	CCSGOPlayerAnimState* m_pClientAnimState = tickrecord ? tickrecord->m_animstate_server : nullptr;
	C_CSGOPlayerAnimState* clanimstate = &tickrecord->m_animstate;
	float flDuckSpeedClamp, flRunningSpeedClamp, flLerped, flSpeed, flMaxYaw, flMinYaw, flLadderCycle;
	bool bOnLadder;
	Vector& vVelocity = m_pClientAnimState ? m_pClientAnimState->m_vVelocity : clanimstate->m_vVelocity;
	float velAngle = (atan2(-vVelocity.y, -vVelocity.x) * 180.0f) * (1.0f / M_PI);
	if (velAngle < 0.0f)
		velAngle += 360.0f;

	if (m_pClientAnimState)
	{
		flDuckSpeedClamp = clamp(m_pClientAnimState->m_flDuckingSpeed, 0.0f, 1.0f);
		flRunningSpeedClamp = clamp(m_pClientAnimState->m_flRunningSpeed, 0.0f, 1.0f);
		flLerped = ((flDuckSpeedClamp - flRunningSpeedClamp) * m_pClientAnimState->m_fDuckAmount) + flRunningSpeedClamp;
		flSpeed = m_pClientAnimState->m_flSpeed;
		flMaxYaw = m_pClientAnimState->m_flMaxYaw;
		flMinYaw = m_pClientAnimState->m_flMinYaw;
		flLadderCycle = m_pClientAnimState->m_flLadderCycle;
		bOnLadder = m_pClientAnimState->m_bOnLadder;
	}
	else
	{
		flDuckSpeedClamp = clamp(clanimstate->m_flDuckingSpeed, 0.0f, 1.0f);
		flRunningSpeedClamp = clamp(clanimstate->m_flRunningSpeed, 0.0f, 1.0f);
		flLerped = ((flDuckSpeedClamp - flRunningSpeedClamp) * clanimstate->m_fDuckAmount) + flRunningSpeedClamp;
		flSpeed = clanimstate->m_flSpeed;
		flMaxYaw = clanimstate->m_flMaxYaw;
		flMinYaw = clanimstate->m_flMinYaw;
		flLadderCycle = clanimstate->m_flLadderCycle;
		bOnLadder = clanimstate->m_bOnLadder;
	}
	float flBiasMove = Bias(flLerped, 0.18f);

	float m_flCurrentMoveDirGoalFeetDelta, m_flGoalMoveDirGoalFeetDelta, _m_flCurrentMoveDirGoalFeetDelta, _m_flGoalMoveDirGoalFeetDelta;
	if (oldtickrecord)
	{
		_m_flCurrentMoveDirGoalFeetDelta = oldtickrecord->m_animstate_server ? oldtickrecord->m_animstate_server->m_flCurrentMoveDirGoalFeetDelta : oldtickrecord->m_animstate.m_flCurrentMoveDirGoalFeetDelta;
		_m_flGoalMoveDirGoalFeetDelta = oldtickrecord->m_animstate_server ? oldtickrecord->m_animstate_server->m_flGoalMoveDirGoalFeetDelta : oldtickrecord->m_animstate.m_flGoalMoveDirGoalFeetDelta;
	}
	else
	{
		_m_flCurrentMoveDirGoalFeetDelta = tickrecord->m_animstate_server ? tickrecord->m_animstate_server->m_flCurrentMoveDirGoalFeetDelta : tickrecord->m_animstate.m_flCurrentMoveDirGoalFeetDelta;
		_m_flGoalMoveDirGoalFeetDelta = tickrecord->m_animstate_server ? tickrecord->m_animstate_server->m_flGoalMoveDirGoalFeetDelta : tickrecord->m_animstate.m_flGoalMoveDirGoalFeetDelta;
	}

	Entity->WriteAnimLayers(tickrecord->m_AnimLayer);
	Entity->WritePoseParameters(tickrecord->m_flPoseParams);
	Vector pseudoup = { 0.0f, 0.0f, 1.0f };

	for (int i = 0; i < 360; i += 2)
	{
		//cast int to float to prevent accuracy loss from the for loop
		absangles.y = (float)i;

		if (!tickrecord)
		{
			//Transform the old matrix to the current player position for aesthetic reasons
			MatrixInvert(entitytoworldtransform_fake, OriginalMatrixInverted);
			MatrixCopy(entitytoworldtransform_fake, EndMatrix);

			AngleMatrix(absangles, EndMatrix);

			//Get a positional matrix from the current position
			PositionMatrix(networkorigin, EndMatrix);

			//Get a relative transform
			matrix3x4_t TransformedMatrix;
			ConcatTransforms(EndMatrix, OriginalMatrixInverted, TransformedMatrix);

			for (int j = 0; j < numbones; j++)
			{
				//Now concat the original matrix with the rotated one
				ConcatTransforms(TransformedMatrix, original_fake_matrix[j], testmatrix[j]);
				//old matrix		dest new matrix
			}
		}
		else
		{
			m_flCurrentMoveDirGoalFeetDelta = _m_flCurrentMoveDirGoalFeetDelta;
			m_flGoalMoveDirGoalFeetDelta = _m_flCurrentMoveDirGoalFeetDelta;

			if (flSpeed > 0.0f)
				m_flGoalMoveDirGoalFeetDelta = AngleNormalize(AngleDiff(velAngle, absangles.y));

			float m_flFeetVelDirDelta = AngleNormalize(AngleDiff(m_flGoalMoveDirGoalFeetDelta, m_flCurrentMoveDirGoalFeetDelta));
			m_flCurrentMoveDirGoalFeetDelta = Entity->GetAnimOverlayDirect(7)->m_flWeight >= 1.0f ? m_flGoalMoveDirGoalFeetDelta : AngleNormalize(((flBiasMove + 0.1f) * m_flFeetVelDirDelta) + m_flCurrentMoveDirGoalFeetDelta);
			Entity->SetPoseParameter(7, m_flCurrentMoveDirGoalFeetDelta); //move_yaw

			float new_body_yaw_pose = 0.0f; //not initialized?
			float eye_goalfeet_delta = AngleDiff(original_eye_yaw_positive, absangles.y);


			//////////////////////////////////////////////////////////////
			///////////////fixme fix me ///////////////fixme fix me ///////////////fixme fix me ///////////////fixme fix me ///////////////fixme fix me ///////////////fixme fix me ///////////////fixme fix me ///////////////fixme fix me ///////////////fixme fix me ///////////////fixme fix me ///////////////fixme fix me ///////////////fixme fix me ///////////////fixme fix me ///////////////fixme fix me ///////////////fixme fix me 

			if (eye_goalfeet_delta < 0.0f || flMaxYaw == 0.0f)
				if (flMinYaw != 0.0f)
					new_body_yaw_pose = (eye_goalfeet_delta / flMinYaw) * -58.0f;
				else
					new_body_yaw_pose = (eye_goalfeet_delta / flMaxYaw) * 58.0f;

			Entity->SetAbsAngles(absangles); //fix the abs angles to the goalfeetyaw

			auto m_pClientAnimState = Entity->GetPlayerAnimState();
			m_pClientAnimState->m_arrPoseParameters[6].SetValue(m_pClientAnimState->pBaseEntity, new_body_yaw_pose); //fix the body yaw pose parameter

			if (Entity->IsStrafing())
				Entity->SetPoseParameter(14, AngleNormalize(m_flCurrentMoveDirGoalFeetDelta));

			if (flLadderCycle > 0.0f || bOnLadder)
			{
				Vector laddernormal = Entity->GetVecLadderNormal();
				QAngle ladderAngles;
				VectorAngles(laddernormal, ladderAngles);
				Entity->SetPoseParameter(4, AngleDiff(ladderAngles.y, absangles.y));
			}

			QAngle angles;
			VectorAngles(tickrecord->m_animstate.m_vecSetupLeanVelocityInterpolated, pseudoup, angles); //TODO: check to see if lean velocity is calculated correct on client
			Entity->SetPoseParameter(2, AngleNormalize(absangles.y - angles.y)); //lean_yaw

			AllowSetupBonesToUpdateAttachments = false;
			Entity->InvalidateBoneCache();
			Entity->SetLastOcclusionCheckFlags(0);
			Entity->SetLastOcclusionCheckFrameCount(Interfaces::Globals->framecount);
			Entity->ReevaluateAnimLOD();
			Entity->SetupBonesRebuilt(testmatrix, MAXSTUDIOBONES, BONE_USED_BY_HITBOX, Interfaces::Globals->curtime);
		}

		//QAngle new_angles;
		//MatrixAngles(testmatrix[thorax->bone], new_angles);
		//if (fabsf(AngleDiff(new_angles.y, original_thorax_angles.y)) > 120.0f)
		//	continue;

		QAngle new_head_angle = CalcAngle(networkorigin, Entity->GetBonePositionCachedOnly(HITBOX_HEAD, testmatrix));
		float angle_diff = fabsf(AngleDiff(new_head_angle.y, fake_head_angle.y));
		//if (angle_diff > 122.0f)
		//	continue;

		m_cSpheres.clear();
		m_cOBBs.clear();

		for (int k = 0; k < HITBOX_MAX; k++)
		{
			mstudiobbox_t* pbox = set->pHitbox(k);
			if (pbox->radius != -1.0f)
			{
				Vector vMin, vMax;
				VectorTransformZ(pbox->bbmin, testmatrix[pbox->bone], vMin);
				VectorTransformZ(pbox->bbmax, testmatrix[pbox->bone], vMax);
				SetupCapsule(vMin, vMax, pbox->radius, k, pbox->group, m_cSpheres);
			}
			else
			{
				m_cOBBs.push_back(COBB(pbox->bbmin, pbox->bbmax, pbox->angles, &testmatrix[pbox->bone], k, pbox->group));
			}
		}

		trace_t tr;
		Vector exitpos;
		TRACE_HITBOX(Entity, starttoendray, tr, m_cSpheres, m_cOBBs, &exitpos);

		if (tr.m_pEnt && tr.physicsbone == Entity->GetForceBone() && (tr.hitgroup == IdealHitGroup || (tr.hitgroup == HITGROUP_HEAD && IdealHitGroup == HITGROUP_NECK)))
		{
			float distancefromexittostart = (exitpos - tracestart).Length();
			float distancefromimpacttostart = (impactorigin - tracestart).Length();
			//if the exit is further away from the impact, of course
			if (distancefromexittostart > distancefromimpacttostart)
			{
				//Interfaces::DebugOverlay->AddBoxOverlay(tr.endpos, Vector(-0.5, -0.5, -0.5), Vector(0.5, 0.5, 0.5), angZero, 255, 255, 0, 255, 5.0f);

				//ghetto fix until i trace out further to fix hands/legs, etc
				Vector delta = impactorigin - tr.endpos;
				///if (delta.Length() <= 2.0f)
				{
					//Entity->DrawHitboxesFromCache(ColorRGBA(255, 0, 0, 255), 1.6f, tmpmatrix);

					//Make sure if we trace slightly out of the impact origin, we don't still hit the same thing, otherwise the angle isn't correct
					TRACE_HITBOX(Entity, impacttoexcluderay, tr, m_cSpheres, m_cOBBs);

					if (!tr.m_pEnt)
					{
						FoundRealHeadPosition = true;
						pbestmatrixfound = testmatrix;
						best_head_angle_diff = angle_diff;
						break;
					}

					if (!FoundGeneralAreaOfHead)
					{
						firstyawfound = absangles.y;
						memcpy(firstmatrixfound, testmatrix, sizeof(matrix3x4_t) * MAXSTUDIOBONES);
						pbestmatrixfound = firstmatrixfound;
						FoundGeneralAreaOfHead = true;
						general_head_angle_diff = angle_diff;
					}
				}
			}
		}
	}

	if (pbestmatrixfound)
	{
		if (!FoundRealHeadPosition)
		{
			absangles.y = firstyawfound;
			best_head_angle_diff = general_head_angle_diff;
		}

		const float drawsecs = 2.5f;
		Vector HeadPos = Entity->GetBonePositionCachedOnly(HITBOX_HEAD, pbestmatrixfound);
		int redamount = FoundRealHeadPosition ? 0 : 255; //Draw as yellow if we didn't find the exact head position

#ifdef _DEBUG
		Interfaces::DebugOverlay->AddTextOverlay(Vector(HeadPos.x, HeadPos.y, HeadPos.z + 1.0f), drawsecs, "Resolved");
		Interfaces::DebugOverlay->AddBoxOverlay(HeadPos, Vector(-2, -2, -2), Vector(2, 2, 2), QAngle(0, 0, 0), redamount, 255, 0, 255, drawsecs);
#endif

		float diff;
		float positive_new_thorax_yaw;
		float positive_old_thorax_yaw;

		if (!tickrecord)
		{
			QAngle new_thorax_angles;
			MatrixAngles(pbestmatrixfound[thorax->bone], new_thorax_angles);

			diff = AngleNormalize(AngleDiff(new_thorax_angles.y, original_thorax_angles.y));

			positive_new_thorax_yaw = AngleNormalizePositive(new_thorax_angles.y);
			positive_old_thorax_yaw = AngleNormalizePositive(original_thorax_angles.y);
		}
		else
		{
			positive_old_thorax_yaw = original_yaw_fake;
			positive_new_thorax_yaw = absangles.y;
			//diff = AngleNormalize(AngleDiff(positive_new_thorax_yaw, positive_old_thorax_yaw));
			diff = AngleNormalize(AngleDiff(absangles.y, Entity->GetEyeAngles().y));
		}

		bool left = diff < 0.0f;//positive_new_thorax_yaw > positive_old_thorax_yaw;
		if (speed <= 0.1f)
		{
			pCPlayer->Impact.m_bUsedBodyHitResolveDelta = true;
			pCPlayer->Impact.m_bRealYawIsLeft = left;
			pCPlayer->Impact.m_flDesyncDelta = diff;
		}
		else
		{
			pCPlayer->Impact.m_bUsedBodyHitResolveDelta = true; //here temporarily
			pCPlayer->Impact.m_bIsBodyHitResolved_Moving = true;
			pCPlayer->Impact.m_bRealYawIsLeft_Moving = left;
			pCPlayer->Impact.m_flDesyncDelta = diff; //here temporarily
			pCPlayer->Impact.m_flDesyncDelta_Moving = diff;
		}
		pCPlayer->Impact.m_flResolvedYaw = AngleNormalizePositive(original_yaw + diff);
		pCPlayer->Impact.m_flResolvedBodyYaw = body_yaw;
		pCPlayer->Impact.m_flLastBodyHitResolveTime = Interfaces::Globals->realtime;
		pCPlayer->Impact.m_vecResolvedOrigin = networkorigin;
		pCPlayer->Impact.m_NetworkedEyeYaw = original_eye_yaw;
		//pCPlayer->m_iShotsMissed = 0;

		if (!shotrecord)
			LocalPlayer.ManualFireShotInfo.m_flLastBodyHitResolveTime = Interfaces::Globals->realtime;

		//if (g_Convars.Visuals.visuals_draw_hit->GetBool())
		Entity->DrawHitboxesFromCache(ColorRGBA(redamount, 255, 0.0f, 75.0f), drawsecs, pbestmatrixfound);
	}

	if (backupstate)
	{
		backupstate->RestoreData();
		delete backupstate;
	}
#endif

}