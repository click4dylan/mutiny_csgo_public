#include "imgui_extra.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS

#include "ImGui/imgui_internal.h"
#include "adr_util.hpp"
#include "ImGUI/font_compressed.h"
#include "input.hpp"

namespace ImFontEx
{
	ImFont* weapons = nullptr;
	ImFont* header = nullptr;
	ImFont* tab = nullptr;
	ImFont* checkbox = nullptr;
	ImFont* smallest = nullptr;
	ImFont* medium = nullptr;
}

namespace ImGuiEx
{
	static const ImS32 IM_S32_MIN = 0x80000000; // INT_MIN;
	static const ImS32 IM_S32_MAX = 0x7FFFFFFF; // INT_MAX;
	static const ImU32 IM_U32_MIN = 0;
	static const ImU32 IM_U32_MAX = 0xFFFFFFFF;
	static const ImS64 IM_S64_MIN = -9223372036854775807ll - 1ll;
	static const ImS64 IM_S64_MAX = 9223372036854775807ll;
	static const ImU64 IM_U64_MIN = 0;
	static const ImU64 IM_U64_MAX = 0xFFFFFFFFFFFFFFFFull;

	struct ImGuiDataTypeInfo
	{
		size_t      Size;
		const char* PrintFmt;   // Unused
		const char* ScanFmt;
	};

	ImGuiDataTypeInfo GDataTypeInfo[] =
	{
		{ sizeof(int),          "%d",   "%d"    },
		{ sizeof(unsigned int), "%u",   "%u"    },
#ifdef _MSC_VER
		{ sizeof(ImS64),        "%I64d","%I64d" },
		{ sizeof(ImU64),        "%I64u","%I64u" },
#else
		{ sizeof(ImS64),        "%lld", "%lld"  },
		{ sizeof(ImU64),        "%llu", "%llu"  },
#endif
		{ sizeof(float),        "%f",   "%f"    },  // float are promoted to double in va_arg
		{ sizeof(double),       "%f",   "%lf"   },
	};

	int DataTypeFormatString(char* buf, int buf_size, ImGuiDataType data_type, const void* data_ptr, const char* format)
	{
		if (data_type == ImGuiDataType_S32 || data_type == ImGuiDataType_U32)   // Signedness doesn't matter when pushing the argument
			return ImFormatString(buf, buf_size, format, *(const ImU32*)data_ptr);

		if (data_type == ImGuiDataType_S64 || data_type == ImGuiDataType_U64)   // Signedness doesn't matter when pushing the argument
			return ImFormatString(buf, buf_size, format, *(const ImU64*)data_ptr);

		if (data_type == ImGuiDataType_Float)
			return ImFormatString(buf, buf_size, format, *(const float*)data_ptr);

		if (data_type == ImGuiDataType_Double)
			return ImFormatString(buf, buf_size, format, *(const double*)data_ptr);

		IM_ASSERT(0);
		return 0;
	}

	const char* PatchFormatStringFloatToInt(const char* fmt)
	{
		if (fmt[0] == '%' && fmt[1] == '.' && fmt[2] == '0' && fmt[3] == 'f' && fmt[4] == 0) // Fast legacy path for "%.0f" which is expected to be the most common case.
			return "%d";

		const char* fmt_start = ImParseFormatFindStart(fmt);    // Find % (if any, and ignore %%)
		const char* fmt_end = ImParseFormatFindEnd(fmt_start);  // Find end of format specifier, which itself is an exercise of confidence/recklessness (because snprintf is dependent on libc or user).

		if (fmt_end > fmt_start && fmt_end[-1] == 'f')
		{
#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
			if (fmt_start == fmt && fmt_end[0] == 0)
				return "%d";
			ImGuiContext& g = *GImGui;
			ImFormatString(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), "%.*s%%d%s", (int)(fmt_start - fmt), fmt, fmt_end); // Honor leading and trailing decorations, but lose alignment/precision.
			return g.TempBuffer;
#else
			IM_ASSERT(0 && "DragInt(): Invalid format string!"); // Old versions used a default parameter of "%.0f", please replace with e.g. "%d"
#endif
		}

