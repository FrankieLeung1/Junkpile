#include "stdafx.h"
#include "ComponentManager.h"
#include "../imgui/ImGuiManager.h"

ComponentManager::ComponentManager():
m_nextFreeEntityId()
{
	m_nextFreeEntityId.m_value = 1u;
	ResourcePtr<EventManager> events;
	events->addListener<ScriptUnloadedEvent>([this](ScriptUnloadedEvent* e) { onScriptUnloaded(e); });
	events->addListener<ScriptLoadedEvent>([this](ScriptLoadedEvent* e) { onScriptLoaded(e); });
}

ComponentManager::~ComponentManager()
{

}

Entity ComponentManager::newEntity()
{
	ResourcePtr<ScriptManager> scripts;
	
	auto allocateNewEntity = [=]() {Entity e = m_nextFreeEntityId; m_nextFreeEntityId.m_value++; m_entityCount++; return e; };
	ScriptManager::Environment::Script script = scripts->getRunningScript();
	if (script)
	{
		// this might be too complicated but if a script deletes an entity, we can reuse their id instead of incrementing m_nextFreeEntityId
		ScriptData& data = m_scriptData[script];
		if (data.m_nextEntityIndex == data.m_entities.size())
		{
			Entity e = allocateNewEntity();
			for (std::size_t i = 0; i < scripts->getCallstackSize(); i++)
			{
				ScriptManager::Environment::Script s = scripts->getCallstack(i);
				ScriptData& stackScriptData = m_scriptData[s];
				stackScriptData.m_entities.push_back(e);
				stackScriptData.m_nextEntityIndex++;
			}
			return e;
		}
		else
		{
			return data.m_entities[data.m_nextEntityIndex++];
		}
	}
	else
	{
		return allocateNewEntity();
	}
}

void ComponentManager::removeEntity(Entity entity)
{
	for(auto it = m_pools.begin(); it != m_pools.end(); ++it)
	{
		ResizeableMemoryPool& buffer = it->second.m_buffer;
		it->second.m_accessor->clearEntity(buffer, entity);
	}

	m_entityCount--;
}

int ComponentManager::debugId(Entity entity) const
{
	return (int)entity.m_value;
}

void ComponentManager::printAllEntityIds() const
{
	for (auto it = m_pools.begin(); it != m_pools.end(); ++it)
	{
		it->second.m_accessor->printEntityIds(it->second.m_buffer);
	}
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

void ComponentManager::onScriptUnloaded(ScriptUnloadedEvent* e)
{
	ResourcePtr<ScriptManager> scripts;
	for (auto& it : m_scriptData)
	{
		StringView path = scripts->getScriptPath(it.first);
		for (auto unloadedPath : e->m_paths)
		{
			if (unloadedPath == path)
			{
				it.second.m_nextEntityIndex--;
				for (Entity& entity : it.second.m_entities)
					removeEntity(entity);

				if (!e->m_reloading)
					it.second.m_entities.pop_back();

				CHECK_F(it.second.m_nextEntityIndex >= 0);
			}
		}
	}
}

void ComponentManager::onScriptLoaded(ScriptLoadedEvent* e)
{

}