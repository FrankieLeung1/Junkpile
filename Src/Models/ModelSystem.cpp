#include "stdafx.h"
#include "ModelSystem.h"
#include "../Managers/EventManager.h"
#include "../ECS/ComponentManager.h"
#include "../Scene/TransformSystem.h"
#include "../Rendering/Unit.h"
#include "../Rendering/Depot.h"
#include "../Rendering/Buffer.h"
#include "../Rendering/Texture.h"
#include "../Files/File.h"
#include "../Scene/CameraSystem.h"

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
	ResourcePtr<Rendering::Shader> vshader = depot->getTexturedVertexShader();
	ResourcePtr<Rendering::Shader> fshader = depot->getTexturedFragmentShader();
	if (!vshader || !fshader)
		return;

	while (it.next())
	{
		ModelComponent* model = it.get<ModelComponent>();
		const ModelManager::ModelData* data = models->getModelData(model->m_model);
		if (!data->m_vBuffer || !data->m_iBuffer)
			continue;

		ResourcePtr<Rendering::Device> device;
		ResourcePtr<CameraSystem> cameras;
		glm::mat4x4 cameraMatrix = cameras->getMatrix(m_cameraEntity);
		std::vector<char> pushData(sizeof(glm::mat4));
		memcpy(&pushData[0], &cameraMatrix, sizeof(glm::mat4));

		Rendering::Unit unit;
		unit.in(vshader);
		unit.in(fshader);
		unit.in(data->m_vBuffer);
		unit.in(data->m_iBuffer);
		unit.in(Rendering::Unit::Binding<ResourcePtr<Rendering::Texture>>(vk::ShaderStageFlagBits::eFragment, 0, model->m_texture1));
		unit.in({ vk::ShaderStageFlagBits::eVertex, std::move(pushData) });
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
	ModelComponent* model = entityIt.get<ModelComponent>();
	model->m_model = models->getModel("Models/Model/characterMedium.fbx");

	ResourcePtr<File> file(NewPtr, "Models/Skins/uv.png");
	unsigned char* pixels;
	unsigned int width, height;
	int error = lodepng_decode32(&pixels, &width, &height, (const unsigned char*)file->getContents(), file->getSize());
	if (error == 0)
	{
		model->m_texture1 = ResourcePtr<Rendering::Texture>(TakeOwnershipPtr, new Rendering::Texture());
		model->m_texture1->setSoftware(width, height, 32);
		char* dest = (char*)model->m_texture1->map();
		memcpy(dest, pixels, width * height * 4);
		model->m_texture1->unmap();
		free(pixels);

		Rendering::Unit unit;
		unit.in(model->m_texture1);
		unit.out(*model->m_texture1);
		unit.submit();
	}
	
	auto cameraIt = components->addEntity<TransformComponent, CameraComponent>();
	cameraIt.get<CameraComponent>()->m_controlType = CameraComponent::Orbit;
	cameraIt.get<TransformComponent>()->m_position.z = -5.0f;
	modelSystem->m_cameraEntity = cameraIt.getEntity();
}