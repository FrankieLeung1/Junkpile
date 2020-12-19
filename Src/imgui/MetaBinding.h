#pragma once

#include "../Meta/Meta.h"
#include "imgui/imgui.h"
struct ImGuiMeta {};
template<>
inline Meta::Object Meta::instanceMeta<ImGuiMeta>()
{
	return Object("ImGui").
		func("Begin", &ImGui::Begin, { "name", "p_open", "flags" }, { nullptr, 0 });
		//func("End", &ImGui::End);
}