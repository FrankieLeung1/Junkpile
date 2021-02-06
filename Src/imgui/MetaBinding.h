#pragma once

#include "../Meta/Meta.h"
#include "imgui/imgui.h"
struct ImGuiMeta {};
template<>
inline Meta::Object Meta::instanceMeta<ImGuiMeta>()
{
	void(*text)(const char*) = [](const char* t) { ImGui::Text(t); };
	return Object("ImGui").
		func("Begin", &ImGui::Begin, { "name", "p_open", "flags" }, { (bool*)nullptr, 0 }).
		func("End", &ImGui::End).
		func("Text", text, { "text" }).
		func("Button", &ImGui::Button, { "label", "size" }, { ImVec2(0, 0) }).
		func("SmallButton", &ImGui::SmallButton, { "label" }).
		func("Separator", &ImGui::Separator, {});
}

template<>
inline Meta::Object Meta::instanceMeta<ImVec2>()
{
	return Meta::Object("ImVec2").
		var("x", &ImVec2::x).
		var("y", &ImVec2::y);
}