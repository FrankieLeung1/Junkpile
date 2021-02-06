#include "stdafx.h"
#include "SystemTray.h"
#include "../Misc/Misc.h"
#include "../imgui/Widgets.h"
#include "../imgui/ImGuiManager.h"
#include "../Managers/TimeManager.h"
#include "../Managers/InputManager.h"

SystemTray::SystemTray():
m_currentStyle(0),
m_fileProcessing(false),
m_memoryStore(false)
{
	
}

SystemTray::~SystemTray()
{

}

void SystemTray::imgui()
{
	bool opened = true;
	if (ImGui::Begin("SystemTray Demo", &opened))
	{
		ResourcePtr<ImGuiManager> im;
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

		int selected = -1;
		ImGui::Columns(2);

		ImGui::Text("Notification");
		ImGui::NextColumn();
		if (ImGui::Button("With Title"))
			im->newToast("NOTIFICATION TITLE", [](bool*) { ImGui::Text("THIS IS A NOTIFICATION TEST"); });
		
		ImGui::SameLine();
		if (ImGui::Button("Without Title"))
			im->newToast("", [](bool*) { ImGui::Text("THIS IS A NOTIFICATION TEST"); });

		ImGui::NextColumn();
		ImGui::Text("GIT/SVN Conflict");
		ImGui::NextColumn();
		if (ImGui::Button("Start"))
			im->newToast("TestFile.txt Conflicted", [](bool* b) {
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "C:\\Junkpile\\Res\\TestFile.txt");
				ImGui::TextWrapped("A file is conflicted. What would you like to do?");

				bool pressed = false;
				pressed = ImGui::Button("Use Mine"); ImGui::SameLine();
				pressed = pressed || ImGui::Button("Use Theirs"); ImGui::SameLine();
				pressed = pressed || ImGui::Button("Merge");

				if (pressed) *b = false;
			}, 0);

		ImGui::NextColumn();
		ImGui::Text("Who wants cawfee?");
		ImGui::NextColumn();
		ImGui::PushID("Cawfee");
		if (ImGui::Button("Start"))
		{
			std::shared_ptr<int> stage = std::make_shared<int>();
			std::shared_ptr<float> startTime = std::make_shared<float>();
			im->newToast("Covfefe?", [stage, startTime](bool* b) {
				if (*stage == 0)
				{
					ImGui::Text("It's 4:15pm and Sean is making coffee");
					bool pressed = false;
					if (ImGui::Button("I'm in!"))
					{
						*stage = 1;
						*startTime = ResourcePtr<TimeManager>()->getTime();
					}
					ImGui::SameLine();
					*b = !ImGui::Button("<Silent Headshake No>");
				}
				else
				{
					const float waitTime = 5.0f;
					float time = ResourcePtr<TimeManager>()->getTime();
					float aliveTime = time - *startTime;
					float fraction = (aliveTime - waitTime) / 20.0f;
					ImGui::Text(fraction < 1.0f ? "Coffee in progress..." : "Coffee Time!");
					ImGui::ProgressBar(fraction);
				}
			}, 60.0f);
		}
		ImGui::PopID();

		ImGui::NextColumn();
		ImGui::PushID("Backup");
		ImGui::Text("Backup");
		ImGui::NextColumn();
		if (ImGui::Button("Start"))
		{
			im->newToast("Saves", [](bool* b) {
				ImGui::TestDateChooser();
				if (ImGui::ListBoxHeader("##List"))
				{
					ImGui::Selectable("Current", true);
					ImGui::Selectable("Jan 15, 2021", false);
					ImGui::Selectable("Jan 14, 2021", false);
					ImGui::Selectable("Jan 13, 2021", false);
					ImGui::Selectable("Jan 12, 2021", false);
					ImGui::Selectable("Jan 11, 2021", false);
					ImGui::Selectable("Jan 10, 2021", false);
					ImGui::Selectable("Jan 9, 2021", false);
					ImGui::ListBoxFooter();
				}

				static int current = 0;
				ImGui::Text("Save Every"); ImGui::SameLine();
				ImGui::Combo("##Save Every", &current, "Day\0Week\0Month\0\0");
			}, 0);
		}
		ImGui::PopID();

		ImGui::Columns(1);
	}
	ImGui::End();

	if (m_fileProcessing)
	{
		int noSelection = -1;
		int selection = -1;
		static char* in[] = {"C:/Res/"};
		static char* scripts[] = { "Asset Optimizer", "Script Checker", "Crash Collector" };
		static char* out[] = { "C:/Processed/" };
		if (ImGui::Begin("File Processing", &m_fileProcessing))
		{
			ImGui::Text("File Processor");

			float arrowWidth = 4.0f;
			float availableWidth = ImGui::GetWindowContentRegionWidth() - (arrowWidth * 2);
			float listWidth = availableWidth / 3;

			if (ImGui::BeginPopup("AddRemoveMenu"))
			{
				ImGui::Selectable("Add Path");
				ImGui::Selectable("Remove Path");
				ImGui::EndPopup();
			}

			if (ImGui::BeginPopup("NewScriptMenu"))
			{
				ImGui::Selectable("New Script");
				ImGui::Selectable("Remove Script");
				ImGui::EndPopup();
			}

			ImGui::SetNextItemWidth(listWidth);
			ImGui::ListBox("##in", &noSelection, in, IM_ARRAYSIZE(in));
			ImGui::OpenPopupOnItemClick("AddRemoveMenu", ImGuiPopupFlags_MouseButtonRight);
			
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::SetNextItemWidth(arrowWidth);
			ImGui::Text("=>"); 
			
			ImGui::SameLine();
			ImGui::SetNextItemWidth(listWidth);
			ImGui::ListBox("##script", &selection, scripts, IM_ARRAYSIZE(scripts));
			ImGui::OpenPopupOnItemClick("NewScriptMenu", ImGuiPopupFlags_MouseButtonRight);

			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::SetNextItemWidth(arrowWidth);
			ImGui::Text("=>");
			
			ImGui::SameLine();
			ImGui::SetNextItemWidth(listWidth);
			ImGui::ListBox("##out", &noSelection, out, IM_ARRAYSIZE(out)); ImGui::SameLine();
			ImGui::OpenPopupOnItemClick("AddRemoveMenu", ImGuiPopupFlags_MouseButtonRight);
		}
		ImGui::End();
	}

	if (m_memoryStore)
	{
		if (ImGui::Begin("Memory Store", &m_memoryStore))
		{
			static float f[4] = { 34.1f, 1038.2f, 603.8f, 2.3f };
			ImGui::Text("Buffers: %.1fmb", f[0]);
			ImGui::Text("Textures: %.1fmb", f[1]);
			ImGui::Text("Files: %.1fmb", f[2]);
			ImGui::Separator();
			ImGui::Text("Total %.1fgb", f[3]);
			if (ImGui::Button("Clear"))
				memset(f, 0x00, sizeof(f));
		}
		ImGui::End();
	}

	// Context Menu
	ResourcePtr<InputManager> input;
	if (input->wantsTrayContext())
	{
		ResourcePtr<FrameworkClass> f;
		m_contextPos = ImGui::GetIO().MousePos;
		m_contextPos.y = f->getDesktopRect().w;
		ImGui::OpenPopup("context");

		input->setWantsTrayContext(false);
	}

	if (ImGui::BeginPopup("context"))
	{
		ImVec2 size = ImGui::GetWindowSize();
		ImGui::SetWindowPos({ m_contextPos.x, m_contextPos.y - size.y });

		ImGui::Text("Junkpile");
		ImGui::Text("Mouse x:%d y:%d", (int)ImGui::GetIO().MousePos.x, (int)ImGui::GetIO().MousePos.y);
		ImGui::Separator();

		if (ImGui::Selectable("File Processing"))
			m_fileProcessing = true;

		if (ImGui::Selectable("Memory Store"))
			m_memoryStore = true;

		ImGui::Separator();
		if (ImGui::MenuItem("Quit"))
		{
			ResourcePtr<VulkanFramework>()->setShouldQuit(true);
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	if (!opened)
		ResourcePtr<VulkanFramework>()->setShouldQuit(true);
}