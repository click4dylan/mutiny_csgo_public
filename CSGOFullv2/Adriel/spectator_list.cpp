#include "../precompiled.h"
#include "spectator_list.hpp"
#include "ImGui/font_compressed.h"
#include "ui.hpp"

#include "../LocalPlayer.h"
#include "adr_util.hpp"

spectator_list::spectator_list() = default;

spectator_list::~spectator_list() = default;

std::list<int> spectator_list::get_observervators()
{
	std::list<int> list;
	CBaseEntity* local_ent = (CBaseEntity*)Interfaces::ClientEntList->GetClientEntity(Interfaces::EngineClient->GetLocalPlayer());

	if (!local_ent || !Interfaces::ClientEntList->EntityExists(local_ent))
		return list;

	CBaseEntity* player = local_ent;

	if (!local_ent->GetAlive())
	{
		const auto h_observer_target = local_ent->GetObserverTarget();
		if (!h_observer_target)
			return list;

		player = h_observer_target;
	}

	for (auto i = 1; i < Interfaces::EngineClient->GetMaxClients(); i++)
	{
		auto p_player = Interfaces::ClientEntList->GetBaseEntity(i);
		if (!p_player || !Interfaces::ClientEntList->EntityExists(p_player))
			continue;

		if (p_player->GetDormant() || p_player->GetAlive())
			continue;

		const auto p_target = p_player->GetObserverTarget();
		if (player != p_target)
			continue;

		list.push_back(i);
	}

	return list;
}

void spectator_list::render() const
{
	return;

	auto b_show = false;
	if (Interfaces::EngineClient->IsInGame())
		b_show = true;
	else
		if (ui::get().is_visible())
			b_show = true;

	if (!b_show)
		return; 

	if (!variable::get().visuals.spectator_list.b_enabled.get())
		return;

	//return; //TODO: fix spectator list deadlock

	//ImGui::SetNextWindowContentSize(ImVec2(200, 0));
	ImGui::SetNextWindowPos(ImVec2(150, 50), ImGuiCond_FirstUseEver);

	const auto style = &ImGui::GetStyle();
	auto wincolor = style->Colors[ImGuiCol_WindowBg];
	wincolor.w = variable::get().visuals.spectator_list.f_alpha;
	if (variable::get().visuals.spectator_list.f_alpha < 0.f)
		wincolor.w += 0.1f;

	auto winpos = ImVec2();
	auto winsize = ImVec2();

	ImGui::PushStyleColor(ImGuiCol_WindowBg, wincolor);

	//decrypts(0)

	std::string format = ICON_FA_USERS;
	format += XorStr("  SPECTATORS");
	if (ImGui::Begin(format.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
	{
		winpos = ImGui::GetWindowPos();
		winsize = ImGui::GetWindowSize();

		RENDER_MUTEX.Lock();
		if (LocalPlayer.Entity)
		{
			auto &list_spec = g_Info.m_SpectatorList;
			for (auto i : list_spec)
			{
				if (i == Interfaces::EngineClient->GetLocalPlayer())
					continue;

				auto p_player = Interfaces::ClientEntList->GetBaseEntity(i);
				if (!p_player)
					continue;

				player_info_t p_info {};
				p_player->GetPlayerInfo(&p_info);
				if (std::string(p_info.name) == XorStr("GOTV"))
					continue;

				ImGui::Text("%s", adr_util::sanitize_name(p_info.name).c_str());
			}
		}
		RENDER_MUTEX.Unlock();

		/*if (ImGui::BeginPopupContextItem("speclistalpha", 1))
		{
			ImGui::PushItemWidth(160);
			ImGui::SliderFloat(xorstr("##ALPHA_SPEC"), &variable::get().visual.spectator_list.f_alpha, 0.f, 1.f, "ALPHA: %0.2f");
			ImGui::PopItemWidth();
			ImGui::EndPopup();
		}*/

		ImGui::Dummy(ImVec2(150, -1));
		ImGui::End();
	}
	ImGui::PopStyleColor();
	//encrypts(0)

	if (ui::get().is_visible())
	{
		//decrypts(0)
		if (ImGui::Begin(XorStr("##speclistalpha"), nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar))
		{
			ImGui::SetWindowPos(ImVec2(winpos.x, winpos.y + winsize.y + 2));
			ImGui::SetWindowSize(ImVec2(winsize.x, -1));

			ImGui::PushItemWidth(-1);
			ImGui::SliderFloat(XorStr("##ALPHA_SPEC"), &variable::get().visuals.spectator_list.f_alpha, 0.f, 1.f, XorStr("ALPHA: %0.2f"));
			ImGui::PopItemWidth();

			ImGui::End();
		}
		//encrypts(0)
	}
}