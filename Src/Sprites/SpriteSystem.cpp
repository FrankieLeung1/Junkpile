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

SpriteSystem::SpriteSystem()
{

}

SpriteSystem::~SpriteSystem()
{

}

void SpriteSystem::process(float)
{

}

void SpriteSystem::render()
{

}

void SpriteSystem::imgui()
{
	ResourcePtr<ComponentManager> components;
	EntityIterator<TransformComponent, CameraComponent> it(components, true);
	ResourcePtr<CameraSystem> cameras;
	ImGui::Begin("SpriteSystem");
	while (it.next())
	{
		auto transform = it.get<TransformComponent>();
		auto camera = it.get<CameraComponent>();

		//ImGui::DragFloat4("rot", &it.get<TransformComponent>()->m_rotation.x);

		/*glm::mat4x4 m = cameras->getMatrix(it.getEntity());
		ImGui::DragFloat3("m1", &m[0].x);
		ImGui::DragFloat3("m2", &m[1].x);
		ImGui::DragFloat3("m3", &m[2].x);
		ImGui::DragFloat3("m4", &m[3].x);*/

		float distance = (float)glm::length(transform->m_position);
		ImGui::DragFloat3("angles", &camera->m_angles.x);
		ImGui::DragFloat3("position", &transform->m_position.x);
		ImGui::DragFloat("distance", &distance);

		/*glm::vec3 dir = glm::normalize(it.get<TransformComponent>()->m_position - it.get<CameraComponent>()->m_lookAt);
		ImGui::DragFloat3("cam", &it.get<TransformComponent>()->m_position.x);
		ImGui::DragFloat3("look", &it.get<CameraComponent>()->m_lookAt.x);
		ImGui::DragFloat3("up", &it.get<CameraComponent>()->m_up.x);
		ImGui::DragFloat3("dir", &dir.x);
		ImGui::DragFloat("r", &it.get<CameraComponent>()->m_r);*/
	}
	ImGui::End();
}

void SpriteSystem::test(std::function<void(float)>& update, std::function<void()>& render)
{
	ResourcePtr<Rendering::Device> device;
	ResourcePtr<ComponentManager> components;
	ResourcePtr<TransformSystem> transforms;
	ResourcePtr<SpriteSystem> sprites;
	ResourcePtr<SpriteManager> spriteManager;

	const int spriteCount = 1;
	struct Vert { glm::vec3 m_position; glm::vec2 m_uv; };
	Rendering::Buffer* vbuffer = createTestResource<Rendering::Buffer>(Rendering::Buffer::Vertex, Rendering::Buffer::Mapped, sizeof(Vert) * 4 * spriteCount);
	vbuffer->setFormat({
			{vk::Format::eR32G32B32Sfloat, sizeof(glm::vec3)},
			{vk::Format::eR32G32Sfloat, sizeof(glm::vec2)},
		}, sizeof(Vert));

	Rendering::Buffer* ibuffer = createTestResource<Rendering::Buffer>(Rendering::Buffer::Index, Rendering::Buffer::Mapped, sizeof(short) * 4);
	{
		short* index = (short*)ibuffer->map();
		for (short i = 0; i < 4; i++)
			index[i] = i;

		ibuffer->unmap();
	}
	ibuffer->setFormat({ {vk::Format::eR16Sint, sizeof(short)} }, sizeof(short));

	char vertexCode[] =
		"#version 450 core\n"
		"layout(location = 0) in vec2 aPos;\n"
		"layout(location = 1) in vec2 aUV;\n"
		"layout(push_constant) uniform PushConsts{ mat4 vp; } pushConsts;\n"
		"out gl_PerVertex{ vec4 gl_Position; };\n"
		"layout(location = 0) out vec2 UV;\n"
		"void main()\n"
		"{\n"
		"	gl_Position = pushConsts.vp * vec4(aPos.xy, 0.0, 1.0);\n"
		"	UV = aUV;\n"
		"}";

	ResourcePtr<Rendering::Shader> vertShader(NewPtr, Rendering::Shader::Type::Vertex, vertexCode);

	char pixelCode[] =
		"#version 450 core\n"
		"layout(location = 0) in vec2 UV;\n"
		"layout(location = 0) out vec4 fColor; \n"
		"layout(binding = 0) uniform sampler2D sTexture;\n"
		"void main()\n"
		"{\n"
		"	fColor = texture(sTexture, UV.st);\n"
		"}";

	ResourcePtr<Rendering::Shader> fragShader(NewPtr, Rendering::Shader::Type::Pixel, pixelCode);
	
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
	//camera.get<CameraComponent>()->m_controlType = CameraComponent::Orbit;
	camera.get<CameraComponent>()->m_controlType = CameraComponent::WASD;

	update = [vbuffer, spriteManager](float delta)
	{
		ResourcePtr<ComponentManager> components;
		EntityIterator<TransformComponent, SpriteComponent> it(components, true);
		Vert* map = (Vert*)vbuffer->map();
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
			float scale = 1.0f;
			float halfWidth = (frame.m_texture->getWidth() / 2.0f) * scale;
			float halfHeight = (frame.m_texture->getHeight() / 2.0f) * scale;

			map[0] = Vert{ transform->m_position + glm::vec3{ halfWidth, -halfHeight, 0.0f }, { uv2.x, uv1.y } };
			map[1] = Vert{ transform->m_position + glm::vec3{ -halfWidth, -halfHeight, 0.0f }, { uv1.x, uv1.y } };
			map[2] = Vert{ transform->m_position + glm::vec3{ halfWidth, halfHeight, 0.0f }, { uv2.x, uv2.y } };
			map[3] = Vert{ transform->m_position + glm::vec3{ -halfWidth, halfHeight, 0.0f }, { uv1.x, uv2.y } } ;
			map += 4;
		}
		
		vbuffer->unmap();
	};

	auto& clearColour = m_clearColour;
	render = [vertShader, fragShader, ibuffer, vbuffer, spriteManager, &clearColour, cameraEntity]()
	{
		ResourcePtr<ComponentManager> components;
		EntityIterator<SpriteComponent> it(components, true);
		int i = 0;

		ResourcePtr<Rendering::Device> device;
		ResourcePtr<CameraSystem> cameras;
		glm::mat4x4 cameraMatrix = cameras->getMatrix(cameraEntity);
		std::vector<char> pushData(sizeof(glm::mat4));
		memcpy(&pushData[0], &cameraMatrix, sizeof(glm::mat4));

		Rendering::Unit unit(device->getRootUnit());
		unit.in(vbuffer);
		unit.in(ibuffer);
		unit.in(vertShader);
		unit.in(fragShader);
		unit.in(std::array<float, 4>{ clearColour.x, clearColour.y, clearColour.z, clearColour.w, });
		
		unit.in({ vk::ShaderStageFlagBits::eVertex, std::move(pushData) });

		while (it.next())
		{
			auto sprite = it.get<SpriteComponent>();
			std::tuple<SpriteData*, Rendering::TextureAtlas*> spriteData = spriteManager->getSpriteData(sprite->m_sprite);
			ResourcePtr<Rendering::Texture> texture(NoOwnershipPtr, std::get<1>(spriteData));
			if (!std::get<1>(spriteData))
				continue; // still loading

			if(i == 0)
				unit.in({ vk::ShaderStageFlagBits::eFragment, 0, texture });

			unit.in({ 4, 1, (uint32_t)i++ * 4, 0 });
		}

		if(i > 0)
			unit.submit();
	};
}

glm::vec4 SpriteSystem::m_clearColour(0.45f, 0.55f, 0.6f, 1.0f);