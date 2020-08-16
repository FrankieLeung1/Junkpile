#include "stdafx.h"
#include "ComponentManager.h"
#include "../imgui/ImGuiManager.h"

ComponentManager::ComponentManager():
m_nextFreeEntityId()
{
	m_nextFreeEntityId.m_value = 1u;
}

ComponentManager::~ComponentManager()
{

}

Entity ComponentManager::newEntity()
{
	Entity result = m_nextFreeEntityId;
	m_nextFreeEntityId.m_value++;
	return result;
}

void ComponentManager::removeEntity(Entity)
{
	LOG_F(FATAL, "TODO");
}

bool ComponentManager::validPointer(const ComponentPool& pool, void* p) const
{
	const char* begin = (const char*)pool.m_accessor->front(pool.m_buffer);
	return begin <= p && begin + (pool.m_accessor->size(pool.m_buffer) * pool.m_accessor->elementSize()) > p;
}

void ComponentManager::imgui()
{
	ResourcePtr<ImGuiManager> im;
	bool* opened = im->win("Entities");
	if (*opened == false)
		return;

	if (ImGui::Begin("Entities", opened))
	{
		ImGui::Text("%d entities", m_entityCount);
	}
	ImGui::End();
}

void ComponentManager::clearAllComponents()
{
	for (auto& it : m_pools)
	{
		it.second.m_accessor->clear(it.second.m_buffer);
	}
}

template<> Meta::Object Meta::instanceMeta<ComponentManager>()
{
	return Object("ComponentManager").
		func("newEntity", &ComponentManager::newEntity);
}

template<> Meta::Object Meta::instanceMeta<Entity>()
{
	return Object("Entity");
}