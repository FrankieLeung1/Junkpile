#pragma once
#include "ScriptManager.h"

class PythonEnvironment : public ScriptManager::Environment
{
public:
	PythonEnvironment();
	~PythonEnvironment();

	bool isScript(const char* path) const;
	Script newScript(const char* debugName);
	void deleteScript(Script);
	std::tuple<std::string, int> loadScript(Script, const char* buffer, std::size_t);

protected:
	bool m_pythonInited{ false };
	
	struct ScriptData
	{
		std::string m_debugName;
	};
	std::vector<ScriptData> m_scripts;
	
};