#include "ui.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui_internal.h"

#include "ImGui/font_compressed.h"
#include "ImGui/image_compressed.h"

#include "imgui_extra.hpp"
#include "adr_util.hpp"
#include "config.hpp"
#include "../IModelInfoClient.h"
#include "adr_math.hpp"
#include "../LocalPlayer.h"
#include "clantag_changer.hpp"

#include "../UsedConvars.h"
#include "renderer.hpp"

#include "..\misc.h"
#include "..\WaypointSystem.h"
#include <vector>

ui::ui() : b_visible(false), f_scale(1.f)
{
}

ui::~ui() = default;

void ui::skin() const
{
	auto& var = variable::get();
	ImGui::SetNextWindowSize(ImVec2(700 /** f_scale*/, 422 /** f_scale*/));
	ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);

	//auto skin_title = XorStr(ICON_FA_PAINT_BRUSH "  SKIN");
	//decrypts(0)
	std::string skin_title = ICON_FA_PAINT_BRUSH;
	skin_title += XorStr(" SKIN");
	//encrypts(0)

	if (ImGui::Begin(skin_title.data(), &var.ui.b_skin, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
	{
		if (std::get<2>(var.global.cfg_mthread))
		{
			ImGui::PushFont(ImFontEx::header);
			const auto col_controller = var.ui.col_controller.color().ToImGUI();
			//ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), adr_util::string::format(XorStr(ICON_FA_COGS "  Please wait  %c"), "|/-\\"[static_cast<int>(ImGui::GetTime() / 0.25f) & 3]).c_str());
			//decrypts(0)
			std::string format = ICON_FA_COGS;
			format += XorStr("  Please wait  %c");
			ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), adr_util::string::format(format, XorStr("|/-\\")[static_cast<int>(ImGui::GetTime() / 0.25f) & 3]).c_str());
			//encrypts(0)
			ImGui::PopFont();
			ImGui::End();
			return;
		}

		ImGui::End();
	}
}

void ui::playerlist() const
{
	RENDER_MUTEX.Lock();

	LocalPlayer.Get(&LocalPlayer);
	if (!LocalPlayer.Entity || !Interfaces::EngineClient->IsInGame())
	{
		RENDER_MUTEX.Unlock();
		return;
	}

	auto& var = variable::get();

	ImGui::SetNextWindowSize(ImVec2(700 /** f_scale*/, 400 /** f_scale*/));
	ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);

	const auto col_controller = var.ui.col_controller.color().ToImGUI();

	//auto misc_title = XorStr(ICON_FA_USER "  MISC");
	//decrypts(0)
	std::string playerlist_title = ICON_FA_USERS;
	playerlist_title += XorStr(" PLAYERS");
	//encrypts(0)

	static int SelectedPlayerIndex = INVALID_PLAYER;
	bool FoundSelectedPlayerIndex = false;

	if (ImGui::Begin(playerlist_title.data(), &var.ui.b_playerlist, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
	{
		ImGui::Columns(2, nullptr, false);
		ImGui::PushItemWidth(150);
		for (int i = 1; i <= MAX_PLAYERS; ++i)
		{
			CBaseEntity *pEntity = Interfaces::ClientEntList->GetBaseEntity(i);
			if (pEntity && !pEntity->IsLocalPlayer() && pEntity->GetTeam() != TEAM_GOTV && !pEntity->IsBot())
			{
				CPlayerrecord *_playerRecord = pEntity->ToPlayerRecord();
				if (_playerRecord)
				{
					std::string victim_name = adr_util::sanitize_name(_playerRecord->m_PlayerInfo.name);

					bool IsEnemy = pEntity->IsEnemy(LocalPlayer.Entity);
					
					if (IsEnemy)
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.f));

					if (ImGui::Selectable(victim_name.c_str(), SelectedPlayerIndex == i))
						SelectedPlayerIndex = i;

					if (!FoundSelectedPlayerIndex && i == SelectedPlayerIndex)
						FoundSelectedPlayerIndex = true;
					
					if (IsEnemy)
						ImGui::PopStyleColor();
				}
			}
		}
		ImGui::NextColumn();
		if (FoundSelectedPlayerIndex && SelectedPlayerIndex != INVALID_PLAYER)
		{
			CPlayerrecord *_playerRecord = g_LagCompensation.GetPlayerrecord(SelectedPlayerIndex);
			CBaseEntity* _Entity = Interfaces::ClientEntList->GetBaseEntity(SelectedPlayerIndex);
			if (_Entity && _playerRecord && _playerRecord->m_pEntity == _Entity)
			{
				int side = _playerRecord->m_iResolveSide;
				auto bodyresolveinfo = _playerRecord->GetBodyHitResolveInfo();

				const char *sides[MAX_RESOLVE_SIDES + 1];

				//decrypts(0)

				if (side == ResolveSides::NONE && _playerRecord->m_iResolveMode == RESOLVE_MODE_MANUAL)
					sides[ResolveSides::NONE] = XorStr("None (Forced)");
				else
					sides[ResolveSides::NONE] = XorStr("None");

				if (side == ResolveSides::POSITIVE_35 && _playerRecord->m_iResolveMode == RESOLVE_MODE_MANUAL)
					sides[ResolveSides::POSITIVE_35] = XorStr("+35 (Forced)");
				else
					sides[ResolveSides::POSITIVE_35] = XorStr("+35");

				if (side == ResolveSides::NEGATIVE_35 && _playerRecord->m_iResolveMode == RESOLVE_MODE_MANUAL)
					sides[ResolveSides::NEGATIVE_35] = XorStr("-35 (Forced)");
				else
					sides[ResolveSides::NEGATIVE_35] = XorStr("-35");

				if (side == ResolveSides::POSITIVE_60 && _playerRecord->m_iResolveMode == RESOLVE_MODE_MANUAL)
					sides[ResolveSides::POSITIVE_60] = XorStr("60 (Forced)");
				else
					sides[ResolveSides::POSITIVE_60] = XorStr("60");

				if (side == ResolveSides::NEGATIVE_60 && _playerRecord->m_iResolveMode == RESOLVE_MODE_MANUAL)
					sides[ResolveSides::NEGATIVE_60] = XorStr("-60 (Forced)");
				else
					sides[ResolveSides::NEGATIVE_60] = XorStr("-60");

				sides[ResolveSides::MAX_RESOLVE_SIDES] = XorStr("Automatic");

				if (ImGui::Combo(XorStrCT("Resolve Side"), &side, sides, ARRAYSIZE(sides)))
				{
					if (side == MAX_RESOLVE_SIDES)
					{
						_playerRecord->m_iResolveMode = RESOLVE_MODE_BRUTE_FORCE;
						if (bodyresolveinfo && bodyresolveinfo->m_bIsBodyHitResolved)
						{
							if (bodyresolveinfo->m_bIsNearMaxDesyncDelta)
							{
								if (bodyresolveinfo->m_flDesyncDelta > 0.0f)
									side = ResolveSides::POSITIVE_60;
								else
									side = ResolveSides::NEGATIVE_60;
							}
							else if(bodyresolveinfo->m_flDesyncDelta > 0.0f)
							{
								side = ResolveSides::POSITIVE_35;
							}
							else
							{
								side = ResolveSides::NEGATIVE_35;
							}
						}
						else
						{
							side = ResolveSides::POSITIVE_60;
						}
					}
					else
					{
						_playerRecord->m_iResolveMode = RESOLVE_MODE_MANUAL;
					}

					_playerRecord->SetResolveSide((ResolveSides)side, XorStrCT(__FUNCTION__));
				}

				//encrypts(0)

				//decrypts(0)

				std::string stra = XorStrCT("Force Not Legit");
				ImGui::Checkbox(stra.c_str(), &_playerRecord->m_bForceNotLegit);

				std::string str = XorStrCT("Detect Desync Amount");
				if (bodyresolveinfo && bodyresolveinfo->m_bIsBodyHitResolved)
				{
					if (bodyresolveinfo->m_bIsNearMaxDesyncDelta)
					{
						if (g_LagCompensation.IsResolveSide60((ResolveSides)side))
						{
#if defined _DEBUG || defined INTERNAL_DEBUG || defined TEST_BUILD
							str += XorStrCT(" (Detected ");
							str += std::to_string(bodyresolveinfo->m_flDesyncDelta);
							str += ") ";
#else
							str += XorStrCT(" (Detected) ");
#endif
						}
					}
					else if (g_LagCompensation.IsResolveSide35((ResolveSides)side))
					{
#if defined _DEBUG || defined INTERNAL_DEBUG || defined TEST_BUILD
						str += XorStrCT(" (Detected ");
						str += std::to_string(bodyresolveinfo->m_flDesyncDelta);
						str += ") ";
#else
						str += XorStrCT(" (Detected) ");
#endif
					}
				}

				ImGui::Checkbox(str.c_str(), &_playerRecord->m_bAllowBodyHitResolver);

				if (_playerRecord->m_iResolveMode == RESOLVE_MODE_MANUAL)
				{
					auto& _style = ImGui::GetStyle();
					ImGui::PushStyleColor(ImGuiCol_Text, _style.Colors[ImGuiCol_TextDisabled]);
				}

				ImGui::Checkbox(XorStr("Enable Moving Resolver"), &_playerRecord->m_bAllowMovingResolver);
				ImGui::Checkbox(XorStr("Enable Jitter Resolver"), &_playerRecord->m_bAllowJitterResolver);

				if (_playerRecord->m_iResolveMode == RESOLVE_MODE_MANUAL)
					ImGui::PopStyleColor();

				//ImGui::Separator();

				std::string is_alive = XorStrCT("Is Alive: ");

				if (_Entity->GetAlive())
					is_alive += XorStrCT("Yes");
				else
					is_alive += XorStrCT("No");

				ImGui::Text(is_alive.c_str());

				std::string is_dormant = XorStrCT("Is Dormant: ");

				if (_Entity->GetDormant())
					is_dormant += XorStrCT("Yes");
				else
					is_dormant += XorStrCT("No");

				ImGui::Text(is_dormant.c_str());

#ifdef USE_FAR_ESP
				std::string far_esp = XorStrCT("Visible by Far ESP: ");

				if (_playerRecord->GetFarESPPacket())
					far_esp += XorStrCT("Yes");
				else
					far_esp += XorStrCT("No");

				ImGui::Text(far_esp.c_str());
#endif

				std::string is_legit = XorStrCT("Is Legit: ");

				if (_playerRecord->m_bLegit && !_playerRecord->m_bForceNotLegit)
					is_legit += XorStrCT("Yes");
				else
					is_legit += XorStrCT("No");

				ImGui::Text(is_legit.c_str());

				CTickrecord *_currentRecord = _playerRecord->GetCurrentRecord();
				if (_currentRecord)
				{
					std::string angles = XorStrCT("Eye Angles: ");
					QAngle eyeangles = _currentRecord->m_EyeAngles;
					NormalizeAngles(eyeangles);
					angles = angles + std::to_string(eyeangles.x) + " " + std::to_string(eyeangles.y);

					ImGui::Text(angles.c_str());

					std::string lowerbodyyaw = XorStrCT("Lower Body Yaw: ");
					lowerbodyyaw += std::to_string(_currentRecord->m_flLowerBodyYaw);
					ImGui::Text(lowerbodyyaw.c_str());

					std::string goalfeetyaw = XorStrCT("Feet Yaw: ");
					goalfeetyaw += std::to_string(_currentRecord->m_AbsAngles.y);
					ImGui::Text(goalfeetyaw.c_str());

					std::string goalfeetyaw_normalized = XorStrCT("Feet Yaw (Normalized): ");
					goalfeetyaw_normalized += std::to_string(AngleNormalize(_currentRecord->m_AbsAngles.y));
					ImGui::Text(goalfeetyaw_normalized.c_str());

					std::string upperbodyyaw = XorStrCT("Upper Body Yaw: ");
					float flMin, flMax;
					GetPoseParameterRangeGame(_playerRecord->m_pEntity, body_yaw, flMin, flMax);
					float bodyyawpose = _currentRecord->m_flPoseParams[body_yaw] * (flMax - flMin) + flMin;
					upperbodyyaw += std::to_string(bodyyawpose);
					ImGui::Text(upperbodyyaw.c_str());

					std::string moving_in_place = XorStrCT("Moving In Place: ");

					if (_currentRecord->m_bIsUsingMovingLBYMeme)
						moving_in_place += XorStrCT("Yes");
					else
						moving_in_place += XorStrCT("No");

					ImGui::Text(moving_in_place.c_str());

					std::string max_desync_delta = XorStrCT("Max Desync Delta: ");
					max_desync_delta += std::to_string(_Entity->GetMaxDesyncDelta(_currentRecord));
					ImGui::Text(max_desync_delta.c_str());
				}

				std::string missed_count = XorStr("Shots Missed: ");
				missed_count += std::to_string(_playerRecord->m_iShotsMissed); \

				ImGui::Text(missed_count.c_str());

				if (bodyresolveinfo)
				{
					std::string missed_impact_count = XorStr("Detected Desync Shots Missed: ");
					missed_impact_count += bodyresolveinfo->m_iShotsMissed;
				}

				//encrypts(0)
			}
			else
			{
				SelectedPlayerIndex = INVALID_PLAYER;
			}
		}
		else
		{
			SelectedPlayerIndex = INVALID_PLAYER;
		}
		ImGui::PopItemWidth();
		ImGui::End();
	}
	RENDER_MUTEX.Unlock();
}

void ui::misc() const
{
	auto& var = variable::get();

	ImGui::SetNextWindowSize(ImVec2(700 /** f_scale*/, 400 /** f_scale*/));
	ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);

	const auto col_controller = var.ui.col_controller.color().ToImGUI();

	//auto misc_title = XorStr(ICON_FA_USER "  MISC");
	//decrypts(0)
	std::string misc_title = ICON_FA_USER;
	misc_title += XorStr(" MISC");
	//encrypts(0)

	if (ImGui::Begin(misc_title.data(), &var.ui.b_misc, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
	{
		if (std::get<2>(var.global.cfg_mthread))
		{
			ImGui::PushFont(ImFontEx::header);
			//ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), adr_util::string::format(XorStr(ICON_FA_COGS "  Please wait  %c"), "|/-\\"[static_cast<int>(ImGui::GetTime() / 0.25f) & 3]).c_str());
			//decrypts(0)
			std::string format = ICON_FA_COGS;
			format += XorStr("  Please wait  %c");
			ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), adr_util::string::format(format, XorStr("|/-\\")[static_cast<int>(ImGui::GetTime() / 0.25f) & 3]).c_str());
			//encrypts(0)
			ImGui::PopFont();
			ImGui::End();
			return;
		}

		//ImGui::Separator();
		//ImGui::Dummy(ImVec2(-10, 0));
		//ImGui::SameLine();

		ImGui::Columns(2, nullptr, false);

		//decrypts(0)
		ImGui::BeginChild(XorStr("##MISC"), ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar); // no scrollbar visible but can scroll with mouse
		{
			ImGui::PushItemWidth(ImGui::GetWindowWidth() / 2);
			if (!LocalPlayer.IsAllowedUntrusted())
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
			}
			ImGuiEx::Checkbox(XorStr("Unlimited Duck"), &var.misc.b_unlimited_duck, true, nullptr, true, ImVec4(1.f, 1.f, 0.f, 1.f));
			ImGuiEx::SetTip(XorStr("Enable an exploit that will remove the cooldown between multiple crouches (required for fakeduck)"));
			if (!LocalPlayer.IsAllowedUntrusted())
			{
				ImGui::PopItemFlag();
				ImGui::PopStyleVar();
			}
			ImGui::Separator();

			ImGuiEx::Checkbox(XorStr("Bhop"), &var.misc.b_bhop);
			ImGuiEx::SetTip(XorStr("Automatically jump at the perfect time to achieve maximum velocity allowed on a server while holding the jump key"));

			ImGui::Separator();

			ImGuiEx::Checkbox(XorStr("Autostrafe"), &var.misc.b_autostrafe);
			ImGuiEx::SetTip(XorStr("Automatically strafe to achieve maximum airborne acceleration allowed on a server while in air"));

			ImGui::Separator();

			ImGuiEx::Checkbox(XorStr("Single Scope"), &var.misc.b_single_scope);
			ImGuiEx::SetTip(XorStr("Automatically unscope when trying to double scope"));

			ImGui::Separator();

			ImGuiEx::Checkbox(XorStr("Watermark"), &var.misc.b_water_mark);
			ImGuiEx::SetTip(XorStr("displays cheat name to the corner of your screen with version/build date"));

			ImGui::Separator();

			ImGui::AlignTextToFramePadding();
			ImGui::TextColored(ImColor(255, 255, 0, 255), XorStr("Fakeduck:"));
			ImGui::SameLine(ImGui::GetWindowWidth() / 2);
			ImGuiEx::KeyBindButton(XorStr("##MISC_FAKEDUCK"), var.misc.i_fakeduck_key, false);
			ImGuiEx::SetTip(XorStr("Choose a button to hold to use an exploit to emulate us crouching but allow us to shoot over objects (requires unlimited duck)"));

			ImGui::Separator();

			ImGui::AlignTextToFramePadding();
			ImGui::Text(XorStr("Minwalk:"));
			ImGui::SameLine(ImGui::GetWindowWidth() / 2);
			ImGuiEx::KeyBindButton(XorStr("##MISC_MINWALK"), var.misc.i_minwalk_key, false);
			ImGuiEx::SetTip(XorStr("Choose a button to hold to strengthen the effect of desync by forcing us to move at the minimum amount of speed to retain maximum accuracy"));

			ImGui::Separator();

			// todo: nit; port over from v4 or v3
			////ImGui::AlignFirstTextHeightToWidgets();
			//ImGui::Text(XorStr("Override Resolver:"));
			//ImGui::SameLine(ImGui::GetWindowWidth() / 2);
			//ImGuiEx::KeyBindButton(XorStr("##MISC_OVERRIDE"), var.ragebot.i_override_key, false);

			ImGui::AlignTextToFramePadding();
			ImGui::Text(XorStr("Change Name:"));
			ImGui::SameLine(ImGui::GetWindowWidth() / 2);
			ImGui::Combo(XorStr("##NAMEMODE"), &var.misc.namechanger.i_mode, XorStrCT(" Advertise\0 No Name\0 Backwards\00"));

			if (var.misc.namechanger.b_enabled)
			{
				ImGui::ButtonEx(XorStr("Set"), ImVec2(-1, 20), ImGuiButtonFlags_Disabled);
			}
			else
			{
				if (ImGui::ButtonEx(XorStr("Set"), ImVec2(-1, 20), ImGuiButtonFlags_PressedOnClickRelease))
					var.misc.namechanger.b_enabled = true;
			}
			ImGui::Separator();

			ImGui::AlignTextToFramePadding();
			ImGuiEx::Checkbox(XorStr("Clantag"), &var.misc.clantag.b_enabled.b_state, true, &var.misc.clantag.b_enabled);
			ImGuiEx::SetTip(XorStr("Enable a forced clantag"));
			ImGui::SameLine(ImGui::GetWindowWidth() / 2);
			ImGui::Combo(XorStr("##MISC_CLANTAG"), &var.misc.clantag.i_anim, XorStrCT(" Static\0 Custom\00"));

			if (var.misc.clantag.i_anim == 1)
			{
				ImGui::AlignTextToFramePadding();
				ImGui::Text(XorStr("Custom Clantag:"));
				ImGui::SameLine(ImGui::GetWindowWidth() / 2);

				char buf[16] = { 0 };
				strcpy_s<16>(buf, var.misc.clantag.str_text.data());
				ImGui::InputText(XorStr("##MISC_CUSTOM_CLANTAG"), buf, sizeof(buf));
				ImGuiEx::SetTip(XorStr("Type out your own custom clantag"));

				static std::string last_buf = "";
				if (buf != last_buf)
				{
					last_buf = buf;

					if (last_buf.length() > 15)
						last_buf.erase(15, (last_buf.length() - 15));

					var.misc.clantag.str_text = last_buf;
				}
			}

			ImGui::Separator();

			ImGuiEx::Checkbox(XorStr("Preserve Killfeed"), &var.misc.b_preserve_killfeed);
			ImGuiEx::SetTip(XorStr("Force the killfeed to not disappear after killing an enemy"));

			if (!var.misc.b_preserve_killfeed)
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
			}

			ImGui::SameLine(ImGui::GetWindowWidth() / 2);
			ImGuiEx::Checkbox(XorStr("Include Assists"), &var.misc.b_include_assists_killfeed);
			ImGuiEx::SetTip(XorStr("Force the killfeed to not disappear after assisting in killing an enemy"));

			ImGui::AlignTextToFramePadding();
			ImGui::Text(XorStr("Killfeed Duration:"));
			ImGui::SameLine(ImGui::GetWindowWidth() / 2);
			ImGui::SliderFloat(XorStr("##MISC_KILLFEED"), &var.misc.f_preserve_killfeed, 1.f, 300.f, XorStr("%.f secs"));
			ImGuiEx::SetTip(XorStr("Choose how long until the killfeed including you, takes to disappear"));

			if (!var.misc.b_preserve_killfeed)
			{
				ImGui::PopItemFlag();
				ImGui::PopStyleVar();
			}

			ImGui::Separator();

			ImGui::AlignTextToFramePadding();
			ImGui::Text(XorStr("disconnect2 command reason:"));
			ImGui::SameLine(ImGui::GetWindowWidth() / 2);

			char buf[512] = { 0 };
			strcpy_s<512>(buf, var.misc.disconnect_reason.str_text.data());
			ImGui::InputText(XorStr("##MISC_DISCONNECT_REASON"), buf, sizeof(buf));
			ImGuiEx::SetTip(XorStr("When using command disconnect2, it will tell other players this reason you left"));

			static std::string last_discon_buf = "";
			if (buf != last_discon_buf)
			{
				last_discon_buf = buf;

				if (last_discon_buf.length() > 511)
					last_discon_buf.erase(511, (last_discon_buf.length() - 511));

				var.misc.disconnect_reason.str_text = last_discon_buf;
			}

			ImGui::PopItemWidth();
			ImGui::EndChild();
		}
		//encrypts(0)

		ImGui::NextColumn();

		//decrypts(0)
		if(ImGui::BeginChild(XorStr("##MISC_AUTOBUY"), ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar)) // no scrollbar visible but can scroll with mouse
		{
			ImGui::PushItemWidth(ImGui::GetWindowWidth() / 2);

			ImGui::PushFont(ImFontEx::header);
			ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Autobuy"));
			ImGui::PopFont();
			ImGui::Separator();
			ImGui::Spacing();

			//decrypts(1)
			ImGuiEx::Checkbox(XorStr("Enable##AUTOBUY"), &var.misc.b_autobuy);
			ImGuiEx::SetTip(XorStr("Automatically buy weapons, grenades and items upon the start of every round"));
			//encrypts(1)

			if (!var.misc.b_autobuy)
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
			}

			//ImGui::Checkbox(XorStr("Algorithmic Purchases"), &var.misc.b_autobuy_ai);
			//ImGui::AlignFirstTextHeightToWidgets();
			ImGui::AlignTextToFramePadding();
			ImGui::Text(XorStr("T Primary:"));
			ImGui::SameLine(ImGui::GetWindowWidth() / 2);
			ImGui::Combo(XorStr("##MISC_AB_T_PRIM"), &var.misc.i_autobuy_t_primary, XorStrCT("None\0AK47\0Galil\0SG-553\0G3GS1\0MAC-10\0Sawed Off\0MP7-MP5SD\0AWP\0Scout\0UMP-45\0Bizon\0P90\0Nova\0XM1014\0M249\0Negev\0\0"));

			//ImGui::AlignFirstTextHeightToWidgets();
			ImGui::AlignTextToFramePadding();
			ImGui::Text(XorStr("T Secondary:"));
			ImGui::SameLine(ImGui::GetWindowWidth() / 2);
			ImGui::Combo(XorStr("##MISC_AB_T_SEC"), &var.misc.i_autobuy_t_secondary, XorStrCT("None\0TEC-9/CZ-75\0Deagle/R8\0P250\0Dual Berettas\0\0"));

			//ImGui::AlignFirstTextHeightToWidgets();
			ImGui::AlignTextToFramePadding();
			ImGui::Text(XorStr("CT Primary:"));
			ImGui::SameLine(ImGui::GetWindowWidth() / 2);
			ImGui::Combo(XorStr("##MISC_AB_CT_PRIM"), &var.misc.i_autobuy_ct_primary, XorStrCT("None\0M4A4/M4A1-S\0Famas\0AUG\0SCAR-20\0MP9\0MAG-7\0MP7-MP5SD\0AWP\0Scout\0UMP-45\0Bizon\0P90\0Nova\0XM1014\0M249\0Negev\0\0"));

			//ImGui::AlignFirstTextHeightToWidgets();
			ImGui::AlignTextToFramePadding();
			ImGui::Text(XorStr("CT Secondary:"));
			ImGui::SameLine(ImGui::GetWindowWidth() / 2);
			ImGui::Combo(XorStr("##MISC_AB_CT_SEC"), &var.misc.i_autobuy_ct_secondary, XorStrCT("None\0Five-Seven/CZ-75\0Deagle/R8\0P250\0Dual Berettas\0\0"));

			// todo: nit; make this able to do multiple of one and keep track of how many nades are allowed (4 nades total, only 2 flashbangs, 1 smoke, 1 fire, 1 frag, 1 decoy max)
			//decrypts(1)
			static std::vector<std::tuple<const char*, bool*, const char*>> grenades_buybot_ui =
			{
				{ XorStr("Flashbang"),						&var.misc.b_autobuy_flash,	nullptr },
				{ XorStr("Frag Grenade"),					&var.misc.b_autobuy_frag,	nullptr },
				{ XorStr("Smoke Grenade"),					&var.misc.b_autobuy_smoke,	nullptr },
				{ XorStr("Molotov/Incendiary Grenade"),		&var.misc.b_autobuy_fire,	nullptr },
				{ XorStr("Decoy Grenade"),					&var.misc.b_autobuy_decoy,	nullptr },
			};

			ImGui::AlignTextToFramePadding();
			ImGui::Text(XorStr("Grenades:"));
			ImGui::SameLine((ImGui::GetWindowWidth() / 2));
			ImGuiEx::SelectableCombo(XorStr("##MISC_AB_GRENADES"), grenades_buybot_ui);
			ImGuiEx::SetTip(XorStr("Choose what grenades you'd like to autobuy"));
			//encrypts(1)

			//decrypts(1)
			static std::vector<std::tuple<const char*, bool*, const char*>> armor_misc_ui =
			{
				{ XorStr("Kevlar"),					&var.misc.b_autobuy_armor,	nullptr },
				{ XorStr("Defuse/Rescue Kit"),		&var.misc.b_autobuy_kit,	nullptr },
				{ XorStr("Zeus"),					&var.misc.b_autobuy_zeus,	nullptr },
			};

			ImGui::AlignTextToFramePadding();
			ImGui::Text(XorStr("Armor/Misc:"));
			ImGui::SameLine((ImGui::GetWindowWidth() / 2));
			ImGuiEx::SelectableCombo(XorStr("##MISC_AB_ARMORMISC"), armor_misc_ui);
			ImGuiEx::SetTip(XorStr("Choose if you'd like to autobuy armor/kit/zeus"));
			//encrypts(1)

			if (!var.misc.b_autobuy)
			{
				ImGui::PopItemFlag();
				ImGui::PopStyleVar();
			}

			ImGui::PopItemWidth();
			ImGui::EndChild();
		}
		//encrypts(0)

		ImGui::Columns(1);
		ImGui::End();
	}
}

