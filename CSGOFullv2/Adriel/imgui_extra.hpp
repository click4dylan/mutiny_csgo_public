#pragma once
#include "stdafx.hpp"
#include "custom_def.hpp"

namespace ImFontEx
{
	extern ImFont* weapons;
	extern ImFont* header;
	extern ImFont* tab;
	extern ImFont* checkbox;
	extern ImFont* smallest;
	extern ImFont* medium;
}

namespace ImGuiEx
{
	void* CopyFont(const unsigned int* src, size_t size);
	bool Combo(const char* label, int* currIndex, std::vector<std::string>& values);
	void SelectableCombo(const char* label, const std::vector<std::tuple<const char*, bool*, const char*>> &items_with_bools, int text_size_until_cutoff = 18);
	
	bool ListBox(const char* label, int* currIndex, std::vector<std::string>& values, int height_in_items);
	bool ListBox(const char* label, int* current_item, std::function<const char*(int)> lambda, int items_count, int height_in_items);
	bool ListBox(const char* label, int* current_item, bool (*items_getter)(void*, int, const char**), void* data, int items_count, ImVec2 vecsize);
	
	bool LabelClick(const char* label, bool* v, const char* unique_id);
	bool RadioButtonIcon(const char* label, bool active);
	void KeyBindButton(std::string text, int& key, bool label, bool block = false);
	void RenderTabs(std::vector<const char*> tabs, int& activetab, float w, float h, bool sameline);
	void SetTip(const char* text);
	bool Radio(const char* text, int* var, int v_button);

	bool Checkbox(const char* text, bool* var, bool icon = true, bool_sw* sw_var = nullptr, bool force_text_color = false, const ImVec4& color_to_force = ImVec4(1.f, 1.f, 1.f, 1.f));
	bool CheckboxIcon(const char* label, bool* v, bool_sw* sw_var = nullptr, bool force_text_color = false, const ImVec4& color_to_force = ImVec4(1.f, 1.f, 1.f, 1.f));
	bool CheckboxNormal(const char* label, bool* v, bool_sw* sw_var = nullptr, bool force_text_color = false, const ImVec4& color_to_force = ImVec4(1.f, 1.f, 1.f, 1.f));

	bool SelectableCenter(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size_arg);
	bool SelectableCenter(const char* label, bool* p_selected, ImGuiSelectableFlags flags, const ImVec2& size_arg);
	bool SelectableBind(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size_arg, int& i_key);

	bool ColorPicker(float* col, bool alphabar);
	bool ColorPicker3(float col[3]);
	bool ColorPicker4(float col[4]);

	void ColorPicker(const char* text, color_var* cv_col, float spaces);
	void ColorPicker(const char* text, health_color_var* cv_col, float spaces);

	bool SelectableBackground(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size_arg);
	//void ButtonConfigChams(const char* text, variable::struct_visual::struct_esp_player::struct_chams* cfg, float spaces = -1.f);
}