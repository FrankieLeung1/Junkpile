#pragma once

#include "GrindstoneEditor.h"
#include "Analytics.h"
#include "Narrative.h"
class Standalone
{
public:
	Standalone();
	~Standalone();

	void imgui();

protected:
	GrindstoneEditor m_ge;
	Analytics m_analytics;
	Narrative m_narrative;

	std::vector<std::pair<const char*, std::function<void()>>> m_windows;
	int m_current;
	int m_currentStyle;
};