void ui::visual() const
{
	auto& var = variable::get();

	ImGui::SetNextWindowSize(ImVec2(900 /** f_scale*/, 525 /** f_scale*/));
	ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);

	const auto col_controller = var.ui.col_controller.color().ToImGUI();

	//auto visuals_title = XorStr(ICON_FA_EYE "  VISUALS");
	//decrypts(0)
	std::string visuals_title = ICON_FA_EYE;
	visuals_title += XorStr("  VISUALS");
	//encrypts(0)

	if (ImGui::Begin(visuals_title.data(), &var.ui.b_visual, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse /*| ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse*/))
	{
		if (std::get<2>(var.global.cfg_mthread))
		{
			ImGui::PushFont(ImFontEx::header);
			//ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), adr_util::string::format(XorStr(ICON_FA_COGS "  Please wait  %c"), "|/-\\"[static_cast<int>(ImGui::GetTime() / 0.25f) & 3]).c_str());
			//decrypts(0)
			std::string format = ICON_FA_COGS;
			format += XorStr("  Please wait  %c");
			ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), adr_util::string::format(format, XorStr("|/-\\")[static_cast<int>(ImGui::GetTime() / 0.25f) & 3]).c_str());
			//encrypts(0)
			ImGui::PopFont();
			ImGui::End();
			return;
		}

		static auto active_tab = 0;

		//decrypts(0)
		std::vector<const char*> tab_groups =
		{
			XorStr("MAIN"),
			XorStr("MISC")
		};
		static auto f_tab_group_max_size = (ImGui::GetWindowWidth() - 21.f) / tab_groups.size();

		ImGui::Separator();
		ImGui::Dummy(ImVec2(-10, 0));
		ImGui::SameLine();
		ImGuiEx::RenderTabs(tab_groups, active_tab, f_tab_group_max_size, 15, true);
		//encrypts(0)
		ImGui::Separator();

		if (active_tab == 1)
		{
			ImGui::Columns(4, nullptr, false);
			{
				//decrypts(0)
				ImGui::BeginChild(XorStr("##VISUALS_COLUMN1"), ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize);
				{
					ImGui::PushItemWidth(-1);
					auto f_win_width = ImGui::GetWindowWidth();

					ImGui::PushFont(ImFontEx::header);
					ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Removals"));
					ImGui::PopFont();
					ImGui::Separator();
					ImGui::Spacing();

					//decrypts(1)
					static std::vector<std::tuple<const char*, bool*, const char*>> removals_ui =
					{
						{ XorStr("Smoke"),					&var.visuals.b_no_smoke,			XorStr("Remove smoke from smoke grenades") },
						{ XorStr("Flash Effect"),			&var.visuals.b_no_flash,			XorStr("Remove the blinding effect from flashbang grenades") },
						{ XorStr("Visual Recoil"),			&var.visuals.b_no_visual_recoil,	XorStr("Remove the visual jerks weapons give when they're fired") },
						{ XorStr("Scope Zoom"),				&var.visuals.b_no_zoom,				XorStr("Remove the actual zoom effect the camera gives when scoping in") },
						{ XorStr("Scope Overlay & Blur"),	&var.visuals.b_no_scope,			XorStr("Remove the scope overlay and blur weapons give when scoping in") },
					};
					ImGuiEx::SelectableCombo(XorStr("##VISUALS_MISC_REMOVALS"), removals_ui, 32);
					ImGuiEx::SetTip(XorStr("Choose what visual features you'd like to remove"));
					//encrypts(1)

					//if (ImGui::ListBoxHeader(XorStr("Removals##VISUALS_MISC_REMOVALS"), ImVec2(-1, 101)))
					//{
					//	//decrypts(1)
					//	ImGui::Selectable(XorStr("Smoke"), &var.visuals.b_no_smoke);
					//	ImGuiEx::SetTip(XorStr("Remove smoke from smoke grenades"));
					//	ImGui::Selectable(XorStr("Flash Effect"), &var.visuals.b_no_flash);
					//	ImGuiEx::SetTip(XorStr("Remove the blinding effect from flashbang grenades"));
					//	ImGui::Selectable(XorStr("Visual Recoil"), &var.visuals.b_no_visual_recoil);
					//	ImGuiEx::SetTip(XorStr("Remove the visual jerks weapons give when they're fired"));
					//	ImGui::Selectable(XorStr("Scope Zoom"), &var.visuals.b_no_zoom);
					//	ImGuiEx::SetTip(XorStr("Remove the actual zoom effect the camera gives when scoping in"));
					//	ImGui::Selectable(XorStr("Scope Overlay & Blur"), &var.visuals.b_no_scope);
					//	ImGuiEx::SetTip(XorStr("Remove the scope overlay and blur weapons give when scoping in"));
					//	ImGui::ListBoxFooter();
					//	//encrypts(1)
					//}

					ImGui::Separator();

					//ImGuiEx::Checkbox(XorStr("Offscreen"), &var.visuals.b_offscreen_esp);
					//ImGui::SameLine(f_win_width / 2);
					//ImGui::SliderFloat(XorStr("##OFFSCREEN_SIZE"), &var.visuals.f_offscreen_esp, 4.f, 20.f, XorStr("SIZE: %0.1f0"));

					ImGui::Spacing();
					ImGui::PushFont(ImFontEx::header);
					ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Indicators"));
					ImGui::PopFont();
					ImGui::Separator();
					ImGui::Spacing();

					ImGuiEx::Checkbox(XorStr("Enable##MISC_INDICATORS"), &var.visuals.b_indicators);
					ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 12.f);

					if (!var.visuals.b_indicators)
					{
						ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
						ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
					}

					ImGuiEx::Checkbox(XorStr("Only Active"), &var.visuals.b_indicators_only_active);
					ImGuiEx::SetTip(XorStr("Display only active indicators"));

					//decrypts(1)
					static std::vector<std::tuple<const char*, bool*, const char*>> indicators_ui =
					{
						{ XorStr("Desync Amount"),			&var.visuals.b_indicator_desync,		nullptr },
						{ XorStr("Lag Compensation"),		&var.visuals.b_indicator_lc,			nullptr },
						{ XorStr("Aimbot Priority"),		&var.visuals.b_indicator_priority,		nullptr },
						{ XorStr("Edge/Freestanding"),		&var.visuals.b_indicator_edge,			nullptr },
						{ XorStr("Fakeduck"),				&var.visuals.b_indicator_fakeduck,		nullptr },
						{ XorStr("Minwalk"),				&var.visuals.b_indicator_minwalk,		nullptr },
						{ XorStr("Choke Amount"),			&var.visuals.b_indicator_choke,			nullptr },
						{ XorStr("Tickbase Shift"),			&var.visuals.b_indicator_tickbase,		nullptr },
						{ XorStr("Multi Tap"),				&var.visuals.b_indicator_double_tap,	nullptr },
						{ XorStr("Teleport"),				&var.visuals.b_indicator_teleport,		nullptr },
						{ XorStr("Lean Direction"),			&var.visuals.b_indicator_lean_dir,		nullptr },
#if defined _DEBUG || defined INTERNAL_DEBUG || defined TEST_BUILD
						{ XorStr("Stats"),					&var.visuals.b_indicator_stats,			nullptr },
#endif
					};
					ImGuiEx::SelectableCombo(XorStr("##VISUALS_INDICATORS"), indicators_ui, 32);
					ImGuiEx::SetTip(XorStr("Choose what indicators you'd like to display"));
					//encrypts(1)

					if (!var.visuals.b_indicators)
					{
						ImGui::PopItemFlag();
						ImGui::PopStyleVar();
					}

					ImGui::Separator();
					ImGui::Spacing();

					ImGui::PushFont(ImFontEx::header);
					ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Anti-Aim"));
					ImGui::PopFont();
					ImGui::Separator();
					ImGui::Spacing();

					ImGuiEx::Checkbox(XorStr("Verbose Resolver"), &var.visuals.b_verbose_resolver);
					ImGuiEx::Checkbox(XorStr("Draw Angles"), &var.visuals.b_show_antiaim_angles);

					if (!var.visuals.b_show_antiaim_angles)
					{
						ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
						ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
					}

					//decrypts(1)
					static std::vector<std::tuple<const char*, bool*, const char*>> draw_angles_ui =
					{
						{ XorStr("Real"),			&var.visuals.b_show_antiaim_real,			nullptr },
						{ XorStr("Fake"),			&var.visuals.b_show_antiaim_fake,			nullptr },
						{ XorStr("LBY"),			&var.visuals.b_show_antiaim_lby,			nullptr },
					};
					ImGuiEx::SelectableCombo(XorStr("##VISUALS_DRAW_ANGLES"), draw_angles_ui, 32);
					ImGuiEx::SetTip(XorStr("Choose what angle lines you'd like to display on yourself"));
					//encrypts(1)

					if (!var.visuals.b_show_antiaim_angles)
					{
						ImGui::PopItemFlag();
						ImGui::PopStyleVar();
					}

					ImGui::Separator();
					ImGui::Spacing();

					ImGui::PopItemWidth();
				}
				ImGui::EndChild();
				//encrypts(0)
			}
			ImGui::NextColumn();
			{
				//decrypts(0)
				ImGui::BeginChild(XorStr("##VISUALS_COLUMN2"), ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize);
				{
					ImGui::PushItemWidth(-1);
					auto f_win_width = ImGui::GetWindowWidth();

					ImGui::PushFont(ImFontEx::header);
					ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Sound"));
					ImGui::PopFont();
					ImGui::Separator();
					ImGui::Spacing();

					ImGuiEx::Checkbox(XorStr("Hit Sound"), &var.visuals.b_hit_sound);
					//ImGuiEx::Checkbox(XorStr("Hitmarker"), &var.visuals.b_hitmarker);

					ImGui::Spacing();
					ImGui::Spacing();

					//ImGui::AlignFirstTextHeightToWidgets();
					ImGui::AlignTextToFramePadding();
					ImGui::Text(XorStr("Visualize Sound:"));
					ImGui::SameLine(f_win_width / 2);
					ImGui::Combo(XorStr("##VISUALS_MISC_VIZSOUND"), &var.visuals.i_visualize_sound, XorStrCT(" Off\0 On\0 Only Footsteps\0"));

					ImGuiEx::Checkbox(XorStr("Enemies"), &var.visuals.b_visualize_sound_enemy);
					ImGuiEx::ColorPicker(XorStr("##VISUALS_MISC_VIZSOUND_COL1"), &var.visuals.col_visualize_sound_enemy, f_win_width - 27.f);

					ImGuiEx::Checkbox(XorStr("Teammates"), &var.visuals.b_visualize_sound_teammate);
					ImGuiEx::ColorPicker(XorStr("##VISUALS_MISC_VIZSOUND_COL2"), &var.visuals.col_visualize_sound_teammate, f_win_width - 27.f);

					ImGui::Spacing();
					ImGui::Spacing();
					ImGui::PushFont(ImFontEx::header);
					ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Crosshair"));
					ImGui::PopFont();
					ImGui::Separator();
					ImGui::Spacing();

					ImGuiEx::Checkbox(XorStr("Draw Sniper Lines"), &var.visuals.b_scope_lines);
					ImGuiEx::SetTip(XorStr("Draw custom lines when scoped"));
					ImGuiEx::ColorPicker(XorStr("Scope Lines"), &var.visuals.col_scope_lines, f_win_width - 27.f);

					//ImGui::AlignFirstTextHeightToWidgets();
					ImGui::AlignTextToFramePadding();
					ImGui::Text(XorStr("Autowall:"));
					ImGui::SameLine(f_win_width / 2);
					ImGui::Combo(XorStr("##VISUALS_MISC_AWXHAIR"), &var.visuals.i_autowall_xhair, XorStrCT(" Off\0 On\0 On With Damage\0"));

					ImGuiEx::Checkbox(XorStr("Spread Radius"), &var.visuals.b_spread_circle);
					ImGuiEx::ColorPicker(XorStr("##VISUALS_MISC_SPREAD"), &var.visuals.col_spread_circle, f_win_width - 27.f);

					ImGui::PopItemWidth();
				}
				ImGui::EndChild();
				//encrypts(0)
			}
			ImGui::NextColumn();
			{
				//decrypts(0)
				ImGui::BeginChild(XorStr("##VISUALS_COLUMN3"), ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize);
				{
					ImGui::PushItemWidth(-1);
					auto f_win_width = ImGui::GetWindowWidth();

					ImGui::PushFont(ImFontEx::header);
					ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Event Logger"));
					ImGui::PopFont();
					ImGui::Separator();
					ImGui::Spacing();

					//ImGui::AlignFirstTextHeightToWidgets();
					ImGui::AlignTextToFramePadding();
					ImGui::Text(XorStr("Buy Logs:"));
					ImGui::SameLine(f_win_width / 2);
					ImGui::Combo(XorStr("##VISUALS_MISC_LOGS_BUY"), &var.visuals.i_buy_logs, XorStrCT(" None\0 Console Only\0 Chat Only\0 Both\0"));

					//ImGui::AlignFirstTextHeightToWidgets();
					ImGui::AlignTextToFramePadding();
					ImGui::Text(XorStr("Damage Logs:"));
					ImGui::SameLine(f_win_width / 2);
					ImGui::Combo(XorStr("##VISUALS_MISC_LOGS_SHOT"), &var.visuals.i_shot_logs, XorStrCT(" None\0 Console Only\0 Chat Only\0 Both\0"));

					//ImGui::AlignFirstTextHeightToWidgets();
					ImGui::AlignTextToFramePadding();
					ImGui::Text(XorStr("Hurt Logs:"));
					ImGui::SameLine(f_win_width / 2);
					ImGui::Combo(XorStr("##VISUALS_MISC_LOGS_HURT"), &var.visuals.i_hurt_logs, XorStrCT(" None\0 Console Only\0 Chat Only\0 Both\0"));

					ImGui::Spacing();
					ImGui::Spacing();
					ImGui::PushFont(ImFontEx::header);
					ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Camera"));
					ImGui::PopFont();
					ImGui::Separator();

					//ImGuiEx::Checkbox(XorStr("Spectate All"), &var.misc.b_spectate_all);

					ImGuiEx::Checkbox(XorStr("Thirdperson"), &var.misc.thirdperson.b_enabled.b_state, true, &var.misc.thirdperson.b_enabled);
					ImGuiEx::Checkbox(XorStr("Disable on Nades"), &var.misc.thirdperson.b_disable_on_nade);
					ImGui::SliderFloat(XorStr("##THIRDPERSONDISTANCE"), &var.misc.thirdperson.f_distance, 25.f, 200.f, XorStr("Distance: %.1f0"));

					ImGui::Spacing();
					ImGui::Spacing();
					ImGui::PushFont(ImFontEx::header);
					ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("FOV"));
					ImGui::PopFont();
					ImGui::Separator();
					ImGui::Spacing();

					//ImGui::AlignFirstTextHeightToWidgets();
					ImGui::AlignTextToFramePadding();
					ImGui::Text(XorStr("ThirdPerson FOV:"));
					ImGui::SameLine(f_win_width / 2);
					ImGui::SliderFloat(XorStr("##FOV_TP"), &var.visuals.f_thirdperson_fov, 10.f, 180.f, XorStr("FOV: %0.1f0"));
					ImGuiEx::SetTip(XorStr("Set the limit of field-of-view the third person camera has."));

					//ImGui::AlignFirstTextHeightToWidgets();
					ImGui::AlignTextToFramePadding();
					ImGui::Text(XorStr("FirstPerson FOV:"));
					ImGui::SameLine(f_win_width / 2);
					ImGui::SliderFloat(XorStr("##FOV_FP"), &var.visuals.f_fov, 10.f, 180.f, XorStr("FOV: %0.1f0"));
					ImGuiEx::SetTip(XorStr("Set the limit of field-of-view the third person camera has."));

					ImGui::PopItemWidth();
				}
				ImGui::EndChild();
				//encrypts(0)
			}
			ImGui::NextColumn();
			{
				//decrypts(0)
				ImGui::BeginChild(XorStr("##VISUALS_COLUMN4"), ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize);
				{
					ImGui::PushItemWidth(-1);
					auto f_win_width = ImGui::GetWindowWidth();

					ImGui::PushFont(ImFontEx::header);
					ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("World"));
					ImGui::PopFont();
					ImGui::Separator();
					ImGui::Spacing();

					ImGuiEx::Checkbox(XorStr("World Modulation"), &var.visuals.asuswall.b_enabled);
					ImGuiEx::SetTip(XorStr("Use custom colors and alphas to apply to the world and props"));

					if (!var.visuals.asuswall.b_enabled)
					{
						ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
						ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
					}

					//decrypts(1)
					ImGui::AlignTextToFramePadding();
					ImGui::Text(XorStr("World Color:"));
					ImGuiEx::ColorPicker(XorStr("World Color##WORLD_MOD"), &var.visuals.asuswall.col_world, f_win_width - 27.f);

					ImGui::AlignTextToFramePadding();
					ImGui::Text(XorStr("Static Props Color:"));
					ImGuiEx::ColorPicker(XorStr("Prop Color##PROP_MOD"), &var.visuals.asuswall.col_prop, f_win_width - 27.f);
					//encrypts(1)

					if (!var.visuals.asuswall.b_enabled)
					{
						ImGui::PopItemFlag();
						ImGui::PopStyleVar();
					}

					ImGui::Separator();
					ImGui::Spacing();

					ImGui::PushFont(ImFontEx::header);
					ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Sky"));
					ImGui::PopFont();
					ImGui::Separator();
					ImGui::Spacing();

					ImGuiEx::Checkbox(XorStr("Sky Changer"), &var.visuals.skychanger.b_enabled);

					if (!var.visuals.skychanger.b_enabled)
					{
						ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
						ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
					}
					//decrypts(1)
					ImGui::AlignTextToFramePadding();
					ImGui::Text(XorStr("Sky Color:"));
					ImGuiEx::ColorPicker(XorStr("Sky Color##PROP_MOD"), &var.visuals.asuswall.col_sky, f_win_width - 27.f);

					ImGui::AlignTextToFramePadding();
					ImGui::Text(XorStr("Skybox:"));
					ImGui::SameLine(f_win_width / 2);
					ImGui::Combo(XorStr("##SKY"), &var.visuals.skychanger.i_type, XorStrCT(" cs_baggage_skybox_\0 cs_tibet\0 embassy\0 italy\0 jungle\0 nukeblank\0 office\0 sky_cs15_daylight01_hdr\0 sky_cs15_daylight02_hdr\0 sky_cs15_daylight03_hdr\0 sky_cs15_daylight04_hdr\0 sky_csgo_cloudy01\0 sky_csgo_night02\0 sky_csgo_night02b\0 sky_dust\0 sky_venice\0 vertigo\0 vietnam\0\0"));
					//encrypts(1)

					if (!var.visuals.skychanger.b_enabled)
					{
						ImGui::PopItemFlag();
						ImGui::PopStyleVar();
					}

					ImGui::PushFont(ImFontEx::header);
					ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Interface"));
					ImGui::PopFont();
					ImGui::Separator();
					ImGui::Spacing();

					ImGuiEx::Checkbox(XorStr("Spectator List"), &var.visuals.spectator_list.b_enabled.b_state, true, &var.visuals.spectator_list.b_enabled);

					ImGui::PopItemWidth();
				}
				//encrypts(0)
				ImGui::EndChild();
			}
			ImGui::Columns(1);
		}
		else
		{
			ImGui::Dummy(ImVec2(-10, 0));
			ImGui::SameLine();
			//decrypts(0)
			ImGuiEx::Checkbox(XorStr("Enable##VISUALS"), &var.visuals.b_enabled, true);
			//encrypts(0)

			if (!var.visuals.b_enabled)
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
			}
			ImGui::Separator();

			static auto active_filter = 0;

			//decrypts(0)
			std::vector<const char*> filter_groups =
			{
				XorStr("YOU"),
				XorStr("ENEMIES"),
				XorStr("TEAMMATES"),
				XorStr("PROJECTILES"),
				XorStr("WEAPONS"),
				XorStr("MISC##2")
			};
			static auto f_filter_group_max_size = (ImGui::GetWindowWidth() - 35.f) / filter_groups.size();

			ImGui::Dummy(ImVec2(-10, 0));
			ImGui::SameLine();
			ImGuiEx::RenderTabs(filter_groups, active_filter, f_filter_group_max_size, 15, true);
			ImGui::Separator();
			//encrypts(0)

			//decrypts(0)
			ImGui::BeginChild(XorStr("##VISUALS_FILTER"), ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize);
			{
				ImGui::Columns(3, nullptr, false);
				{
					//decrypts(1)
					ImGui::BeginChild(XorStr("##VISUALS_COLUMN1"), ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize);
					{
						ImGui::PushItemWidth(-1);
						auto f_win_width = ImGui::GetWindowWidth();

						if (active_filter == 1) // ENEMIES
						{
							ImGui::Dummy(ImVec2(-4, 0));
							ImGui::SameLine(); // this could be hella easier if we had a group for everything...
							//decrypts(2)
							ImGuiEx::Checkbox(XorStr("Enable##GENERAL"), &var.visuals.pf_enemy.vf_main.b_enabled);
							//encrypts(2)

							if (!var.visuals.pf_enemy.vf_main.b_enabled)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}

							ImGui::Separator();

							//decrypts(2)
							ImGuiEx::Checkbox(XorStr("Render Model##GENERAL"), &var.visuals.pf_enemy.vf_main.b_render);

							ImGuiEx::Checkbox(XorStr("Box##GENERAL"), &var.visuals.pf_enemy.vf_main.b_box);
							ImGuiEx::ColorPicker(XorStr("Main Color##GENERAL"), &var.visuals.pf_enemy.vf_main.col_main, f_win_width - 37.f);

							ImGuiEx::Checkbox(XorStr("Name##GENERAL"), &var.visuals.pf_enemy.vf_main.b_name);
							ImGuiEx::ColorPicker(XorStr("Name Color##GENERAL"), &var.visuals.pf_enemy.vf_main.col_name, f_win_width - 37.f);

							//ImGuiEx::Checkbox(XorStr("Ammo##GENERAL"), &var.visuals.pf_enemy.vf_main.b_info);
							//ImGuiEx::ColorPicker(XorStr("Ammo Color##GENERAL"), &var.visuals.pf_enemy.vf_main.col_ammo, f_win_width - 37.f);


							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Dormancy Fade Speed:"));
							ImGui::SameLine(f_win_width / 2);
							ImGui::SliderFloat(XorStr("##TIMOUTSCALE"), &var.visuals.pf_enemy.vf_main.f_timeout, 0.01f, 5.0f, XorStr("%0.2f"));

							ImGuiEx::Checkbox(XorStr("Offscreen##GENERAL"), &var.visuals.pf_enemy.b_offscreen_esp);
							if (!var.visuals.pf_enemy.b_offscreen_esp)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}

							ImGuiEx::ColorPicker(XorStr("Offscreen Color##GENERAL"), &var.visuals.pf_enemy.col_offscreen_esp, f_win_width - 37.f);
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Indicator Size:"));
							ImGui::SameLine(f_win_width / 2);
							ImGui::SliderFloat(XorStr("##ENEMY_OFFSCREEN_SIZE"), &var.visuals.pf_enemy.f_offscreen_esp, 4.f, 20.f, XorStr("%0.1f0"));
							//encrypts(2)

							if (!var.visuals.pf_enemy.b_offscreen_esp)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}

							//decrypts(2)
							ImGuiEx::Checkbox(XorStr("Far ESP##GENERAL"), &var.visuals.pf_enemy.b_faresp);
							//encrypts(2)

							if (!var.visuals.pf_enemy.b_faresp)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}

							ImGui::AlignTextToFramePadding();
							//decrypts(2)
							ImGui::Text(XorStr("Packets Per Tick:"));
							ImGui::SameLine(f_win_width / 2);
							ImGui::SliderInt(XorStr("##FARESP"), &var.visuals.pf_enemy.i_faresp, 1, 64, XorStr("%d"));
							//encrypts(2)

							if (!var.visuals.pf_enemy.b_faresp)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}

							//decrypts(2)
							static std::vector<std::tuple<const char*, bool*, const char*>> enemy_visual_flags_ui =
							{
								{ XorStr("Health"),				&var.visuals.pf_enemy.b_health,					XorStr("Display a health bar") },
								{ XorStr("Armor"),				&var.visuals.pf_enemy.b_armor,					XorStr("Display an armor bar") },
								{ XorStr("Weapon Name"),		&var.visuals.pf_enemy.b_weapon,					XorStr("Display the current equipped weapon") },
								{ XorStr("Money"),				&var.visuals.pf_enemy.b_money,					XorStr("Display the amount of money") },
								{ XorStr("Armor Status"),		&var.visuals.pf_enemy.b_kevlar_helm,			XorStr("Display H and K depending on if they have a helmet and/or kevlar") },
								{ XorStr("Bomb Carrier"),		&var.visuals.pf_enemy.b_bomb_carrier,			XorStr("Display BOMB if they are carrying the bomb") },
								{ XorStr("Bomb Interaction"),	&var.visuals.pf_enemy.b_bomb_interaction,		XorStr("Display PLANT/DEFUSE if they are currently interacting with the bomb") },
								{ XorStr("Has Kit"),			&var.visuals.pf_enemy.b_has_kit,				XorStr("Display KIT if they own a defuse/rescue kit") },
								{ XorStr("Scoped"),				&var.visuals.pf_enemy.b_scoped,					XorStr("Display SCOPE if they are currently scoping in") },
								{ XorStr("Pin Pull"),			&var.visuals.pf_enemy.b_pin_pull,				XorStr("Display PIN PULL if they are currently cooking a grenade") },
								{ XorStr("Blindness"),			&var.visuals.pf_enemy.b_blind,					XorStr("Display BLIND if they are currently blind") },
								{ XorStr("Burning"),			&var.visuals.pf_enemy.b_burn,					XorStr("Display BURN if they are currently taking burn damage") },
								{ XorStr("Reload"),				&var.visuals.pf_enemy.b_reload,					XorStr("Display RELOAD if they are currently reloading their weapon") },
								{ XorStr("Vulnerability"),		&var.visuals.pf_enemy.b_vuln,					XorStr("Display VULN if they are currently unable to shoot/harm you") },
								{ XorStr("Fakeduck"),			&var.visuals.pf_enemy.b_fakeduck,				XorStr("Display FD if they are currently are fakeducking") },
								{ XorStr("Aimbot Behavior"),	&var.visuals.pf_enemy.b_aimbot_debug,			XorStr("Display information related to the ragebot on this entity") },
								{ XorStr("Scanned Points"),		&var.visuals.b_aimbot_points,					XorStr("Display points that are currently being scanned by the ragebot") },

#if defined _DEBUG || defined INTERNAL_DEBUG
									{ XorStr("Smooth Animation"),	&var.visuals.pf_enemy.b_smooth_animation,		XorStr("Smooth out animated fakelag (experimental)") },
									{ XorStr("Entity Debug"),		&var.visuals.pf_enemy.b_entity_debug,			XorStr("Display their origin and entity index above their head") },
#endif
							};
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Information:"));
							ImGui::SameLine(f_win_width / 2);
							ImGuiEx::SelectableCombo(XorStr("##VISUALS_ENEMY_FLAGS"), enemy_visual_flags_ui);
							ImGuiEx::SetTip(XorStr("Choose what type of information you'd like to know about your enemies"));

							ImGuiEx::Checkbox(XorStr("Resolver Information##GENERAL"), &var.visuals.pf_enemy.b_resolver);
							ImGuiEx::SetTip(XorStr("Show R in different colors to showcase what mode/assurance the resolver currently has on that entity"));
							//encrypts(2)

							if (!var.visuals.pf_enemy.b_resolver)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}

							//decrypts(2)
							ImGuiEx::Checkbox(XorStr("Verbose##GENERAL"), &var.visuals.b_verbose_resolver);
							ImGuiEx::SetTip(XorStr("Show all related information about the resolver on this entity including current amount of misses and more"));
							//encrypts(2)

							if (!var.visuals.pf_enemy.b_resolver)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}

							if (!var.visuals.pf_enemy.vf_main.b_enabled)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}
						}
						else if (active_filter == 2) // TEAMMATES
						{
							//decrypts(2)
							ImGui::Dummy(ImVec2(-4, 0));
							ImGui::SameLine(); // this could be hella easier if we had a group for everything...
							ImGuiEx::Checkbox(XorStr("Enable##GENERAL"), &var.visuals.pf_teammate.vf_main.b_enabled);
							//encrypts(2)

							if (!var.visuals.pf_teammate.vf_main.b_enabled)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}

							ImGui::Separator();

							//decrypts(2)
							ImGuiEx::Checkbox(XorStr("Render Model##GENERAL"), &var.visuals.pf_teammate.vf_main.b_render);

							ImGuiEx::Checkbox(XorStr("Box##GENERAL"), &var.visuals.pf_teammate.vf_main.b_box);
							ImGuiEx::ColorPicker(XorStr("Main Color##GENERAL"), &var.visuals.pf_teammate.vf_main.col_main, f_win_width - 37.f);

							ImGuiEx::Checkbox(XorStr("Name##GENERAL"), &var.visuals.pf_teammate.vf_main.b_name);
							ImGuiEx::ColorPicker(XorStr("Name Color##GENERAL"), &var.visuals.pf_teammate.vf_main.col_name, f_win_width - 37.f);

							//ImGuiEx::Checkbox(XorStr("Ammo##GENERAL"), &var.visuals.pf_teammate.vf_main.b_info);
							//ImGuiEx::ColorPicker(XorStr("Ammo Color##GENERAL"), &var.visuals.pf_teammate.vf_main.col_ammo, f_win_width - 37.f);

							ImGuiEx::Checkbox(XorStr("Offscreen##GENERAL"), &var.visuals.pf_teammate.b_offscreen_esp);
							if (!var.visuals.pf_teammate.b_offscreen_esp)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}

							ImGuiEx::ColorPicker(XorStr("Offscreen Color##GENERAL"), &var.visuals.pf_teammate.col_offscreen_esp, f_win_width - 37.f);
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Indicator Size:"));
							ImGui::SameLine(f_win_width / 2);
							ImGui::SliderFloat(XorStr("##ENEMY_OFFSCREEN_SIZE"), &var.visuals.pf_teammate.f_offscreen_esp, 4.f, 20.f, XorStr("%0.1f0"));
							//encrypts(2)

							if (!var.visuals.pf_teammate.b_offscreen_esp)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}

							//decrypts(2)
							static std::vector<std::tuple<const char*, bool*, const char*>> team_visual_flags_ui =
							{
								{ XorStr("Health"),				&var.visuals.pf_teammate.b_health,					XorStr("Display a health bar") },
								{ XorStr("Armor"),				&var.visuals.pf_teammate.b_armor,					XorStr("Display an armor bar") },
								{ XorStr("Weapon Name"),		&var.visuals.pf_teammate.b_weapon,					XorStr("Display the current equipped weapon") },
								{ XorStr("Money"),				&var.visuals.pf_teammate.b_money,					XorStr("Display the amount of money") },
								{ XorStr("Armor Status"),		&var.visuals.pf_teammate.b_kevlar_helm,			XorStr("Display H and K depending on if they have a helmet and/or kevlar") },
								{ XorStr("Bomb Carrier"),		&var.visuals.pf_teammate.b_bomb_carrier,			XorStr("Display BOMB if they are carrying the bomb") },
								{ XorStr("Bomb Interaction"),	&var.visuals.pf_teammate.b_bomb_interaction,		XorStr("Display PLANT/DEFUSE if they are currently interacting with the bomb") },
								{ XorStr("Has Kit"),			&var.visuals.pf_teammate.b_has_kit,				XorStr("Display KIT if they own a defuse/rescue kit") },
								{ XorStr("Scoped"),				&var.visuals.pf_teammate.b_scoped,					XorStr("Display SCOPE if they are currently scoping in") },
								{ XorStr("Pin Pull"),			&var.visuals.pf_teammate.b_pin_pull,				XorStr("Display PIN PULL if they are currently cooking a grenade") },
								{ XorStr("Blindness"),			&var.visuals.pf_teammate.b_blind,					XorStr("Display BLIND if they are currently blind") },
								{ XorStr("Burning"),			&var.visuals.pf_teammate.b_burn,					XorStr("Display BURN if they are currently taking burn damage") },
								{ XorStr("Reload"),				&var.visuals.pf_teammate.b_reload,					XorStr("Display RELOAD if they are currently reloading their weapon") },
								{ XorStr("Vulnerability"),		&var.visuals.pf_teammate.b_vuln,					XorStr("Display VULN if they are currently unable to shoot/harm you") },
								{ XorStr("Fakeduck"),			&var.visuals.pf_teammate.b_fakeduck,				XorStr("Display FD if they are currently are fakeducking") },
#if defined _DEBUG || defined INTERNAL_DEBUG
									{ XorStr("Resolver"),			&var.visuals.pf_teammate.b_resolver,				nullptr },
									{ XorStr("Smooth Animation"),	&var.visuals.pf_teammate.b_smooth_animation,		XorStr("Smooth out animated fakelag (experimental)") },
									{ XorStr("Entity Debug"),		&var.visuals.pf_teammate.b_entity_debug,			XorStr("Display their origin and entity index above their head") },
#endif
							};
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Information:"));
							ImGui::SameLine(f_win_width / 2);
							ImGuiEx::SelectableCombo(XorStr("##VISUALS_TEAM_FLAGS"), team_visual_flags_ui);
							ImGuiEx::SetTip(XorStr("Choose what type of information you'd like to know about your teammates"));
							//encrypts(2)

							if (!var.visuals.pf_teammate.vf_main.b_enabled)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}
						}
						else if (active_filter == 3) // PROJECTILES
						{
							//decrypts(2)
							ImGui::Dummy(ImVec2(-4, 0));
							ImGui::SameLine();
							ImGuiEx::Checkbox(XorStr("Nade Prediction##PROJ_PRED"), &var.visuals.nade_prediction.b_enabled);
							//encrypts(2)

							if (!var.visuals.nade_prediction.b_enabled)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}

							//decrypts(2)
							ImGui::Separator();
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Hidden Color:"));
							ImGuiEx::ColorPicker(XorStr("##PROJ_PRED_PATH_MAIN"), &var.visuals.nade_prediction.col, f_win_width - 27.f);

							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Visible Color:"));
							ImGuiEx::ColorPicker(XorStr("##PROJ_PRED_PATH_VIS"), &var.visuals.nade_prediction.col_visible, f_win_width - 27.f);

							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Hit Color:"));
							ImGuiEx::ColorPicker(XorStr("##PROJ_PRED_PATH_HIT"), &var.visuals.nade_prediction.col_hit, f_win_width - 27.f);
							ImGui::Spacing();
							//encrypts(2)

							if (!var.visuals.nade_prediction.b_enabled)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}

							ImGui::Spacing();
							ImGui::Separator();

							ImGui::Dummy(ImVec2(-4, 0));
							ImGui::SameLine();
							//decrypts(2)
							ImGuiEx::Checkbox(XorStr("Enable##GENERAL"), &var.visuals.vf_projectile.b_enabled);
							//encrypts(2)

							if (!var.visuals.vf_projectile.b_enabled)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}

							ImGui::Separator();

							//decrypts(2)
							ImGuiEx::Checkbox(XorStr("Render Model##PROJ_GENERAL"), &var.visuals.vf_projectile.b_render);

							ImGuiEx::Checkbox(XorStr("Box##GENERAL"), &var.visuals.vf_projectile.b_box);
							ImGuiEx::ColorPicker(XorStr("Main Color##GENERAL"), &var.visuals.vf_projectile.col_main, f_win_width - 27.f);

							ImGuiEx::Checkbox(XorStr("Name##GENERAL"), &var.visuals.vf_projectile.b_name);
							ImGuiEx::ColorPicker(XorStr("Name Color##GENERAL"), &var.visuals.vf_projectile.col_name, f_win_width - 27.f);

							ImGuiEx::Checkbox(XorStr("Owner##GENERAL"), &var.visuals.b_projectile_owner);
							//encrypts(2)

							if (!var.visuals.vf_projectile.b_enabled)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}
						}
						else if (active_filter == 4) // WEAPONS
						{
							//decrypts(2)
							ImGui::Dummy(ImVec2(-4, 0));
							ImGui::SameLine();
							ImGuiEx::Checkbox(XorStr("Enable##GENERAL"), &var.visuals.vf_weapon.b_enabled);
							ImGui::Spacing();
							//encrypts(2)

							if (!var.visuals.vf_weapon.b_enabled)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}
							ImGui::Separator();
							ImGui::Spacing();

							//decrypts(2)
							ImGuiEx::Checkbox(XorStr("Render Model##PROJ_GENERAL"), &var.visuals.vf_weapon.b_render);

							ImGuiEx::Checkbox(XorStr("Box##GENERAL"), &var.visuals.vf_weapon.b_box);
							ImGuiEx::ColorPicker(XorStr("Main Color##GENERAL"), &var.visuals.vf_weapon.col_main, f_win_width - 27.f);

							ImGuiEx::Checkbox(XorStr("Name##GENERAL"), &var.visuals.vf_weapon.b_name);
							ImGuiEx::ColorPicker(XorStr("Name Color##GENERAL"), &var.visuals.vf_weapon.col_name, f_win_width - 27.f);

							ImGuiEx::Checkbox(XorStr("Ammo##GENERAL"), &var.visuals.vf_weapon.b_info);
							//encrypts(2)

							if (!var.visuals.vf_weapon.b_enabled)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}
						}
						else if (active_filter == 5) // MISC // BOMB
						{
							//decrypts(2)
							ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Bomb:"));
							ImGui::Dummy(ImVec2(-4, 0));
							ImGui::SameLine(); // this could be hella easier if we had a group for everything...
							ImGuiEx::Checkbox(XorStr("Enable##BOMB_GENERAL"), &var.visuals.vf_bomb.b_enabled);
							ImGui::Spacing();
							//encrypts(2)

							if (!var.visuals.vf_bomb.b_enabled)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}

							ImGui::Separator();
							ImGui::Spacing();

							//decrypts(2)
							ImGuiEx::Checkbox(XorStr("Render Model##GENERAL"), &var.visuals.vf_bomb.b_render);

							ImGuiEx::Checkbox(XorStr("Box##BOMB"), &var.visuals.vf_bomb.b_box);
							ImGuiEx::ColorPicker(XorStr("Main Color##BOMB_MAIN"), &var.visuals.vf_bomb.col_main, f_win_width - 37.f);

							ImGuiEx::Checkbox(XorStr("Name##BOMB"), &var.visuals.vf_bomb.b_name);
							ImGuiEx::ColorPicker(XorStr("Name Color##BOMB_NAME"), &var.visuals.vf_bomb.col_name, f_win_width - 37.f);

							ImGuiEx::Checkbox(XorStr("Bomb Timer"), &var.visuals.b_c4timer);

							ImGui::Spacing();
							ImGui::Separator();

							ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Glow:"));
							ImGui::Dummy(ImVec2(-4, 0));
							ImGui::SameLine(); // this could be hella easier if we had a group for everything...
							ImGuiEx::Checkbox(XorStr("Enable##BOMB_GLOW_GENERAL"), &var.visuals.vf_bomb.glow.b_enabled);
							//encrypts(2)

							if (!var.visuals.vf_bomb.glow.b_enabled)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}

							//decrypts(2)
							ImGuiEx::ColorPicker(XorStr("Color##GLOW_BOMB"), &var.visuals.vf_bomb.glow.col_color, f_win_width - 37.f);
							ImGui::Spacing();

							ImGuiEx::Checkbox(XorStr("FullBloom##GLOW_BOMB"), &var.visuals.vf_bomb.glow.b_fullbloom);

							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Style:"));
							ImGui::SameLine(f_win_width / 2);
							ImGui::Combo(XorStr("##GLOW_STYLE_BOMB"), &var.visuals.vf_bomb.glow.i_type, XorStrCT(" Outlined\0 Model Based\0 Inward\0 Inward Pulsing\0"));

							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Blend:"));
							ImGui::SameLine(f_win_width / 2);
							ImGui::SliderFloat(XorStr("##GLOW_BLEND_BOMB"), &var.visuals.vf_bomb.glow.f_blend, 0.f, 100.f, XorStr("BLEND: %0.1f0%%"));
							//encrypts(2)

							if (!var.visuals.vf_bomb.glow.b_enabled)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}

							ImGui::Spacing();
							ImGui::Separator();

							//decrypts(2)
							ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Chams:"));
							ImGui::Dummy(ImVec2(-4, 0));
							ImGui::SameLine();
							ImGuiEx::Checkbox(XorStr("Enable##BOMB_CHAMS"), &var.visuals.vf_bomb.chams.b_enabled);
							//encrypts(2)

							if (!var.visuals.vf_bomb.chams.b_enabled)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}

							//decrypts(2)
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Material:"));
							ImGui::SameLine(f_win_width / 2);
							ImGui::PushItemWidth((f_win_width / 2) - 27.f);
							ImGui::Combo(XorStr("##CHAMS_VIS_MAT_GENERAL"), &var.visuals.vf_bomb.chams.i_mat_visible, XorStrCT(" Flat\0 Shaded\0 Pulse\0 Metallic\0 Glass\0"));
							ImGui::PopItemWidth();
							ImGuiEx::ColorPicker(XorStr("Color##COL_CHAMS_VIS_MAT_GENERAL"), &var.visuals.vf_bomb.chams.col_visible, f_win_width - 37.f);

							ImGuiEx::Checkbox(XorStr("XQZ##CHAMS_GENERAL"), &var.visuals.vf_bomb.chams.b_xqz);
							ImGui::SameLine(f_win_width / 2);
							ImGui::PushItemWidth((f_win_width / 2) - 27.f);
							ImGui::Combo(XorStr("##CHAMS_INVIS_MAT_GENERAL"), &var.visuals.vf_bomb.chams.i_mat_invisible, XorStrCT(" Flat\0 Shaded\0 Pulse\0 Metallic\0 Glass\0"));
							ImGui::PopItemWidth();
							ImGuiEx::ColorPicker(XorStr("Color##COL_CHAMS_INVIS_MAT_GENERAL"), &var.visuals.vf_bomb.chams.col_invisible, f_win_width - 37.f);
							//encrypts(2)

							if (!var.visuals.vf_bomb.chams.b_enabled)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}

							if (!var.visuals.vf_bomb.b_enabled)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}
						}
						else
						{
							//decrypts(2)
							ImGui::Dummy(ImVec2(-4, 0));
							ImGui::SameLine(); // this could be hella easier if we had a group for everything...
							ImGuiEx::Checkbox(XorStr("Enable##GENERAL"), &var.visuals.pf_local_player.vf_main.b_enabled);
							//encrypts(2)

							if (!var.visuals.pf_local_player.vf_main.b_enabled)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}

							ImGui::Separator();

							//decrypts(2)
							ImGuiEx::Checkbox(XorStr("Render Model##GENERAL"), &var.visuals.pf_local_player.vf_main.b_render);

							ImGuiEx::Checkbox(XorStr("Box##GENERAL"), &var.visuals.pf_local_player.vf_main.b_box);
							ImGuiEx::ColorPicker(XorStr("Main Color##GENERAL"), &var.visuals.pf_local_player.vf_main.col_main, f_win_width - 27.f);

							ImGuiEx::Checkbox(XorStr("Name##GENERAL"), &var.visuals.pf_local_player.vf_main.b_name);
							ImGuiEx::ColorPicker(XorStr("Name Color##GENERAL"), &var.visuals.pf_local_player.vf_main.col_name, f_win_width - 27.f);

							static std::vector<std::tuple<const char*, bool*, const char*>> you_visual_flags_ui =
							{
								{ XorStr("Health"),				&var.visuals.pf_local_player.b_health,					XorStr("Display your health bar") },
								{ XorStr("Armor"),				&var.visuals.pf_local_player.b_armor,					XorStr("Display your armor bar") },
								{ XorStr("Weapon Name"),		&var.visuals.pf_local_player.b_weapon,					XorStr("Display your current equipped weapon") },
								{ XorStr("Weapon Ammo"),		&var.visuals.pf_local_player.vf_main.b_info,			XorStr("Display your current equipped weapon's ammo") },
								{ XorStr("Money"),				&var.visuals.pf_local_player.b_money,					XorStr("Display your amount of money") },
								{ XorStr("Armor Status"),		&var.visuals.pf_local_player.b_kevlar_helm,				XorStr("Display H and K depending on if you have a helmet and/or kevlar") },
								{ XorStr("Bomb Carrier"),		&var.visuals.pf_local_player.b_bomb_carrier,			XorStr("Display BOMB if you are carrying the bomb") },
								{ XorStr("Bomb Interaction"),	&var.visuals.pf_local_player.b_bomb_interaction,		XorStr("Display PLANT/DEFUSE if you are currently interacting with the bomb") },
								{ XorStr("Has Kit"),			&var.visuals.pf_local_player.b_has_kit,					XorStr("Display KIT if you own a defuse/rescue kit") },
								{ XorStr("Scoped"),				&var.visuals.pf_local_player.b_scoped,					XorStr("Display SCOPE if you are currently scoping in") },
								{ XorStr("Pin Pull"),			&var.visuals.pf_local_player.b_pin_pull,				XorStr("Display PIN PULL if you are currently cooking a grenade") },
								{ XorStr("Blindness"),			&var.visuals.pf_local_player.b_blind,					XorStr("Display BLIND if you are currently blind") },
								{ XorStr("Burning"),			&var.visuals.pf_local_player.b_burn,					XorStr("Display BURN if you are currently taking burn damage") },
								{ XorStr("Reload"),				&var.visuals.pf_local_player.b_reload,					XorStr("Display RELOAD if you are currently reloading their weapon") },
								{ XorStr("Vulnerability"),		&var.visuals.pf_local_player.b_vuln,					XorStr("Display VULN if you are currently unable to shoot/harm you") },
								{ XorStr("Fakeduck"),			&var.visuals.pf_local_player.b_fakeduck,				XorStr("Display FD if you are currently fakeducking") },

#if defined _DEBUG || defined INTERNAL_DEBUG
									{ XorStr("Smooth Animation"),	&var.visuals.pf_local_player.b_smooth_animation, XorStr("Smooth out animated fakelag (experimental)") },
									{ XorStr("Entity Debug"),		&var.visuals.pf_local_player.b_entity_debug,			XorStr("Display their origin and entity index above their head") },
#endif
							};
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Information:"));
							ImGui::SameLine(f_win_width / 2);
							ImGuiEx::SelectableCombo(XorStr("##VISUALS_YOU_FLAGS"), you_visual_flags_ui);
							ImGuiEx::SetTip(XorStr("Choose what type of information you'd like to know about yourself"));
							//encrypts(2)

							if (!var.visuals.pf_local_player.vf_main.b_enabled)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}
						}
						ImGui::PopItemWidth();
					}
					//encrypts(1)
					ImGui::EndChild();
				}
				ImGui::NextColumn();
				{
					//decrypts(1)
					ImGui::BeginChild(XorStr("##VISUALS_COLUMN2"), ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize);
					{
						ImGui::PushItemWidth(-1);
						auto f_win_width = ImGui::GetWindowWidth();

						if (active_filter == 1) // ENEMIES
						{
							if (!var.visuals.pf_enemy.vf_main.b_enabled)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}
							ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Glow:"));
							ImGui::Dummy(ImVec2(-4, 0));
							ImGui::SameLine(); // this could be hella easier if we had a group for everything...
							ImGuiEx::Checkbox(XorStr("Enable##GENERAL"), &var.visuals.pf_enemy.vf_main.glow.b_enabled);

							if (!var.visuals.pf_enemy.vf_main.glow.b_enabled)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}
							//decrypts(2)
							ImGuiEx::ColorPicker(XorStr("Color##GLOW_GENERAL"), &var.visuals.pf_enemy.vf_main.glow.col_color, f_win_width - 27.f);
							ImGui::Spacing();

							ImGuiEx::Checkbox(XorStr("FullBloom##GLOW_GENERAL"), &var.visuals.pf_enemy.vf_main.glow.b_fullbloom);

							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Style:"));
							ImGui::SameLine(f_win_width / 2);
							ImGui::Combo(XorStr("##GLOW_STYLE_GENERAL"), &var.visuals.pf_enemy.vf_main.glow.i_type, XorStrCT(" Outlined\0 Model Based\0 Inward\0 Inward Pulsing\0"));

							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Blend:"));
							ImGui::SameLine(f_win_width / 2);
							ImGui::SliderFloat(XorStr("##LOW_BLEND_GENERAL"), &var.visuals.pf_enemy.vf_main.glow.f_blend, 0.f, 100.f, XorStr("BLEND: %0.1f0%%"));
							//encrypts(2)
							if (!var.visuals.pf_enemy.vf_main.glow.b_enabled)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}
							if (!var.visuals.pf_enemy.vf_main.b_enabled)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}
						}
						else if (active_filter == 2) // TEAMMATES
						{
							if (!var.visuals.pf_teammate.vf_main.b_enabled)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}
							ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Glow:"));
							ImGui::Dummy(ImVec2(-4, 0));
							ImGui::SameLine(); // this could be hella easier if we had a group for everything...
							ImGuiEx::Checkbox(XorStr("Enable##GENERAL"), &var.visuals.pf_teammate.vf_main.glow.b_enabled);

							if (!var.visuals.pf_teammate.vf_main.glow.b_enabled)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}

							//decrypts(2)
							ImGuiEx::ColorPicker(XorStr("Color##GLOW_GENERAL"), &var.visuals.pf_teammate.vf_main.glow.col_color, f_win_width - 27.f);
							ImGui::Spacing();

							ImGuiEx::Checkbox(XorStr("FullBloom##GLOW_GENERAL"), &var.visuals.pf_teammate.vf_main.glow.b_fullbloom);

							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Style:"));
							ImGui::SameLine(f_win_width / 2);
							ImGui::Combo(XorStr("##GLOW_STYLE_GENERAL"), &var.visuals.pf_teammate.vf_main.glow.i_type, XorStrCT(" Outlined\0 Model Based\0 Inward\0 Inward Pulsing\0"));

							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Blend:"));
							ImGui::SameLine(f_win_width / 2);
							ImGui::SliderFloat(XorStr("##LOW_BLEND_GENERAL"), &var.visuals.pf_teammate.vf_main.glow.f_blend, 0.f, 100.f, XorStr("BLEND: %0.1f0%%"));
							//encrypts(2)

							if (!var.visuals.pf_teammate.vf_main.glow.b_enabled)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}
							if (!var.visuals.pf_teammate.vf_main.b_enabled)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}
						}
						else if (active_filter == 3) // PROJECTILES
						{
							if (!var.visuals.vf_projectile.b_enabled)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}
							ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Glow:"));
							ImGui::Dummy(ImVec2(-4, 0));
							ImGui::SameLine(); // this could be hella easier if we had a group for everything...
							ImGuiEx::Checkbox(XorStr("Enable##GENERAL"), &var.visuals.vf_projectile.glow.b_enabled);

							if (!var.visuals.vf_projectile.glow.b_enabled)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}
							//decrypts(2)
							ImGuiEx::ColorPicker(XorStr("Color##GLOW_GENERAL"), &var.visuals.vf_projectile.glow.col_color, f_win_width - 27.f);
							ImGui::Spacing();

							ImGuiEx::Checkbox(XorStr("FullBloom##GLOW_GENERAL"), &var.visuals.vf_projectile.glow.b_fullbloom);

							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Style:"));
							ImGui::SameLine(f_win_width / 2);
							ImGui::Combo(XorStr("##GLOW_STYLE_GENERAL"), &var.visuals.vf_projectile.glow.i_type, XorStrCT(" Outlined\0 Model Based\0 Inward\0 Inward Pulsing\0"));

							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Blend:"));
							ImGui::SameLine(f_win_width / 2);
							ImGui::SliderFloat(XorStr("##LOW_BLEND_GENERAL"), &var.visuals.vf_projectile.glow.f_blend, 0.f, 100.f, XorStr("BLEND: %0.1f0%%"));
							//encrypts(2)

							if (!var.visuals.vf_projectile.glow.b_enabled)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}
							if (!var.visuals.vf_projectile.b_enabled)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}
						}
						else if (active_filter == 4) // WEAPONS
						{
							if (!var.visuals.vf_weapon.b_enabled)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}
							ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Glow:"));
							ImGui::Dummy(ImVec2(-4, 0));
							ImGui::SameLine(); // this could be hella easier if we had a group for everything...
							ImGuiEx::Checkbox(XorStr("Enable##GENERAL"), &var.visuals.vf_weapon.glow.b_enabled);

							if (!var.visuals.vf_weapon.glow.b_enabled)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}

							//decrypts(2)
							ImGuiEx::ColorPicker(XorStr("Color##GLOW_GENERAL"), &var.visuals.vf_weapon.glow.col_color, f_win_width - 27.f);
							ImGui::Spacing();

							ImGuiEx::Checkbox(XorStr("FullBloom##GLOW_GENERAL"), &var.visuals.vf_weapon.glow.b_fullbloom);

							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Style:"));
							ImGui::SameLine(f_win_width / 2);
							ImGui::Combo(XorStr("##GLOW_STYLE_GENERAL"), &var.visuals.vf_weapon.glow.i_type, XorStrCT(" Outlined\0 Model Based\0 Inward\0 Inward Pulsing\0"));

							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Blend:"));
							ImGui::SameLine(f_win_width / 2);
							ImGui::SliderFloat(XorStr("##LOW_BLEND_GENERAL"), &var.visuals.vf_weapon.glow.f_blend, 0.f, 100.f, XorStr("BLEND: %0.1f0%%"));
							//encrypts(2)

							if (!var.visuals.vf_weapon.glow.b_enabled)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}
							if (!var.visuals.vf_weapon.b_enabled)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}
						}
						else if (active_filter == 5) // MISC // VIEWMODEL
						{
							ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("View Model:"));
							ImGui::Dummy(ImVec2(-4, 0));
							ImGui::SameLine(); // this could be hella easier if we had a group for everything...
							ImGuiEx::Checkbox(XorStr("Enable##VM_GENERAL"), &var.visuals.vf_viewweapon.b_enabled);
							ImGui::Spacing();

							if (!var.visuals.vf_viewweapon.b_enabled)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}
							//decrypts(2)
							ImGui::Separator();
							ImGui::Spacing();

							ImGuiEx::Checkbox(XorStr("Render Model##VM_GENERAL"), &var.visuals.vf_viewweapon.b_render);
							//encrypts(2)

							ImGui::Spacing();
							ImGui::Separator();

							ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Chams:"));
							ImGui::Dummy(ImVec2(-4, 0));
							ImGui::SameLine();
							ImGuiEx::Checkbox(XorStr("Enable##VM_CHAMS"), &var.visuals.vf_viewweapon.chams.b_enabled);
							//encrypts(2)

							if (!var.visuals.vf_viewweapon.chams.b_enabled)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}
							//decrypts(2)
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Material:"));
							ImGui::SameLine(f_win_width / 2);
							ImGui::PushItemWidth((f_win_width / 2) - 27.f);
							ImGui::Combo(XorStr("##VM_CHAMS_VIS_MAT"), &var.visuals.vf_viewweapon.chams.i_mat_visible, XorStrCT(" Flat\0 Shaded\0 Pulse\0 Metallic\0 Glass\0"));
							ImGui::PopItemWidth();
							ImGuiEx::ColorPicker(XorStr("Color##VM_CHAMS_VIS_MAT"), &var.visuals.vf_viewweapon.chams.col_visible, f_win_width - 27.f);
							//encrypts(2)

							if (!var.visuals.vf_viewweapon.chams.b_enabled)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}
							if (!var.visuals.vf_viewweapon.b_enabled)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}
						}
						else
						{
							if (!var.visuals.pf_local_player.vf_main.b_enabled)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}
							ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Glow:"));
							ImGui::Dummy(ImVec2(-4, 0));
							ImGui::SameLine(); // this could be hella easier if we had a group for everything...
							ImGuiEx::Checkbox(XorStr("Enable##GENERAL"), &var.visuals.pf_local_player.vf_main.glow.b_enabled);

							if (!var.visuals.pf_local_player.vf_main.glow.b_enabled)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}
							//decrypts(2)
							ImGuiEx::ColorPicker(XorStr("Color##GENERAL"), &var.visuals.pf_local_player.vf_main.glow.col_color, f_win_width - 27.f);
							ImGui::Spacing();

							ImGuiEx::Checkbox(XorStr("FullBloom##GENERAL"), &var.visuals.pf_local_player.vf_main.glow.b_fullbloom);

							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Style:"));
							ImGui::SameLine(f_win_width / 2);
							ImGui::Combo(XorStr("##GLOW_LP_STYLE"), &var.visuals.pf_local_player.vf_main.glow.i_type, XorStrCT(" Outlined\0 Model Based\0 Inward\0 Inward Pulsing\0"));

							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Blend:"));
							ImGui::SameLine(f_win_width / 2);
							ImGui::SliderFloat(XorStr("##GLOW_LP_BLEND"), &var.visuals.pf_local_player.vf_main.glow.f_blend, 0.f, 100.f, XorStr("BLEND: %0.1f0%%"));
							//encrypts(2)

							if (!var.visuals.pf_local_player.vf_main.glow.b_enabled)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}

							if (!var.visuals.pf_local_player.vf_main.b_enabled)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}
						}
						ImGui::PopItemWidth();
					}
					ImGui::EndChild();
					//encrypts(1)
				}
				ImGui::NextColumn();
				{
					//decrypts(1)
					ImGui::BeginChild(XorStr("##VISUALS_COLUMN3"), ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize);
					{
						ImGui::PushItemWidth(-1);
						auto f_win_width = ImGui::GetWindowWidth();

						if (active_filter == 1) // ENEMIES
						{
							if (!var.visuals.pf_enemy.vf_main.b_enabled)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}

							ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Chams:"));
							ImGui::Dummy(ImVec2(-4, 0));
							ImGui::SameLine(); // this could be hella easier if we had a group for everything...
							ImGuiEx::Checkbox(XorStr("Enable##CHAMS_GENERAL"), &var.visuals.pf_enemy.vf_main.chams.b_enabled);

							if (!var.visuals.pf_enemy.vf_main.chams.b_enabled)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}

							//decrypts(2)
							ImGui::Spacing();
							ImGuiEx::Checkbox(XorStr("Backtrack##GENERAL"), &var.visuals.pf_enemy.b_backtrack);
							ImGuiEx::ColorPicker(XorStr("##CHAMS_BACKTRACK_GENERAL"), &var.visuals.pf_enemy.col_backtrack, f_win_width - 27.f);

							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Material:"));
							ImGui::SameLine(f_win_width / 2);
							ImGui::PushItemWidth((f_win_width / 2) - 27.f);
							ImGui::Combo(XorStr("##CHAMS_VIS_MAT_GENERAL"), &var.visuals.pf_enemy.vf_main.chams.i_mat_visible, XorStrCT(" Flat\0 Shaded\0 Pulse\0 Metallic\0 Glass\0"));
							ImGui::PopItemWidth();
							ImGuiEx::ColorPicker(XorStr("Color##COL_CHAMS_VIS_MAT_GENERAL"), &var.visuals.pf_enemy.vf_main.chams.col_visible, f_win_width - 27.f);

							ImGuiEx::Checkbox(XorStr("XQZ##CHAMS_GENERAL"), &var.visuals.pf_enemy.vf_main.chams.b_xqz);
							ImGui::SameLine(f_win_width / 2);
							ImGui::PushItemWidth((f_win_width / 2) - 27.f);
							ImGui::Combo(XorStr("##CHAMS_INVIS_MAT_GENERAL"), &var.visuals.pf_enemy.vf_main.chams.i_mat_invisible, XorStrCT(" Flat\0 Shaded\0 Pulse\0 Metallic\0 Glass\0"));
							ImGui::PopItemWidth();
							ImGuiEx::ColorPicker(XorStr("Color##COL_CHAMS_INVIS_MAT_GENERAL"), &var.visuals.pf_enemy.vf_main.chams.col_invisible, f_win_width - 27.f);
							//encrypts(2)

							if (!var.visuals.pf_enemy.vf_main.chams.b_enabled)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}

							if (!var.visuals.pf_enemy.vf_main.b_enabled)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}
						}
						else if (active_filter == 2) // TEAMMATES
						{
							if (!var.visuals.pf_teammate.vf_main.b_enabled)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}
							ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Chams:"));
							ImGui::Dummy(ImVec2(-4, 0));
							ImGui::SameLine(); // this could be hella easier if we had a group for everything...
							ImGuiEx::Checkbox(XorStr("Enable##CHAMS_GENERAL"), &var.visuals.pf_teammate.vf_main.chams.b_enabled);

							if (!var.visuals.pf_teammate.vf_main.chams.b_enabled)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}

							//decrypts(2)
							ImGui::Spacing();
							ImGuiEx::Checkbox(XorStr("Backtrack##GENERAL"), &var.visuals.pf_teammate.b_backtrack);
							ImGuiEx::ColorPicker(XorStr("##CHAMS_BACKTRACK_GENERAL"), &var.visuals.pf_teammate.col_backtrack, f_win_width - 27.f);

							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Material:"));
							ImGui::SameLine(f_win_width / 2);
							ImGui::PushItemWidth((f_win_width / 2) - 27.f);
							ImGui::Combo(XorStr("##CHAMS_VIS_MAT_GENERAL"), &var.visuals.pf_teammate.vf_main.chams.i_mat_visible, XorStrCT(" Flat\0 Shaded\0 Pulse\0 Metallic\0 Glass\0"));
							ImGui::PopItemWidth();
							ImGuiEx::ColorPicker(XorStr("Color##COL_CHAMS_VIS_MAT_GENERAL"), &var.visuals.pf_teammate.vf_main.chams.col_visible, f_win_width - 27.f);

							ImGuiEx::Checkbox(XorStr("XQZ##CHAMS_GENERAL"), &var.visuals.pf_teammate.vf_main.chams.b_xqz);
							ImGui::SameLine(f_win_width / 2);
							ImGui::PushItemWidth((f_win_width / 2) - 27.f);
							ImGui::Combo(XorStr("##CHAMS_INVIS_MAT_GENERAL"), &var.visuals.pf_teammate.vf_main.chams.i_mat_invisible, XorStrCT(" Flat\0 Shaded\0 Pulse\0 Metallic\0 Glass\0"));
							ImGui::PopItemWidth();
							ImGuiEx::ColorPicker(XorStr("Color##COL_CHAMS_INVIS_MAT_GENERAL"), &var.visuals.pf_teammate.vf_main.chams.col_invisible, f_win_width - 27.f);
							//encrypts(2)
							if (!var.visuals.pf_teammate.vf_main.chams.b_enabled)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}
							if (!var.visuals.pf_teammate.vf_main.b_enabled)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}
						}
						else if (active_filter == 3) // PROJECTILES
						{
							if (!var.visuals.vf_projectile.b_enabled)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}
							ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Chams:"));
							ImGui::Dummy(ImVec2(-4, 0));
							ImGui::SameLine(); // this could be hella easier if we had a group for everything...
							ImGuiEx::Checkbox(XorStr("Enable##CHAMS_GENERAL"), &var.visuals.vf_projectile.chams.b_enabled);

							if (!var.visuals.vf_projectile.chams.b_enabled)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}
							//decrypts(2)
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Material:"));
							ImGui::SameLine(f_win_width / 2);
							ImGui::PushItemWidth((f_win_width / 2) - 27.f);
							ImGui::Combo(XorStr("##CHAMS_VIS_MAT_GENERAL"), &var.visuals.vf_projectile.chams.i_mat_visible, XorStrCT(" Flat\0 Shaded\0 Pulse\0 Metallic\0 Glass\0"));
							ImGui::PopItemWidth();
							ImGuiEx::ColorPicker(XorStr("Color##COL_CHAMS_VIS_MAT_GENERAL"), &var.visuals.vf_projectile.chams.col_visible, f_win_width - 27.f);

							ImGuiEx::Checkbox(XorStr("XQZ##CHAMS_GENERAL"), &var.visuals.vf_projectile.chams.b_xqz);
							ImGui::SameLine(f_win_width / 2);
							ImGui::PushItemWidth((f_win_width / 2) - 27.f);
							ImGui::Combo(XorStr("##CHAMS_INVIS_MAT_GENERAL"), &var.visuals.vf_projectile.chams.i_mat_invisible, XorStrCT(" Flat\0 Shaded\0 Pulse\0 Metallic\0 Glass\0"));
							ImGui::PopItemWidth();
							ImGuiEx::ColorPicker(XorStr("Color##COL_CHAMS_INVIS_MAT_GENERAL"), &var.visuals.vf_projectile.chams.col_invisible, f_win_width - 27.f);
							//encrypts(2)
							if (!var.visuals.vf_projectile.chams.b_enabled)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}
							if (!var.visuals.vf_projectile.b_enabled)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}
						}
						else if (active_filter == 4) // WEAPONS
						{
							if (!var.visuals.vf_weapon.b_enabled)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}
							ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Chams:"));
							ImGui::Dummy(ImVec2(-4, 0));
							ImGui::SameLine(); // this could be hella easier if we had a group for everything...
							ImGuiEx::Checkbox(XorStr("Enable##CHAMS_GENERAL"), &var.visuals.vf_weapon.chams.b_enabled);

							if (!var.visuals.vf_weapon.chams.b_enabled)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}
							//decrypts(2)
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Material:"));
							ImGui::SameLine(f_win_width / 2);
							ImGui::PushItemWidth((f_win_width / 2) - 27.f);
							ImGui::Combo(XorStr("##CHAMS_VIS_MAT_GENERAL"), &var.visuals.vf_weapon.chams.i_mat_visible, XorStrCT(" Flat\0 Shaded\0 Pulse\0 Metallic\0 Glass\0"));
							ImGui::PopItemWidth();
							ImGuiEx::ColorPicker(XorStr("Color##COL_CHAMS_VIS_MAT_GENERAL"), &var.visuals.vf_weapon.chams.col_visible, f_win_width - 27.f);

							ImGuiEx::Checkbox(XorStr("XQZ##CHAMS_GENERAL"), &var.visuals.vf_weapon.chams.b_xqz);
							ImGui::SameLine(f_win_width / 2);
							ImGui::PushItemWidth((f_win_width / 2) - 27.f);
							ImGui::Combo(XorStr("##CHAMS_INVIS_MAT_GENERAL"), &var.visuals.vf_weapon.chams.i_mat_invisible, XorStrCT(" Flat\0 Shaded\0 Pulse\0 Metallic\0 Glass\0"));
							ImGui::PopItemWidth();
							ImGuiEx::ColorPicker(XorStr("Color##COL_CHAMS_INVIS_MAT_GENERAL"), &var.visuals.vf_weapon.chams.col_invisible, f_win_width - 27.f);
							//encrypts(2)
							if (!var.visuals.vf_weapon.chams.b_enabled)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}
							if (!var.visuals.vf_weapon.b_enabled)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}
						}
						else if (active_filter == 5) // MISC // ARMS
						{
							ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Arms:"));
							ImGui::Dummy(ImVec2(-4, 0));
							ImGui::SameLine(); // this could be hella easier if we had a group for everything...
							ImGuiEx::Checkbox(XorStr("Enable##ARMS_GENERAL"), &var.visuals.vf_arms.b_enabled);
							ImGui::Spacing();

							if (!var.visuals.vf_arms.b_enabled)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}
							//decrypts(2)
							ImGui::Separator();
							ImGui::Spacing();

							ImGuiEx::Checkbox(XorStr("Render Model##ARMS_GENERAL"), &var.visuals.vf_arms.b_render);
							//encrypts(2)

							ImGui::Spacing();
							ImGui::Separator();

							ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Chams:"));
							ImGui::Dummy(ImVec2(-4, 0));
							ImGui::SameLine();
							ImGuiEx::Checkbox(XorStr("Enable##ARMS_CHAMS"), &var.visuals.vf_arms.chams.b_enabled);
							//encrypts(2)

							if (var.visuals.vf_arms.chams.b_enabled)
							{
								//decrypts(2)
								ImGui::AlignTextToFramePadding();
								ImGui::Text(XorStr("Material:"));
								ImGui::SameLine(f_win_width / 2);
								ImGui::PushItemWidth((f_win_width / 2) - 27.f);
								ImGui::Combo(XorStr("##ARMS_CHAMS_VIS_MAT"), &var.visuals.vf_arms.chams.i_mat_visible, XorStrCT(" Flat\0 Shaded\0 Pulse\0 Metallic\0 Glass\0"));
								ImGui::PopItemWidth();
								ImGuiEx::ColorPicker(XorStr("Color##ARMS_CHAMS_VIS_MAT"), &var.visuals.vf_arms.chams.col_visible, f_win_width - 27.f);
								//encrypts(2)
							}
							if (!var.visuals.vf_arms.b_enabled)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}
						}
						else
						{
							ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Chams:"));
							ImGui::Dummy(ImVec2(-4, 0));
							ImGui::SameLine(); // this could be hella easier if we had a group for everything...
							ImGuiEx::Checkbox(XorStr("Real##CHAMS_GENERAL"), &var.visuals.pf_local_player.vf_main.chams.b_enabled);

							//decrypts(2)
							ImGuiEx::Checkbox(XorStr("Fake"), &var.visuals.b_fake);
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Fake Material:"));
							ImGui::SameLine(f_win_width / 2);
							ImGui::PushItemWidth((f_win_width / 2) - 27.f);
							ImGui::Combo(XorStr("##CHAMS_FAKE_MAT_GENERAL"), &var.visuals.pf_local_player.vf_main.chams.i_mat_desync_type, XorStrCT(" Flat\0 Shaded\0 Pulse\0 Metallic\0 Glass\0"));
							ImGui::PopItemWidth();
							ImGuiEx::ColorPicker(XorStr("##LP_CHAMS_FAKE_GENERAL"), &var.visuals.col_fake, f_win_width - 27.f);

							ImGui::Spacing();

							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Material:"));
							ImGui::SameLine(f_win_width / 2);
							ImGui::PushItemWidth((f_win_width / 2) - 27.f);
							ImGui::Combo(XorStr("##CHAMS_VIS_MAT_GENERAL"), &var.visuals.pf_local_player.vf_main.chams.i_mat_visible, XorStrCT(" Flat\0 Shaded\0 Pulse\0 Metallic\0 Glass\0"));
							ImGui::PopItemWidth();
							ImGuiEx::ColorPicker(XorStr("Color##COL_CHAMS_VIS_MAT_GENERAL"), &var.visuals.pf_local_player.vf_main.chams.col_visible, f_win_width - 27.f);

							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Blend When Scoped:"));
							ImGui::SameLine(f_win_width / 2);
							ImGui::SliderFloat(XorStr("##CHAMS_BLEND_SCOPE_GENERAL"), &var.visuals.f_blend_scope, 10.f, 100.f, XorStr("BLEND: %0.1f0%%"));
							//encrypts(2)

						}
						ImGui::PopItemWidth();
					}
					ImGui::EndChild();
					//encrypts(1)
				}
				ImGui::Columns(1);
			}
			//encrypts(0)
			ImGui::EndChild();
			if (!var.visuals.b_enabled)
			{
				ImGui::PopItemFlag();
				ImGui::PopStyleVar();
			}
		}
		ImGui::Columns(1);
		ImGui::End();
	}
}

