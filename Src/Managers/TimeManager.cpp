#include "stdafx.h"
#include "TimeManager.h"

TimeManager::TimeManager():
m_appStartTime(std::chrono::high_resolution_clock::now()),
m_lastFrameTime(m_appStartTime),
m_frame(0),
m_time(0),
m_delta(0)
{

}

TimeManager::~TimeManager()
{

}

void TimeManager::update()
{
	auto now = std::chrono::high_resolution_clock::now();
	m_time = (now - m_appStartTime).count() / 1000000000.0f;
	m_delta = (now - m_lastFrameTime).count() / 1000000000.0f;
	m_lastFrameTime = now;
	m_frame++;

	for (auto* timer : m_timers)
		timer->update(m_delta);
}

float TimeManager::getTime() const
{
	return m_time;
}

float TimeManager::getDelta() const
{
	return m_delta;
}

unsigned int TimeManager::getFrame() const
{
	return m_frame;
}

static const float inf = std::numeric_limits<float>::infinity();
Timer::Timer():
m_triggerTime(inf),
m_interval(inf)
{
	m_time->m_timers.push_back(this);
}

Timer::~Timer()
{
	ResourcePtr<EventManager> e;
	auto event = e->addOneFrameEvent<TimerEvent>();
	event->m_type = TimerEvent::Deleted;
	event->m_timer = this;

	auto it = std::remove_if(m_time->m_timers.begin(), m_time->m_timers.end(), [this](Timer* t){ return t == this; });
	m_time->m_timers.erase(it, m_time->m_timers.end());
}

void Timer::oneShot(float wait)
{
	repeat(inf, wait);
}

void Timer::repeat(float interval, float delay)
{
	if (delay == -1)
		delay = interval;

	m_triggerTime = m_time->getTime() + delay;
	m_interval = interval;
}

void Timer::stop()
{
	repeat(inf, inf);
}

bool Timer::isActive() const
{
	return m_triggerTime != inf;
}

void Timer::update(float)
{
	float time = m_time->getTime();
	if (time >= m_triggerTime)
	{
		ResourcePtr<EventManager> e;
		auto event = e->addOneFrameEvent<TimerEvent>();
		event->m_type = TimerEvent::Trigger;
		event->m_timer = this;

		m_triggerTime = time + m_interval;
	}
}