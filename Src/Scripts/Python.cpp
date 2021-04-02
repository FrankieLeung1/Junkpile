#include "stdafx.h"
#include "Python.h"
#include "../Meta/Meta.h"
#include "../Framework/Framework.h"

PythonEnvironment::PythonEnvironment():
m_global(nullptr)
{
}

PythonEnvironment::~PythonEnvironment()
{
	if (m_pythonInited)
	{
		int r = Py_FinalizeEx();
		LOG_IF_F(ERROR, r != 0, "Python exited with %d\n", r);
	}
	m_exported.clear();
}

void PythonEnvironment::init()
{
	if (!m_pythonInited)
	{
		m_pythonInited = true;

#ifdef _WIN32
		WCHAR pythonHome[MAX_PATH + 1];
		if(GetEnvironmentVariable(L"PYTHONHOME", pythonHome, sizeof(pythonHome)) == 0)
			LOG_F(FATAL, "Environment variable PYTHONHOME does not exist");
#endif
		
		Py_Initialize();

		//m_global = PyEval_GetGlobals();
		m_global = PyModule_GetDict(PyImport_ImportModule("__main__"));

		const char* code =
			"import Junkpile\n"
			"import sys\n"
			"sys.path.append('%s')\n"
			"class Out:\n"
			"	def write(self, stuff):\n"
			"		Junkpile.print(stuff)\n"
			"sys.stdout = Out()\n"
			"class Error:\n"
			"	def write(self, stuff):\n"
			"		Junkpile.printError(stuff)\n"
			"sys.stderr = Error()\n"
			"class MyMetaFinder():\n"
			"	def find_spec(self, fullname, path, target = None):\n"
			//"		print(type(path))\n"
			"		if path == None:\n"
			"			Junkpile.addModuleDependency(fullname, path)\n"
			"		return None\n"
			"sys.meta_path.insert(0, MyMetaFinder())";
		auto r = PyRun_SimpleString(stringf(code, "../Res/Tray/").c_str());
		LOG_IF_F(FATAL, r != 0, "PyRun_SimpleString failed\n");

		for (auto& exported : m_exported)
		{
			if (std::get<0>(exported.m_instance) != nullptr)
			{
				PyObject* name = PyUnicode_FromString(std::get<1>(exported.m_instance));
				PyObject* ptr = exported.m_registerer->instancePointer(std::get<0>(exported.m_instance));
				PyDict_SetItem(m_global, name, ptr);
				Py_DECREF(name);
				Py_DECREF(ptr);
			}
		}
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
	Script s;
	s.m_value = m_scripts.size() - 1;
	return s; // TODO: figure out a way to store this for multiple environments
}

void PythonEnvironment::deleteScript(Script s)
{
	LOG_F(FATAL, "todo");
}

PythonEnvironment::Error PythonEnvironment::loadScript(Script script, StringView codeString)
{
	init();

	char* codeBuffer = (char*)alloca(codeString.size() + 1);
	memcpy(codeBuffer, codeString.c_str(), codeString.size());
	codeBuffer[codeString.size()] = '\0';
	
	std::string name = m_scripts[(std::size_t)script.m_value].m_debugName.c_str();
	PyObject* code = Py_CompileString(codeBuffer, name.c_str(), Py_file_input);
	if (code != nullptr)
	{
		PyObject* result = PyEval_EvalCode(code, m_global, m_global);
		if (!PyErr_Occurred())
		{
			Py_DECREF(code);
			Py_XDECREF(result);
			return {};
		}
	}
	
	if (PyErr_ExceptionMatches(PyExc_SyntaxError) || PyErr_ExceptionMatches(PyExc_IndentationError) || PyErr_ExceptionMatches(PyExc_TabError))
	{
		int line = -1 , offset = -1;
		char* msg  = nullptr;
		PyObject *exc = nullptr, *val = nullptr, *trb = nullptr, *obj = nullptr, *filename = nullptr, *errorText = nullptr;
		PyErr_Fetch(&exc, &val, &trb);
		PyArg_ParseTuple(val, "sO", &msg, &obj);
		PyArg_ParseTuple(obj, "OiiO", &filename, &line, &offset, &errorText);

		Error error;
		error.m_message = msg;
		error.m_filename = PyUnicode_AsUTF8(filename);
		error.m_stacktrace = "";
		error.m_line = line;
		error.m_offset = offset;

		Py_XDECREF(code);
		Py_XDECREF(exc);
		Py_XDECREF(val);
		Py_XDECREF(trb);
		return error;
	}
	else
	{
		PyObject *exc = nullptr, *val = nullptr, *traceback = nullptr;
		PyErr_Fetch(&exc, &val, &traceback);

		PyObject* repr = PyObject_Repr(exc);
		PyObject* lineNumber = PyObject_GetAttrString(traceback, "tb_lineno");
		PyObject* frame = PyObject_GetAttrString(traceback, "tb_frame");
		PyObject* code = PyObject_GetAttrString(frame, "f_code");
		PyObject* filename = PyObject_GetAttrString(code, "co_filename");

		const char* messageStr = PyUnicode_AsUTF8(val);
		if (!messageStr) messageStr = PyUnicode_AsUTF8(PyObject_Repr(val));
		const char* filenameStr = PyUnicode_AsUTF8(filename);

		Error error;
		error.m_message = messageStr ? messageStr : PyUnicode_AsUTF8(repr);
		error.m_filename = filenameStr ? filenameStr : "";
		error.m_line = PyLong_AsLong(lineNumber);
		error.m_stacktrace = "";
		error.m_offset = 0;

		Py_XDECREF(code);
		Py_XDECREF(exc);
		Py_XDECREF(val);
		Py_XDECREF(repr);
		Py_XDECREF(traceback);
		Py_XDECREF(lineNumber);
		Py_XDECREF(frame);
		Py_XDECREF(code);
		Py_XDECREF(filename);
		return error;
	}
}

PyObject* PythonEnvironment::printMethod(PyObject* self, PyObject* args)
{
	const char* message;
	if (!PyArg_ParseTuple(args, "s", &message))
		return NULL;

	char* messageWithFlag = (char*)alloca(strlen(message) + 2);
	sprintf(messageWithFlag, "\x1%s", message);
	LOG_F(INFO, messageWithFlag);
	return PyLong_FromLong(0);
}

PyObject* PythonEnvironment::printErrorMethod(PyObject* self, PyObject* args)
{
	const char* message;
	if (!PyArg_ParseTuple(args, "s", &message))
		return NULL;

	char* messageWithFlag = (char*)alloca(strlen(message) + 2);
	sprintf(messageWithFlag, "\x1%s", message);
	LOG_F(ERROR, messageWithFlag);
	return PyLong_FromLong(0);
}

PyObject* PythonEnvironment::addModuleDependencyMethod(PyObject* self, PyObject* args)
{
	const char* name = nullptr, *path = nullptr;
	if (!PyArg_ParseTuple(args, "sZ", &name, &path))
		return NULL;

	ResourcePtr<ScriptManager> scripts;
	scripts->addDependency(name);
	return PyLong_FromLong(0);
}

static PyMethodDef JunkpileMethods[] = {
	{"print", PythonEnvironment::printMethod, METH_VARARGS, "prints using INFO"},
	{"printError", PythonEnvironment::printErrorMethod, METH_VARARGS, "prints using ERROR"},
	{"addModuleDependency", PythonEnvironment::addModuleDependencyMethod, METH_VARARGS, "registers a loaded module"},
	{NULL, NULL, 0, NULL}        /* Sentinel */
};

PyObject* meta_call(PyObject* self, PyObject* args, PyObject* kwargs)
{
	return PyLong_FromLong(0);
}

PyObject* meta_getattro(PyObject* self, PyObject* attr)
{
	const char* str = PyUnicode_AsUTF8(attr);
	return PyLong_FromLong(0);
}

PyObject* PythonEnvironment::moduleInit()
{
	auto This = PythonEnvironment::getThis(); // this should be in the module state?
	PyModuleDef& modDef = This->m_moduleDef;
	modDef.m_base = PyModuleDef_HEAD_INIT;
	modDef.m_name = "Junkpile";
	modDef.m_doc = nullptr;
	modDef.m_methods = JunkpileMethods;

	PyObject* mod = PyModule_Create(&modDef);
	for (auto& reg : This->m_exported)
	{
		if (!reg.m_registerer->addObject(mod))
		{
			LOG_F(FATAL, "Cannot register object to Python");
		}
	}

	return mod;
}

bool PythonEnvironment::registerObject(const Meta::Object& object, const char* exposedName, const char* doc, std::tuple<void*, const char*> instance)
{
	CHECK_F(!m_pythonInited);
	if (!m_moduleRegistered)
	{
		PyImport_AppendInittab("Junkpile", moduleInit);
		m_moduleRegistered = true;
	}
	// Maybe meta::object references shouldn't be const all the time?
	m_exported.emplace_back(const_cast<Meta::Object&>(object), exposedName, doc, instance);
	return true;
}

PythonEnvironment* PythonEnvironment::getThis()
{
	ResourcePtr<ScriptManager> s;
	return (PythonEnvironment*)s->getEnvironment("Python");
}

Meta::PythonRegisterer* PythonEnvironment::findRegisterer(const char* name)
{
	PythonEnvironment* e = getThis();
	for (auto& exported : e->m_exported)
	{
		if (exported.m_registerer->matches(name))
		{
			return &(*exported.m_registerer);
		}
	}

	return nullptr;
}

Meta::PythonRegisterer* PythonEnvironment::findRegisterer(const Meta::Object& object)
{
	PythonEnvironment* e = getThis();
	for (auto& exported : e->m_exported)
	{
		if (exported.m_registerer->matches(object))
		{
			return &(*exported.m_registerer);
		}
	}

	return nullptr;
}

PythonEnvironment::ExportedObject::ExportedObject(Meta::Object& object, const char* name, const char* doc, std::tuple<void*, const char*> instance):
m_name(name),
m_registerer(new Meta::PythonRegisterer(object, name, doc)),
m_instance(instance)
{

}