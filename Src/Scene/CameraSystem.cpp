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

glm::mat4x4 CameraSystem::getMatrix(Entity e) const
{
	ResourcePtr<TransformSystem> transforms;
	EntityIterator<TransformComponent, CameraComponent> it = m_components->findEntity<TransformComponent, CameraComponent>(e);
	TransformComponent* t = it.get<TransformComponent>();
	CameraComponent* c = it.get<CameraComponent>();

	glm::mat4 view = glm::mat4(1.0f);
	if (t)
	{
		view[2][2] = -1.0f; // flip the z axis
		view = glm::mat4_cast(t->m_rotation) * glm::translate(view, t->m_position);
	}

	if (c->m_flags & CameraComponent::Perspective)
	{
		return glm::perspective(c->m_fov, c->m_aspect, c->m_near, c->m_far) * view;
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