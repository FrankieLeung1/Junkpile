#include "stdafx.h"
#include "CameraSystem.h"
#include "../Rendering/RenderingDevice.h"
#include "TransformSystem.h"

CameraSystem::CameraSystem()
{

}

CameraSystem::~CameraSystem()
{

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