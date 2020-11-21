#include "stdafx.h"
#include "DebugManager.h"
#include "../imgui/ImGuiManager.h"
#include "../Managers/EventManager.h"
#include "../Rendering/Buffer.h"
#include "../Rendering/Shader.h"
#include "../Rendering/Depot.h"
#include "../Rendering/Unit.h"

template<typename T>
DebugManager::Var::Var(T v, const char* name, float increment): m_value(v), m_name(name), m_increment(increment) { }

DebugManager::DebugManager():
m_variables(),
m_increment(1.0f),
m_vertexShader(Vertex::getVertexShader()),
m_fragmentShader(Vertex::getFragmentShader()),
m_vertexBuffer(nullptr),
m_indexBuffer(nullptr)
{
	ResourcePtr<ImGuiManager>()->registerCallback({ [](void* v) { static_cast<DebugManager*>(v)->imgui(); }, (void*)nullptr });

	ResourcePtr<EventManager> events;
	events->addListener<RenderEvent>([this](RenderEvent* e) { render(e); });
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

void DebugManager::addLine3D(const glm::vec4& p1, const glm::vec4& p2, const glm::vec4& c)
{
	Line l;
	l.m_positions[0] = p1;
	l.m_positions[1] = p2;
	l.m_colour = c;
	m_lines.push_back(l);
}

void DebugManager::render(RenderEvent* e)
{
	if (m_lines.empty()) return;

	ensureBufferSizes(m_lines.size() * sizeof(Vertex), m_lines.size() * sizeof(short));
	Vertex* vertex = (Vertex*)m_vertexBuffer->map();
	short* index = (short*)m_indexBuffer->map();
	short currentIndex = 0;
	for(const auto& line : m_lines)
	{
		Vertex v;
		v.m_position = line.m_positions[0];
		v.m_colour = line.m_colour;
		*vertex = v;
		vertex += 1;

		v.m_position = line.m_positions[1];
		*vertex = v;
		vertex += 1;

		*index = currentIndex++;
		index++;
		*index = currentIndex++;
		index++;
	}
	m_vertexBuffer->unmap();
	m_indexBuffer->unmap();
	m_lines.clear();

	std::vector<char> pushData(sizeof(glm::mat4));
	memcpy(&pushData[0], &e->m_projection, sizeof(glm::mat4));

	Rendering::Unit unit;
	unit.in(m_vertexShader);
	unit.in(m_fragmentShader);
	unit.in(&(*m_vertexBuffer));
	unit.in(&(*m_indexBuffer));
	unit.in({ vk::ShaderStageFlagBits::eVertex, std::move(pushData) });
	// unit.in({ 4, 1, (uint32_t)i++ * 4, 0 });
	unit.submit();
}

void DebugManager::ensureBufferSizes(std::size_t vSize, std::size_t iSize)
{
	if (!m_vertexBuffer || !m_indexBuffer|| m_vertexBuffer->getSize() < vSize || m_indexBuffer->getSize() < iSize)
	{
		if (!m_vertexBuffer)
			m_vertexBuffer = std::make_shared<Rendering::Buffer>(Rendering::Buffer::Vertex, Rendering::Buffer::Mapped, sizeof(Vertex) * 1024);

		if (m_indexBuffer)
			m_indexBuffer = std::make_shared<Rendering::Buffer>(Rendering::Buffer::Index, Rendering::Buffer::Mapped, sizeof(short) * 1024);

		m_vertexBuffer->grow(vSize);
		m_indexBuffer->grow(iSize);
	}
}