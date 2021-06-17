#include "stdafx.h"
#include "LuaEnvironment.h"

LuaEnvironment::LuaEnvironment(lua_State* lua):
m_state(lua),
m_ownsState(false)
{
	if (!lua)
	{
		m_state = luaL_newstate();
		m_ownsState = true;
	}
}

LuaEnvironment::~LuaEnvironment()
{
	if (m_ownsState)
		lua_close(m_state);
}

const char* LuaEnvironment::getName() const
{
	return "Lua";
}

bool LuaEnvironment::isScript(const char* path) const
{
	std::size_t size = strlen(path);
	return (size >= 3 && path[size - 3] == '.' && path[size - 2] == 'l' && path[size - 1] == 'u' && path[size - 1] == 'a');
}

bool LuaEnvironment::newScript(Script, const char* debugName)
{
	return true;
}

void LuaEnvironment::deleteScript(Script)
{

}

LuaEnvironment::Error LuaEnvironment::loadScript(Script, StringView)
{
	return {};
}

bool LuaEnvironment::registerObject(const Meta::Object&, const char* exposedName, const char* doc, std::tuple<void*, const char*> instance)
{
	return true;
}