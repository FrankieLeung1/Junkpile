#include "stdafx.h"
#include "ScriptManager.h"
#include "../imgui/ImGuiManager.h"
#include "../LuaHelpers.h"
#include "../Files/FileManager.h"
#include "ImGuiColorTextEdit/TextEditor.h"

ScriptManager::ScriptManager():
m_state(luaL_newstate())
{
	LuaStackCheck::s_defaultState = m_state;
}

ScriptManager::~ScriptManager()
{
	delete m_editor;
	lua_close(m_state);
}

void ScriptManager::runScriptsInFolder(const char* path, bool recursive)
{
	ResourcePtr<FileManager> fileManagers;
	std::vector<std::string> files = fileManagers->files(path);
	for (const std::string& current : files)
	{
		if (recursive && fileManagers->type(path) == FileManager::Type::Directory)
			runScriptsInFolder(current.c_str(), true);

		if (run(current.c_str()))
			break;
	}
}

bool ScriptManager::run(const char* path)
{
	for (auto& languages : m_languages)
	{
		if (languages->isScript(path))
		{
			auto script = languages->newScript(path);

			ResourcePtr<File> f(NewPtr, path);
			std::tuple<std::string, int> e = languages->loadScript(script, f->getContents(), f->getSize());
			LOG_IF_F(ERROR, std::get<int>(e) > -1, "%s (%d): %s\n", path, std::get<int>(e), std::get<std::string>(e));
			return true;
		}
	}
	return false;
}

lua_State* ScriptManager::getLua() const
{
	return m_state;
}

void ScriptManager::setEditorContent(const char* content)
{
	initEditor();
	m_editor->SetText(content);
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

void ScriptManager::imgui()
{
	ResourcePtr<ImGuiManager> imgui;
	bool* opened = imgui->win("Script Editor");
	if (!*opened)
		return;

	initEditor();

	auto cpos = m_editor->GetCursorPosition();
	ImGui::Begin("Script editor", opened, ImGuiWindowFlags_HorizontalScrollbar);
	ImGui::SetWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);

	//ImGui::Button("Open External"); ImGui::SameLine();
	ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s ", cpos.mLine + 1, cpos.mColumn + 1, m_editor->GetTotalLines(),
		m_editor->IsOverwrite() ? "Ovr" : "Ins",
		m_editor->CanUndo() ? "*" : " ",
		m_editor->GetLanguageDefinition().mName.c_str());

	ImGuiIO& io = ImGui::GetIO();
	if (m_needsToCheckSyntax && (!m_editor->GetErrorMarkers().empty() && m_editor->IsTextChanged()) || (io.MouseDelta.x != 0 || io.MouseDelta.y != 0) ||
		(io.KeysDown[GLFW_KEY_ENTER] || io.KeysDown[GLFW_KEY_LEFT] || io.KeysDown[GLFW_KEY_RIGHT] || io.KeysDown[GLFW_KEY_UP] || io.KeysDown[GLFW_KEY_DOWN]))
	{
		std::string s = m_editor->GetText();
		lua_State* state = luaL_newstate();
		int error = luaL_loadbuffer(state, s.c_str(), s.length(), "");

		if (error == LUA_ERRSYNTAX)
		{
			int line;
			char message[256] = { 0 };
			const char* error = lua_tostring(state, -1);
			int top = lua_gettop(state);
			int r = sscanf_s(error, "[string \"\"]:%d: %[^\t\n]", &line, message, (unsigned int)sizeof(message));
			CHECK_F(r == 2);

			std::map<int, std::string> markers;
			markers[std::min(line, m_editor->GetTotalLines())] = message;
			m_editor->SetErrorMarkers(markers);
		}
		else
		{
			m_editor->SetErrorMarkers({});
		}

		lua_close(state);
	}
	else if (m_editor->IsTextChanged())
	{
		m_needsToCheckSyntax = true;
	}

	m_editor->Render("TextEditor");

	ImGui::End();
}