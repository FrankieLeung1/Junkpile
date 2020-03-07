#include "stdafx.h"
#include "PythonRegisterer.h"
#include "Python.h"

Meta::PythonRegisterer::PythonRegisterer(Meta::Object& metaObject, const char* name, const char* doc):
m_members(),
m_callables(),
m_visitingFunction(nullptr),
m_name(name),
m_nameWithModule("Junkpile."),
m_doc(doc ? doc : ""),
m_metaObject(metaObject)
{
	m_nameWithModule.append(name);
	metaObject.visit(this, nullptr);
}

Meta::PythonRegisterer::~PythonRegisterer()
{
	for (auto it : m_callables)
	{
		Py_XDECREF(it.second.m_callablePyObject);
	}
	m_callables.clear();
}

template<typename T> int Meta::PythonRegisterer::getType(bool null) const { static_assert(false, "Unknown Python Type"); }
template<> int Meta::PythonRegisterer::getType<bool>(bool null) const { return T_BOOL; }
template<> int Meta::PythonRegisterer::getType<int>(bool null) const { return T_INT; }
template<> int Meta::PythonRegisterer::getType<float>(bool null) const { return T_FLOAT; }
template<> int Meta::PythonRegisterer::getType<std::string>(bool null) const { return T_STRING; }
template<> int Meta::PythonRegisterer::getType<bool*>(bool null) const { return null ? T_NONE : T_BOOL; }
template<> int Meta::PythonRegisterer::getType<int*>(bool null) const { return null ? T_NONE : T_INT; }
template<> int Meta::PythonRegisterer::getType<float*>(bool null) const { return null ? T_NONE : T_FLOAT; }
template<> int Meta::PythonRegisterer::getType<std::string*>(bool null) const { return null ? T_NONE : T_STRING; }
template<> int Meta::PythonRegisterer::getType<const Meta::Object>(bool null) const { return null ? T_NONE : T_OBJECT; }

template<typename T> 
bool Meta::PythonRegisterer::visitArg(const char* name, T& d)
{
	if (!m_visitingFunction)
		return false;

	m_visitingFunction->m_args.emplace_back();
	Callable::Arg& arg = m_visitingFunction->m_args.back();
	arg.m_name = name;
	arg.m_default = d;
	arg.m_type = getType<T>();
	return true;
}

template<typename T> bool Meta::PythonRegisterer::visitMember(const char* name, T&)
{
	if (m_visitingFunction)
		return false;

	m_members.emplace_back();
	Member& member = m_members.back();
	member.m_name = name;
	member.m_type = getType<T>();
	member.m_offset = 0;
	return true;
}

PyObject* Meta::PythonRegisterer::instanceObject()
{
	return NULL;
}

bool Meta::PythonRegisterer::matches(const char* name)
{
	return m_nameWithModule == name;
}

int Meta::PythonRegisterer::visit(const char* name, bool& v)
{
	if (visitArg(name, v)) return 0;
	visitMember(name, v);
	return 0;
}

int Meta::PythonRegisterer::visit(const char* name, int& v)
{
	if (visitArg(name, v)) return 0;
	visitMember(name, v);
	return 0;
}

int Meta::PythonRegisterer::visit(const char* name, float& v)
{
	if (visitArg(name, v)) return 0;
	visitMember(name, v);
	return 0;
}

int Meta::PythonRegisterer::visit(const char* name, std::string& v)
{
	if (visitArg(name, v)) return 0;
	visitMember(name, v);
	return 0;
}

int Meta::PythonRegisterer::visit(const char* name, bool* v)
{
	if (visitArg(name, v)) return 0;
	visitMember(name, v);
	return 0;
}

int Meta::PythonRegisterer::visit(const char* name, int* v)
{
	if (visitArg(name, v)) return 0;
	visitMember(name, v);
	return 0;
}

int Meta::PythonRegisterer::visit(const char* name, float* v)
{
	if (visitArg(name, v)) return 0;
	visitMember(name, v);
	return 0;
}