void ui::waypoints() const
{
	auto& var = variable::get();
	auto& _style = ImGui::GetStyle();

	ImGui::SetNextWindowPos(ImVec2(500, 500), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(273 /** f_scale*/, 575 /** f_scale*/));

	const auto col_controller = var.ui.col_controller.color().ToImGUI();

	//decrypts(0)
	std::string config_title = ICON_FA_COGS;
	config_title += XorStr("  WAYPOINTS");
	//encrypts(0)

	if (ImGui::Begin(config_title.data(), &var.ui.b_config, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
	{
		m_WaypointsMutex.lock();

		//ImVec2 WindowPos = ImGui::GetWindowPos();
		//ImVec2 WindowExtents = ImGui::GetWindowSize();
		ImVec2 CursorPos = ImGui::GetMousePos();
		bool Clicking = ImGui::IsMouseClicked(0);
		ImRect WindowRect = ImGui::GetCurrentWindow()->Rect();

		bool ClickingInWindow = ImGui::IsMouseHoveringRect(WindowRect.Min, WindowRect.Max) && Clicking;

		//decrypts(0)
		ImGui::PushFont(ImFontEx::header);
		ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Menu"));
		ImGui::PopFont();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::PushItemWidth(-1);

		ImGuiEx::Checkbox(XorStr("Enable Walk-Bot (Alpha)"), &var.waypoints.b_enable_walkbot, true, nullptr, true, ImVec4(1.f, 1.f, 0.f, 1.f));
		ImGuiEx::SetTip(XorStr("Automatically walk around the map and shoot at players"));

		if (var.waypoints.b_enable_walkbot)
		{
			ImGuiEx::Checkbox(XorStr("Rage Walk-Bot"), &var.waypoints.b_walkbot_ragehack, true, nullptr, true, ImVec4(1.f, 1.f, 0.f, 1.f));
			ImGuiEx::SetTip(XorStr("The Walk-Bot will rage hack for you instead of trying to look legit"));
		}

		ImGuiEx::Checkbox(XorStr("Draw One-Ways"), &var.waypoints.b_draw_oneways, true, nullptr, true, ImVec4(1.f, 1.f, 0.f, 1.f));
		ImGuiEx::SetTip(XorStr("Draws information about one ways in the level"));

		ImGuiEx::Checkbox(XorStr("Draw Camp Spots"), &var.waypoints.b_draw_campspots, true, nullptr, true, ImVec4(1.f, 1.f, 0.f, 1.f));
		ImGuiEx::SetTip(XorStr("Draws information about good places to camp in the level"));

		ImGuiEx::Checkbox(XorStr("Auto Download Waypoints"), &var.waypoints.b_download_waypoints, true, nullptr, true, ImVec4(1.f, 1.f, 0.f, 1.f));
		ImGuiEx::SetTip(XorStr("Automatically downloads waypoints for the current level if available"));

		if (Interfaces::EngineClient->IsInGame())
		{
			ImGuiEx::Checkbox(XorStr("Enable Waypoint Creator"), &var.waypoints.b_enable_creator, true, nullptr, true, ImVec4(1.f, 1.f, 0.f, 1.f));
			ImGuiEx::SetTip(XorStr("Draws waypoints and allows you to create and edit them using hotkeys or selecting them in the world with the menu open and clicking on one"));

			if (var.waypoints.b_enable_creator)
			{
				LocalPlayer.Get(&LocalPlayer);

				const auto z = ImGui::ButtonEx(XorStr("Clear Waypoints"), ImVec2(122, 20), ImGuiButtonFlags_PressedOnClickRelease);
				ImGuiEx::SetTip(XorStr("Clears all waypoints for the current level"));

				if (z)
					ClearAllWaypoints();

				const auto a = ImGui::ButtonEx(XorStr("Load Waypoints"), ImVec2(122, 20), ImGuiButtonFlags_PressedOnClickRelease);
				ImGuiEx::SetTip(XorStr("Loads the waypoints for the current level if available"));

				if (a)
					ReadWaypoints();

				const auto b = ImGui::ButtonEx(XorStr("Save Waypoints"), ImVec2(122, 20), ImGuiButtonFlags_PressedOnClickRelease);
				ImGuiEx::SetTip(XorStr("Saves the waypoints to a file"));

				if (b)
					SaveWaypoints();

				ImGuiEx::Checkbox(XorStr("Auto waypoint"), &var.waypoints.b_enable_autowaypoint, true, nullptr, true, ImVec4(1.f, 1.f, 0.f, 1.f));
				ImGuiEx::SetTip(XorStr("Creates waypoints around as you walk, crouch, jump around the map and tags them for you as best it can. May need manual intervention for some waypoints"));

				ImGui::AlignTextToFramePadding();
				ImGui::TextColored(ImColor(255, 255, 0, 255), XorStr("Create Waypoint Key:"));
				ImGui::SameLine(ImGui::GetWindowWidth() / 2);
				ImGuiEx::KeyBindButton(XorStr("##WAYPOINTS_CREATE"), var.waypoints.i_createwaypoint_key, false);
				ImGuiEx::SetTip(XorStr("Choose a button that you will use to spawn a waypoint at your feet"));

				ImGui::AlignTextToFramePadding();
				ImGui::TextColored(ImColor(255, 255, 0, 255), XorStr("Delete Waypoint Key:"));
				ImGui::SameLine(ImGui::GetWindowWidth() / 2);
				ImGuiEx::KeyBindButton(XorStr("##WAYPOINTS_DELETE"), var.waypoints.i_deletewaypoint_key, false);
				ImGuiEx::SetTip(XorStr("Choose a button that you will use to delete the closest waypoint to you"));

				if (Clicking && !ClickingInWindow && LocalPlayer.Entity)
				{
					//Find the nearest waypoint to the mouse cursor position

					Vector vecDir;
					Vector WorldPosition;
					var.waypoints.m_pSelectedWaypoint = GetWaypointFromScreenPosition(CursorPos, &WorldPosition, &vecDir);

					//If no waypoint was selected, show the creation menu
					if (!var.waypoints.m_pSelectedWaypoint && !WorldPosition.IsZero())
					{
						var.waypoints.SelectedWorldPosition.m_vecOrigin = WorldPosition;
						var.waypoints.SelectedWorldPosition.m_vecDir = vecDir;
					}
				}

				if (!var.waypoints.m_pSelectedWaypoint)
				{
					if (!var.waypoints.SelectedWorldPosition.m_vecOrigin.IsZero())
					{
						//There is a selected position on the map, draw something to let us know that we selected it

						ImGui::Separator();
						ImGui::PushFont(ImFontEx::header);
						ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Waypoint Editor"));
						ImGui::Spacing();
						ImGui::PopFont();

						QAngle Angles;
						VectorAngles(var.waypoints.SelectedWorldPosition.m_vecDir, Angles);
						Interfaces::DebugOverlay->AddBoxOverlay(var.waypoints.SelectedWorldPosition.m_vecOrigin, Vector(-5, -5, -5), Vector(5, 5, 5), Angles, 195, 195, 195, 125, TICKS_TO_TIME(2));
						Interfaces::DebugOverlay->AddTextOverlay(var.waypoints.SelectedWorldPosition.m_vecOrigin, TICKS_TO_TIME(2), XorStr("Selected Waypoint Position"));

						//Show the creation menu
						const auto c = ImGui::ButtonEx(XorStr("Create Waypoint"), ImVec2(122, 20), ImGuiButtonFlags_PressedOnClickRelease);
						ImGuiEx::SetTip(XorStr("Create a waypoint at the selected position in the world"));

						if (c)
						{
							var.waypoints.m_pSelectedWaypoint = CreateWaypoint();
							if (var.waypoints.m_pSelectedWaypoint)
							{
								var.waypoints.m_pSelectedWaypoint->m_vecOrigin = var.waypoints.SelectedWorldPosition.m_vecOrigin;
								VectorAngles(var.waypoints.SelectedWorldPosition.m_vecDir, var.waypoints.m_pSelectedWaypoint->m_vecAngles);
								var.waypoints.m_pSelectedWaypoint->m_vecNormal.Init();
								var.waypoints.m_pSelectedWaypoint->m_vecMins = { -5.0f, -5.0f, -5.0f };
								var.waypoints.m_pSelectedWaypoint->m_vecMaxs = { 5.0f, 5.0f, 5.0f };
								var.waypoints.m_pSelectedWaypoint->m_iFlags = 0;
								Interfaces::Surface->Play_Sound(XorStr("ui\\csgo_ui_crate_item_scroll.wav"));
							}
							var.waypoints.SelectedWorldPosition.Reset();
						}
					}
					else if (LocalPlayer.Entity && LocalPlayer.Entity->GetAlive())
					{
						//Show the button to create a waypoint at the player's feet

						//Show the creation menu
						const auto c = ImGui::ButtonEx(XorStr("Create Waypoint"), ImVec2(122, 20), ImGuiButtonFlags_PressedOnClickRelease);
						ImGuiEx::SetTip(XorStr("Create a waypoint where you are standing"));

						if (c)
						{
							var.waypoints.m_pSelectedWaypoint = CreateWaypoint();
							if (var.waypoints.m_pSelectedWaypoint)
							{
								var.waypoints.m_pSelectedWaypoint->m_vecOrigin = LocalPlayer.Current_Origin;
								Interfaces::EngineClient->GetViewAngles(var.waypoints.m_pSelectedWaypoint->m_vecAngles);
								var.waypoints.m_pSelectedWaypoint->m_vecAngles.x = 0.0f;
								var.waypoints.m_pSelectedWaypoint->m_vecAngles.z = 0.0f;
								var.waypoints.m_pSelectedWaypoint->m_vecNormal.Init();
								var.waypoints.m_pSelectedWaypoint->m_vecMins = { -5.0f, -5.0f, -5.0f };
								var.waypoints.m_pSelectedWaypoint->m_vecMaxs = { 5.0f, 5.0f, 5.0f };
								var.waypoints.m_pSelectedWaypoint->m_iFlags = 0;
								Interfaces::Surface->Play_Sound(XorStr("ui\\csgo_ui_crate_item_scroll.wav"));
							}
							var.waypoints.SelectedWorldPosition.Reset();
						}
					}
				}

				//If there is a waypoint selected, show the editor
				else if (var.waypoints.m_pSelectedWaypoint)
				{
					//Highlight it in the world
					int alpha = 25;
					if (var.waypoints.m_pSelectedWaypoint->m_iFlags & (WAYPOINT_FLAG_ONEWAY | WAYPOINT_FLAG_ONEWAY_VICTIM))
						alpha = 5; //less visible if this is a waypoint that we can edit the mins/maxs/angles
					Interfaces::DebugOverlay->AddBoxOverlay(var.waypoints.m_pSelectedWaypoint->m_vecOrigin, Vector(-16, -16, -16), Vector(16, 16, 16), var.waypoints.m_pSelectedWaypoint->m_vecAngles, 239, 228, 176, alpha, TICKS_TO_TIME(2));
					Interfaces::DebugOverlay->AddTextOverlay(var.waypoints.m_pSelectedWaypoint->m_vecOrigin + Vector(0, 0, -5), TICKS_TO_TIME(2), XorStr("Selected Waypoint"));

					ImGui::Separator();
					ImGui::PushFont(ImFontEx::header);
					ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Waypoint Editor"));
					ImGui::Spacing();
					ImGui::PopFont();

					var.waypoints.SelectedWorldPosition.Reset();

					const auto c = ImGui::ButtonEx(XorStr("Delete Waypoint"), ImVec2(122, 20), ImGuiButtonFlags_PressedOnClickRelease);
					ImGuiEx::SetTip(XorStr("Delete the selected waypoint"));

					if (c)
					{
						DeleteWaypoint(var.waypoints.m_pSelectedWaypoint);
						var.waypoints.m_pSelectedWaypoint = nullptr;
						Interfaces::Surface->Play_Sound(XorStr("items\\suitchargeno1.wav"));
					}

					//Do we still have a waypoint selected, if so, draw the editor
					if (var.waypoints.m_pSelectedWaypoint)
					{
						if (var.waypoints.m_pSelectedWaypoint->m_iFlags & (WAYPOINT_FLAG_CAMP | WAYPOINT_FLAG_ONEWAY | WAYPOINT_FLAG_ONEWAY_VICTIM | WAYPOINT_FLAG_FAKEDUCK))
						{
							//decrypts(1)
							ImGui::Text(XorStr("Bounding Mins"));
							ImGui::PushItemWidth(75);
							ImGui::SliderFloat(XorStr("##WAYPOINT_MINS_X"), &var.waypoints.m_pSelectedWaypoint->m_vecMins.x, -64.f, 64.f, XorStr("X %0.1f"));
							ImGui::SameLine();
							ImGui::SliderFloat(XorStr("##WAYPOINT_MINS_Y"), &var.waypoints.m_pSelectedWaypoint->m_vecMins.y, -64.f, 64.f, XorStr("Y %0.1f"));
							ImGui::SameLine();
							ImGui::SliderFloat(XorStr("##WAYPOINT_MINS_Z"), &var.waypoints.m_pSelectedWaypoint->m_vecMins.z, -64.f, 64.f, XorStr("Z %0.1f"));
							ImGui::PopItemWidth();

							ImGui::Text(XorStr("Bounding Maxs"));
							ImGui::PushItemWidth(75);
							ImGui::SliderFloat(XorStr("##WAYPOINT_MAXS_X"), &var.waypoints.m_pSelectedWaypoint->m_vecMaxs.x, -64.f, 64.f, XorStr("X %0.1f"));
							ImGui::SameLine();
							ImGui::SliderFloat(XorStr("##WAYPOINT_MAXS_Y"), &var.waypoints.m_pSelectedWaypoint->m_vecMaxs.y, -64.f, 64.f, XorStr("Y %0.1f"));
							ImGui::SameLine();
							ImGui::SliderFloat(XorStr("##WAYPOINT_MAXS_Z"), &var.waypoints.m_pSelectedWaypoint->m_vecMaxs.z, -64.f, 64.f, XorStr("Z %0.1f"));
							ImGui::PopItemWidth();

							ImGui::Text(XorStr("Angles"));
							ImGui::PushItemWidth(75);
							ImGui::SliderFloat(XorStr("##WAYPOINT_ANGLES_X"), &var.waypoints.m_pSelectedWaypoint->m_vecAngles.x, -180.f, 180.f, XorStr("Pitch %0.1f"));
							ImGui::SameLine();
							ImGui::SliderFloat(XorStr("##WAYPOINT_ANGLES_Y"), &var.waypoints.m_pSelectedWaypoint->m_vecAngles.y, -180.f, 180.f, XorStr("Yaw %0.1f"));
							ImGui::SameLine();
							ImGui::SliderFloat(XorStr("##WAYPOINT_ANGLES_Z"), &var.waypoints.m_pSelectedWaypoint->m_vecAngles.z, -180.f, 180.f, XorStr("Roll %0.1f"));
							ImGui::PopItemWidth();
							//encrypts(1)
						}

						//decrypts(1)
						static std::vector<std::tuple<const char*, bool*, const char*>> flags_ui =
						{
							{ XorStr("Jump"),							&var.waypoints.editor.b_jump,	nullptr },
							{ XorStr("Duck"),							&var.waypoints.editor.b_duck, nullptr },
							{ XorStr("Fake Duck"),						&var.waypoints.editor.b_fakeduck, nullptr },
							{ XorStr("Camp Spot"),						&var.waypoints.editor.b_campspot, nullptr },
							{ XorStr("One Way Advantage"),				&var.waypoints.editor.b_oneway, nullptr },
							{ XorStr("One Way Disadvantage"),			&var.waypoints.editor.b_oneway_victim, nullptr },
							{ XorStr("Bomb Site"),						&var.waypoints.editor.b_bombsite, nullptr },
							{ XorStr("Safe Zone"),						&var.waypoints.editor.b_safezone, nullptr },
							{ XorStr("CT Spawn"),						&var.waypoints.editor.b_ctspawn, nullptr },
							{ XorStr("T Spawn"),						&var.waypoints.editor.b_tspawn, nullptr },
							{ XorStr("Ladder Top"),						&var.waypoints.editor.b_ladder_top, nullptr },
							{ XorStr("Ladder Step"),					&var.waypoints.editor.b_ladder_step, nullptr },
							{ XorStr("Ladder Bottom"),					&var.waypoints.editor.b_ladder_bottom, nullptr },
							{ XorStr("Press Use Key"),					&var.waypoints.editor.b_use, nullptr },
							{ XorStr("Recommended Path"),				&var.waypoints.editor.b_path, nullptr },
							{ XorStr("Breakable"),						&var.waypoints.editor.b_breakable, nullptr }
						};

						//set the menu bools to the values from the selected waypoint
						auto &flags = var.waypoints.m_pSelectedWaypoint->m_iFlags;
						var.waypoints.editor.b_jump = (flags & WAYPOINT_FLAG_JUMP) != 0;
						var.waypoints.editor.b_duck = (flags & WAYPOINT_FLAG_CROUCH) != 0;
						var.waypoints.editor.b_fakeduck = (flags & WAYPOINT_FLAG_FAKEDUCK) != 0;
						var.waypoints.editor.b_campspot = (flags & WAYPOINT_FLAG_CAMP) != 0;
						var.waypoints.editor.b_oneway = (flags & WAYPOINT_FLAG_ONEWAY) != 0;
						var.waypoints.editor.b_oneway_victim = (flags & WAYPOINT_FLAG_ONEWAY_VICTIM) != 0;
						var.waypoints.editor.b_bombsite = (flags & WAYPOINT_FLAG_BOMBSITE) != 0;
						var.waypoints.editor.b_safezone = (flags & WAYPOINT_FLAG_SAFEZONE) != 0;
						var.waypoints.editor.b_ctspawn = (flags & WAYPOINT_FLAG_CTSPAWN) != 0;
						var.waypoints.editor.b_tspawn = (flags & WAYPOINT_FLAG_TSPAWN) != 0;
						var.waypoints.editor.b_ladder_top = (flags & WAYPOINT_FLAG_LADDER_TOP) != 0;
						var.waypoints.editor.b_ladder_step = (flags & WAYPOINT_FLAG_LADDER_STEP) != 0;
						var.waypoints.editor.b_ladder_bottom = (flags & WAYPOINT_FLAG_LADDER_BOTTOM) != 0;
						var.waypoints.editor.b_use = (flags & WAYPOINT_FLAG_USE) != 0;
						var.waypoints.editor.b_path = (flags & WAYPOINT_FLAG_PATH) != 0;
						var.waypoints.editor.b_breakable = (flags & WAYPOINT_FLAG_BREAKABLE) != 0;

						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Waypoint Tags:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 12.f);
						ImGuiEx::SelectableCombo(XorStr("##WAYPOINTS_TAGS"), flags_ui, 6);
						ImGuiEx::SetTip(XorStr("Choose the tags for this waypoint"));

						auto SetFlag = [&flags](unsigned int flag, bool value)
						{
							if (value)
								flags |= flag;
							else
								flags &= ~flag;
						};

						//now store the new flags from the menu into the waypoint
						SetFlag(WAYPOINT_FLAG_JUMP, var.waypoints.editor.b_jump);
						SetFlag(WAYPOINT_FLAG_CROUCH, var.waypoints.editor.b_duck);
						SetFlag(WAYPOINT_FLAG_FAKEDUCK, var.waypoints.editor.b_fakeduck);
						SetFlag(WAYPOINT_FLAG_CAMP, var.waypoints.editor.b_campspot);
						SetFlag(WAYPOINT_FLAG_ONEWAY, var.waypoints.editor.b_oneway);
						SetFlag(WAYPOINT_FLAG_ONEWAY_VICTIM, var.waypoints.editor.b_oneway_victim);
						SetFlag(WAYPOINT_FLAG_BOMBSITE, var.waypoints.editor.b_bombsite);
						SetFlag(WAYPOINT_FLAG_SAFEZONE, var.waypoints.editor.b_safezone);
						SetFlag(WAYPOINT_FLAG_CTSPAWN, var.waypoints.editor.b_ctspawn);
						SetFlag(WAYPOINT_FLAG_TSPAWN, var.waypoints.editor.b_tspawn);
						SetFlag(WAYPOINT_FLAG_LADDER_TOP, var.waypoints.editor.b_ladder_top);
						SetFlag(WAYPOINT_FLAG_LADDER_STEP, var.waypoints.editor.b_ladder_step);
						SetFlag(WAYPOINT_FLAG_LADDER_BOTTOM, var.waypoints.editor.b_ladder_bottom);
						SetFlag(WAYPOINT_FLAG_USE, var.waypoints.editor.b_use);
						SetFlag(WAYPOINT_FLAG_PATH, var.waypoints.editor.b_path);
						SetFlag(WAYPOINT_FLAG_BREAKABLE, var.waypoints.editor.b_breakable);

						//encrypts(1)
					}
				}

				ImGui::Separator();
				ImGui::AlignTextToFramePadding();
				ImGui::Text(XorStr("Hint: Click anywhere in the level to use the editor."));
				ImGui::Spacing();
				ImGui::Text(XorStr("With the menu open, you can create waypoints"));
				ImGui::Text(XorStr("by clicking on the world and then select Create."));
				ImGui::Spacing();
				ImGui::Text(XorStr("You can also click on an existing waypoint to delete it"));
				ImGui::Text(XorStr("or modify its tags."));
				ImGui::Spacing();
				ImGui::Text(XorStr("Key binds allow you to create a waypoint on-the-fly"));
				ImGui::Text(XorStr("or delete the waypoint nearest to you."));

			}
		}
		ImGui::PopItemWidth();
		ImGui::End();
		//encrypts(0)
		m_WaypointsMutex.unlock();
	}
}

void ui::config() const
{
	auto& var = variable::get();
	auto& _style = ImGui::GetStyle();

	ImGui::SetNextWindowPos(ImVec2(500, 500), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(273 /** f_scale*/, 575 /** f_scale*/));

	const auto col_controller = var.ui.col_controller.color().ToImGUI();
	//decrypts(0)
	std::string config_title = ICON_FA_COGS;
	config_title += XorStr("  CONFIG");
	//encrypts(0)

	if (ImGui::Begin(config_title.data(), &var.ui.b_config, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
	{
		//decrypts(0)
		ImGui::PushFont(ImFontEx::header);
		ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Menu"));
		ImGui::PopFont();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::PushItemWidth(-1);

		ImGui::Text(XorStr("Controller Color"));
		ImGuiEx::ColorPicker(XorStr("##CONT_COLOR"), &var.ui.col_controller, 234);

		ImGui::Text(XorStr("Text Color"));
		ImGuiEx::ColorPicker(XorStr("##TEXT_COLOR"), &var.ui.col_text, 234);

		ImGui::Text(XorStr("Background Color"));
		ImGuiEx::ColorPicker(XorStr("##BACK_COLOR"), &var.ui.col_background, 234);

		ImGui::Text(XorStr("Animation Speed:"));

		ImGui::SliderFloat(XorStr("##MENU_SPEED"), &var.ui.f_menu_time, 0.f, 1.f, XorStr("ANIMATION SPEED: %0.2f"));
		ImGui::PopItemWidth();

		ImGuiEx::Checkbox(XorStr("Allow Untrusted Features"), &var.ui.b_allow_untrusted, true, nullptr, true, ImVec4(1.f, 1.f, 0.f, 1.f));
		ImGuiEx::SetTip(XorStr("Bypass the protection of not allowing yellow-colored features on Valve servers. Use this at your own risk."));

		ImGuiEx::Checkbox(XorStr("Enable Tooltips"), &var.ui.b_use_tooltips);
		ImGuiEx::SetTip(XorStr("Enable tooltips on features to give more information on them such as this one."));

		ImGuiEx::Checkbox(XorStr("Allow Manual Values"), &var.ui.b_allow_manual_edits);
		ImGuiEx::SetTip(XorStr("Allow the action of 'Ctrl+Click' on UI elements for manual input. Use this at your risk."));

		ImGui::Spacing();

		ImGui::PushFont(ImFontEx::header);
		ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Config"));
		ImGui::PopFont();
		ImGui::Separator();
		ImGui::Spacing();

		static auto i_config_selected = 0;
		auto vec_configs = config::get().get_configs();

		ImGui::PushItemWidth(-1);
		static char buf[64] = "";

		ImGui::InputTextEx(XorStr("##CONFIGTEXT"), XorStrCT("Config name to be created."), buf, 64, ImVec2(225, 20), 0);
		ImGuiEx::SetTip(XorStr("Text input for config name."));

		ImGui::SameLine();

		const auto a = ImGui::ButtonEx(XorStr("..."), ImVec2(20, 20), ImGuiButtonFlags_PressedOnClickRelease);
		ImGuiEx::SetTip(XorStr("Create a config named by the text input."));

		if (a)
		{
			var.global.cfg_mthread = std::tuple<std::string, std::string, bool>(XorStr("create"), std::string(buf), true);
			memset(buf, 0, sizeof buf);
		}
		//encrypts(0)

		//decrypts(0)
		if (ImGui::ListBoxHeader(XorStr("##CONFIGS"), ImVec2(250, 160)))
		{
			for (auto i = 0; i < static_cast<int>(vec_configs.size()); i++)
			{
				auto label = adr_util::string::format(XorStr("%s##%04d"), vec_configs[i].c_str(), i);

				//auto b_change = false;
				//if ( i_config_selected == i )
				//{
				//	ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( _style.Colors[ ImGuiCol_TextDisabled ].x * 0.7f, _style.Colors[ ImGuiCol_TextDisabled ].y * 0.7f, _style.Colors[ ImGuiCol_TextDisabled ].z * 0.7f, 1.f ) );
				//	b_change = true;
				//}

				if (ImGui::Selectable(label.c_str(), i_config_selected == i, ImGuiSelectableFlags_SpanAllColumns))
					i_config_selected = i;

				//if ( b_change )
				//	ImGui::PopStyleColor();
			}
			ImGui::ListBoxFooter();
		}
		//encrypts(0)

		ImGui::Spacing();
		ImGui::PopItemWidth();

		if (!vec_configs.empty())
		{
			ImGui::PushItemWidth(-1);

			//decrypts(0)
			const auto b = ImGui::ButtonEx(XorStr("LOAD"), ImVec2(122, 20), ImGuiButtonFlags_PressedOnClickRelease);
			ImGuiEx::SetTip(XorStr("Load the selected config."));

			ImGui::SameLine(140);

			const auto c = ImGui::ButtonEx(XorStr("SAVE"), ImVec2(122, 20), ImGuiButtonFlags_PressedOnClickRelease);
			ImGuiEx::SetTip(XorStr("Save the current values to the selected config."));

			const auto d = ImGui::ButtonEx(XorStr("DELETE"), ImVec2(122, 20), ImGuiButtonFlags_PressedOnClickRelease);
			ImGuiEx::SetTip(XorStr("Delete the selected config."));

			ImGui::SameLine(140);

			const auto f = ImGui::ButtonEx(XorStr("CLEAR"), ImVec2(122, 20), ImGuiButtonFlags_PressedOnClickRelease);
			ImGuiEx::SetTip(XorStr("Clear all the values loaded from the config."));

			if (b)
			{
				var.global.cfg_mthread = std::tuple<std::string, std::string, bool>(XorStr("load"), vec_configs[i_config_selected], true);
			}

			if (f)
				config::get().initialize();

			if (c)
			{
				var.global.cfg_mthread = std::tuple<std::string, std::string, bool>(XorStr("save"), vec_configs[i_config_selected], true);
			}

			if (d)
			{
				std::string icon = ICON_FA_EXCLAMATION_TRIANGLE;
				icon += XorStr("  DELETE?");
				ImGui::OpenPopup(icon.data());
			}

			ImGui::PopItemWidth();
			//encrypts(0)
		}
		else
		{
			//decrypts(0)
			ImGui::TextWrapped(XorStr("You can create a config by clicking at the [ ... ] button."));
			//encrypts(0)
		}

		//decrypts(0)
		std::string popup = ICON_FA_EXCLAMATION_TRIANGLE;
		popup += XorStr("  DELETE?");

		if (ImGui::BeginPopupModal(popup.data(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text(XorStr("Are you sure you want to continue?"));
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			std::string check = ICON_FA_CHECK;
			check += XorStr("  Yes, delete.");
			if (ImGui::Button(check.data(), ImVec2(120, 0)))
			{
				var.global.cfg_mthread = std::tuple<std::string, std::string, bool>(XorStr("remove"), vec_configs[i_config_selected], true);
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();

			std::string times = ICON_FA_TIMES;
			times += XorStr("  No");

			if (ImGui::Button(times.data(), ImVec2(120, 0)))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
		//encrypts(0)
		ImGui::End();
	}
}

#ifdef INCLUDE_LEGIT
void ui::legit() const
{
	static auto i_active_tab = 0;
	if (i_active_tab == 0)
		ImGui::SetNextWindowSize(ImVec2(700, 393));
	else
		ImGui::SetNextWindowSize(ImVec2(700, 316));

	ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
	const auto col_controller = variable::get().ui.col_controller.color().ToImGUI();

	//decrypts(0)
	std::string legit_title = ICON_FA_GAMEPAD;
	legit_title += XorStr("  LEGIT");
	//encrypts(0)

	if (ImGui::Begin(legit_title.data(), &variable::get().ui.b_legit, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
	{
		if (std::get<2>(variable::get().global.cfg_mthread))
		{
			ImGui::PushFont(ImFontEx::header);
			//ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), adr_util::string::format(XorStr(ICON_FA_COGS "  Please wait  %c"), "|/-\\"[static_cast<int>(ImGui::GetTime() / 0.25f) & 3]).c_str());
			//decrypts(0)
			std::string format = ICON_FA_COGS;
			format += XorStr("  Please wait  %c");
			ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), adr_util::string::format(format, XorStr("|/-\\")[static_cast<int>(ImGui::GetTime() / 0.25f) & 3]).c_str());
			//encrypts(0)
			ImGui::PopFont();
			ImGui::End();
			return;
		}

		ImGui::Separator();
		ImGui::Dummy(ImVec2(-10, 0));
		ImGui::SameLine();
		//decrypts(0)
		static std::vector<const char*> vec_tab = { XorStr("AIM"), XorStr("TRIGGER") };
		static auto f_tab_max_size = (ImGui::GetWindowWidth() - 5.f) / vec_tab.size();
		ImGuiEx::RenderTabs(vec_tab, i_active_tab, f_tab_max_size, 15.f, true);
		ImGui::Separator();
		//encrypts(0)

		if (i_active_tab == 0)
		{
			//decrypts(0)
			ImGui::Columns(3, XorStr("##LEGITHEADER"), false);
			{
				ImGui::Dummy(ImVec2(-10, 10));
				ImGui::SameLine();
				ImGuiEx::Checkbox(XorStr("Enabled"), &variable::get().legitbot.aim.b_enabled.b_state, true, &variable::get().legitbot.aim.b_enabled);
			}
			ImGui::NextColumn();
			{
				ImGuiEx::Checkbox(XorStr("Per Weapon"), &variable::get().legitbot.aim.b_per_weapon);
			}
			ImGui::NextColumn();
			{
				ImGuiEx::Checkbox(XorStr("Backtrack"), &variable::get().legitbot.aim.b_lag_compensation);
			}
			ImGui::NextColumn();
			ImGui::Columns(1);
			//encrypts(0)

			ImGui::Separator();
			ImGui::Dummy(ImVec2(-10, 10));
			ImGui::SameLine();

			auto i_index = 0;
			auto i_type = 0;

			if (variable::get().legitbot.aim.b_per_weapon)
			{
				static auto selected = 0;

				//decrypts(0)
				static std::vector<std::pair<short, std::string>> aim_weapon_names =
				{
					{ WEAPON_NONE, XorStr("Default") },
					{ WEAPON_AK47, XorStr("AK-47") },
					{ WEAPON_AUG, XorStr("AUG") },
					{ WEAPON_AWP, XorStr("AWP") },
					{ WEAPON_CZ75A, XorStr("CZ75 Auto") },
					{ WEAPON_DEAGLE, XorStr("Desert Eagle") },
					{ WEAPON_ELITE, XorStr("Dual Berettas") },
					{ WEAPON_FAMAS, XorStr("FAMAS") },
					{ WEAPON_FIVESEVEN, XorStr("Five-SeveN") },
					{ WEAPON_G3SG1, XorStr("G3SG1") },
					{ WEAPON_GALILAR, XorStr("Galil AR") },
					{ WEAPON_GLOCK, XorStr("Glock-18") },
					{ WEAPON_M249, XorStr("M249") },
					{ WEAPON_M4A1_SILENCER, XorStr("M4A1-S") },
					{ WEAPON_M4A1, XorStr("M4A4") },
					{ WEAPON_MAC10, XorStr("MAC-10") },
					{ WEAPON_MAG7, XorStr("MAG-7") },
					{ WEAPON_MP5SD, XorStr("MP5-SD") },
					{ WEAPON_MP7, XorStr("MP7") },
					{ WEAPON_MP9, XorStr("MP9") },
					{ WEAPON_NEGEV, XorStr("Negev") },
					{ WEAPON_NOVA, XorStr("Nova") },
					{ WEAPON_HKP2000, XorStr("P2000") },
					{ WEAPON_P250, XorStr("P250") },
					{ WEAPON_P90, XorStr("P90") },
					{ WEAPON_BIZON, XorStr("PP-Bizon") },
					{ WEAPON_REVOLVER, XorStr("R8 Revolver") },
					{ WEAPON_SAWEDOFF, XorStr("Sawed-Off") },
					{ WEAPON_SCAR20, XorStr("SCAR-20") },
					{ WEAPON_SSG08, XorStr("SSG 08") },
					{ WEAPON_SG556, XorStr("SG 556") },
					{ WEAPON_TEC9, XorStr("Tec-9") },
					{ WEAPON_UMP45, XorStr("UMP-45") },
					{ WEAPON_USP_SILENCER, XorStr("USP-S") },
					{ WEAPON_XM1014, XorStr("XM1014") },
				};
				//encrypts(0)

				//decrypts(0)
				ImGui::Columns(4, XorStr("##WEAPONHEADER"), false);
				{
					ImGui::Dummy(ImVec2(-11, 10));
					ImGui::SameLine();
					ImGui::PushItemWidth(160);

					ImGui::Combo(XorStr("##WEAPON"), &selected, [](void* data, int idx, const char** out_text)
						{
							*out_text = aim_weapon_names[idx].second.data();
							return true;
						}, nullptr, aim_weapon_names.size(), 6);

					i_index = aim_weapon_names[selected].first;
					ImGui::PopItemWidth();
				}
				//encrypts(0)
				ImGui::NextColumn();
				{
					if (Interfaces::EngineClient->IsInGame() && Interfaces::EngineClient->IsConnected() && LocalPlayer.Entity && LocalPlayer.Entity->GetAlive())
					{
						//decrypts(0)
						std::string id = XorStr("Get Current Weapon");
						//encrypts(0)
						if (ImGui::ButtonEx(id.data(), ImVec2(160, 20), ImGuiButtonFlags_PressedOnRelease))
						{
							auto p_weapon = LocalPlayer.Entity->GetActiveCSWeapon();
							auto i_current_index = p_weapon->GetItemDefinitionIndex();

							if (p_weapon && !p_weapon->IsGrenade() && !p_weapon->IsKnife() && !p_weapon->IsBomb() && i_index != WEAPON_TASER)
							{
								i_index = i_current_index;
								for (auto i = 0; i < static_cast<int>(aim_weapon_names.size()); i++)
								{
									if (aim_weapon_names[i].first == i_index)
										selected = i;
								}
							}
						}
					}
				}

				if (adr_util::is_pistol(static_cast<ItemDefinitionIndex>(i_index)))
					i_type = WEAPONTYPE_PISTOL;
				else if (adr_util::is_shotgun(static_cast<ItemDefinitionIndex>(i_index)))
					i_type = WEAPONTYPE_SHOTGUN;
				else if (adr_util::is_smg(static_cast<ItemDefinitionIndex>(i_index)))
					i_type = WEAPONTYPE_SUBMACHINEGUN;
				else if (adr_util::is_rifle(static_cast<ItemDefinitionIndex>(i_index)))
					i_type = WEAPONTYPE_RIFLE;
				else if (adr_util::is_sniper(static_cast<ItemDefinitionIndex>(i_index)))
					i_type = WEAPONTYPE_SNIPER_RIFLE;
				else if (adr_util::is_heavy(static_cast<ItemDefinitionIndex>(i_index)))
					i_type = WEAPONTYPE_MACHINEGUN;

				ImGui::NextColumn();
				{
					if (i_type != -1 && i_index > 0)
					{
						//decrypts(0)
						if (ImGui::ButtonEx(XorStr("Import Default"), ImVec2(160, 20), ImGuiButtonFlags_PressedOnRelease))
							variable::get().legitbot.aim_cfg[i_index] = variable::get().legitbot.aim_cfg[0];
						//encrypts(0)
					}
				}
				ImGui::NextColumn();
				ImGui::Columns(1);
			}
			else
			{
				static auto i_active_group = 0;
				std::vector<const char*> vec_weapon_groups = { (u8"\uE001"), (u8"\uE018"), (u8"\uE007"), (u8"\uE019"), (u8"\uE009"), (u8"\uE01C") };
				static auto f_group_max_size = (ImGui::GetWindowWidth() - 41.f) / vec_weapon_groups.size();
				ImGui::PushFont(ImFontEx::weapons);
				ImGuiEx::RenderTabs(vec_weapon_groups, i_active_group, f_group_max_size, 19, true);
				ImGui::PopFont();
				i_index = i_active_group + 67;
				i_type = i_active_group + 1;
			}
			ImGui::Separator();
			//ImGui::Spacing();
			if (i_type != -1 && i_index != -1)
			{
				auto& var = variable::get().legitbot.aim_cfg[i_index];

				ImGui::Dummy(ImVec2(-10, 0));
				ImGui::SameLine();
				static auto i_active_inside_tab = 0;
				//decrypts(0)
				std::vector<const char*> vec_inside_tab = { XorStr("TARGET##SUBTAB1"), XorStr("ACCURACY##SUBTAB1") };
				static auto f_inside_tab_max_size = (ImGui::GetWindowWidth() - 3.f) / vec_inside_tab.size();
				ImGuiEx::RenderTabs(vec_inside_tab, i_active_inside_tab, f_inside_tab_max_size, 16.f, true);
				ImGui::Separator();
				//encrypts(0)

				if (i_active_inside_tab == 0)
				{
					//decrypts(0)
					ImGui::Columns(3, XorStr("##LEGITAIMCOL"), false);
					{
						ImGui::BeginChild(XorStr("##LEGITAIMCOL1"), ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize);
						{
							ImGui::PushItemWidth(-1);

							ImGui::PushFont(ImFontEx::header);
							ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Target"));
							ImGui::PopFont();
							ImGui::Separator();
							ImGui::Spacing();

							ImGui::Text(XorStr("FOV:"));
							ImGui::SliderFloat(XorStr("##LEGITAIMFOV"), &var.f_fov, 0.f, 20.f, XorStr("FOV: %.1f0"));

							ImGui::Checkbox(XorStr("Dynamic"), &var.b_dynamic);
							ImGuiEx::SetTip(XorStr("Distance based fov."));

							ImGui::Checkbox(XorStr("In Crosshair"), &var.b_incross);
							ImGuiEx::SetTip(XorStr("Aim will check if there is a target in crosshair before trying to use FOV."));

							ImGui::Separator();

							ImGui::Text(XorStr("Re-Target:"));
							ImGui::SliderInt(XorStr("##LEGITAIMRETARGET"), &var.i_retarget_time, 0, 3000, XorStr("TIME: %0.0f ms"));

							if (i_type == WEAPONTYPE_RIFLE || i_type == WEAPONTYPE_SUBMACHINEGUN || i_type == WEAPONTYPE_MACHINEGUN)
							{
								ImGui::Separator();
								ImGui::Text(XorStr("Activation:"));
								ImGui::SliderInt(XorStr("##ACTIVATIONSHOT"), &var.i_activation_shots, 0, 5, XorStr("SHOTS: %0.0f"));
							}

							ImGui::PopItemWidth();
							ImGui::EndChild();
						}
					}
					//encrypts(0)

					ImGui::NextColumn();
					{
						//decrypts(0)
						ImGui::BeginChild(XorStr("##LEGITAIMCOL2"), ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize);
						{
							ImGui::PushItemWidth(-1);

							ImGui::PushFont(ImFontEx::header);
							ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Misc"));
							ImGui::PopFont();
							ImGui::Separator();
							ImGui::Spacing();

							ImGui::Checkbox(XorStr("Smoke Check"), &var.b_check_smoke);
							ImGui::Checkbox(XorStr("Flash Check"), &var.b_check_flash);

							if (i_type == WEAPONTYPE_SNIPER_RIFLE)
								ImGui::Checkbox(XorStr("Scope Check"), &var.b_check_scope);

							ImGui::Separator();
							ImGui::Checkbox(XorStr("Wait for IN_ATTACK"), &var.b_wait_for_in_attack);
							ImGui::Checkbox(XorStr("First Shot Only"), &var.b_first_shot_assist);
							ImGuiEx::SetTip(XorStr("Aim will just activate for the first bullet."));
							ImGui::Checkbox(XorStr("Lock On First Shot"), &var.b_lock);
							ImGuiEx::SetTip(XorStr("Aim will not follow the target if moving."));
							ImGui::Checkbox(XorStr("Ignore In Crosshair"), &var.b_skip_incross);
							ImGuiEx::SetTip(XorStr("Aim will not activate if there is a target in crosshair."));

							ImGui::PopItemWidth();
							ImGui::EndChild();
						}
						//encrypts(0)
					}
					ImGui::NextColumn();
					{
						//decrypts(0)
						ImGui::BeginChild(XorStr("##LEGITAIMCOL3"), ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize);
						{
							ImGui::PushItemWidth(-1);

							ImGui::PushFont(ImFontEx::header);
							ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Hitboxes"));
							ImGui::PopFont();
							ImGui::Separator();
							ImGui::Spacing();

							if (ImGui::ListBoxHeader(XorStr("##LEGITHITBOX"), ImVec2(-1, -1)))
							{
								for (auto i = 0; i < static_cast<int>(var.hitboxes.size()); i++)
									ImGui::Selectable(std::get<0>(var.hitboxes[i]).c_str(), &std::get<2>(var.hitboxes[i]));

								ImGui::ListBoxFooter();
							}
							ImGui::PopItemWidth();
							ImGui::EndChild();
						}
						//encrypts(0)
					}
					ImGui::Columns(1);
				}
				else
				{
					//decrypts(0)
					ImGui::Columns(3, XorStr("##LEGITAIMCOL"), false);
					{
						ImGui::BeginChild(XorStr("##LEGITAIMCOL1"), ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize);
						{
							ImGui::PushItemWidth(-1);

							ImGui::Separator();
							ImGui::Dummy(ImVec2(-10, 0));
							ImGui::SameLine();
							static auto i_active_mode_tab = 0;
							//decrypts(1)
							std::vector<const char*> vec_inside_tab = { XorStr("ACCURACY"), XorStr("MISC") };
							static auto f_inside_tab_max_size = (ImGui::GetWindowWidth() - 3.f) / vec_inside_tab.size();
							ImGuiEx::RenderTabs(vec_inside_tab, i_active_mode_tab, f_inside_tab_max_size, 16.f, true);
							ImGui::Separator();
							//encrypts(1)

							if (i_active_mode_tab == 0)
							{
								if (!var.b_silentaim)
								{
									//decrypts(1)
									ImGui::Text(XorStr("Speed Type:"));
									ImGui::Combo(XorStr("##SPEEDTYPE"), &var.i_speed_type, XorStrCT(" Constant\0 Slow End\0 Fast End\0"));
									ImGui::SliderFloat(XorStr("##SPEEDAIM"), &var.f_speed, 0.f, 1.f, XorStr("SPEED: %0.2f"));
									ImGui::Checkbox(XorStr("Randomize Speed"), &var.b_randomize_speed);

									ImGui::Checkbox(XorStr("Align Speed"), &var.b_aim_time);
									ImGuiEx::SetTip(XorStr("Align the speed to the aim time."));

									ImGui::SliderInt(XorStr("##SLOWAIM"), &var.i_slow_moving, 0, 100, XorStr("SLOW WHEN MOVING: %0.2f%"));
									ImGuiEx::SetTip(XorStr("Speed percentage to be kept when target is moving."));
									//encrypts(1)
								}
							}
							else
							{
								//decrypts(1)
								ImGui::Text(XorStr("Recoil:"));
								ImGui::SliderFloat(XorStr("##HORIZONTALLEGITBOT"), &var.f_x_rcs, 0.f, 1.f, XorStr("HORIZONTAL CONTROL: %0.2f"));
								ImGui::SliderFloat(XorStr("##VERTICALLEGITBOT"), &var.f_y_rcs, 0.f, 1.f, XorStr("VERTICAL CONTROL: %0.2f"));
								ImGui::Checkbox(XorStr("Randomize Recoil"), &var.b_randomize_rcs);
								ImGui::Separator();
								ImGui::SliderFloat(XorStr("##HITCHANCEAIM"), &var.f_hitchance, 0.f, 100.f, XorStr("SPREAD LIMIT: %0.2f %"));
								//encrypts(1)
							}

							ImGui::PopItemWidth();
							ImGui::EndChild();
						}
					}
					//encrypts(0)
					ImGui::NextColumn();
					{
						//decrypts(0)
						ImGui::BeginChild(XorStr("##LEGITAIMCOL2"), ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize);
						{
							ImGui::PushItemWidth(-1);

							ImGui::PushFont(ImFontEx::header);
							ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("General"));
							ImGui::PopFont();
							ImGui::Separator();
							ImGui::Spacing();

							if (i_type == WEAPONTYPE_SNIPER_RIFLE)
								ImGui::Checkbox(XorStr("Auto Scope"), &var.b_auto_scope);

							if (i_type == WEAPONTYPE_PISTOL)
								ImGui::Checkbox(XorStr("Auto Pistol"), &var.b_auto_pistol);

							ImGui::Checkbox(XorStr("Auto Stop"), &var.b_auto_stop);
							ImGui::Checkbox(XorStr("Auto Wall"), &var.b_autowall);
							ImGui::Checkbox(XorStr("Auto Shoot"), &var.b_auto_shoot);
							ImGui::Checkbox(XorStr("Aim Step"), &var.b_aim_step);
							ImGui::Checkbox(XorStr("Silent Aim"), &var.b_silentaim);

							ImGui::PopItemWidth();
							ImGui::EndChild();
						}
						//encrypts(0)
					}
					ImGui::NextColumn();
					{
						//decrypts(0)
						ImGui::BeginChild(XorStr("##LEGITAIMCOL3"), ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize);
						{
							ImGui::PushItemWidth(-1);

							ImGui::PushFont(ImFontEx::header);
							ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Standalone RCS"));
							ImGui::PopFont();
							ImGui::Separator();
							ImGui::Spacing();

							ImGui::Checkbox(XorStr("Enabled##SRCS"), &var.b_standalone_rcs);
							ImGui::SliderFloat(XorStr("##SPEEDRCS"), &var.f_standalone_rcs_speed, 0.f, 1.f, XorStr("SPEED: %0.2f"));
							ImGui::Checkbox(XorStr("Randomize Speed"), &var.b_standalone_rcs_randomize_speed);
							ImGui::SliderFloat(XorStr("##HORIZONTALRCS"), &var.f_x_standalone_rcs, 0.f, 1.f, XorStr("HORIZONTAL CONTROL: %0.2f"));
							ImGui::SliderFloat(XorStr("##VERTICALRCS"), &var.f_y_standalone_rcs, 0.f, 1.f, XorStr("VERTICAL CONTROL: %0.2f"));
							ImGui::Checkbox(XorStr("Randomize Recoil"), &var.b_standalone_rcs_randomize_rcs);
							ImGui::Checkbox(XorStr("Only After Kill"), &var.b_standalone_rcs_after_kill);
							ImGuiEx::SetTip(XorStr("Standalone RCS will just turn on after the AIM (LegitBot) target dies."));

							/*if (i_type == WEAPONTYPE_RIFLE || i_type == WEAPONTYPE_SUBMACHINEGUN || i_type == WEAPONTYPE_MACHINEGUN)
							{
								ImGui::Text(XorStr("Activation:"));
								ImGui::SliderInt(XorStr("##ACTIVATIONSHOTRCS"), &var.i_standalone_rcs_activation_shots, 0, 5, XorStr("SHOTS: %0.0f"));
							}*/

							ImGui::PopItemWidth();
							ImGui::EndChild();
						}
						//encrypts(0)
					}
					ImGui::Columns(1);
				}
			}
		}
		else
		{
			//decrypts(0)
			ImGui::Columns(4, XorStr("##TRIGGERHEADER"), false);
			{
				ImGui::Dummy(ImVec2(-10, 10));
				ImGui::SameLine();
				ImGuiEx::Checkbox(XorStr("Enabled"), &variable::get().legitbot.trigger.b_enabled.b_state, true, &variable::get().legitbot.trigger.b_enabled);
			}
			//encrypts(0)
			ImGui::NextColumn();
			ImGui::NextColumn();
			ImGui::NextColumn();
			ImGui::NextColumn();
			ImGui::Columns(1);
			ImGui::Separator();
			ImGui::Dummy(ImVec2(-10, 10));
			ImGui::SameLine();

			static auto i_active_group = 0;
			std::vector<const char*> vec_weapon_groups = { (u8"\uE001"), (u8"\uE018"), (u8"\uE007"), (u8"\uE019"), (u8"\uE009"), (u8"\uE01C") };
			static auto f_group_max_size = (ImGui::GetWindowWidth() - 41.f) / vec_weapon_groups.size();
			ImGui::PushFont(ImFontEx::weapons);
			ImGuiEx::RenderTabs(vec_weapon_groups, i_active_group, f_group_max_size, 19, true);
			ImGui::PopFont();
			const auto i_type = i_active_group + 1;

			ImGui::Separator();
			//ImGui::Spacing();
			if (i_type != -1)
			{
				auto& var = variable::get().legitbot.trigger_cfg[i_type];
				//decrypts(0)
				ImGui::Columns(2, XorStr("##TRIGGERCOL"), false);
				{
					ImGui::BeginChild(XorStr("##TRIGGERCOL1"), ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize);
					{
						ImGui::PushItemWidth(-1);
						ImGui::PushFont(ImFontEx::header);
						ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("General"));
						ImGui::PopFont();
						ImGui::Separator();
						ImGui::Spacing();
						ImGui::SliderInt(XorStr("##TRIGGERDELAY"), &var.i_delay, 0, 1000, XorStr("DELAY: %0.0f ms"));
						ImGui::SliderFloat(XorStr("##TRIGGERHITCHANCE"), &var.f_hitchance, 0.f, 100.f, XorStr("HITCHANCE: %0.2f %"));
						ImGui::Spacing();
						ImGui::Spacing();
						ImGui::Columns(2, XorStr("##TRIGGERMISC"), false);
						{
							ImGui::Checkbox(XorStr("Smoke Check"), &var.b_check_smoke);
							ImGui::Checkbox(XorStr("Flash Check"), &var.b_check_flash);
						}
						ImGui::NextColumn();
						{
							if (i_type == WEAPONTYPE_SNIPER_RIFLE)
							{
								//decrypts(1)
								ImGui::Checkbox(XorStr("Scope Check"), &var.b_check_scope);
								ImGui::Checkbox(XorStr("Auto Scope"), &var.b_auto_scope);
								//encrypts(1)
							}
						}
						ImGui::Columns(1);
						ImGui::PopItemWidth();
						ImGui::EndChild();
					}
				}
				//encrypts(0)
				ImGui::NextColumn();
				{
					//decrypts(0)
					ImGui::BeginChild(XorStr("##TRIGGERCOL2"), ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize);
					{
						ImGui::PushItemWidth(-1);
						ImGui::PushFont(ImFontEx::header);
						ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Filter"));
						ImGui::PopFont();
						ImGui::Separator();
						ImGui::Spacing();
						if (ImGui::ListBoxHeader(XorStr("##RAGEBOTLISTBOXHITBOX"), ImVec2(-1, 101)))
						{
							//decrypts(1)
							ImGui::Selectable(XorStr("HEAD"), &var.b_filter_head);
							ImGui::Selectable(XorStr("CHEST"), &var.b_filter_chest);
							ImGui::Selectable(XorStr("STOMACH"), &var.b_filter_stomach);
							ImGui::Selectable(XorStr("ARMS"), &var.b_filter_arms);
							ImGui::Selectable(XorStr("LEGS"), &var.b_filter_legs);
							ImGui::ListBoxFooter();
							//encrypts(1)
						}
						ImGui::Checkbox(XorStr("Auto Wall"), &var.b_autowall);
						ImGui::PopItemWidth();
						ImGui::EndChild();
					}
					//encrypts(0)
				}
				ImGui::Columns(1);
			}
		}
		ImGui::End();
	}
}
#endif

void ui::rage() const
{
	auto& var = variable::get();

	ImGui::SetNextWindowSize(ImVec2(650 /** f_scale*/, 680 /** f_scale*/));
	ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);

	const auto col_controller = var.ui.col_controller.color().ToImGUI();

	//decrypts(0)
	std::string rage_title = ICON_FA_CROSSHAIRS;
	rage_title += XorStr("  RAGE");
	//encrypts(0)

	if (ImGui::Begin(rage_title.data(), &var.ui.b_rage, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
	{
		if (std::get<2>(var.global.cfg_mthread))
		{
			ImGui::PushFont(ImFontEx::header);
			//ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), adr_util::string::format(XorStr(ICON_FA_COGS "  Please wait  %c"), "|/-\\"[static_cast<int>(ImGui::GetTime() / 0.25f) & 3]).c_str());
			//decrypts(0)
			std::string format = ICON_FA_COGS;
			format += XorStr("  Please wait  %c");
			ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), adr_util::string::format(format, XorStr("|/-\\")[static_cast<int>(ImGui::GetTime() / 0.25f) & 3]).c_str());
			//encrypts(0)
			ImGui::PopFont();
			ImGui::End();
			return;
		}

		ImGui::Separator();
		ImGui::Dummy(ImVec2(-10, 0));
		ImGui::SameLine();
		//decrypts(0)
		ImGuiEx::Checkbox(XorStr("Enable##RAGE"), &var.ragebot.b_enabled, true);
		//encrypts(0)
		ImGui::Separator();

		if (!var.ragebot.b_enabled)
		{
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}

		static auto active_tab = 0;
		//decrypts(0)
		std::vector<const char*> tab_groups =
		{
			XorStr("AIMBOT"),
			XorStr("ANTIAIM")
		};
		auto f_tab_group_max_size = ImGui::GetWindowWidth() / 2;

		ImGui::Separator();
		ImGui::Dummy(ImVec2(-10, 0));
		ImGui::SameLine();
		ImGuiEx::RenderTabs(tab_groups, active_tab, f_tab_group_max_size, 15, true);
		ImGui::Separator();
		//encrypts(0)

		bool has_body = false, has_head = false;
		const bool is_hitscanning = var.ragebot.is_hitscanning(&has_body, &has_head);

		if (active_tab == 0)
		{
			//decrypts(0)
			ImGui::Columns(2, nullptr, false);
			{
				if (ImGui::BeginChild(XorStr("##RAGE_TAB1COL1"), ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar))
				{
					ImGui::PushFont(ImFontEx::header);
					ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Ragebot"));
					ImGui::PopFont();
					ImGui::Separator();
					ImGui::Spacing();

					ImGui::PushItemWidth(ImGui::GetWindowWidth() / 2);

					// -- body of child --

					//ImGui::Checkbox(XorStr("Strict Hitboxes"), &var.ragebot.b_strict_hitboxes);
					//ImGuiEx::SetTip(XorStr("Only fire if the predicted hitbox matches your picked hitboxes (May cause ragebot not to fire in certain situations)"));

					//decrypts(1)
					static std::vector<std::tuple<const char*, bool*, const char*>> hitboxes_ui =
					{
						{ XorStr("Head"),		&var.ragebot.hitscan_head.b_enabled,	nullptr },
						{ XorStr("Shoulders"),	&var.ragebot.hitscan_shoulders.b_enabled,	nullptr },
						{ XorStr("Hands"),		&var.ragebot.hitscan_hands.b_enabled,	nullptr },
						{ XorStr("Chest"),		&var.ragebot.hitscan_chest.b_enabled,	nullptr },
						//{ XorStr("Arms"),		&var.ragebot.hitscan_arms.b_enabled,	nullptr },
						{ XorStr("Stomach"),	&var.ragebot.hitscan_stomach.b_enabled, nullptr },
						{ XorStr("Thighs"),		&var.ragebot.hitscan_legs.b_enabled,	nullptr },
						{ XorStr("Feet"),		&var.ragebot.hitscan_feet.b_enabled,	nullptr },
					};

					ImGui::AlignTextToFramePadding();
					ImGui::Text(XorStr("Hitboxes:"));
					ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 12.f);
					ImGuiEx::SelectableCombo(XorStr("##RAGE_HITBOX"), hitboxes_ui);
					ImGuiEx::SetTip(XorStr("Choose what hitboxes you want the ragebot to attempt to target"));
					//encrypts(1)

					if (!is_hitscanning)
					{
						ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
						ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
					}

					//decrypts(1)
					std::vector<std::tuple<const char*, bool*, const char*>> multipoints_ui;
					if (var.ragebot.hitscan_head.b_enabled)
					{
						multipoints_ui.push_back(std::make_tuple(XorStr("Head"), &var.ragebot.hitscan_head.b_multipoint, nullptr));
					}
					//if (var.ragebot.hitscan_arms.b_enabled)
					//{
					//	multipoints_ui.push_back(std::make_tuple(XorStr("Arms"), &var.ragebot.hitscan_arms.b_multipoint, nullptr));
					//}
					//if (var.ragebot.hitscan_hands.b_enabled)
					//{
					//	multipoints_ui.push_back(std::make_tuple(XorStr("Hands"), &var.ragebot.hitscan_hands.b_multipoint, nullptr));
					//}
					if (var.ragebot.hitscan_chest.b_enabled)
					{
						multipoints_ui.push_back(std::make_tuple(XorStr("Chest"), &var.ragebot.hitscan_chest.b_multipoint, nullptr));
					}
					if (var.ragebot.hitscan_stomach.b_enabled)
					{
						multipoints_ui.push_back(std::make_tuple(XorStr("Stomach"), &var.ragebot.hitscan_stomach.b_multipoint, nullptr));
					}
					if (var.ragebot.hitscan_legs.b_enabled)
					{
						multipoints_ui.push_back(std::make_tuple(XorStr("Knees"), &var.ragebot.hitscan_legs.b_multipoint, nullptr));
					}
					//if (var.ragebot.hitscan_feet.b_enabled)
					//{
					//	multipoints_ui.push_back(std::make_tuple(XorStr("Feet"), &var.ragebot.hitscan_feet.b_multipoint, nullptr));
					//}

					// todo: nit; idk where the fuck to put this so it looks nice .-.

					ImGui::AlignTextToFramePadding();
					ImGui::Text(XorStr("Multipoint:"));
					ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 12.f);
					ImGuiEx::SelectableCombo(XorStr("##RAGE_MULTIPOINT"), multipoints_ui);
					ImGuiEx::SetTip(XorStr("Choose what hitboxes you'd like to multipoint for the ragebot"));
					//encrypts(1)

					ImGuiEx::Checkbox(XorStr("Use Individual Pointscales"), &var.ragebot.b_advanced_multipoint);
					ImGuiEx::SetTip(XorStr("Use individual pointscales for each selected hitbox rather than just shared head and body ones"));

					if (var.ragebot.hitscan_head.b_enabled && var.ragebot.hitscan_head.b_multipoint)
					{
						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Head Pointscale:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 12.f);
						ImGui::SliderFloat(XorStr("##RAGE_HEAD_PS"), &var.ragebot.hitscan_head.f_pointscale, 1.f, 100.f, XorStr("%.f%%"));
					}
					if (var.ragebot.hitscan_chest.b_enabled && var.ragebot.hitscan_chest.b_multipoint)
					{
						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Chest Pointscale:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 12.f);
						if (var.ragebot.b_advanced_multipoint)
							ImGui::SliderFloat(XorStr("##RAGE_CHEST_PS"), &var.ragebot.hitscan_chest.f_pointscale, 1.f, 100.f, XorStr("%.f%%"));
						else
							ImGui::SliderFloat(XorStr("##RAGE_BODY_PS"), &var.ragebot.hitscan_chest.f_pointscale, 1.f, 100.f, XorStr("%.f%%"));
					}
#if 0
					if (var.ragebot.hitscan_arms.b_enabled && var.ragebot.hitscan_arms.b_multipoint)
					{
						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Arms Pointscale:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 12.f);
						if (var.ragebot.b_advanced_multipoint)
							ImGui::SliderFloat(XorStr("##RAGE_ARMS_PS"), &var.ragebot.hitscan_arms.f_pointscale, 1.f, 100.f, XorStr("%.f%%"));
						else
							ImGui::SliderFloat(XorStr("##RAGE_BODY_PS"), &var.ragebot.hitscan_arms.f_pointscale, 1.f, 100.f, XorStr("%.f%%"));
					}
#endif
					if (var.ragebot.hitscan_stomach.b_enabled && var.ragebot.hitscan_stomach.b_multipoint)
					{
						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Stomach Pointscale:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 12.f);
						if (var.ragebot.b_advanced_multipoint)
							ImGui::SliderFloat(XorStr("##RAGE_STOMACH_PS"), &var.ragebot.hitscan_stomach.f_pointscale, 1.f, 100.f, XorStr("%.f%%"));
						else
							ImGui::SliderFloat(XorStr("##RAGE_BODY_PS"), &var.ragebot.hitscan_stomach.f_pointscale, 1.f, 100.f, XorStr("%.f%%"));
					}
#if 0
					if (var.ragebot.hitscan_legs.b_enabled && var.ragebot.hitscan_legs.b_multipoint)
					{
						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Legs Pointscale:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 12.f);
						if (var.ragebot.b_advanced_multipoint)
							ImGui::SliderFloat(XorStr("##RAGE_LEGS_PS"), &var.ragebot.hitscan_legs.f_pointscale, 1.f, 100.f, XorStr("%.f%%"));
						else
							ImGui::SliderFloat(XorStr("##RAGE_BODY_PS"), &var.ragebot.hitscan_legs.f_pointscale, 1.f, 100.f, XorStr("%.f%%"));
					}
					if (var.ragebot.hitscan_feet.b_enabled && var.ragebot.hitscan_feet.b_multipoint)
					{
						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Feet Pointscale:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 12.f);
						if (var.ragebot.b_advanced_multipoint)
							ImGui::SliderFloat(XorStr("##RAGE_FEET_PS"), &var.ragebot.hitscan_feet.f_pointscale, 1.f, 100.f, XorStr("%.f%%"));
						else
							ImGui::SliderFloat(XorStr("##RAGE_BODY_PS"), &var.ragebot.hitscan_feet.f_pointscale, 1.f, 100.f, XorStr("%.f%%"));
					}
#endif

					ImGui::Separator();
					ImGui::Spacing();

					ImGuiEx::Checkbox(XorStr("Autofire"), &var.ragebot.b_autofire);
					ImGuiEx::SetTip(XorStr("Automatically fire your current weapon when the minimum damage/hitchance match requirements"));
					ImGuiEx::Checkbox(XorStr("Autostop"), &var.ragebot.b_autostop);
					ImGuiEx::SetTip(XorStr("Automatically stop moving when the minimum damage/hitchance match requirements"));
					ImGuiEx::Checkbox(XorStr("Autoscope"), &var.ragebot.b_autoscope);
					ImGuiEx::SetTip(XorStr("Automatically scope your current weapon when the minimum damage/hitchance match requirements"));

					//ImGuiEx::Checkbox(XorStr("Alternative Pointscale"), &var.ragebot.b_use_alternative_multipoint);
					//ImGuiEx::SetTip(XorStr("This will automatic scale your points based on resolving")); //TICKBASE_FAKELAG_LIMIT

					ImGuiEx::Checkbox(XorStr("Safe Point"), &var.ragebot.b_safe_point);
					ImGuiEx::SetTip(XorStr("Only hitscan points that line up even when not resolved"));

					if (var.ragebot.b_safe_point)
					{
						ImGui::SameLine();
						ImGuiEx::Checkbox(XorStr("Include head"), &var.ragebot.b_safe_point_head);
						ImGuiEx::SetTip(XorStr("Only shoots at head if unresolved sides line up"));
					}

					ImGui::Separator();
					ImGui::Spacing();

					ImGuiEx::Checkbox(XorStr("Use Resolver"), &var.ragebot.b_resolver);
					ImGuiEx::SetTip(XorStr("Enable this to attempt to counter desync/fake-angles and reveal their real angles in order to properly aim at enemies"));


					if (var.ragebot.b_resolver)
					{
						//ImGuiEx::Checkbox(XorStr("Alternative Animation Method"), &var.ragebot.b_resolver_nov6build);
						//ImGuiEx::SetTip(XorStr("Attempt to resolve and use the enemy's choked eyeangles during animation"));
						ImGuiEx::Checkbox(XorStr("Moving Player Resolver"), &var.ragebot.b_resolver_moving);
						ImGuiEx::SetTip(XorStr("Enable this to enable different resolver logic when players are moving around"));
#if defined _DEBUG || defined INTERNAL_DEBUG
						ImGuiEx::Checkbox(XorStr("EXP Resolver"), &var.ragebot.b_resolver_experimental);
						ImGuiEx::SetTip(XorStr("Enable this to attempt to resolve any antiaim the normal resolver cannot"));
						if (var.ragebot.b_resolver_experimental)
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("ER Exclusion Leniency:"));
							ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 12.f);
							ImGui::SliderFloat(XorStr("##RAGE_BH_EL"), &var.ragebot.f_resolver_experimental_leniency_exclude, 1.0f, 8.0f, "%.2f");
							ImGuiEx::SetTip(XorStr("Distance away from the position that we exclude the target hitbox from."));

							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("ER Impact Leniency:"));
							ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 12.f);
							ImGui::SliderFloat(XorStr("##RAGE_BH_IL"), &var.ragebot.f_resolver_experimental_leniency_impact, 0.0f, 8.0f, "%.2f");
							ImGuiEx::SetTip(XorStr("Trace this distance further into the impact"));
						}
