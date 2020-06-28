#include "stdafx.h"
#include "TestManager.h"
#include "../Managers/EventManager.h"
#include "../imgui/ImGuiManager.h"
#include "../ECS/ComponentManager.h"

TestManager::TestManager()
{
	ResourcePtr<EventManager> events;
	events->addListener<UpdateEvent>([this](UpdateEvent* e) { update(e->m_delta); render(); });
	m_imgui->registerCallback({[](TestManager* t) { t->imgui(); }, this });
}

TestManager::~TestManager()
{

}

void TestManager::addTest(const char* name, void(*testFn)())
{
	m_tests.emplace(name, [=](UpdateFn&, RenderFn&) { testFn(); });
}

void TestManager::addTest(const char* name, void(*testFn)(UpdateFn&, RenderFn&))
{
	m_tests.emplace(name, testFn);
}

void TestManager::update(float delta)
{
	if (m_current.m_update)
		m_current.m_update(delta);
}

void TestManager::render()
{
	if (m_current.m_render)
		m_current.m_render();
}

void TestManager::imgui()
{
	using namespace ImGui;
	bool* opened = m_imgui->win("Tests");
	if ((*opened) == false)
		return;

	if (Begin("Tests", opened))
	{
		Columns(2);
		for (auto& it : m_tests)
		{
			bool current = (m_current.m_name != nullptr && strcmp(m_current.m_name, it.first) == 0);

			Text(it.first);
			NextColumn();
			PushID(it.first);
			if (Button(current ? "Reset" : "Start"))
			{
				ResourcePtr<ComponentManager> components;
				components->clearAllComponents();

				m_current.m_name = it.first;
				it.second(m_current.m_update, m_current.m_render);
			}
			PopID();
			NextColumn();
		}
		Columns(1);
		if (Button("Stop All"))
		{
			ResourcePtr<ComponentManager> components;
			components->clearAllComponents();
			m_current = Current();
		}
		
	}
	End();
}

void TestManager::reset()
{
	ResourcePtr<ComponentManager> components;
	components->clearAllComponents();
}