#pragma once

#include "../Misc/Callbacks.h"
#include "../Resources/ResourceManager.h"

namespace Rendering
{
	class Texture;
	class Device;
}

class FileManager;
class LuaTableResource;
class ImGuiManager : public SingletonResource<ImGuiManager>
{
public:
	ImGuiManager();
	~ImGuiManager();

	void setMicrosoftStyle();

	void update();
	void render();

	bool* win(const char* name, const char* category = nullptr);

	void registerCallback(BasicFunction<void>);

	void saveOpenedWindows();

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

	std::vector< BasicFunction<void> > m_renderCallbacks;
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
		render();
		fc->render();

		if (fc->shouldQuit())
			exit(0); // dirty exit if user closes the window during the imguiLoop
	}
}