#endif
						ImGuiEx::Checkbox(XorStr("Disable Jitter Resolver"), &var.ragebot.b_resolver_nojitter.b_state, true, &var.ragebot.b_resolver_nojitter);
						ImGuiEx::SetTip(XorStr("Disables jitter detection while pressing this key. Temporary feature until player list is added"));

						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Flip Enemy Side Key:"));
						ImGui::SameLine(ImGui::GetWindowWidth() / 2);
						ImGuiEx::KeyBindButton(XorStr("##RAGE_RESOLVER_FLIPSIDES"), var.ragebot.i_flipenemy_key, false);
						ImGuiEx::SetTip(XorStr("Choose a button to press to flip the resolve side of the enemy you are looking at)"));
					}

					ImGuiEx::Checkbox(XorStr("Use Forwardtrack"), &var.ragebot.b_forwardtrack.b_state, true, &var.ragebot.b_forwardtrack);
					ImGuiEx::SetTip(XorStr("Allow the ragebot to try to predict players with invalid/manipulated lag records"));

					if (!is_hitscanning || !has_body)
					{
						ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
						ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
					}

					ImGui::PushFont(ImFontEx::header);
					ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Body-Aim"));
					ImGui::PopFont();
					ImGui::Separator();
					ImGui::Spacing();

					ImGuiEx::Checkbox(XorStr("Always Force"), &var.ragebot.baim_main.b_force.b_state, true, &var.ragebot.baim_main.b_force);
					ImGuiEx::SetTip(XorStr("Override any 'force baim' conditions and the resolver to only shoot at hittable chest, upper chest and stomach"));

					if (!var.ragebot.baim_main.b_force.get())
					{
						//decrypts(1)
						static std::vector<std::tuple<const char*, bool*, const char*>> baim_ui =
						{
							{ XorStr("After Head Misses"),	&var.ragebot.baim_main.b_after_misses,		XorStr("Force the ragebot to aim at the body after missing a specific amount of times on our target") },
							{ XorStr("Enemy Velocity"),		&var.ragebot.baim_main.b_moving_target,		XorStr("Force the ragebot to aim at the body if our target matches a minimum amount of speed") },
							{ XorStr("Airborne Enemies"),	&var.ragebot.baim_main.b_airborne_target,	XorStr("Force the ragebot to aim at the body if our target is in the air") },
							{ XorStr("We're Airborne"),		&var.ragebot.baim_main.b_airborne_local,	XorStr("Force the ragebot to aim at the body if we're in the air") },
							{ XorStr("Lethal Damage"),		&var.ragebot.baim_main.b_lethal,			XorStr("Force the ragebot to aim at the body if the target is hurt and health is less than the damage of your weapon") },
							{ XorStr("Health"),		&var.ragebot.baim_main.b_after_health,			XorStr("Force the ragebot to aim at the body if the target health is equal or under the given value") },

						};
						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Conditions:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 12.f);
						ImGuiEx::SelectableCombo(XorStr("##RAGE_BAIM"), baim_ui);
						ImGuiEx::SetTip(XorStr("Choose what conditions you want the ragebot to follow in order to force body-aim"));
						//encrypts(1)

						if (var.ragebot.baim_main.b_after_misses)
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Baim After Head Misses:"));
							ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 12.f);
							ImGui::SliderInt(XorStr("##RAGE_BAIM_MISS"), &var.ragebot.baim_main.i_after_misses, 1, 10, XorStr("%d misses"));
							ImGuiEx::SetTip(XorStr("Choose how many head misses it will take before aiming at the body"));
						}

						if (var.ragebot.baim_main.b_after_health)
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Baim Health:"));
							ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 12.f);
							ImGui::SliderFloat(XorStr("##RAGE_HEALTH_MISS"), &var.ragebot.baim_main.body_aim_health, 0.f, 100.f, XorStr("%0.1f health"));
							ImGuiEx::SetTip(XorStr("Choose the minimum health for body aim"));
						}


						if (var.ragebot.baim_main.b_moving_target)
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Baim Once Enemy Moves:"));
							ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 12.f);
							ImGui::SliderFloat(XorStr("##RAGE_BAIM_ENEMY_SPEED"), &var.ragebot.baim_main.f_moving_target, 0.f, 300.f, XorStr("%0.1f vel"));
							ImGuiEx::SetTip(XorStr("Choose how fast the enemy has to be moving before aiming at the body"));
						}
					}

					if (!is_hitscanning || !has_body)
					{
						ImGui::PopItemFlag();
						ImGui::PopStyleVar();
					}

					if (!is_hitscanning)
					{
						ImGui::PopItemFlag();
						ImGui::PopStyleVar();
					}

					ImGui::Separator();
					ImGui::Spacing();

					// -- end of child --

					ImGui::PopItemWidth();
					ImGui::EndChild();
				}
			}
			//encrypts(0)

			//decrypts(0)
			ImGui::NextColumn();
			{
				if (ImGui::BeginChild(XorStr("##RAGE_TAB1COL2"), ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize))
				{
					ImGui::PushFont(ImFontEx::header);
					ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Damage & Accuracy"));
					ImGui::PopFont();
					ImGui::Separator();
					ImGui::Spacing();

					ImGui::PushItemWidth(ImGui::GetWindowWidth() / 2);

					// -- body of child --

					if (!is_hitscanning)
					{
						ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
						ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
					}

					ImGui::AlignTextToFramePadding();
					ImGui::Text(XorStr("Min Visible Damage:"));
					ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 12.f);
					ImGui::SliderInt(XorStr("##RAGE_MINDMG"), &var.ragebot.i_mindmg, 1, 100, XorStr("%d dmg"));
					ImGuiEx::SetTip(XorStr("Select the minimum amount of damage required to use the ragebot"));

					ImGui::AlignTextToFramePadding();
					ImGui::Text(XorStr("Min Autowall Damage:"));
					ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 12.f);
					ImGui::SliderInt(XorStr("##RAGE_MINDMG_AW"), &var.ragebot.i_mindmg_aw, 1, 100, XorStr("%d dmg"));
					ImGuiEx::SetTip(XorStr("Select the minimum amount of damage through walls required to use the ragebot"));

					if (weapon_accuracy_nospread.GetVar()->GetInt() < 1)
					{
						if (!has_head)
						{
							ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
							ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Head Hitchance:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 12.f);
						ImGui::SliderFloat(XorStr("##RAGE_HITCHANCE_HEAD"), &var.ragebot.f_hitchance, 1.f, 100.f, XorStr("%.f%%"));
						ImGuiEx::SetTip(XorStr("Select the minimum percentage of possible hits to the head based on spread to use the ragebot"));

						if (!has_head)
						{
							ImGui::PopItemFlag();
							ImGui::PopStyleVar();
						}

						if (!has_body)
						{
							ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
							ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Body Hitchance:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 12.f);
						ImGui::SliderFloat(XorStr("##RAGE_HITCHANCE_BODY"), &var.ragebot.f_body_hitchance, 1.f, 100.f, XorStr("%.f%%"));
						ImGuiEx::SetTip(XorStr("Select the minimum percentage of possible hits to the body based on spread to use the ragebot"));

						if (!has_body)
						{
							ImGui::PopItemFlag();
							ImGui::PopStyleVar();
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Doubletap Hitchance:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 12.f);
						ImGui::SliderFloat(XorStr("##RAGE_HITCHANCE_DOUBLETAP"), &var.ragebot.f_doubletap_hitchance, 1.f, 100.f, XorStr("%.f%%"));
						ImGuiEx::SetTip(XorStr("Select the hitchance to use on the second shot when using doubletap"));

						ImGui::Separator();
						ImGui::Spacing();
					}

					ImGui::AlignTextToFramePadding();
					ImGui::Text(XorStr("Targets Per Tick:"));
					ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 12.f);
					ImGui::SliderInt(XorStr("##RAGE_TARGETS_PER_TICK"), &var.ragebot.i_targets_per_tick, 1, 32);
					ImGuiEx::SetTip(XorStr("Select the number of targets to scan per tick to help allievate low FPS"));

					ImGuiEx::Checkbox(XorStr("Scan Through Teammates"), &var.ragebot.b_scan_through_teammates);
					ImGuiEx::SetTip(XorStr("Allow the ragebot to scan through teammates for more accurate results"));

					ImGuiEx::Checkbox(XorStr("Ignore Limbs On Moving Targets"), &var.ragebot.b_ignore_limbs_if_moving);
					ImGuiEx::SetTip(XorStr("Ignore hands/feet if your target is moving to prevent the ragebot to prefer more vital spots"));
					if (!var.ragebot.b_ignore_limbs_if_moving)
					{
						ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
						ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
					}


					ImGui::AlignTextToFramePadding();
					ImGui::Text(XorStr("Speed:"));
					ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 12.f);
					ImGui::SliderFloat(XorStr("##RAGE_IGNORE_LIMBS_SPEED"), &var.ragebot.f_ignore_limbs_if_moving, 0.1f, 350.f, XorStr("%0.1f vel"));
					ImGuiEx::SetTip(XorStr("Choose how fast the enemy has to be moving before ignoring limbs"));

					if (!var.ragebot.b_ignore_limbs_if_moving)
					{
						ImGui::PopItemFlag();
						ImGui::PopStyleVar();
					}

					ImGui::Separator();
					ImGui::Spacing();

					ImGui::PushFont(ImFontEx::header);
					ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Exploits"));
					ImGui::PopFont();
					ImGui::Separator();
					ImGui::Spacing();

					if (!LocalPlayer.IsAllowedUntrusted())
					{
						ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
						ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
					}

					ImGuiEx::Checkbox(XorStr("Hide Shots"), &var.ragebot.exploits.b_hide_shots, true, nullptr, true, ImVec4(1.f, 1.f, 0.f, 1.f));
					ImGuiEx::SetTip(XorStr("Enable this to make it difficult to backtrack to your shots. Limits fakelag to 11")); //TICKBASE_FAKELAG_LIMIT

					ImGuiEx::Checkbox(XorStr("Hide Record"), &var.ragebot.exploits.b_hide_record, true, nullptr, true, ImVec4(1.f, 1.f, 0.f, 1.f));
					ImGuiEx::SetTip(XorStr("Makes it harder for enemies to hit you while moving. This feature is not recommended to use all the time. Incompatible with Multi-Tap")); //TICKBASE_FAKELAG_LIMIT

					ImGuiEx::Checkbox(XorStr("Multi-Tap"), &var.ragebot.exploits.b_multi_tap.b_state, true, &var.ragebot.exploits.b_multi_tap, true, ImVec4(1.f, 1.f, 0.f, 1.f));
					ImGuiEx::SetTip(XorStr("Attempt to force your weapon to fire multiple times at the same time. Only the first shot hides your real angles. Limits fakelag to 11")); //TICKBASE_FAKELAG_LIMIT

					ImGuiEx::Checkbox(XorStr("Disable Fakelag While Multi-Tapping"), &var.ragebot.exploits.b_disable_fakelag_while_multitapping, true, nullptr, true, ImVec4(1.f, 1.f, 0.f, 1.f));
					ImGuiEx::SetTip(XorStr("Limits the amount of fakelag to 1 while multi-tap is enabled to ensure fast firing"));

					ImGuiEx::Checkbox(XorStr("Teleport"), &var.ragebot.exploits.b_nasa_walk.b_state, true, &var.ragebot.exploits.b_nasa_walk, true, ImVec4(1.f, 1.f, 0.f, 1.f));
					ImGuiEx::SetTip(XorStr("Teleport yourself in short bursts; very useful for peeking"));

					ImGuiEx::Checkbox(XorStr("Fake-Lag On Peek"), &var.ragebot.b_fakelag_peek, true, nullptr, true, ImVec4(1.f, 1.f, 0.f, 1.f));
					ImGuiEx::SetTip(XorStr("Automatically fake lags when you are running around a corner. Disables hide shots/multitap/etc while peeking"));

					ImGui::AlignTextToFramePadding();
					ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), XorStr("Ticks To Shift:"));
					ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 12.f);
					ImGui::SliderInt(XorStr("##RAGE_SHIFT_AMOUNT"), &var.ragebot.exploits.i_ticks_to_wait, 1, MAX_USER_CMDS - 2, XorStr("%d ticks"));
					ImGuiEx::SetTip(XorStr("Select the amount of ticks to shift. Larger numbers make doubletap quicker. Higher amounts of fake lag will require you to lower this to use multi-tap properly"));

					if (!LocalPlayer.IsAllowedUntrusted())
					{
						ImGui::PopItemFlag();
						ImGui::PopStyleVar();
					}

					if (!is_hitscanning)
					{
						ImGui::PopItemFlag();
						ImGui::PopStyleVar();
					}

					// -- end of child --

					ImGui::PopItemWidth();
					ImGui::EndChild();
				}
			}
			//encrypts(0)

			ImGui::Columns(1);
		}
		else
		{
			//decrypts(0)
			if (ImGui::BeginChild(XorStr("##RAGE_TABTWO"), ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGuiEx::Checkbox(XorStr("Enable##ANTIAIM"), &var.ragebot.b_antiaim, true);
				ImGuiEx::SetTip(XorStr("Enable overall anti-aim in efforts to make it harder to kill you"));

				ImGui::Separator();

				if (!var.ragebot.b_antiaim)
				{
					ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
				}

				static auto active_hvh = 0;
				//decrypts(1)
				std::vector<const char*> active_hvh_groups =
				{
					XorStr("STANDING"),
					XorStr("MINWALKING"),
					XorStr("MOVING"),
					XorStr("AIRBORNE"),
				};

				static auto f_hvh_group_max_size = (ImGui::GetWindowWidth() - 14.f) / active_hvh_groups.size();

				ImGui::Dummy(ImVec2(-4, 0));
				ImGui::SameLine();
				ImGuiEx::RenderTabs(active_hvh_groups, active_hvh, f_hvh_group_max_size, 15, true);
				ImGui::Separator();
				//encrypts(1)

				if (ImGui::BeginChild(XorStr("##RAGE_HVH_CHILD"), ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize))
				{
					ImGui::PushItemWidth(-1);

					switch (active_hvh)
					{
						// -- STANDING --
						case 0:
						{
						ImGui::PushFont(ImFontEx::header);
						ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Fakelag"));
						ImGui::PopFont();
						ImGui::Separator();
						ImGui::Spacing();

						ImGuiEx::Checkbox(XorStr("Enable##HVH_FAKELAG_STAND_ENABLE"), &var.ragebot.standing.fakelag.b_enabled, true);
						ImGuiEx::SetTip(XorStr("Enable fakelag options while standing still"));

						if (!var.ragebot.standing.fakelag.b_enabled)
						{
							ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
							ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
						}
						//ImGui::AlignFirstTextHeightToWidgets();
						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Mode:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
						ImGui::Combo(XorStr("##HVH_FAKELAG_MODE"), &var.ragebot.standing.fakelag.i_mode, XorStrCT(" Factor\0 Step\0 Pingpong\0 Latency Seed\0 Random\0\0"));

						// factor or adaptive
						if (var.ragebot.standing.fakelag.i_mode == 0)
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Amount:"));
							ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
							ImGui::SliderInt(XorStr("##HVH_FAKELAG_STANDING_AMT"), &var.ragebot.standing.fakelag.i_static_ticks, 1, MAX_TICKS_TO_CHOKE, XorStr("%d ticks"));
							ImGuiEx::SetTip(XorStr("Choose a set amount of ticks to stop sending to emulate actual lag (while standing still)"));
						}
						else
						{
							// everything else

							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Min Amount:"));
							ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
							ImGui::SliderInt(XorStr("##HVH_FAKELAG_STANDING_MINAMT"), &var.ragebot.standing.fakelag.i_min_ticks, 1, MAX_TICKS_TO_CHOKE, XorStr("%d ticks"));
							ImGuiEx::SetTip(XorStr("Choose a set minimum amount of ticks to stop sending to emulate actual lag (while standing still)"));

							if (var.ragebot.standing.fakelag.i_min_ticks > var.ragebot.standing.fakelag.i_max_ticks)
								var.ragebot.standing.fakelag.i_min_ticks = var.ragebot.standing.fakelag.i_max_ticks;

							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Max Amount:"));
							ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
							ImGui::SliderInt(XorStr("##HVH_FAKELAG_STANDING_MAXAMT"), &var.ragebot.standing.fakelag.i_max_ticks, 1, MAX_TICKS_TO_CHOKE, XorStr("%d ticks"));
							ImGuiEx::SetTip(XorStr("Choose a set maximum amount of ticks to stop sending to emulate actual lag (while standing still)"));

							if (var.ragebot.standing.fakelag.i_max_ticks < var.ragebot.standing.fakelag.i_min_ticks)
								var.ragebot.standing.fakelag.i_max_ticks = var.ragebot.standing.fakelag.i_min_ticks;
						}

						// disruption
						ImGuiEx::Checkbox(XorStr("Apply Disruption##HVH_FAKELAG_STANDING_DISRUPT"), &var.ragebot.standing.fakelag.b_disrupt, true);
						ImGuiEx::SetTip(XorStr("Apply a chance of fluctuating the set amount of ticks to choke (while standing still)"));

						if (!var.ragebot.standing.fakelag.b_disrupt)
						{
							ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
							ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Disrupt Chance:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
						ImGui::SliderFloat(XorStr("##HVH_FAKELAG_STANDING_DISRUPT_CHANCE"), &var.ragebot.standing.fakelag.f_disrupt_chance, 1.f, 100.f, XorStr("%0.1f0"));

						if (!var.ragebot.standing.fakelag.b_disrupt)
						{
							ImGui::PopItemFlag();
							ImGui::PopStyleVar();
						}

						if (!var.ragebot.standing.fakelag.b_enabled)
						{
							ImGui::PopItemFlag();
							ImGui::PopStyleVar();
						}

						ImGui::Separator();
						ImGui::Spacing();

						ImGui::PushFont(ImFontEx::header);
						ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Angles"));
						ImGui::PopFont();
						ImGui::Separator();
						ImGui::Spacing();

						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Pitch:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
						ImGui::Combo(XorStr("##HVH_PITCH_STAND"), &var.ragebot.standing.i_pitch, XorStrCT(" None\0 Zero\0 Down\0 Up\0 Minimal\0 Random\0 Custom\0\0"));
						ImGuiEx::SetTip(XorStr("Choose which way you'd like your angles to show related to looking up or down (pitch) (while standing still)"));

						if (var.ragebot.standing.i_pitch == 6)
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Custom Pitch:"));
							ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
							ImGui::SliderFloat(XorStr("##HVH_PITCHCUSTOM_STAND"), &var.ragebot.standing.f_custom_pitch, -89.f, 89.f, XorStr("%0.1f0"));
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Yaw:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
						ImGui::Combo(XorStr("##HVH_YAW_STAND"), &var.ragebot.standing.i_yaw, XorStrCT(" None\0 Viewangle\0 At-Target\0\0"));
						ImGuiEx::SetTip(XorStr("Choose which way you'd like your angles to show related to looking left or right (yaw) (while standing still)"));

						if (var.ragebot.standing.i_yaw > 0)
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Yaw Offset:"));
							ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
							ImGui::SliderFloat(XorStr("##HVH_YAWOFFS_STAND"), &var.ragebot.standing.f_yaw_offset, 0.f, 360.f, XorStr("%0.1f0"));
							ImGuiEx::SetTip(XorStr("Choose how much you'd like to add on your set yaw (while standing still)"));

							ImGuiEx::Checkbox(XorStr("Jitter"), &var.ragebot.standing.b_jitter, true);
							ImGuiEx::SetTip(XorStr("Add on additional flicking back and forth upon your yaw (while standing still)"));
							if (!var.ragebot.standing.b_jitter)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}

							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Jitter Amount:"));
							ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
							ImGui::SliderFloat(XorStr("##HVH_YAWJIT_STAND"), &var.ragebot.standing.f_jitter, 0.f, 180.f, XorStr("%0.1f0"));
							ImGuiEx::SetTip(XorStr("Choose how much jitter you'd like to add on your set yaw (while standing still)"));

							if (!var.ragebot.standing.b_jitter)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}
						}

						ImGui::Separator();
						ImGui::Spacing();

						//ImGui::Checkbox(XorStr("Desync##DESYNC_STAND"), &var.ragebot.standing.b_desync);
						ImGuiEx::Checkbox(XorStr("Desync##DESYNC_STAND"), &var.ragebot.standing.b_desync, true);
						ImGuiEx::SetTip(XorStr("Enable an exploit to desync your networked angle away from your real (while standing still)"));

						ImGuiEx::Checkbox(XorStr("Break LBY by Flicking##DESYNC_TYPE_STAND"), &var.ragebot.standing.b_break_lby_by_flicking, true);
						ImGuiEx::SetTip(XorStr("Breaks LBY without triggering small player movements"));

						ImGuiEx::Checkbox(XorStr("Sway##DESYNC_STAND"), &var.ragebot.standing.b_apply_sway, true);
						ImGuiEx::SetTip(XorStr("Your real angle will sway back and forth (while standing still)"));

						if (var.ragebot.standing.b_apply_sway)
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Sway Speed:"));
							ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
							ImGui::SliderFloat(XorStr("##HVH_SWAYSPEED_STANDING"), &var.ragebot.standing.f_sway_speed, 0.1f, 50.f, XorStr("%0.1f"));
							ImGuiEx::SetTip(XorStr("Choose how fast your body sways"));

							ImGuiEx::Checkbox(XorStr("Sway Delay##DESYNC_SWAYDELAY_STANDING"), &var.ragebot.standing.b_sway_wait);
							ImGuiEx::SetTip(XorStr("Sway will wait after reaching a side"));

							if (var.ragebot.standing.b_sway_wait)
							{
								ImGui::AlignTextToFramePadding();
								ImGui::Text(XorStr("Sway Wait Min Time (Secs):"));
								ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
								ImGui::SliderFloat(XorStr("##HVH_SWAYWAITMIN_STANDING"), &var.ragebot.standing.f_sway_min, 0.01f, 5.f, XorStr("%0.1f"));

								ImGui::AlignTextToFramePadding();
								ImGui::Text(XorStr("Sway Wait Max Time (Secs):"));
								ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
								ImGui::SliderFloat(XorStr("##HVH_SWAYWAITMAX_STANDING"), &var.ragebot.standing.f_sway_max, 0.01f, 5.f, XorStr("%0.1f"));
							}
						}

						if (!var.ragebot.standing.b_desync)
						{
							ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
							ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
						}
						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Lean Direction:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
						ImGui::Combo(XorStr("##HVH_DESYNC_STAND"), &var.ragebot.standing.i_desync_style, XorStrCT(" Right\0 Left\0\0")); // todo: nit; add 'auto' to use anti-freestanding to determine side
						ImGuiEx::SetTip(XorStr("Choose which way you want the desynced angle to show (while standing still)"));

						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Desync Amount:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
						ImGui::SliderFloat(XorStr("##HVH_DESYNCAMT_STAND"), &var.ragebot.standing.f_desync_amt, -180.f, 180.f, XorStr("%0.1f0"));
						ImGuiEx::SetTip(XorStr("Choose how much you'd like to desync (while standing still)"));

						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Lower Body Delta:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
						ImGui::SliderFloat(XorStr("##HVH_LBYDELTA_STAND"), &var.ragebot.standing.f_lby_delta, 0.f, 180.f, XorStr("%0.1f0"));
						ImGuiEx::SetTip(XorStr("Choose how spread apart your legs should be from your torso (while standing still)"));
						if (!var.ragebot.standing.b_desync)
						{
							ImGui::PopItemFlag();
							ImGui::PopStyleVar();
						}
						break;
					}
					// -- MINWALKING --
						case 1:
						{
						ImGui::PushFont(ImFontEx::header);
						ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Fakelag"));
						ImGui::PopFont();
						ImGui::Separator();
						ImGui::Spacing();

						ImGuiEx::Checkbox(XorStr("Enable##HVH_FAKELAG_MW_ENABLE"), &var.ragebot.minwalking.fakelag.b_enabled, true);
						ImGuiEx::SetTip(XorStr("Enable fakelag options while minwalking"));

						if (!var.ragebot.minwalking.fakelag.b_enabled)
						{
							ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
							ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
						}
						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Mode:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
						ImGui::Combo(XorStr("##HVH_FAKELAG_MODE"), &var.ragebot.minwalking.fakelag.i_mode, XorStrCT(" Factor\0 Step\0 Adaptive\0 Pingpong\0 Latency Seed\0 Random\0\0"));

						// factor
						if (var.ragebot.minwalking.fakelag.i_mode == 0)
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Amount:"));
							ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
							ImGui::SliderInt(XorStr("##HVH_FAKELAG_MW_AMT"), &var.ragebot.minwalking.fakelag.i_static_ticks, 1, MAX_TICKS_TO_CHOKE, XorStr("%d ticks"));
							ImGuiEx::SetTip(XorStr("Choose a set amount of ticks to stop sending to emulate actual lag (while minwalking)"));
						}
						else
						{
							// everything else

							// adaptive doesn't have any options
							if (var.ragebot.minwalking.fakelag.i_mode != 2)
							{
								ImGui::AlignTextToFramePadding();
								ImGui::Text(XorStr("Min Amount:"));
								ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
								ImGui::SliderInt(XorStr("##HVH_FAKELAG_MW_MINAMT"), &var.ragebot.minwalking.fakelag.i_min_ticks, 1, MAX_TICKS_TO_CHOKE, XorStr("%d ticks"));
								ImGuiEx::SetTip(XorStr("Choose a set minimum amount of ticks to stop sending to emulate actual lag (while minwalking)"));

								if (var.ragebot.minwalking.fakelag.i_min_ticks > var.ragebot.minwalking.fakelag.i_max_ticks)
									var.ragebot.minwalking.fakelag.i_min_ticks = var.ragebot.minwalking.fakelag.i_max_ticks;

								ImGui::AlignTextToFramePadding();
								ImGui::Text(XorStr("Max Amount:"));
								ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
								ImGui::SliderInt(XorStr("##HVH_FAKELAG_MW_MAXAMT"), &var.ragebot.minwalking.fakelag.i_max_ticks, 1, MAX_TICKS_TO_CHOKE, XorStr("%d ticks"));
								ImGuiEx::SetTip(XorStr("Choose a set maximum amount of ticks to stop sending to emulate actual lag (while minwalking)"));

								if (var.ragebot.minwalking.fakelag.i_max_ticks < var.ragebot.minwalking.fakelag.i_min_ticks)
									var.ragebot.minwalking.fakelag.i_max_ticks = var.ragebot.minwalking.fakelag.i_min_ticks;
							}
						}

						// disruption
						ImGuiEx::Checkbox(XorStr("Apply Disruption##HVH_FAKELAG_MW_DISRUPT"), &var.ragebot.minwalking.fakelag.b_disrupt);
						ImGuiEx::SetTip(XorStr("Apply a chance of fluctuating the set amount of ticks to choke (while minwalking)"));
						if (!var.ragebot.minwalking.fakelag.b_disrupt)
						{
							ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
							ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Disrupt Chance:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
						ImGui::SliderFloat(XorStr("##HVH_FAKELAG_MW_DISRUPT_CHANCE"), &var.ragebot.minwalking.fakelag.f_disrupt_chance, 1.f, 100.f, XorStr("%0.1f0"));

						if (!var.ragebot.minwalking.fakelag.b_disrupt)
						{
							ImGui::PopItemFlag();
							ImGui::PopStyleVar();
						}

						if (!var.ragebot.minwalking.fakelag.b_enabled)
						{
							ImGui::PopItemFlag();
							ImGui::PopStyleVar();
						}

						ImGui::Separator();
						ImGui::Spacing();

						ImGui::PushFont(ImFontEx::header);
						ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Angles"));
						ImGui::PopFont();
						ImGui::Separator();
						ImGui::Spacing();

						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Pitch:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
						ImGui::Combo(XorStr("##HVH_PITCH_MW"), &var.ragebot.minwalking.i_pitch, XorStrCT(" None\0 Zero\0 Down\0 Up\0 Minimal\0 Random\0 Custom\0\0"));
						ImGuiEx::SetTip(XorStr("Choose which way you'd like your angles to show related to looking up or down (pitch) (while minwalking)"));

						if (var.ragebot.minwalking.i_pitch == 6)
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Custom Pitch:"));
							ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
							ImGui::SliderFloat(XorStr("##HVH_PITCHCUSTOM_MW"), &var.ragebot.minwalking.f_custom_pitch, -89.f, 89.f, XorStr("%0.1f0"));
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Yaw:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
						ImGui::Combo(XorStr("##HVH_YAW_MW"), &var.ragebot.minwalking.i_yaw, XorStrCT(" None\0 Viewangle\0 At-Target\0\0"));
						ImGuiEx::SetTip(XorStr("Choose which way you'd like your angles to show related to looking left or right (yaw) (while minwalking)"));

						if (var.ragebot.minwalking.i_yaw > 0)
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Yaw Offset:"));
							ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
							ImGui::SliderFloat(XorStr("##HVH_YAWOFFS_MW"), &var.ragebot.minwalking.f_yaw_offset, 0.f, 360.f, XorStr("%0.1f0"));
							ImGuiEx::SetTip(XorStr("Choose how much you'd like to add on your set yaw (while minwalking)"));

							ImGuiEx::Checkbox(XorStr("Jitter"), &var.ragebot.minwalking.b_jitter);
							ImGuiEx::SetTip(XorStr("Add on additional flicking back and forth upon your yaw (while minwalking)"));

							if (!var.ragebot.minwalking.b_jitter)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}

							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Jitter Amount:"));
							ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
							ImGui::SliderFloat(XorStr("##HVH_YAWJIT_MW"), &var.ragebot.minwalking.f_jitter, 0.f, 180.f, XorStr("%0.1f0"));
							ImGuiEx::SetTip(XorStr("Choose how much jitter you'd like to add on your set yaw (while minwalking)"));

							if (!var.ragebot.minwalking.b_jitter)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}
						}

						ImGui::Separator();
						ImGui::Spacing();

						ImGuiEx::Checkbox(XorStr("Desync##DESYNC_MW"), &var.ragebot.minwalking.b_desync);
						ImGuiEx::SetTip(XorStr("Enable an exploit to desync your networked angle away from your real (while minwalking)"));

						ImGuiEx::Checkbox(XorStr("Sway##DESYNC_MW"), &var.ragebot.minwalking.b_apply_sway, true);
						ImGuiEx::SetTip(XorStr("Your real angle will sway back and forth (while minwalking)"));

						if (var.ragebot.minwalking.b_apply_sway)
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Sway Speed:"));
							ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
							ImGui::SliderFloat(XorStr("##HVH_SWAYSPEED_MW"), &var.ragebot.minwalking.f_sway_speed, 0.1f, 50.f, XorStr("%0.1f"));
							ImGuiEx::SetTip(XorStr("Choose how fast your body sways"));

							ImGuiEx::Checkbox(XorStr("Sway Delay##DESYNC_SWAYDELAY_MW"), &var.ragebot.minwalking.b_sway_wait);
							ImGuiEx::SetTip(XorStr("Sway will wait after reaching a side"));

							if (var.ragebot.minwalking.b_sway_wait)
							{
								ImGui::AlignTextToFramePadding();
								ImGui::Text(XorStr("Sway Wait Min Time (Secs):"));
								ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
								ImGui::SliderFloat(XorStr("##HVH_SWAYWAITMIN_MINWALKING"), &var.ragebot.minwalking.f_sway_min, 0.01f, 5.f, XorStr("%0.1f"));

								ImGui::AlignTextToFramePadding();
								ImGui::Text(XorStr("Sway Wait Max Time (Secs):"));
								ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
								ImGui::SliderFloat(XorStr("##HVH_SWAYWAITMAX_MINWALKING"), &var.ragebot.minwalking.f_sway_max, 0.01f, 5.f, XorStr("%0.1f"));
							}
						}

						if (!var.ragebot.minwalking.b_desync)
						{
							ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
							ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
						}
						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Lean Direction:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
						ImGui::Combo(XorStr("##HVH_DESYNC_MW"), &var.ragebot.minwalking.i_desync_style, XorStrCT(" Right\0 Left\0"));
						ImGuiEx::SetTip(XorStr("Choose which way you want the desynced angle to show (while minwalking)"));

						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Desync Amount:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
						ImGui::SliderFloat(XorStr("##HVH_DESYNCAMT_MW"), &var.ragebot.minwalking.f_desync_amt, -180.f, 180.f, XorStr("%0.1f0"));
						ImGuiEx::SetTip(XorStr("Choose how much you'd like to desync (while minwalking)"));

						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Lower Body Delta:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
						ImGui::SliderFloat(XorStr("##HVH_LBYDELTA_MW"), &var.ragebot.minwalking.f_lby_delta, 0.f, 180.f, XorStr("%0.1f0"));
						ImGuiEx::SetTip(XorStr("Choose how spread apart your legs should be from your torso (while minwalking)"));

						if (!var.ragebot.minwalking.b_desync)
						{
							ImGui::PopItemFlag();
							ImGui::PopStyleVar();
						}

						break;
					}
					// -- MOVING --
						case 2:
						{
						ImGui::PushFont(ImFontEx::header);
						ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Fakelag"));
						ImGui::PopFont();
						ImGui::Separator();
						ImGui::Spacing();

						ImGuiEx::Checkbox(XorStr("Enable##HVH_FAKELAG_MOVING_ENABLE"), &var.ragebot.moving.fakelag.b_enabled);
						ImGuiEx::SetTip(XorStr("Enable fakelag options while moving"));

						if (!var.ragebot.moving.fakelag.b_enabled)
						{
							ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
							ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Mode:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
						ImGui::Combo(XorStr("##HVH_FAKELAG_MODE"), &var.ragebot.moving.fakelag.i_mode, XorStrCT(" Factor\0 Step\0 Adaptive\0 Pingpong\0 Latency Seed\0 Random\00"));

						// factor or adaptive
						if (var.ragebot.moving.fakelag.i_mode == 0)
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Amount:"));
							ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
							ImGui::SliderInt(XorStr("##HVH_FAKELAG_MOVING_AMT"), &var.ragebot.moving.fakelag.i_static_ticks, 1, MAX_TICKS_TO_CHOKE, XorStr("%d ticks"));
							ImGuiEx::SetTip(XorStr("Choose a set amount of ticks to stop sending to emulate actual lag (while moving)"));
						}
						else
						{
							// everything else

							if (var.ragebot.moving.fakelag.i_mode != 2)
							{
								ImGui::AlignTextToFramePadding();
								ImGui::Text(XorStr("Min Amount:"));
								ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
								ImGui::SliderInt(XorStr("##HVH_FAKELAG_MOVING_MINAMT"), &var.ragebot.moving.fakelag.i_min_ticks, 1, MAX_TICKS_TO_CHOKE, XorStr("%d ticks"));
								ImGuiEx::SetTip(XorStr("Choose a set minimum amount of ticks to stop sending to emulate actual lag (while moving)"));

								if (var.ragebot.moving.fakelag.i_min_ticks > var.ragebot.moving.fakelag.i_max_ticks)
									var.ragebot.moving.fakelag.i_min_ticks = var.ragebot.moving.fakelag.i_max_ticks;

								ImGui::AlignTextToFramePadding();
								ImGui::Text(XorStr("Max Amount:"));
								ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
								ImGui::SliderInt(XorStr("##HVH_FAKELAG_MOVING_MAXAMT"), &var.ragebot.moving.fakelag.i_max_ticks, 1, MAX_TICKS_TO_CHOKE, XorStr("%d ticks"));
								ImGuiEx::SetTip(XorStr("Choose a set maximum amount of ticks to stop sending to emulate actual lag (while moving)"));

								if (var.ragebot.moving.fakelag.i_max_ticks < var.ragebot.moving.fakelag.i_min_ticks)
									var.ragebot.moving.fakelag.i_max_ticks = var.ragebot.moving.fakelag.i_min_ticks;
							}
						}

						// disruption
						ImGuiEx::Checkbox(XorStr("Apply Disruption##HVH_FAKELAG_MOVING_DISRUPT"), &var.ragebot.moving.fakelag.b_disrupt);
						ImGuiEx::SetTip(XorStr("Apply a chance of fluctuating the set amount of ticks to choke (while moving)"));

						if (!var.ragebot.moving.fakelag.b_disrupt)
						{
							ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
							ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Disrupt Chance:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
						ImGui::SliderFloat(XorStr("##HVH_FAKELAG_MOVING_DISRUPT_CHANCE"), &var.ragebot.moving.fakelag.f_disrupt_chance, 1.f, 100.f, XorStr("%0.1f0"));

						if (!var.ragebot.moving.fakelag.b_disrupt)
						{
							ImGui::PopItemFlag();
							ImGui::PopStyleVar();
						}

						if (!var.ragebot.moving.fakelag.b_enabled)
						{
							ImGui::PopItemFlag();
							ImGui::PopStyleVar();
						}

						ImGui::Separator();
						ImGui::Spacing();

						ImGui::PushFont(ImFontEx::header);
						ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Angles"));
						ImGui::PopFont();
						ImGui::Separator();
						ImGui::Spacing();

						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Pitch:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
						ImGui::Combo(XorStr("##HVH_PITCH_MOVE"), &var.ragebot.moving.i_pitch, XorStrCT(" None\0 Zero\0 Down\0 Up\0 Minimal\0 Random\0 Custom\0"));
						ImGuiEx::SetTip(XorStr("Choose which way you'd like your angles to show related to looking up or down (pitch) (while moving)"));

						if (var.ragebot.moving.i_pitch == 6)
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Custom Pitch:"));
							ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
							ImGui::SliderFloat(XorStr("##HVH_PITCHCUSTOM_MOVE"), &var.ragebot.moving.f_custom_pitch, -89.f, 89.f, XorStr("%0.1f0"));
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Yaw:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
						ImGui::Combo(XorStr("##HVH_YAW_MOVE"), &var.ragebot.moving.i_yaw, XorStrCT(" None\0 Viewangle\0 At-Target\00"));
						ImGuiEx::SetTip(XorStr("Choose which way you'd like your angles to show related to looking left or right (yaw) (while moving)"));

						if (var.ragebot.moving.i_yaw > 0)
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Yaw Offset:"));
							ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
							ImGui::SliderFloat(XorStr("##HVH_YAWOFFS_MOVE"), &var.ragebot.moving.f_yaw_offset, 0.f, 360.f, XorStr("%0.1f0"));
							ImGuiEx::SetTip(XorStr("Choose how much you'd like to add on your set yaw (while moving)"));

							ImGuiEx::Checkbox(XorStr("Jitter"), &var.ragebot.moving.b_jitter);
							ImGuiEx::SetTip(XorStr("Add on additional flicking back and forth upon your yaw (while moving)"));

							if (!var.ragebot.moving.b_jitter)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}

							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Jitter Amount:"));
							ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
							ImGui::SliderFloat(XorStr("##HVH_YAWJIT_MOVE"), &var.ragebot.moving.f_jitter, 0.f, 180.f, XorStr("%0.1f0"));
							ImGuiEx::SetTip(XorStr("Choose how much jitter you'd like to add on your set yaw (while moving)"));

							if (!var.ragebot.moving.b_jitter)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}
						}

						ImGui::Separator();
						ImGui::Spacing();

						ImGuiEx::Checkbox(XorStr("Desync##DESYNC_MOVE"), &var.ragebot.moving.b_desync);
						ImGuiEx::SetTip(XorStr("Enable an exploit to desync your networked angle away from your real (while moving)"));

						ImGuiEx::Checkbox(XorStr("Sway##DESYNC_MOVE"), &var.ragebot.moving.b_apply_sway, true);
						ImGuiEx::SetTip(XorStr("Your real angle will sway back and forth (while moving)"));

						if (var.ragebot.moving.b_apply_sway)
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Sway Speed:"));
							ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
							ImGui::SliderFloat(XorStr("##HVH_SWAYSPEED_MOVING"), &var.ragebot.moving.f_sway_speed, 0.1f, 50.f, XorStr("%0.1f"));
							ImGuiEx::SetTip(XorStr("Choose how fast your body sways"));

							ImGuiEx::Checkbox(XorStr("Sway Delay##DESYNC_SWAYDELAY_MOVING"), &var.ragebot.moving.b_sway_wait);
							ImGuiEx::SetTip(XorStr("Sway will wait after reaching a side"));

							if (var.ragebot.moving.b_sway_wait)
							{
								ImGui::AlignTextToFramePadding();
								ImGui::Text(XorStr("Sway Wait Min Time (Secs):"));
								ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
								ImGui::SliderFloat(XorStr("##HVH_SWAYWAITMIN_MOVING"), &var.ragebot.moving.f_sway_min, 0.01f, 5.f, XorStr("%0.1f"));

								ImGui::AlignTextToFramePadding();
								ImGui::Text(XorStr("Sway Wait Max Time (Secs):"));
								ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
								ImGui::SliderFloat(XorStr("##HVH_SWAYWAITMAX_MOVING"), &var.ragebot.moving.f_sway_max, 0.01f, 5.f, XorStr("%0.1f"));
							}
						}

						if (!var.ragebot.moving.b_desync)
						{
							ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
							ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Lean Direction:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
						ImGui::Combo(XorStr("##HVH_DESYNC_MOVE"), &var.ragebot.moving.i_desync_style, XorStrCT(" Right\0 Left\0"));
						ImGuiEx::SetTip(XorStr("Choose which way you want the desynced angle to show (while moving)"));

						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Desync Amount:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
						ImGui::SliderFloat(XorStr("##HVH_DESYNCAMT_MOVING"), &var.ragebot.moving.f_desync_amt, -180.f, 180.f, XorStr("%0.1f0"));
						ImGuiEx::SetTip(XorStr("Choose how much you'd like to desync"));

						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Lower Body Delta:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
						ImGui::SliderFloat(XorStr("##HVH_LBYDELTA_MOVING"), &var.ragebot.moving.f_lby_delta, 0.f, 180.f, XorStr("%0.1f0"));
						ImGuiEx::SetTip(XorStr("Choose how spread apart your legs should be from your torso"));

						if (!var.ragebot.moving.b_desync)
						{
							ImGui::PopItemFlag();
							ImGui::PopStyleVar();
						}

						break;
					}
					// -- AIRBORNE --
						case 3:
						{
						ImGui::PushFont(ImFontEx::header);
						ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Fakelag"));
						ImGui::PopFont();
						ImGui::Separator();
						ImGui::Spacing();

						ImGuiEx::Checkbox(XorStr("Enable##HVH_FAKELAG_AIR_ENABLE"), &var.ragebot.in_air.fakelag.b_enabled);
						ImGuiEx::SetTip(XorStr("Enable fakelag options while in the air"));

						if (!var.ragebot.in_air.fakelag.b_enabled)
						{
							ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
							ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Mode:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
						ImGui::Combo(XorStr("##HVH_FAKELAG_MODE"), &var.ragebot.in_air.fakelag.i_mode, XorStrCT(" Factor\0 Step\0 Adaptive\0 Pingpong\0 Latency Seed\0 Random\00"));

						// factor/adaptive
						if (var.ragebot.in_air.fakelag.i_mode == 0)
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Amount:"));
							ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
							ImGui::SliderInt(XorStr("##HVH_FAKELAG_AIR_AMT"), &var.ragebot.in_air.fakelag.i_static_ticks, 1, MAX_TICKS_TO_CHOKE, XorStr("%d ticks"));
							ImGuiEx::SetTip(XorStr("Choose a set amount of ticks to stop sending to emulate actual lag (while in air)"));
						}
						else
						{
							// everything else
							if (var.ragebot.in_air.fakelag.i_mode != 2)
							{
								ImGui::AlignTextToFramePadding();
								ImGui::Text(XorStr("Min Amount:"));
								ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
								ImGui::SliderInt(XorStr("##HVH_FAKELAG_AIR_MINAMT"), &var.ragebot.in_air.fakelag.i_min_ticks, 1, MAX_TICKS_TO_CHOKE, XorStr("%d ticks"));
								ImGuiEx::SetTip(XorStr("Choose a set minimum amount of ticks to stop sending to emulate actual lag (while in air)"));

								if (var.ragebot.in_air.fakelag.i_min_ticks > var.ragebot.in_air.fakelag.i_max_ticks)
									var.ragebot.in_air.fakelag.i_min_ticks = var.ragebot.in_air.fakelag.i_max_ticks;

								ImGui::AlignTextToFramePadding();
								ImGui::Text(XorStr("Max Amount:"));
								ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
								ImGui::SliderInt(XorStr("##HVH_FAKELAG_AIR_MAXAMT"), &var.ragebot.in_air.fakelag.i_max_ticks, 1, MAX_TICKS_TO_CHOKE, XorStr("%d ticks"));
								ImGuiEx::SetTip(XorStr("Choose a set maximum amount of ticks to stop sending to emulate actual lag (while in air)"));

								if (var.ragebot.in_air.fakelag.i_max_ticks < var.ragebot.in_air.fakelag.i_min_ticks)
									var.ragebot.in_air.fakelag.i_max_ticks = var.ragebot.in_air.fakelag.i_min_ticks;
							}
						}

						// disruption
						ImGuiEx::Checkbox(XorStr("Apply Disruption##HVH_FAKELAG_AIR_DISRUPT"), &var.ragebot.in_air.fakelag.b_disrupt);
						ImGuiEx::SetTip(XorStr("Apply a chance of fluctuating the set amount of ticks to choke (while in air)"));

						if (!var.ragebot.in_air.fakelag.b_disrupt)
						{
							ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
							ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Disrupt Chance:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
						ImGui::SliderFloat(XorStr("##HVH_FAKELAG_AIR_DISRUPT_CHANCE"), &var.ragebot.in_air.fakelag.f_disrupt_chance, 1.f, 100.f, XorStr("%0.1f0"));

						if (!var.ragebot.in_air.fakelag.b_disrupt)
						{
							ImGui::PopItemFlag();
							ImGui::PopStyleVar();
						}

						if (!var.ragebot.in_air.fakelag.b_enabled)
						{
							ImGui::PopItemFlag();
							ImGui::PopStyleVar();
						}

						ImGui::Separator();
						ImGui::Spacing();

						ImGui::PushFont(ImFontEx::header);
						ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Angles"));
						ImGui::PopFont();
						ImGui::Separator();
						ImGui::Spacing();

						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Pitch:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
						ImGui::Combo(XorStr("##HVH_PITCH_AIR"), &var.ragebot.in_air.i_pitch, XorStrCT(" None\0 Zero\0 Down\0 Up\0 Minimal\0 Random\0 Custom\0"));
						ImGuiEx::SetTip(XorStr("Choose which way you'd like your angles to show related to looking up or down (pitch) (while in air)"));

						if (var.ragebot.in_air.i_pitch == 6)
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Custom Pitch:"));
							ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
							ImGui::SliderFloat(XorStr("##HVH_PITCHCUSTOM_MOVE"), &var.ragebot.in_air.f_custom_pitch, -89.f, 89.f, XorStr("%0.1f0"));
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Yaw:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
						ImGui::Combo(XorStr("##HVH_YAW_AIR"), &var.ragebot.in_air.i_yaw, XorStrCT(" None\0 Viewangle\0 At-Target\00"));
						ImGuiEx::SetTip(XorStr("Choose which way you'd like your angles to show related to looking left or right (yaw) (while in air)"));

						if (var.ragebot.in_air.i_yaw > 0)
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Yaw Offset:"));
							ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
							ImGui::SliderFloat(XorStr("##HVH_YAWOFFS_AIR"), &var.ragebot.in_air.f_yaw_offset, 0.f, 360.f, XorStr("%0.1f0"));
							ImGuiEx::SetTip(XorStr("Choose how much you'd like to add on your set yaw (while in air)"));

							ImGuiEx::Checkbox(XorStr("Jitter"), &var.ragebot.in_air.b_jitter);
							ImGuiEx::SetTip(XorStr("Add on additional flicking back and forth upon your yaw (while in air)"));

							if (!var.ragebot.in_air.b_jitter)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}

							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Jitter Amount:"));
							ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
							ImGui::SliderFloat(XorStr("##HVH_YAWJIT_AIR"), &var.ragebot.in_air.f_jitter, 0.f, 180.f, XorStr("%0.1f0"));
							ImGuiEx::SetTip(XorStr("Choose how much jitter you'd like to add on your set yaw (while in air)"));

							if (!var.ragebot.in_air.b_jitter)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}
						}

						ImGui::Separator();
						ImGui::Spacing();

						ImGuiEx::Checkbox(XorStr("Desync##DESYNC_AIR"), &var.ragebot.in_air.b_desync);
						ImGuiEx::SetTip(XorStr("Enable an exploit to desync your networked angle away from your real (while in air)"));

						ImGuiEx::Checkbox(XorStr("Sway##DESYNC_AIR"), &var.ragebot.in_air.b_apply_sway, true);
						ImGuiEx::SetTip(XorStr("Your real angle will sway back and forth (while in air)"));

						if (var.ragebot.in_air.b_apply_sway)
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(XorStr("Sway Speed:"));
							ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
							ImGui::SliderFloat(XorStr("##HVH_SWAYSPEED_AIR"), &var.ragebot.in_air.f_sway_speed, 0.1f, 50.f, XorStr("%0.1f"));
							ImGuiEx::SetTip(XorStr("Choose how fast your body sways"));

							ImGuiEx::Checkbox(XorStr("Sway Delay##DESYNC_SWAY_INAIR"), &var.ragebot.in_air.b_sway_wait);
							ImGuiEx::SetTip(XorStr("Sway will wait after reaching a side"));

							if (var.ragebot.in_air.b_sway_wait)
							{
								ImGui::AlignTextToFramePadding();
								ImGui::Text(XorStr("Sway Wait Min Time (Secs):"));
								ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
								ImGui::SliderFloat(XorStr("##HVH_SWAYWAITMIN_INAIR"), &var.ragebot.in_air.f_sway_min, 0.01f, 5.f, XorStr("%0.1f"));

								ImGui::AlignTextToFramePadding();
								ImGui::Text(XorStr("Sway Wait Max Time (Secs):"));
								ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
								ImGui::SliderFloat(XorStr("##HVH_SWAYWAITMAX_INAIR"), &var.ragebot.in_air.f_sway_max, 0.01f, 5.f, XorStr("%0.1f"));
							}
						}

						if (!var.ragebot.in_air.b_desync)
						{
							ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
							ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Lean Direction:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
						ImGui::Combo(XorStr("##HVH_DESYNC_AIR"), &var.ragebot.in_air.i_desync_style, XorStrCT(" Right\0 Left\0"));
						ImGuiEx::SetTip(XorStr("Choose which way you want the desynced angle to show (while in air)"));

						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Desync Amount:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
						ImGui::SliderFloat(XorStr("##HVH_DESYNCAMT_AIR"), &var.ragebot.in_air.f_desync_amt, -180.f, 180.f, XorStr("%0.1f0"));
						ImGuiEx::SetTip(XorStr("Choose how much you'd like to desync (while in air)"));

						ImGui::AlignTextToFramePadding();
						ImGui::Text(XorStr("Lower Body Delta:"));
						ImGui::SameLine((ImGui::GetWindowWidth() / 2) - 50.f);
						ImGui::SliderFloat(XorStr("##HVH_LBYDELTA_AIR"), &var.ragebot.in_air.f_lby_delta, 0.f, 180.f, XorStr("%0.1f0"));
						ImGuiEx::SetTip(XorStr("Choose how spread apart your legs should be from your torso (while in air)"));

						if (!var.ragebot.in_air.b_desync)
						{
							ImGui::PopItemFlag();
							ImGui::PopStyleVar();
						}

						break;
					}
						}

					ImGui::Spacing();
					ImGui::Separator();

					ImGui::PushFont(ImFontEx::header);
					ImGui::TextColored(ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f), XorStr("Manual Keys"));
					ImGui::PopFont();
					ImGui::Separator();
					ImGui::Spacing();

					ImGui::AlignTextToFramePadding();
					ImGui::Text(XorStr("Swap Lean Direction:"));
					ImGui::SameLine(ImGui::GetWindowWidth() / 2);
					ImGuiEx::KeyBindButton(XorStr("##HVH_MAN_AA_LEANDIR"), var.ragebot.i_manual_aa_lean_dir, false);
					ImGuiEx::SetTip(XorStr("Select a key to force your desync lean direction to the current opposite direction. (Applies in according to standing/moving/in-air)"));

					ImGui::AlignTextToFramePadding();
					ImGui::Text(XorStr("Manual AA - Left:"));
					ImGui::SameLine(ImGui::GetWindowWidth() / 2);
					ImGuiEx::KeyBindButton(XorStr("##HVH_MAN_AA_LEFT"), var.ragebot.i_manual_aa_left_key, false);
					ImGuiEx::SetTip(XorStr("Select a key to force your yaw to face to the left - Pressing it again will revert back to your set configuration"));

					ImGui::AlignTextToFramePadding();
					ImGui::Text(XorStr("Manual AA - Right:"));
					ImGui::SameLine(ImGui::GetWindowWidth() / 2);
					ImGuiEx::KeyBindButton(XorStr("##HVH_MAN_AA_RIGHT"), var.ragebot.i_manual_aa_right_key, false);
					ImGuiEx::SetTip(XorStr("Select a key to force your yaw to face to the right - Pressing it again will revert back to your set configuration"));

					ImGui::AlignTextToFramePadding();
					ImGui::Text(XorStr("Manual AA - Backwards:"));
					ImGui::SameLine(ImGui::GetWindowWidth() / 2);
					ImGuiEx::KeyBindButton(XorStr("##HVH_MAN_AA_BACK"), var.ragebot.i_manual_aa_back_key, false);
					ImGuiEx::SetTip(XorStr("Select a key to force your yaw to face backwards - Pressing it again will revert back to your set configuration"));

					ImGui::EndChild();
				}

				if (!var.ragebot.b_antiaim)
				{
					ImGui::PopItemFlag();
					ImGui::PopStyleVar();
				}

				ImGui::EndChild();
			}
			//encrypts(0)
		}

		if (!var.ragebot.b_enabled)
		{
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}

		ImGui::End();
	}
}

