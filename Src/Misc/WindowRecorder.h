#pragma once

#include "ScreenCapture.h"

class WindowRecorder
{
public:
	WindowRecorder();
	~WindowRecorder();

	void startCaptureWindow();
	void startCaptureMonitor();

	static void test();

protected:
	struct Impl;
	std::shared_ptr<Impl> m_p;
};