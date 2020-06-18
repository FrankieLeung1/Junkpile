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
	for (auto& it : m_callables)
	{
		//Py_XDECREF(it.m_callablePyObject);
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

	if (name && strcmp(name, "return") == 0)
	{
		m_visitingFunction->m_return.m_name = name;
		m_visitingFunction->m_return.m_default = d;
		m_visitingFunction->m_return.m_type = getType<T>();
	}
	else
	{
		m_visitingFunction->m_args.emplace_back();
		Callable::Arg& arg = m_visitingFunction->m_args.back();
		if (name) arg.m_name = name;
		arg.m_default = d;
		arg.m_type = getType<T>();
	}
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

int Meta::PythonRegisterer::visit(const char* name, void** v, const Object& object)
{
	if (visitArg(name, object)) return 0;
	visitMember(name, object);
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

int Meta::PythonRegisterer::startFunction(const char* name, bool hasReturn, bool isConstructor)
{
	m_callables.emplace_back();
	m_visitingFunction = &m_callables.back();
	m_visitingFunction->m_name = name;
	m_visitingFunction->m_isConstructor = isConstructor;
	
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
	self->m_ptr.reset();

	// match the args
	for (Meta::PythonRegisterer::Callable& callable : self->m_class->m_callables)
	{
		if (!callable.m_isConstructor)
			continue;

		Meta::PythonRegisterer::ArgsVisitor visitor(&callable, args, kwds);
		if (visitor.getResult() == 1)
		{
			Any r = self->m_class->m_metaObject.callWithVisitor(callable.m_name.c_str(), &visitor);
			if (r.isType<Meta::StaticFunction::CallFailure>())
				continue;

			self->m_ptr = r;
			break;
		}
	}

	// construct with default
	if (!self->m_ptr)
		self->m_ptr = self->m_class->m_metaObject.construct();

	return 0;
}

void Meta::PythonRegisterer::pyDealloc(PyObject* object)
{
	InstanceData* self = (InstanceData*)object;
	self->m_class->m_metaObject.destruct(self->m_ptr.get<void*>());
}

PyObject* Meta::PythonRegisterer::pyGet(PyObject* self, PyObject* attr)
{
	InstanceData* data = (InstanceData*)self;
	const char* name = PyUnicode_AsUTF8(attr);
	auto it = std::find_if(data->m_class->m_callables.begin(), data->m_class->m_callables.end(), [=](const Meta::PythonRegisterer::Callable& c) { return c.m_name == name; });
	if (it == data->m_class->m_callables.end())
	{
		PyErr_SetString(PyExc_TypeError, "parameter must be callable");
		return PyLong_FromLong(0);
	}

	// TODO: seperate callable is specific to this instance, (it) is global for the whole class
	PyCallable* callable = PyObject_NEW(PyCallable, &data->m_class->m_callableDef);
	callable->m_callable = &(*it);
	callable->m_instance = data;
	Py_INCREF(data);
	return (PyObject*)callable;
}

int Meta::PythonRegisterer::pySet(PyObject *self, PyObject *attr, PyObject *value)
{
	LOG_F(FATAL, "todo pySet\n");
	return 0;
}

PyObject* Meta::PythonRegisterer::pyCallableCall(PyObject* self, PyObject* args, PyObject* kwargs)
{
	// TODO: implement call overloading here (don't use pyCallable->m_callable, lookup by function name)
	PyCallable* pyCallable = ((PyCallable*)self);
	Callable* callable = pyCallable->m_callable;
	Meta::PythonRegisterer::ArgsVisitor visitor(callable, args, kwargs);
	if (visitor.getResult() != 1)
	{
		PyErr_SetString(PyExc_TypeError, "Meta::PythonRegisterer::pyCallableCall failed");
		return NULL;
	}

	InstanceData* instance = pyCallable->m_instance;
	Any r = instance->m_class->m_metaObject.callWithVisitor(callable->m_name.c_str(), instance->m_ptr.isType<void*>() ? instance->m_ptr.get<void*>() : nullptr, &visitor);
	if (r.isType<Meta::Function::CallFailure>())
	{
		PyErr_SetString(PyExc_ValueError, "callWithVisitor failed");
		return NULL;
	}

	if (r.isType<int>()) return PyLong_FromLong(r.get<int>());
	else if(r.isType<float>()) return PyFloat_FromDouble(r.get<float>());
	else if (r.isType<std::string>()) return PyUnicode_FromString(r.get<std::string>().c_str());
	else if (r.isType<bool>()) return r.get<bool>() ? Py_True : Py_False;
	else return PyLong_FromLong(0);
}

void Meta::PythonRegisterer::pyCallableDealloc(PyObject* object)
{
	Py_DECREF(((PyCallable*)object)->m_instance);
	PyObject_Del(object);
}

Meta::PythonRegisterer::ArgsVisitor::ArgsVisitor(Callable* callable, PyObject* args, PyObject* keywords) :
m_callable(callable),
m_args(args),
m_keywords(keywords),
m_argIndex(0),
m_result(1)
{
	CHECK_F(m_data.size() >= callable->m_args.size());
	//memset(m_data.front(), 0xFE, sizeof(m_data));
	std::fill(m_data.begin(), m_data.end(), (void*)0xFE);

	const std::size_t ts = std::tuple_size<decltype(m_data)>() + 2;
	char format[ts] = { '|' };
	int i = 1;
	for (; i <= callable->m_args.size(); ++i)
	{
		switch (callable->m_args[i - 1].m_type)
		{
		case T_BOOL: format[i] = 'b'; break;
		case T_INT: format[i] = 'i'; break;
		case T_FLOAT: format[i] = 'f'; break;
		case T_STRING: format[i] = 's'; break;
		case T_OBJECT: format[i] = 'O'; break; 
		case T_NONE: format[i] = 's'; break;
		default: LOG_F(FATAL, "todo");
		}
	}
	format[i] = '\0';

	int r = PyArg_ParseTuple(args, format, &m_data[0], &m_data[1], &m_data[2], &m_data[3], &m_data[4], &m_data[5], &m_data[6], &m_data[7], &m_data[8]);
	InstanceData* d = (InstanceData*)m_data[0];

	if (r != (int)true)
	{
		PyErr_Clear();
		LOG_F(ERROR, "PyArg_ParseTuple failed %d\n", r);
		m_result = r;
	}
}

template<typename T>
int Meta::PythonRegisterer::ArgsVisitor::visit(T& v)
{
	CHECK_F(m_result == 1);
	m_argIndex++;
	if (m_data[m_argIndex - 1] == (void*)0xFE)
		return -1;

	const T* vptr = reinterpret_cast<const T*>(&m_data[m_argIndex - 1]);
	v = *vptr;
	return 0;
}

int Meta::PythonRegisterer::ArgsVisitor::visit(const char* name, bool& v) { return visit(v); }
int Meta::PythonRegisterer::ArgsVisitor::visit(const char* name, int& v) { return visit(v); }
int Meta::PythonRegisterer::ArgsVisitor::visit(const char* name, float& v) { return visit(v); }
int Meta::PythonRegisterer::ArgsVisitor::visit(const char* name, std::string& v) { char* s = nullptr; int r = visit(s); if (r == 0) v = s; return r; }
int Meta::PythonRegisterer::ArgsVisitor::visit(const char* name, const char*& v) { return visit(v); }
int Meta::PythonRegisterer::ArgsVisitor::visit(const char* name, bool* v) { return visit(v); }
int Meta::PythonRegisterer::ArgsVisitor::visit(const char* name, int* v) { return visit(v); }
int Meta::PythonRegisterer::ArgsVisitor::visit(const char* name, float* v) { return visit(v); }
int Meta::PythonRegisterer::ArgsVisitor::visit(const char* name, std::string* v) { return visit(v); }
int Meta::PythonRegisterer::ArgsVisitor::visit(const char* name, void* object, const Object& o) {
	m_argIndex++;
	if (m_data[m_argIndex - 1] == (void*)0xFE)
		return -1;

	InstanceData* data = (InstanceData*)m_data[m_argIndex - 1];
	data->m_ptr.copyDerefTo(&object);
	return 0; 
}
int Meta::PythonRegisterer::ArgsVisitor::visit(const char* name, void** object, const Object& o ) { return -1; }
int Meta::PythonRegisterer::ArgsVisitor::startObject(const char* name, const Meta::Object& objectInfo) { return -1; }
int Meta::PythonRegisterer::ArgsVisitor::endObject() { return -1; }
int Meta::PythonRegisterer::ArgsVisitor::startArray(const char* name) { return -1; }
int Meta::PythonRegisterer::ArgsVisitor::endArray(std::size_t) { return -1; }
int Meta::PythonRegisterer::ArgsVisitor::startFunction(const char* name, bool hasReturn, bool isConstructor) { return -1; }
int Meta::PythonRegisterer::ArgsVisitor::endFunction() { return -1; }
int Meta::PythonRegisterer::ArgsVisitor::getResult() const { return m_result; }