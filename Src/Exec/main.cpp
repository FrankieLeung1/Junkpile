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

//#define GRINDSTONE_EDITOR

static void tests(std::function<void(float)>& update, std::function<void()>& render)
{
	//Any::test();
	//Meta::LuaRegisterer::test();
	//Sprite::test();
	//physicsTest();

	/*struct Test
	{
		Test()
		{
			memset(this + sizeof(Test), 0xFF, 4);
		}
	};
	char buffer[32];
	new(buffer) Test();*/

	//Meta::test();
	//Rendering::Shader::test();
	//EventManager::test();
	//functionTest();

	//TextureGenerator::test();

	//WindowRecorder::test();
	//SpriteSystem::test(update, render);
	ModelSystem::test();
	/*update = [](float update)
	{

	};

	render = []()
	{

	};*/
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_EVERY_1024_DF);
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