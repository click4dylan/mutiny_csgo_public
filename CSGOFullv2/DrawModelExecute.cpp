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
#include "Adriel/stdafx.hpp"

DrawModelExecuteFn oDrawModelExecute;

/*void ForceMaterial(bool ignoreZ, Color clr, IMaterial* material)
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

	Interfaces::ModelRender->ForcedMaterialOverride(material);
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

	Interfaces::ModelRender->ForcedMaterialOverride(material);
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

	Interfaces::ModelRender->ForcedMaterialOverride(material);
}
void ForceMaterialAlpha(int alpha, IMaterial* material)
{
	if (!material)
		return;

	material->AlphaModulate(clamp((float)alpha / 255.f, 0.1f, 1.f));
	Interfaces::ModelRender->ForcedMaterialOverride(material);
}*/

void __fastcall Hooks::DrawModelExecute(void* thisptr, int edx, void* ctx, void* state, const ModelRenderInfo_t& pInfo, matrix3x4_t* pCustomBoneToWorld)
{
	START_PROFILING
	// update LocalPlayer
	LocalPlayer.Get(&LocalPlayer);
	
	// ingame and we exist
	if (variable::get().visuals.b_enabled && Interfaces::EngineClient->IsInGame() && LocalPlayer.Entity)
	{
		// remove AUG/SG scope blur the right way
		static bool prev_scope_blur = variable::get().visuals.b_no_scope;
		static float prev_spawn_time = LocalPlayer.Entity->GetSpawnTime();
		float spawn_time = LocalPlayer.Entity->GetSpawnTime();

		// only change materials once to save CPU
		if (g_Visuals.BlurMaterials.size() == 3 && (prev_scope_blur != variable::get().visuals.b_no_scope || spawn_time != prev_spawn_time))
		{
			for (auto mat : g_Visuals.BlurMaterials)
				mat->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, variable::get().visuals.b_no_scope);

			prev_scope_blur = variable::get().visuals.b_no_scope;
			prev_spawn_time = spawn_time;
		}

		//if (variable::get().visuals.b_enabled)
		{
			// get model name
			std::string _ModelName = Interfaces::ModelInfoClient->GetModelName(pInfo.pModel);

			// arms chams
			//if (_ModelName.find(XorStrCT("arms")) != std::string::npos)
			if(IntFromChars(_ModelName, 24) == 'smra' && IntFromChars(_ModelName, 15) == 'om_v')
			{
				if (!variable::get().visuals.vf_arms.b_render && variable::get().visuals.vf_arms.b_enabled)
				{
					END_PROFILING
					return;
				}
			}
			// view-weapon chams
			//else if (_ModelName.find(XorStrCT("weapons/v")) != std::string::npos)
			else if (IntFromChars(_ModelName, 7) == 'paew' && _ModelName[15] == 'v')
			{
				if (!variable::get().visuals.vf_viewweapon.b_render && variable::get().visuals.vf_viewweapon.b_enabled)
				{
					END_PROFILING
					return;
				}
			}
			// bomb chams
			else if (IntFromChars(_ModelName, 17) == '_dei')
			{
				if (!variable::get().visuals.vf_bomb.b_render && variable::get().visuals.vf_bomb.b_enabled)
				{
					END_PROFILING
					return;
				}
			}
			// thrown projectiles
			else if (IntFromChars(_ModelName, 16) == '_qe_')
			{
				if (!variable::get().visuals.vf_projectile.b_render && variable::get().visuals.vf_projectile.b_enabled)
				{
					END_PROFILING
						return;
				}
			}
			// dropped weapon chams
 			else if (_ModelName[15] == 'w')
			{
				if (!variable::get().visuals.vf_weapon.b_render && variable::get().visuals.vf_weapon.b_enabled)
				{
					END_PROFILING
					return;
				}
			}
			// player chams
			//else if (_ModelName.find(XorStrCT("play")) != std::string::npos)
			else if(IntFromChars(_ModelName, 21) == 'yalp') // models/player/custom_player/*
			{
				CBaseEntity* entity = (CBaseEntity*)Interfaces::ClientEntList->GetClientEntity(pInfo.entity_index);

				if (entity)
				{
					if (!entity->IsRagdoll())
					{
						if (!entity->IsLocalPlayer())
						{
							bool enemy = entity->IsEnemy(LocalPlayer.Entity);
							if (enemy && !variable::get().visuals.pf_enemy.vf_main.b_render && variable::get().visuals.pf_enemy.vf_main.b_enabled)
							{
								END_PROFILING
								return;
							}
							else if (!enemy && !variable::get().visuals.pf_teammate.vf_main.b_render && variable::get().visuals.pf_teammate.vf_main.b_enabled)
							{
								END_PROFILING
								return;
							}
						}
						else
						{
							if (!variable::get().visuals.pf_local_player.vf_main.b_render && variable::get().visuals.pf_local_player.vf_main.b_enabled)
							{
								END_PROFILING
								return;
							}
						}
					}
					else
					{
						if (!variable::get().visuals.vf_ragdolls.b_render && variable::get().visuals.vf_ragdolls.b_enabled)
						{
							END_PROFILING
							return;
						}
					}
				}
			}
		}
	}

	oDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);
	END_PROFILING
}
