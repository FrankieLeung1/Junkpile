#pragma once
#include "ScriptManager.h"

class PythonEnvironment : public ScriptManager::Environment
{
public:
	PythonEnvironment();
	~PythonEnvironment();

	const char* getName() const;

	bool isScript(const char* path) const;
	Script newScript(const char* debugName);
	void deleteScript(Script);
	std::tuple<std::string, int> loadScript(Script, const char* buffer, std::size_t);
	bool registerObject(const Meta::Object&, const char* exposedName, std::tuple<void*, const char*> instance);

protected:
	inline void init();
	static PythonEnvironment* getThis();
	static PyObject* moduleInit();

protected:
	bool m_pythonInited{ false };
	bool m_moduleRegistered{ false };
	
	struct ScriptData
	{
		std::string m_debugName;
	};
	std::vector<ScriptData> m_scripts;

	struct ExportedObject
	{
		std::string m_name;
		const Meta::Object& m_object;
	};
	std::vector<ExportedObject> m_exported;
	PyModuleDef m_moduleDef;

	struct ModuleState
	{
		PythonEnvironment* m_environment;

	};
};