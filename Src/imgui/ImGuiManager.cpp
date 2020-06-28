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

void ImGuiManager::setMicrosoftStyle()
{
	ImGuiIO& io = ImGui::GetIO();

	// TODO: why don't fonts work
	/*io.Fonts->Clear();
	ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
	if (font != NULL) {
		font->DisplayOffset.y -= 1;
		io.FontDefault = font;
	}
	else {
		io.Fonts->AddFontDefault();
	}
	io.Fonts->Build();*/

	ImGuiStyle* style = &ImGui::GetStyle();
	float hspacing = 8.0f;
	float vspacing = 6.0f;
	style->DisplaySafeAreaPadding = ImVec2(0, 0);
	style->WindowPadding = ImVec2(hspacing / 2, vspacing);
	style->FramePadding = ImVec2(hspacing, vspacing);
	style->ItemSpacing = ImVec2(hspacing, vspacing);
	style->ItemInnerSpacing = ImVec2(hspacing, vspacing);
	style->IndentSpacing = 20.0f;

	style->WindowRounding = 0.0f;
	style->FrameRounding = 0.0f;

	style->WindowBorderSize = 0.0f;
	style->FrameBorderSize = 1.0f;
	style->PopupBorderSize = 1.0f;

	style->ScrollbarSize = 20.0f;
	style->ScrollbarRounding = 0.0f;
	style->GrabMinSize = 5.0f;
	style->GrabRounding = 0.0f;

	ImVec4 white = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	ImVec4 transparent = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	ImVec4 dark = ImVec4(0.00f, 0.00f, 0.00f, 0.20f);
	ImVec4 darker = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);

	ImVec4 background = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
	ImVec4 text = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	ImVec4 border = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	ImVec4 grab = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
	ImVec4 header = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
	ImVec4 active = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);
	ImVec4 hover = ImVec4(0.00f, 0.47f, 0.84f, 0.20f);

	style->Colors[ImGuiCol_Text] = text;
	style->Colors[ImGuiCol_WindowBg] = background;
	style->Colors[ImGuiCol_ChildBg] = background;
	style->Colors[ImGuiCol_PopupBg] = white;

	style->Colors[ImGuiCol_Border] = border;
	style->Colors[ImGuiCol_BorderShadow] = transparent;

	style->Colors[ImGuiCol_Button] = header;
	style->Colors[ImGuiCol_ButtonHovered] = hover;
	style->Colors[ImGuiCol_ButtonActive] = active;

	style->Colors[ImGuiCol_FrameBg] = white;
	style->Colors[ImGuiCol_FrameBgHovered] = hover;
	style->Colors[ImGuiCol_FrameBgActive] = active;

	style->Colors[ImGuiCol_MenuBarBg] = header;
	style->Colors[ImGuiCol_Header] = header;
	style->Colors[ImGuiCol_HeaderHovered] = hover;
	style->Colors[ImGuiCol_HeaderActive] = active;

	style->Colors[ImGuiCol_CheckMark] = text;
	style->Colors[ImGuiCol_SliderGrab] = grab;
	style->Colors[ImGuiCol_SliderGrabActive] = darker;

	style->Colors[ImGuiCol_ScrollbarBg] = header;
	style->Colors[ImGuiCol_ScrollbarGrab] = grab;
	style->Colors[ImGuiCol_ScrollbarGrabHovered] = dark;
	style->Colors[ImGuiCol_ScrollbarGrabActive] = darker;
}

// IMPROVEMENTS:
// Script editor tags (Python: #`varname` = (`float`, `float`, `float`)) (lua: --local `varname` = (`float`, `float`, `float`))