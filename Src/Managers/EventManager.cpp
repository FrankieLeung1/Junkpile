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
	clearEventBuffer(m_oneFrameBuffer, m_oneFrameBufferTypes);
}

void EventManager::process(float delta)
{
	std::vector<char> processingEvents = std::move(m_oneFrameBuffer);
	std::vector<TypeHelper*> types = std::move(m_oneFrameBufferTypes);

	for (auto& queuedListeners : m_queuedListeners)
	{
		std::map<int, FunctionPool>& eventListeners = m_listeners[queuedListeners.first];
		for (auto& eventListener : queuedListeners.second)
		{
			eventListeners[eventListener.first].move_back(eventListener.second);
		}
	}

	m_queuedListeners.clear();

	while(!processingEvents.empty())
	{
		char* current = &processingEvents.front();
		char* end = &processingEvents.back() + 1;

		while (current < end)
		{
			EventBase* event = (EventBase*)current;

			std::map<int, FunctionPool>& map = m_listeners[event->m_id];
			for (auto it = map.rbegin(); it != map.rend(); ++it)
			{
				FunctionPool& listeners = it->second;
				for (auto it = listeners.begin(); it != listeners.end();)
				{
					ListenerResult r = (*it)(event);
					if (r == ListenerResult::Discard)
						it = listeners.erase(it);
					else
						++it;
				}
			}

			current += event->m_size;
		}

		clearEventBuffer(processingEvents, types);
	}

	auto prevIt = m_persistentEvents.before_begin();
	for (auto& it = m_persistentEvents.begin(); it != m_persistentEvents.end();)
	{
		PersistentEvent<void>* event = (PersistentEvent<void>*)(it->get());
		std::map<int, FunctionPool>& map = m_listeners[event->m_id];
		for (auto it = map.rbegin(); it != map.rend(); ++it)
		{
			FunctionPool& listeners = it->second;
			for (auto& listener = listeners.begin(); listener != listeners.end();)
			{
				ListenerResult r = (*listener)(event);
				if (r == ListenerResult::Discard)
					listener = listeners.erase(listener);
				else
					++listener;
			}
		}

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
		LOG_F(INFO, "Persistent! %f %f\n", t->m_eventLife, t->m_eventDeath);
		return EventManager::ListenerResult::Persist;
	});

	struct TestEventOneShot : Event<TestEventOneShot> {};
	em.addOneFrameEvent<TestEventOneShot>();
	em.addListener<TestEventOneShot>([](const EventBase* b) {
		TestEventOneShot* t = (TestEventOneShot*)b;
		LOG_F(INFO, "One Shot!\n");
		return EventManager::ListenerResult::Persist;
	});

	ResourcePtr<TimeManager> t;
	while (em.hasEvents())
	{
		t->update();
		em.process(t->getDelta());
	}
}

void EventManager::clearEventBuffer(std::vector<char>& buffer, std::vector<TypeHelper*>& types)
{
	auto it = buffer.begin();
	auto typeIt = types.begin();
	while (it != buffer.end())
	{
		(*typeIt)->destruct(&(*it));
		std::advance(it, (*typeIt)->getSize());
		++typeIt;
	}
	buffer.clear();
	types.clear();
}