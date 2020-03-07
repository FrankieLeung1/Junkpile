#include "stdafx.h"
#include "LuaRegisterer.h"
#include "../LuaHelpers.h"
#include "Meta.h"

namespace Meta {

LuaRegisterer::LuaRegisterer():
m_functions()
{
	
}

LuaRegisterer::~LuaRegisterer()
{

}

int LuaRegisterer::visit(const char* name, bool&)
{
	return 0;
}

int LuaRegisterer::visit(const char* name, int&)
{
	return 0;
}

int LuaRegisterer::visit(const char* name, float&)
{
	return 0;
}

int LuaRegisterer::visit(const char* name, std::string&)
{
	return 0;
}

int LuaRegisterer::visit(const char* name, bool*)
{
	return 0;
}

int LuaRegisterer::visit(const char* name, int*)
{
	return 0;
}

int LuaRegisterer::visit(const char* name, float*)
{
	return 0;
}

int LuaRegisterer::visit(const char* name, std::string*)
{
	return 0;
}

int LuaRegisterer::visit(const char* name, void* object, const Object&)
{
	return 0;
}

int LuaRegisterer::startObject(const char* name, const Meta::Object& objectInfo)
{
	return 0;
}

int LuaRegisterer::endObject()
{
	return 0;
}

int LuaRegisterer::startArray(const char* name)
{
	return 0;
}

int LuaRegisterer::endArray(std::size_t)
{
	return 0;
}

int LuaRegisterer::startFunction(const char* name, bool hasReturn)
{
	return 0;
}

int LuaRegisterer::endFunction()
{
	return 0;
}

int LuaRegisterer::call(lua_State*)
{
	return 0;
}

void LuaRegisterer::open(const Object& o)
{

}

struct Test
{
	int foo() { LOG_F(INFO, "called foo\n"); return 99; }
};

template<>
Object Meta::instanceMeta<Test>()
{
	return Object("LuaRegistererTest")
		.func("foo", &Test::foo);
}

void LuaRegisterer::test()
{
	lua_State* state = luaL_newstate();
	LuaRegisterer r;

	typedef int(*F)(int, std::string);
	F f = [](int i, std::string name) { LOG_F(INFO, "called %d \"%s\"\n", i, name.c_str()); return 0; };
	pushCFunction(state, f);
	lua_setglobal(state, "f");
	luaL_dostring(state, "f(3, \"test\")");

	lua_close(state);
}

}