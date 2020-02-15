#include "stdafx.h"
#include "TimeManager.h"

TimeManager::TimeManager():
m_appStartTime(std::chrono::high_resolution_clock::now()),
m_lastFrameTime(m_appStartTime),
m_frame(0),
m_time(std::numeric_limits<float>::infinity()),
m_delta(std::numeric_limits<float>::infinity())
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