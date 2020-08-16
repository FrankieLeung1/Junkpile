#pragma once

#include "../Resources/ResourceManager.h"
#include "../ECS/ComponentManager.h"
#include "SpriteManager.h"

class SpriteData;
namespace Rendering
{
	class Shader;
	class Buffer;
	class TextureAtlas;
}

struct SpriteComponent : public Component<SpriteComponent>
{
	SpriteId m_sprite;
	float m_time;
};

struct RenderEvent;
class SpriteSystem : public SingletonResource<SpriteSystem>
{
public:
	SpriteSystem();
	~SpriteSystem();

	void process(float);
	void render(const RenderEvent&);

	void imgui();

	SpriteComponent* addComponent(Entity, StringView spritePath);

	static void test(std::function<void(float)>& update, std::function<void()>& render);

	static glm::vec4 m_clearColour;

protected:
	ResourcePtr<ComponentManager> m_components;
	std::vector<ResourcePtr<SpriteData>> m_spriteData;
	std::vector<Rendering::TextureAtlas> m_textures;

	struct Vertex{ glm::vec3 m_position; glm::vec2 m_uv; };
	ResourcePtr<Rendering::Shader> m_vertexShader, m_fragmentShader;
	std::shared_ptr<Rendering::Buffer> m_vertexBuffer, m_indexBuffer;
};

namespace Meta {
	template<> Object instanceMeta<SpriteComponent>();
	template<> Object instanceMeta<SpriteSystem>();
}