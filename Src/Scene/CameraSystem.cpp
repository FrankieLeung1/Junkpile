#include "stdafx.h"
#include "CameraSystem.h"
#include "../Rendering/RenderingDevice.h"
#include "TransformSystem.h"
#include "../Managers/EventManager.h"
#include "../Managers/InputManager.h"

CameraSystem::CameraSystem()
{
	m_components->addComponentType<CameraComponent>();

	ResourcePtr<EventManager> events;
	events->addListener<UpdateEvent>([this](UpdateEvent* e) { update(e); });
}

CameraSystem::~CameraSystem()
{
}

CameraComponent* CameraSystem::addComponentPerspective(Entity e, float fov)
{
	ResourcePtr<ComponentManager> components;
	CameraComponent* component = components->addComponents<CameraComponent>(e).get<CameraComponent>();
	component->m_flags = CameraComponent::Perspective;
	component->m_aspect = 16.0f / 9.0f;
	component->m_near = 0.1f;
	component->m_far = 10000.0f;
	component->m_angles = glm::vec3(0.0f);
	component->m_offset = glm::vec3(0.0f);
	component->m_fov = fov;
	return component;
}

CameraComponent* CameraSystem::addComponentOrthographic(Entity e)
{
	ResourcePtr<ComponentManager> components;
	CameraComponent* component = components->addComponents<CameraComponent>(e).get<CameraComponent>();
	component->m_flags = CameraComponent::Orthographic;
	component->m_left = 0.0f;
	component->m_right = 0.0f;
	component->m_bottom = 0.0f;
	component->m_top = 0.0f;
	component->m_angles = glm::vec3(0.0f);
	component->m_offset = glm::vec3(0.0f);

	LOG_F(INFO, "addComponentOrthographic %d\n", e);
	return component;
}

void CameraSystem::setCameraActive(Entity e)
{
	ResourcePtr<ComponentManager> components;
	EntityIterator<CameraComponent> it(components, true);
	while (it.next())
	{
		if (it.getEntity() == e)
			it.get<CameraComponent>()->m_flags |= CameraComponent::ActiveCamera;
		else
			it.get<CameraComponent>()->m_flags &= ~CameraComponent::ActiveCamera;
	}
}

void CameraSystem::setWASDInput(Entity e)
{
	ResourcePtr<ComponentManager> components;
	EntityIterator<CameraComponent> it(components, true);
	while (it.next())
	{
		if (it.getEntity() == e)
			it.get<CameraComponent>()->m_controlType = CameraComponent::ControlType::WASD;
	}
}

void CameraSystem::setNoInput(Entity e)
{
	ResourcePtr<ComponentManager> components;
	EntityIterator<CameraComponent> it(components, true);
	while (it.next())
	{
		if (it.getEntity() == e)
			it.get<CameraComponent>()->m_controlType = CameraComponent::ControlType::None;
	}
}

