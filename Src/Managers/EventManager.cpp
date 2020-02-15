#include "stdafx.h"
#include "EventManager.h"
#include "../imgui/ImGuiManager.h"
#include "../Managers/TimeManager.h"

// IDEA: prioritize listeners. Process higher priority listeners first

EventManager::EventManager()
{
}

EventManager::~EventManager()
{
}

void EventManager::process(float delta)
{
	if (!m_oneFrameBuffer.empty())
	{
		char* current = &m_oneFrameBuffer.front();
		char* end = &m_oneFrameBuffer.back() + 1;

		while (current < end)
		{
			EventBase* message = (EventBase*)current;

			FunctionPool& listeners = m_listeners[message->m_id];
			for (auto it = listeners.begin(); it != listeners.end();)
			{
				ListenerResult r = (*it)(message);
				if (r == ListenerResult::Discard)
					it = listeners.erase(it);
				else
					++it;
			}

			current += message->m_size;
		}

		m_oneFrameBuffer.clear();
	}

	auto prevIt = m_persistentEvents.before_begin();
	for (auto& it = m_persistentEvents.begin(); it != m_persistentEvents.end();)
	{
		FunctionPool& listeners = m_listeners[(*it)->m_id];
		for (auto& listener = listeners.begin(); listener != listeners.end();)
		{
			EventBase* event = it->get();
			ListenerResult r = (*listener)(event);
			if (r == ListenerResult::Discard)
				listener = listeners.erase(listener);
			else
				++listener;
		}

		PersistentEvent<void>* event = (PersistentEvent<void>*)(it->get());

		if (event->m_eventLife >= event->m_eventDeath)
		{
			it = m_persistentEvents.erase_after(prevIt);
		}
		else
		{
			event->m_eventLife += delta; // increment here so callbacks get at least one listen where life > death
			prevIt = it;
			++it;
		}
	}
}

bool EventManager::hasEvents() const
{
	return !m_oneFrameBuffer.empty() || !m_persistentEvents.empty();
}

void EventManager::imgui()
{
	ResourcePtr<ImGuiManager> im;
	bool* opened = im->win("Events");
	if (*opened == false)
		return;

	if (ImGui::Begin("Events", opened))
	{
		ImGui::Columns(3);

		ImGui::Separator();
		ImGui::Text("Name"); ImGui::NextColumn();
		ImGui::Text("Lifetime"); ImGui::NextColumn();
		ImGui::Text("Deathtime"); ImGui::NextColumn();
		ImGui::Separator();

		for (auto& it = m_persistentEvents.begin(); it != m_persistentEvents.end(); ++it)
		{
			PersistentEvent<void>* event = (PersistentEvent<void>*)it->get();
			auto name = m_idToName.find(event->m_id);
			ImGui::Text(name == m_idToName.end() ? "" : name->second); ImGui::NextColumn();
			ImGui::Text("%f", event->m_eventLife); ImGui::NextColumn();
			ImGui::Text("%f", event->m_eventDeath); ImGui::NextColumn();
		}
		ImGui::Separator();
	}
	ImGui::End();
}

void EventManager::test()
{
	EventManager em;
	struct TestEvent : PersistentEvent<TestEvent> {};
	TestEvent* test = em.addPersistentEvent<TestEvent>();
	test->m_eventDeath = 5.0f;
	em.addListener<TestEvent>([](const EventBase* b) {
		TestEvent* t = (TestEvent*)b;
		LOG_F(INFO, "We did it! %f %f\n", t->m_eventLife, t->m_eventDeath);
		return EventManager::ListenerResult::Persist;
	});

	ResourcePtr<TimeManager> t;
	while (em.hasEvents())
	{
		t->update();
		em.process(t->getDelta());
	}
}