void ui::render()
{
	auto& var = variable::get();
	//ImGui::GetIO().MouseDrawCursor = b_visible;
	ImGui::GetIO().WantCaptureKeyboard = !b_visible;
	ImGui::GetIO().WantCaptureMouse = !b_visible;

	auto f_speed = adr_math::my_super_clamp(var.ui.f_menu_time, 0.1f, 1.f);
	if (f_speed != 1.f)
	{
		static auto fl_alpha = 0.f;
		auto& _style = ImGui::GetStyle();
		if (b_visible)
		{
			fl_alpha = adr_math::my_super_clamp(fl_alpha + f_speed / 10.f, 0.f, 1.f);
			_style.Alpha = adr_math::my_super_clamp(fl_alpha + 0.00001f, 0.f, 1.f);
			ImGui::SetNextWindowPos(ImVec2(-140 + fl_alpha * 140, 0));
		}
		else
		{
			static auto fl_reverse = 0.f;
			if (fl_alpha > 0.f)
			{
				fl_reverse = adr_math::my_super_clamp(fl_reverse + f_speed / 150.f, 0.f, 1.f);
				fl_alpha -= fl_reverse;
				fl_alpha = adr_math::my_super_clamp(fl_alpha, 0.f, 1.f);
				_style.Alpha = adr_math::my_super_clamp(fl_alpha + 0.00001f, 0.f, 1.f);
				ImGui::SetNextWindowPos(ImVec2(fl_alpha * 140 - 140, 0));
				ImGui::CloseCurrentPopup();
			}
			else
			{
				fl_reverse = 0.f;
				_style.Alpha = 1.f;
				ImGui::SetNextWindowPos(ImVec2(-140, 0));
				return;
			}
		}
	}
	else
	{
		auto& _style = ImGui::GetStyle();
		if (b_visible)
		{
			_style.Alpha = 1.f;
			ImGui::SetNextWindowPos(ImVec2(0, 0));
		}
		else
		{
			return;
		}
	}

	ImVec2 vec_main_pos;
	auto i_screenx = 0, i_screeny = 0;
	Interfaces::EngineClient->GetScreenSize(i_screenx, i_screeny);

	auto _viewport = render::get().get_viewport();
	if (_viewport.Width > 0)
		f_scale = /*i_client_width <= 1920 ? 1.f :*/ _viewport.Width / 1920.f;
	else
		f_scale = 1.f;

	//decrypts(0)
	auto main_title = XorStr("##MAIN");
	//encrypts(0)

	if (ImGui::Begin(main_title, &b_visible, ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoTitleBar))
	{
		ImGui::SetWindowSize(ImVec2(140, i_screeny));
		const auto i_middle = i_screeny / 2;
		const auto i_pad_top = i_middle / 4.f;

		ImGui::Dummy(ImVec2(-1, i_pad_top * f_scale));
		ImGui::Dummy(ImVec2(-10, -1));
		ImGui::SameLine();
		ImGui::Image(dxt_logo, ImVec2(125, 95));
		ImGui::Dummy(ImVec2(-1, i_pad_top * f_scale));

		//decrypts(0)
		ImGui::PushFont(ImFontEx::tab);
		{
			std::string tab_rage = ICON_FA_CROSSHAIRS;
			tab_rage += XorStr("  RAGE");

#ifdef INCLUDE_LEGIT
			std::string tab_legit = ICON_FA_GAMEPAD;
			tab_legit += XorStr("  LEGIT");
#endif

			std::string tab_visuals = ICON_FA_EYE;
			tab_visuals += XorStr("  VISUALS");

			std::string tab_playerlist = ICON_FA_USER;
			tab_playerlist += XorStr("  PLAYERS");

			std::string tab_misc = ICON_FA_USER;
			tab_misc += XorStr("  MISC");

			std::string tab_waypoints = ICON_FA_STREET_VIEW;
			tab_waypoints += XorStr("  WAYPOINTS");

			std::string tab_skin = ICON_FA_PAINT_BRUSH;
			tab_skin += XorStr("  SKIN");

			std::string tab_config = ICON_FA_COGS;
			tab_config += XorStr("  CONFIG");

			// todo: nit; change this hacky logic to use tabs
			if (var.ui.b_only_one) // FUCKING GHETTO, WHATEVER
			{
				std::vector<const char*> vec_tab = { tab_rage.data(), tab_visuals.data(), tab_playerlist.data(), tab_misc.data(), tab_waypoints.data(), tab_skin.data(), tab_config.data() };


				static int i_active_tab = TAB_INVALID;

				if (!var.ui.b_visual && i_active_tab == TAB_VISUALS)
					i_active_tab = TAB_INVALID;
				else if (!var.ui.b_playerlist && i_active_tab == TAB_PLAYERS)
					i_active_tab = TAB_INVALID;
				else if (!var.ui.b_rage && i_active_tab == TAB_RAGE)
					i_active_tab = TAB_INVALID;
				else if (!var.ui.b_misc && i_active_tab == TAB_MISC)
					i_active_tab = TAB_INVALID;
				else if (!var.ui.b_waypoints && i_active_tab == TAB_WAYPOINTS)
				{
					i_active_tab = TAB_INVALID;
					var.waypoints.ResetSelectedInformation();
				}
				else if (!var.ui.b_skin && i_active_tab == TAB_SKIN)
					i_active_tab = TAB_INVALID;
				else if (!var.ui.b_config && i_active_tab == TAB_CONFIG)
					i_active_tab = TAB_INVALID;

				ImGuiEx::RenderTabs(vec_tab, i_active_tab, 115.f, 40.f, false);

				if (i_active_tab == TAB_VISUALS)
				{
					var.ui.b_visual = true;
					var.ui.b_playerlist = false;
					var.ui.b_legit = false;
					var.ui.b_rage = false;
					var.ui.b_misc = false;
					var.ui.b_waypoints = false;
					var.ui.b_skin = false;
					var.ui.b_config = false;
					var.ui.b_waypoints = false;
					var.waypoints.ResetSelectedInformation();
				}
				else if (i_active_tab == TAB_PLAYERS)
				{
					var.ui.b_visual = false;
					var.ui.b_playerlist = true;
					var.ui.b_legit = false;
					var.ui.b_rage = false;
					var.ui.b_misc = false;
					var.ui.b_skin = false;
					var.ui.b_config = false;
					var.ui.b_waypoints = false;
					var.waypoints.ResetSelectedInformation();
				}
				else if (i_active_tab == TAB_RAGE)
				{
					var.ui.b_visual = false;
					var.ui.b_playerlist = false;
					var.ui.b_legit = false;
					var.ui.b_rage = true;
					var.ui.b_misc = false;
					var.ui.b_skin = false;
					var.ui.b_config = false;
					var.ui.b_waypoints = false;
					var.waypoints.ResetSelectedInformation();
				}
				else if (i_active_tab == TAB_MISC)
				{
					var.ui.b_visual = false;
					var.ui.b_playerlist = false;
					var.ui.b_legit = false;
					var.ui.b_rage = false;
					var.ui.b_misc = true;
					var.ui.b_skin = false;
					var.ui.b_config = false;
					var.ui.b_waypoints = false;
					var.waypoints.ResetSelectedInformation();
				}
				else if (i_active_tab == TAB_WAYPOINTS)
				{
					var.ui.b_visual = false;
					var.ui.b_playerlist = false;
					var.ui.b_legit = false;
					var.ui.b_rage = false;
					var.ui.b_misc = false;
					var.ui.b_skin = false;
					var.ui.b_config = false;
					var.ui.b_waypoints = true;
				}
				else if (i_active_tab == TAB_SKIN)
				{
					var.ui.b_visual = false;
					var.ui.b_playerlist = false;
					var.ui.b_legit = false;
					var.ui.b_rage = false;
					var.ui.b_misc = false;
					var.ui.b_skin = true;
					var.ui.b_config = false;
					var.ui.b_waypoints = false;
					var.waypoints.ResetSelectedInformation();
				}
				else if (i_active_tab == TAB_CONFIG)
				{
					var.ui.b_visual = false;
					var.ui.b_playerlist = false;
					var.ui.b_legit = false;
					var.ui.b_rage = false;
					var.ui.b_misc = false;
					var.ui.b_skin = false;
					var.ui.b_config = true;
					var.ui.b_waypoints = false;
					var.waypoints.ResetSelectedInformation();
				}
				else if (i_active_tab == TAB_INVALID)
				{
					var.ui.b_visual = false;
					var.ui.b_playerlist = false;
					var.ui.b_legit = false;
					var.ui.b_rage = false;
					var.ui.b_misc = false;
					var.ui.b_skin = false;
					var.ui.b_config = false;
					var.ui.b_waypoints = false;
					var.waypoints.ResetSelectedInformation();
				}
			}
			else
			{
				ImGuiEx::SelectableCenter(tab_rage.data(), &var.ui.b_rage, 0, ImVec2{ 115, 40 });
				ImGuiEx::SelectableCenter(tab_visuals.data(), &var.ui.b_visual, 0, ImVec2{ 115, 40 });
				ImGuiEx::SelectableCenter(tab_playerlist.data(), &var.ui.b_playerlist, 0, ImVec2{ 115, 40 });
				ImGuiEx::SelectableCenter(tab_misc.data(), &var.ui.b_misc, 0, ImVec2{ 115, 40 });
				ImGuiEx::SelectableCenter(tab_waypoints.data(), &var.ui.b_waypoints, 0, ImVec2{ 115, 40 });
				ImGuiEx::SelectableCenter(tab_skin.data(), &var.ui.b_skin, 0, ImVec2{ 115, 40 });
				ImGuiEx::SelectableCenter(tab_config.data(), &var.ui.b_config, 0, ImVec2{ 115, 40 });
			}
		}
		//encrypts(0)
		ImGui::PopFont();

		//decrypts(0)
		ImGui::Dummy(ImVec2(-1, i_pad_top * f_scale));
		ImGui::TextColored({ 1.f, 1.f, 1.f, 1.f }, XorStr("User: "));
		ImGui::SameLine();

		auto str_user = var.user.str_user;
		if (str_user.length() > 10)
		{
			str_user = str_user.substr(7);
			str_user += XorStr("...");
		}

		ImGui::TextColored({ 1.f, 0.29f, 0.05f, 1.f }, "%s", str_user.c_str());
		ImGui::TextColored({ 1.f, 1.f, 1.f, 1.f }, XorStr("Server:"));
		ImGui::SameLine();
		ImGui::TextColored({ 0.f, 1.f, 0.f, 1.f }, "%s", adr_util::string::format(XorStr("ON  %c"), XorStr("|/-\\")[static_cast<int>(ImGui::GetTime() / 0.25f) & 3]).c_str());
		ImGui::TextColored({ 1.f, 1.f, 1.f, 1.f }, XorStr("Framerate:"));
		ImGui::SameLine();
		ImGui::TextColored({ 1.f, 0.f, 0.f, 1.f }, "%s", adr_util::string::format(XorStr("%.2f FPS"), ImGui::GetIO().Framerate).c_str());

		ImGui::Checkbox(XorStr("Only One Tab"), &var.ui.b_only_one);
		ImGui::Checkbox(XorStr("Stream Proof"), &var.ui.b_stream_proof);

		//todo: nit/shark/phyz; uncomment when unload works
		//if (ImGui::Button(XorStr("Unload"), ImVec2(-1, 20)))
		//	var.ui.b_unload = true;

		vec_main_pos = ImGui::GetWindowPos();
		ImGui::End();
		//encrypts(0)
	}

	const auto col_controller = var.ui.col_controller.color().ToImGUI();
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 0.9f));
	{
		//decrypts(0)
		if (ImGui::Begin(XorStr("##MAINBORDER"), &b_visible, ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoTitleBar))
		{
			ImGui::SetWindowPos(ImVec2(vec_main_pos.x, 0));
			ImGui::SetWindowSize(ImVec2(144, i_screeny));
			ImGui::End();
		}
		//encrypts(0)
		ImGui::PopStyleColor();
	}

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.15f, 0.65f));
	{
		//decrypts(0)
		if (ImGui::Begin(XorStr("##BACKGROUND"), &b_visible, ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoTitleBar))
		{
			ImGui::SetWindowPos(ImVec2(0, 0));
			ImGui::SetWindowSize(ImVec2(i_screenx, i_screeny));
			ImGui::End();
		}
		//encrypts(0)
		ImGui::PopStyleColor();
	}

	if (var.ui.b_rage)
		rage();
