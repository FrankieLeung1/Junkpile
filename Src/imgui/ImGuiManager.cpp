#include "stdafx.h"
#include "ImGuiManager.h"
#include "imgui_internal.h"
#include "../Files/FileManager.h"
#include "../LuaHelpers.h"
#include "../Managers/InputManager.h"
#include "../Managers/TimeManager.h"
#include "imgui_impl_glfw.h"

ImGuiManager::ImGuiManager() :
m_persistence(NewPtr, "imgui.lua", File::CreateIfDoesNotExist),
m_nextToastId(0),
m_toastRight(0)
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
		ResourcePtr<EventManager> events;
		events->processEventImmediately(&ImGuiRenderEvent{});
	}


	const float toastSpacing = 10.0f;
	std::vector<int> closed;
	for (auto& it : m_toasts)
	{
		bool opened = true;
		if (ImGui::Begin(stringf("%s##toast%d", it.m_title.c_str(), it.m_id).c_str(), &opened, it.m_flags))
		{
			bool focused = ImGui::IsWindowFocused();
			it.m_render(&opened);

			ImGuiContext* context = ImGui::GetCurrentContext();
			ImGuiWindow* window = ImGui::GetCurrentWindowRead();
			if (window)
			{
				ResourcePtr<FrameworkClass> framework;
				if (it.m_bottom < 0.0f)
				{
					float top = std::numeric_limits<float>::max();
					for (auto& it2 : m_toasts)
						if(it2.m_bottom >= 0.0f)
							top = std::min(top, it2.m_bottom - it2.m_height);

					if (top == std::numeric_limits<float>::max())
					{
						// no other toasts exist, init some vars
						glm::vec4 rect = framework->getDesktopRect();
						it.m_bottom = rect.w - toastSpacing;
						m_toastRight = rect.z - toastSpacing;
					}
					else if (top - toastSpacing - window->Size.y <= 0.0f)
					{
						// toast would be above the top of the screen, skip
						goto windowEnd;
					}
					else
					{
						it.m_bottom = top - toastSpacing;
					}
				}

				ImVec2 pos(m_toastRight - window->Size.x, it.m_bottom - window->Size.y);
				ImGui::SetWindowPos(window, window->Appearing ? ImVec2(9999, 9999) : pos, 0);
				it.m_height = window->Size.y;

				if (focused && !it.m_title.empty())
					it.m_lifeTime = -1.0f;

				if (it.m_lifeTime > 0.0f)
				{
					ResourcePtr<TimeManager> time;
					float alive = time->getTime() - it.m_startTime;
					float fraction = alive / it.m_lifeTime;
					ImGui::ProgressBar(1.0f - fraction, ImVec2(window->ContentSize.x - window->WindowPadding.x, 3.0f), "");
					if (fraction >= 1.0f)
						opened = false;
				}
				else
				{
					ImGui::Dummy(ImVec2(-FLT_MIN, 3.0f));
				}
			}
		}

	windowEnd:
		ImGui::End();

		if (!opened)
			closed.push_back(it.m_id);
	}

	for (auto& id : closed)
	{
		m_toasts.remove_if([id](const ToastData& d) { return d.m_id == id; });
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

void ImGuiManager::newToast(const char* name, const std::function<void(bool*)>& r, float lifetime, ImGuiWindowFlags flags)
{
	if(!name || name[0] == '\0')
		flags |= ImGuiWindowFlags_NoTitleBar;

	flags |= ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing;
	m_toasts.emplace_front(name ? name : "", m_nextToastId++, flags, r, lifetime, ResourcePtr<TimeManager>()->getTime());
}

void ImGuiManager::bringToFront()
{
	ImGui_ImplGlfw_BringAllToFront();
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
	return;

	ImVec2 pos;
	ResourcePtr<InputManager> input;
	if (input->wantsTrayContext())
	{
		ResourcePtr<FrameworkClass> f;
		pos = ImGui::GetIO().MousePos;
		pos.y = f->getDesktopRect().w;
		ImGui::OpenPopup("testWindow");

		input->setWantsTrayContext(false);
	}

	if (ImGui::BeginPopup("testWindow"))
	{
		ImVec2 size = ImGui::GetWindowSize();
		ImGui::SetWindowPos({ pos.x, pos.y - size.y });

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