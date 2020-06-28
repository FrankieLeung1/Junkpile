#include "stdafx.h"
#include "ScriptManager.h"
#include "../imgui/ImGuiManager.h"
#include "../LuaHelpers.h"
#include "../Files/FileManager.h"
#include "../Managers/TimeManager.h"
#include "../Misc/Misc.h"
#include "ImGuiColorTextEdit/TextEditor.h"

ScriptManager::Environment::Script ScriptManager::Environment::InvalidScript = reinterpret_cast<void*>(std::numeric_limits<std::size_t>::max());
ScriptManager::ScriptManager():
m_state(luaL_newstate())
{
	LuaStackCheck::s_defaultState = m_state;
	ResourcePtr<EventManager> e;
	ResourcePtr<TimeManager> t;

	e->addListener<FileChangeEvent>([this](FileChangeEvent* c) { onFileChange(*c); });
}

ScriptManager::~ScriptManager()
{
	delete m_editor;
	lua_close(m_state);

	for (auto& l : m_languages)
		delete l;
}

void ScriptManager::runScriptsInFolder(const char* path, bool recursive)
{
	ResourcePtr<FileManager> fileManagers;
	std::vector<FileManager::FileInfo> files = fileManagers->files(path);
	for (const FileManager::FileInfo& current : files)
	{
		const std::string path = current.m_path;
		if (recursive && current.m_type == FileManager::Type::Directory)
			runScriptsInFolder(path.c_str(), true);
		else if (current.m_type == FileManager::Type::File && !run(path.c_str()))
			break;
	}
}

bool ScriptManager::run(const char* path, Environment::Script script, Environment::Script owner)
{
	for (auto& languages : m_languages)
	{
		if (languages->isScript(path))
		{
			m_scripts.emplace_front();
			ScriptData& data = m_scripts.front();
			data.m_script = script;
			data.m_path = path;
			data.m_env = languages;

			m_callstack.push_back(&data);

			if(script == Environment::InvalidScript)
				script = languages->newScript(path);

			Environment::Error error;
			ResourcePtr<File> f(NewPtr, path);
			do
			{
				error = languages->loadScript(script, f->getContents(), f->getSize());
				m_error = error;
				if (!error)
					break;

				char* content = (char*)alloca(f->getSize() + 1);
				memcpy(content, f->getContents(), f->getSize());
				content[f->getSize()] = '\0';
				setEditorContent(content, path);

				ResourcePtr<ImGuiManager> imgui;
				imgui->imguiLoop([f]() { return f->checkChanged(); });
			} while (1);

			m_callstack.pop_back();

			return true;
		}
	}
	return false;
}

lua_State* ScriptManager::getLua() const
{
	return m_state;
}

void ScriptManager::setEditorContent(const char* content, const char* pathToSave)
{
	initEditor();
	m_editor->SetText(content);
	if (pathToSave)	m_editorSavePath = pathToSave;
	else			m_editorSavePath.clear();
}

void ScriptManager::initEditor()
{
	if (!m_editor)
	{
		m_editor = new TextEditor;
		auto lang = TextEditor::LanguageDefinition::Lua();
		m_editor->SetLanguageDefinition(lang);
	}
}

ScriptManager::Environment* ScriptManager::getEnvironment(const char* name) const
{
	for (auto& l : m_languages)
		if (strcmp(l->getName(), name) == 0)
			return l;

	return nullptr;
}

void ScriptManager::onFileChange(const FileChangeEvent& e)
{
	std::set<ScriptData*> scriptsToReload;
	for (auto path : e.m_files)
	{
		// direct script
		for (auto& script : m_scripts)
		{
			if (endsWith(script.m_path, path.c_str(), path.length()))
			{
				scriptsToReload.insert(&script);
				break;
			}
		}

		// is user of script?
	userOfScript:
		for (auto& script : m_scripts)
		{
			if (scriptsToReload.find(&script) != scriptsToReload.end())
				continue;

			for (auto& child : script.m_children)
			{
				if (scriptsToReload.find(child) != scriptsToReload.end())
				{
					scriptsToReload.insert(&script);
					goto userOfScript;
				}
			}
		}
	}

	ResourcePtr<EventManager> events;
	ScriptReloadedEvent* scriptEvent = events->addOneFrameEvent<ScriptReloadedEvent>();
	while (!scriptsToReload.empty())
	{
		LOG_F(INFO, "reloading %s\n", e.m_files.front().c_str());

		ScriptData* script = *scriptsToReload.begin();
		ResourcePtr<File> f(NewPtr, script->m_path.c_str());
		f->checkChanged();
		run(script->m_path.c_str(), script->m_script);
		scriptEvent->m_paths.push_back(script->m_path);
		scriptsToReload.erase(scriptsToReload.begin());
	}
}

void ScriptManager::addDependency(const char* name)
{
	if (m_callstack.empty())
		return;

	m_callstack.back()->m_dependencies.insert(name);
}

void ScriptManager::imgui()
{
	ResourcePtr<ImGuiManager> imgui;
	bool* opened = imgui->win("Script Editor");
	if (!*opened && !m_error)
		return;

	initEditor();

	auto cpos = m_editor->GetCursorPosition();
	ImGui::Begin("Script editor", m_error ? nullptr : opened, ImGuiWindowFlags_HorizontalScrollbar);
	ImGui::SetWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);

	//ImGui::Button("Open External"); ImGui::SameLine();
	ImGui::Text("%6d/%-6d %6d lines | %s | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, m_editor->GetTotalLines(),
		m_editor->IsOverwrite() ? "Ovr" : "Ins",
		m_editor->CanUndo() ? "*" : " ",
		m_editor->GetLanguageDefinition().mName.c_str(), m_editorSavePath.c_str());

	std::map<int, std::string> markers;
	markers[m_error.m_line] = m_error.m_message;
	m_editor->SetErrorMarkers(markers);

	ImGuiIO& io = ImGui::GetIO();
	if (!m_editorSavePath.empty() && io.KeysDown[GLFW_KEY_LEFT_CONTROL] && io.KeysDown[GLFW_KEY_S])
	{
		ResourcePtr<FileManager> f;
		std::string s = m_editor->GetText();
		std::vector<char> content(s.begin(), s.end());
		f->save(m_editorSavePath.c_str(), std::move(content));
	}

	m_editor->Render("TextEditor");

	ImGui::End();
}