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
#include "../Meta/Meta.h"
#include "../Misc/Any.h"
#include "../Rendering/Shader.h"
#include "../Sprites/SpriteSystem.h"
#include "../Sprites/Sprite.h"
#include "../Misc/ClassMask.h"
#include "../Misc/WindowRecorder.h"
#include "../Generators/TextureGenerator.h"
#include "../Misc/GrindstoneEditor.h"
#include "../Models/ModelSystem.h"
#include "../Scene/TransformSystem.h"
#include "../Managers/TestManager.h"
#include "../imgui/AssetBrowser.h"
#include "../Game/Game.h"

//#define GRINDSTONE_EDITOR

static void tests(std::function<void(float)>& update, std::function<void()>& render)
{
	ResourcePtr<TestManager> tests;
	tests->addTest("Any", &Any::test);
	tests->addTest("LuaRegisterer", &Meta::LuaRegisterer::test);
	tests->addTest("Sprite", &Sprite::test);
	tests->addTest("Physics", &physicsTest);

	tests->addTest("Meta", &Meta::test);
	tests->addTest("Shader", &Rendering::Shader::test);
	tests->addTest("EventManager", &EventManager::test);
	tests->addTest("Function", &functionTest);

	tests->addTest("TextureGenerator", &TextureGenerator::test);

	tests->addTest("WindowRecorder", &WindowRecorder::test);
	tests->addTest("SpriteSystem", &SpriteSystem::test);
	tests->addTest("ModelSystem", &ModelSystem::test);
	tests->addTest("Game", &Game::test);
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_EVERY_1024_DF);
	loguru::init(__argc, __argv);
	initLoggingForVisualStudio("App.log");

	VulkanFramework::AppType type = VulkanFramework::AppType::MainWindow;
#ifdef GRINDSTONE_EDITOR
	type = VulkanFramework::AppType::ImGuiOnly;
#endif

	ResourceManager r; r.init();
	ResourcePtr<VulkanFramework> vf; vf->init(type);
	ResourcePtr<TimeManager> t;
	ResourcePtr<FileManager> f;
	ResourcePtr<ImGuiManager> m;
	ResourcePtr<EventManager> em;
	ResourcePtr<ComponentManager> cm;
	ResourcePtr<PhysicsSystem> ps;
	ResourcePtr<ScriptManager> sm;
	ResourcePtr<Rendering::Device> rd;
	ResourcePtr<InputManager> i;
	ResourcePtr<SpriteSystem> s;
	ResourcePtr<TransformSystem> transformSystem;
	ResourcePtr<AssetBrowser> ab;
	ResourcePtr<Game> g;

	sm->addEnvironment<PythonEnvironment>();
	
	r.setFreeResources(false);

	m->registerCallback({ [](ResourceManager* r) { r->imgui(); }, &r });
	m->registerCallback({ [](ImGuiManager* im) { bool* b = im->win("Demo"); if(*b) ImGui::ShowDemoWindow(b);  }, m.get() });
	m->registerCallback({ [](ImGuiManager* im) { im->drawMainMenuBar(); im->drawFramerate(); im->drawTrayContext(); }, m.get() });
	m->registerCallback({ [](ComponentManager* cm) { cm->imgui(); }, cm.get() });
	m->registerCallback({ [](PhysicsSystem* ps) { ps->imgui(); }, ps.get() });
	m->registerCallback({ [](EventManager* em) { em->imgui(); }, em.get() });
	m->registerCallback({ [](ScriptManager* im) { im->imgui(); }, sm.get() });
	m->registerCallback({ [](Rendering::Device* rd) { rd->imgui(); }, rd.get() });
	m->registerCallback({ [](SpriteSystem* s) { s->imgui(); }, s.get() });
	m->registerCallback({ [](Game* g) { g->imgui(); }, g.get() });
	m->registerCallback({ [](AssetBrowser* ab) { ab->imgui(); }, ab.get() });

#ifdef GRINDSTONE_EDITOR
	GrindstoneEditor ge;
	m->registerCallback({ [](GrindstoneEditor* ge) { ge->imgui(); }, &ge });
#endif

	TileLevel level;
	m->registerCallback({ [](TileLevel* level) { level->imgui(); }, &level });

	r.startLoading();
	r.setAutoStartTasks(true);

	//sm->registerObject<Meta::MetaTest>("MetaTest");
	//sm->runScriptsInFolder("Tray");

	std::function<void(float)> testUpdate;
	std::function<void()> testRender;
	tests(testUpdate, testRender);
	em->addListener<UpdateEvent>([testUpdate, testRender](UpdateEvent* e) {
		if (testUpdate)
			testUpdate(e->m_delta);

		if (testRender)
			testRender();
	});

	while (!vf->shouldQuit())
	{
		t->update();

		auto* update = em->addOneFrameEvent<UpdateEvent>();
		update->m_delta = t->getDelta();
		update->m_frame = t->getFrame();
		em->process(update->m_delta);
	}

	ResourcePtr<Rendering::Device> d;
	d->waitIdle();
	deleteTestResources();
	return 0;
}