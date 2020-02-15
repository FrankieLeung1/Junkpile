#pragma once
#include <chrono>
#include "../Resources/ResourceManager.h"
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
};