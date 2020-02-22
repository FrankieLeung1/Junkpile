#include "stdafx.h"
#include "Python.h"

PythonEnvironment::PythonEnvironment()
{
}

PythonEnvironment::~PythonEnvironment()
{
	if (m_pythonInited)
	{
		int r = Py_FinalizeEx();
		LOG_IF_F(ERROR, r != 0, "Python exited with %d\n", r);
	}
}

bool PythonEnvironment::isScript(const char* path) const
{
	// ends with .py or .pyw
	std::size_t size = strlen(path);
	return (size >= 3 && path[size - 3] == '.' && path[size - 2] == 'p' && path[size - 1] == 'y') ||
		(size >= 4 && path[size - 4] == '.' && path[size - 3] == 'p' && path[size - 2] == 'y' && path[size - 1] == 'w');
}

PythonEnvironment::Script PythonEnvironment::newScript(const char* debugName)
{
	m_scripts.push_back(ScriptData{debugName});
	return reinterpret_cast<Script>(m_scripts.size() - 1);
}

void PythonEnvironment::deleteScript(Script s)
{
	LOG_F(FATAL, "todo");
}

std::tuple<std::string, int> PythonEnvironment::loadScript(Script, const char* buffer, std::size_t size)
{
	if (!m_pythonInited)
	{
		m_pythonInited = true;
		Py_Initialize();
	}

	// TODO: shouldn't there be a SimpleBuffer function
	char* b = (char*)alloca(size + 1);
	memcpy(b, buffer, size);
	b[size] = '\0';
	
	int error = PyRun_SimpleString(b);
	if (error == 0)
		return { "", -1 };
	else
		return { "PyRun_SimpleString failed", 0 };
}