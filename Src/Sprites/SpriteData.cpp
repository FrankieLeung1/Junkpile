#include "stdafx.h"
#include "SpriteData.h"
#include "../Files/FileManager.h"
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

struct WorkingData
{
	int m_mode;
	SpriteData* m_sprite;
};

static void frame(void* ud, GIF_WHDR* whdr)
{
	CHECK_F(whdr->mode == GIF_NONE || whdr->mode == GIF_CURR || whdr->mode == GIF_BKGD); // TODO: GIF_PREV = init with the data from two frames previous
	WorkingData* workingData = static_cast<WorkingData*>(ud);
	SpriteData* sprite = workingData->m_sprite;
	SpriteData::FrameData data;
	data.m_duration = (float)whdr->time / 100.0f;
	data.m_texture = ResourcePtr<Rendering::Texture>(NewPtr);

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
		const SpriteData::FrameData& prev = sprite->m_frames[whdr->ifrm - 1];
		if (workingData->m_mode == GIF_CURR)
		{
			// init with previous frame
			void* prevData = prev.m_texture->map();
			memcpy(textureData, prevData, width * height * pixelSize);
			prev.m_texture->unmap();
		}

		data.m_time = prev.m_time + prev.m_duration;
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
	workingData->m_mode = whdr->mode;
}

bool SpriteData::loadFromGif(ResourcePtr<File> f, Rendering::Device& d)
{
	std::tuple<int, std::string> error;
	std::size_t size = f->getSize();
	const char* content = f->getContents();

	WorkingData data{0, this};
	GIF_Load(const_cast<char*>(content), (long)size, frame, nullptr, &data, 0);
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

	std::size_t i = 0;
	std::vector<FrameData>::const_iterator it = m_frames.begin();
	while (time >= it->m_time + it->m_duration)
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

StringView SpriteData::getPath() const
{
	return m_path;
}

SpriteData::SpriteDataLoader::SpriteDataLoader(StringView filePath) :
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
	std::string ext = FileManager::extension(m_file->getPath());
	if (ext == "png")
	{
		data->addFrame({ 0, 0, std::numeric_limits<float>::max(), Rendering::Texture::loadPng(*m_file) });
	}
	else if(ext == "gif")
	{
		WorkingData workingData{ 0, data };
		GIF_Load(const_cast<char*>(content), (long)size, frame, nullptr, &workingData, 0);
	}
	else if(ext == "py")
	{
		data->addFrame({ 0, 0, std::numeric_limits<float>::max(), Rendering::Texture::loadPy(*m_file) });
	}
	else
	{
		LOG_F(WARNING, "Unable to load sprite \"%s\"\n", m_file->getPath().c_str());
	}

	return data;
}

SpriteData::Reloader* SpriteData::SpriteDataLoader::createReloader()
{
	std::string path = m_file->getPath();
	auto create = [path]() { return new SpriteDataLoader(path); };
	return new ReloaderOnFileChange(path, create);
}

StringView SpriteData::SpriteDataLoader::getTypeName() const
{
	return "Sprite Data";
}