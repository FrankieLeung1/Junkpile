#include "stdafx.h"
#include "ScriptManager.h"
#include "../imgui/ImGuiManager.h"
#include "../LuaHelpers.h"
#include "../Files/FileManager.h"
#include "../Managers/TimeManager.h"
#include "../Misc/Misc.h"
#include "ImGuiColorTextEdit/TextEditor.h"

bool ScriptManager::s_inited = false;
ScriptManager::Environment::Script ScriptManager::Environment::InvalidScript = { reinterpret_cast<void*>(std::numeric_limits<std::size_t>::max()) };
ScriptManager::ScriptManager() :
	m_state(luaL_newstate()),
	m_editorScriptData(nullptr),
	m_colourIndexOpened(-1),
	m_objectsRegistered(false)
{
	LuaStackCheck::s_defaultState = m_state;
	ResourcePtr<EventManager> e;
	e->addListener<FileChangeEvent>([this](FileChangeEvent* c) { onFileChange(*c); });

	s_inited = true;
}

ScriptManager::~ScriptManager()
{
	delete m_editor;
	lua_close(m_state);

	for (auto& l : m_languages)
		delete l;
}

void ScriptManager::runScriptsInFolder(StringView path, bool recursive, ScriptLoadedEvent* event)
{
	ResourcePtr<FileManager> fileManagers;
	std::vector<FileManager::FileInfo> files = fileManagers->files(path);
	if (files.empty())
		return;

	if (!event)
	{
		ResourcePtr<EventManager> events;
		event = events->addOneFrameEvent<ScriptLoadedEvent>();
	}

	for (const FileManager::FileInfo& current : files)
	{
		const std::string path = current.m_path;
		if (recursive && current.m_type == FileManager::Type::Directory)
			runScriptsInFolder(path.c_str(), true, event);
		else if (current.m_type == FileManager::Type::File)
		{
			if(run(path.c_str()))
				event->m_paths.push_back(path);
		}
	}
	event->m_reloading = false;
}

