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
m_nextSpriteId(1)
{

}

SpriteManager::~SpriteManager()
{
	m_atlases.clear();
	m_nameSprites.clear();
	m_idSprites.clear();
}

SpriteId SpriteManager::getSprite(const char* path)
{
	SpriteId sprite = 0;
	std::string normalizedPath = normalizePath(path);
	auto namedIt = m_nameSprites.find(normalizedPath);
	if (namedIt == m_nameSprites.end())
	{
		// load it
		ResourcePtr<SpriteData> data{ NewPtr, normalizedPath.c_str() };
		m_nameSprites.insert(decltype(m_nameSprites)::value_type(normalizedPath, data));
		sprite = ++m_nextSpriteId;
		m_idSprites.insert(decltype(m_idSprites)::value_type(sprite, data));

		ResourcePtr<EventManager> events;
		events->addListener<ResourceStateChanged>([=](const ResourceStateChanged* c) {
			if (data == c->m_resourceData)
			{
				this->onSpriteLoaded(data);
				return EventManager::ListenerResult::Discard;
			}
			return EventManager::ListenerResult::Persist;
		});
	}

	if (sprite)
		return sprite;
	
	// find it
	for (auto it : m_idSprites)
	{
		if (it.second == namedIt->second)
			return it.first;
	}

	return 0; // fail
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

void SpriteManager::onSpriteLoaded(const ResourcePtr<SpriteData>& sprite)
{
	m_atlases.push_back(std::make_shared<AtlasData>(new Rendering::TextureAtlas));
	auto& atlasData = m_atlases.back();
	
	atlasData->m_atlas->addSprite(sprite);
	atlasData->m_atlas->setPadding(2);
	atlasData->m_atlas->layoutAtlas();
	atlasData->m_sprites.push_back(sprite);

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
m_atlas(g_resourceManager->addLoadedResource(atlas, "Texture Atlas"))
{
	
}