#include "stdafx.h"
#include "Game.h"
#include "../imgui/ImGuiManager.h"
#include "../imgui/FileTree.h"
#include "../Rendering/Texture.h"
#include "../Rendering/RenderingDevice.h"
#include "../Framework/Framework.h"

Game::Game():
	m_currentLayer(0)
{
	m_layers.push_back(Layer{ "Layer 1" });
	m_layers.push_back(Layer{ "Layer 2" });
	m_layers.push_back(Layer{ "Layer 3" });

	m_layers[0].m_subLayers.push_back({ "sub1", SubLayer::Tilemap });
	m_layers[0].m_subLayers.push_back({ "sub2", SubLayer::Collision });
	m_layers[0].m_subLayers.push_back({ "sub3", SubLayer::Entities });
	m_layers[0].m_subLayers.push_back({ "sub4", SubLayer::Data });
}

Game::~Game()
{

}

void Game::imgui()
{
	ResourcePtr<ImGuiManager> manager;
	bool* game = manager->win("Game");
	if (!*game)
		return;

	ImGui::Begin("Game", game);

	Layer& layer = m_layers[m_currentLayer];
	if (layer.m_selectedLayers.size() != layer.m_subLayers.size())
		layer.m_selectedLayers.resize(layer.m_subLayers.size(), false);

	ImGui::Columns((int)layer.m_subLayers.size() + 1);
	SubLayer* currentSubLayer = nullptr;
	for (int i = 0; i < layer.m_subLayers.size(); ++i)
	{
		SubLayer& sublayer = layer.m_subLayers[i];
		std::vector<bool>& selected = layer.m_selectedLayers;

		if (ImGui::Selectable(sublayer.m_name.c_str(), selected[i]))
		{
			std::fill(selected.begin(), selected.end(), false);
			selected[i] = true;
			currentSubLayer = &sublayer;
		}
		ImGui::NextColumn();
	}
	
	ImGui::SmallButton("M");
	ImGui::Columns(1);
	
	ImGui::BeginChild("SubLayerInfo", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true);
	std::size_t selectedCount = std::count(layer.m_selectedLayers.begin(), layer.m_selectedLayers.end(), true);
	if (selectedCount > 1)
	{
		ImGui::Text("Multi-select");
	}
	else if(selectedCount != 0)
	{
		for (int i = 0; i < layer.m_selectedLayers.size(); i++)
		{
			if(layer.m_selectedLayers[i])
				imgui(&layer.m_subLayers[i]);
		}
	}
	ImGui::EndChild();

	auto layerGetter = [](void* v, int index, const char** text) { *text = static_cast<Game*>(v)->m_layers[index].m_name.c_str(); return true; };
	ImGui::Combo("", &m_currentLayer, layerGetter, this, (int)m_layers.size()); ImGui::SameLine();
	ImGui::SmallButton("M");

	ImGui::End();
}

void Game::imgui(SubLayer* sublayer)
{
	ImGui::Text("Hi");
}

void Game::test()
{

}