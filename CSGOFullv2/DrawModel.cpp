#include "precompiled.h"
#include "CreateMove.h"
#include "CSGO_HX.h"
#include "Targetting.h"
#include "AutoWall.h"
#include "ThirdPerson.h"
#include "LocalPlayer.h"
#include "FarESP.h"
#include "ServerSide.h"
#include "CPlayerrecord.h"
#include "ServerSide.h"
#include "worldsize.h"

#include "Adriel/stdafx.hpp"

struct RenderableInfo_t
{
	IClientRenderable* m_pRenderable;
	void* m_pAlphaProperty;
	int m_EnumCount;
	int m_nRenderFrame;
	unsigned short m_FirstShadow;
	unsigned short m_LeafList;
	short m_Area;
	uint16_t m_Flags;				// 0x0016
	uint16_t m_Flags2;				// 0x0018
	Vector m_vecBloatedAbsMins;
	Vector m_vecBloatedAbsMaxs;
	Vector m_vecAbsMins;
	Vector m_vecAbsMaxs;
	int pad;
};

const Vector coord_mins = Vector(MIN_COORD_FLOAT, MIN_COORD_FLOAT, MIN_COORD_FLOAT);
const Vector coord_maxs = Vector(MAX_COORD_FLOAT, MAX_COORD_FLOAT, MAX_COORD_FLOAT);

DrawModelFn oDrawModel;
ListLeavesInBox oListLeavesInBox;

void ForceMaterial(bool ignoreZ, Color clr, IMaterial* material)
{
	if (!material)
		return;

	if (ignoreZ)
	{
		material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, true);
		material->SetMaterialVarFlag(MATERIAL_VAR_ZNEARER, true);
		material->SetMaterialVarFlag(MATERIAL_VAR_NOFOG, true);
	}
	else
	{
		material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);
		material->SetMaterialVarFlag(MATERIAL_VAR_ZNEARER, false);
		material->SetMaterialVarFlag(MATERIAL_VAR_NOFOG, false);
	}

	material->AlphaModulate((float)clr.a() / 255.f);
	material->ColorModulate((float)clr.r() / 255.f, (float)clr.g() / 255.f, (float)clr.b() / 255.f);

	Interfaces::StudioRender->ForcedMaterialOverride(material);
}
void ForceMaterial(bool ignoreZ, int r, int g, int b, int a, IMaterial* material)
{
	if (!material)
		return;

	if (ignoreZ)
	{
		material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, true);
		material->SetMaterialVarFlag(MATERIAL_VAR_ZNEARER, true);
		material->SetMaterialVarFlag(MATERIAL_VAR_NOFOG, true);
	}
	else
	{
		material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);
		material->SetMaterialVarFlag(MATERIAL_VAR_ZNEARER, false);
		material->SetMaterialVarFlag(MATERIAL_VAR_NOFOG, false);
	}

	material->AlphaModulate((float)a / 255.f);
	material->ColorModulate((float)r / 255.f, (float)g / 255.f, (float)b / 255.f);

	Interfaces::StudioRender->ForcedMaterialOverride(material);
}

void ForceMaterialAlpha(bool ignoreZ, float a, IMaterial* material)
{
	if (!material)
		return;

	if (ignoreZ)
	{
		material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, true);
		material->SetMaterialVarFlag(MATERIAL_VAR_ZNEARER, true);
		material->SetMaterialVarFlag(MATERIAL_VAR_NOFOG, true);
	}
	else
	{
		material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);
		material->SetMaterialVarFlag(MATERIAL_VAR_ZNEARER, false);
		material->SetMaterialVarFlag(MATERIAL_VAR_NOFOG, false);
	}

	material->AlphaModulate((float)a / 255.f);

	Interfaces::StudioRender->ForcedMaterialOverride(material);
}

void ForceMaterialAlpha(int alpha, IMaterial* material)
{
	if (!material)
		return;

	material->AlphaModulate(clamp((float)alpha / 255.f, 0.1f, 1.f));
	Interfaces::StudioRender->ForcedMaterialOverride(material);
}

void ForceMaterial(IMaterial *mat, ImColor color, bool z_flag)
{
	if (!mat)
		return;

	mat->SetMaterialVarFlag(MATERIAL_VAR_ZNEARER, z_flag);
	mat->SetMaterialVarFlag(MATERIAL_VAR_NOFOG, z_flag);
	mat->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, z_flag);

	mat->ColorModulate(color.Value.x, color.Value.y, color.Value.z);
	mat->AlphaModulate(color.Value.w);

	Interfaces::StudioRender->ForcedMaterialOverride(mat);
}

