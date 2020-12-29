#include "stdafx.h"
#include "ImGuiManager.h"
#include "../Files/FileManager.h"
#include "../LuaHelpers.h"
#include "../Managers/InputManager.h"

ImGuiManager::ImGuiManager() :
m_persistence(NewPtr, "imgui.lua", File::CreateIfDoesNotExist)
{
	//setMicrosoftStyle();

	ResourcePtr<EventManager> events;
	events->addListener<UpdateEvent>([this](UpdateEvent*) { update(); });
}

ImGuiManager::~ImGuiManager()
{
	saveOpenedWindows();
}

void ImGuiManager::update()
{
	ResourcePtr<FrameworkClass> vf;

	vf->newFrameImGui();
	ImGui::NewFrame();

	if (!m_pipDisable)
	{
		for (BasicFunction<void>& fn : m_renderCallbacks)
			fn.call();
	}

	ImGui::EndFrame();
}

bool* ImGuiManager::win(const char* name, const char* category)
{
	if (!m_persistence.released())
	{
		std::tuple<int, std::string> error;
		m_persistence.error(&error);
		if (std::get<int>(error) == 0)
		{
			LuaStackCheck lsc(*m_persistence);
			LuaIterator it(*m_persistence);
			while (it.next())
			{
				if (it.isValueType<bool>())
				{
					std::string key = it.getKey<std::string>();
					bool value = it.getValue<bool>();
					m_menuBarWindows[key] = value;
				}
				else if (it.isValueType<LuaTable>())
				{
					LuaIterator it2(it.getValue<LuaTable>());
					while(it2.next())
					{
						auto cat = it.getKey<std::string>();
						auto name = it2.getKey<std::string>();
						m_menuBarSubWindows[it.getKey<std::string>()][it2.getKey<std::string>()] = it2.getValue<bool>();
					}
				}
			}
		}

		m_persistence.release();
	}

	if (category)
		return &m_menuBarSubWindows[category][name];
	else
		return &m_menuBarWindows[name];
}

void ImGuiManager::saveOpenedWindows()
{
	std::stringstream ss;
	lua_State* l = luaL_newstate();
	{
		LuaStackCheck lsc(l);

		LuaTable t(l);
		for (auto it = m_menuBarSubWindows.begin(); it != m_menuBarSubWindows.end(); ++it)
		{
			LuaTable category(t.getLuaState());
			for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
			{
				category.set(it2->first.c_str(), it2->second);
			}
			t.set(it->first.c_str(), category);
		}

		for (auto it = m_menuBarWindows.begin(); it != m_menuBarWindows.end(); ++it)
		{
			t.set(it->first.c_str(), it->second);
		}

		ss << "return ";
		t.serialize(ss, LuaTable::SerializeMode::Readable);
	}
	lua_close(l);

	const std::string& s = ss.str();
	std::vector<char> contents(s.begin(), s.end());
	m_fileManager->save("imgui.lua", std::move(contents));
}

void ImGuiManager::setPipDisable(bool b)
{
	m_pipDisable = b;
}

void ImGuiManager::registerCallback(BasicFunction<void> f)
{
	m_renderCallbacks.push_back(f);
}

void ImGuiManager::drawMainMenuBar()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit", "Alt+F4"))
				ResourcePtr<VulkanFramework>()->setShouldQuit(true);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Windows"))
		{
			for (auto& it : m_menuBarSubWindows)
			{
				if (ImGui::BeginMenu(it.first.c_str()))
				{
					for (auto& it2 : it.second)
					{
						if (ImGui::MenuItem(it2.first.c_str(), nullptr, it2.second))
						{
							it2.second = !it2.second;
						}
					}

					ImGui::EndMenu();
				}
			}

			for (auto& it : m_menuBarWindows)
			{
				if (ImGui::MenuItem(it.first.c_str(), nullptr, it.second))
				{
					it.second = !it.second;
				}
			}

			ImGui::EndMenu();
		}
	} ImGui::EndMainMenuBar();
}

void ImGuiManager::drawFramerate()
{
	if (*win("Framerate"))
	{
		ImGuiViewport* vp = ImGui::GetMainViewport();
		float width = vp->Size.x;
		ImGui::SetNextWindowPos(vp->Pos + ImVec2(width - 150.0f, 20.0f));
		ImGui::SetNextWindowSize(ImVec2(130.0f, 20.0f));
		ImGui::SetNextWindowBgAlpha(0.25f);
		ImGui::Begin("Framerate", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs);
		ImGui::Text("Framerate %0.2f", ImGui::GetIO().Framerate);
		ImGui::End();
	}
}

void ImGuiManager::drawTrayContext()
{
	ResourcePtr<InputManager> input;
	static bool testOpened = true;
	if (input->wantsTrayContext())
	{
		testOpened = true;
		ImGui::SetNextWindowPos(ImGui::GetIO().MousePos);
		ImGui::OpenPopup("testWindow");

		input->setWantsTrayContext(false);
	}

	if (ImGui::BeginPopup("testWindow"))
	{
		ImGui::Text("Just a normal ImGui popup");
		ImGui::Text("Mouse x:%d y:%d", (int)ImGui::GetIO().MousePos.x, (int)ImGui::GetIO().MousePos.y);
		ImGui::Separator();

		ImGui::Text("ImGui");
		ImGui::Text("Is");
		ImGui::Text("Actually");
		ImGui::Text("Kinda");
		ImGui::Text("Insane");

		ImGui::Separator();
		if (ImGui::MenuItem("Quit"))
		{
			ResourcePtr<VulkanFramework>()->setShouldQuit(true);
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void ImGuiManager::setPersistenceFilePath(const char* persistencePath)
{
	m_persistence = ResourcePtr< LuaTableResource>(NewPtr, persistencePath, File::CreateIfDoesNotExist);
}

void ImGuiManager::setDefaultStyle()
{
	ImGui::StyleColorsClassic();
}

void ImGuiManager::setDarkStyle()
{
	ImGui::StyleColorsDark();
	return;

	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
	colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
	colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.21f, 0.22f, 0.54f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.40f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 0.67f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.44f, 0.44f, 0.44f, 0.40f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.46f, 0.47f, 0.48f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.42f, 0.42f, 0.42f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.70f, 0.70f, 0.70f, 0.31f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.70f, 0.70f, 0.70f, 0.80f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.48f, 0.50f, 0.52f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.72f, 0.72f, 0.72f, 0.78f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.46f, 0.46f, 0.46f, 0.95f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.73f, 0.60f, 0.15f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.87f, 0.87f, 0.87f, 0.35f);
	//colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
}

void ImGuiManager::setLightStyle()
{
	ImGui::StyleColorsLight();
}

// IMPROVEMENTS:
// Script editor tags (Python: #`varname` = (`float`, `float`, `float`)) (lua: --local `varname` = (`float`, `float`, `float`))