#pragma once
#include "stdafx.h"
#include "../imgui/ImGuiManager.h"
#include "../ECS/ComponentManager.h"
#include "../Framework/VulkanFramework.h"
#include "../Scene/TransformSystem.h"

void physicsTest()
{
	ResourcePtr<PhysicsSystem> ps;
	ResourcePtr<ComponentManager> cm;
	ResourcePtr<EventManager> em;
	for (int i = 0; i < 10; i++)
	{
		auto it = cm->addEntity<TransformComponent>();
		it.get<TransformComponent>()->m_position.x = (std::rand() / (float)RAND_MAX) * 1300.0f;
		it.get<TransformComponent>()->m_position.y = ((std::rand() / (float)RAND_MAX) * 50.0f) + 200.0f;
		ps->createBox(it.getEntity(), glm::vec3(1.0f), 1);

		em->addListener<CollisionEvent>([=](EventBase* e) { LOG_F(INFO, "COLLISION %d\n", i);  e->discardListener(); });
	}

	{ // init ground
		auto it = cm->addEntity<TransformComponent>();
		it.get<TransformComponent>()->m_position.x = 0.0f;
		it.get<TransformComponent>()->m_position.y = 700.0f;
		ps->createBox(it.getEntity(), glm::vec3(2000.0f, 10.0f, 2000.0f), 0);
	}
}