#pragma once

#include "../Resources/ResourceManager.h"
#include "../Managers/EventManager.h"
#include "Markup.h"

struct lua_State;
class TextEditor;
class ScriptManager;
struct FileChangeEvent;
struct ScriptUnloadedEvent : public Event<ScriptUnloadedEvent>
{
	// TODO: replace this with Script or something
	std::vector<StringView> m_paths;
	bool m_reloading{ false };
};
struct ScriptLoadedEvent : public Event<ScriptLoadedEvent>
{
	// TODO: replace this with Script or something
	std::vector<StringView> m_paths;
	bool m_reloading{ false };
};
struct ScriptRemarkEvent : public Event<ScriptRemarkEvent>
{
	// TODO: replace this with Script or something
	std::vector<StringView> m_paths;
};

namespace Meta { class Object; }
class ScriptManager : public SingletonResource<ScriptManager>
{
public:
	class Environment
	{
	public:
		struct Script : public OpaqueHandle <Script, ScriptManager, std::size_t, std::numeric_limits<std::size_t>::max()> { Script() {} friend class Python; friend class PythonEnvironment; };

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
		virtual Error loadScript(Script, StringView) =0;
		virtual bool registerObject(const Meta::Object&, const char* exposedName, const char* doc, std::tuple<void*, const char*> instance = {}) =0;
	};

public:
	ScriptManager();
	~ScriptManager();

	void runScriptsInFolder(StringView path, bool recursive = false);
	bool run(const char* path, Environment::Script script = Environment::Script(), Environment::Script owner = Environment::Script());
	void remark(const char* path);

	template<typename T> void registerObject(const char* exposedName, const char* doc = nullptr, std::tuple<T*, const char*> instance = {});
	template<typename T> void addEnvironment();

	Environment::Script getRunningScript() const;
	StringView getScriptPath(Environment::Script) const;

	void showEditor();
	void hideEditor();
	void setEditorContent(const char* content, const char* pathToSave);

	lua_State* getLua() const;
	void imgui();

protected:
	void initEditor();
	Environment* getEnvironment(const char* name) const;
	void onFileChange(const FileChangeEvent&);
	void addDependency(const char* name);

	void runScriptsInFolder(StringView path, bool recursive, std::vector<StringView>* filesRan);

	bool imguiColourPicker4(StringView name, ImGuiColorEditFlags flags, float colour[4], float prevColour[4]);
	std::string getDefaultValue(const Markup&, int index, bool stripWhitespace) const;

	void registerObjects();

	static bool s_inited;
	friend class EventManager;

protected:
	lua_State* m_state{ nullptr };
	TextEditor* m_editor{ nullptr };
	std::string m_editorSavePath;
	struct ScriptData
	{
		Environment* m_env;
		Environment::Script m_script;
		std::vector<ScriptData*> m_children;
		std::set<std::string> m_dependencies;
		std::string m_path;
		Markup m_markup;
	};

	ScriptData* m_editorScriptData;
	
	std::list<Environment*> m_languages;
	std::forward_list<ScriptData> m_scripts;
	std::vector<ScriptData*> m_callstack;

	std::stack<Environment::Script> m_scriptStack;

	glm::vec4 m_currentColour, m_prevColour;
	int m_colourIndexOpened;

	Environment::Error m_error;
	bool m_objectsRegistered;

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