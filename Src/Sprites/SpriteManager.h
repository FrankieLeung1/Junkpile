#pragma once

#include "../Resources/ResourceManager.h"
#include "../Misc/Misc.h"
#include "../ECS/ECS.h"

class SpriteData;
class SpriteManager;
namespace Rendering { class TextureAtlas; }
struct SpriteId : public OpaqueHandle<SpriteManager, unsigned int> { };
class SpriteManager : public SingletonResource<SpriteManager>
{
public:
	SpriteManager();
	~SpriteManager();

	SpriteId getSprite(const char* path);
	SpriteId getSprite(ResourcePtr<Rendering::Texture> texture);
	std::tuple<SpriteData*, Rendering::TextureAtlas*> getSpriteData(SpriteId);

	void imgui();

protected:
	void onSpriteLoaded(const ResourcePtr<SpriteData>&, StringView id);
	Rendering::TextureAtlas* findTexture(const ResourcePtr<SpriteData>&);

protected:
	std::map<SpriteId, ResourcePtr<SpriteData>> m_idSprites;
	std::map<std::string, ResourcePtr<SpriteData>> m_nameSprites;
	
	struct AtlasData
	{
		std::string m_id;
		ResourcePtr<Rendering::TextureAtlas> m_atlas;
		std::vector<ResourcePtr<SpriteData>> m_sprites;
		AtlasData(Rendering::TextureAtlas*);
	};
	std::vector<std::shared_ptr<AtlasData>> m_atlases;

	SpriteId m_nextSpriteId;
};

namespace Meta{
	template<> Object instanceMeta<SpriteId>();
}