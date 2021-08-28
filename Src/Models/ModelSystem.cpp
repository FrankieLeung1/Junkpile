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
#include "../imgui/ImGuiManager.h"

ModelSystem::ModelSystem():
m_cameraBuffer(nullptr),
m_shaderType(0),
m_skin(0)
{
	ResourcePtr<EventManager> events;
	events->addListener<UpdateEvent>([=](UpdateEvent* e) { update(e->m_delta); });

	m_cameraBuffer = new Rendering::Buffer(Rendering::Buffer::Uniform, Rendering::Buffer::Mapped, sizeof(CameraUBO));
}

ModelSystem::~ModelSystem()
{
	delete m_cameraBuffer;
}

void ModelSystem::update(float delta)
{
	ResourcePtr<ModelManager> models;
	ResourcePtr<Rendering::Depot> depot;
	EntityIterator<TransformComponent, ModelComponent> it(true);
	ResourcePtr<Rendering::Shader> vshader = depot->getTexturedVertexShader();
	ResourcePtr<Rendering::Shader> fshader{ EmptyPtr };
	if (m_shaderType == 0) fshader = depot->getTexturedFragmentShader();
	else if (m_shaderType == 1) fshader = depot->getBWFragmentShader();
	else fshader = depot->getInvertFragmentShader();

	if (!vshader || !fshader)
		return;

	while (it.next())
	{
		ModelComponent* model = it.get<ModelComponent>();
		const ModelManager::ModelData* data = models->getModelData(model->m_model);
		if (!data->m_vBuffer)
			continue;

		ResourcePtr<Rendering::Device> device;
		ResourcePtr<CameraSystem> cameras;
		glm::mat4 view, proj;
		cameras->getMatrices(m_cameraEntity, &view, &proj);

		/*glm::mat4x4 cameraMatrix = view * proj;
		std::vector<char> pushData(sizeof(glm::mat4));
		memcpy(&pushData[0], &cameraMatrix, sizeof(glm::mat4));*/

		CameraUBO* m = (CameraUBO*)m_cameraBuffer->map();
		m->m_projection = proj;
		m->m_model = view;
		m_cameraBuffer->unmap();

		Rendering::Unit unit;
		unit.in(vshader);
		unit.in(fshader);
		unit.in(data->m_vBuffer);
		if(data->m_iBuffer) unit.in(data->m_iBuffer);
		unit.in(Rendering::Unit::DepthTest{ vk::CompareOp::eLessOrEqual, true, true });
		unit.in((vk::CullModeFlags)vk::CullModeFlagBits::eBack);
		unit.in(Rendering::Unit::Binding<ResourcePtr<Rendering::Texture>>(vk::ShaderStageFlagBits::eFragment, 1, model->m_texture1));
		//unit.in({ vk::ShaderStageFlagBits::eVertex, std::move(pushData) });
		unit.in(Rendering::Unit::Binding<Rendering::Buffer*>{ vk::ShaderStageFlagBits::eVertex, 0, m_cameraBuffer });
		unit.in(std::array<float, 4>{ 0.45f, 0.55f, 0.6f, 1.0f });
		unit.in(vk::PrimitiveTopology::eTriangleList);

		//  m_indexCount, m_instanceCount, m_firstIndex, m_vertexOffset, m_firstInstance
		if (data->m_indexCount)
			unit.in({ (uint32_t)data->m_indexCount, 1, 0, 0, 0 });
		else
			unit.in({ (uint32_t)data->m_vertexCount, 1, 0, 0 });

		unit.submit();
	}
}

void ModelSystem::test()
{
	ResourcePtr<ModelManager> models;
	ResourcePtr<ModelSystem> modelSystem;
	ResourcePtr<ComponentManager> components;
	ResourcePtr<CameraSystem> cameras;
	auto entityIt = components->addEntity<TransformComponent, ModelComponent>();
	ModelComponent* model = entityIt.get<ModelComponent>();
	model->m_model = models->getModel("Art/Characters1/Model/characterMedium.fbx");
	model->m_texture1 = ResourcePtr<Rendering::Texture>(NewPtr, "Art/Characters1/Skins/criminalMaleA.png");
	//model->m_model = models->getModel("Models/Animations/idle.fbx");
	
	model->m_texture1.waitReady(nullptr);

	Rendering::Unit unit;
	unit.in(model->m_texture1);
	unit.out(*model->m_texture1.get());
	unit.submit();
	
	auto cameraIt = components->addEntity<TransformComponent>();
	CameraComponent* cameraComponent = cameras->addComponentPerspective(cameraIt.getEntity(), 90.0f);
	cameraComponent->m_controlType = CameraComponent::Orbit;
	cameraComponent->m_useModel = true;
	//cameraIt.get<TransformComponent>()->m_position.x = 0.0f;
	modelSystem->m_cameraEntity = cameraIt.getEntity();

	ResourcePtr<EventManager> events;
	events->addListener<ImGuiRenderEvent>([](ImGuiRenderEvent*) {
		ResourcePtr<ModelSystem> models;
		ImGui::Begin("Model");

		const char* items[] = { "criminalMaleA", "cyborgFemaleA", "skaterFemaleA", "skaterMaleA", "UV" };
		if (ImGui::ListBox("Models", &models->m_skin, items, (int)countof(items)))
		{
			EntityIterator<ModelComponent> entityIt(true);
			entityIt.next();
			ModelComponent* model = entityIt.get<ModelComponent>();
			const char* path = nullptr;
			switch (models->m_skin)
			{
			case 0: path = "Art/Characters1/Skins/criminalMaleA.png"; break;
			case 1: path = "Art/Characters1/Skins/cyborgFemaleA.png"; break;
			case 2: path = "Art/Characters1/Skins/skaterFemaleA.png"; break;
			case 3: path = "Art/Characters1/Skins/skaterMaleA.png"; break;
			case 4: path = "Art/Characters1/Skins/uv.png"; break;
			}
			ResourcePtr<Rendering::Texture> texture(NewPtr, path);
			texture.waitReady(nullptr);
			model->m_texture1 = texture;

			Rendering::Unit unit;
			unit.in(model->m_texture1);
			unit.out(*model->m_texture1.get());
			unit.submit();
		}
		ImGui::Combo("Shader", &models->m_shaderType, "Textured\0Grayscale\0Inverted\0\0");
		ImGui::End();
	});
}