#pragma once

class Analytics
{
public:
	Analytics();
	~Analytics();

	void imgui();

protected:
	tm m_startTime, m_endTime;
};