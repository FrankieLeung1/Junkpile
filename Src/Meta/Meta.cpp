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
m_name(name)
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
		case Base::BaseType::Hook: static_cast<Hook&>(it).~Hook(); break;
		}
	}
}

const char* Object::getName() const
{
	return m_name;
}

Object& Object::hook(const BasicFunction<int, Visitor*, void*>& fn)
{
	m_variables.emplace_back<Meta::Hook>(fn);
	return *this;
}

int Object::visit(Visitor* v, void* o)
{
	// TODO: do something with return values

	for (auto& it : m_variables)
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

			case Base::BaseType::Hook:
			{
				Hook& hook = (Hook&)it;
				hook.visit(v, o);
			}
			break;
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
	if (m_defaultCount)
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

	return m_implementation->visit(v, m_name, object, argNames, defaults);
}

const char* Meta::Function::getName() const
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

struct MetaTest
{
	bool m_variable1{ false };
	int m_variable2{ 0 };
	float m_variable3{ 0.0f };
	std::string m_variable4{ "test" };
	std::vector<int> m_array1;

	struct InnerMetaTest {
		int m_variable1;
	};
	InnerMetaTest m_object1;

	int* m_pointer1{ nullptr };
	InnerMetaTest* m_pointer2{ nullptr };

	void f(int i, float f) { LOG_F(INFO, "Meta Called! %d %f\n", i, f); }
};

template<>
Object Meta::getMeta<MetaTest>()
{
	return Object("MetaTest")
		/*.hook([](void*, Visitor* v, void*) { return v->visit("pre", std::string("hi")); })
		.var("variable1", &MetaTest::m_variable1)
		.var("variable2", &MetaTest::m_variable2)
		.var("variable3", &MetaTest::m_variable3)
		.var("variable4", &MetaTest::m_variable4)
		.var("array1", &MetaTest::m_array1)
		.var("object1", &MetaTest::m_object1)
		.var("pointer1", &MetaTest::m_pointer1)
		.var("pointer2", &MetaTest::m_pointer2)*/
		.func("f", &MetaTest::f, { "argi", "argf" }, { 255, 255.9f });
		//.hook([](void*, Visitor* v, void*) { return v->visit("post", std::string("bye")); });
}

template<>
Object Meta::getMeta<MetaTest::InnerMetaTest>()
{
	return Object("InnerMetaTest")
		.var("variable1", &MetaTest::InnerMetaTest::m_variable1);
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

	Object& o = getMetaSingleton<MetaTest>();
	o.call<MetaTest, void>("f", &t, 23423, 123.4f);

	ResourcePtr<ScriptManager> s;
	s->setEditorContent(v.getString().c_str());
}

}