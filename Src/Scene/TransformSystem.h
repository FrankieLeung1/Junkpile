#pragma once

#include "../Resources/ResourceManager.h"
#include "../ECS/ComponentManager.h"

class TransformSystem;
struct TransformComponent : public Component<TransformComponent, TransformSystem>
{
	static constexpr const char* m_cid = "Transform";

	glm::vec3 m_position;
	glm::vec3 m_scale;
	glm::quat m_rotation;
	//TODO: parent
};

class TransformSystem : public SingletonResource<TransformSystem>
{
public:
	TransformSystem();
	~TransformSystem();

	TransformComponent* addComponent(Entity);

protected:
	ResourcePtr<ComponentManager> m_components;
};

namespace Meta
{
	template<> Object instanceMeta<TransformComponent>();
	template<> Object instanceMeta<TransformSystem>();
}