int serverticksallowed;
int clientticksallowed;

void __fastcall Hooks::DrawModel(void* ecx, void* edx, void* pResults, const DrawModelInfo_t& info, matrix3x4a_t *pBoneToWorld, float *pFlexWeights, float *pFlexDelayedWeights, Vector &modelOrigin, int flags)
{
	START_PROFILING
	LocalPlayer.Get(&LocalPlayer);

	if (!Interfaces::EngineClient->IsInGame() || !LocalPlayer.Entity || !info.m_pClientEntity || !variable::get().visuals.b_enabled)
	{
		oDrawModel(ecx, pResults, info, pBoneToWorld, pFlexDelayedWeights, pFlexDelayedWeights, modelOrigin, flags);
		END_PROFILING
		return;
	}

	if (!LocalPlayer.IsFakeLaggingOnPeek
		&& info.m_StudioHdr->name[0] == 'p' // NOTE: player\\contactshadow\\contactshadow_leftfoot.mdl player\\contactshadow\\contactshadow_rightfoot.mdl
		&& info.m_StudioHdr->name[7] == 'c'
		&& info.m_StudioHdr->name[14] == 's'
		&& info.m_StudioHdr->name[19] == 'w'
		&& info.m_StudioHdr->name[21] == 'c')
	{
		LocalPlayer.NetvarMutex.Lock();
		Vector footpos = LocalPlayer.Entity->GetBonePositionCachedOnly(info.m_StudioHdr->name[35] == 'l' ? HITBOX_LEFT_FOOT : HITBOX_RIGHT_FOOT, (matrix3x4_t*)LocalPlayer.Entity->GetCachedBoneData()->Base());
		float len = (modelOrigin - footpos).Length();
		if (len < 10.0f)
		{
			//This is our foot shadow, fix its position
			Vector delta = modelOrigin - LocalPlayer.LastAnimatedOrigin;

			pBoneToWorld->SetOrigin(LocalPlayer.Entity->GetAbsOriginDirect() + delta);
		}
		LocalPlayer.NetvarMutex.Unlock();

		oDrawModel(ecx, pResults, info, pBoneToWorld, pFlexDelayedWeights, pFlexDelayedWeights, modelOrigin, flags);
		END_PROFILING
		return;
	}

	// release render context
	auto pRenderContext = Interfaces::MatSystem->GetRenderContext();
	ITexture* texture = pRenderContext->GetRenderTarget();
	pRenderContext->Release();

	// don't draw over _rt_fullframefb
	if (texture)
	{
		const char* name = texture->GetName();
		if (name[4] == 'f' && name[10] == 'a')
		{
			oDrawModel(ecx, pResults, info, pBoneToWorld, pFlexWeights, pFlexDelayedWeights, modelOrigin, flags);
			END_PROFILING;
			return;
		}
	}

	bool drawn = false;
	matrix3x4_t* final_matrix_follow_entities = nullptr;

	enum : int
	{
		MODELTYPE_INVALID,
		MODELTYPE_PLAYER,
		MODELTYPE_WEAPON,
		MODELTYPE_ARMS,
		MODELTYPE_VIEWWEAPON,
		MODELTYPE_BOMB,
		MODELTYPE_PROJ,
	};

	auto get_model_info = [](const DrawModelInfo_t& info) -> int
	{
		if (!info.m_StudioHdr || !info.m_pClientEntity)
			return MODELTYPE_INVALID;

		auto entity = (CBaseEntity*)((DWORD)info.m_pClientEntity - 0x4);
		if (entity && entity->IsProjectile())
			return MODELTYPE_PROJ;

		std::string name = info.m_StudioHdr->name;

		//if (name.find(XorStrCT("play")) != std::string::npos)
		if (IntFromChars(name, 14) == 'yalp')
			return MODELTYPE_PLAYER;

		//if (name.find(XorStrCT("arms")) != std::string::npos)
		if(IntFromChars(name, 17) == 'smra')
			return MODELTYPE_ARMS;

		//if (name.find(XorStrCT("weap")) != std::string::npos)
		if(IntFromChars(name, 0) == 'paew')
		{
			if (name[8] == 'v')
				return MODELTYPE_VIEWWEAPON;

			else if (IntFromChars(name, 10) == '_dei')
				return MODELTYPE_BOMB;

			return MODELTYPE_WEAPON;
		}

		return MODELTYPE_INVALID;
	};

	const auto model_type = get_model_info(info);

	if (model_type == MODELTYPE_INVALID)
	{
		oDrawModel(ecx, pResults, info, pBoneToWorld, pFlexWeights, pFlexDelayedWeights, modelOrigin, flags);
		END_PROFILING
		return;
	}

	// get renderable entity
	auto entity = (CBaseEntity*)((DWORD)info.m_pClientEntity - 0x4);

	// check if the player exists (less CPU)
	if (model_type == MODELTYPE_PLAYER)
	{
		if (!entity /*|| !Interfaces::ClientEntList->PlayerExists(entity)*/)
		{
			oDrawModel(ecx, pResults, info, pBoneToWorld, pFlexWeights, pFlexDelayedWeights, modelOrigin, flags);
			END_PROFILING
			return;
		}
	}

	bool enemy = entity->IsEnemy(LocalPlayer.Entity);

	// thirdperson local player aesthetics
	if (Interfaces::Input->CAM_IsThirdPerson())
	{
		if (!entity->IsLocalPlayer() && entity->GetMoveParent()->IsLocalPlayer())
		{
			if (!LocalPlayer.IsFakeLaggingOnPeek)
			{
				LocalPlayer.NetvarMutex.Lock();
				// this must be a following entity such as the player foot shadow or view model
				// fix it by matching the position to the player position
				auto parent = entity->GetMoveParent();

				matrix3x4_t OriginalMatrixInverted, EndMatrix;
				auto cache = entity->GetCachedBoneData();
				int numbones = cache->Count();

				//Transform the old matrix to the current player position for aesthetic reasons
				MatrixInvert(entity->EntityToWorldTransform(), OriginalMatrixInverted);
				MatrixCopy(entity->EntityToWorldTransform(), EndMatrix);

				//Get relative position from parent position
				Vector relative_position = entity->GetAbsOriginDirect() - LocalPlayer.LastAnimatedOrigin;

				//Get a positional matrix from the current position of the parent + the relative position of the following entity
				PositionMatrix(parent->GetAbsOriginDirect() + relative_position, EndMatrix);

				//Get a relative transform
				matrix3x4_t TransformedMatrix;
				ConcatTransforms(EndMatrix, OriginalMatrixInverted, TransformedMatrix);

				final_matrix_follow_entities = new matrix3x4_t[MAXSTUDIOBONES];

				for (int i = 0; i < numbones; i++)
				{
					//Now concat the original matrix with the rotated one
					ConcatTransforms(TransformedMatrix, pBoneToWorld[i], final_matrix_follow_entities[i]);
					//old matrix		dest new matrix
				}

				//Draw the newly transformed matrix
				pBoneToWorld = (matrix3x4a_t*)final_matrix_follow_entities;
				LocalPlayer.NetvarMutex.Unlock();
			}
		}
	}

	/*auto p_renderable_info = (RenderableInfo_t*)entity->GetClientRenderable();
	if (!p_renderable_info->m_pRenderable)
		return oDrawModel(ecx, pResults, info, pBoneToWorld, pFlexWeights, pFlexDelayedWeights, modelOrigin, flags);*/

	// deal with rendering specific model types
	if (model_type == MODELTYPE_PLAYER)
	{
		bool alive = static_cast<bool>(entity->GetAlive());
		int index = entity->entindex();

		matrix3x4_t* final_matrix_Real = nullptr;
		matrix3x4_t* final_matrix_Fake = nullptr;
		
		if (alive)
		{
			// local player check
			if (entity->IsLocalPlayer())
			{
				LocalPlayer.NetvarMutex.Lock();
				// draw real angle model
				matrix3x4_t * original_matrix_Real = LocalPlayer.RealAngleMatrix;
				matrix3x4_t * original_matrix_Fake = LocalPlayer.FakeAngleMatrix;
				matrix3x4_t OriginalMatrixInverted_Real, EndMatrix_Real;
				matrix3x4_t OriginalMatrixInverted_Fake, EndMatrix_Fake;
				final_matrix_Real = new matrix3x4_t[MAXSTUDIOBONES];
				final_matrix_Fake = new matrix3x4_t[MAXSTUDIOBONES];
				auto cache = entity->GetCachedBoneData();
				int numbones = cache->Count();

				//Transform the old matrix to the current player position for aesthetic reasons
				MatrixInvert(LocalPlayer.RealAngleEntityToWorldTransform, OriginalMatrixInverted_Real);
				MatrixInvert(LocalPlayer.FakeAngleEntityToWorldTransform, OriginalMatrixInverted_Fake);
				MatrixCopy(LocalPlayer.RealAngleEntityToWorldTransform, EndMatrix_Real);
				MatrixCopy(LocalPlayer.FakeAngleEntityToWorldTransform, EndMatrix_Fake);

				//Set the angles for the new matrix
				//AngleMatrix(QAngle(0.0f, LocalPlayer.LowerBodyYaw, 0.0f), EndMatrix);

				//Get a positional matrix from the current position
				PositionMatrix(entity->GetAbsOriginDirect(), EndMatrix_Real);
				PositionMatrix(entity->GetAbsOriginDirect(), EndMatrix_Fake);

				//Get a relative transform
				matrix3x4_t TransformedMatrix_Real, TransformedMatrix_Fake;
				ConcatTransforms(EndMatrix_Real, OriginalMatrixInverted_Real, TransformedMatrix_Real);
				ConcatTransforms(EndMatrix_Fake, OriginalMatrixInverted_Fake, TransformedMatrix_Fake);

				for (int i = 0; i < numbones; i++)
				{
					//Now concat the original matrix with the rotated one
					ConcatTransforms(TransformedMatrix_Real, original_matrix_Real[i], final_matrix_Real[i]);
					ConcatTransforms(TransformedMatrix_Fake, original_matrix_Fake[i], final_matrix_Fake[i]);
				}

				//Draw the newly transformed matrix
				if (!LocalPlayer.IsFakeLaggingOnPeek)
					pBoneToWorld = (matrix3x4a_t*)final_matrix_Real;
				else
					pBoneToWorld = (matrix3x4a_t*)original_matrix_Real;

#ifdef USE_SERVER_SIDE
				CSGOPacket* packet = nullptr;
				if (pServerSide.IsSocketCreated())
				{
					auto record = g_LagCompensation.GetPlayerrecord(entity);
					if (record)
					{
						bool found = false;
						//sometimes we receive a newer simulation time from the serverside plugin faster than the actual game server sends
						for (int i = 0; i < 2 && !found; ++i)
						{
							record->serversidemutex.lock();
							if (i == 0)
								packet = record->GetServerSidePacket();
							else
								packet = record->ServerSidePackets.size() > 1 ? &record->ServerSidePackets.at(1) : nullptr;

							if (packet)
							{
								float flTargetTime = packet->flSimulationTime;
								if (LocalPlayer.m_RealMatrixBackups.buffersize)
								{
									for (BackupMatrixStruct* pk = LocalPlayer.m_RealMatrixBackups.rbegin<BackupMatrixStruct>(); pk != LocalPlayer.m_RealMatrixBackups.rend<BackupMatrixStruct>(); --pk)
									{
										if (pk->m_SimulationTime == flTargetTime)
										{
											modelOrigin = packet->absorigin;
											pBoneToWorld = (matrix3x4a_t*)pk->m_Matrix;
											serverticksallowed = packet->ticksallowedforprocessing;
											clientticksallowed = pk->m_TicksAllowedForProcessing;
											//Interfaces::DebugOverlay->AddTextOverlay(modelOrigin, TICKS_TO_TIME(2), "Server: %i Client: %i", packet->ticksallowedforprocessing, (*pk)->m_TicksAllowedForProcessing);
											found = true;
											break;
										}
									}
								}
							}
							record->serversidemutex.unlock();
						}
					}
				}
#endif

				if (LocalPlayer.Config_IsFakelagging() || LocalPlayer.Config_IsDesyncing())
				{
					// draw fake model
					if (variable::get().visuals.b_fake && variable::get().visuals.b_enabled)
					{

						matrix3x4a_t *dest = !LocalPlayer.IsFakeLaggingOnPeek ? (matrix3x4a_t*)final_matrix_Fake : (matrix3x4a_t*)original_matrix_Fake;

#ifdef USE_SERVER_SIDE
						if (pServerSide.IsSocketCreated() && packet)
						{
							auto record = g_LagCompensation.GetPlayerrecord(entity);
							if (record)
							{
								record->serversidemutex.lock();
								float flTargetTime = packet->flSimulationTime;
								if (LocalPlayer.m_FakeMatrixBackups.buffersize)
								{
									for (BackupMatrixStruct* pk = LocalPlayer.m_FakeMatrixBackups.rbegin<BackupMatrixStruct>(); pk != LocalPlayer.m_FakeMatrixBackups.rend<BackupMatrixStruct>(); --pk)
									{
										if (pk->m_SimulationTime == flTargetTime)
										{
											modelOrigin = packet->absorigin;
											dest = (matrix3x4a_t*)pk->m_Matrix;
											break;
										}

									}
								}
								record->serversidemutex.unlock();
							}
						}
#endif

						ImColor color = variable::get().visuals.col_fake.color().ToImGUI();

						// blend scope check
						if (entity->IsScoped())
						{
							color.Value.w = variable::get().visuals.f_blend_scope * 0.01f;
						}

						ForceMaterial(g_Visuals.MutinyMaterials[variable::get().visuals.pf_local_player.vf_main.chams.i_mat_desync_type], color, false);

						oDrawModel(ecx, pResults, info, dest, pFlexDelayedWeights, pFlexDelayedWeights, modelOrigin, flags);
					}
				}

				Interfaces::ModelRender->ForcedMaterialOverride(nullptr);


				if (variable::get().visuals.pf_local_player.vf_main.b_enabled && variable::get().visuals.pf_local_player.vf_main.chams.b_enabled)
				{
					ImColor color = variable::get().visuals.pf_local_player.vf_main.chams.col_visible.color().ToImGUI();

					// blend scope check
					if (entity->IsScoped())
					{
						color.Value.w = variable::get().visuals.f_blend_scope * 0.01f;
					}
					ForceMaterial(g_Visuals.MutinyMaterials[variable::get().visuals.pf_local_player.vf_main.chams.i_mat_visible], color, true);
				}

				// draw real model
				oDrawModel(ecx, pResults, info, pBoneToWorld, pFlexDelayedWeights, pFlexDelayedWeights, modelOrigin, flags);
				drawn = true;

				if (final_matrix_Real)
					delete[]final_matrix_Real;
				if (final_matrix_Fake)
					delete[]final_matrix_Fake;

				LocalPlayer.NetvarMutex.Unlock();
			}
			// other players
			else if (index >= 0 && index <= MAX_PLAYERS)
			{
				//uncomment this when testing multipoint
				//ForceMaterial(false, Color(255, 255, 255, 50), g_Info.LitMaterial);

				// get playerrecord
				auto _playerRecord = &m_PlayerRecords[index];

				LocalPlayer.NetvarMutex.Lock();

				// get current record
				CTickrecord *currentrecord = nullptr;
				for (const auto& tick : _playerRecord->m_Tickrecords)
				{
					if (tick->m_bCachedBones)
					{
						currentrecord = tick;
						break;
					}
				}

				if (currentrecord)
				{
					// set bonematrix
					pBoneToWorld = reinterpret_cast<matrix3x4a_t*>(currentrecord->m_PlayerBackup.CachedBoneMatrices);

					// get render origin
					Vector _RenderOrigin = currentrecord ? currentrecord->m_AbsOrigin : entity->GetAbsOriginDirect();

					bool cham = false, xqz = false, backtrack = false;
					ImColor vis_color = Color::White().ToImGUI();
					ImColor invis_color = Color::White().ToImGUI();
					ImColor backtrack_color = Color::White().ToImGUI();
					int vis_mat = 0, invis_mat = 0;

					if (alive)
					{
						if (enemy)
						{
							cham = (variable::get().visuals.pf_enemy.vf_main.b_enabled && variable::get().visuals.pf_enemy.vf_main.chams.b_enabled && variable::get().visuals.pf_enemy.vf_main.b_render);
							xqz = (cham && variable::get().visuals.pf_enemy.vf_main.chams.b_xqz);
							backtrack = (cham && variable::get().visuals.pf_enemy.b_backtrack);
							invis_color = variable::get().visuals.pf_enemy.vf_main.chams.col_invisible.color().ToImGUI();
							vis_color = variable::get().visuals.pf_enemy.vf_main.chams.col_visible.color().ToImGUI();
							backtrack_color = variable::get().visuals.pf_enemy.col_backtrack.color().ToImGUI();

							vis_mat = variable::get().visuals.pf_enemy.vf_main.chams.i_mat_visible;
							invis_mat = variable::get().visuals.pf_enemy.vf_main.chams.i_mat_invisible;
						}
						else
						{
							cham = (variable::get().visuals.pf_teammate.vf_main.b_enabled && variable::get().visuals.pf_teammate.vf_main.chams.b_enabled && variable::get().visuals.pf_teammate.vf_main.b_render);
							xqz = (cham && variable::get().visuals.pf_teammate.vf_main.chams.b_xqz);
							backtrack = (cham && variable::get().visuals.pf_teammate.b_backtrack);
							invis_color = variable::get().visuals.pf_teammate.vf_main.chams.col_invisible.color().ToImGUI();
							vis_color = variable::get().visuals.pf_teammate.vf_main.chams.col_visible.color().ToImGUI();
							backtrack_color = variable::get().visuals.pf_teammate.col_backtrack.color().ToImGUI();

							vis_mat = variable::get().visuals.pf_teammate.vf_main.chams.i_mat_visible;
							invis_mat = variable::get().visuals.pf_teammate.vf_main.chams.i_mat_invisible;
						}
					}
					else
					{
						cham = (variable::get().visuals.vf_ragdolls.b_enabled && variable::get().visuals.vf_ragdolls.chams.b_enabled && variable::get().visuals.vf_ragdolls.b_render);
						xqz = (cham && variable::get().visuals.vf_ragdolls.chams.b_xqz);
						backtrack = false;
						invis_color = variable::get().visuals.vf_ragdolls.chams.col_invisible.color().ToImGUI();
						vis_color = variable::get().visuals.vf_ragdolls.chams.col_visible.color().ToImGUI();

						vis_mat = variable::get().visuals.vf_ragdolls.chams.i_mat_visible;
						invis_mat = variable::get().visuals.vf_ragdolls.chams.i_mat_invisible;
					}

					// xqz
					if (xqz)
					{
						ForceMaterial(g_Visuals.MutinyMaterials[invis_mat], invis_color, true);
						oDrawModel(ecx, pResults, info, pBoneToWorld, pFlexDelayedWeights, pFlexDelayedWeights, modelOrigin, flags);
					}

					// regular
					if (cham)
					{
						ForceMaterial(g_Visuals.MutinyMaterials[vis_mat], vis_color, false);
						oDrawModel(ecx, pResults, info, pBoneToWorld, pFlexDelayedWeights, pFlexDelayedWeights, modelOrigin, flags);
					}

					// backtrack
					if (backtrack)
					{
						// get oldest record if it's not the same as the one cached in createmove
						auto _record = _playerRecord->GetOldestValidRecord();

						// origin changed
						if (_record && _record->m_AbsOrigin != _RenderOrigin)
						{
							// no bones cached
							if (!_record->m_bCachedBones)
							{
								// cache bones
								PlayerBackup_t *backupstate = new PlayerBackup_t(entity);
								_playerRecord->CM_RestoreAnimations(_record);
								_playerRecord->CM_RestoreNetvars(_record);
								AllowSetupBonesToUpdateAttachments = false;
								_playerRecord->CacheBones(Interfaces::Globals->curtime, false, _record);
								backupstate->RestoreData();
								delete backupstate;
							}

							// set bonematrix
							pBoneToWorld = reinterpret_cast<matrix3x4a_t*>(_record->m_PlayerBackup.CachedBoneMatrices);

							// draw backtrack model
							ForceMaterial(g_Visuals.MutinyMaterials[FLAT], backtrack_color, true);
							oDrawModel(ecx, pResults, info, pBoneToWorld, pFlexDelayedWeights, pFlexDelayedWeights, modelOrigin, flags);
						}
					}
				}
				LocalPlayer.NetvarMutex.Unlock();
			}
		}
	}
	// todo: nit; DrawModel only draws on skinned weapons and not base models. We need to convert these chams to SceneEnd eventually.
	else if (model_type == MODELTYPE_VIEWWEAPON)
	{
		bool cham = variable::get().visuals.vf_viewweapon.b_enabled && variable::get().visuals.vf_viewweapon.chams.b_enabled && variable::get().visuals.vf_viewweapon.b_render;
		bool xqz = cham && variable::get().visuals.vf_viewweapon.chams.b_xqz;
		ImColor vis_color = variable::get().visuals.vf_viewweapon.chams.col_visible.color().ToImGUI();
		ImColor invis_color = variable::get().visuals.vf_viewweapon.chams.col_invisible.color().ToImGUI();
		int vis_mat = variable::get().visuals.vf_viewweapon.chams.i_mat_visible;
		int invis_mat = variable::get().visuals.vf_viewweapon.chams.i_mat_invisible;

		if (xqz)
		{
			ForceMaterial(g_Visuals.MutinyMaterials[invis_mat], invis_color, true);
			oDrawModel(ecx, pResults, info, pBoneToWorld, pFlexDelayedWeights, pFlexDelayedWeights, modelOrigin, flags);
		}

		if (cham)
		{
			ForceMaterial(g_Visuals.MutinyMaterials[vis_mat], vis_color, false);
			oDrawModel(ecx, pResults, info, pBoneToWorld, pFlexDelayedWeights, pFlexDelayedWeights, modelOrigin, flags);
		}
	}
	else if (model_type == MODELTYPE_WEAPON)
	{
		bool cham = variable::get().visuals.vf_weapon.b_enabled && variable::get().visuals.vf_weapon.chams.b_enabled && variable::get().visuals.vf_weapon.b_render;
		bool xqz = cham && variable::get().visuals.vf_weapon.chams.b_xqz;
		ImColor vis_color = variable::get().visuals.vf_weapon.chams.col_visible.color().ToImGUI();
		ImColor invis_color = variable::get().visuals.vf_weapon.chams.col_invisible.color().ToImGUI();
		int vis_mat = variable::get().visuals.vf_weapon.chams.i_mat_visible;
		int invis_mat = variable::get().visuals.vf_weapon.chams.i_mat_invisible;

		if (xqz)
		{
			ForceMaterial(g_Visuals.MutinyMaterials[invis_mat], invis_color, true);
			oDrawModel(ecx, pResults, info, pBoneToWorld, pFlexDelayedWeights, pFlexDelayedWeights, modelOrigin, flags);
		}

		if (cham)
		{
			ForceMaterial(g_Visuals.MutinyMaterials[vis_mat], vis_color, false);
			oDrawModel(ecx, pResults, info, pBoneToWorld, pFlexDelayedWeights, pFlexDelayedWeights, modelOrigin, flags);
		}
	}
	else if (model_type == MODELTYPE_BOMB)
	{
		bool cham = variable::get().visuals.vf_bomb.b_enabled && variable::get().visuals.vf_bomb.chams.b_enabled && variable::get().visuals.vf_bomb.b_render;
		bool xqz = cham && variable::get().visuals.vf_bomb.chams.b_xqz;
		ImColor vis_color = variable::get().visuals.vf_bomb.chams.col_visible.color().ToImGUI();
		ImColor invis_color = variable::get().visuals.vf_bomb.chams.col_invisible.color().ToImGUI();
		int vis_mat = variable::get().visuals.vf_bomb.chams.i_mat_visible;
		int invis_mat = variable::get().visuals.vf_bomb.chams.i_mat_invisible;

		if (xqz)
		{
			ForceMaterial(g_Visuals.MutinyMaterials[invis_mat], invis_color, true);
			oDrawModel(ecx, pResults, info, pBoneToWorld, pFlexDelayedWeights, pFlexDelayedWeights, modelOrigin, flags);
		}

		if (cham)
		{
			ForceMaterial(g_Visuals.MutinyMaterials[vis_mat], vis_color, false);
			oDrawModel(ecx, pResults, info, pBoneToWorld, pFlexDelayedWeights, pFlexDelayedWeights, modelOrigin, flags);
		}
	}
	else if (model_type == MODELTYPE_ARMS)
	{
		bool cham = variable::get().visuals.vf_arms.b_enabled && variable::get().visuals.vf_arms.chams.b_enabled && variable::get().visuals.vf_weapon.b_render;
		bool xqz = cham && variable::get().visuals.vf_arms.chams.b_xqz;
		ImColor vis_color = variable::get().visuals.vf_arms.chams.col_visible.color().ToImGUI();
		ImColor invis_color = variable::get().visuals.vf_arms.chams.col_invisible.color().ToImGUI();
		int vis_mat = variable::get().visuals.vf_arms.chams.i_mat_visible;
		int invis_mat = variable::get().visuals.vf_arms.chams.i_mat_invisible;

		if (xqz)
		{
			ForceMaterial(g_Visuals.MutinyMaterials[invis_mat], invis_color, true);
			oDrawModel(ecx, pResults, info, pBoneToWorld, pFlexDelayedWeights, pFlexDelayedWeights, modelOrigin, flags);
		}

		if (cham)
		{
			ForceMaterial(g_Visuals.MutinyMaterials[vis_mat], vis_color, false);
			oDrawModel(ecx, pResults, info, pBoneToWorld, pFlexDelayedWeights, pFlexDelayedWeights, modelOrigin, flags);
		}
	}
	else if (model_type == MODELTYPE_PROJ)
	{
		bool cham = variable::get().visuals.vf_projectile.b_enabled && variable::get().visuals.vf_projectile.chams.b_enabled && variable::get().visuals.vf_projectile.b_render;
		bool xqz = cham && variable::get().visuals.vf_projectile.chams.b_xqz;
		ImColor vis_color = variable::get().visuals.vf_projectile.chams.col_visible.color().ToImGUI();
		ImColor invis_color = variable::get().visuals.vf_projectile.chams.col_invisible.color().ToImGUI();
		int vis_mat = variable::get().visuals.vf_projectile.chams.i_mat_visible;
		int invis_mat = variable::get().visuals.vf_projectile.chams.i_mat_invisible;

		if (xqz)
		{
			ForceMaterial(g_Visuals.MutinyMaterials[invis_mat], invis_color, true);
			oDrawModel(ecx, pResults, info, pBoneToWorld, pFlexDelayedWeights, pFlexDelayedWeights, modelOrigin, flags);
		}

		if (cham)
		{
			ForceMaterial(g_Visuals.MutinyMaterials[vis_mat], vis_color, false);
			oDrawModel(ecx, pResults, info, pBoneToWorld, pFlexDelayedWeights, pFlexDelayedWeights, modelOrigin, flags);
		}
	}
	
	if (!drawn)
	{
		// apply material
		oDrawModel(ecx, pResults, info, pBoneToWorld, pFlexDelayedWeights, pFlexDelayedWeights, modelOrigin, flags);
	}

	// reset wireframe
	if (g_Info.LitMaterial)
		g_Info.LitMaterial->SetMaterialVarFlag(MATERIAL_VAR_WIREFRAME, false);

	Interfaces::ModelRender->ForcedMaterialOverride(nullptr);

	if (final_matrix_follow_entities)
		delete[]final_matrix_follow_entities;

	END_PROFILING
}

