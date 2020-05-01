#include "stdafx.h"
#include "CameraSystem.h"
#include "../Rendering/RenderingDevice.h"
#include "TransformSystem.h"
#include "../Managers/EventManager.h"
#include "../Managers/InputManager.h"

CameraSystem::CameraSystem()
{
	ResourcePtr<EventManager> events;
	events->addListener<UpdateEvent>([this](UpdateEvent* e) { this->update(e); });
}

CameraSystem::~CameraSystem()
{

}

void CameraSystem::update(const UpdateEvent* e)
{
	ResourcePtr<ComponentManager> components;
	ResourcePtr<InputManager> inputs;
	float speed = 1.0f;

	glm::vec3 transformVector(0.0f);
	if (inputs->isDown('W')) transformVector.y -= 1;
	if (inputs->isDown('A')) transformVector.x -= 1;
	if (inputs->isDown('S')) transformVector.y += 1;
	if (inputs->isDown('D')) transformVector.x += 1;
	if (inputs->isDown(VK_LSHIFT)) speed *= 2.0f;
	if (transformVector == glm::vec3(0.0f))
		return;

	transformVector = glm::normalize(transformVector) * (e->m_delta * speed);

	EntityIterator<TransformComponent, CameraComponent> it(components, true);
	while (it.next())
	{
		it.get<TransformComponent>()->m_position += transformVector;
	}
}

glm::mat4x4 CameraSystem::getMatrix(Entity e) const
{
	ResourcePtr<TransformSystem> transforms;
	EntityIterator<TransformComponent, CameraComponent> it = m_components->findEntity<TransformComponent, CameraComponent>(e);
	TransformComponent* t = it.get<TransformComponent>();
	CameraComponent* c = it.get<CameraComponent>();

	glm::mat4 view = glm::mat4(1.0f);
	if (t)
	{
		view = glm::translate(view, t->m_position);
		view *= glm::mat4_cast(t->m_rotation);
	}

	if (c->m_flags & CameraComponent::Perspective)
	{
		return view * glm::perspective(c->m_fov, c->m_aspect, c->m_near, c->m_far);
	}
	else if (c->m_flags & CameraComponent::Orthographic)
	{
		if (!c->m_left && !c->m_right && !c->m_top && !c->m_bottom)
		{
			ResourcePtr<Rendering::Device> device;
			auto resolution = std::get<1>(device->getFrameBuffer());
			float halfWidth = resolution.x / 2.0f, halfHeight = resolution.y / 2.0f;
			return view * glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight);
		}
		else
		{
			return view * glm::ortho(c->m_left, c->m_right, c->m_bottom, c->m_top);
		}
	}
	
	return glm::mat4x4();
}