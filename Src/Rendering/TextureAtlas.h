#pragma once

#include "Texture.h"

class SpriteData;
namespace Rendering
{
	class TextureAtlas : public Texture
	{
	public:
		TextureAtlas(const glm::u8vec4& padding = glm::vec4(0.0f));
		~TextureAtlas();

		void setPadding(const glm::u8vec4&);

		int addFrame(Texture*);
		void addSprite(SpriteData*);

		void layoutAtlas();

		std::tuple< glm::vec2, glm::vec2 > getUV(int frameId) const;

	protected:
		std::vector<Texture*> m_textures;
		glm::u8vec4 m_padding;

		struct Frame
		{
			int m_id;
			glm::vec2 m_uv1;
			glm::vec2 m_uv2;
		};
		std::vector<Frame> m_frames;
	};
}