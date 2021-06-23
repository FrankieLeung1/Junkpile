#pragma once

#include "../Sprites/SpriteData.h"
namespace Rendering { class TextureAtlas; }
class SpriteEditor
{
public:
	SpriteEditor();
	~SpriteEditor();

	void imgui();

protected:
	SpriteData m_sprite;
	std::shared_ptr<Rendering::TextureAtlas> m_atlas;
	std::size_t m_currentFrame;
	int m_comboSelection;

	struct Hitbox
	{
		std::string m_name;
		glm::vec3 m_min, m_max;
		unsigned int m_colour;
	};

	struct FrameData
	{
		int m_index;
		std::vector<Hitbox> m_hitboxes;
	};
	std::vector<FrameData> m_frameData;
};