#ifdef INCLUDE_LEGIT
	if (var.ui.b_legit)
		legit();
#endif
	if (var.ui.b_visual)
		visual();

	if (var.ui.b_playerlist)
		playerlist();

	if (var.ui.b_misc)
		misc();

	if (var.ui.b_waypoints)
		waypoints();
	else
		var.waypoints.ResetSelectedInformation();

	if (var.ui.b_skin)
		skin();

	if (var.ui.b_config)
		config();

	set_style();
}

void ui::show()
{
	b_visible = true;
}

void ui::hide()
{
	if (b_visible)
	{
		auto& var = variable::get();
		var.waypoints.ResetSelectedInformation();
	}
	b_visible = false;
}

void ui::toggle()
{
	if (b_visible)
	{
		auto& var = variable::get();
		var.waypoints.ResetSelectedInformation();
	}
	b_visible = !b_visible;
}

float ui::get_scale() const
{
	return f_scale;
}

void ui::update_objects(IDirect3DDevice9* p_device)
{
	D3DDEVICE_CREATION_PARAMETERS cparams;
	p_device->GetCreationParameters(&cparams);

	RECT rect;
	GetWindowRect(cparams.hFocusWindow, &rect);
	auto i_client_width = rect.right - rect.left;
	f_scale = /*i_client_width <= 1920 ? 1.f :*/ i_client_width / 1920.f;

	create_fonts();
	set_style();
}