int Meta::PythonRegisterer::visit(const char* name, std::string* v)
{
	if (visitArg(name, v)) return 0;
	visitMember(name, v);
	return 0;
}

int Meta::PythonRegisterer::visit(const char* name, void* v, const Object& object)
{
	if (visitArg(name, object)) return 0;
	visitMember(name, object);
	return 0;
}

int Meta::PythonRegisterer::startObject(const char* name, const Meta::Object& v)
{
	if (visitArg(name, v)) return 0;
	return -1; // todo

	/*m_members.emplace_back();
	Member& member = m_members.back();
	member.m_name = name;
	member.m_type = getType<const Meta::Object>();
	member.m_offset = 0;
	return 0;*/
}

int Meta::PythonRegisterer::endObject()
{
	return 0;
}

int Meta::PythonRegisterer::startArray(const char* name)
{
	return 0;
}

int Meta::PythonRegisterer::endArray(std::size_t)
{
	return 0;
}

int Meta::PythonRegisterer::startFunction(const char* name, bool hasReturn)
{
	auto it = m_callables.insert(std::make_pair(name, Callable()));
	m_visitingFunction = &it.first->second;
	m_visitingFunction->m_name = name;
	
	return 0;
}

int Meta::PythonRegisterer::endFunction()
{
	m_visitingFunction = nullptr;
	return 0;
}

bool Meta::PythonRegisterer::addObject(PyObject* mod)
{
	static PyMemberDef nomembers[] = { {NULL} };
	static PyMethodDef nomethods[] = { {NULL} }; // static functions go here?
	m_typeDef = { PyVarObject_HEAD_INIT(NULL, 0) };
	m_typeDef.tp_name = m_nameWithModule.c_str();
	m_typeDef.tp_doc = m_doc.empty() ? nullptr : m_doc.c_str();
	m_typeDef.tp_basicsize = sizeof(InstanceData);
	m_typeDef.tp_itemsize = 0;
	m_typeDef.tp_flags = Py_TPFLAGS_DEFAULT;
	m_typeDef.tp_new = PyType_GenericNew;
	m_typeDef.tp_init = (initproc)pyInit;
	m_typeDef.tp_finalize = pyDealloc;
	m_typeDef.tp_members = nomembers;
	m_typeDef.tp_methods = nomethods;
	m_typeDef.tp_getattro = pyGet;
	m_typeDef.tp_setattro = pySet;
	if (PyType_Ready(&m_typeDef) < 0)
		return false;

	Py_INCREF(&m_typeDef);
	if (PyModule_AddObject(mod, m_name.c_str(), (PyObject*)&m_typeDef) < 0)
	{
		Py_DECREF(&m_typeDef);
		return false;
	}

	m_callableDef = { PyVarObject_HEAD_INIT(NULL, 0) };
	m_callableDef.tp_name = (m_nameWithModule + ".Callable").c_str();
	m_callableDef.tp_doc = nullptr;
	m_callableDef.tp_basicsize = sizeof(PyCallable);
	m_callableDef.tp_itemsize = 0;
	m_callableDef.tp_flags = Py_TPFLAGS_DEFAULT;
	m_callableDef.tp_new = PyType_GenericNew;
	m_callableDef.tp_call = pyCallableCall;
	m_callableDef.tp_finalize = pyCallableDealloc;
	m_callableDef.tp_members = nomembers;
	m_callableDef.tp_methods = nomethods;
	if (PyType_Ready(&m_callableDef) < 0)
		return false;

	Py_INCREF(&m_callableDef);
	if (PyModule_AddObject(mod, "_Callable", (PyObject*)&m_callableDef) < 0)
	{
		Py_DECREF(&m_typeDef);
		Py_DECREF(&m_callableDef);
		return false;
	}

	/*InstanceData* o = PyObject_NEW(InstanceData, &m_typeDef);
	m_typeDef.tp_init((PyObject*)o, Py_None, Py_None);*/
	return true;
}

