#pragma once

#include <string>
#include <vector>

#include "../Resources/ResourceManager.h"

class File;
namespace Rendering
{
	class Device;
	class Texture;
}

class SpriteData : public Resource
{
public:
	struct FrameData
	{
		int m_id;
		float m_time;
		float m_duration;
		std::shared_ptr<Rendering::Texture> m_texture;
	};
	std::vector<FrameData> m_frames;

public:
	SpriteData();
	~SpriteData();

	bool loadFromGif(ResourcePtr<File>, Rendering::Device&);

	void addFrame(const FrameData&);

	const FrameData& getFrame(float time) const;
	float getDuration() const;

	glm::vec2 getDimensions() const;

public:
	class SpriteDataLoader : public Loader
	{
	public:
		ResourcePtr<File> m_file;
		SpriteDataLoader(const char* filePath);
		SpriteData* load(std::tuple<int, std::string>* error);
		const char* getTypeName() const;
	};

	template<typename... Ts>
	static SpriteDataLoader* createLoader(Ts&&... args)
	{
		return new SpriteDataLoader(std::forward<Ts>(args)...);
	}

protected:
	std::string m_path;
};