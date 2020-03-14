#pragma once

#include "../Resources/ResourceManager.h"

struct lua_State;
class TextEditor;
struct FileChangeEvent;
namespace Meta { class Object; }
class ScriptManager : public SingletonResource<ScriptManager>
{
public:
	class Environment
	{
	public:
		typedef void* Script;
		static Script InvalidScript;

		struct Error
		{
			std::string m_message, m_filename, m_stacktrace;
			int m_line, m_offset;
			operator bool() { return !m_message.empty(); };
		};

	public:
		virtual ~Environment() = 0 {};
		virtual const char* getName() const = 0;
		virtual bool isScript(const char* path) const = 0;
		virtual Script newScript(const char* debugName) =0;
		virtual void deleteScript(Script) = 0;
		virtual Error loadScript(Script, const char* buffer, std::size_t size) =0;
		virtual bool registerObject(const Meta::Object&, const char* exposedName, const char* doc, std::tuple<void*, const char*> instance = {}) =0;
	};

public:
	ScriptManager();
	~ScriptManager();

	void runScriptsInFolder(const char* path, bool recursive = false);
	bool run(const char* path, Environment::Script script = Environment::InvalidScript, Environment::Script owner = Environment::InvalidScript);

	template<typename T> void registerObject(const char* exposedName, const char* doc = nullptr, std::tuple<T*, const char*> instance = {});
	template<typename T> void addEnvironment();

	void setEditorContent(const char* content, const char* pathToSave);
	lua_State* getLua() const;
	void imgui();

protected:
	void initEditor();
	Environment* getEnvironment(const char* name) const;
	void onFileChange(const FileChangeEvent&);
	void addDependency(const char* name);

protected:
	lua_State* m_state{ nullptr };
	TextEditor* m_editor{ nullptr };
	std::string m_editorSavePath;
	
	std::list<Environment*> m_languages;

	struct ScriptData
	{
		Environment* m_env;
		Environment::Script m_script;
		std::vector<ScriptData*> m_children;
		std::set<std::string> m_dependencies;
		std::string m_path;
	};
	std::forward_list<ScriptData> m_scripts;
	std::vector<ScriptData*> m_callstack;

	Environment::Error m_error;

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