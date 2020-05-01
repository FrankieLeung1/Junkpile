#pragma once

#include "../Resources/ResourceManager.h"
#include "../ECS/ComponentManager.h"
#include "SpriteManager.h"

class SpriteData;
namespace Rendering
{
	class TextureAtlas;
}

struct SpriteComponent : public Component<SpriteComponent>
{
	SpriteId m_sprite;
	float m_time;
};

class SpriteSystem : public SingletonResource<SpriteSystem>
{
public:
	SpriteSystem();
	~SpriteSystem();

	void process(float);
	void render();

	void imgui();

	static void test(std::function<void(float)>& update, std::function<void()>& render);

	static glm::vec4 m_clearColour;

protected:
	ResourcePtr<ComponentManager> m_components;
	std::vector<ResourcePtr<SpriteData>> m_spriteData;
	std::vector<Rendering::TextureAtlas> m_textures;

	
};