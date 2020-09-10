#include "stdafx.h"
#include "DebugManager.h"
#include "../imgui/ImGuiManager.h"

template<typename T>
DebugManager::Var::Var(T v, const char* name, float increment): m_value(v), m_name(name), m_increment(increment) { }

DebugManager::DebugManager():
m_variables(),
m_increment(1.0f)
{
	ResourcePtr<ImGuiManager>()->registerCallback({ [](void* v) { static_cast<DebugManager*>(v)->imgui(); }, this });
}

DebugManager::~DebugManager()
{
}

void DebugManager::debug(float* v, const char* name, const char* file) { m_variables[file].emplace_back(v, name, m_increment); }
void DebugManager::debug(int* v, const char* name, const char* file) { m_variables[file].emplace_back(v, name, m_increment); }
void DebugManager::debug(std::string* v, const char* name, const char* file) { m_variables[file].emplace_back(v, name, m_increment); }
void DebugManager::debug(glm::vec2* v, const char* name, const char* file) { m_variables[file].emplace_back(v, name, m_increment); }
void DebugManager::debug(glm::vec3* v, const char* name, const char* file) { m_variables[file].emplace_back(v, name, m_increment); }
void DebugManager::debug(glm::vec4* v, const char* name, const char* file) { m_variables[file].emplace_back(v, name, m_increment); }
void DebugManager::debug(glm::mat4* v, const char* name, const char* file) { m_variables[file].emplace_back(v, name, m_increment); }
void DebugManager::debug(float v, const char* name, const char* file) { m_variables[file].emplace_back(v, name, m_increment); }
void DebugManager::debug(int v, const char* name, const char* file) { m_variables[file].emplace_back(v, name, m_increment); }
void DebugManager::debug(const std::string& v, const char* name, const char* file) { m_variables[file].emplace_back(v, name, m_increment); }
void DebugManager::debug(const glm::vec2& v, const char* name, const char* file) { m_variables[file].emplace_back(v, name, m_increment); }
void DebugManager::debug(const glm::vec3& v, const char* name, const char* file) { m_variables[file].emplace_back(v, name, m_increment); }
void DebugManager::debug(const glm::vec4& v, const char* name, const char* file) { m_variables[file].emplace_back(v, name, m_increment); }
void DebugManager::debug(const glm::mat4& v, const char* name, const char* file) { m_variables[file].emplace_back(v, name, m_increment); }
void DebugManager::setIncrement(float f) { m_increment = f; }

void DebugManager::imgui()
{
	for (auto fileIt = m_variables.begin(); fileIt != m_variables.end(); ++fileIt)
	{
		ImGui::Begin(fileIt->first);

		for (auto varIt = fileIt->second.begin(); varIt != fileIt->second.end(); ++varIt)
		{
			Any& value = varIt->m_value;
			if (auto v = value.getPtr<float*>()) ImGui::DragFloat(varIt->m_name, *v, varIt->m_increment);
			else if (auto v = value.getPtr<int*>()) ImGui::DragInt(varIt->m_name, *v, varIt->m_increment);
			else if (auto v = value.getPtr<glm::vec2*>()) ImGui::DragFloat2(varIt->m_name, &(*v)->x, varIt->m_increment);
			else if (auto v = value.getPtr<glm::vec3*>()) ImGui::DragFloat3(varIt->m_name, &(*v)->x, varIt->m_increment);
			else if (auto v = value.getPtr<glm::vec4*>()) ImGui::DragFloat4(varIt->m_name, &(*v)->x, varIt->m_increment);
			else if (auto v = value.getPtr<float>()) ImGui::DragFloat(varIt->m_name, v, varIt->m_increment);
			else if (auto v = value.getPtr<int>()) ImGui::DragInt(varIt->m_name, v, varIt->m_increment);
			else if (auto v = value.getPtr<glm::vec2>()) ImGui::DragFloat2(varIt->m_name, &v->x, varIt->m_increment);
			else if (auto v = value.getPtr<glm::vec3>()) ImGui::DragFloat3(varIt->m_name, &v->x, varIt->m_increment);
			else if (auto v = value.getPtr<glm::vec4>()) ImGui::DragFloat4(varIt->m_name, &v->x, varIt->m_increment);
			else if (auto v = value.getPtr<std::string>()) ImGui::InputText(varIt->m_name, const_cast<char*>(v->c_str()), v->size(), ImGuiInputTextFlags_ReadOnly);
			else if (auto v = value.getPtr<glm::mat4*>()) {
				ImGui::DragFloat4(varIt->m_name, &((**v)[0].x), varIt->m_increment);
				ImGui::DragFloat4(varIt->m_name, &((**v)[1].x), varIt->m_increment);
				ImGui::DragFloat4(varIt->m_name, &((**v)[2].x), varIt->m_increment);
				ImGui::DragFloat4(varIt->m_name, &((**v)[3].x), varIt->m_increment);
			}
			else if (auto v = value.getPtr<glm::mat4>()) {
				ImGui::DragFloat4(varIt->m_name, &((*v)[0].x), varIt->m_increment);
				ImGui::DragFloat4(varIt->m_name, &((*v)[1].x), varIt->m_increment);
				ImGui::DragFloat4(varIt->m_name, &((*v)[2].x), varIt->m_increment);
				ImGui::DragFloat4(varIt->m_name, &((*v)[3].x), varIt->m_increment);
			}
			else if (auto v = value.getPtr<std::string*>())
			{
				const std::size_t additionalSize = 128;
				char* t = (char*)alloca((*v)->size() + additionalSize);
				memcpy(t, (*v)->c_str(), (*v)->size() + 1);
				if(ImGui::InputText(varIt->m_name, t, (*v)->size() + additionalSize))
					**v = t;
			}
			else ImGui::Text("Unknown Variable Type: %s", varIt->m_name);
		}

		ImGui::End();
	}

	m_variables.clear();
}