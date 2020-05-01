#pragma once

#include "../Misc/Misc.h"
#include "../Misc/Callbacks.h"
#include "../Misc/ResizableMemoryPool.h"
#include "../Resources/ResourceManager.h"

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
	unsigned int m_frame;
};

struct ResourceStateChanged : public Event<ResourceStateChanged>
{
	ResourceData* m_resourceData;
	ResourceData::State m_newState;
	ResourceStateChanged(ResourceData* data = nullptr, ResourceData::State state = ResourceData::State::WAITING) :m_resourceData(data), m_newState(state) {}
};

class EventManager : public SingletonResource<EventManager>
{
public:
	EventManager();
	~EventManager();

	template<typename EventType> EventType* addOneFrameEvent();
	template<typename EventType> EventType* addPersistentEvent();

	typedef FunctionBase<void, EventBase*> EventCallback;
	template<typename Event, typename FunctionType> void addListener(FunctionType, int priority = 0);

	void process(float delta);
	void imgui();

	bool hasEvents() const;

	static void test();

protected:
	void clearEventBuffer(std::vector<char>&, std::vector<TypeHelper*>&);

protected:
	typedef VariableSizedMemoryPool<EventCallback, EventCallback::PoolHelper> FunctionPool;
	std::map<EventBase::Id, std::map<int, FunctionPool> > m_listeners;
	std::map<EventBase::Id, std::map<int, FunctionPool> > m_queuedListeners;
	std::map<EventBase::Id, const char*> m_idToName;

	std::vector<char> m_oneFrameBuffer;
	std::vector<TypeHelper*> m_oneFrameBufferTypes;
	std::forward_list< std::unique_ptr<EventBase> > m_persistentEvents;
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
	//static_assert(std::is_same<decltype(fn((EventType*)nullptr)), ListenerResult>::value, "fn must return ListenerResult");
	// TODO: static_assert the arg types
	m_queuedListeners[EventType::id()][priority].push_back(makeFunction(fn));
}