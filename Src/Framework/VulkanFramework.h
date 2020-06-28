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
	enum class AppType
	{
		MainWindow,
		ImGuiOnly,
		SystemTray
	};

public:
	VulkanFramework();
	~VulkanFramework();

	void init(AppType);

	void newFrameImGui();

	void uploadTexture(Rendering::Texture*);

	void update();
	void render();

	void setShouldQuit(bool);
	bool shouldQuit() const;

	bool isMinimized() const;

	void setPip(int quad, bool disableGui = false);

	AppType getAppType() const;

	ImGui_ImplVulkanH_Window* getMainWindowData() const;
	GLFWwindow* getMainWindow() const;
	const char* getWindowTitle() const;
	std::size_t getWindowHandle() const;

	void onIconify(GLFWwindow* window, bool);

protected:
	int initImGui(AppType);

	void SetupVulkanWindow(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height);
	void SetupVulkan(const char** extensions, uint32_t extensions_count);
	void FramePresent(ImGui_ImplVulkanH_Window* wd);
	void FrameRender(ImGui_ImplVulkanH_Window* wd);
	void CleanupVulkanWindow();
	void CleanupVulkan();

	static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

protected:
	ResourcePtr<Rendering::Device> m_device;
	GLFWwindow* m_window;
	std::string m_windowTitle;
	std::array<int, 4> m_winDimensions;
	bool m_hasSystemTray;
	ImVec4 m_clearColour;
	bool m_inited;
	AppType m_appType;

	int m_pip;
	struct PrePIPData
	{
		int m_x, m_y, m_width, m_height;
	};
	PrePIPData m_prePIP;

	GLFWscrollfun m_prevScrollCallback;
};