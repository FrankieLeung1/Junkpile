#include "stdafx.h"
#include "ComponentManager.h"
#include "../imgui/ImGuiManager.h"

ComponentManager::ComponentManager():
m_nextFreeEntityId(1)
{

}

ComponentManager::~ComponentManager()
{

}

void ComponentManager::removeEntity(Entity)
{
	LOG_F(FATAL, "TODO");
}

bool ComponentManager::validPointer(const ResizeableMemoryPool& pool, void* p) const
{
	const char* begin = &pool[0];
	return begin <= p && begin + pool.size() > p;
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