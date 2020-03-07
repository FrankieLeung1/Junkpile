#include "stdafx.h"
#include "Python.h"
#include "../Meta/Meta.h"

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
	m_exported.clear();
}

void PythonEnvironment::init()
{
	if (!m_pythonInited)
	{
		m_pythonInited = true;
		Py_Initialize();
		PyRun_SimpleString("import Junkpile\nprint=Junkpile.print");
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

static PyObject* printMethod(PyObject* self, PyObject* args)
{
	const char* message;
	if (!PyArg_ParseTuple(args, "s", &message))
		return NULL;

	char* messageWithFlag = (char*)alloca(strlen(message) + 2);
	sprintf(messageWithFlag, "\x1%s", message);
	LOG_F(INFO, messageWithFlag);
	return PyLong_FromLong(0);
}

static PyMethodDef JunkpileMethods[] = {
	{"print",  printMethod, METH_VARARGS, "prints using"},
	{NULL, NULL, 0, NULL}        /* Sentinel */
};

// object test
static int meta_init(Meta::MetaTest* self, PyObject *args, PyObject *kwds)
{
	new(self) Meta::MetaTest();
	self->m_pointer2 = reinterpret_cast<Meta::MetaTest::InnerMetaTest*>(0x01);
	self->m_variable4 = "inited!";
	return 0;
}

static PyObject* meta_test(Meta::MetaTest* self, PyObject* args)
{
	LOG_F(INFO, "test\n");
	return PyLong_FromLong(0);
}

PyObject* meta_call(PyObject* self, PyObject* args, PyObject* kwargs)
{
	LOG_F(INFO, "call\n");
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
	m_exported.emplace_back(const_cast<Meta::Object&>(object), exposedName, doc);
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

PythonEnvironment::ExportedObject::ExportedObject(Meta::Object& object, const char* name, const char* doc):
m_name(name),
m_registerer(new Meta::PythonRegisterer(object, name, doc))
{

}