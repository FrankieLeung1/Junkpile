#include "stdafx.h"
#include "WindowRecorder.h"
#include "ScreenCapture.h"
#include "../Rendering/Texture.h"
#include "../imgui/ImGuiManager.h"
#include "../Rendering/RenderingDevice.h"
#include "../Misc/Misc.h"

struct WindowRecorder::Impl
{
	std::shared_ptr<SL::Screen_Capture::ICaptureConfiguration<SL::Screen_Capture::ScreenCaptureCallback>> m_screenCapture;
	std::shared_ptr<SL::Screen_Capture::ICaptureConfiguration<SL::Screen_Capture::WindowCaptureCallback>> m_windowCapture;

	Rendering::Texture* m_texture{ nullptr };
	std::mutex m_textureMutex;
};

WindowRecorder::WindowRecorder() :
m_p(new Impl)
{
	auto getWindows = []() {
		ResourcePtr<VulkanFramework> vf;
		std::size_t handle = vf->getWindowHandle();

		std::vector<SL::Screen_Capture::Window> windows = SL::Screen_Capture::GetWindows();
		for (auto& window : windows)
		{
			if(handle == window.Handle)
				return std::vector<SL::Screen_Capture::Window>{ 1, window };
		}

		LOG_F(ERROR, "Recorder couldn't find main window");
		return std::vector<SL::Screen_Capture::Window>{};
	};
	
	auto capture = SL::Screen_Capture::CreateCaptureConfiguration(getWindows);
	capture->onNewFrame([this](const SL::Screen_Capture::Image& img, const SL::Screen_Capture::Window& window)
	{
		using namespace SL::Screen_Capture;
		std::lock_guard<std::mutex> l(m_p->m_textureMutex);
		int width = Width(img), height = Height(img);

		Rendering::Texture* texture = m_p->m_texture;
		if (!texture)
		{
			ResourcePtr<Rendering::Device> device;
			device->allocateThreadResources(std::this_thread::get_id());
			texture = new Rendering::Texture();
			texture->setHostVisible(nextPowerOf2(width), nextPowerOf2(height), sizeof(ImageBGRA));
		}

		
		struct RGBA { unsigned char r, g, b, a; };
		void* map = texture->map();
		const ImageBGRA* bgra = StartSrc(img);
		RGBA* rgba = (RGBA*)map;
		//memcpy(rgba, bgra, width * height * sizeof(ImageBGRA));

		for (int y = 0; y < height; y++)
		{
			const ImageBGRA* rowStart = bgra;
			for (int x = 0; x < width; x++)
			{
				/*float f = ((float)y / (float)height) * 255.0f;
				float f2 = ((float)x / (float)width) * 255.0f;
				rgba->r = 0;
				rgba->g = f2;
				rgba->b = 0;*/

				rgba->r = bgra[x].R;
				rgba->g = bgra[x].G;
				rgba->b = bgra[x].B;
				rgba->a = 255;
				rgba++;
			}

			rgba += nextPowerOf2(width) - width;
			bgra = SL::Screen_Capture::GotoNextRow(img, rowStart);
		}
		texture->flush();
		texture->unmap();

		m_p->m_texture = texture;
	});

	capture->start_capturing()->setFrameChangeInterval(std::chrono::microseconds(100));
	m_p->m_windowCapture = capture;
}

WindowRecorder::~WindowRecorder()
{
	m_p->m_screenCapture = nullptr;
	m_p->m_windowCapture = nullptr;
	delete m_p->m_texture;
	delete m_p;
}

void WindowRecorder::imgui()
{

}

void WindowRecorder::test()
{
	WindowRecorder* recorder = createTestResource<WindowRecorder>();
	ResourcePtr<ImGuiManager> m;
	m->registerCallback({ [](void* ud) {
		WindowRecorder* This = static_cast<WindowRecorder*>(ud);
		Impl* m_p = This->m_p;
		std::lock_guard<std::mutex> l(m_p->m_textureMutex);

		ImGui::Begin("WindowRecorder");

		Rendering::Texture* t = m_p->m_texture;
		if (t)
		{
			ImGui::Image(t->getImTexture(), ImVec2((float)t->getWidth(), (float)t->getHeight()));
		}

		ImGui::End();

	}, recorder });
}