int Meta::PythonRegisterer::pyInit(InstanceData* self, PyObject* args, PyObject* kwds)
{
	PyTypeObject* t = Py_TYPE(self);
	self->m_class = PythonEnvironment::findRegisterer(t->tp_name);
	self->m_ptr = self->m_class->m_metaObject.construct();
	return 0;
}

void Meta::PythonRegisterer::pyDealloc(PyObject* object)
{
	InstanceData* self = (InstanceData*)object;
	self->m_class->m_metaObject.destruct(self->m_ptr);
}

PyObject* Meta::PythonRegisterer::pyGet(PyObject* self, PyObject* attr)
{
	InstanceData* data = (InstanceData*)self;
	const char* name = PyUnicode_AsUTF8(attr);
	auto it = data->m_class->m_callables.find(name);
	if (it == data->m_class->m_callables.end())
	{
		PyErr_SetString(PyExc_TypeError, "parameter must be callable");
		return PyLong_FromLong(0);
	}

	PyObject*& callObject = it->second.m_callablePyObject;
	if (!callObject)
	{
		PyCallable* callable = PyObject_NEW(PyCallable, &data->m_class->m_callableDef);
		callable->m_callable = &it->second;
		callable->m_instance = data;
		callObject = (PyObject*)callable;
		Py_INCREF(data);
	}

	return callObject;
}

int Meta::PythonRegisterer::pySet(PyObject *self, PyObject *attr, PyObject *value)
{
	LOG_F(FATAL, "todo pySet\n");
	return 0;
}

PyObject* Meta::PythonRegisterer::pyCallableCall(PyObject* self, PyObject* args, PyObject* kwargs)
{
	PyCallable* pyCallable = ((PyCallable*)self);
	Callable* callable = pyCallable->m_callable;
	Meta::PythonRegisterer::ArgsVisitor visitor(callable, args, kwargs);
	auto instance = pyCallable->m_instance;
	instance->m_class->m_metaObject.callWithVisitor(callable->m_name.c_str(), instance->m_ptr, &visitor);
	return PyLong_FromLong(0);
}

void Meta::PythonRegisterer::pyCallableDealloc(PyObject* object)
{
	Py_DECREF(((PyCallable*)object)->m_instance);
	PyObject_Del(object);
}

Meta::PythonRegisterer::ArgsVisitor::ArgsVisitor(Callable* callable, PyObject* args, PyObject* keywords) : m_callable(callable), m_args(args), m_keywords(keywords) {}
int Meta::PythonRegisterer::ArgsVisitor::visit(const char* name, bool&) { return 0; }
int Meta::PythonRegisterer::ArgsVisitor::visit(const char* name, int&) {
	LOG_F(INFO, "name %s\n", name);
	return -1; 
}
int Meta::PythonRegisterer::ArgsVisitor::visit(const char* name, float&) { return -1; }
int Meta::PythonRegisterer::ArgsVisitor::visit(const char* name, std::string&) { return 0; }
int Meta::PythonRegisterer::ArgsVisitor::visit(const char* name, bool*) { return 0; }
int Meta::PythonRegisterer::ArgsVisitor::visit(const char* name, int*) { return 0; }
int Meta::PythonRegisterer::ArgsVisitor::visit(const char* name, float*) { return 0; }
int Meta::PythonRegisterer::ArgsVisitor::visit(const char* name, std::string*) { return 0; }
int Meta::PythonRegisterer::ArgsVisitor::visit(const char* name, void* object, const Object&) { return 0; }
int Meta::PythonRegisterer::ArgsVisitor::startObject(const char* name, const Meta::Object& objectInfo) { return 0; }
int Meta::PythonRegisterer::ArgsVisitor::endObject() { return 0; }
int Meta::PythonRegisterer::ArgsVisitor::startArray(const char* name) { return 0; }
int Meta::PythonRegisterer::ArgsVisitor::endArray(std::size_t) { return 0; }
int Meta::PythonRegisterer::ArgsVisitor::startFunction(const char* name, bool hasReturn) { return 0; }
int Meta::PythonRegisterer::ArgsVisitor::endFunction() { return 0; }