		return fmt;
	}

	auto vector_getter = [](void* vec, int idx, const char** out_text)
	{
		auto& vector = *static_cast<std::vector<std::string>*>(vec);
		if (idx < 0 || idx >= static_cast<int>(vector.size()))
			return false;

		*out_text = vector.at(idx).c_str();
		return true;
	};

	bool Combo(const char* label, int* currIndex, std::vector<std::string>& values)
	{
		if (values.empty())
			return false;

		return ImGui::Combo(label, currIndex, vector_getter, static_cast<void*>(&values), values.size());
	}

	void SelectableCombo(const char* label, const std::vector<std::tuple<const char*, bool*, const char*>> &items_with_bools, int text_size_until_cutoff)
	{
		std::vector<std::string> items;
		for (size_t i = 0; i < items_with_bools.size(); ++i)
		{
			if (std::get<1>(items_with_bools[i]) != nullptr && *std::get<1>(items_with_bools[i]))
			{
				items.push_back(std::get<0>(items_with_bools[i]));
			}
		}

		std::string preview;
		for (auto iter = items.begin(); iter != items.end(); iter++) 
		{
			if (iter != items.begin())
			{
				//decrypts(0)
				preview += XorStr(", ");
				//encrypts(0)
			}

			preview += *iter;
		}

		if (preview.size() > text_size_until_cutoff)
		{
			preview.erase(text_size_until_cutoff, (preview.length() - text_size_until_cutoff));
			preview.erase(preview.find_last_of(',') + 1);
			//decrypts(0)
			preview += XorStr(" ...");
			//encrypts(0)
		}

		if (preview.empty())
		{
			//decrypts(0)
			preview = XorStr("None");
			//encrypts(0)
		}

		if (ImGui::BeginCombo(label, preview.data()))
		{
			for (size_t i = 0; i < items_with_bools.size(); ++i)
			{
				ImGui::Selectable(std::get<0>(items_with_bools[i]), std::get<1>(items_with_bools[i]), ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups);

				if (std::get<2>(items_with_bools[i]) != nullptr)
				{
					ImGuiEx::SetTip(std::get<2>(items_with_bools[i]));
				}
			}

			ImGui::EndCombo();
		}
	}

	bool ListBox(const char* label, int* currIndex, std::vector<std::string>& values, int height_in_items)
	{
		if (values.empty())
			return false;
		
		return ImGui::ListBox(label, currIndex, vector_getter, static_cast<void*>(&values), values.size(), height_in_items);
	}

	bool ListBox(const char* label, int* current_item, std::function<const char*(int)> lambda, int items_count, int height_in_items)
	{
		return ImGui::ListBox(label, current_item, [](void* data, int idx, const char** out_text)
		{
			*out_text = (*reinterpret_cast<std::function<const char*(int)>*>(data))(idx);
			return true;
		}, &lambda, items_count, height_in_items);
	}

	bool ListBox(const char* label, int* current_item, bool (* items_getter)(void*, int, const char**), void* data, int items_count, ImVec2 vecsize)
	{
		if (!ImGui::ListBoxHeader(label, vecsize))
			return false; // Assume all items have even height (= 1 line of text). If you need items of different or variable sizes you can create a custom version of ListBox() in your code without using the clipper.
		
		auto value_changed = false;
		ImGuiListClipper clipper(items_count, ImGui::GetTextLineHeightWithSpacing());
		
		while (clipper.Step())
		{
			for (auto i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
			{
				const auto item_selected = (i == *current_item);
				const char* item_text;
				
				if (!items_getter(data, i, &item_text))
				{
					//decrypts(0)
					item_text = XorStr("*Unknown item*");
					//encrypts(0)
				}
				
				ImGui::PushID(i);
				if (ImGui::Selectable(item_text, item_selected))
				{
					*current_item = i;
					value_changed = true;
				}
				ImGui::PopID();
			}
		}

		ImGui::ListBoxFooter();
		return value_changed;
	}

	bool LabelClick(const char* label, bool* v, const char* unique_id)
	{
		auto window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		auto& g = *GImGui;
		const auto& style = g.Style;
		const auto id = window->GetID(unique_id);
		const auto label_size = ImGui::CalcTextSize(label, nullptr, true);
		
		const ImRect check_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(label_size.y + style.FramePadding.y * 2, label_size.y + style.FramePadding.y * 2));
		ImGui::ItemSize(check_bb, style.FramePadding.y);
		auto total_bb = check_bb;

		if (label_size.x > 0)
			ImGui::SameLine(0, style.ItemInnerSpacing.x);
		
		const ImRect text_bb(window->DC.CursorPos + ImVec2(0, style.FramePadding.y), window->DC.CursorPos + ImVec2(0, style.FramePadding.y) + label_size);
		if (label_size.x > 0)
		{
			ImGui::ItemSize(ImVec2(text_bb.GetWidth(), check_bb.GetHeight()), style.FramePadding.y);
			total_bb = ImRect(ImMin(check_bb.Min, text_bb.Min), ImMax(check_bb.Max, text_bb.Max));
		}

		if (!ImGui::ItemAdd(total_bb, id))
			return false;

		bool hovered, held;
		const auto pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
		if (pressed)
			*v = !(*v);

		if (label_size.x > 0.0f)
		{
			//decrypts(0)
			ImGui::RenderText(check_bb.GetTL(), adr_util::string::format(XorStr("[ %s%s ]"), "", label).c_str());
			//encrypts(0)
		}

		return pressed;
	}

	bool CheckboxIcon(const char* label, bool* v, bool_sw* sw_var, bool force_text_color, const ImVec4 &color_to_force)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		ImGuiStyle& style = g.Style;

		const ImGuiID id = window->GetID(label);
		const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

		const float square_sz = ImGui::GetFrameHeight();
		const ImVec2 pos = window->DC.CursorPos;
		const ImRect total_bb(pos, pos + ImVec2(square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), label_size.y + style.FramePadding.y * 2.0f));
		ImGui::ItemSize(total_bb, style.FramePadding.y);
		if (!ImGui::ItemAdd(total_bb, id))
			return false;

		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
		if (pressed)
		{
			*v = !(*v);
			ImGui::MarkItemEdited(id);
		}

		const ImRect check_bb(pos, pos + ImVec2(square_sz, square_sz));
		//ImGui::RenderNavHighlight(total_bb, id);
		//ImGui::RenderFrame(check_bb.Min, check_bb.Max, ImGui::GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), true, style.FrameRounding);
		const float pad = ImMax(1.0f, (float)(int)(square_sz / 6.0f));
		if (*v)
		{
			auto old_txt = style.Colors[ImGuiCol_Text];
			style.Colors[ImGuiCol_Text] = style.Colors[ImGuiCol_CheckMark];
			ImGui::RenderText(check_bb.Min + ImVec2(0.f, pad + 4.f), ICON_FA_CHECK_SQUARE_O); //window->DrawList->AddText(check_bb.Min /*+ ImVec2(pad - 2.f, pad + 4.f)*/, ImGui::GetColorU32(ImGuiCol_CheckMark), ICON_FA_CHECK_SQUARE_O);
			style.Colors[ImGuiCol_Text] = old_txt;
		}
		else
		{
			auto old_txt = style.Colors[ImGuiCol_Text];
			style.Colors[ImGuiCol_Text] = style.Colors[ImGuiCol_FrameBg];
			ImGui::RenderText(check_bb.Min + ImVec2(0.f, pad + 4.f), ICON_FA_SQUARE_O); //window->DrawList->AddText(check_bb.Min /*+ ImVec2(pad - 2.f, pad + 4.f)*/, ImGui::GetColorU32(ImGuiCol_CheckMark), ICON_FA_SQUARE_O);
			style.Colors[ImGuiCol_Text] = old_txt;
		}
			
		if (g.LogEnabled)
			ImGui::LogRenderedText(&total_bb.Min, *v ? "[x]" : "[ ]");
		if (label_size.x > 0.0f)
		{
			if (force_text_color)
				ImGui::PushStyleColor(ImGuiCol_Text, color_to_force);
			
			ImGui::RenderText(ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x - 6, check_bb.Min.y + style.FramePadding.y), label);

			if (force_text_color)
				ImGui::PopStyleColor();
		}
		
		if (sw_var)
		{
			auto str_id = adr_util::string::format("##%s%d", label, id);

			if (ImGui::ItemHoverable(total_bb, id) && g.IO.MouseReleased[1])
				ImGui::OpenPopup(adr_util::string::format("%spop", str_id.c_str()).c_str());

			if (ImGui::BeginPopup(adr_util::string::format("%spop", str_id.c_str()).c_str(), ImGuiWindowFlags_NoMove))
			{
				ImGui::PushItemWidth(120);
				ImGui::Combo(adr_util::string::format("%scombo", str_id.c_str()).c_str(), &sw_var->i_mode, XorStrCT(" Normal\0 Key Hold\0 Key Toggle\0"));
				
				if (sw_var->i_mode == 1 || sw_var->i_mode == 2)
					KeyBindButton(adr_util::string::format("%skey", str_id.c_str()).c_str(), sw_var->i_key, false);

				ImGui::PopItemWidth();
				ImGui::EndPopup();
			}
		}

		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
		return pressed;
	}

	bool CheckboxNormal(const char* label, bool* v, bool_sw* sw_var, bool force_text_color, const ImVec4 &color_to_force)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

		const float square_sz = ImGui::GetFrameHeight();
		const ImVec2 pos = window->DC.CursorPos;
		const ImRect total_bb(pos, pos + ImVec2(square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), label_size.y + style.FramePadding.y * 2.0f));
		ImGui::ItemSize(total_bb, style.FramePadding.y);
		if (!ImGui::ItemAdd(total_bb, id))
			return false;

		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
		if (pressed)
		{
			*v = !(*v);
			ImGui::MarkItemEdited(id);
		}

		const ImRect check_bb(pos, pos + ImVec2(square_sz, square_sz));
		ImGui::RenderNavHighlight(total_bb, id);
		ImGui::RenderFrame(check_bb.Min, check_bb.Max, ImGui::GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), true, style.FrameRounding);
		if (*v)
		{
			const float pad = ImMax(1.0f, (float)(int)(square_sz / 6.0f));
			ImGui::RenderCheckMark(check_bb.Min + ImVec2(pad, pad), ImGui::GetColorU32(ImGuiCol_CheckMark), square_sz - pad * 2.0f);
		}

		if (g.LogEnabled)
			ImGui::LogRenderedText(&total_bb.Min, *v ? "[x]" : "[ ]");
		if (label_size.x > 0.0f)
		{
			if (force_text_color)
				ImGui::PushStyleColor(ImGuiCol_Text, color_to_force);
			
			ImGui::RenderText(ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y), label);

			if (force_text_color)
				ImGui::PopStyleColor();
		}
		
		if (sw_var)
		{
			auto str_id = adr_util::string::format("##%s%d", label, id);

			if (ImGui::ItemHoverable(total_bb, id) && g.IO.MouseReleased[1])
				ImGui::OpenPopup(adr_util::string::format("%spop", str_id.c_str()).c_str());

			if (ImGui::BeginPopup(adr_util::string::format("%spop", str_id.c_str()).c_str(), ImGuiWindowFlags_NoMove))
			{
				ImGui::PushItemWidth(120);
				ImGui::Combo(adr_util::string::format("%scombo", str_id.c_str()).c_str(), &sw_var->i_mode, XorStrCT(" Normal\0 Key Hold\0 Key Toggle\0"));
				
				if (sw_var->i_mode == 1 || sw_var->i_mode == 2)
					KeyBindButton(adr_util::string::format("%skey", str_id.c_str()).c_str(), sw_var->i_key, false);

				ImGui::PopItemWidth();
				ImGui::EndPopup();
			}
		}

		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
		return pressed;
	}

	bool RadioButtonIcon(const char* label, bool active)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

		const float square_sz = ImGui::GetFrameHeight();
		const ImVec2 pos = window->DC.CursorPos;
		const ImRect check_bb(pos, pos + ImVec2(square_sz, square_sz));
		const ImRect total_bb(pos, pos + ImVec2(square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), label_size.y + style.FramePadding.y * 2.0f));
		ImGui::ItemSize(total_bb, style.FramePadding.y);
		if (!ImGui::ItemAdd(total_bb, id))
			return false;

		ImVec2 center = check_bb.GetCenter();
		center.x = (float)(int)center.x + 0.5f;
		center.y = (float)(int)center.y + 0.5f;
		const float radius = (square_sz - 1.0f) * 0.5f;

		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
		if (pressed)
			ImGui::MarkItemEdited(id);

		//ImGui::RenderNavHighlight(total_bb, id);
		//window->DrawList->AddCircleFilled(center, radius, ImGui::GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), 16);
		
		const float pad = ImMax(1.0f, (float)(int)(square_sz / 6.0f));
		if (active)
			window->DrawList->AddText(check_bb.Min + ImVec2(pad - 2.f, pad + 4.f), ImGui::GetColorU32(ImGuiCol_CheckMark), u8"\uf05d");
		else
			window->DrawList->AddText(check_bb.Min + ImVec2(pad - 2.f, pad + 4.f), ImGui::GetColorU32(ImGuiCol_CheckMark), u8"\uf10c");

		if (style.FrameBorderSize > 0.0f)
		{
			window->DrawList->AddCircle(center + ImVec2(1, 1), radius, ImGui::GetColorU32(ImGuiCol_BorderShadow), 16, style.FrameBorderSize);
			window->DrawList->AddCircle(center, radius, ImGui::GetColorU32(ImGuiCol_Border), 16, style.FrameBorderSize);
		}

		if (g.LogEnabled)
			ImGui::LogRenderedText(&total_bb.Min, active ? "(x)" : "( )");
		if (label_size.x > 0.0f)
			ImGui::RenderText(ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x - 6, check_bb.Min.y + style.FramePadding.y), label);

		return pressed;
	}

	void* CopyFont(const unsigned int* src, size_t size)
	{
		const auto ret = static_cast<void*>(new unsigned int[size / 4]);
		memcpy_s(ret, size, src, size);
		return ret;
	}

	void KeyBindButton(std::string text, int& key, bool label, bool block)
	{
		static std::unordered_map<std::string, bool> map;
		const auto got = map.find(text);
		
		if (got == map.end())
		{
			map.emplace(text, false); //console::get().print(stringm::get().format("added %s", text.c_str()), 1, true);
			return;
		}
		
		auto& b_get = got->second;
		//decrypts(0)
		std::string str_state = XorStr("..."); //
		//encrypts(0)

		if (b_get)
		{
			for (auto i = 1; i < 255; i++)
			{
				if (input::get().is_key_down(i))
				{
					if (!block)
					{
						key = i == VK_ESCAPE ? 0 : i;
						b_get = false;
						break;
					}

					if (i != VK_LBUTTON && i != VK_RBUTTON)
					{
						key = i == VK_ESCAPE ? 0 : i;
						b_get = false;
						break;
					}
				}
			}
			//decrypts(0)
			str_state = XorStr("Press a key");
			//encrypts(0)
		}
		else if (!b_get && key == 0)
		{
			//decrypts(0)
			str_state = XorStr("Click to bind");
			//encrypts(0)
			if (label)
			{
				//decrypts(0)
				str_state = XorStr("...");
				//encrypts(0)
			}
		}
		else if (!b_get && key != 0)
		{
			//decrypts(0)
			str_state = XorStr("Key ~ ") + adr_util::get_key_name_by_id(key);
			//encrypts(0)
		}

		//decrypts(0)
		auto str_id = adr_util::string::format(XorStr("%s##%s%d"), str_state.c_str(), text.c_str(), key);
		//encrypts(0)
		if (label)
		{
			//ImGui::AlignFirstTextHeightToWidgets(); // OBSOLETE
			LabelClick(str_id.c_str(), &b_get, str_id.c_str());
		}
		else
		{
			if (ImGui::Button(str_id.c_str(), ImVec2(120, 20)))
				b_get = true;
		}
	}

	void RenderTabs(std::vector<const char*> tabs, int& activetab, float w, float h, bool sameline)
	{
		bool values[15] = { false };
		values[activetab] = true;
		
		for (auto i = 0; i < static_cast<int>(tabs.size()); ++i)
		{
			if (SelectableCenter(tabs[i], &values[i], 0, ImVec2{ w, h }))
				activetab = i;

			if (sameline && i < static_cast<int>(tabs.size()) - 1)
				ImGui::SameLine();
		}
	}

	void SetTip(const char* text)
	{
		if (variable::get().ui.b_use_tooltips)
		{
			if (ImGui::IsItemHovered())
			{
				//decrypts(0)
				ImGui::SetTooltip(XorStr("%s"), text);
				//encrypts(0)
			}
		}
	}

	bool Checkbox(const char* text, bool* var, bool icon, bool_sw* sw_var, bool force_text_color, const ImVec4 &color_to_force)
	{
		ImGui::PushFont(ImFontEx::checkbox);

		auto bret = false;
		if (icon)
			bret = CheckboxIcon(text, var, sw_var, force_text_color, color_to_force);
		else
			bret = CheckboxNormal(text, var, sw_var, force_text_color, color_to_force);

		ImGui::PopFont();

		if (sw_var)
		{
			std::string key_info = "[";
			//decrypts(0)
			switch (sw_var->i_mode)
			{
				case 0:
				{
					key_info += "-";
					break;
				}
				case 1:
				{
					key_info += XorStr("Hold ");
					key_info += adr_util::get_key_name_by_id(sw_var->i_key);
					break;
				}
				case 2:
				{
					key_info += XorStr("Toggle ");
					key_info += adr_util::get_key_name_by_id(sw_var->i_key);
					break;
				}

				default:
					break;
			}
			//encrypts(0)

			key_info += "]";
			ImGui::SameLine();

			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.f), key_info.data());
		}
		
		return bret;
	}

	bool Radio(const char* text, int* var, int v_button)
	{
		ImGui::PushFont(ImFontEx::checkbox);
		const auto pressed = RadioButtonIcon(text, *var == v_button);
		ImGui::PopFont();
		if (pressed)
			*var = v_button;
		
		return pressed;
	}

	bool SelectableCenter(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size_arg)
	{
		auto window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;
		
		auto& g = *GImGui;
		const auto& style = g.Style;

		if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.CurrentColumns) // FIXME-OPT: Avoid if vertically clipped.
			ImGui::PushColumnsBackground();
		
		ImGuiID id = window->GetID(label);
		ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
		ImVec2 size(size_arg.x != 0.0f ? size_arg.x : label_size.x, size_arg.y != 0.0f ? size_arg.y : label_size.y);
		ImVec2 pos = window->DC.CursorPos;
		pos.y += window->DC.CurrLineTextBaseOffset;
		ImRect bb_inner(pos, pos + size);
		ImGui::ItemSize(size);
		
		// Fill horizontal space.
		ImVec2 window_padding = window->WindowPadding;
		float max_x = (flags & ImGuiSelectableFlags_SpanAllColumns) ? ImGui::GetWindowContentRegionMax().x : ImGui::GetContentRegionMax().x;
		float w_draw = ImMax(label_size.x, window->Pos.x + max_x - window_padding.x - pos.x);
		ImVec2 size_draw((size_arg.x != 0 && !(flags & ImGuiSelectableFlags_DrawFillAvailWidth)) ? size_arg.x : w_draw, size_arg.y != 0.0f ? size_arg.y : size.y);
		ImRect bb(pos, pos + size_draw);
		if (size_arg.x == 0.0f || (flags & ImGuiSelectableFlags_DrawFillAvailWidth))
			bb.Max.x += window_padding.x;

		// Selectables are tightly packed together so we extend the box to cover spacing between selectable.
		const float spacing_x = style.ItemSpacing.x;
		const float spacing_y = style.ItemSpacing.y;
		const float spacing_L = (float)(int)(spacing_x * 0.50f);
		const float spacing_U = (float)(int)(spacing_y * 0.50f);
		bb.Min.x -= spacing_L;
		bb.Min.y -= spacing_U;
		bb.Max.x += (spacing_x - spacing_L);
		bb.Max.y += (spacing_y - spacing_U);
		
		bool item_add;
		if (flags & ImGuiSelectableFlags_Disabled)
		{
			ImGuiItemFlags backup_item_flags = window->DC.ItemFlags;
			window->DC.ItemFlags |= ImGuiItemFlags_Disabled | ImGuiItemFlags_NoNavDefaultFocus;
			item_add = ImGui::ItemAdd(bb, id);
			window->DC.ItemFlags = backup_item_flags;
		}
		else
		{
			item_add = ImGui::ItemAdd(bb, id);
		}
		if (!item_add)
		{
			if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.CurrentColumns)
				ImGui::PopColumnsBackground();
			return false;
		}

		// We use NoHoldingActiveID on menus so user can click and _hold_ on a menu then drag to browse child entries
		ImGuiButtonFlags button_flags = 0;
		if (flags & ImGuiSelectableFlags_NoHoldingActiveID) button_flags |= ImGuiButtonFlags_NoHoldingActiveID;
		if (flags & ImGuiSelectableFlags_PressedOnClick) button_flags |= ImGuiButtonFlags_PressedOnClick;
		if (flags & ImGuiSelectableFlags_PressedOnRelease) button_flags |= ImGuiButtonFlags_PressedOnRelease;
		if (flags & ImGuiSelectableFlags_Disabled) button_flags |= ImGuiButtonFlags_Disabled;
		if (flags & ImGuiSelectableFlags_AllowDoubleClick) button_flags |= ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick;
		if (flags & ImGuiSelectableFlags_AllowItemOverlap) button_flags |= ImGuiButtonFlags_AllowItemOverlap;

		if (flags & ImGuiSelectableFlags_Disabled)
			selected = false;

		const bool was_selected = selected;
		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, button_flags);
		// Hovering selectable with mouse updates NavId accordingly so navigation can be resumed with gamepad/keyboard (this doesn't happen on most widgets)
		if (pressed || hovered)
			if (!g.NavDisableMouseHover && g.NavWindow == window && g.NavLayer == window->DC.NavLayerCurrent)
			{
				g.NavDisableHighlight = true;
				ImGui::SetNavID(id, window->DC.NavLayerCurrent);
			}
		if (pressed)
			ImGui::MarkItemEdited(id);

		if (flags & ImGuiSelectableFlags_AllowItemOverlap)
			ImGui::SetItemAllowOverlap();
		
		// In this branch, Selectable() cannot toggle the selection so this will never trigger.
		if (selected != was_selected) //-V547
			window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_ToggledSelection;

		// Render
		if (hovered || selected)
		{
			const ImU32 col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
			ImGui::RenderFrame(bb.Min, bb.Max, col, false, 0.0f);
			ImGui::RenderNavHighlight(bb, id, ImGuiNavHighlightFlags_TypeThin | ImGuiNavHighlightFlags_NoRounding);
		}

		if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.CurrentColumns)
		{
			ImGui::PopColumnsBackground();
			bb.Max.x -= (ImGui::GetContentRegionMax().x - max_x);
		}

		if (flags & ImGuiSelectableFlags_Disabled) ImGui::PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_TextDisabled]);
		ImGui::RenderTextClipped((bb_inner.Max + bb_inner.Min) / 2 - label_size / 2, bb_inner.Max, label, NULL, &label_size, style.SelectableTextAlign, &bb);
		if (flags & ImGuiSelectableFlags_Disabled) ImGui::PopStyleColor();

		// Automatically close popups
		if (pressed && (window->Flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiSelectableFlags_DontClosePopups) && !(window->DC.ItemFlags & ImGuiItemFlags_SelectableDontClosePopup))
			ImGui::CloseCurrentPopup();

		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags);
		return pressed;
	}

	bool SelectableCenter(const char* label, bool* p_selected, ImGuiSelectableFlags flags, const ImVec2& size_arg)
	{
		if (SelectableCenter(label, *p_selected, flags, size_arg))
		{
			*p_selected = !*p_selected;
			return true;
		}
		return false;
	}

	bool SelectableBind(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size_arg, int& i_key)
	{
		auto window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		auto& g = *GImGui;
		const auto& style = g.Style;

		if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.CurrentColumns) // FIXME-OPT: Avoid if vertically clipped.
			ImGui::PushColumnsBackground();

		ImGuiID id = window->GetID(label);
		ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
		ImVec2 size(size_arg.x != 0.0f ? size_arg.x : label_size.x, size_arg.y != 0.0f ? size_arg.y : label_size.y);
		ImVec2 pos = window->DC.CursorPos;
		pos.y += window->DC.CurrLineTextBaseOffset;
		ImRect bb_inner(pos, pos + size);
		ImGui::ItemSize(size);

		// Fill horizontal space.
		ImVec2 window_padding = window->WindowPadding;
		float max_x = (flags & ImGuiSelectableFlags_SpanAllColumns) ? ImGui::GetWindowContentRegionMax().x : ImGui::GetContentRegionMax().x;
		float w_draw = ImMax(label_size.x, window->Pos.x + max_x - window_padding.x - pos.x);
		ImVec2 size_draw((size_arg.x != 0 && !(flags & ImGuiSelectableFlags_DrawFillAvailWidth)) ? size_arg.x : w_draw, size_arg.y != 0.0f ? size_arg.y : size.y);
		ImRect bb(pos, pos + size_draw);
		if (size_arg.x == 0.0f || (flags & ImGuiSelectableFlags_DrawFillAvailWidth))
			bb.Max.x += window_padding.x;

		// Selectables are tightly packed together so we extend the box to cover spacing between selectable.
		const float spacing_x = style.ItemSpacing.x;
		const float spacing_y = style.ItemSpacing.y;
		const float spacing_L = (float)(int)(spacing_x * 0.50f);
		const float spacing_U = (float)(int)(spacing_y * 0.50f);
		bb.Min.x -= spacing_L;
		bb.Min.y -= spacing_U;
		bb.Max.x += (spacing_x - spacing_L);
		bb.Max.y += (spacing_y - spacing_U);

		bool item_add;
		if (flags & ImGuiSelectableFlags_Disabled)
		{
			ImGuiItemFlags backup_item_flags = window->DC.ItemFlags;
			window->DC.ItemFlags |= ImGuiItemFlags_Disabled | ImGuiItemFlags_NoNavDefaultFocus;
			item_add = ImGui::ItemAdd(bb, id);
			window->DC.ItemFlags = backup_item_flags;
		}
		else
		{
			item_add = ImGui::ItemAdd(bb, id);
		}
		if (!item_add)
		{
			if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.CurrentColumns)
				ImGui::PopColumnsBackground();
			return false;
		}

		// We use NoHoldingActiveID on menus so user can click and _hold_ on a menu then drag to browse child entries
		ImGuiButtonFlags button_flags = 0;
		if (flags & ImGuiSelectableFlags_NoHoldingActiveID) button_flags |= ImGuiButtonFlags_NoHoldingActiveID;
		if (flags & ImGuiSelectableFlags_PressedOnClick) button_flags |= ImGuiButtonFlags_PressedOnClick;
		if (flags & ImGuiSelectableFlags_PressedOnRelease) button_flags |= ImGuiButtonFlags_PressedOnRelease;
		if (flags & ImGuiSelectableFlags_Disabled) button_flags |= ImGuiButtonFlags_Disabled;
		if (flags & ImGuiSelectableFlags_AllowDoubleClick) button_flags |= ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick;
		if (flags & ImGuiSelectableFlags_AllowItemOverlap) button_flags |= ImGuiButtonFlags_AllowItemOverlap;

		if (flags & ImGuiSelectableFlags_Disabled)
			selected = false;

		const bool was_selected = selected;
		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, button_flags);
		// Hovering selectable with mouse updates NavId accordingly so navigation can be resumed with gamepad/keyboard (this doesn't happen on most widgets)
		if (pressed || hovered)
			if (!g.NavDisableMouseHover && g.NavWindow == window && g.NavLayer == window->DC.NavLayerCurrent)
			{
				g.NavDisableHighlight = true;
				ImGui::SetNavID(id, window->DC.NavLayerCurrent);
			}
		if (pressed)
			ImGui::MarkItemEdited(id);

		if (flags & ImGuiSelectableFlags_AllowItemOverlap)
			ImGui::SetItemAllowOverlap();

		// In this branch, Selectable() cannot toggle the selection so this will never trigger.
		if (selected != was_selected) //-V547
			window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_ToggledSelection;

		// Render
		if (hovered || selected)
		{
			const ImU32 col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
			ImGui::RenderFrame(bb.Min, bb.Max, col, false, 0.0f);
			ImGui::RenderNavHighlight(bb, id, ImGuiNavHighlightFlags_TypeThin | ImGuiNavHighlightFlags_NoRounding);
		}

		if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.CurrentColumns)
		{
			ImGui::PopColumnsBackground();
			bb.Max.x -= (ImGui::GetContentRegionMax().x - max_x);
		}

		if (flags & ImGuiSelectableFlags_Disabled) ImGui::PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_TextDisabled]);
		ImGui::RenderTextClipped(bb_inner.Min, bb_inner.Max, label, NULL, &label_size, style.SelectableTextAlign, &bb);
		if (flags & ImGuiSelectableFlags_Disabled) ImGui::PopStyleColor();

		// Automatically close popups
		if (pressed && (window->Flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiSelectableFlags_DontClosePopups) && !(window->DC.ItemFlags & ImGuiItemFlags_SelectableDontClosePopup))
			ImGui::CloseCurrentPopup();

		/*if (selected)
		{
			auto str_id = adr_util::string::format("##%s%d", label, id);

			if (ImGui::ItemHoverable(bb_inner, id) && g.IO.MouseReleased[1])
			{
				ImGui::OpenPopup(adr_util::string::format("%spop", str_id.c_str()).c_str());
				variable::get().ui.b_ready_to_load = false;
			}

			if (ImGui::BeginPopup(adr_util::string::format("%spop", str_id.c_str()).c_str(), ImGuiWindowFlags_NoMove))
			{
				ImGui::PushItemWidth(120);

				KeyBindButton(adr_util::string::format("%skey", str_id.c_str()).c_str(), i_key, false, true);

				ImGui::PopItemWidth();
				ImGui::EndPopup();
			}

			if (!ImGui::IsPopupOpen(adr_util::string::format("%spop", str_id.c_str()).c_str()) && !variable::get().ui.b_ready_to_load)
				variable::get().ui.b_ready_to_load = true;
		}*/

		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags);
	    return pressed;
	}

	bool ColorPicker(float* col, bool alphabar)
	{
		const auto edge_size = 200; // = int(ImGui::GetWindowWidth() * 0.75f);
		const auto sv_picker_size = ImVec2(edge_size, edge_size);
		const auto spacing = ImGui::GetStyle().ItemInnerSpacing.x;
		const auto hue_picker_width = 20.f;
		const auto crosshair_size = 7.0f;

		ImColor color(col[0], col[1], col[2]);
		auto value_changed = false;
		auto draw_list = ImGui::GetWindowDrawList();
		const auto picker_pos = ImGui::GetCursorScreenPos();

		float hue, saturation, value;
		ImGui::ColorConvertRGBtoHSV(color.Value.x, color.Value.y, color.Value.z, hue, saturation, value);
		ImColor colors[] = {
			ImColor(255, 0, 0), ImColor(255, 255, 0), ImColor(0, 255, 0), ImColor(0, 255, 255), ImColor(0, 0, 255), ImColor(255, 0, 255), ImColor(255, 0, 0)
		};

		for (auto i = 0; i < 6; i++)
			draw_list->AddRectFilledMultiColor(ImVec2(picker_pos.x + sv_picker_size.x + spacing, picker_pos.y + i * (sv_picker_size.y / 6)), ImVec2(picker_pos.x + sv_picker_size.x + spacing + hue_picker_width, picker_pos.y + (i + 1) * (sv_picker_size.y / 6)), colors[i], colors[i], colors[i + 1], colors[i + 1]);

		draw_list->AddLine(ImVec2(picker_pos.x + sv_picker_size.x + spacing - 2, picker_pos.y + hue * sv_picker_size.y), ImVec2(picker_pos.x + sv_picker_size.x + spacing + 2 + hue_picker_width, picker_pos.y + hue * sv_picker_size.y), ImColor(255, 255, 255));
		if (alphabar)
		{
			const auto alpha = col[3];
			draw_list->AddRectFilledMultiColor(ImVec2(picker_pos.x + sv_picker_size.x + 2 * spacing + hue_picker_width, picker_pos.y), ImVec2(picker_pos.x + sv_picker_size.x + 2 * spacing + 2 * hue_picker_width, picker_pos.y + sv_picker_size.y), ImColor(0, 0, 0), ImColor(0, 0, 0), ImColor(255, 255, 255), ImColor(255, 255, 255));
			draw_list->AddLine(ImVec2(picker_pos.x + sv_picker_size.x + 2 * (spacing - 2) + hue_picker_width, picker_pos.y + alpha * sv_picker_size.y), ImVec2(picker_pos.x + sv_picker_size.x + 2 * (spacing + 2) + 2 * hue_picker_width, picker_pos.y + alpha * sv_picker_size.y), ImColor(255.f - alpha, 255.f, 255.f));
		}

		const auto c_o_color_black = ImGui::ColorConvertFloat4ToU32(ImVec4(0.f, 0.f, 0.f, 1.f));
		const auto c_o_color_black_transparent = ImGui::ColorConvertFloat4ToU32(ImVec4(0.f, 0.f, 0.f, 0.f));
		const auto c_o_color_white = ImGui::ColorConvertFloat4ToU32(ImVec4(1.f, 1.f, 1.f, 1.f));
		
		ImVec4 c_hue_value(1, 1, 1, 1);
		ImGui::ColorConvertHSVtoRGB(hue, 1, 1, c_hue_value.x, c_hue_value.y, c_hue_value.z);

		const auto o_hue_color = ImGui::ColorConvertFloat4ToU32(c_hue_value);
		draw_list->AddRectFilledMultiColor(ImVec2(picker_pos.x, picker_pos.y), ImVec2(picker_pos.x + sv_picker_size.x, picker_pos.y + sv_picker_size.y), c_o_color_white, o_hue_color, o_hue_color, c_o_color_white);
		draw_list->AddRectFilledMultiColor(ImVec2(picker_pos.x, picker_pos.y), ImVec2(picker_pos.x + sv_picker_size.x, picker_pos.y + sv_picker_size.y), c_o_color_black_transparent, c_o_color_black_transparent, c_o_color_black, c_o_color_black);
		
		const auto x = saturation * sv_picker_size.x;
		const auto y = (1 - value) * sv_picker_size.y;
		const ImVec2 p(picker_pos.x + x, picker_pos.y + y);
		
		draw_list->AddLine(ImVec2(p.x - crosshair_size, p.y), ImVec2(p.x - 2, p.y), ImColor(255, 255, 255));
		draw_list->AddLine(ImVec2(p.x + crosshair_size, p.y), ImVec2(p.x + 2, p.y), ImColor(255, 255, 255));
		draw_list->AddLine(ImVec2(p.x, p.y + crosshair_size), ImVec2(p.x, p.y + 2), ImColor(255, 255, 255));
		draw_list->AddLine(ImVec2(p.x, p.y - crosshair_size), ImVec2(p.x, p.y - 2), ImColor(255, 255, 255));
		
		ImGui::InvisibleButton(XorStrCT("saturation_value_selector"), sv_picker_size);
		
		if (ImGui::IsItemActive() && ImGui::GetIO().MouseDown[0])
		{
			auto mouse_pos_in_canvas = ImVec2(ImGui::GetIO().MousePos.x - picker_pos.x, ImGui::GetIO().MousePos.y - picker_pos.y);
			if (mouse_pos_in_canvas.x < 0)
				mouse_pos_in_canvas.x = 0;
			else if (mouse_pos_in_canvas.x >= sv_picker_size.x - 1)
				mouse_pos_in_canvas.x = sv_picker_size.x - 1;
			
			if (mouse_pos_in_canvas.y < 0)
				mouse_pos_in_canvas.y = 0;
			else if (mouse_pos_in_canvas.y >= sv_picker_size.y - 1)
				mouse_pos_in_canvas.y = sv_picker_size.y - 1;
			
			value = 1 - (mouse_pos_in_canvas.y / (sv_picker_size.y - 1));
			saturation = mouse_pos_in_canvas.x / (sv_picker_size.x - 1);
			value_changed = true;
		} // hue bar logic
		
		ImGui::SetCursorScreenPos(ImVec2(picker_pos.x + spacing + sv_picker_size.x, picker_pos.y));
		ImGui::InvisibleButton(XorStrCT("hue_selector"), ImVec2(hue_picker_width, sv_picker_size.y));
		
		if (ImGui::GetIO().MouseDown[0] && (ImGui::IsItemHovered() || ImGui::IsItemActive()))
		{
			auto mouse_pos_in_canvas = ImVec2(ImGui::GetIO().MousePos.x - picker_pos.x, ImGui::GetIO().MousePos.y - picker_pos.y);
			if (mouse_pos_in_canvas.y < 0)
				mouse_pos_in_canvas.y = 0;
			else if (mouse_pos_in_canvas.y >= sv_picker_size.y - 1)
				mouse_pos_in_canvas.y = sv_picker_size.y - 1;
			
			hue = mouse_pos_in_canvas.y / (sv_picker_size.y - 1);
			value_changed = true;
		}
		
		if (alphabar)
		{
			ImGui::SetCursorScreenPos(ImVec2(picker_pos.x + spacing * 2 + hue_picker_width + sv_picker_size.x, picker_pos.y));
			ImGui::InvisibleButton(XorStrCT("alpha_selector"), ImVec2(hue_picker_width, sv_picker_size.y));
			
			if (ImGui::GetIO().MouseDown[0] && (ImGui::IsItemHovered() || ImGui::IsItemActive()))
			{
				auto mouse_pos_in_canvas = ImVec2(ImGui::GetIO().MousePos.x - picker_pos.x, ImGui::GetIO().MousePos.y - picker_pos.y);
				if (mouse_pos_in_canvas.y < 0)
					mouse_pos_in_canvas.y = 0;
				else if (mouse_pos_in_canvas.y >= sv_picker_size.y - 1)
					mouse_pos_in_canvas.y = sv_picker_size.y - 1;
				
				const auto alpha = mouse_pos_in_canvas.y / (sv_picker_size.y - 1);
				col[3] = alpha;
				value_changed = true;
			}
		}
		
		color = ImColor::HSV(hue >= 1 ? hue - 10 * 0.000001f : hue, saturation > 0 ? saturation : 10 * 0.000001f, value > 0 ? value : 0.000001f);
		col[0] = color.Value.x;
		col[1] = color.Value.y;
		col[2] = color.Value.z;
		ImGui::PushItemWidth((alphabar ? spacing + hue_picker_width : 0) + sv_picker_size.x + spacing + hue_picker_width - 2 * ImGui::GetStyle().FramePadding.x);
		
		const auto widget_used = alphabar ? ImGui::ColorEdit4("", col) : ImGui::ColorEdit3("", col);
		ImGui::PopItemWidth();
		
		float new_hue, new_sat, new_val;
		ImGui::ColorConvertRGBtoHSV(col[0], col[1], col[2], new_hue, new_sat, new_val);
		
		if (new_hue <= 0 && hue > 0)
		{
			if (new_val <= 0 && value != new_val)
			{
				color = ImColor::HSV(hue, saturation, new_val <= 0 ? value * 0.5f : new_val);
				col[0] = color.Value.x;
				col[1] = color.Value.y;
				col[2] = color.Value.z;
			}
			else if (new_sat <= 0)
			{
				color = ImColor::HSV(hue, new_sat <= 0 ? saturation * 0.5f : new_sat, new_val);
				col[0] = color.Value.x;
				col[1] = color.Value.y;
				col[2] = color.Value.z;
			}
		}
		
		return value_changed | widget_used;
	}

	bool ColorPicker3(float col[3])
	{
		return ColorPicker(col, false);
	}

	bool ColorPicker4(float col[4])
	{
		return ColorPicker(col, true);
	}

	bool SelectableBackground(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size_arg)
	{
		auto window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		auto& g = *GImGui;
		const auto& style = g.Style;

		if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.CurrentColumns) // FIXME-OPT: Avoid if vertically clipped.
			ImGui::PushColumnsBackground();

		ImGuiID id = window->GetID(label);
		ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
		ImVec2 size(size_arg.x != 0.0f ? size_arg.x : label_size.x, size_arg.y != 0.0f ? size_arg.y : label_size.y);
		ImVec2 pos = window->DC.CursorPos;
		pos.y += window->DC.CurrLineTextBaseOffset;
		ImRect bb_inner(pos, pos + size);
		ImGui::ItemSize(size);

		// Fill horizontal space.
		ImVec2 window_padding = window->WindowPadding;
		float max_x = (flags & ImGuiSelectableFlags_SpanAllColumns) ? ImGui::GetWindowContentRegionMax().x : ImGui::GetContentRegionMax().x;
		float w_draw = ImMax(label_size.x, window->Pos.x + max_x - window_padding.x - pos.x);
		ImVec2 size_draw((size_arg.x != 0 && !(flags & ImGuiSelectableFlags_DrawFillAvailWidth)) ? size_arg.x : w_draw, size_arg.y != 0.0f ? size_arg.y : size.y);
		ImRect bb(pos, pos + size_draw);
		if (size_arg.x == 0.0f || (flags & ImGuiSelectableFlags_DrawFillAvailWidth))
			bb.Max.x += window_padding.x;

		// Selectables are tightly packed together so we extend the box to cover spacing between selectable.
		const float spacing_x = style.ItemSpacing.x;
		const float spacing_y = style.ItemSpacing.y;
		const float spacing_L = (float)(int)(spacing_x * 0.50f);
		const float spacing_U = (float)(int)(spacing_y * 0.50f);
		bb.Min.x -= spacing_L;
		bb.Min.y -= spacing_U;
		bb.Max.x += (spacing_x - spacing_L);
		bb.Max.y += (spacing_y - spacing_U);

		bool item_add;
		if (flags & ImGuiSelectableFlags_Disabled)
		{
			ImGuiItemFlags backup_item_flags = window->DC.ItemFlags;
			window->DC.ItemFlags |= ImGuiItemFlags_Disabled | ImGuiItemFlags_NoNavDefaultFocus;
			item_add = ImGui::ItemAdd(bb, id);
			window->DC.ItemFlags = backup_item_flags;
		}
		else
		{
			item_add = ImGui::ItemAdd(bb, id);
		}
		if (!item_add)
		{
			if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.CurrentColumns)
				ImGui::PopColumnsBackground();
			return false;
		}

		// We use NoHoldingActiveID on menus so user can click and _hold_ on a menu then drag to browse child entries
		ImGuiButtonFlags button_flags = 0;
		if (flags & ImGuiSelectableFlags_NoHoldingActiveID) button_flags |= ImGuiButtonFlags_NoHoldingActiveID;
		if (flags & ImGuiSelectableFlags_PressedOnClick) button_flags |= ImGuiButtonFlags_PressedOnClick;
		if (flags & ImGuiSelectableFlags_PressedOnRelease) button_flags |= ImGuiButtonFlags_PressedOnRelease;
		if (flags & ImGuiSelectableFlags_Disabled) button_flags |= ImGuiButtonFlags_Disabled;
		if (flags & ImGuiSelectableFlags_AllowDoubleClick) button_flags |= ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick;
		if (flags & ImGuiSelectableFlags_AllowItemOverlap) button_flags |= ImGuiButtonFlags_AllowItemOverlap;

		if (flags & ImGuiSelectableFlags_Disabled)
			selected = false;

		const bool was_selected = selected;
		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, button_flags);
		// Hovering selectable with mouse updates NavId accordingly so navigation can be resumed with gamepad/keyboard (this doesn't happen on most widgets)
		if (pressed || hovered)
			if (!g.NavDisableMouseHover && g.NavWindow == window && g.NavLayer == window->DC.NavLayerCurrent)
			{
				g.NavDisableHighlight = true;
				ImGui::SetNavID(id, window->DC.NavLayerCurrent);
			}
		if (pressed)
			ImGui::MarkItemEdited(id);

		if (flags & ImGuiSelectableFlags_AllowItemOverlap)
			ImGui::SetItemAllowOverlap();

		// In this branch, Selectable() cannot toggle the selection so this will never trigger.
		if (selected != was_selected) //-V547
			window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_ToggledSelection;

		// Render
		if (hovered || selected)
		{
			const ImU32 col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
			ImGui::RenderFrame(bb.Min, bb.Max, col, false, 0.0f);
			ImGui::RenderNavHighlight(bb, id, ImGuiNavHighlightFlags_TypeThin | ImGuiNavHighlightFlags_NoRounding);
		}
		else
			ImGui::RenderFrame(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_HeaderActive), false, 0.0f);

		if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.CurrentColumns)
		{
			ImGui::PopColumnsBackground();
			bb.Max.x -= (ImGui::GetContentRegionMax().x - max_x);
		}

		if (flags & ImGuiSelectableFlags_Disabled) ImGui::PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_TextDisabled]);
		ImGui::RenderTextClipped(bb_inner.Min, bb_inner.Max, label, NULL, &label_size, style.SelectableTextAlign, &bb);
		if (flags & ImGuiSelectableFlags_Disabled) ImGui::PopStyleColor();

		// Automatically close popups
		if (pressed && (window->Flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiSelectableFlags_DontClosePopups) && !(window->DC.ItemFlags & ImGuiItemFlags_SelectableDontClosePopup))
			ImGui::CloseCurrentPopup();

		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags);
		return pressed;
	}

	void ColorPicker( const char* text, color_var* cv_col, float spaces )
	{
		const auto window = ImGui::GetCurrentWindow();
		if ( window->SkipItems )
			return;

		auto& g = *GImGui;
		const auto& style = g.Style;

		ImGui::PushID( text );
		if ( spaces > -1 )
			ImGui::SameLine( spaces, style.ItemInnerSpacing.x );

		const ImVec4 col_display( static_cast< float >( cv_col->color().r() ) / 255.f, static_cast< float >( cv_col->color().g() ) / 255.f, static_cast< float >( cv_col->color().b() ) / 255.f, static_cast< float >( cv_col->color().a() ) / 255.f );
		if ( ImGui::ColorButton( text, col_display ) )
			ImGui::OpenPopup( text );

		if ( ImGui::BeginPopup( text, ImGuiWindowFlags_NoMove) )
		{
			//decrypts(0)
			ImGuiEx::Radio( XorStr("Static Color"), &cv_col->i_mode, 0 );
			ImGuiEx::Radio( XorStr("Rainbow"), &cv_col->i_mode, 1 );
			//encrypts(0)

			if ( cv_col->i_mode == 1 )
			{
				ImGui::PushItemWidth( 220 );
				//decrypts(0)
				ImGui::SliderFloat( XorStr("##RAINBOWSPEED"), &cv_col->f_rainbow_speed, 0.f, 1.f, XorStr("SPEED: %0.2f") );
				ImGui::SliderFloat( XorStr("##ALPHA"), &cv_col->col_color.Value.w, 0.f, 1.f, XorStr("ALPHA: %0.2f") );
				//encrypts(0)
				ImGui::PopItemWidth();
			}
			else
				ImGuiEx::ColorPicker4( reinterpret_cast< float* >( cv_col ) );

			ImGui::EndPopup();
		}

		ImGui::PopID();
	}

	void ColorPicker( const char* text, health_color_var* cv_col, float spaces )
	{
		const auto window = ImGui::GetCurrentWindow();
		if ( window->SkipItems )
			return;

		auto& g = *GImGui;
		const auto& style = g.Style;

		ImGui::PushID( text );
		if ( spaces > -1 )
			ImGui::SameLine( spaces, style.ItemInnerSpacing.x );

		const ImVec4 col_display( static_cast< float >( cv_col->color(100).r() ) / 255.f, static_cast< float >( cv_col->color(100).g() ) / 255.f, static_cast< float >( cv_col->color(100).b() ) / 255.f, static_cast< float >( cv_col->color(100).a() ) / 255.f );
		if ( ImGui::ColorButton( text, col_display ) )
			ImGui::OpenPopup( text );

		if ( ImGui::BeginPopup( text, ImGuiWindowFlags_NoMove ) )
		{
			//decrypts(0)
			ImGuiEx::Radio( XorStr("Static Color"), &cv_col->i_mode, 0 );
			ImGuiEx::Radio( XorStr("Health-Based"), &cv_col->i_mode, 1 );
			ImGuiEx::Radio( XorStr("Rainbow"), &cv_col->i_mode, 2 );
			//encrypts(0)

			if ( cv_col->i_mode == 0 )
				ImGuiEx::ColorPicker4( reinterpret_cast< float* >( cv_col ) );
			else if ( cv_col->i_mode == 2 )
			{
				ImGui::PushItemWidth( 220 );
				//decrypts(0)
				ImGui::SliderFloat( XorStr("##RAINBOWSPEED"), &cv_col->f_rainbow_speed, 0.f, 1.f, XorStr("SPEED: %0.2f") );
				ImGui::SliderFloat( XorStr("##ALPHA"), &cv_col->col_color.Value.w, 0.f, 1.f, XorStr("ALPHA: %0.2f") );
				//encrypts(0)
				ImGui::PopItemWidth();
			}
			else
			{
				ImGui::PushItemWidth( 220 );
				//decrypts(0)
				ImGui::SliderFloat( XorStr("##ALPHA"), &cv_col->col_color.Value.w, 0.f, 1.f, XorStr("ALPHA: %0.2f") );
				//encrypts(0)
				ImGui::PopItemWidth();
			}

			ImGui::EndPopup();
		}

		ImGui::PopID();
	}

	/*void ButtonConfigChams(const char* text, variable::struct_visual::struct_esp_player::struct_chams* cfg, float spaces)
	{
		const auto window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return;

		auto& g = *GImGui;
		const auto& style = g.Style;

		ImGui::PushID(text);
		if (spaces > -1.f)
			ImGui::SameLine(spaces, style.ItemInnerSpacing.x - 90);

		if (ImGui::Button(adr_util::string::format(XorStrCT("Material Config##%s"), text).c_str(), ImVec2(90, 20)))
			ImGui::OpenPopup(text);

		if (ImGui::BeginPopup(text, ImGuiWindowFlags_NoMove))
		{
			ImGui::PushItemWidth(160);
			ImGui::Checkbox(adr_util::string::format(XorStrCT("Additive##%s"), text).c_str(), &cfg->b_additive);
			ImGui::Checkbox(adr_util::string::format(XorStrCT("Flat##%s"), text).c_str(), &cfg->b_flat);
			ImGui::Checkbox(adr_util::string::format(XorStrCT("Colorize Reflection ##%s"), text).c_str(), &cfg->b_color_reflection);
			SetTip(XorStrCT("Better used with High Tint"));

			ImGui::SliderFloat(adr_util::string::format(XorStrCT("##Reflectivity##%s"), text).c_str(), &cfg->f_reflectivity, 0.f, 1.f, XorStrCT("Reflection: %0.2f"));
			SetTip(XorStrCT("Better used with no Flat"));
			ImGui::SliderFloat(adr_util::string::format(XorStrCT("##LightBoost##%s"), text).c_str(), &cfg->f_light_boost, 0.f, 1.f, XorStrCT("Light: %0.2f"));
			SetTip(XorStrCT("Better used with no Flat"));
			ImGui::SliderFloat(adr_util::string::format(XorStrCT("##Phong##%s"), text).c_str(), &cfg->f_phong_boost, 0.f, 1.f, XorStrCT("Diffusion: %0.2f"));
			SetTip(XorStrCT("Better used with no Flat"));
			ImGui::SliderFloat(adr_util::string::format(XorStrCT("##Tint##%s"), text).c_str(), &cfg->f_envmap_tint, 0.f, 1.f, XorStrCT("Highlights: %0.2f"));
			SetTip(XorStrCT("Better used with no Flat"));

			ImGui::PopItemWidth();
			ImGui::EndPopup();
		}

		ImGui::PopID();
	}*/
}