#include "stdafx.h"
#include "TextureGenerator.h"
#include "../Rendering/Unit.h"
#include "../Rendering/Texture.h"
#include "../Misc/ResizableMemoryPool.h"
#include "../Managers/EventManager.h"
#include "../Files/FileManager.h"
#include "../Scripts/ScriptManager.h"
#include "../imgui/ImGuiManager.h"

using namespace cimg_library;

struct TextureGenerator::Impl
{
	typedef FunctionBase<void, CImg<unsigned char>*> Operation;
	VariableSizedMemoryPool<Operation, Operation::PoolHelper> m_operations;
};

TextureGenerator* TextureGenerator::Instance = nullptr;

TextureGenerator::TextureGenerator():
p(new Impl)
{
	Instance = this;
	LOG_F(INFO, "TextureGenerator %x\n", this);
}

TextureGenerator::~TextureGenerator()
{
	LOG_F(INFO, "~TextureGenerator %x\n", this);
}

template<typename T> 
void TextureGenerator::push(T f)
{
	p->m_operations.push_back(makeFunction(f));
}

void TextureGenerator::clear(const glm::vec4& colour)
{
	LOG_F(INFO, "clear %x\n", this);
	push([=](CImg<unsigned char>* img)
	{
		unsigned char c[] = { (unsigned char)(colour.x * 255), (unsigned char)(colour.y * 255), (unsigned char)(colour.z * 255), 255 };
		img->draw_rectangle(0, 0, img->width(), img->height(), c, (unsigned char)(colour.w * 255));
	});
}

void TextureGenerator::rect(const glm::vec2& p1, const glm::vec2& p2, const glm::vec4& colour)
{
	push([=](CImg<unsigned char>* img)
	{
		int x1 = (int)(p1.x * img->width());
		int y1 = (int)(p1.y * img->height());
		int x2 = (int)(p2.x * img->width());
		int y2 = (int)(p2.y * img->height());
		unsigned char c[] = { (unsigned char)(colour.x * 255), (unsigned char)(colour.y * 255), (unsigned char)(colour.z * 255), (unsigned char)(colour.w * 255) };
		img->draw_rectangle(x1, y1, x2, y2, c, 255);
	});
}

void TextureGenerator::line(const glm::vec2& p1, const glm::vec2& p2, const glm::vec4& colour)
{
	push([=](CImg<unsigned char>* img)
	{
		int x1 = (int)(p1.x * img->width());
		int y1 = (int)(p1.y * img->height());
		int x2 = (int)(p2.x * img->width());
		int y2 = (int)(p2.y * img->height());
		unsigned char foreground[] = { (unsigned char)(colour.x * 255), (unsigned char)(colour.y * 255), (unsigned char)(colour.z * 255), 255 };
		img->draw_line(x1, y1, x2, y2, foreground, colour.w);
	});
}

void TextureGenerator::text(const char* text, const glm::vec2& position, int size, const glm::vec4& colour)
{
	LOG_F(INFO, "text %x\n", this);
	push([=](CImg<unsigned char>* img)
	{
		int x = (int)(position.x * img->width());
		int y = (int)(position.y * img->height());
		unsigned char foreground[] = { (unsigned char)(colour.x * 255), (unsigned char)(colour.y * 255), (unsigned char)(colour.z * 255), 255 };
		img->draw_text(x, y, text, foreground, 0, colour.w, size);
	});
}

ResourcePtr<Rendering::Texture> TextureGenerator::generate(unsigned int width, unsigned int height) const
{
	CImg<unsigned char> img(width, height, 1, 4, 255);
	for (auto& op : p->m_operations)
	{
		op(&img);
	}

	ResourcePtr<Rendering::Texture> texture(TakeOwnershipPtr, new Rendering::Texture);
	texture->setSoftware((int)width, (int)height, sizeof(glm::u8vec4));
	
	glm::u8vec4* dest = (glm::u8vec4*)texture->map();

	cimg_forXY(img, x, y)
	{
		*dest = glm::u8vec4(img(x, y, 0), img(x, y, 1), img(x, y, 2), 255);
		dest++;
	}

	texture->unmap();

	return texture;
}

void TextureGenerator::test()
{
	ResourcePtr<Rendering::Texture>* ptr = createTestResource< ResourcePtr<Rendering::Texture> >(EmptyPtr);
	auto generate = [ptr](bool remark)
	{
		ResourcePtr<ScriptManager> sm;
		if(remark)
			sm->remark("Scripts/Generators/TestGen.py");
		else
			sm->run("Scripts/Generators/TestGen.py");

		/*TextureGenerator gen;
		gen.clear({ 1.0f, 0.0f, 0.0f, 1.0f });
		gen.text("PROTOTYPE TEXTURE", { 0.0f, 0.0f }, 10);

		//center
		gen.line({ 0.5f, 0.0f }, { 0.5f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f });
		gen.line({ 0.0f, 0.5f }, { 1.0f, 0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f });

		// top right
		gen.line({ 0.0f, 0.25f }, { 1.0f, 0.25f }, { 1.0f, 1.0f, 1.0f, 0.25f });
		gen.line({ 0.75f, 0.0f }, { 0.75f, 1.0f }, { 1.0f, 1.0f, 1.0f, 0.25f });

		// bottom left
		gen.line({ 0.25f, 0.0f }, { 0.25f, 1.0f }, { 1.0f, 1.0f, 1.0f, 0.25f });
		gen.line({ 0.0f, 0.75f }, { 1.0f, 0.75f }, { 1.0f, 1.0f, 1.0f, 0.25f });*/

		ResourcePtr<Rendering::Texture> texture = TextureGenerator::Instance->generate(256, 256);

		ResourcePtr<VulkanFramework> vf;
		vf->uploadTexture(texture);

		*ptr = texture;
	};

	generate(false);

	ResourcePtr<ScriptManager> sm;
	sm->setEditorContent(nullptr, "../Res/Scripts/Generators/TestGen.py");

	ResourcePtr<EventManager> events;
	events->addListener<ScriptReloadedEvent>([=](ScriptReloadedEvent* e) {
		for (auto& change : e->m_paths)
		{
			if (change == "Scripts/Generators/TestGen.py")
				generate(false);
		}
	});

	events->addListener<ScriptRemarkEvent>([=](ScriptRemarkEvent* e) {
		for (auto& change : e->m_paths)
		{
			if (change == "Scripts/Generators/TestGen.py")
				generate(true);
		}
	});

	ResourcePtr<ImGuiManager> imgui;
	imgui->registerCallback({[](ResourcePtr<Rendering::Texture>* texture)
	{
		if (!texture)
			return;

		ImGui::Begin("TexGen");

		ImGui::Image((*texture), ImVec2(256, 256));

		ImGui::End();
	}, ptr });
}

template<>
Meta::Object Meta::instanceMeta<TextureGenerator>()
{
	return Meta::Object("TextureGenerator")
		.defaultFactory<TextureGenerator>()
		.func("clear", &TextureGenerator::clear)
		.func("text", &TextureGenerator::text)
		.func("line", &TextureGenerator::line);
}