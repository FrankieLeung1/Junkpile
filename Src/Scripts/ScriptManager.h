#pragma once

#include "../Resources/ResourceManager.h"

struct lua_State;
class TextEditor;
class ScriptManager : public SingletonResource<ScriptManager>
{
public:
	ScriptManager();
	~ScriptManager();

	void runInitScripts();

	void setEditorContent(const char*);
	lua_State* getLua() const;
	void imgui();

protected:
	void initEditor();

protected:
	lua_State* m_state{ nullptr };
	TextEditor* m_editor{ nullptr };
	bool m_needsToCheckSyntax{ false };
};