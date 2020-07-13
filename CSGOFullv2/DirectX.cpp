#include "precompiled.h"
#include "DirectX.h"
#include <intrin.h>

#include "Adriel/console.hpp"
#include "Adriel/renderer.hpp"
#include "Adriel/util.hpp"
#include "Adriel/ui.hpp"
#include "Adriel/spectator_list.hpp"
#include "Adriel/ImGui/imgui_freetype.h"
#include "Adriel/nade_prediction.hpp"

#include "c_renderer.h"
#include "Visuals_imi.h"
#include "LocalPlayer.h"

//#include "Adriel/ImGui/win32/imgui_impl_win32.h"

HRESULT(WINAPI* oReset)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*) = nullptr;
HRESULT(WINAPI* oEndScene)(IDirect3DDevice9*) = nullptr;

struct FreeTypeTest
{
	enum FontBuildMode
	{
		FontBuildMode_FreeType,
		FontBuildMode_Stb
	};

	FontBuildMode BuildMode;
	bool          WantRebuild;
	float         FontsMultiply;
	int           FontsPadding;
	unsigned int  FontsFlags;

	FreeTypeTest()
	{
		BuildMode = FontBuildMode_FreeType;
		WantRebuild = true;
		FontsMultiply = 1.0f;
		FontsPadding = 1;
		FontsFlags = 0;
	}

	// Call _BEFORE_ NewFrame()
	bool UpdateRebuild()
	{
		if (!WantRebuild)
			return false;
		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->TexGlyphPadding = FontsPadding;
		for (int n = 0; n < io.Fonts->ConfigData.Size; n++)
		{
			ImFontConfig* font_config = (ImFontConfig*)&io.Fonts->ConfigData[n];
			font_config->RasterizerMultiply = FontsMultiply;
			font_config->RasterizerFlags = (BuildMode == FontBuildMode_FreeType) ? FontsFlags : 0x00;
		}
		if (BuildMode == FontBuildMode_FreeType)
			ImGuiFreeType::BuildFontAtlas(io.Fonts, FontsFlags);
		else if (BuildMode == FontBuildMode_Stb)
			io.Fonts->Build();
		WantRebuild = false;
		return true;
	}

	// Call to draw interface
	void ShowFreetypeOptionsWindow()
	{
		ImGui::Begin("FreeType Options");
		ImGui::ShowFontSelector("Fonts");
		WantRebuild |= ImGui::RadioButton("FreeType", (int*)&BuildMode, FontBuildMode_FreeType);
		ImGui::SameLine();
		WantRebuild |= ImGui::RadioButton("Stb (Default)", (int*)&BuildMode, FontBuildMode_Stb);
		WantRebuild |= ImGui::DragFloat("Multiply", &FontsMultiply, 0.001f, 0.0f, 2.0f);
		WantRebuild |= ImGui::DragInt("Padding", &FontsPadding, 0.1f, 0, 16);
		if (BuildMode == FontBuildMode_FreeType)
		{
			WantRebuild |= ImGui::CheckboxFlags("NoHinting", &FontsFlags, ImGuiFreeType::NoHinting);
			WantRebuild |= ImGui::CheckboxFlags("NoAutoHint", &FontsFlags, ImGuiFreeType::NoAutoHint);
			WantRebuild |= ImGui::CheckboxFlags("ForceAutoHint", &FontsFlags, ImGuiFreeType::ForceAutoHint);
			WantRebuild |= ImGui::CheckboxFlags("LightHinting", &FontsFlags, ImGuiFreeType::LightHinting);
			WantRebuild |= ImGui::CheckboxFlags("MonoHinting", &FontsFlags, ImGuiFreeType::MonoHinting);
			WantRebuild |= ImGui::CheckboxFlags("Bold", &FontsFlags, ImGuiFreeType::Bold);
			WantRebuild |= ImGui::CheckboxFlags("Oblique", &FontsFlags, ImGuiFreeType::Oblique);
			WantRebuild |= ImGui::CheckboxFlags("Monochrome", &FontsFlags, ImGuiFreeType::Monochrome);
		}
		ImGui::End();
	}
};

FreeTypeTest freetype_test;

