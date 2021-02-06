#pragma once

class SystemTray
{
public:
	SystemTray();
	~SystemTray();

	void imgui();

protected:
	int m_currentStyle;
	bool m_fileProcessing;
	bool m_memoryStore;

	ImVec2 m_contextPos;
	
};