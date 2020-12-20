#include "stdafx.h"
#include "WindowRecorder.h"
#include "ScreenCapture.h"
#include "CImg.h"
#include "../Rendering/Texture.h"
#include "../imgui/ImGuiManager.h"
#include "../Rendering/RenderingDevice.h"
#include "../Misc/Misc.h"

struct WindowRecorder::Impl
{
	std::shared_ptr<SL::Screen_Capture::ICaptureConfiguration<SL::Screen_Capture::ScreenCaptureCallback>> m_screenCapture;
	std::shared_ptr<SL::Screen_Capture::ICaptureConfiguration<SL::Screen_Capture::WindowCaptureCallback>> m_windowCapture;

	std::mutex m_mutex;
	cimg_library::CImgList<unsigned char> m_imgList;
	unsigned int m_nextFreeInList{ 0 };

	bool m_didSave{ false };

	std::shared_ptr<Rendering::Texture> m_texture;
	int m_width, m_height;

	void capture(const SL::Screen_Capture::Image& img);
};

WindowRecorder::WindowRecorder() :
m_p(nullptr)
{
	startCaptureMonitor();
}

WindowRecorder::~WindowRecorder()
{
	m_p->m_screenCapture = nullptr;
	m_p->m_windowCapture = nullptr;
}

void WindowRecorder::startCaptureWindow()
{
	m_p = std::make_shared<Impl>();
	auto getWindows = []() {
		ResourcePtr<VulkanFramework> vf;
		std::size_t handle = vf->getWindowHandle();

		std::vector<SL::Screen_Capture::Window> windows = SL::Screen_Capture::GetWindows();
		for (auto& window : windows)
		{
			if (handle == window.Handle)
				return std::vector<SL::Screen_Capture::Window>{ 1, window };
		}

		LOG_F(ERROR, "Recorder couldn't find main window");
		return std::vector<SL::Screen_Capture::Window>{};
	};
	auto capture = SL::Screen_Capture::CreateCaptureConfiguration(getWindows);
	capture->onNewFrame([this](const SL::Screen_Capture::Image& img, const SL::Screen_Capture::Window& window) { m_p->capture(img); });
	capture->start_capturing()->setFrameChangeInterval(std::chrono::milliseconds(32));
	m_p->m_windowCapture = capture;
}

void WindowRecorder::startCaptureMonitor()
{
	m_p = std::make_shared<Impl>();
	auto getMonitor = []() {
		std::vector<SL::Screen_Capture::Monitor> monitors = SL::Screen_Capture::GetMonitors();
		return std::vector< SL::Screen_Capture::Monitor>{ 1, monitors[0] };
	};
	auto capture = SL::Screen_Capture::CreateCaptureConfiguration(getMonitor);
	capture->onNewFrame([this](const SL::Screen_Capture::Image& img, const SL::Screen_Capture::Monitor& monitor) { m_p->capture(img); });
	capture->start_capturing()->setFrameChangeInterval(std::chrono::milliseconds(32));
	m_p->m_screenCapture = capture;
}

void WindowRecorder::Impl::capture(const SL::Screen_Capture::Image& img)
{
	using namespace SL::Screen_Capture;
	std::lock_guard<std::mutex> l(m_mutex);
	int width = Width(img), height = Height(img);

	Rendering::Texture* texture = m_texture.get();
	if (!texture)
	{
		ResourcePtr<Rendering::Device> device;
		device->allocateThreadResources(std::this_thread::get_id());
		texture = new Rendering::Texture();
		texture->setHostVisible(nextPowerOf2(width), nextPowerOf2(height), sizeof(ImageBGRA));
		m_texture = std::shared_ptr<Rendering::Texture>(texture);
	}
	m_width = width;
	m_height = height;

	cimg_library::CImg<unsigned char> cimg(width, height, 1, 3);
	struct RGBA { unsigned char r, g, b, a; };
	void* map = texture->map();
	const ImageBGRA* bgra = StartSrc(img);
	RGBA* rgba = (RGBA*)map;
	for (int y = 0; y < height; y++)
	{
		const ImageBGRA* rowStart = bgra;
		for (int x = 0; x < width; x++)
		{
			rgba->r = bgra[x].R;
			rgba->g = bgra[x].G;
			rgba->b = bgra[x].B;
			rgba->a = 255;

			*cimg.data(x, y, 0, 0) = bgra[x].R;
			*cimg.data(x, y, 0, 1) = bgra[x].G;
			*cimg.data(x, y, 0, 2) = bgra[x].B;
			rgba++;
		}

		rgba += nextPowerOf2(width) - width;
		bgra = SL::Screen_Capture::GotoNextRow(img, rowStart);
	}

	texture->flush();
	texture->unmap();

	// uncomment this to resize the img before storing them
	//const int w = 1280, h = 720;
	//cimg = cimg.resize(w, h, 1, 3, 1);

	cimg_library::CImgList<unsigned char>& imgList = m_imgList;
	if (imgList.size() < 640)
	{
		imgList.push_back(cimg);
	}
	else
	{
		imgList(m_nextFreeInList++) = cimg;
		if (m_nextFreeInList >= imgList.size())
			m_nextFreeInList = 0;
	}
}

void WindowRecorder::test()
{
	WindowRecorder* recorder = createTestResource<WindowRecorder>();
	ResourcePtr<ImGuiManager> m;
	m->registerCallback({ [](void* ud) {
		WindowRecorder* This = static_cast<WindowRecorder*>(ud);
		Impl* m_p = This->m_p.get();

		ImGui::Begin("WindowRecorder", nullptr, ImGuiWindowFlags_NoScrollbar);

		if (!m_p)
		{
			ImGui::Text("No Capture");
			ImGui::End();
			return;
		}

		std::lock_guard<std::mutex> l(m_p->m_mutex);

		std::shared_ptr<Rendering::Texture> t = m_p->m_texture;
		if (t)
		{
			float width = (float)m_p->m_width, height = (float)m_p->m_height;
			ImVec2 size = ImGui::GetContentRegionMax();
			size.y = size.x * (height / width);

			ImVec2 uv1 = { 0, 0 }, uv2 = { width / t->getWidth(), height / t->getHeight() };
			ImGui::Image(t.get(), size, uv1, uv2);
		}

		std::size_t imgSize = m_p->m_imgList.is_empty() ? 0 : m_p->m_imgList.size() * m_p->m_imgList[0].width() * m_p->m_imgList[0].height() * 3;
		ImGui::Text("Frame Count %d | List Size %s", m_p->m_imgList.size(), prettySize(imgSize)); ImGui::SameLine();
		if (ImGui::Button("Export"))
		{
			auto begin = m_p->m_imgList.begin(), end = m_p->m_imgList.end();
			std::rotate(begin, begin + m_p->m_nextFreeInList, end);
			m_p->m_imgList.save_ffmpeg_external("test.mp4", 25, 0, 4096);

			m_p->m_didSave = true;
		}

		if (m_p->m_didSave)
		{
			ImGui::SameLine();
			if (ImGui::Button("Open"))
				ShellExecute(NULL, L"open", L"test.mp4", NULL, NULL, SW_SHOWDEFAULT);
		}

		ImGui::End();

	}, recorder });
}