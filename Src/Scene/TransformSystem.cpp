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
	return m_components->addComponents<TransformComponent>(e).get<TransformComponent>();
}