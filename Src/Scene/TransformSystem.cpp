#include "stdafx.h"
#include "TransformSystem.h"

TransformSystem::TransformSystem()
{
	m_components->addComponentType<TransformComponent>();
}

TransformSystem::~TransformSystem()
{

}

TransformComponent* TransformSystem::addComponent(Entity e)
{
	LOG_F(INFO, "TransformSystem::addComponent %d\n", e);
	auto t = m_components->addComponents<TransformComponent>(e).get<TransformComponent>();
	t->m_position = glm::vec3(0.0f);
	t->m_scale = glm::vec3(1.0f);
	t->m_rotation = glm::quat();
	return t;
}

template<> Meta::Object Meta::instanceMeta<TransformComponent>()
{
	return Object("TransformComponent").
		var("m_position", &TransformComponent::m_position).
		var("m_scale", &TransformComponent::m_scale).
		var("m_rotation", &TransformComponent::m_rotation);
}

template<> Meta::Object Meta::instanceMeta<TransformSystem>()
{
	return Object("TransformSystem").
		func("addComponent", &TransformSystem::addComponent, { "entity" });
}