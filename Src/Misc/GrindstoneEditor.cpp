#include "stdafx.h"
#include "GrindstoneEditor.h"
#include "../LuaHelpers.h"
#include "ImGuiColorTextEdit/TextEditor.h"
#include "../Files/File.h"
#include "../Rendering/Texture.h"
#include "../Rendering/Unit.h"

GrindstoneEditor::GrindstoneEditor() :
m_data(NewPtr, "Grindstone/GrindstoneData.lua", 0),
m_logoFile(NewPtr, "Grindstone/Logo.png"),
m_logoTexture(TakeOwnershipPtr, new Rendering::Texture),
m_textEditor(nullptr)
{
	m_textEditor = new TextEditor;
	auto lang = TextEditor::LanguageDefinition::Lua();
	m_textEditor->SetLanguageDefinition(lang);
}

GrindstoneEditor::~GrindstoneEditor()
{
	delete m_textEditor;
}

std::string GrindstoneEditor::loadData()
{
	if (!m_rewards.empty())
		return "";

	std::tuple<int, std::string> e;
	if (m_data.error(&e))
		return std::get<1>(e);

	unsigned char* pixels;
	unsigned int width, height;
	int error = lodepng_decode32(&pixels, &width, &height, (const unsigned char*)m_logoFile->getContents().c_str(), m_logoFile->getSize());
	if (error == 0)
	{
		m_logoTexture->setSoftware(width, height, 32);
		char* dest = (char*)m_logoTexture->map();
		memcpy(dest, pixels, width * height * 4);
		m_logoTexture->unmap();
		free(pixels);
	}

	auto loadIntoVector = [](const LuaTable& data, std::vector<char>& vector)
	{
		vector.reserve(2048);
		for (int i = 1; i <= data.getLength(); ++i)
		{
			std::string name = data.get<std::string>(i);
			for (auto c : name)
				vector.push_back(c);

			vector.push_back('\0');
		}
		vector.push_back('\0');
	};

	loadIntoVector(m_data->get<LuaTable>("rewards"), m_rewards);
	loadIntoVector(m_data->get<LuaTable>("entities"), m_entities);

	return "";
}

void GrindstoneEditor::imgui()
{
	static bool mainOpened = true;
	if (!mainOpened)
	{
		ResourcePtr<VulkanFramework> vf;
		vf->setShouldQuit(true);
		return;
	}

	std::string error = loadData();
	if (!error.empty())
	{
		ImGui::OpenPopup("Error");
		if (ImGui::BeginPopupModal("Delete?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Failed to read Grindstone.lua\n %s\n", error);
			ImGui::EndPopup();
		}
		return;
	}

	ImGui::Begin("Grindstone", &mainOpened);

	ImGui::Image(m_logoTexture.get(), ImVec2((float)m_logoTexture->getWidth(), (float)m_logoTexture->getHeight()));

	char name[256] = { '\0' }; ImGui::InputText("Name", name, 256);
	char bg[256] = { '\0' }; ImGui::InputText("bg", bg, 256);
	char amb[256] = { '\0' }; ImGui::InputText("amb", amb, 256);
	char song[256] = { '\0' }; ImGui::InputText("song", song, 256);

	int moves = 0; ImGui::InputInt("moves", &moves);
	int AggroRate = 0; ImGui::InputInt("AggroRate", &AggroRate);
	int DoorPower = 0; ImGui::InputInt("DoorPower", &DoorPower);
	int goalCreeps = 0; ImGui::InputInt("goalCreeps", &goalCreeps);
	int goalCrowns = 0; ImGui::InputInt("goalCrowns", &goalCrowns);
	int goalBigChests = 0; ImGui::InputInt("goalBigChests", &goalBigChests);
	int purples = 0; ImGui::InputInt("purples", &purples);
	int blues = 0; ImGui::InputInt("blues", &blues);
	int oranges = 0; ImGui::InputInt("oranges", &oranges);
	int reds = 0; ImGui::InputInt("reds", &reds);
	int greens = 0; ImGui::InputInt("greens", &greens);

	static int currentEntity = 0;
	static int currentReward = 0;
	ImGui::Combo("Entities", &currentEntity, &m_entities.front());
	ImGui::Combo("Rewards", &currentReward, &m_rewards.front());

	//ImGui::ImageButton(my_tex_id, ImVec2(32, 32), ImVec2(0, 0), ImVec2(32.0f / my_tex_w, 32 / my_tex_h), frame_padding, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

	ImGui::End();

	ImGui::Begin("Code");

	m_textEditor->Render("GrindstoneEditor");

	ImGui::End();
}