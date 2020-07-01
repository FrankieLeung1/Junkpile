#include "stdafx.h"
#include "SpriteData.h"
#include "../Files/File.h"
#include "../Rendering/Texture.h"
#include "../Misc/Misc.h"
#include "stb_rect_pack.h"

#pragma warning( push )
#pragma warning(disable:4244)
#include "gif_load/gif_load.h"
#pragma warning( pop )

SpriteData::SpriteData()
{

}

SpriteData::~SpriteData()
{
	/*for(std::vector<FrameData>::iterator it = m_frames.begin(); it != m_frames.end(); ++it)
		delete it->m_texture;*/
}

static void frame(void* ud, GIF_WHDR* whdr)
{
	CHECK_F(whdr->mode == GIF_CURR);
	SpriteData* sprite = static_cast<SpriteData*>(ud);
	SpriteData::FrameData data;
	data.m_duration = (float)whdr->time / 100.0f;
	data.m_texture = std::make_shared<Rendering::Texture>();

	// build RGBA data
	struct RGBA { unsigned char r, g, b, a; };
	int pixelSize = sizeof(RGBA);
	int width = whdr->xdim, height = whdr->ydim;
	data.m_texture->setSoftware(width, height, pixelSize);
	char* textureData = (char*)data.m_texture->map();

	if (whdr->ifrm == 0)
	{
		data.m_id = 0;
		data.m_time = 0;
		memset(textureData, 0x5A, width * height * pixelSize);
	}
	else
	{
		// init with previous frame
		const SpriteData::FrameData& prev = sprite->m_frames[whdr->ifrm - 1];
		void* prevData = prev.m_texture->map();
			memcpy(textureData, prevData, width * height * pixelSize);
		prev.m_texture->unmap();

		data.m_time = prev.m_time + data.m_duration;
		data.m_id = prev.m_id + 1;
		
	}

	for (int y = 0; y < whdr->fryd; y++)
	{
		RGBA* current = (RGBA*)&textureData[(((y + whdr->fryo) * width) + whdr->frxo) * sizeof(RGBA)];

		for (int x = 0; x < whdr->frxd; x++)
		{
			uint8_t index = whdr->bptr[(y * whdr->frxd) + x];
			if (index != whdr->tran && index != whdr->bkgd)
			{
				current->r = whdr->cpal[index].R;
				current->g = whdr->cpal[index].G;
				current->b = whdr->cpal[index].B;
				current->a = 255;
			}
			current++;
		}
	}

	data.m_texture->unmap();
	sprite->addFrame(std::move(data));
}

bool SpriteData::loadFromGif(ResourcePtr<File> f, Rendering::Device& d)
{
	std::tuple<int, std::string> error;
	std::size_t size = f->getSize();
	const char* content = f->getContents();

	GIF_Load(const_cast<char*>(content), (long)size, frame, nullptr, this, 0);
	return true;
}

void SpriteData::addFrame(const FrameData& fd)
{
	m_frames.push_back(fd);
}

const SpriteData::FrameData& SpriteData::getFrame(float time) const
{
	float totalDuration = getDuration();
	while (time >= totalDuration)
		time -= totalDuration;

	std::vector<FrameData>::const_iterator it = m_frames.begin();
	while (time < it->m_time || time >= it->m_time + it->m_duration)
		++it;

	return *it;
}

float SpriteData::getDuration() const
{
	const FrameData& f = m_frames.back();
	return f.m_time + f.m_duration;
}

glm::vec2 SpriteData::getDimensions() const
{
	glm::vec2 d(0.0f);
	for (const FrameData& f : m_frames)
	{
		d.x = std::max(d.x, (float)f.m_texture->getWidth());
		d.y = std::max(d.y, (float)f.m_texture->getHeight());
	}
	return d;
}

SpriteData::SpriteDataLoader::SpriteDataLoader(const char* filePath) :
	m_file(NewPtr, filePath)
{

}

SpriteData* SpriteData::SpriteDataLoader::load(std::tuple<int, std::string>* error)
{
	if (!ready(error, m_file))
		return nullptr;

	SpriteData* data = new SpriteData;
	std::size_t size = m_file->getSize();
	const char* content = m_file->getContents();

	GIF_Load(const_cast<char*>(content), (long)size, frame, nullptr, data, 0);

	return data;
}

StringView SpriteData::SpriteDataLoader::getTypeName() const
{
	return "Sprite Data";
}