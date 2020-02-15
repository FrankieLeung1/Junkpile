#pragma once

#include "../Misc/SparseStructures.h"

class TileLevel
{
public:
	TileLevel();
	~TileLevel();

	void imgui();

protected:
	struct Tile;
	struct Level;

	Tile* getTile(const glm::ivec3&);
	void setTile(const glm::ivec3&, Tile*);
	void removeTile(const glm::ivec3&);

protected:
	int m_tileSize{ 20 };

	struct Tile
	{
		ImColor m_color{ 255, 255, 255 };
	};

	struct Level
	{
		SparseArray< SparseArray<Tile> > m_grid;
	};
	SparseArray<Level> m_levels;
};