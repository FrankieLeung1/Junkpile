#pragma once

#include "../Resources/ResourceManager.h"

namespace Rendering
{
	class Texture;
}
class Game : public SingletonResource<Game>
{
public:
	Game();
	~Game();

	void imgui();
	static void test();

protected:
	struct Layer;
	struct SubLayer;
	void imgui(SubLayer*);

protected:
	struct SubLayer
	{
		enum Type {Tilemap, Collision, Entities, Data};
		std::string m_name;
		Type m_type;

		struct PaletteValue
		{
			std::string m_path;
			ResourcePtr<Rendering::Texture> m_texture;
		};
		std::vector<PaletteValue> m_palette;
		int m_cellSize{ 0 };
	};

	struct Layer
	{
		std::string m_name;
		std::vector<SubLayer> m_subLayers;
		std::vector<bool> m_selectedLayers;
	};

	std::vector<Layer> m_layers;
	int m_currentLayer;
};