void ui::create_fonts() const
{
	static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
	static const ImWchar weapons_ranges[] = { 0xE000, 0xE20C, 0 };

	ImFontConfig normal_config;

	ImFontConfig icons_config;
	icons_config.MergeMode = true;
	icons_config.PixelSnapH = true;

	ImFontConfig symbol_cfg;
	symbol_cfg.MergeMode = true;

	ImFontConfig csgo_config;
	csgo_config.PixelSnapH = true;

	ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(ruda_compressed_data_base85, 12 /** f_scale*/, &normal_config, ImGui::GetIO().Fonts->GetOnlyGlyphBasicRanges());
	ImGui::GetIO().Fonts->AddFontFromMemoryTTF(ImGuiEx::CopyFont(font_awesome_data, font_awesome_size), font_awesome_size, 14 /** f_scale*/, &icons_config, icons_ranges);
	//decrypts(0)
	ImGui::GetIO().Fonts->AddFontFromFileTTF(std::string(adr_util::get_disk() + XorStr("Windows\\Fonts\\l_10646.ttf")).c_str(), 12 /** f_scale*/, &symbol_cfg, ImGui::GetIO().Fonts->GetOnlyGlyphRangesCyrillic());
	//encrypts(0)

	ImFontEx::header = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(ruda_compressed_data_base85, 13 /** f_scale*/);
	ImFontEx::header = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(ImGuiEx::CopyFont(font_awesome_data, font_awesome_size), font_awesome_size, 14 /** f_scale*/, &icons_config, icons_ranges);

	ImFontEx::checkbox = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(ruda_compressed_data_base85, 12 /** f_scale*/);
	ImFontEx::checkbox = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(ImGuiEx::CopyFont(font_awesome_data, font_awesome_size), font_awesome_size, 17 /** f_scale*/, &icons_config, icons_ranges);

	ImFontEx::tab = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(ruda_compressed_data_base85, 20 /** f_scale*/);
	ImFontEx::tab = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(ImGuiEx::CopyFont(font_awesome_data, font_awesome_size), font_awesome_size, 20 /** f_scale*/, &icons_config, icons_ranges);

	ImFontEx::weapons = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(csgo_icons_compressed_data_base85, 18 /** f_scale*/, &csgo_config, weapons_ranges);

	ImFontEx::smallest = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(smallest_pixel_compressed_data_base85, 10 /** f_scale*/);
	ImFontEx::smallest = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(ImGuiEx::CopyFont(font_awesome_data, font_awesome_size), font_awesome_size, 12 /** f_scale*/, &icons_config, icons_ranges);
}

