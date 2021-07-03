#include "stdafx.h"
#include "SpriteSystem.h"
#include "SpriteManager.h"
#include "SpriteData.h"
#include "../Files/File.h"
#include "../Rendering/Shader.h"
#include "../Rendering/RenderingDevice.h"
#include "../Rendering/TextureAtlas.h"
#include "../ECS/ECS.h"
#include "../ECS/ComponentManager.h"
#include "../Rendering/Unit.h"
#include "../Rendering/Buffer.h"
#include "../Misc/Misc.h"
#include "../Scene/CameraSystem.h"
#include "../Scene/TransformSystem.h"
#include "../Managers/EventManager.h"
#include "../Managers/DebugManager.h"

SpriteSystem::SpriteSystem():
m_vertexShader(EmptyPtr),
m_fragmentShader(EmptyPtr)
{
	m_components->addComponentType<SpriteComponent>();

	int spriteCount = 999;
	m_vertexBuffer = std::make_shared<Rendering::Buffer>(Rendering::Buffer::Vertex, Rendering::Buffer::Mapped, sizeof(Vertex) * 4 * spriteCount);
	m_vertexBuffer->setFormat({
			{vk::Format::eR32G32B32Sfloat, sizeof(glm::vec3)},
			{vk::Format::eR32G32Sfloat, sizeof(glm::vec2)},
		}, sizeof(Vertex));

	m_indexBuffer = std::make_shared<Rendering::Buffer>(Rendering::Buffer::Index, Rendering::Buffer::Mapped, sizeof(short) * 4);
	{
		short* index = (short*)m_indexBuffer->map();
		for (short i = 0; i < 4; i++)
			index[i] = i;

		m_indexBuffer->unmap();
	}
	m_indexBuffer->setFormat({ {vk::Format::eR16Sint, sizeof(short)} }, sizeof(short));

	char vertexCode[] =
		"#version 450 core\n"
		"layout(location = 0) in vec2 aPos;\n"
		"layout(location = 1) in vec2 aUV;\n"
		"layout(push_constant) uniform PushConsts{ mat4 vp; } pushConsts;\n"
		"out gl_PerVertex{ vec4 gl_Position; };\n"
		"layout(location = 0) out vec2 UV;\n"
		"void main()\n"
		"{\n"
		"	gl_Position = pushConsts.vp * vec4(aPos.xy, 1.0, 1.0);\n"
		"	UV = aUV;\n"
		"}";

	 m_vertexShader = ResourcePtr<Rendering::Shader>(NewPtr, Rendering::Shader::Type::Vertex, vertexCode);

	char pixelCode[] =
		"#version 450 core\n"
		"layout(location = 0) in vec2 UV;\n"
		"layout(location = 0) out vec4 fColor; \n"
		"layout(binding = 0) uniform sampler2D sTexture;\n"
		"void main()\n"
		"{\n"
		"	fColor = texture(sTexture, UV.st);\n"
		"}";

	m_fragmentShader = ResourcePtr<Rendering::Shader>(NewPtr, Rendering::Shader::Type::Pixel, pixelCode);

	ResourcePtr<EventManager> events;
	events->addListener<UpdateEvent>([this](UpdateEvent* e) { process(e->m_delta); });
	events->addListener<RenderEvent>([this](RenderEvent* e) { render(*e); });
}

SpriteSystem::~SpriteSystem()
{

}

void SpriteSystem::process(float delta)
{
	ResourcePtr<DebugManager> debugManager;
	ResourcePtr<SpriteManager> spriteManager;
	ResourcePtr<ComponentManager> components;
	EntityIterator<TransformComponent, SpriteComponent> it(components, true);
	Vertex* map = (Vertex*)m_vertexBuffer->map();
	while (it.next())
	{
		auto sprite = it.get<SpriteComponent>();
		sprite->m_time += delta;

		std::tuple<SpriteData*, Rendering::TextureAtlas*> spriteData = spriteManager->getSpriteData(sprite->m_sprite);
		if (!std::get<1>(spriteData))
			continue; // still loading

		glm::vec2 uv1, uv2;
		const SpriteData::FrameData& frame = std::get<0>(spriteData)->getFrame(sprite->m_time);
		std::tie(uv1, uv2) = std::get<1>(spriteData)->getUV(frame.m_id);

		TransformComponent* transform = it.get<TransformComponent>();
		const glm::vec3& scale = transform->m_scale;
		float halfWidth = (frame.m_texture->getWidth() / 2.0f) * scale.x;
		float halfHeight = (frame.m_texture->getHeight() / 2.0f) * scale.y;

		map[0] = Vertex{ transform->m_position + glm::vec3{ halfWidth, -halfHeight, 0.0f }, { uv2.x, uv2.y } };
		map[1] = Vertex{ transform->m_position + glm::vec3{ halfWidth, halfHeight, 0.0f }, { uv2.x, uv1.y } };
		map[2] = Vertex{ transform->m_position + glm::vec3{ -halfWidth, -halfHeight, 0.0f }, { uv1.x, uv2.y } };
		map[3] = Vertex{ transform->m_position + glm::vec3{ -halfWidth, halfHeight, 0.0f }, { uv1.x, uv1.y } };

		/*debugManager->addLine3D(glm::vec4(map[0].m_position, 1), glm::vec4(map[1].m_position, 1), glm::vec4(0, 0, 0, 1));
		debugManager->addLine3D(glm::vec4(map[1].m_position, 1), glm::vec4(map[3].m_position, 1), glm::vec4(0, 0, 0, 1));
		debugManager->addLine3D(glm::vec4(map[2].m_position, 1), glm::vec4(map[3].m_position, 1), glm::vec4(0, 0, 0, 1));
		debugManager->addLine3D(glm::vec4(map[2].m_position, 1), glm::vec4(map[0].m_position, 1), glm::vec4(0, 0, 0, 1));*/

		map += 4;
	}

	m_vertexBuffer->unmap();
}

