#pragma once
#include "../Resources/ResourceManager.h"

struct GLFWwindow;
struct ImGui_ImplVulkanH_Window;

namespace Rendering
{
	class Device;
	class Texture;
}

class VulkanFramework : public SingletonResource<VulkanFramework>
{
public:
	VulkanFramework();
	~VulkanFramework();

	int initImGui(bool systemTray);

	void newFrameImGui();

	void uploadTexture(Rendering::Texture*);

	void update();
	void render();

	void setShouldQuit(bool);
	bool shouldQuit() const;

	ImGui_ImplVulkanH_Window* getMainWindowData() const;
	GLFWwindow* getMainWindow() const;
	const char* getWindowTitle() const;
	std::size_t getWindowHandle() const;

protected:
	void SetupVulkanWindow(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height);
	void SetupVulkan(const char** extensions, uint32_t extensions_count);
	void FramePresent(ImGui_ImplVulkanH_Window* wd);
	void FrameRender(ImGui_ImplVulkanH_Window* wd);
	void CleanupVulkanWindow();
	void CleanupVulkan();

protected:
	ResourcePtr<Rendering::Device> m_device;
	GLFWwindow* m_window;
	std::string m_windowTitle;
	bool m_hasSystemTray;
	ImVec4 m_clearColour;
};