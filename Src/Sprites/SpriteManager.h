#pragma once

#include "../Resources/ResourceManager.h"

typedef int SpriteId;
class SpriteData;
namespace Rendering { class TextureAtlas; }
class SpriteManager : public SingletonResource<SpriteManager>
{
public:
	SpriteManager();
	~SpriteManager();

	SpriteId getSprite(const char* path);
	std::tuple<SpriteData*, Rendering::TextureAtlas*> getSpriteData(SpriteId);

	void imgui();

protected:
	void onSpriteLoaded(const ResourcePtr<SpriteData>&);
	Rendering::TextureAtlas* findTexture(const ResourcePtr<SpriteData>&);

protected:
	std::map<SpriteId, ResourcePtr<SpriteData>> m_idSprites;
	std::map<std::string, ResourcePtr<SpriteData>> m_nameSprites;
	
	struct AtlasData
	{
		ResourcePtr<Rendering::TextureAtlas> m_atlas;
		std::vector<ResourcePtr<SpriteData>> m_sprites;
		AtlasData(Rendering::TextureAtlas*);
	};
	std::vector<std::shared_ptr<AtlasData>> m_atlases;

	SpriteId m_nextSpriteId;
};