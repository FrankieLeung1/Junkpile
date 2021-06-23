#include "stdafx.h"
#include "SpriteEditor.h"
#include "../Files/File.h"
#include "../Rendering/TextureAtlas.h"
#include "../Rendering/RenderingDevice.h"

SpriteEditor::SpriteEditor():
m_currentFrame(0),
m_comboSelection(2)
{
	ResourcePtr<VulkanFramework> vf;
	ResourcePtr<Rendering::Device> device;
	ResourcePtr<File> spriteFile(NewPtr, "ryu.gif");
	g_resourceManager->startLoading();

	m_sprite.loadFromGif(std::move(spriteFile), *device);

	m_atlas = std::make_shared<Rendering::TextureAtlas>();
	m_atlas->addSprite(&m_sprite);

	m_atlas->layoutAtlas();
	vf->uploadTexture(&(*m_atlas));

	Hitbox hitbox;
	hitbox.m_name = "Punch";
	hitbox.m_min = glm::vec3(90.0f, 50.0f, 0.0f);
	hitbox.m_max = glm::vec3(110.0f, 70.0f, 0.0f);
	hitbox.m_colour = 0xFFE600FF;

	FrameData hitFrame;
	hitFrame.m_index = 5;
	hitFrame.m_hitboxes.push_back(hitbox);
	m_frameData.push_back(hitFrame);

	hitFrame.m_index = 6;
	m_frameData.push_back(hitFrame);

	hitFrame.m_index = 7;
	hitbox.m_min = glm::vec3(80.0f, 50.0f, 0.0f);
	hitbox.m_max = glm::vec3(100.0f, 70.0f, 0.0f);
	hitFrame.m_hitboxes[0] = hitbox;
	m_frameData.push_back(hitFrame);
}

SpriteEditor::~SpriteEditor()
{

}

void SpriteEditor::imgui()
{
	ImGui::Begin("Sprite Editor");

	ImGui::Columns(2);
	FrameData* frameData = nullptr;
	for (FrameData& currentFrame : m_frameData)
	{
		if (currentFrame.m_index == m_currentFrame)
		{
			frameData = &currentFrame;
			break;
		}
	}

	if (m_atlas)
	{
		Rendering::TextureAtlas& t = *m_atlas;

		glm::vec2 uv1, uv2;
		const SpriteData::FrameData& frame = m_sprite.m_frames[m_currentFrame];
		std::tie(uv1, uv2) = t.getUV(frame.m_id);

		glm::vec2 d = m_sprite.getDimensions();
		ImGui::Image(&t, ImVec2(d.x, d.y), ImVec2(uv1.x, uv1.y), ImVec2(uv2.x, uv2.y), ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 1));

		if (frameData)
		{
			for (const Hitbox& hitbox : frameData->m_hitboxes)
			{
				ImVec2 pos = ImGui::GetWindowPos();
				ImVec2 min(hitbox.m_min.x + pos.x, hitbox.m_min.y + pos.y), max(hitbox.m_max.x + pos.x, hitbox.m_max.y + pos.y);
				ImDrawList* drawList = ImGui::GetForegroundDrawList();
				drawList->AddRect(min, max, hitbox.m_colour);
			}
		}
	}

	if (ImGui::SmallButton("<"))
	{
		if (m_currentFrame == 0)
			m_currentFrame = m_sprite.m_frames.size() - 1;
		else
			m_currentFrame -= 1;
	}
	ImGui::SameLine();

	ImGui::Text("%d / %d", m_currentFrame + 1, m_sprite.m_frames.size()); ImGui::SameLine();

	if (ImGui::SmallButton(">"))
	{
		if (m_currentFrame == m_sprite.m_frames.size() - 1)
		{
			m_currentFrame = 0;
		}
		else
		{
			m_currentFrame += 1;
		}
	}
	ImGui::SameLine();

	ImGui::NextColumn();

	if (frameData)
	{
		Hitbox& hitbox = frameData->m_hitboxes[0];
		ImGui::InputText("Name", &hitbox.m_name);

		const char* values = "Light\0Medium\0Heavy\0";
		ImGui::Combo("Power", &m_comboSelection, values);
	}
	else
	{
		ImGui::Text("No Data");
	}

	ImGui::Columns(1);

	ImGui::End();
}