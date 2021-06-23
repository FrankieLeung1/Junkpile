#pragma once

#include "Texture.h"

class SpriteData;
namespace Rendering
{
	class TextureAtlas : public Texture
	{
	public:
		TextureAtlas();
		~TextureAtlas();

		void setPadding(unsigned int padding);

		int addFrame(Texture*);
		void addSprite(SpriteData*);

		void layoutAtlas();

		std::tuple< glm::vec2, glm::vec2 > getUV(int frameId) const;

	protected:
		std::vector<Texture*> m_textures;
		unsigned int m_padding;

		struct Frame
		{
			int m_id;
			glm::vec2 m_uv1;
			glm::vec2 m_uv2;
		};
		std::vector<Frame> m_frames;

		// TODO: this class needs a loader
	};
}