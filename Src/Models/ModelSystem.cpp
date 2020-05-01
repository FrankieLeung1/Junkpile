#include "stdafx.h"
#include "ModelSystem.h"
#include "../Managers/EventManager.h"
#include "../ECS/ComponentManager.h"
#include "../Scene/TransformSystem.h"
#include "../Rendering/Unit.h"
#include "../Rendering/Depot.h"
#include "../Rendering/Buffer.h"

ModelSystem::ModelSystem()
{
	ResourcePtr<EventManager> events;
	events->addListener<UpdateEvent>([=](UpdateEvent* e) { this->update(e->m_delta); });
}

ModelSystem::~ModelSystem()
{

}

void ModelSystem::update(float delta)
{
	ResourcePtr<ModelManager> models;
	ResourcePtr<Rendering::Depot> depot;
	ResourcePtr<ComponentManager> components;
	EntityIterator<TransformComponent, ModelComponent> it(components, true);
	ResourcePtr<Rendering::Shader> vshader = depot->getPassthroughVertexShader();
	ResourcePtr<Rendering::Shader> fshader = depot->getPassthroughFragmentShader();
	if (!vshader || !fshader)
		return;

	while (it.next())
	{
		const ModelManager::ModelData* data = models->getModelData(it.get<ModelComponent>()->m_model);
		if (!data->m_vBuffer || !data->m_iBuffer)
			continue;

		Rendering::Unit unit;
		unit.in(vshader);
		unit.in(fshader);
		unit.in(data->m_vBuffer);
		unit.in(data->m_iBuffer);
		unit.in(vk::PrimitiveTopology::eTriangleList);

		//  m_indexCount, m_instanceCount, m_firstIndex, m_vertexOffset, m_firstInstance
		unit.in({ (uint32_t)data->m_indexCount, 1, 0, 0, 0 });
		unit.submit();
	}
}

void ModelSystem::test()
{
	ResourcePtr<ModelManager> models;
	ResourcePtr<ModelSystem> modelSystem;
	ResourcePtr<ComponentManager> components;
	auto entityIt = components->addEntity<TransformComponent, ModelComponent>();
	entityIt.get<ModelComponent>()->m_model = models->getModel("Models/Model/characterMedium.fbx");
	//entityIt.get<ModelComponent>()->m_model = models->getModel("Models/Model/cube.fbx");
}