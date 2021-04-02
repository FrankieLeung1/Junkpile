#include "stdafx.h"
#include "Standalone.h"
#include "../imgui/ImGuiManager.h"
#include "../Framework/VulkanFramework.h"
#include "../Scripts/ScriptManager.h"

Standalone::Standalone():
m_current(-1),
m_currentStyle(0),
m_scriptingExample(false),
m_scriptLoaded(false)
{
	m_windows.emplace_back("Demo", []() { ImGui::ShowDemoWindow(); });
	m_windows.emplace_back("Web", [this]() { m_server.imgui(); });
	m_windows.emplace_back("Level", [this]() { m_ge.imgui(); });
	m_windows.emplace_back("Analytics", [this]() { m_analytics.imgui(); });
	m_windows.emplace_back("Narrative", [this]() { m_narrative.imgui(); });
	// gif
	// sprite editor

	// TRAY
	// save select, debugging
}

Standalone::~Standalone()
{

}

void Standalone::imgui()
{
	bool opened = true;
	if (ImGui::Begin("Standalone Demo", &opened))
	{
		if (ImGui::Combo("Style", &m_currentStyle, "ImGui\0Light\0Dark\0\0"))
		{
			ResourcePtr<ImGuiManager> im;
			switch (m_currentStyle)
			{
			case 0: im->setDefaultStyle(); break;
			case 1: im->setLightStyle(); break;
			case 2: im->setDarkStyle(); break;
			}
		}

		ImGui::Columns(2);
		for(int i = 0; i < m_windows.size(); i++)
		{
			const char* name = m_windows[i].first;
			bool current = (i == m_current);

			ImGui::Text(name);
			ImGui::NextColumn();
			ImGui::PushID(name);
			if (ImGui::Button(current ? "Stop" : "Start"))
				m_current = (current ? -1 : i);

			ImGui::PopID();
			ImGui::NextColumn();
		}

		ImGui::Text("Scripting");
		ImGui::NextColumn();
		if (ImGui::Button(m_scriptingExample ? "Stop##Scripting" : "Start##Scripting"))
			m_scriptingExample = true;

		ImGui::NextColumn();
		ImGui::Columns(1);
	}
	ImGui::End();

	if (m_scriptingExample)
	{
		if (!m_scriptLoaded)
		{
			ResourcePtr<ScriptManager> scripts;
			scripts->run("../res/imguiscript.py");
			scripts->setEditorContent(nullptr, "../res/imguiscript.py");
			scripts->showEditor();
			m_scriptLoaded = true;
		}
	}

	if (!opened)
		ResourcePtr<VulkanFramework>()->setShouldQuit(true);

	if (m_current >= 0)
		m_windows[m_current].second();
}