long __stdcall Hooked_EndScene(IDirect3DDevice9* pDevice)
{	
	//decrypts(0)
	static auto b_once = (logger::add(LWARN, XorStr("End Scene Hooked at: 0x%.8X"), oEndScene), true);
	//encrypts(0)

	if (!variable::get().ui.b_init)
	{
		ui::get().initialize(pDevice);
		c_renderer::get().init_device_objects(pDevice);
		render::get().initialize(pDevice);
		variable::get().ui.b_init = true;
		return oEndScene(pDevice);
	}

	if (variable::get().ui.b_unload)
		return oEndScene(pDevice);

	D3DDEVICE_CREATION_PARAMETERS cparams;
	RECT rect;
	
	pDevice->GetCreationParameters(&cparams);
	GetWindowRect(cparams.hFocusWindow, &rect);

	if (rect.bottom <= 0 || rect.right <= 0)
		return oEndScene(pDevice);

	static uintptr_t gameoverlay_return_address = 0;
	static uintptr_t shaderapidx9_return_address = 0;

	if (!gameoverlay_return_address)
	{
		MEMORY_BASIC_INFORMATION info;
		VirtualQuery(_ReturnAddress(), &info, sizeof(MEMORY_BASIC_INFORMATION));

		char mod[MAX_PATH];
		GetModuleFileNameA(static_cast<HMODULE>(info.AllocationBase), mod, MAX_PATH);

		//decrypts(0)
		if (adr_util::string::to_lower(std::string(mod)).find(XorStr("gameoverlay")) != std::string::npos)
		{
			gameoverlay_return_address = reinterpret_cast<uintptr_t>(_ReturnAddress());
			logger::add(LWARN, XorStr("SteamGameOverlay found at EndScene, _ReturnAddress() = 0x%.8X"), gameoverlay_return_address);
		}
		//encrypts(0)
	}
	else
	{
		if (!shaderapidx9_return_address)
		{
			shaderapidx9_return_address = reinterpret_cast<uintptr_t>(_ReturnAddress());
			//decrypts(0)
			logger::add(LWARN, XorStr("Shader API DX9 found at EndScene, _ReturnAddress() = 0x%.8X"), shaderapidx9_return_address);
			//encrypts(0)
		}
	}

	if (gameoverlay_return_address != reinterpret_cast<uintptr_t>(_ReturnAddress()) && variable::get().ui.b_stream_proof)
		return oEndScene(pDevice);

	if (shaderapidx9_return_address != reinterpret_cast<uintptr_t>(_ReturnAddress()) && !variable::get().ui.b_stream_proof)
		return oEndScene(pDevice);

	IDirect3DVertexDeclaration9* decl = nullptr;
	IDirect3DVertexShader9* shader = nullptr;
	IDirect3DStateBlock9* block = nullptr;

	pDevice->GetVertexDeclaration(&decl);
	pDevice->GetVertexShader(&shader);
	pDevice->CreateStateBlock(D3DSBT_PIXELSTATE, &block);
	block->Capture();

	//backup render states
	DWORD colorwrite, srgbwrite;
	pDevice->GetRenderState(D3DRS_COLORWRITEENABLE, &colorwrite);
	pDevice->GetRenderState(D3DRS_SRGBWRITEENABLE, &srgbwrite);

	// fix drawing without calling engine functons/cl_showpos
	pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0xffffffff);
	// removes the source engine color correction
	pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, false);

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();

	if (freetype_test.UpdateRebuild())
	{
		ImGui_ImplDX9_CreateDeviceObjects();
		ImGui_ImplDX9_InvalidateDeviceObjects();
	}

	ImGui::NewFrame();

	//freetype_test.ShowFreetypeOptionsWindow();
	{
		spectator_list::get().render();
		//event_logger::get().render();
		//radar::get().render();
		ui::get().render();
	}

	c_renderer::get().setup_render_state();

	/*if (variable::get().visuals.b_enabled && Interfaces::EngineClient->IsInGame())
	{
		RENDER_MUTEX.Lock();

		RENDER_MUTEX.Unlock();
	}*/

	//restore these
	pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, colorwrite);
	pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, srgbwrite);

	ImGui::EndFrame();

	ImGui::Render(render::get().get_draw_data());
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	block->Apply();
	block->Release();
	pDevice->SetVertexShader(shader);
	pDevice->SetVertexDeclaration(decl);

	return oEndScene(pDevice);
}

long __stdcall Hooked_Reset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
	//decrypts(0)
	static auto b_once = (logger::add(LWARN, XorStr("Reset Hooked at: 0x%.8X"), oReset), true);
	//encrypts(0)
	if (!variable::get().ui.b_init)
		return oReset(pDevice, pPresentationParameters);

	//variable::get().global.b_reseting = true;
	render::get().invalidate_objects();
	c_renderer::get().invalidate_device_objects();
	ImGui_ImplDX9_InvalidateDeviceObjects();		
	ImGui::DestroyContext();

	auto h_return = oReset(pDevice, pPresentationParameters);
	if (h_return == D3DERR_INVALIDCALL)
	{
		// we stoopid
		return h_return;
	}

	ImGui::CreateContext();
	ui::get().update_objects(pDevice);
	render::get().create_objects();
	ImGui_ImplDX9_CreateDeviceObjects();

	c_renderer::get().init_device_objects(pDevice);

	//variable::get().global.b_reseting = false;
	return h_return;
}