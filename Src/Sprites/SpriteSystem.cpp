#include "stdafx.h"
#include "SpriteSystem.h"
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
	return;

	ImGui::Begin("SpriteSystem");
		ImGui::ColorEdit4("MyColor##1", (float*)&m_clearColour);
	ImGui::End();
}

void SpriteSystem::test(std::function<void(float)>& update, std::function<void()>& render)
{
	ResourcePtr<Rendering::Device> device;
	ResourcePtr<ComponentManager> components;
	ResourcePtr<SpriteSystem> sprites;
	ResourcePtr<File> spriteFile(NewPtr, "TestGif.gif");

	SpriteData data;
	ResourcePtr<Rendering::TextureAtlas> atlas = g_resourceManager->addLoadedResource(new Rendering::TextureAtlas, "Texture Atlas");
	data.loadFromGif(std::move(spriteFile), *device);
	atlas->addSprite(&data);

	atlas->layoutAtlas();
	//vf->uploadTexture(&std::get<Rendering::TextureAtlas>(data));

	ResourcePtr<Rendering::Texture> texture(ResourcePtr<Rendering::Texture>::NoOwnershipPtr{}, createTestResource<Rendering::Texture>());
	Rendering::Unit upload = device->createUnit();
	upload.in(atlas);
	upload.out(*texture);
	upload.submit();

	struct Vert { glm::vec3 m_position; glm::vec2 m_uv; };
	Rendering::Buffer* vbuffer = createTestResource<Rendering::Buffer>(Rendering::Buffer::Vertex, Rendering::Buffer::Mapped, sizeof(Vert) * 4);
	/*{
		Vert* map = (Vert*)vbuffer->map();
		float f = 500.0f;
		map[0] = Vert{ { -f, f, 0.0f }, { 0.0f, 1.0f } };
		map[1] = Vert{ { f, f, 0.0f }, { 1.0f, 1.0f } };
		map[2] = Vert{ { -f, -f, 0.0f }, { 0.0f, 0.0f } };
		map[3] = Vert{ { f, -f, 0.0f }, { 1.0f, 0.0f } };
		vbuffer->unmap();
	}*/
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
		//"	gl_Position = vec4(aPos, 0, 1);\n"
		"	gl_Position = pushConsts.vp * vec4(aPos.xy, 0.0, 1.0);\n"
		"	UV = aUV;\n"
		"}";

	ResourcePtr<Rendering::Shader> vertShader(NewPtr, Rendering::Shader::Type::Vertex, vertexCode);
	//vertShader->addBindings({ {Rendering::Binding::Type::Constant, 0, vk::Format::eR32G32Sfloat, 0 } });

	char pixelCode[] =
		"#version 450 core\n"
		"layout(location = 0) in vec2 UV;\n"
		"layout(location = 0) out vec4 fColor; \n"
		"layout(binding = 0) uniform sampler2D sTexture;\n"
		"void main()\n"
		"{\n"
		"	fColor = texture(sTexture, UV.st);\n"
		//"	fColor = vec4(UV, 0.0, 1.0);\n"
		"}";

	ResourcePtr<Rendering::Shader> fragShader(NewPtr, Rendering::Shader::Type::Pixel, pixelCode);
	//fragShader->addBindings({ {Rendering::Binding::Type::Constant, 0, vk::Format::eR8G8B8Unorm, 0} });

	auto entity = components->addEntity<PositionComponent, SpriteComponent>();
	SpriteComponent* sprite = entity.get<SpriteComponent>();

	//auto resolution = std::get<1>(device->getFrameBuffer());
	float halfWidth = 1280.0f / 2.0f, halfHeight = 720.0f / 2.0f;
	glm::mat4x4 ortho = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight);

	update = [vbuffer, data, atlas](float delta)
	{
		ResourcePtr<ComponentManager> components;
		EntityIterator<SpriteComponent> it(components, true);
		Vert* map = (Vert*)vbuffer->map();
		while (it.next())
		{
			auto sprite = it.get<SpriteComponent>();
			sprite->m_time += delta;

			glm::vec2 uv1, uv2;
			const SpriteData::FrameData& frame = data.getFrame(sprite->m_time);
			std::tie(uv1, uv2) = atlas->getUV(frame.m_id);

			float scale = 1.0f;
			float halfWidth = (frame.m_texture->getWidth() / 2.0f) * scale;
			float halfHeight = (frame.m_texture->getHeight() / 2.0f) * scale;

			map[0] = Vert{ { -halfWidth, halfHeight, 0.0f }, { uv1.x, uv2.y } };
			map[1] = Vert{ { halfWidth, halfHeight, 0.0f }, { uv2.x, uv2.y } };
			map[2] = Vert{ { -halfWidth, -halfHeight, 0.0f }, { uv1.x, uv1.y } };
			map[3] = Vert{ { halfWidth, -halfHeight, 0.0f }, { uv2.x, uv1.y } };
		}
		
		vbuffer->unmap();		
	};

	auto& clearColour = m_clearColour;
	render = [vertShader, fragShader, ibuffer, vbuffer, texture, &clearColour, ortho]()
	{
		ResourcePtr<Rendering::Device> device;
		Rendering::Unit unit(device->getRootUnit());
		unit.in(vbuffer);
		unit.in(ibuffer);
		unit.in(vertShader);
		unit.in(fragShader);
		unit.in(std::array<float, 4>{ clearColour.x, clearColour.y, clearColour.z, clearColour.w, });
		unit.in({vk::ShaderStageFlagBits::eFragment, 0, texture});
		unit.in({ vk::ShaderStageFlagBits::eVertex, Rendering::Unit::PushConstant, ortho });
		
		unit.submit();
	};
}

glm::vec4 SpriteSystem::m_clearColour(0.45f, 0.55f, 0.6f, 1.0f);