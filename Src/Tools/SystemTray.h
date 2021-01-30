#pragma once

class SystemTray
{
public:
	SystemTray();
	~SystemTray();

	void imgui();

protected:
	int m_currentStyle;
};