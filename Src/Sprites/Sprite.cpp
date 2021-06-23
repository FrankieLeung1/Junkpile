#include "stdafx.h"
#include "Sprite.h"
#include "SpriteData.h"

#include "../Rendering/RenderingDevice.h"
#include "../Rendering/TextureAtlas.h"
#include "../imgui/ImGuiManager.h"
#include "../ECS/ComponentManager.h"
#include "../Framework/VulkanFramework.h"
#include "../Files/FileManager.h"
#include "../Misc/Misc.h"

void Sprite::test()
{
	ResourcePtr<VulkanFramework> vf;
	ResourcePtr<ImGuiManager> m;
	ResourcePtr<Rendering::Device> device;
	ResourcePtr<File> spriteFile(NewPtr, "TestGif.gif");
	g_resourceManager->startLoading();

	auto* data = createTestResource<std::tuple<SpriteData, Rendering::TextureAtlas>>();

	std::get<SpriteData>(*data).loadFromGif(std::move(spriteFile), *device);
	std::get<Rendering::TextureAtlas>(*data).addSprite(&std::get<SpriteData>(*data));

	std::get<Rendering::TextureAtlas>(*data).layoutAtlas();
	vf->uploadTexture(&std::get<Rendering::TextureAtlas>(*data));

	m->registerCallback({ [](void* ud) {
		auto* tuple = (std::tuple<SpriteData, Rendering::TextureAtlas>*)ud;
		auto& sprite = std::get<SpriteData>(*tuple);
		auto& t = std::get<Rendering::TextureAtlas>(*tuple);

		ImGui::Begin("Image");

		static bool atlas = false;
		if (atlas)
		{
			ImGui::Image(&t, ImVec2((float)t.getWidth(), (float)t.getHeight()), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 1));
		}
		else
		{
			static float time = (float)glfwGetTime();
			float currentTime = (float)glfwGetTime() - time;

			glm::vec2 uv1, uv2;
			const SpriteData::FrameData& frame = sprite.getFrame(currentTime);
			std::tie(uv1, uv2) = t.getUV(frame.m_id);

			glm::vec2 d = sprite.getDimensions();

			ImGui::Image(&t, ImVec2(d.x, d.y),
				ImVec2(uv1.x, uv1.y), ImVec2(uv2.x, uv2.y), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		}

		if (ImGui::IsItemClicked())
			atlas = !atlas;

		ImGui::Text("It's peanut butter jelly time!");
		ImGui::End();

	}, data });
}