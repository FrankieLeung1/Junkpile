#include "stdafx.h"
//#include <vld.h>
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
#include "../Meta/LuaRegisterer.h"
#include "../Framework/VulkanFramework.h"
#include "../Meta/Meta.h"
#include "../Misc/Any.h"
#include "../Rendering/Shader.h"
#include "../Sprites/SpriteSystem.h"
#include "../Sprites/Sprite.h"
#include "../Misc/ClassMask.h"
#include "../Misc/WindowRecorder.h"

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

	WindowRecorder::test();
	//SpriteSystem::test(update, render);
	/*update = [](float update)
	{

	};

	render = []()
	{

	};*/
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);
	loguru::init(__argc, __argv);
	initLoggingForVisualStudio("App.log");

	ResourceManager r;
	ResourcePtr<VulkanFramework> vf;
	ResourcePtr<TimeManager> t;
	ResourcePtr<FileManager> f;
	ResourcePtr<ImGuiManager> m;
	ResourcePtr<EventManager> em;
	ResourcePtr<ComponentManager> cm;
	ResourcePtr<PhysicsSystem> ps;
	ResourcePtr<ScriptManager> sm;
	ResourcePtr<Rendering::Device> rd;
	ResourcePtr<InputManager> i;
	ComponentPtr<PositionComponent> pos(cm.get());
	
	r.setFreeResources(false);

	m->registerCallback({ [](ResourceManager* r) { r->imgui(); }, &r });
	m->registerCallback({ [](ImGuiManager* im) { bool* b = im->win("Demo"); if(*b) ImGui::ShowDemoWindow(b);  }, m.get() });
	m->registerCallback({ [](ImGuiManager* im) { im->drawMainMenuBar(); im->drawFramerate(); im->drawTrayContext(); }, m.get() });
	m->registerCallback({ [](ComponentManager* cm) { cm->imgui(); }, cm.get() });
	m->registerCallback({ [](PhysicsSystem* ps) { ps->imgui(); }, ps.get() });
	m->registerCallback({ [](EventManager* em) { em->imgui(); }, em.get() });
	m->registerCallback({ [](ScriptManager* im) { im->imgui(); }, sm.get() });
	m->registerCallback({ [](Rendering::Device* rd) { rd->imgui(); }, rd.get() });

	TileLevel level;
	m->registerCallback({ [](TileLevel* level) { level->imgui(); }, &level });

	r.startLoading();
	r.setAutoStartTasks(true);

	std::function<void(float)> testUpdate;
	std::function<void()> testRender;
	tests(testUpdate, testRender);

	while (!vf->shouldQuit())
	{
		i->update();
		rd->update();
		t->update();
		f->update();
		ps->processWorld(0.16f);
		ps->process(0.16f);
		r.process();
		em->process(t->getDelta());
		vf->update();
		m->update();
		if(testUpdate)
			testUpdate(0.16f);

		rd->submitAll();

		vf->render();
		m->render();

		if (testRender)
			testRender();
	}

	return 0;
}