int __fastcall Hooks::ListLeavesInBox(void* ecx, void* edx, Vector& mins, Vector& maxs, unsigned short* list, int list_max)
{
	auto& var = variable::get();

	if (!LocalPlayer.Entity || !var.visuals.b_enabled || *(uint32_t*)_ReturnAddress() != StaticOffsets.GetOffsetValue(_ListLeavesInBox_ReturnAddrBytes))
		return oListLeavesInBox(ecx, mins, maxs, list, list_max);

	auto info = *(RenderableInfo_t**)((uintptr_t)_AddressOfReturnAddress() + 0x14); // todo: nit; make a sig for this address of return addr offset | ?
	if (!info || !info->m_pRenderable)
		return oListLeavesInBox(ecx, mins, maxs, list, list_max);

	auto base_entity = info->m_pRenderable->GetIClientUnknown()->GetBaseEntity();
	if(!base_entity || base_entity->IsLocalPlayer() || !base_entity->IsPlayer())
		return oListLeavesInBox(ecx, mins, maxs, list, list_max);

	const bool is_enemy = base_entity->IsEnemy(LocalPlayer.Entity);

	if(is_enemy && !var.visuals.pf_enemy.vf_main.b_render || !var.visuals.pf_enemy.vf_main.b_enabled && !var.visuals.pf_enemy.vf_main.chams.b_enabled && !var.visuals.pf_enemy.vf_main.chams.b_xqz)
		return oListLeavesInBox(ecx, mins, maxs, list, list_max);

	if(!is_enemy && !var.visuals.pf_teammate.vf_main.b_render || !var.visuals.pf_teammate.vf_main.b_enabled && !var.visuals.pf_teammate.vf_main.chams.b_enabled && !var.visuals.pf_teammate.vf_main.chams.b_xqz)
		return oListLeavesInBox(ecx, mins, maxs, list, list_max);

	info->m_Flags &= ~0x100;
	info->m_Flags2 |= 0xC0;

	return oListLeavesInBox(ecx, coord_mins, coord_maxs, list, list_max);
}