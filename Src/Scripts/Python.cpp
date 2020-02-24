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

void PythonEnvironment::init()
{
	if (!m_pythonInited)
	{
		m_pythonInited = true;
		Py_Initialize();
	}
}

const char* PythonEnvironment::getName() const
{
	return "Python";
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
	init();
	m_scripts.push_back(ScriptData{debugName});
	return reinterpret_cast<Script>(m_scripts.size() - 1);
}

void PythonEnvironment::deleteScript(Script s)
{
	LOG_F(FATAL, "todo");
}

std::tuple<std::string, int> PythonEnvironment::loadScript(Script, const char* buffer, std::size_t size)
{
	init();

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

static PyObject*
spam_system(PyObject* self, PyObject* args)
{
	const char* command;
	int sts;

	if (!PyArg_ParseTuple(args, "s", &command))
		return NULL;
	sts = system(command);
	if (sts < 0) {
		
		return NULL;
	}
	return PyLong_FromLong(sts);
}

static PyMethodDef SpamMethods[] = {
	{"system",  spam_system, METH_VARARGS, "Execute a shell command."},
	{NULL, NULL, 0, NULL}        /* Sentinel */
};

PyObject* PythonEnvironment::moduleInit()
{
	auto This = PythonEnvironment::getThis();
	PyModuleDef& mod = This->m_moduleDef;
	mod.m_base = PyModuleDef_HEAD_INIT;
	mod.m_name = "Junkpile";
	mod.m_doc = nullptr;

	return PyModule_Create(&mod);
}

bool PythonEnvironment::registerObject(const Meta::Object& object, const char* exposedName, std::tuple<void*, const char*> instance)
{
	CHECK_F(!m_pythonInited);
	if (!m_moduleRegistered)
	{
		PyImport_AppendInittab("Junkpile", moduleInit);
		m_moduleRegistered = true;
	}

	return true;
}

PythonEnvironment* PythonEnvironment::getThis()
{
	ResourcePtr<ScriptManager> s;
	return (PythonEnvironment*)s->getEnvironment("Python");
}