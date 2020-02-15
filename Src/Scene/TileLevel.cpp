#include "stdafx.h"
#include "TileLevel.h"
#include "imgui/imgui.h"
#include "../imgui/ImguiManager.h"

TileLevel::TileLevel()
{

}

TileLevel::~TileLevel()
{

}

void TileLevel::imgui()
{
	ResourcePtr<ImGuiManager> im;
	bool* opened = im->win("Tile Level");
	if (!*opened)
		return;

	using namespace ImGui;
	ImColor colour = ImColor(1.0f, 1.0f, 1.0f, 0.1f);

	SetNextWindowViewport(GetMainViewport()->ID);
	Begin("TileLevel", opened, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
	ImDrawList* drawList = GetBackgroundDrawList();

	ImGuiIO& io = GetIO();
	ImGuiViewport* viewport = GetMainViewport();
	const ImVec2& viewportPos = viewport->Pos;
	const ImVec2& viewportSize = viewport->Size;

	for (int x = 0; x <= viewportSize.x; x += m_tileSize)
	{
		ImVec2 p1 = { viewportPos.x + x, 0.0f };
		ImVec2 p2 = { viewportPos.x + x, viewportPos.y + viewportSize.y };
		drawList->AddLine(p1, p2, colour);
	}

	for (int y = 0; y <= viewportSize.y; y += m_tileSize)
	{
		ImVec2 p1 = { viewportPos.x, viewportPos.y + y };
		ImVec2 p2 = { viewportPos.x + viewportSize.x, viewportPos.y + y };
		drawList->AddLine(p1, p2, colour);
	}

	ImVec2 cursor(io.MousePos.x - viewportPos.x, io.MousePos.y - viewportPos.y);
	ImVec2 hoveredTile(floor(cursor.x / m_tileSize), floor(cursor.y / m_tileSize));
	if (!io.WantCaptureMouse)
	{
		ImVec2 center(viewportPos.x + hoveredTile.x * m_tileSize, viewportPos.y + hoveredTile.y * m_tileSize);
		drawList->AddRectFilled(ImVec2(center.x, center.y), ImVec2(center.x + m_tileSize, center.y + m_tileSize), ImColor(1.0f, 0.5, 1.0f), 0.25);

		if (io.MouseDown[0])
		{
			Tile t;
			setTile({ hoveredTile.x, hoveredTile.y, 0 }, &t);
		}
	}

	int tilesDrawn = 0;
	if (!m_levels.empty())
	{
		auto levelIt = m_levels.begin();
		for (auto y = levelIt->m_grid.begin(); y != levelIt->m_grid.end(); ++y)
		{
			for (auto x = y->begin(); x != y->end(); ++x)
			{
				ImVec2 center(viewportPos.x + x.m_index * m_tileSize, viewportPos.y + y.m_index * m_tileSize);
				drawList->AddRectFilled(ImVec2(center.x, center.y), ImVec2(center.x + m_tileSize, center.y + m_tileSize), x->m_color, 0.25);
				tilesDrawn++;
			}
		}
	}

	End();

	Begin("Tile Editor", nullptr, 0);
	SliderInt("Tile Size", &m_tileSize, 1, 40);
	Text("TilesDrawn: %d", tilesDrawn);
	End();
}

void TileLevel::setTile(const glm::ivec3& coords, Tile* tile)
{
	Level* l = m_levels.get(coords.z);
	if (!l)
		l = m_levels.set<Level&&>(coords.z, Level());

	SparseArray<Tile>* row = l->m_grid.get(coords.y);
	if (!row)
		row = l->m_grid.set(coords.y, std::move(SparseArray<Tile>()));

	row->set(coords.x, *tile);
}

void TileLevel::removeTile(const glm::ivec3& coords)
{
	Level* l = m_levels.get(coords.z);
	if (l)
	{
		auto* row = l->m_grid.get(coords.y);
		if (row)
		{
			row->erase(coords.x);
			if (row->empty())
			{
				l->m_grid.erase(coords.y);
				if (l->m_grid.empty())
					m_levels.erase(coords.z);
			}
		}
	}
}

TileLevel::Tile* TileLevel::getTile(const glm::ivec3& coords)
{
	Level* l = m_levels.get(coords.z);
	if (l)
	{
		auto* row = l->m_grid.get(coords.y);
		return row ? row->get(coords.x) : nullptr;
	}

	return nullptr;
}