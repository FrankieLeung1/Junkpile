#pragma once

#include "../Resources/ResourceManager.h"

class ComponentManager;
class SpriteData;
namespace Rendering
{
	class TextureAtlas;
}

class SpriteSystem : public SingletonResource<SpriteSystem>
{
public:
	SpriteSystem();
	~SpriteSystem();

	void process(float);
	void render();

	void imgui();

	static void test(std::function<void(float)>& update, std::function<void()>& render);

protected:
	ResourcePtr<ComponentManager> m_components;
	std::vector<ResourcePtr<SpriteData>> m_spriteData;
	std::vector<Rendering::TextureAtlas> m_textures;

	static glm::vec4 m_clearColour;
};