#include "stdafx.h"
#include <vld.h>
#include <Windows.h>
#include "../Threading/ThreadPool.h"
#include "../imgui/ImGuiManager.h"
#include "../Managers/TimeManager.h"
#include "../Misc/Misc.h"
#include "../ECS/ECS.h"
#include "../ECS/ComponentManager.h"
#include "../Rendering/Texture.h"
#include "../Files/FileManager.h"
#include "../Files/File.h"
#include "../Sprites/SpriteData.h"
#include "../Rendering/RenderingDevice.h"
#include "../Rendering/TextureAtlas.h"
#include "../Resources/ResourceManager.h"
#include "GLFW/glfw3.h"
#include "../Rendering/Pipeline.h"
#include "../Scene/TileLevel.h"
#include "../Physics/PhysicsSystem.h"
#include "../Managers/EventManager.h"
#include "../Managers/InputManager.h"
#include "../Misc/ResizableMemoryPool.h"
#include <functional>
#include "../Misc/Tests.h"
#include "../Scripts/ScriptManager.h"
#include "../Scripts/Python.h"
#include "../Meta/LuaRegisterer.h"
#include "../Framework/VulkanFramework.h"
#include "../Multiplayer/MultiplayerManager.h"
#include "../Meta/Meta.h"
#include "../Misc/Any.h"
#include "../Rendering/Shader.h"
#include "../Sprites/SpriteSystem.h"
#include "../Sprites/Sprite.h"
#include "../Misc/ClassMask.h"
#include "../Misc/WindowRecorder.h"
#include "../Generators/TextureGenerator.h"
#include "../Tools/GrindstoneEditor.h"
#include "../Models/ModelSystem.h"
#include "../Scene/TransformSystem.h"
#include "../Managers/TestManager.h"
#include "../Misc/WebServer.h"
#include "../imgui/AssetBrowser.h"
#include "../Scene/CameraSystem.h"
#include "../Rendering/Unit.h"
#include "../Game/Game.h"
#include "../Tools/Standalone.h"
#include "../Tools/SystemTray.h"
#include "../Game/TicTacToe.h"

//#define STANDALONE_TOOLS
//#define SYSTEMTRAY_TOOLS

static void tests(std::function<void(float)>& update, std::function<void()>& render)
{
	ResourcePtr<TestManager> tests;
	//tests->addTest("Any", &Any::test);
	//tests->addTest("LuaRegisterer", &Meta::LuaRegisterer::test);
	tests->addTest("Sprite", &Sprite::test);
	//tests->addTest("Physics", &physicsTest);

	//tests->addTest("Meta", &Meta::test);
	//tests->addTest("Shader", &Rendering::Shader::test);
	//tests->addTest("EventManager", &EventManager::test);
	//tests->addTest("Function", &functionTest);

	tests->addTest("TextureGenerator", &TextureGenerator::test);
	//TextureGenerator::test();

	//tests->addTest("WindowRecorder", &WindowRecorder::test);
	//tests->addTest("WebServer", &WebServer::test);
	//tests->addTest("SpriteSystem", &SpriteSystem::test);
	tests->addTest("ModelSystem", &ModelSystem::test);
	tests->addTest("Game", &Game::test);
	//tests->startTest("Game");
	//tests->startTest("ModelSystem");
	//tests->startTest("TextureGenerator");

	//tests->startTest("Sprite");
}

static void standaloneDemo(std::function<void(float)>& update, std::function<void()>& render)
{
	ResourcePtr<TestManager> tests;
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_EVERY_1024_DF);
	loguru::init(__argc, __argv);
	initLoggingForVisualStudio("App.log");

	VulkanFramework::AppType type = VulkanFramework::AppType::MainWindow;
#ifdef STANDALONE_TOOLS
	type = VulkanFramework::AppType::ImGuiOnly;
#endif

#ifdef SYSTEMTRAY_TOOLS
	type = VulkanFramework::AppType::SystemTray;