bool ScriptManager::run(const char* path, Environment::Script script, Environment::Script owner)
{
	registerObjects();

	for (auto& language : m_languages)
	{
		if (language->isScript(path))
		{
			m_scripts.emplace_front();
			ScriptData& data = m_scripts.front();
			data.m_script = script;
			data.m_path = path;
			data.m_env = language;

			m_callstack.push_back(&data);

			if (script == Environment::InvalidScript)
			{
				script = language->newScript(path);
				data.m_script = script;
			}

			Environment::Error error;
			ResourcePtr<File> f(NewPtr, path);
			do
			{
				data.m_markup.parseScript(f->getContents());
				std::string marked = data.m_markup.markUp(f->getContents());
				m_scriptStack.push(script);
				error = language->loadScript(script, marked);
				m_scriptStack.pop();
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

void ScriptManager::remark(const char* path)
{
	for (auto it = m_scripts.begin(); it != m_scripts.end(); ++it)
	{
		if (it->m_path == path)
		{
			ResourcePtr<File> f(NewPtr, path);
			std::string marked = it->m_markup.markUp(f->getContents());
			it->m_env->loadScript(it->m_script, marked);
		}
	}
}

lua_State* ScriptManager::getLua() const
{
	return m_state;
}

ScriptManager::Environment::Script ScriptManager::getRunningScript() const
{
	return m_scriptStack.empty() ? Environment::InvalidScript : m_scriptStack.top();
}

StringView ScriptManager::getScriptPath(Environment::Script script) const
{
	auto it = std::find_if(m_scripts.begin(), m_scripts.end(), [script](const ScriptData& scriptData) { return scriptData.m_script == script; });
	return it == m_scripts.end() ? StringView() : StringView(it->m_path);
}

void ScriptManager::setEditorContent(const char* content, const char* pathToSave)
{
	initEditor();
	if (!content && pathToSave)
	{
		ResourcePtr<File> file(NewPtr, pathToSave);
		m_editor->SetText(std::string(file->getContents().c_str(), file->getSize()));
	}
	else
	{
		m_editor->SetText(content);
	}

	ResourcePtr<FileManager> files;
	std::string ext = files->extension(pathToSave);
	if(ext == "lua")		m_editor->SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
	else if(ext == "py")	m_editor->SetLanguageDefinition(TextEditor::LanguageDefinition::Python());

	if (pathToSave)
	{
		m_editorSavePath = pathToSave;
		auto it = std::find_if(m_scripts.begin(), m_scripts.end(), [=](const ScriptData& d) { return ("../Res/" + d.m_path) == pathToSave; });
		m_editorScriptData = it != m_scripts.end() ? &(*it) : nullptr;
	}
	else
	{
		m_editorSavePath.clear();
		m_editorScriptData = nullptr;
	}
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

	if (scriptsToReload.empty())
		return;

	ResourcePtr<EventManager> events;
	ScriptUnloadedEvent* scriptUnloadedEvent = events->addOneFrameEvent<ScriptUnloadedEvent>();
	for(ScriptData* script : scriptsToReload)
	{
		LOG_F(INFO, "unloading %s\n", script->m_path.c_str());
		ResourcePtr<File> f(NewPtr, script->m_path.c_str());
		f->checkChanged();
		scriptUnloadedEvent->m_paths.push_back(script->m_path);
	}
	scriptUnloadedEvent->m_reloading = true;

	events->addListener<ScriptUnloadedEvent>([events, scriptsToReload, this](ScriptUnloadedEvent* e) {
		ScriptLoadedEvent* scriptLoadedEvent = events->addOneFrameEvent<ScriptLoadedEvent>();
		for (ScriptData* script : scriptsToReload)
		{
			LOG_F(INFO, "reloading %s\n", script->m_path.c_str());

			run(script->m_path.c_str(), script->m_script);
			scriptLoadedEvent->m_paths.push_back(script->m_path);
		}
		scriptLoadedEvent->m_reloading = true;
		e->discardListener();
	}, -10);
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
	ImGui::Begin("Script Editor", m_error ? nullptr : opened, ImGuiWindowFlags_HorizontalScrollbar);
	ImGui::SetWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);

	if (m_editorScriptData)
		ImGui::Columns(2);

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
		std::vector<char> content(s.begin(), s.end() - 1); // BUG: TextEditor adds a newline to the end of GetText(), don't save it
		f->save(m_editorSavePath.c_str(), std::move(content));
	}

	m_editor->Render("TextEditor");
	
	if (m_editorScriptData)
	{
		Markup& markup = m_editorScriptData->m_markup;
		ImGui::NextColumn();

		bool remark = false;
		const std::vector<Markup::Mark>& marks = markup.getMarks();
		for(int i = 0; i < marks.size(); i++)
		{
			const Markup::Mark& mark = marks[i];
			switch(mark.m_type)
			{
			case Markup::VariableType::Float:
				{
					float f;
					if (!markup.getValue(i, &f))
						sscanf((char*)getDefaultValue(markup, i, true).c_str(), "%f", &f);

					if (ImGui::InputFloat(markup.getName(i).c_str(), &f))
					{
						markup.setValue(i, f);
						remark = true;
					}
				}
				break;
			case Markup::VariableType::String:
				{
					std::string s;
					if (!markup.getValue(i, &s))
					{
						std::string d = getDefaultValue(markup, i, false);
						if (d.front() == '"') d.erase(d.begin());
						if (d.back() == '"') d.erase(d.end() - 1);
						s = d;
					}

					char buffer[512];
					strcpy_s(buffer, s.c_str());
					if (ImGui::InputText(markup.getName(i), buffer, countof(buffer)))
					{
						markup.setValue(i, buffer);
						remark = true;
					}
				}
				break;
			case Markup::VariableType::RGB:
			case Markup::VariableType::RGBA:
				{
					bool hasAlpha = (mark.m_type == Markup::VariableType::RGBA);
					glm::vec4 c(1.0f, 0.0f, 0.0f, 1.0f);
					if (!markup.getValue(i, &c))
					{
						std::string d = getDefaultValue(markup, i, true);
						sscanf((char*)d.c_str(), hasAlpha ? "%f,%f,%f,%f" : "%f,%f,%f", &c.x, &c.y, &c.z, &c.w);
					}

					const char* name = markup.getName(i).c_str();
					if (ImGui::ColorButton(name, ImVec4(c.x, c.y, c.z, c.w)))
					{
						ImGui::OpenPopup("colourPicker");
						m_colourIndexOpened = i;
						m_currentColour = m_prevColour = c;
					}
					ImGui::SameLine();
					ImGui::Text(name);
				}
				break;
			}
		}
		
		if (imguiColourPicker4("colourPicker", 0, &m_currentColour.x, &m_prevColour.x))
		{
			remark = true;
			markup.setValue(m_colourIndexOpened, m_currentColour);
		}

		if (remark)
		{
			ResourcePtr<EventManager> e;
			auto event = e->addOneFrameEvent<ScriptRemarkEvent>();
			event->m_paths.push_back(m_editorScriptData->m_path);
		}
	}

	ImGui::End();
}

std::string ScriptManager::getDefaultValue(const Markup& mark, int index, bool stripWhitespace) const
{
	std::string s;
	mark.getDefault(index, &s);
	if(stripWhitespace)
		s.erase(std::remove_if(s.begin(), s.end(), [](char c) {return c == ' '; }));

	return std::move(s);
}

bool ScriptManager::imguiColourPicker4(StringView name, ImGuiColorEditFlags flags, float colour[4], float prevColour[4])
{
	static bool saved_palette_init = true;
	static ImVec4 saved_palette[32] = {};
	if (saved_palette_init)
	{
		for (int n = 0; n < IM_ARRAYSIZE(saved_palette); n++)
		{
			ImGui::ColorConvertHSVtoRGB(n / 31.0f, 0.8f, 0.8f,
				saved_palette[n].x, saved_palette[n].y, saved_palette[n].z);
			saved_palette[n].w = 1.0f; // Alpha
		}
		saved_palette_init = false;
	}

	ImVec4 imColour(colour[0], colour[1], colour[2], colour[3]), imPrevColour(prevColour[0], prevColour[1], prevColour[2], prevColour[3]);
	bool valueChanged = false;
	if (ImGui::BeginPopup(name))
	{
		//ImGui::Text("MY CUSTOM COLOR PICKER WITH AN AMAZING PALETTE!");
		//ImGui::Separator();
		valueChanged = ImGui::ColorPicker3("##picker", colour, flags | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
		ImGui::SameLine();

		ImGui::BeginGroup(); // Lock X position
		ImGui::Text("Current");
		ImGui::ColorButton("##current", imColour, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(60, 40));
		ImGui::Text("Previous");
		if (ImGui::ColorButton("##previous", imPrevColour, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(60, 40)))
		{
			memcpy(colour, prevColour, sizeof(float) * 3);
			valueChanged = true;
		}

		/*ImGui::Separator();
		ImGui::Text("Palette");
		for (int n = 0; n < IM_ARRAYSIZE(saved_palette); n++)
		{
			ImGui::PushID(n);
			if ((n % 8) != 0)
				ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.y);

			ImGuiColorEditFlags palette_button_flags = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoTooltip;
			if (ImGui::ColorButton("##palette", saved_palette[n], palette_button_flags, ImVec2(20, 20)))
				*colour = ImVec4(saved_palette[n].x, saved_palette[n].y, saved_palette[n].z, colour->w); // Preserve alpha!

			// Allow user to drop colors into each palette entry. Note that ColorButton() is already a
			// drag source by default, unless specifying the ImGuiColorEditFlags_NoDragDrop flag.
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
					memcpy((float*)&saved_palette[n], payload->Data, sizeof(float) * 3);
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
					memcpy((float*)&saved_palette[n], payload->Data, sizeof(float) * 4);
				ImGui::EndDragDropTarget();
			}

			ImGui::PopID();
		}*/
		ImGui::EndGroup();
		ImGui::EndPopup();
	}

	return valueChanged;
}

// all the classes exposed the scripts
#include "../Generators/TextureGenerator.h"
#include "../Scene/TransformSystem.h"
#include "../Sprites/SpriteSystem.h"
#include "../Managers/EventManager.h"
#include "../Scene/CameraSystem.h"
#include "../Scripts/Python.h"
void ScriptManager::registerObjects()
{
	if (!m_objectsRegistered)
	{
		m_objectsRegistered = true;

		addEnvironment<PythonEnvironment>();
		registerObject<TextureGenerator>("TextureGenerator");
		registerObject<glm::vec2>("vec2");
		registerObject<glm::vec3>("vec3");
		registerObject<glm::vec4>("vec4");
		registerObject<UpdateEvent>("UpdateEvent");
		registerObject<Entity>("Entity");
		registerObject<TransformComponent>("TransformComponent");
		registerObject<SpriteComponent>("SpriteComponent");
		registerObject<CameraComponent>("CameraComponent");
		registerObject<EventManager>("EventManager", nullptr, std::make_tuple(ResourcePtr<EventManager>(NewPtr).get(), "eventManager"));
		registerObject<ComponentManager>("ComponentManager", nullptr, std::make_tuple(ResourcePtr<ComponentManager>(NewPtr).get(), "componentManager"));
		registerObject<TransformSystem>("TransformSystem", nullptr, std::make_tuple(ResourcePtr<TransformSystem>(NewPtr).get(), "transformSystem"));
		registerObject<SpriteSystem>("SpriteSystem", nullptr, std::make_tuple(ResourcePtr<SpriteSystem>(NewPtr).get(), "spriteSystem"));
		registerObject<CameraSystem>("CameraSystem", nullptr, std::make_tuple(ResourcePtr<CameraSystem>(NewPtr).get(), "cameraSystem"));
	}
}