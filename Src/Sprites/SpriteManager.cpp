#include "stdafx.h"
#include "SpriteManager.h"
#include "SpriteData.h"
#include "../Misc/Misc.h"
#include "../Managers/EventManager.h"
#include "../Rendering/Unit.h"
#include "../Rendering/RenderingDevice.h"
#include "../Rendering/TextureAtlas.h"

SpriteManager::SpriteManager():
m_idSprites(),
m_nameSprites(),
m_nextSpriteId()
{
	m_nextSpriteId.m_value = 0u;
}

SpriteManager::~SpriteManager()
{
	m_atlases.clear();
	m_nameSprites.clear();
	m_idSprites.clear();
}

SpriteId SpriteManager::getSprite(const char* path)
{
	SpriteId sprite;
	std::string normalizedPath = normalizePath(path);
	auto namedIt = m_nameSprites.find(normalizedPath);
	if (namedIt == m_nameSprites.end())
	{
		// load it
		ResourcePtr<SpriteData> data{ NewPtr, normalizedPath.c_str() };
		m_nameSprites.insert(decltype(m_nameSprites)::value_type(normalizedPath, data));
		m_nextSpriteId.m_value++;
		sprite = m_nextSpriteId;
		m_idSprites.insert(decltype(m_idSprites)::value_type(sprite, data));

		ResourcePtr<EventManager> events;
		events->addListener<ResourceStateChanged>([this, data, normalizedPath](ResourceStateChanged* c) {
			if (data == c->m_resourceData)
			{
				onSpriteLoaded(data, normalizedPath);
				// TODO: discardListener() when sprite gets deleted
				//c->discardListener();
			}
		});
	}

	if (sprite)
		return sprite;
	
	// find it
	for (auto& it : m_idSprites)
	{
		if (it.second == namedIt->second)
			return it.first;
	}

	return { }; // fail
}

std::tuple<SpriteData*, Rendering::TextureAtlas*> SpriteManager::getSpriteData(SpriteId id)
{
	auto it = m_idSprites.find(id);
	return it == m_idSprites.end() ? 
		std::tuple<SpriteData*, Rendering::TextureAtlas*>( nullptr, nullptr ) :
		std::tuple<SpriteData*, Rendering::TextureAtlas*>{ &(*it->second), findTexture(it->second) };
}

Rendering::TextureAtlas* SpriteManager::findTexture(const ResourcePtr<SpriteData>& data)
{
	for (auto& atlasData : m_atlases)
	{
		for (auto& sprite : atlasData->m_sprites)
			if (sprite == data)
				return atlasData->m_atlas;
	}

	return nullptr;
}

void SpriteManager::onSpriteLoaded(const ResourcePtr<SpriteData>& sprite, StringView id)
{
	std::shared_ptr<AtlasData> atlasData = nullptr;
	auto it = std::find_if(m_atlases.begin(), m_atlases.end(), [&](const std::shared_ptr<AtlasData>& a) { return a->m_id == id.str(); });
	if (it == m_atlases.end())
	{
		m_atlases.push_back(std::make_shared<AtlasData>(new Rendering::TextureAtlas));
		atlasData = m_atlases.back();
	}
	else
	{
		atlasData = *it;
		atlasData->m_atlas = g_resourceManager->addLoadedResource(new Rendering::TextureAtlas, "Texture Atlas");
	}
	
	atlasData->m_atlas->addSprite(sprite);
	atlasData->m_atlas->setPadding(2);
	atlasData->m_atlas->layoutAtlas();
	atlasData->m_sprites.push_back(sprite);
	atlasData->m_id = id.str();

	ResourcePtr<Rendering::Device> device;
	Rendering::Unit upload = device->createUnit();
	upload.in(atlasData->m_atlas);
	upload.out(*atlasData->m_atlas);
	upload.submit();
}

void SpriteManager::imgui()
{

}

SpriteManager::AtlasData::AtlasData(Rendering::TextureAtlas* atlas):
m_atlas(g_resourceManager->addLoadedResource(atlas, "Sprite Atlas"))
{
	
}

template<> Meta::Object Meta::instanceMeta<SpriteId>()
{
	return Object("SpriteId");
}