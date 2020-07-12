#pragma once
#include "ScriptManager.h"
#include "PythonRegisterer.h"

class PythonEnvironment : public ScriptManager::Environment
{
public:
	PythonEnvironment();
	~PythonEnvironment();

	const char* getName() const;

	bool isScript(const char* path) const;
	Script newScript(const char* debugName);
	void deleteScript(Script);
	Error loadScript(Script, StringView);
	bool registerObject(const Meta::Object&, const char* exposedName, const char* doc, std::tuple<void*, const char*> instance);

protected:
	inline void init();
	static PythonEnvironment* getThis();
	static Meta::PythonRegisterer* findRegisterer(const char* name);
	static PyObject* moduleInit();

public:
	static PyObject* printMethod(PyObject* self, PyObject* args);
	static PyObject* printErrorMethod(PyObject* self, PyObject* args);
	static PyObject* addModuleDependencyMethod(PyObject* self, PyObject* args);;

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
		std::unique_ptr<Meta::PythonRegisterer> m_registerer;
		ExportedObject(Meta::Object& object, const char* name, const char* doc);
	};
	std::vector<ExportedObject> m_exported;
	PyModuleDef m_moduleDef;
	PyObject* m_global;

	struct ModuleState
	{
		PythonEnvironment* m_environment;
	};

	friend class Meta::PythonRegisterer;
};