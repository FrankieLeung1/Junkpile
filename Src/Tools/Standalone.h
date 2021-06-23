#pragma once

#include "GrindstoneEditor.h"
#include "Analytics.h"
#include "Narrative.h"
#include "SpriteEditor.h"
#include "../Misc/WebServer.h"
class Standalone
{
public:
	Standalone();
	~Standalone();

	void imgui();

protected:
	GrindstoneEditor m_ge;
	SpriteEditor m_se;
	Analytics m_analytics;
	Narrative m_narrative;
	WebServer m_server;
	bool m_scriptingExample;
	bool m_scriptLoaded;

	std::vector<std::pair<const char*, std::function<void()>>> m_windows;
	int m_current;
	int m_currentStyle;
};