void CameraSystem::update(const UpdateEvent* e)
{
	ResourcePtr<ComponentManager> components;
	ResourcePtr<InputManager> inputs;
	float speed = 50.0f;
	float mouseWheel = inputs->getMouseWheel();

	glm::vec3 wasdTransformVector(0.0f);
	if (inputs->isDown('W')) wasdTransformVector.y -= 1;
	if (inputs->isDown('A')) wasdTransformVector.x -= 1;
	if (inputs->isDown('S')) wasdTransformVector.y += 1;
	if (inputs->isDown('D')) wasdTransformVector.x += 1;
	if (inputs->isDown(VK_LSHIFT)) speed *= 2.0f;
	if(wasdTransformVector != glm::vec3(0.0f))
		wasdTransformVector = glm::normalize(wasdTransformVector) * (e->m_delta * speed);

	EntityIterator<TransformComponent, CameraComponent> it(components, true);
	while (it.next())
	{
		TransformComponent* transform = it.get<TransformComponent>();
		CameraComponent* camera = it.get<CameraComponent>();

		/*float distance = (float)glm::length(transform->m_position);
		DEBUG_VAR("angles", camera->m_angles);
		DEBUG_VAR("position", transform->m_position);
		DEBUG_VAR("offset", camera->m_offset);
		DEBUG_VAR("distance", distance);*/

		switch (camera->m_controlType)
		{
		case CameraComponent::WASD:
			transform->m_position += wasdTransformVector;
			transform->m_position.z -= mouseWheel;
			break;

		case CameraComponent::Orbit:
			{
				glm::vec3 offsetFromOrigin = camera->m_offset - transform->m_position;
				if (inputs->isDown(VK_MBUTTON))
				{
					float m = inputs->getCursorPosDelta().y * 0.05f;
					if (m)
					{
						float x = glm::abs(offsetFromOrigin.x), y = glm::abs(offsetFromOrigin.y), z = glm::abs(offsetFromOrigin.z);
						if (z > x && z > y) camera->m_offset.y += offsetFromOrigin.z > 0 ? -m : m;
						else camera->m_offset.z += m;
					}
				}

				if (mouseWheel)
				{
					glm::vec3 diff = glm::normalize(offsetFromOrigin) * -mouseWheel;
					if(offsetFromOrigin + diff != glm::vec3(0.0f))
						offsetFromOrigin += diff;
				}

				if (inputs->justDown('R'))
				{
					transform->m_position = glm::vec3(0.0f, 0.0f, -5.0f);
					camera->m_angles = glm::vec3(0.0f, 0.0f, 0.0f);
					camera->m_offset = glm::vec3(0.0f);
				}

				glm::vec2 drag(0.0f);
				if (inputs->isDown(VK_LBUTTON))
				{
					drag = inputs->getCursorPosDelta() * 0.05f;
					camera->m_angles += glm::vec3(drag.y, -drag.x, 0.0f);
				}

				float x = camera->m_angles.x; // pitch
				float y = camera->m_angles.y; // yaw
				float distance = (float)glm::length(offsetFromOrigin);
				transform->m_position = (glm::vec3(0.0f, 0.0f, -1.0f) * glm::quat(glm::vec3(x, y, 0.0f)) * distance) + camera->m_offset;
				transform->m_rotation = glm::quat(glm::vec3(-x, -y, 0.0f));
				break;
			}
		}
	}
}

bool CameraSystem::getActiveMatrices(glm::mat4* view, glm::mat4* projection) const
{
	ResourcePtr<ComponentManager> components;
	EntityIterator<TransformComponent, CameraComponent> it(components, true);
	while (it.next())
	{
		CameraComponent* c = it.get<CameraComponent>();
		if ((c->m_flags & CameraComponent::ActiveCamera) != 0)
		{
			getMatrices(it.getEntity(), view, projection);
			return true;
		}
	}

	return false;
}

void CameraSystem::getMatrices(Entity e, glm::mat4* view, glm::mat4* projection) const
{
	ResourcePtr<TransformSystem> transforms;
	EntityIterator<TransformComponent, CameraComponent> it = m_components->findEntity<TransformComponent, CameraComponent>(e);
	TransformComponent* t = it.get<TransformComponent>();
	CameraComponent* c = it.get<CameraComponent>();
	if (!view) view = (glm::mat4*)alloca(sizeof(glm::mat4));
	if(!projection) projection = (glm::mat4*)alloca(sizeof(glm::mat4));

	*view = glm::mat4(1.0f);
	if (t)
	{
		(*view)[1][1] = -1.0f; // flip the y axis
		(*view)[2][2] = -1.0f; // flip the z axis
		(*view) = glm::mat4_cast(t->m_rotation) * glm::translate(*view, t->m_position);
	}

	if (c->m_flags & CameraComponent::Perspective)
	{
		int width = 1280, height = 720;
		*projection = glm::perspective(c->m_fov, c->m_aspect, c->m_near, c->m_far);
		//*projection = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 256.0f);
		
	}
	else if (c->m_flags & CameraComponent::Orthographic)
	{
		if (!c->m_left && !c->m_right && !c->m_top && !c->m_bottom)
		{
			ResourcePtr<Rendering::Device> device;
			auto resolution = std::get<1>(device->getFrameBuffer());
			float halfWidth = resolution.x / 2.0f, halfHeight = resolution.y / 2.0f;
			*projection = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight);
		}
		else
		{
			*projection = glm::ortho(c->m_left, c->m_right, c->m_bottom, c->m_top);
		}
	}
	else
	{
		LOG_F(ERROR, "Invalid camera\n");
	}
}