#endif

	ResourceManager r; r.init();
	ResourcePtr<VulkanFramework> vf; vf->init(type);
	ResourcePtr<ScriptManager> sm;
	ResourcePtr<TimeManager> t;
	ResourcePtr<FileManager> f;
	ResourcePtr<ImGuiManager> m;
	ResourcePtr<EventManager> em;
	ResourcePtr<ComponentManager> cm;
	ResourcePtr<PhysicsSystem> ps;
	ResourcePtr<Rendering::Device> rd;
	ResourcePtr<InputManager> i;
	ResourcePtr<SpriteSystem> s;
	ResourcePtr<TransformSystem> transformSystem;
	ResourcePtr<AssetBrowser> ab;
	ResourcePtr<CameraSystem> cs;
	ResourcePtr<TicTacToeSystem> g;
	//ResourcePtr<MultiplayerManager> mp;
	//mp->showServerList(true);

	WindowRecorder recorder;
	
	r.setFreeResources(false);
	em->addListener<ImGuiRenderEvent>([&r](ImGuiRenderEvent*) { r.imgui(); });
	em->addListener<ImGuiRenderEvent>([m](ImGuiRenderEvent*) { bool* b = m->win("Demo"); if (*b) ImGui::ShowDemoWindow(b); });
	em->addListener<ImGuiRenderEvent>([m](ImGuiRenderEvent*) { m->drawMainMenuBar(); m->drawFramerate(); m->drawTrayContext(); });
	em->addListener<ImGuiRenderEvent>([cm](ImGuiRenderEvent*) { cm->imgui(); });
	em->addListener<ImGuiRenderEvent>([ps](ImGuiRenderEvent*) { ps->imgui(); });
	em->addListener<ImGuiRenderEvent>([em](ImGuiRenderEvent*) { em->imgui(); });
	em->addListener<ImGuiRenderEvent>([sm](ImGuiRenderEvent*) { sm->imgui(); });
	em->addListener<ImGuiRenderEvent>([rd](ImGuiRenderEvent*) { rd->imgui(); });
	em->addListener<ImGuiRenderEvent>([s](ImGuiRenderEvent*) { s->imgui(); });
	//em->addListener<ImGuiRenderEvent>([g](ImGuiRenderEvent*) { g->imgui(); });
	em->addListener<ImGuiRenderEvent>([ab](ImGuiRenderEvent*) { ab->imgui(); });
	em->addListener<ImGuiRenderEvent>([&recorder](ImGuiRenderEvent*) { recorder.imgui(); });
	em->addListener<ImGuiRenderEvent>([](ImGuiRenderEvent*) { Meta::Object::imgui(); });

	//TileLevel level;
	//em->addListener<ImGuiRenderEvent>([level](ImGuiRenderEvent*) { level->imgui(); });	

	r.startLoading();
	r.setAutoStartTasks(true);

	std::function<void(float)> testUpdate;
	std::function<void()> testRender;

#ifdef STANDALONE_TOOLS
	Standalone sa;
	m->registerCallback({ [](Standalone* sa) { sa->imgui(); }, &sa });
	m->setPersistenceFilePath("imgui_tools.lua");
	//m->setStandaloneStyle();
	standaloneDemo(testUpdate, testRender);
#endif

#ifdef SYSTEMTRAY_TOOLS
	SystemTray tray;
	m->setPersistenceFilePath("imgui_tools.lua");
	m->registerCallback({ [](SystemTray* tray) { tray->imgui(); }, &tray });
#endif

#if !defined(STANDALONE_TOOLS) && !defined(SYSTEMTRAY_TOOLS)
	tests(testUpdate, testRender);
#endif
	
	em->addListener<UpdateEvent>([testUpdate, testRender](UpdateEvent* e) {
		if (testUpdate)
			testUpdate(e->m_delta);

		if (testRender)
			testRender();
	});

	int tickCount = 0;
	float tickEvery = 2.0f;
	float tickCounter = 0.0f;
	while (!vf->shouldQuit())
	{
		t->update();

		auto* update = em->addOneFrameEvent<UpdateEvent>();
		update->m_delta = t->getDelta();
		update->m_frame = t->getFrame();

		tickCounter += update->m_delta;
		if(tickCounter > tickEvery)
		{
			auto* tickEvent = em->addOneFrameEvent<TickEvent>();
			tickEvent->m_tick = tickCount++;
			tickCounter -= tickEvery;
		}

		glm::mat4x4 view, proj;
		if (cs->getActiveMatrices(&view, &proj))
		{
			auto render = em->addOneFrameEvent<RenderEvent>();
			render->m_delta = t->getDelta();
			render->m_projection = proj * view;
		}

		em->process(update->m_delta);

		r.freeUnreferenced(); // should call this less often
		r.startReloading();
	}

	ResourcePtr<Rendering::Device> d;
	d->waitIdle();
	d->getRootUnit().clearSubmitted();
	deleteTestResources();
	em->clearAllListeners();
	return 0;
}