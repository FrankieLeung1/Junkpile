#include "stdafx.h"
#include "TextureAtlas.h"
#include "../Misc/Misc.h"
#include "../Sprites/SpriteData.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

using namespace Rendering;

TextureAtlas::TextureAtlas():
m_padding(0),
m_frames()
{

}

TextureAtlas::~TextureAtlas()
{

}

void TextureAtlas::setPadding(unsigned int padding)
{
	m_padding = padding;
}

int TextureAtlas::addFrame(Texture* t)
{
	LOG_IF_F(ERROR, m_mode != Texture::Mode::EMPTY, "Texture Atlas must be empty before adding frames\n");

	m_textures.push_back(t);
	return static_cast<int>(m_textures.size() - 1);
}

void TextureAtlas::addSprite(SpriteData* sprite)
{
	for (SpriteData::FrameData& frame : sprite->m_frames)
	{
		int id = addFrame(frame.m_texture.get());
		CHECK_F(id == frame.m_id);
	}
}

void TextureAtlas::layoutAtlas()
{
	LOG_IF_F(ERROR, m_mode != Texture::Mode::EMPTY, "Texture Atlas must be empty before laying out");

	auto smallestArea = [](Texture* t1, Texture* t2) {
		return t1->getWidth() * t1->getHeight() < t2->getWidth() * t2->getHeight();
	};

	auto maxElement = std::max_element(m_textures.begin(), m_textures.end(), smallestArea);

	stbrp_rect* rects = (stbrp_rect*)alloca(m_textures.size() * sizeof(stbrp_rect));
	// subtract one in case we're already a power of 2, we want to keep it
	int width = nextPowerOf2((*maxElement)->getWidth() - 1), height = nextPowerOf2((*maxElement)->getHeight() - 1);
	while (true) // find the width/height of our final texture
	{
		memset(rects, 0x00, m_textures.size() * sizeof(stbrp_rect));
		for (int i = 0; i < m_textures.size(); ++i)
		{
			rects[i].id = i;
			rects[i].w = m_textures[i]->getWidth() + m_padding;
			rects[i].h = m_textures[i]->getHeight() + m_padding;
		}

		stbrp_node nodes[512];
		stbrp_context packer;
		stbrp_init_target(&packer, width, height, nodes, 512);
		int result = stbrp_pack_rects(&packer, rects, (int)m_textures.size());
		if (result == 1)
			break;

		if(width < height)	width = nextPowerOf2(width);
		else				height = nextPowerOf2(height);
	}

	// let's write it into pixels
	int pixelSize = m_textures[0]->getPixelSize();
	setSoftware(width, height, pixelSize);
	char* pixels = (char*)map();
	unsigned int halfPadding = m_padding / 2;
	for (int i = 0; i < m_textures.size(); i++)
	{
		Texture* st = m_textures[i];
		BitBltBuffer d = { pixels, (std::size_t)pixelSize, width, height };
		BitBltBuffer s = { (char*)st->map(), (std::size_t)st->getPixelSize(), st->getWidth(), st->getHeight() };
		stbrp_rect* rect = rects + i;
		bitblt(d, rect->x + halfPadding, rect->y + halfPadding, rect->w - m_padding, rect->h - m_padding, s, 0, 0);
		st->unmap();

		Frame frame;
		frame.m_id = i;
		frame.m_uv1 = { (float)rect->x / (float)width, (float)rect->y / (float)height};
		frame.m_uv2 = { (float)(rect->x + rect->w) / width, (float)(rect->y + rect->h) / (float)height };
		m_frames.push_back(std::move(frame));
	}
	unmap();
}

std::tuple< glm::vec2, glm::vec2 > TextureAtlas::getUV(int frameId) const
{
	if (m_frames.empty())
	{
		LOG_F(ERROR, "Must call layoutAtlas() before getting UVs");
		return {};
	}

	const Frame& frame = m_frames[frameId];
	return{ frame.m_uv1, frame.m_uv2 };
}