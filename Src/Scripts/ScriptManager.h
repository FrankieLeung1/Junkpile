#pragma once

#include "../Resources/ResourceManager.h"

struct lua_State;
class TextEditor;
namespace Meta { class Object; }
class ScriptManager : public SingletonResource<ScriptManager>
{
public:
	class Environment
	{
	public:
		typedef void* Script;

	public:
		virtual ~Environment() = 0 {};
		virtual const char* getName() const = 0;
		virtual bool isScript(const char* path) const = 0;
		virtual Script newScript(const char* debugName) =0;
		virtual void deleteScript(Script) = 0;
		virtual std::tuple<std::string, int> loadScript(Script, const char* buffer, std::size_t size) =0;
		virtual bool registerObject(const Meta::Object&, const char* exposedName, const char* doc, std::tuple<void*, const char*> instance = {}) =0;
	};

public:
	ScriptManager();
	~ScriptManager();

	void runScriptsInFolder(const char* path, bool recursive = false);
	bool run(const char* path);

	template<typename T> void registerObject(const char* exposedName, const char* doc = nullptr, std::tuple<T*, const char*> instance = {});
	template<typename T> void addEnvironment();

	void setEditorContent(const char*);
	lua_State* getLua() const;
	void imgui();

protected:
	void initEditor();
	Environment* getEnvironment(const char* name) const;

protected:
	lua_State* m_state{ nullptr };
	TextEditor* m_editor{ nullptr };
	bool m_needsToCheckSyntax{ false };
	
	std::list<Environment*> m_languages;

	friend class PythonEnvironment;
};

// ----------------------- IMPLEMENTATION ----------------------- 
template<typename T> 
void ScriptManager::registerObject(const char* exposedName, const char* doc, std::tuple<T*, const char*> instance)
{
	for(auto* l : m_languages)
		l->registerObject(Meta::getMeta<T>(), exposedName, doc, std::tie(std::get<0>(instance), std::get<1>(instance)));
}

template<typename T>
void ScriptManager::addEnvironment()
{
	m_languages.push_back(new T);
}