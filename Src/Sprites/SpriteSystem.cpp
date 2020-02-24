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

	struct Vert { float m_x, m_y, m_z; };
	Rendering::Buffer* vbuffer = createTestResource<Rendering::Buffer>(Rendering::Buffer::Vertex, Rendering::Buffer::Mapped, sizeof(Vert) * 4);
	{
		Vert* map = (Vert*)vbuffer->map();
		float f = 1.0f;
		map[0] = Vert{ -f, f, 0.0f };
		map[1] = Vert{ f, f, 0.0f };
		map[2] = Vert{ -f, -f, 0.0f };
		map[3] = Vert{ f, -f, 0.0f };
		vbuffer->unmap();
	}

	Rendering::Buffer* ibuffer = createTestResource<Rendering::Buffer>(Rendering::Buffer::Index, Rendering::Buffer::Mapped, sizeof(short) * 4);
	{
		short* index = (short*)ibuffer->map();
		for (short i = 0; i < 4; i++)
			index[i] = i;

		ibuffer->unmap();
	}

	char vertexCode[] = R";(#version 450 core
layout(location = 0) in vec2 aPos;
out gl_PerVertex{ vec4 gl_Position; };
void main()
{
	gl_Position = vec4(aPos, 0, 1);
});";

	ResourcePtr<Rendering::Shader> vertShader(NewPtr, Rendering::Shader::Type::Vertex, vertexCode);
	LOG_IF_F(ERROR, !vertShader->compile(nullptr), "Vertex Failed\n");

	char pixelCode[] = R";(#version 450 core
layout(location = 0) out vec4 fColor;
void main()
{
    fColor = vec4(1, 0, 0, 1);
}
);";

	ResourcePtr<Rendering::Shader> fragShader(NewPtr, Rendering::Shader::Type::Pixel, pixelCode);
	LOG_IF_F(ERROR, !fragShader->compile(nullptr), "Pixel Failed\n");

	auto entity = components->addEntity<PositionComponent, SpriteComponent>();
	SpriteComponent* sprite = entity.get<SpriteComponent>();

	render = [vertShader, fragShader, ibuffer, vbuffer, texture]()
	{
		ResourcePtr<Rendering::Device> device;
		Rendering::Unit unit(device->getRootUnit());
		unit.in(vbuffer);
		unit.in(ibuffer);
		unit.in(vertShader);
		unit.in(fragShader);
		unit.in(Rendering::Unit::Binding<decltype(texture)>{0, texture});
		unit.submit();
	};
}