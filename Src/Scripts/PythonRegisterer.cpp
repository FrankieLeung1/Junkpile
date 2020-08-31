#include "stdafx.h"
#include "PythonRegisterer.h"
#include "Python.h"
#include "../Scene/TransformSystem.h"

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

	// use m_setupHelper to get the offsets of member variables, never access it
	m_setupHelper = (char*)alloca(metaObject.getSize()); 
	metaObject.visit(this, m_setupHelper);
	m_setupHelper = nullptr;
}

Meta::PythonRegisterer::~PythonRegisterer()
{
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
template<> int Meta::PythonRegisterer::getType<void*>(bool null) const { return null ? T_NONE : T_OBJECT; } // used for metaobjects

char Meta::PythonRegisterer::typeToFmt(int type)
{
	switch (type)
	{
	case T_BOOL: return 'b';
	case T_INT: return 'i';
	case T_FLOAT: return 'f';
	case T_STRING: return 's';
	case T_OBJECT: return 'O';
	case T_NONE: return 's';
	default: LOG_F(FATAL, "todo typeToFmt"); return '\0';
	}
}

template<typename T> 
bool Meta::PythonRegisterer::visitArg(const char* name, T& d)
{
	if (!m_visitingFunction)
		return false;

	if(m_visitingFunctionObject)
	{
		// blah copyandpaste
		if (name && strcmp(name, "return") == 0)
		{
			m_visitingFunctionObject->m_return.m_name = name;
			m_visitingFunctionObject->m_return.m_default = d;
			m_visitingFunctionObject->m_return.m_type = getType<T>();
		}
		else
		{
			m_visitingFunctionObject->m_args.emplace_back();
			Callable::Arg& arg = m_visitingFunctionObject->m_args.back();
			if (name) arg.m_name = name;
			arg.m_default = d;
			arg.m_type = getType<T>();
		}
	}
	else if (name && strcmp(name, "return") == 0)
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

template<typename T> std::size_t Meta::PythonRegisterer::calculateOffset(T& v)
{
	return reinterpret_cast<const char*>(&v) - m_setupHelper;
}
std::size_t Meta::PythonRegisterer::calculateOffset(void* v)
{
	return reinterpret_cast<const char*>(v) - m_setupHelper;
}

template<typename T> bool Meta::PythonRegisterer::visitMember(const char* name, T& v, const Meta::Object* metaObject)
{
	if (m_visitingFunction)
		return false;

	m_members.emplace_back();
	Member& member = m_members.back();
	member.m_name = name;
	member.m_type = getType<T>();
	member.m_metaObject = metaObject;
	member.m_offset = calculateOffset(v);
	return true;
}

PyObject* Meta::PythonRegisterer::instancePointer(void* ptr)
{
	InstanceData* o = PyObject_NEW(InstanceData, &m_typeDef);
	o->m_class = this;
	new(&o->m_ptr) Any(ptr);
	o->m_owns = false;
	return (PyObject*)o;
}

PyObject* Meta::PythonRegisterer::instanceValue(const Any& any)
{
	InstanceData* o = PyObject_NEW(InstanceData, &m_typeDef);
	o->m_class = this;
	new(&o->m_ptr) Any(any);
	o->m_owns = false;
	return (PyObject*)o;
}

bool Meta::PythonRegisterer::matches(const char* name) const
{
	return m_nameWithModule == name;
}

bool Meta::PythonRegisterer::matches(const Meta::Object& object) const
{
	return &m_metaObject == &object;
}

StringView Meta::PythonRegisterer::getName() const
{
	return m_nameWithModule;
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
	visitMember(name, *v, &object);
	return 0;
}

int Meta::PythonRegisterer::visit(const char* name, void* v, const Object& object)
{
	if (visitArg(name, object)) return 0;
	visitMember(name, v, &object);
	return 0;
}

int Meta::PythonRegisterer::startObject(const char* name, void* v, const Meta::Object& o)
{
	if (visitArg(name, o)) return 0;
	visitMember(name, v, &o);
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

int Meta::PythonRegisterer::startFunctionObject(const char* name, bool hasReturn)
{
	CHECK_F(!m_visitingFunctionObject && m_visitingFunction);
	m_visitingFunctionObject = std::make_shared<Callable::FunctionObjectData>();
	m_visitingFunction->m_args.emplace_back();
	Callable::Arg& arg = m_visitingFunction->m_args.back();
	arg.m_name = name ? name : "";
	arg.m_type = T_OBJECT;
	return 0;
}

int Meta::PythonRegisterer::endFunctionObject()
{
	CHECK_F(m_visitingFunction != nullptr);
	m_visitingFunction->m_args.back().m_funcObjData = m_visitingFunctionObject;
	m_visitingFunctionObject = nullptr;
	return 0;
}

bool Meta::PythonRegisterer::set(const char* name, void* _instance, PyObject* value)
{
	char* instance = (char*)_instance;
	for (Member& member : m_members)
	{
		if (member.m_name == name)
		{
			switch (member.m_type)
			{
			case T_BOOL:
				if (!PyBool_Check(value)) return false;
				*((bool*)(instance + member.m_offset)) = PyObject_IsTrue(value);
				break;
			case T_INT:
				if (!PyLong_Check(value)) return false;
				*((int*)(instance + member.m_offset)) = PyLong_AsLong(value);
				break;
			case T_FLOAT:
				if (!PyFloat_Check(value)) return false;
				{
					glm::vec3* c = (glm::vec3*)instance;
					*((float*)(instance + member.m_offset)) = (float)PyFloat_AsDouble(value);
				}
				break;
			case T_STRING:
				if (!PyUnicode_Check(value)) return false;
				*((std::string*)(instance + member.m_offset)) = PyUnicode_AsUTF8(value);
				break;
			case T_OBJECT:
				LOG_F(FATAL, "TODO");
				break;
			}

			return true;
		}
	}

	return false;
}

// template<> int Meta::PythonRegisterer::getType<bool>(bool null) const { return T_BOOL; }
// template<> int Meta::PythonRegisterer::getType<int>(bool null) const { return ; }
// template<> int Meta::PythonRegisterer::getType<float>(bool null) const { return ; }
// template<> int Meta::PythonRegisterer::getType<std::string>(bool null) const { return ; }
// template<> int Meta::PythonRegisterer::getType<bool*>(bool null) const { return null ? T_NONE : T_BOOL; }
// template<> int Meta::PythonRegisterer::getType<int*>(bool null) const { return null ? T_NONE : T_INT; }
// template<> int Meta::PythonRegisterer::getType<float*>(bool null) const { return null ? T_NONE : T_FLOAT; }
// template<> int Meta::PythonRegisterer::getType<std::string*>(bool null) const { return null ? T_NONE : T_STRING; }
// template<> int Meta::PythonRegisterer::getType<const Meta::Object>(bool null) const { return null ? T_NONE : T_OBJECT; }
// template<> int Meta::PythonRegisterer::getType<void*>(bool null) const { return null ? T_NONE : T_OBJECT; } // used for metaobjects


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
	m_typeDef.tp_dealloc = pyDealloc;
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
	m_callableDef.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HEAPTYPE;
	m_callableDef.tp_new = PyType_GenericNew;
	m_callableDef.tp_call = pyCallableCall;
	m_callableDef.tp_dealloc = pyCallableDealloc;
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
	self->m_owns = true;

	// match the args
	for (Meta::PythonRegisterer::Callable& callable : self->m_class->m_callables)
	{
		if (!callable.m_isConstructor)
			continue;

		Meta::PythonRegisterer::ArgsVisitor visitor(&callable, args, kwds);
		if (visitor.getResult() == 1)
		{
			Any r = self->m_class->m_metaObject.callWithVisitor(callable.m_name.c_str(), &visitor);
			if (r.isType<Meta::CallFailure>())
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
	if(self->m_owns && self->m_ptr)
		self->m_class->m_metaObject.destruct(self->m_ptr.toVoidPtr());

	self->m_ptr.~Any();
}

PyObject* Meta::PythonRegisterer::pyGet(PyObject* self, PyObject* attr)
{
	InstanceData* data = (InstanceData*)self;
	const char* name = PyUnicode_AsUTF8(attr);
	auto methodIt = std::find_if(data->m_class->m_callables.begin(), data->m_class->m_callables.end(), [=](const Meta::PythonRegisterer::Callable& c) { return c.m_name == name; });
	if (methodIt != data->m_class->m_callables.end())
	{
		// TODO: seperate callable is specific to this instance, (methodIt) is global for the whole class
		PyCallable* callable = PyObject_NEW(PyCallable, &data->m_class->m_callableDef);
		callable->m_callable = &(*methodIt);
		callable->m_instance = data;
		Py_INCREF(data);
		return (PyObject*)callable;
	}

	auto memberIt = std::find_if(data->m_class->m_members.begin(), data->m_class->m_members.end(), [=](const Meta::PythonRegisterer::Member& c) { return c.m_name == name; });
	if (memberIt != data->m_class->m_members.end())
	{
		char format[2] = { typeToFmt(memberIt->m_type), '\0' };
		char* instance = (char*)(*(void**)data->m_ptr.toVoidPtr());
		if (format[0] == 'O')
		{
			// Find the PythonRegisterer and instancePointer of it
			Meta::PythonRegisterer* registerer = PythonEnvironment::getThis()->findRegisterer(*memberIt->m_metaObject);
			LOG_IF_F(FATAL, !registerer, "Return type (\"%s\") not registered with Python\n", memberIt->m_metaObject->getName());
			return registerer->instancePointer(instance + memberIt->m_offset);
		}
		else
		{
			return Py_BuildValue(format, *(int*)(instance + memberIt->m_offset)); // I think all formats are either 64 bit numbers or pointers?
		}
	}

	PyErr_SetString(PyExc_AttributeError, stringf("could not find \'%s\' in \'%s\'", name, data->m_class->getName().c_str()).c_str());
	return 0;
}

int Meta::PythonRegisterer::pySet(PyObject* self, PyObject* attr, PyObject* value)
{
	// support value == null?
	InstanceData* data = (InstanceData*)self;
	const char* name = PyUnicode_AsUTF8(attr);
	if (data->m_class->set(name, data->m_ptr.get<void*>(), value))
		return 0;
	
	PyErr_SetString(PyExc_AttributeError, stringf("unable to set variable \'%s\' in \'%s\'", name, data->m_class->getName().c_str()).c_str()); // TODO: what type is expected?
	return -1;
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
	if (r.isType<Meta::CallFailure>())
	{
		PyErr_SetString(PyExc_ValueError, stringf("callWithVisitor failed: %s", visitor.getError().c_str()).c_str());
		return NULL;
	}

	if (r.isType<int>()) return PyLong_FromLong(r.get<int>());
	else if(r.isType<float>()) return PyFloat_FromDouble(r.get<float>());
	else if (r.isType<std::string>()) return PyUnicode_FromString(r.get<std::string>().c_str());
	else if (r.isType<bool>()) return r.get<bool>() ? Py_True : Py_False;
	else if (Meta::Object* m = r.getMeta())
	{
		Meta::PythonRegisterer* registerer = PythonEnvironment::getThis()->findRegisterer(*m);
		LOG_IF_F(FATAL, !registerer, "Return type (\"%s\") not registered with Python\n", m->getName());
		return registerer->instanceValue(r);
	}
	else if (Meta::Object* m = r.getDerefMeta())
	{
		Meta::PythonRegisterer* registerer = PythonEnvironment::getThis()->findRegisterer(*m);
		LOG_IF_F(FATAL, !registerer, "Return type (\"%s\") not registered with Python\n", m->getName());
		return registerer->instancePointer(*(void**)r.toVoidPtr());
	}
	else
	{
		return Py_None;
	}
}

void Meta::PythonRegisterer::pyCallableDealloc(PyObject* object)
{
	Py_DECREF(((PyCallable*)object)->m_instance);
	PyObject_Del(object);
}

namespace { static_assert(sizeof(void*) == sizeof(intptr_t), ""); void* invalidData = reinterpret_cast<void*>(std::numeric_limits<intptr_t >::max()); }
Meta::PythonRegisterer::ArgsVisitor::ArgsVisitor(Callable* callable, PyObject* args, PyObject* keywords):
m_argIndex(0),
m_result(1)
{
	CHECK_F(m_data.size() >= callable->m_args.size());
	//memset(m_data.front(), invalidData, sizeof(m_data));
	std::fill(m_data.begin(), m_data.end(), (void*)invalidData);

	const std::size_t ts = std::tuple_size<decltype(m_data)>() + 2;
	char format[ts] = { '|' };
	int i = 1;
	for (; i <= callable->m_args.size(); ++i)
	{
		format[i] = typeToFmt(callable->m_args[i - 1].m_type);
	}
	format[i] = '\0';

	int r = PyArg_ParseTuple(args, format, &m_data[0], &m_data[1], &m_data[2], &m_data[3], &m_data[4], &m_data[5], &m_data[6], &m_data[7], &m_data[8]);

	if (r != (int)true)
	{
		PyErr_Clear();
		LOG_F(ERROR, "PyArg_ParseTuple failed %d\n", r);
		m_result = r;
	}
}

template<typename T, typename DataT>
int Meta::PythonRegisterer::ArgsVisitor::visit(T& v)
{
	CHECK_F(m_result == 1);
	m_argIndex++;
	if (m_data[m_argIndex - 1] == invalidData)
	{
		m_error = stringf("Expected %d arguments\n", m_argIndex);
		return -1;
	}

	const DataT* vptr = reinterpret_cast<const DataT*>(&m_data[m_argIndex - 1]);
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
	if (m_data[m_argIndex - 1] == invalidData)
	{
		m_error = stringf("Expected %d arguments\n", m_argIndex);
		return -1;
	}

	InstanceData* data = (InstanceData*)m_data[m_argIndex - 1];
	if (data->m_ptr.isPtr())
		data->m_ptr.copyDerefTo(&object);
	else
		data->m_ptr.copyTo(object);
	return 0; 
}
int Meta::PythonRegisterer::ArgsVisitor::visit(const char* name, StringView& v){ return visit<StringView, const char*>(v); }
int Meta::PythonRegisterer::ArgsVisitor::visit(const char* name, void** object, const Object& o ) { return -1; }
int Meta::PythonRegisterer::ArgsVisitor::startObject(const char* name, void* v, const Meta::Object& objectInfo) { return -1; }
int Meta::PythonRegisterer::ArgsVisitor::endObject() { return -1; }
int Meta::PythonRegisterer::ArgsVisitor::startArray(const char* name) { return -1; }
int Meta::PythonRegisterer::ArgsVisitor::endArray(std::size_t) { return -1; }
int Meta::PythonRegisterer::ArgsVisitor::startFunction(const char* name, bool hasReturn, bool isConstructor) { return -1; }
int Meta::PythonRegisterer::ArgsVisitor::endFunction() { return -1; }
int Meta::PythonRegisterer::ArgsVisitor::startFunctionObject(const char* name, bool hasReturn){ return -1; }
int Meta::PythonRegisterer::ArgsVisitor::endFunctionObject(){ return -1; }
std::shared_ptr<Meta::Visitor> Meta::PythonRegisterer::ArgsVisitor::callbackToPreviousFunctionObject(int& id) {
	m_argIndex++;
	if (m_data[m_argIndex - 1] == invalidData)
	{
		m_error = stringf("Expected %d arguments\n", m_argIndex);
		return nullptr;
	}

	return std::make_shared<CallbackVisitor>((PyObject*)m_data[m_argIndex - 1]);
}
int Meta::PythonRegisterer::ArgsVisitor::getResult() const { return m_result; }
StringView Meta::PythonRegisterer::ArgsVisitor::getError() const { return m_error; }

Meta::PythonRegisterer::CallbackVisitor::CallbackVisitor(PyObject* callback) : m_callback(callback), m_argIndex(0) {
	CHECK_F(PyCallable_Check(callback) == 1); Py_INCREF(m_callback); m_argFormat[0] = '\0'; memset(m_needsToDec, 0x00, sizeof(m_needsToDec));
}
Meta::PythonRegisterer::CallbackVisitor::~CallbackVisitor() { Py_DECREF(m_callback); }
int Meta::PythonRegisterer::CallbackVisitor::visit(const char* name, bool& v) { return add('p', v); }
int Meta::PythonRegisterer::CallbackVisitor::visit(const char* name, int& v) { return add('l', v); }
int Meta::PythonRegisterer::CallbackVisitor::visit(const char* name, float& v) { return add('f', v); }
int Meta::PythonRegisterer::CallbackVisitor::visit(const char* name, std::string& v) { auto c = v.c_str();  return add('s', c); }
int Meta::PythonRegisterer::CallbackVisitor::visit(const char* name, const char*& v) { return add('s', v); }
int Meta::PythonRegisterer::CallbackVisitor::visit(const char* name, bool* v) { return visit(name, *v); }
int Meta::PythonRegisterer::CallbackVisitor::visit(const char* name, int* v) { return visit(name, *v); }
int Meta::PythonRegisterer::CallbackVisitor::visit(const char* name, float* v) { return visit(name, *v); }
int Meta::PythonRegisterer::CallbackVisitor::visit(const char* name, std::string* v) { return visit(name, *v); }
int Meta::PythonRegisterer::CallbackVisitor::visit(const char* name, void* object, const Object& o) {
	Meta::PythonRegisterer* r = PythonEnvironment::findRegisterer(o);
	LOG_IF_F(FATAL, r == nullptr, "Object not registered with Python %s", o.getName());
	PyObject* ptr = r->instancePointer(object);
	add('O', *ptr);
	m_needsToDec[m_argIndex] = ptr;
	m_argIndex++;
	return 0;
}
int Meta::PythonRegisterer::CallbackVisitor::visit(const char* name, void** object, const Object& o) { return visit(name, *object, o); }
int Meta::PythonRegisterer::CallbackVisitor::startObject(const char* name, void* v, const Meta::Object& objectInfo) { return 0; }
int Meta::PythonRegisterer::CallbackVisitor::endObject() { return 0; }
int Meta::PythonRegisterer::CallbackVisitor::startArray(const char* name) { return 0; }
int Meta::PythonRegisterer::CallbackVisitor::endArray(std::size_t) { return 0; }
int Meta::PythonRegisterer::CallbackVisitor::startFunction(const char* name, bool hasReturn, bool isConstructor) { return 0; }
int Meta::PythonRegisterer::CallbackVisitor::endFunction() { return 0; }
int Meta::PythonRegisterer::CallbackVisitor::startFunctionObject(const char* name, bool hasReturn) { return 0; }
int Meta::PythonRegisterer::CallbackVisitor::endFunctionObject() { return 0; }
int Meta::PythonRegisterer::CallbackVisitor::startCallback(int id) { m_argIndex = 0; for (auto d : m_needsToDec) Py_XDECREF(d); return 0; }
int Meta::PythonRegisterer::CallbackVisitor::endCallback() {
	PyObject* r = PyObject_CallFunction(m_callback, m_argFormat, m_args[0], m_args[1], m_args[2], m_args[3], m_args[4], m_args[5], m_args[6], m_args[7], m_args[8], m_args[9]);
	LOG_IF_F(ERROR, r == nullptr, "PyObject_CallFunction failed\n");
	Py_XDECREF(r);
	return 0; 
}