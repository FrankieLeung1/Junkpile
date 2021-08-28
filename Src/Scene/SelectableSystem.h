#pragma once

#include "../Resources/ResourceManager.h"
#include "../ECS/ComponentManager.h"
class SelectableSystem : public SingletonResource<SelectableSystem>
{
public:
	struct Selected
	{
		Entity m_entity;
		//glm::vec2 m_position;
	};

public:
	SelectableSystem();
	~SelectableSystem();

	std::vector<Selected> castFromCamera(const glm::vec3&, glm::u64 flags = 0xFFFFFFFF, Entity camera = Entity()) const;
	std::vector<Selected> castRay(const glm::vec3& origin, const glm::vec3& ray, glm::u64 flags = 0xFFFFFFFF) const;

protected:

};

struct SelectableComponent : public Component<SelectableComponent, SelectableSystem>
{
	glm::u64 m_flags{ 0xFFFFFFFF };
	glm::vec3 m_min{ std::numeric_limits<float>::infinity() }, m_max{ std::numeric_limits<float>::infinity() };
	float m_radius{ std::numeric_limits<float>::infinity() };
};