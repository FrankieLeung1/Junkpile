#pragma once

#include "ScriptManager.h"
class LuaEnvironment : public ScriptManager::Environment
{
public:
	LuaEnvironment(lua_State* = nullptr);
	~LuaEnvironment();
	const char* getName() const;
	bool isScript(const char* path) const;
	bool newScript(Script, const char* debugName);
	void deleteScript(Script);
	Error loadScript(Script, StringView);
	bool registerObject(const Meta::Object&, const char* exposedName, const char* doc, std::tuple<void*, const char*> instance = {});

protected:
	lua_State* m_state;
	bool m_ownsState;
};