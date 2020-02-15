#pragma once

#include "ScreenCapture.h"

class WindowRecorder
{
public:
	WindowRecorder();
	~WindowRecorder();

	void imgui();

	static void test();

protected:
	struct Impl;
	Impl* m_p;
};