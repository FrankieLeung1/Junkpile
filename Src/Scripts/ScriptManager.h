#pragma once

#include "../Resources/ResourceManager.h"

struct lua_State;
class TextEditor;
class ScriptManager : public SingletonResource<ScriptManager>
{
public:
	class Environment
	{
	public:
		typedef void* Script;

	public:
		virtual ~Environment() = 0 {};
		virtual bool isScript(const char* path) const = 0;
		virtual Script newScript(const char* debugName) =0;
		virtual void deleteScript(Script) = 0;
		virtual std::tuple<std::string, int> loadScript(Script, const char* buffer, std::size_t size) =0;
	};

public:
	ScriptManager();
	~ScriptManager();

	void runScriptsInFolder(const char* path, bool recursive = false);
	bool run(const char* path);

	template<typename T>
	void addEnvironment();

	void setEditorContent(const char*);
	lua_State* getLua() const;
	void imgui();

protected:
	void initEditor();

protected:
	lua_State* m_state{ nullptr };
	TextEditor* m_editor{ nullptr };
	bool m_needsToCheckSyntax{ false };
	
	std::list<Environment*> m_languages;
};

// ----------------------- IMPLEMENTATION ----------------------- 
template<typename T>
void ScriptManager::addEnvironment()
{
	m_languages.push_back(new T);
}