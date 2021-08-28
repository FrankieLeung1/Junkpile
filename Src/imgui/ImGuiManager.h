#pragma once

#include "../Misc/Callbacks.h"
#include "../Resources/ResourceManager.h"
#include "../Managers/EventManager.h"

namespace Rendering
{
	class Texture;
	class Device;
}

struct ImGuiRenderEvent : public Event<ImGuiRenderEvent>
{
};
template<> inline Meta::Object Meta::instanceMeta<ImGuiRenderEvent>() { return Meta::Object("ImGuiRenderEvent"); }

class FileManager;
class LuaTableResource;
class ImGuiManager : public SingletonResource<ImGuiManager>
{
public:
	ImGuiManager();
	~ImGuiManager();

	void setDarkStyle();
	void setLightStyle();
	void setDefaultStyle();

	void setPersistenceFilePath(const char* persistencePath = "imgui.lua");

	void update();

	bool* win(const char* name, const char* category = nullptr);

	void saveOpenedWindows();

	void setPipDisable(bool);

	void newToast(const char* name, const std::function<void(bool* opened)>&, float lifetime = 5.0f, ImGuiWindowFlags flags = 0);

	void bringToFront();

	template<typename Pred>
	void imguiLoop(Pred& pred);

public:
	void drawMainMenuBar();
	void drawFramerate();
	void drawTrayContext();

protected:
	ResourcePtr<FileManager> m_fileManager;
	ResourcePtr<LuaTableResource> m_persistence;

	// std::string = category, std::string = name
	std::map<std::string, std::map<std::string, bool> > m_menuBarSubWindows;
	std::map<std::string, bool> m_menuBarWindows;

	struct ToastData
	{
		int m_id;
		std::string m_title;
		ImGuiWindowFlags m_flags;
		std::function<void(bool*)> m_render;
		float m_height, m_bottom;
		float m_lifeTime, m_startTime;
		ToastData(const char* title, int id, ImGuiWindowFlags flags, const std::function<void(bool*)>& render, float lifetime, float start):
			m_title(title), m_id(id), m_flags(flags), m_render(render), m_height(-1.0f), m_bottom(-1.0f), m_lifeTime(lifetime), m_startTime(start){}
	};
	int m_nextToastId;
	std::forward_list< ToastData > m_toasts;
	float m_toastRight;

	bool m_pipDisable;
};

inline ImVec2 operator+(const ImVec2& v1, const ImVec2& v2) { return{ v1.x + v2.x, v1.y + v2.y }; }
inline ImVec2 operator-(const ImVec2& v1, const ImVec2& v2) { return{ v1.x - v2.x, v1.y - v2.y }; }

// ----------------------- IMPLEMENTATION ----------------------- 
template<typename Pred>
void ImGuiManager::imguiLoop(Pred& pred)
{
	ResourcePtr<FrameworkClass> fc;
	while (!pred())
	{
		update();
		fc->update();
		fc->render();

		if (fc->shouldQuit())
			exit(0); // dirty exit if user closes the window during the imguiLoop
	}
}