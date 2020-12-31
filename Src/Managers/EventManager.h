#pragma once

#include "../Misc/Misc.h"
#include "../Misc/Callbacks.h"
#include "../Misc/ResizableMemoryPool.h"
#include "../Resources/ResourceManager.h"
#include "../Meta/Meta.h"

namespace Rendering
{
	class RenderTarget;
}

class EventBase 
{
public:
	void discardListener();
	void discardEvent();

protected:
	typedef unsigned long long Id;
	Id m_id;
	std::size_t m_size;

private:
	bool m_discardListener;
	bool m_discardEvent;
	friend class EventManager;
};

template<typename T>
struct Event : public EventBase
{
private: static int m_;
public:
	static constexpr Id id() { return reinterpret_cast<unsigned long long>(&m_); }
};
template<typename T> int Event<T>::m_ = 0;

template<typename T>
struct PersistentEvent : public Event<T>
{
	float m_eventLife{ 0.0f };										// how long has this been alive?
	float m_eventDeath{ std::numeric_limits<float>::infinity() };	// how long should this live?
};

struct UpdateEvent : public Event<UpdateEvent>
{
	float m_delta;
	int m_frame;
};

struct RenderEvent : public Event<RenderEvent>
{
	float m_delta;
	glm::mat4x4 m_projection;
	Rendering::RenderTarget* m_target;
};

struct ResourceStateChanged : public Event<ResourceStateChanged>
{
	ResourceData* m_resourceData;
	ResourceData::State m_newState;
	std::shared_ptr<Resource::Loader> m_loader;
	bool m_reload; // change is in response to a resource reload
	ResourceStateChanged(ResourceData* data = nullptr, ResourceData::State state = ResourceData::State::WAITING, std::shared_ptr<Resource::Loader> loader = nullptr, bool reload = false):
		m_resourceData(data), m_newState(state), m_loader(loader), m_reload(reload) {}
};

class ScriptManager;
struct ScriptUnloadedEvent;
class EventManager : public SingletonResource<EventManager>
{
public:
	EventManager();
	~EventManager();

	template<typename EventType> EventType* addOneFrameEvent();
	template<typename EventType> EventType* addPersistentEvent();

	typedef FunctionBase<void, EventBase*> EventCallback;
	template<typename Event, typename FunctionType> void addListener(FunctionType, int priority = 0);
	template<typename Event> void addListener(std::function<void(Event*)>, int priority);
	template<typename Event> void addListenerFromScript(std::function<void(Event*)>, int priority);

	void process(float delta);
	void imgui();

	void clearAllListeners();

	bool hasEvents() const;

	static void test();

protected:
	void adjustListenerData(EventBase::Id, int priority, std::size_t index); // adjust m_listenerData when given listener is deleted
	void onNewListener(EventBase::Id eventId, int priority, std::size_t index);
	void clearEventBuffer(std::vector<char>&, std::vector<TypeHelper*>&);
	void onScriptUnloaded(ScriptUnloadedEvent*);

protected:
	typedef VariableSizedMemoryPool<EventCallback, EventCallback::PoolHelper> FunctionPool;
	std::map<EventBase::Id, std::map<int, FunctionPool> > m_listeners;
	std::map<EventBase::Id, std::map<int, FunctionPool> > m_queuedListeners;
	std::map<EventBase::Id, const char*> m_idToName;

	struct ListenerData
	{
		std::string m_path; //temp
		Any m_script; // typeof ScriptManager::Environment::Script
		EventBase::Id m_eventId;
		int m_priority;
		std::size_t m_index;
		ListenerData& operator=(const ListenerData& d) { m_path = d.m_path; m_script = d.m_script; m_eventId = d.m_eventId; m_priority = d.m_priority; m_index = d.m_index; return *this; }
		bool operator==(const ListenerData& d) const { return d.m_script == m_script && d.m_eventId == m_eventId && d.m_priority == m_priority && d.m_index == m_index; }
	};
	std::vector<ListenerData> m_listenerData;

	std::vector<char> m_oneFrameBuffer;
	std::vector<TypeHelper*> m_oneFrameBufferTypes;
	std::forward_list< std::unique_ptr<EventBase> > m_persistentEvents;
	bool m_listenersRegistered;
};

// ----------------------- IMPLEMENTATION ----------------------- 
template<typename EventType> EventType* EventManager::addOneFrameEvent()
{
	static_assert(std::is_base_of<Event<EventType>, EventType>::value == true, "Must inherit from Event");
	m_idToName[EventType::id()] = typeid(EventType).name();

	std::size_t size = m_oneFrameBuffer.size();
	m_oneFrameBufferTypes.push_back(&TypeHelperInstance<EventType>::s_instance);
	m_oneFrameBuffer.insert(m_oneFrameBuffer.end(), sizeof(EventType), 0);
	EventType* event = new(&m_oneFrameBuffer[size]) EventType();
	event->m_id = EventType::id();
	event->m_size = sizeof(EventType);
	return event;
}

template<typename EventType> EventType* EventManager::addPersistentEvent()
{
	static_assert(std::is_base_of<PersistentEvent<EventType>, EventType>::value == true, "Must inherit from PersistentEvent");
	m_idToName[EventType::id()] = typeid(EventType).name(); // TODO: something else

	std::unique_ptr<EventType> event(new EventType());
	event->m_id = EventType::id();
	event->m_size = sizeof(EventType);
	m_persistentEvents.push_front(std::move(event));
	return (EventType*)m_persistentEvents.front().get();
}

template<typename EventType, typename FunctionType> void EventManager::addListener(FunctionType fn, int priority)
{
	// TODO: static_assert the arg types
	auto& eventListeners = m_queuedListeners[EventType::id()];
	auto& priortyListeners = eventListeners[priority];
	priortyListeners.push_back(makeFunction(fn));
}

template<typename Event> void EventManager::addListener(std::function<void(Event*)> fn, int priority)
{
	addListener<Event>([=](EventBase* b) { fn((Event*)b); }, priority);
}

template<typename Event> void EventManager::addListenerFromScript(std::function<void(Event*)> fn, int priority)
{
	auto& priortyListeners = m_queuedListeners[Event::id()][priority];

	// find the index we're gonna be when we get added to m_listeners
	std::size_t queuedIndex = priortyListeners.size(), listenerIndex = m_listeners[Event::id()][priority].size();
	onNewListener(Event::id(), priority, queuedIndex + listenerIndex);

	addListener<Event>([=](EventBase* b) { fn((Event*)b); }, priority);
}


template<> Meta::Object Meta::instanceMeta<EventManager>();
template<> Meta::Object Meta::instanceMeta<UpdateEvent>();