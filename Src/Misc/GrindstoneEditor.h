#pragma once

class TextEditor;
class LuaTableResource;
class File;
class GrindstoneEditor
{
public:
	GrindstoneEditor();
	~GrindstoneEditor();

	void imgui();

protected:
	std::string loadData();

protected:
	ResourcePtr<LuaTableResource> m_data;
	std::vector<char> m_rewards;
	std::vector<char> m_entities;
	ResourcePtr<File> m_logoFile;
	ResourcePtr<Rendering::Texture> m_logoTexture;

	TextEditor* m_textEditor;
};