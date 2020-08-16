#include "stdafx.h"
#include <string>
#include "Meta.h"
#include "Serializer.h"
#include "../Scripts/ScriptManager.h"

namespace Meta {

Base::Base(std::size_t size, BaseType type):
Element(size),
m_baseType(type)
{

}

Object::Object(const char* name):
Base(sizeof(Object), BaseType::Object),
m_name(name),
m_factory(),
m_copyOperator(nullptr),
m_size(0)
{

}

Object::~Object()
{
	for (auto& it : m_variables)
	{
		switch (it.m_baseType)
		{
		case Base::BaseType::Object: static_cast<Object&>(it).~Object(); break;
		case Base::BaseType::Variable: static_cast<Variable&>(it).~Variable(); break;
		case Base::BaseType::Function: static_cast<Function&>(it).~Function(); break;
		case Base::BaseType::StaticFunction: static_cast<StaticFunction&>(it).~StaticFunction(); break;
		case Base::BaseType::Hook: static_cast<Hook&>(it).~Hook(); break;
		}
	}
}

const char* Object::getName() const
{
	return m_name;
}

std::size_t Object::getSize() const
{
	return m_size;
}

void Object::setSize(std::size_t size)
{
	m_size = size;
}

Object& Object::hook(const BasicFunction<int, Visitor*, void*>& fn)
{
	m_variables.emplace_back<Meta::Hook>(fn);
	return *this;
}

Object& Object::factory(BasicFunction<void*> constructor, BasicFunction<void, void*> destructor)
{
	CHECK_F(!m_factory);
	m_factory.set(constructor, destructor);
	return *this;
}

Any Object::callWithVisitor(const char* name, void* instance, Visitor* v)
{
	for (const auto& it : m_variables)
	{
		if (it.m_baseType == Base::BaseType::Function)
		{
			Function& func = (Function&)it;
			if (strcmp(name, func.getName()) == 0)
			{
				return func.callWithVisitor(v, instance);
			}
		}
	}

	LOG_F(ERROR, "callWithVisitor failed (%s)\n", name);
	return nullptr;
}

Any Object::callWithVisitor(const char* name, Visitor* v)
{
	for (const auto& it : m_variables)
	{
		if (it.m_baseType == Base::BaseType::StaticFunction)
		{
			StaticFunction& func = (StaticFunction&)it;
			if (strcmp(name, func.getName()) == 0)
			{
				return func.callWithVisitor(v);
			}
		}
	}

	LOG_F(ERROR, "callWithVisitor failed (%s)\n", name);
	return nullptr;
}

void* Object::construct()
{
	if (!m_factory)
	{
		LOG_F(ERROR, "factory not provided for %s\n", getName());
		return nullptr;
	}

	return m_factory.construct();
}

void Object::destruct(void* v)
{
	if (!m_factory)
	{
		LOG_F(ERROR, "factory not provided for %s\n", getName());
		return;
	}
	m_factory.destruct(v);
}

void Object::copyTo(void* dest, void* src) const
{
	m_copyOperator(dest, src);
}

int Object::visit(Visitor* v, void* o) const
{
	// TODO: do something with return values

	for (const auto& it : const_cast<Object*>(this)->m_variables)
	{
		switch (it.m_baseType)
		{
			case Base::BaseType::Object:
			{
				Object& object = (Object&)it;
				object.visit(v, o);
			}
			break;

			case Base::BaseType::Variable:
			{
				Variable& var = (Variable&)it;
				var.visit(v, o);
			}
			break;

			case Base::BaseType::Function:
			{
				Function& func = (Function&)it;
				func.visit(v, o);
			}
			break;

			case Base::BaseType::StaticFunction:
			{
				StaticFunction& func = (StaticFunction&)it;
				func.visit(v);
			}
			break;

			case Base::BaseType::Hook:
			{
				Hook& hook = (Hook&)it;
				hook.visit(v, o);
			}
		}
	}

	return 0;
}

int Variable::visit(Visitor* v, void* obj)
{
	return (*m_visitFunction)(v, m_name, (void*)m_pointerBuffer, obj);
}

Meta::Function::~Function()
{

}

int Meta::Function::visit(Visitor* v, void* object)
{
	const char** argNames = m_argCount ? (const char**)(this + 1) : nullptr;
	Any* defaults = nullptr;
	if (m_defaultCount > 0)
	{
		if (argNames)
		{
			char* bytes = (char*)(this + 1);
			defaults = (Any*)(bytes + (m_argCount * sizeof(char*)));
		}
		else
		{
			defaults = (Any*)(this + 1);
		}
	}

	return m_implementation->visit(v, m_name, false, object, argNames, defaults, m_defaultCount);
}

Any Meta::Function::callWithVisitor(Visitor* v, void* ptr)
{
	const char** bytes = (const char**)(this + 1);
	return m_implementation->callWithVisitor(v, m_pointerBuffer, ptr, m_argCount ? bytes : nullptr, (Any*)(bytes + m_argCount), m_defaultCount);
}

const char* Meta::Function::getName() const
{
	return m_name;
}

StaticFunction::~StaticFunction()
{

}

int Meta::StaticFunction::visit(Visitor* v)
{
	const char** argNames = m_argCount ? (const char**)(this + 1) : nullptr;
	Any* defaults = nullptr;
	if (m_defaultCount > 0)
	{
		if (argNames)
		{
			char* bytes = (char*)(this + 1);
			defaults = (Any*)(bytes + (m_argCount * sizeof(char*)));
		}
		else
		{
			defaults = (Any*)(this + 1);
		}
	}

	return m_implementation->visit(v, m_name, m_isConstructor, nullptr, argNames, defaults, m_defaultCount);
}

Any StaticFunction::callWithVisitor(Visitor* v)
{
	const char** bytes = (const char**)(this + 1);
	return m_implementation->callWithVisitor(v, m_pointerBuffer, nullptr, m_argCount ? bytes : nullptr, (Any*)(bytes + m_argCount), m_defaultCount);
}

const char* StaticFunction::getName() const
{
	return m_name;
}

Hook::Hook(const BasicFunction<int, Visitor*, void*>& fn):
Base(sizeof(Hook), BaseType::Hook),
m_fn(fn)
{

}

Hook::~Hook()
{

}

int Hook::visit(Visitor* v, void* object)
{
	return m_fn(v, object);
}

Factory::Factory()
{
}

Factory::~Factory()
{
}

void Factory::set(const BasicFunction<void*>& constructor, const BasicFunction<void, void*>& destructor)
{
	m_constructor = constructor;
	m_destructor = destructor;
}

void* Factory::construct()
{
	return m_constructor();
}

void Factory::destruct(void* v)
{
	m_destructor(v);
}

bool Factory::operator!() const
{
	return !m_constructor || !m_destructor;
}

template<> 
Object Meta::instanceMeta<MetaTest>()
{
	return Object("MetaTest")
		.defaultFactory<MetaTest>()
		//.hook([](void*, Visitor* v, void*) { return v->visit("pre", std::string("hi")); })
		//.var("variable1", &MetaTest::m_variable1)
		//.var("variable2", &MetaTest::m_variable2)
		//.var("variable3", &MetaTest::m_variable3)
		//.var("variable4", &MetaTest::m_variable4)
		//.var("array1", &MetaTest::m_array1)
		//.var("object1", &MetaTest::m_object1)
		//.var("pointer1", &MetaTest::m_pointer1)
		//.var("pointer2", &MetaTest::m_pointer2)
		.func("f", &MetaTest::f, { "argi", "argf" }, { 255, 255.9f })
		.func("s", &MetaTest::s, { "s" } /*, { std::string("default") }*/);
		//.hook([](void*, Visitor* v, void*) { return v->visit("post", std::string("bye")); });
}

template<> 
Object Meta::instanceMeta<MetaTest::InnerMetaTest>()
{
	return Object("InnerMetaTest")
		.var("variable1", &MetaTest::InnerMetaTest::m_variable1);
}

template<> Object Meta::instanceMeta<glm::vec4>()
{
	return Object("glm::vec4")
		.defaultFactory<glm::vec4>()
		.constructor<glm::vec4, float, float, float, float>()
		.copyOperator<glm::vec4>()
		.var("x", &glm::vec4::x)
		.var("y", &glm::vec4::y)
		.var("z", &glm::vec4::z)
		.var("w", &glm::vec4::w);
}

template<> Object Meta::instanceMeta<glm::vec3>()
{
	return Object("glm::vec3")
		.defaultFactory<glm::vec3>()
		.constructor<glm::vec3, float, float, float>()
		.copyOperator<glm::vec3>()
		.var("x", &glm::vec3::x)
		.var("y", &glm::vec3::y)
		.var("z", &glm::vec3::z);
}

template<> Object Meta::instanceMeta<glm::vec2>()
{
	return Object("glm::vec2")
		.defaultFactory<glm::vec2>()
		.constructor<glm::vec2, float, float>()
		.copyOperator<glm::vec2>()
		.var("x", &glm::vec2::x)
		.var("y", &glm::vec2::y);
}

template<> Object Meta::instanceMeta<glm::quat>()
{
	return Object("glm::quat")
		.defaultFactory<glm::quat>()
		.constructor<glm::quat, float, float, float, float>()
		.copyOperator<glm::quat>()
		.var("x", &glm::quat::x)
		.var("y", &glm::quat::y)
		.var("z", &glm::quat::z)
		.var("w", &glm::quat::w);
}

void Meta::test()
{
	MetaTest t;
	Serializer v;

	t.m_array1.push_back(1);
	t.m_array1.push_back(2);
	t.m_array1.push_back(3);
	t.m_variable4 = "this is a \" quote ";
	t.m_object1.m_variable1 = 4;
	t.m_pointer1 = &t.m_variable2;
	t.m_pointer2 = &t.m_object1;
	Meta::visit(&v, "t", &t);

	Object& o = getMeta<MetaTest>();
	o.call<MetaTest, void>("f", &t, 23423, 123.4f);

	ResourcePtr<ScriptManager> s;
	s->setEditorContent(v.getString().c_str(), nullptr);
}

}