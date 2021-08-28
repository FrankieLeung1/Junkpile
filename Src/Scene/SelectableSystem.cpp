#include "stdafx.h"
#include "SelectableSystem.h"
#include "glm/glm/gtx/intersect.hpp"
#include "../Sprites/SpriteSystem.h"
#include "../ECS/ComponentManager.h"
#include "../Scene/CameraSystem.h"
#include "../Scene/TransformSystem.h"

SelectableSystem::SelectableSystem()
{
	ResourcePtr<ComponentManager> components;
	components->addComponentType<SelectableComponent>(10);
}

SelectableSystem::~SelectableSystem()
{

}

std::vector<SelectableSystem::Selected> SelectableSystem::castFromCamera(const glm::vec3& coord, glm::u64 flags, Entity camera) const
{
	ResourcePtr<CameraSystem> cameras;	
	std::tuple<glm::vec3, glm::vec3> ray = cameras->screenToWorld(coord, camera);
	return std::move(castRay(std::get<0>(ray), std::get<1>(ray), flags));
}

std::vector<SelectableSystem::Selected> SelectableSystem::castRay(const glm::vec3& origin, const glm::vec3& ray, glm::u64 flags) const
{
	EntityIterator<SelectableComponent, SpriteComponent, TransformComponent> it(false);
	std::vector<SelectableSystem::Selected> result;
	while (it.next())
	{
		SelectableComponent* selectable = it.get<SelectableComponent>();
		TransformComponent* transform = it.get<TransformComponent>();
		if (transform && selectable && (selectable->m_flags & flags) != 0)
		{
			const glm::vec3 inf{ std::numeric_limits<float>::infinity() };
			if (selectable->m_max != inf && selectable->m_min != inf) // AABB
			{
				LOG_F(FATAL, "TODO");
			}
			else if (selectable->m_radius != std::numeric_limits<float>::infinity()) // sphere
			{
				float distance = 0.0f;
				if (glm::intersectRaySphere(origin, ray, transform->m_position, selectable->m_radius * selectable->m_radius, distance))
					result.push_back({ it.getEntity() });
			}
			else if (SpriteComponent* sprite = it.get<SpriteComponent>()) // sprite quad
			{
				std::array<SpriteSystem::Vertex, 4> vertices;
				ResourcePtr<SpriteSystem> sprites;
				if (sprites->getVertices(&vertices, sprite, transform))
				{
					glm::vec3 position = { 0.0f, 0.0f, 0.0f };
					if (glm::intersectLineTriangle(origin, ray, vertices[0].m_position, vertices[1].m_position, vertices[2].m_position, position) ||
						glm::intersectLineTriangle(origin, ray, vertices[1].m_position, vertices[2].m_position, vertices[3].m_position, position))
					{
						result.push_back({ it.getEntity() });
					}
				}

			}
			
		}
	}

	return std::move(result);
}