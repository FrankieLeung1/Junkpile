#include "stdafx.h"
#include "TicTacToe.h"
#include "../imgui/ImGuiManager.h"
#include "../ECS/ComponentManager.h"
#include "../Sprites/SpriteSystem.h"
#include "../Generators/TextureGenerator.h"
#include "../Rendering/Texture.h"
#include "../Scene/TransformSystem.h"
#include "../Scene/CameraSystem.h"
#include "../Scene/SelectableSystem.h"
#include "../Managers/InputManager.h"

TicTacToeSystem::TicTacToeSystem()
{
	ResourcePtr<EventManager> events;
	events->addListener<UpdateEvent>([this](UpdateEvent* e) { update(); });

	ResourcePtr<ImGuiManager> imgui;
	imgui->registerCallback({ [](TicTacToeSystem* t) { t->imgui(); }, this });

	ResourcePtr<ComponentManager> components;
	components->addComponentType<TicTacToeComponent>(1);
	m_component = components->addEntity<TicTacToeComponent>().getEntity();

	ResourcePtr<CameraSystem> cameras;
	auto camera = components->addEntity<TransformComponent>();
	//cameras->addComponentPerspective(camera.getEntity(), 90.0f);
	cameras->addComponentOrthographic(camera.getEntity());
	cameras->setCameraActive(camera.getEntity());
	camera.get<TransformComponent>()->m_position.z = -250.0f;
}

TicTacToeSystem::~TicTacToeSystem()
{

}

void TicTacToeSystem::update()
{
	ResourcePtr<ComponentManager> components;
	ResourcePtr<SpriteSystem> sprites;
	TicTacToeComponent* t = components->findEntity<TicTacToeComponent>(m_component).get<TicTacToeComponent>();
	switch (t->m_state)
	{
	case TicTacToeComponent::State::Init:
		{
			t->m_state = TicTacToeComponent::State::Turn;
			ResourcePtr<Rendering::Texture> texture = createTexture();
			std::vector<glm::vec2> positions = squarePositions({ 0.0f, 0.0f }, countof(t->m_pieces), { 50.0f, 50.0f });
			for(std::size_t i = 0; i < positions.size(); i++)
			{
				const glm::vec2& p = positions[i];
				auto it = components->addEntity<TransformComponent, SelectableComponent>();
				sprites->addComponent(it.getEntity(), texture);
				//it.get<SelectableComponent>()->m_radius = 25.0f;
				it.get<SelectableComponent>()->m_radius = 25.0f;
				it.get<TransformComponent>()->m_position = glm::vec3(p.x, p.y, 0.0f);
				
				t->m_pieces[i] = it.getEntity();
			}
		}
		break;

	case TicTacToeComponent::State::Reset:
		break;

	case TicTacToeComponent::State::Turn:
		{
			ResourcePtr<InputManager> inputs;
			if (inputs->isMouseDown(0) && !t->m_dragPiece)
			{
				ResourcePtr<SelectableSystem> selectables;
				glm::vec2 cursor = inputs->getCursorPosNDC();
				std::vector<SelectableSystem::Selected> entities = selectables->castFromCamera({ cursor.x, cursor.y, 0.0f });
				if (!entities.empty())
					t->m_dragPiece = entities.front().m_entity;
			}
			else if (t->m_dragPiece && !inputs->isMouseDown(0))
			{
				t->m_dragPiece = Entity();
			}
			else if (t->m_dragPiece)
			{
				TransformComponent* pieceTransform = components->findEntity<TransformComponent>(t->m_dragPiece).get<TransformComponent>();
				pieceTransform->m_position = glm::vec3(inputs->getCursorPosNormalizedInPixels(), 0.0f);
			}
		}
		break;

	case TicTacToeComponent::State::EndGame:
		break;
	}
}

void TicTacToeSystem::imgui()
{
	ImGui::Begin("TicTacToe");

	ResourcePtr<ComponentManager> components;
	TicTacToeComponent* t = components->findEntity<TicTacToeComponent>(m_component).get<TicTacToeComponent>();
	if (t->m_dragPiece)
		ImGui::Text("Entity %d", ComponentManager::debugId(t->m_dragPiece) - 2);
	else
		ImGui::Text("No Entity");

	ImGui::End();
}

ResourcePtr<Rendering::Texture> TicTacToeSystem::createTexture() const
{
	glm::vec4 ringColour{ 1.0f, 0.0f, 0.0f, 1.0f };
	glm::vec4 borderColour{ 1.0f, 1.0f, 1.0f, 1.0f };

	const float minRadius = 0.0f;
	const float maxRadius = 128.0f;
	const float radiusRange = maxRadius - minRadius;
	const int maxRings = 6;
	const int ringThickness = 10;
	const float borderThickness = 3.0f;

	int i = 1;
	TextureGenerator gen;
	gen.clear({ 0.0f, 0.0f, 0.0f, 0.0f });

	float radius = (((float)i / (float)maxRings) * maxRadius) + minRadius;
	gen.circle({ 128.0f, 128.0f }, (int)radius, borderColour);
	gen.circle({ 128.0f, 128.0f }, (int)(radius - borderThickness), ringColour);
	gen.circle({ 128.0f, 128.0f }, (int)(radius - ringThickness - borderThickness), borderColour);
	gen.circle({ 128.0f, 128.0f }, (int)(radius - ringThickness - (borderThickness * 2.0f)), { 0.0f, 0.0f, 0.0f, 0.0f });

	return ResourcePtr<Rendering::Texture> (TakeOwnershipPtr, TextureGenerator::Instance->generate(256, 256));
}

TicTacToeComponent::TicTacToeComponent():
m_state(State::Init),
m_currentTurn(Player::Player1)
{
	memset(m_pieces, 0x0, sizeof(m_pieces));
}