void SpriteSystem::render(const RenderEvent& e)
{
	if (!ready(nullptr, m_vertexShader, m_fragmentShader))
		return;

	ResourcePtr<SpriteManager> spriteManager;
	ResourcePtr<ComponentManager> components;
	EntityIterator<SpriteComponent> it(components, true);

	ResourcePtr<Rendering::Device> device;

	int i = 0;
	while (it.next())
	{
		auto sprite = it.get<SpriteComponent>();
		std::tuple<SpriteData*, Rendering::TextureAtlas*> spriteData = spriteManager->getSpriteData(sprite->m_sprite);
		ResourcePtr<Rendering::Texture> texture(NoOwnershipPtr, std::get<1>(spriteData));
		if (!std::get<1>(spriteData))
			continue; // still loading

		Rendering::Unit unit(device->getRootUnit());
		unit.in(&(*m_vertexBuffer));
		unit.in(&(*m_indexBuffer));
		unit.in(m_vertexShader);
		unit.in(m_fragmentShader);
		unit.in(std::array<float, 4>{ m_clearColour.x, m_clearColour.y, m_clearColour.z, m_clearColour.w, });

		std::vector<char> pushData(sizeof(glm::mat4));
		memcpy(&pushData[0], &e.m_projection, sizeof(glm::mat4));
		unit.in({ vk::ShaderStageFlagBits::eVertex, std::move(pushData) });
		unit.in({ vk::ShaderStageFlagBits::eFragment, 0, texture });
		unit.in(Rendering::Unit::Draw{ 4, 1, (uint32_t)i++ * 4, 0 });
		unit.submit();
	}
}

void SpriteSystem::imgui()
{

}

SpriteComponent* SpriteSystem::addComponent(Entity e, StringView spritePath)
{
	ResourcePtr<SpriteManager> spriteManager;
	ResourcePtr<ComponentManager> components;
	SpriteComponent* sprite = components->addComponents<SpriteComponent>(e).get<SpriteComponent>();
	sprite->m_sprite = spriteManager->getSprite(spritePath);
	sprite->m_time = 0.0f;
	return sprite;
}

void SpriteSystem::test(std::function<void(float)>& update, std::function<void()>& render)
{
	ResourcePtr<Rendering::Device> device;
	ResourcePtr<ComponentManager> components;
	ResourcePtr<TransformSystem> transforms;
	ResourcePtr<SpriteSystem> sprites;
	ResourcePtr<SpriteManager> spriteManager;

	const int spriteCount = 1;
	
	for (int i = 0; i < spriteCount; i++)
	{
		auto entity = components->addEntity<TransformComponent, SpriteComponent>();
		SpriteComponent* sprite = entity.get<SpriteComponent>();
		sprite->m_sprite = spriteManager->getSprite("TestGif.gif");
		TransformComponent* transform = entity.get<TransformComponent>();
		transform->m_position.x = (i * 100.0f);
	}

	auto camera = components->addEntity<TransformComponent, CameraComponent>();
	Entity cameraEntity = camera.getEntity();
	camera.get<TransformComponent>()->m_position.z = -250.0f;
	//camera.get<CameraComponent>()->setControlType(CameraComponent::Orbit);
	camera.get<CameraComponent>()->setControlType(CameraComponent::WASD);
}

glm::vec4 SpriteSystem::m_clearColour(0.45f, 0.55f, 0.6f, 1.0f);

template<> Meta::Object Meta::instanceMeta<SpriteSystem>()
{
	return Object("SpriteSystem").
		func("addComponent", &SpriteSystem::addComponent, { "entity" });
}

template<> Meta::Object Meta::instanceMeta<SpriteComponent>()
{
	return Object("SpriteComponent").
		var("m_sprite", &SpriteComponent::m_sprite);
}