void ui::initialize(IDirect3DDevice9* p_device)
{
	D3DDEVICE_CREATION_PARAMETERS cparams;
	p_device->GetCreationParameters(&cparams);

	b_visible = false;

	ImGui::CreateContext();

	auto& io = ImGui::GetIO();
	//decrypts(0)
	io.IniFilename = XorStr("m1b.ini");
	//encrypts(0)

	ImGui_ImplWin32_Init(cparams.hFocusWindow);
	ImGui_ImplDX9_Init(p_device);

	D3DXCreateTextureFromFileInMemory(p_device, _logo_data, _logo_size, &dxt_logo);

	// pre create and bypass UAC meme agaisnt .ini
	/*auto str_path = std::string(util::get_disk() + "clientdata\\abs\\ui.ini");
	std::ofstream ofs_file;
	ofs_file.open(str_path.c_str(), std::ios::out | std::ios::trunc);
	ofs_file << "";
	ofs_file.close();
	auto& io = ImGui::GetIO();
	io.IniFilename = str_path.c_str();*/

	update_objects(p_device);

	b_amd = IsAMD();
}

void ui::set_style() const
{
	auto& _style = ImGui::GetStyle();
	_style.Alpha = 1.f;									// Global alpha applies to everything in ImGui.
	_style.WindowPadding = ImVec2(12, 12);				// Padding within a window.
	_style.WindowRounding = 0.f;						// Radius of window corners rounding. Set to 0.0f to have rectangular windows.
	_style.WindowMinSize = ImVec2(100, 10);				// Minimum window size. This is a global setting. If you want to constraint individual windows, use SetNextWindowSizeConstraints().
	_style.WindowTitleAlign = ImVec2(0.5f, 0.5f);		// Alignment for title bar text. Defaults to (0.0f,0.5f) for left-aligned,vertically centered.
	_style.WindowBorderSize = 0.f;						// Thickness of border around windows. Generally set to 0.0f or 1.0f. (Other values are not well tested and more CPU/GPU costly).
	_style.ChildRounding = 0.f;							// Radius of child window corners rounding. Set to 0.0f to have rectangular windows.
	_style.ChildBorderSize = 0.f;						// Thickness of border around child windows. Generally set to 0.0f or 1.0f. (Other values are not well tested and more CPU/GPU costly).
	_style.PopupRounding = 0.f;							// Radius of popup window corners rounding.
	_style.PopupBorderSize = 0.f;						// Thickness of border around popup windows. Generally set to 0.0f or 1.0f. (Other values are not well tested and more CPU/GPU costly).	
	_style.FramePadding = ImVec2(4, 4);					// Padding within a framed rectangle (used by most widgets).
	_style.FrameRounding = 1.f;							// Radius of frame corners rounding. Set to 0.0f to have rectangular frame (used by most widgets).
	_style.FrameBorderSize = 0.f;						// Thickness of border around frames. Generally set to 0.0f or 1.0f. (Other values are not well tested and more CPU/GPU costly).
	_style.ItemSpacing = ImVec2(5, 7);					// Horizontal and vertical spacing between widgets/lines.
	_style.ItemInnerSpacing = ImVec2(6, 4);				// Horizontal and vertical spacing between within elements of a composed widget (e.g. a slider and its label).
	_style.TouchExtraPadding = ImVec2(0, 0);			// Expand reactive bounding box for touch-based system where touch position is not accurate enough. Unfortunately we don't sort widgets so priority on overlap will always be given to the first widget. So don't grow this too much!
	_style.IndentSpacing = 25.f;						// Horizontal indentation when e.g. entering a tree node. Generally == (FontSize + FramePadding.x*2).
	_style.ColumnsMinSpacing = 6.f;						// Minimum horizontal spacing between two columns.
	_style.ScrollbarSize = 11.f;						// Width of the vertical scrollbar, Height of the horizontal scrollbar.
	_style.ScrollbarRounding = 12.f;					// Radius of grab corners for scrollbar.
	_style.GrabMinSize = 10.f;							// Minimum width/height of a grab box for slider/scrollbar.
	_style.GrabRounding = 0.f;							// Radius of grabs corners rounding. Set to 0.0f to have rectangular slider grabs.
	_style.ButtonTextAlign = ImVec2(0.5f, 0.5f);		// Alignment of button text when button is larger than text. Defaults to (0.5f,0.5f) for horizontally+vertically centered.
	_style.DisplayWindowPadding = ImVec2(22, 22);		// Window positions are clamped to be visible within the display area by at least this amount. Only covers regular windows.
	_style.DisplaySafeAreaPadding = ImVec2(4, 4);		// If you cannot see the edges of your screen (e.g. on a TV) increase the safe area padding. Apply to popups/tooltips as well regular windows. NB: Prefer configuring your TV sets correctly!
	_style.MouseCursorScale = f_scale;					// Scale software rendered mouse cursor (when io.MouseDrawCursor is enabled). May be removed later.
	_style.AntiAliasedLines = true;						// Enable anti-aliasing on lines/borders. Disable if you are really tight on CPU/GPU.
	_style.AntiAliasedFill = true;						// Enable anti-aliasing on filled shapes (rounded rectangles, circles, etc.)
	_style.CurveTessellationTol = 1.25f;				// Tessellation tolerance when using PathBezierCurveTo() without a specific number of segments. Decrease for highly tessellated curves (higher quality, more polygons), increase to reduce quality.

	const auto col_controller = variable::get().ui.col_controller.color().ToImGUI();
	const auto col_text = variable::get().ui.col_text.color().ToImGUI();
	const auto col_background = variable::get().ui.col_background.color().ToImGUI();

	_style.Colors[ImGuiCol_Header] = ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, col_controller.Value.w);
	_style.Colors[ImGuiCol_HeaderHovered] = ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 0.5f);
	_style.Colors[ImGuiCol_HeaderActive] = ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 0.5f);
	//_style.Colors[ImGuiCol_CloseButton] = ImVec4(0.70f, 0.70f, 0.70f, 0.4f); // NOT IN USE
	//_style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(1.f, 0.29f, 0.05f, 1.f); // NOT IN USE
	//_style.Colors[ImGuiCol_CloseButtonActive] = ImVec4(1.f, 0.29f, 0.05f, 1.f); // NOT IN USE
	_style.Colors[ImGuiCol_TitleBg] = ImVec4(col_background.Value.x * 0.17f, col_background.Value.y * 0.17f, col_background.Value.z * 0.17f, 1.f);
	_style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.75f); // NOT IN USE
	_style.Colors[ImGuiCol_TitleBgActive] = ImVec4(col_background.Value.x * 0.17f, col_background.Value.y * 0.17f, col_background.Value.z * 0.17f, 1.f);
	_style.Colors[ImGuiCol_Text] = ImVec4(col_text.Value.x, col_text.Value.y, col_text.Value.z, col_text.Value.w);
	_style.Colors[ImGuiCol_TextDisabled] = ImVec4(col_text.Value.x * 0.6f, col_text.Value.y * 0.6f, col_text.Value.z * 0.6f, 0.8f);
	_style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 0.43f);
	_style.Colors[ImGuiCol_WindowBg] = ImVec4(col_background.Value.x * 0.1f, col_background.Value.y * 0.1f, col_background.Value.z * 0.1f, col_background.Value.w);
	_style.Colors[ImGuiCol_PopupBg] = ImVec4(col_background.Value.x * 0.17f, col_background.Value.y * 0.17f, col_background.Value.z * 0.17f, col_background.Value.w);
	_style.Colors[ImGuiCol_Border] = ImVec4(col_background.Value.x * 0.55f, col_background.Value.y * 0.55f, col_background.Value.z * 0.55f, 0.90f);
	//_style.Colors[ImGuiCol_BorderShadow] = ImVec4(col_background.Value.x * 0.25f, col_background.Value.y * 0.25f, col_background.Value.z * 0.25f, 0.00f); // NOT IN USE
	_style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(col_background.Value.x * 0.8f, col_background.Value.y * 0.8f, col_background.Value.z * 0.8f, 0.00f);
	_style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(col_background.Value.x * 0.65f, col_background.Value.y * 0.65f, col_background.Value.z * 0.65f, 0.35f);
	_style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(col_background.Value.x * 0.65f, col_background.Value.y * 0.65f, col_background.Value.z * 0.65f, 0.35f);
	_style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(col_background.Value.x * 0.65f, col_background.Value.y * 0.65f, col_background.Value.z * 0.65f, 0.65f);
	_style.Colors[ImGuiCol_FrameBg] = ImVec4(col_background.Value.x * 0.3f, col_background.Value.y * 0.3f, col_background.Value.z * 0.3f, 0.9f);
	_style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(col_background.Value.x * 0.45f, col_background.Value.y * 0.45f, col_background.Value.z * 0.45f, 0.9f);
	_style.Colors[ImGuiCol_FrameBgActive] = ImVec4(col_background.Value.x * 0.55f, col_background.Value.y * 0.55f, col_background.Value.z * 0.55f, 0.9f);
	_style.Colors[ImGuiCol_CheckMark] = ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.f);
	_style.Colors[ImGuiCol_Button] = ImVec4(col_background.Value.x * 0.4f, col_background.Value.y * 0.4f, col_background.Value.z * 0.4f, 1.00f);
	_style.Colors[ImGuiCol_ButtonHovered] = ImVec4(col_background.Value.x * 0.25f, col_background.Value.y * 0.25f, col_background.Value.z * 0.25f, 1.00f);
	_style.Colors[ImGuiCol_ButtonActive] = ImVec4(col_background.Value.x * 0.45f, col_background.Value.y * 0.45f, col_background.Value.z * 0.45f, 1.00f);
	_style.Colors[ImGuiCol_SliderGrab] = ImVec4(col_background.Value.x * 0.8f, col_background.Value.y * 0.8f, col_background.Value.z * 0.8f, 0.31f);
	_style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(col_controller.Value.x, col_controller.Value.y, col_controller.Value.z, 1.00f);
	_style.Colors[ImGuiCol_MenuBarBg] = ImVec4(col_background.Value.x * 0.1f, col_background.Value.y * 0.1f, col_background.Value.z * 0.1f, 1.00f);
	//_style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f); // NOT IN USE
	//_style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f); // NOT IN USE
	//_style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f); // NOT IN USE
	//_style.Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f); // NOT IN USE
	//_style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f); // NOT IN USE
	//_style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f); // NOT IN USE
	//_style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f); // NOT IN USE
	_style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(col_background.Value.x * 0.65f, col_background.Value.y * 0.65f, col_background.Value.z * 0.65f, 0.7f);
}
