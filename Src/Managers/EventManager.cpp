#include "stdafx.h"
#include "EventManager.h"
#include "../imgui/ImGuiManager.h"
#include "../Managers/TimeManager.h"
#include "../Scripts/ScriptManager.h"

EventManager::EventManager():
m_listenersRegistered(false)
{
}

EventManager::~EventManager()
{
	clearEventBuffer(m_oneFrameBuffer, m_oneFrameBufferTypes);
}

void EventManager::process(float delta)
{
	if (!m_listenersRegistered)
	{
		m_listenersRegistered = true;
		addListener<ScriptUnloadedEvent>([this](ScriptUnloadedEvent* e) { onScriptUnloaded(e); });
	}

	std::vector<char> processingEvents = std::move(m_oneFrameBuffer);
	std::vector<TypeHelper*> types = std::move(m_oneFrameBufferTypes);

	// throw m_queuedListeners into m_listeners (uh, I should rename all these variables)
	for (auto& queuedListeners : m_queuedListeners)
	{
		std::map<int, FunctionPool>& eventListeners = m_listeners[queuedListeners.first];
		for (auto& eventListener : queuedListeners.second)
		{
			FunctionPool& pool = eventListeners[eventListener.first];
			pool.move_back(eventListener.second);
		}
	}

	m_queuedListeners.clear();

	// while we got events
	while(!processingEvents.empty())
	{
		char* current = &processingEvents.front();
		char* end = &processingEvents.back() + 1;

		while (current < end)
		{
			EventBase* event = (EventBase*)current;

			std::map<int, FunctionPool>& listeners = m_listeners[event->m_id];
			for (auto it = listeners.rbegin(); it != listeners.rend(); ++it)
			{
				int priority = it->first;
				int listenerIndex = 0;
				FunctionPool& listenersOfSamePriority = it->second;
				for (auto it = listenersOfSamePriority.begin(); it != listenersOfSamePriority.end();)
				{
					event->m_discardEvent = event->m_discardListener = false;
					(*it)(event);
					if (event->m_discardEvent)
						goto endOfEvent;

					if (event->m_discardListener)
					{
						adjustListenerData(event->m_id, priority, listenerIndex);
						it = listenersOfSamePriority.erase(it);
					}
					else
					{
						++listenerIndex;
						++it;
					}
				}
			}

		endOfEvent:
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
			int priority = it->first;
			int listenerIndex = 0;
			FunctionPool& listeners = it->second;
			for (auto& listener = listeners.begin(); listener != listeners.end();)
			{
				event->m_discardEvent = event->m_discardListener = false;
				(*listener)(event);
				if (event->m_discardEvent)
				{
					event->m_eventLife = event->m_eventDeath;
					goto endOfPEvent;
				}

				if (event->m_discardListener)
				{
					adjustListenerData(event->id(), priority, listenerIndex);
					listener = listeners.erase(listener);
				}
				else
				{
					++listener;
					++listenerIndex;
				}
			}
		}

	endOfPEvent:
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

void EventManager::adjustListenerData(EventBase::Id id, int priority, std::size_t index)
{
	for (auto dataIt = m_listenerData.begin(); dataIt != m_listenerData.end();)
	{
		if (dataIt->m_eventId == id && dataIt->m_priority == priority)
		{
			if (dataIt->m_index > index) // if we're after the deleted listener
			{
				//LOG_F(INFO, "Adjusting %d %d from %d to %d\n", id, priority, dataIt->m_index, dataIt->m_index - 1);
				dataIt->m_index--;
				++dataIt;
			}
			else if (dataIt->m_index == index) // if we are the deleted listener
			{
				//LOG_F(INFO, "removing %d %d %d\n", id, priority, index);
				dataIt = m_listenerData.erase(dataIt);
			}
			else
				++dataIt;
		}
		else
			++dataIt;
	}
}

void EventManager::onNewListener(EventBase::Id eventId, int priority, std::size_t index)
{
	if (!ScriptManager::s_inited)
	{
		// special case: ScriptManager registers listeners which makes a circular loop if we're constructing
		m_listenerData.push_back({"", ScriptManager::Environment::Script(), eventId, priority, index });
	}
	else
	{
		ResourcePtr<ScriptManager> scripts;
		StringView path = scripts->getScriptPath(scripts->getRunningScript());
		m_listenerData.push_back({ path, scripts->getRunningScript(), eventId, priority, index });
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
	em.addListener<TestEvent>([](EventBase* b) {
		TestEvent* t = (TestEvent*)b;
		LOG_F(INFO, "Persistent! %f %f\n", t->m_eventLife, t->m_eventDeath);
	});

	struct TestEventOneShot : Event<TestEventOneShot> {};
	em.addOneFrameEvent<TestEventOneShot>();
	em.addListener<TestEventOneShot>([](EventBase* b) {
		TestEventOneShot* t = (TestEventOneShot*)b;
		LOG_F(INFO, "One Shot!\n");
	});

	ResourcePtr<TimeManager> t;
	while (em.hasEvents())
	{
		t->update();
		em.process(t->getDelta());
	}
}

void EventManager::clearAllListeners()
{
	m_listeners.clear();
	m_queuedListeners.clear();
	m_listenerData.clear();
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

void EventManager::onScriptUnloaded(ScriptUnloadedEvent* e)
{
	ResourcePtr<ScriptManager> scripts;
	std::vector<ListenerData> scriptsToRemove;
	for (std::vector<ListenerData>::iterator it = m_listenerData.begin(); it != m_listenerData.end(); ++it)
	{
		if (!it->m_script)
			continue;

		auto script = it->m_script.get<ScriptManager::Environment::Script>();
		if (!script)
			continue;

		StringView path = scripts->getScriptPath(script);
		if (path)
		{
			// BUG? Expression: vector iterators incompatible
			/*auto begin = e->m_paths.begin();
			auto end = e->m_paths.end();
			auto found = std::find(begin, end, path);*/

			for (const StringView& _path : e->m_paths)
			{
				if(_path == path)
					scriptsToRemove.push_back(*it);
			}
		}
	}

	for(auto& script : scriptsToRemove)
	{
		FunctionPool& pool = m_listeners[script.m_eventId][script.m_priority];
		auto it = pool.begin();
		for (int i = 0; i < script.m_index; i++)
			++it;

		std::size_t size = pool.size();
		//LOG_F(INFO, "size %d\n", size);
		pool.erase(it);

		size = pool.size();
		//LOG_F(INFO, "size %d\n", size);
		adjustListenerData(script.m_eventId, script.m_priority, script.m_index);
	}
}

void EventBase::discardListener()
{
	m_discardListener = true;
}

void EventBase::discardEvent()
{
	m_discardEvent = true;
}

#include "../Managers/InputManager.h"

template<>
Meta::Object Meta::instanceMeta<EventManager>()
{
	//template<typename T, typename R, typename... Args> Object& func(const char* name, R(T::*)(Args...));
	return Meta::Object("EventManager").
		func<EventManager, void, std::function<void(UpdateEvent*)>>("addListener_UpdateEvent", &EventManager::addListener<UpdateEvent>).
		func<EventManager, void, std::function<void(InputChanged*)>>("addListener_InputChanged", &EventManager::addListener<InputChanged>).
		func<EventManager, void, std::function<void(InputHeld*)>>("addListener_InputHeld", &EventManager::addListener<InputHeld>);
}

template<>
Meta::Object Meta::instanceMeta<UpdateEvent>()
{
	return Meta::Object("UpdateEvent").
		var("m_delta", &UpdateEvent::m_delta).
		var("m_frame", &UpdateEvent::m_frame);
}

