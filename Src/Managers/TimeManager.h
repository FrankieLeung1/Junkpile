#pragma once
#include <chrono>
#include "../Resources/ResourceManager.h"
#include "../Managers/EventManager.h"

class Timer;
class TimeManager : public SingletonResource<TimeManager>
{
public:
	TimeManager();
	~TimeManager();

	void update();

	float getTime() const;
	float getDelta() const;
	unsigned int getFrame() const;

protected:
	std::chrono::steady_clock::time_point m_appStartTime, m_lastFrameTime;
	float m_time, m_delta;
	unsigned int m_frame;

	std::vector<Timer*> m_timers;
	friend class Timer;
};

struct TimerEvent : public Event<TimerEvent>
{
	enum Type { Trigger, Deleted };
	Type m_type;
	Timer* m_timer;
};

class Timer
{
public:
	Timer();
	Timer(const Timer&) = delete;
	~Timer();

	void oneShot(float);
	void repeat(float interval, float delay = -1);
	void stop();

	bool isActive() const;
	void update(float);
	
	template<typename T>
	void listen(T);

protected:
	float m_triggerTime, m_interval;
	ResourcePtr<TimeManager> m_time;
};

// ----------------------- IMPLEMENTATION ----------------------- 
template<typename T>
void Timer::listen(T fn)
{
	ResourcePtr<EventManager> e;
	auto listener = [=](TimerEvent* t)
	{
		if (t->m_timer == this)
		{
			switch(t->m_type)
			{
			case TimerEvent::Trigger: fn(this); break;
			case TimerEvent::Deleted: t->discardListener();
			}
		}
	};
	e->addListener<TimerEvent>(listener);
}