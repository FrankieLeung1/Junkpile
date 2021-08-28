#include "stdafx.h"
#include "CameraSystem.h"
#include "glm/glm/gtx/rotate_vector.hpp"
#include "../Rendering/RenderingDevice.h"
#include "TransformSystem.h"
#include "../Managers/EventManager.h"
#include "../Managers/InputManager.h"
#include "../Managers/DebugManager.h"

static float g_defaultFarPlane = 9999.0f;

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
	component->m_far = g_defaultFarPlane;
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
	component->m_near = 0.1f;
	component->m_far = g_defaultFarPlane;
	component->m_angles = glm::vec3(0.0f);
	component->m_offset = glm::vec3(0.0f);
	return component;
}

void CameraSystem::setCameraActive(Entity e)
{
	EntityIterator<CameraComponent> it(true);
	while (it.next())
	{
		CameraComponent* component = it.get<CameraComponent>();
		if (it.getEntity() == e)
			component->m_flags = (component->m_flags | CameraComponent::ActiveCamera);
		else
			component->m_flags = (component->m_flags & ~CameraComponent::ActiveCamera);
	}
}

void CameraSystem::setWASDInput(Entity e)
{
	EntityIterator<CameraComponent> it(true);
	while (it.next())
	{
		if (it.getEntity() == e)
			it.get<CameraComponent>()->m_controlType = CameraComponent::ControlType::WASD;
	}
}

void CameraSystem::setNoInput(Entity e)
{
	EntityIterator<CameraComponent> it(true);
	while (it.next())
	{
		if (it.getEntity() == e)
			it.get<CameraComponent>()->m_controlType = CameraComponent::ControlType::None;
	}
}

void CameraSystem::update(const UpdateEvent* e)
{
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

	EntityIterator<TransformComponent, CameraComponent> it(true);
	while (it.next())
	{
		TransformComponent* transform = it.get<TransformComponent>();
		CameraComponent* camera = it.get<CameraComponent>();

		/*float distance = (float)glm::length(transform->m_position);
		DEBUG_VAR(camera->m_angles);
		DEBUG_VAR(transform->m_position);
		DEBUG_VAR(camera->m_offset);
		DEBUG_VAR(distance);*/

		switch (camera->m_controlType)
		{
		case CameraComponent::WASD:
			transform->m_position += wasdTransformVector;
			transform->m_position.z -= mouseWheel;
			break;

		case CameraComponent::Orbit:
			{
				float zoomDelta = 0.0f;
				if (inputs->isDown(VK_MBUTTON)) zoomDelta = inputs->getCursorPosDelta().y;
				if (mouseWheel) zoomDelta = (mouseWheel > 0 ? -1.0f : 1.0f);
				camera->m_offset.z += zoomDelta;

				if (inputs->justDown('R'))
				{
					transform->m_position = glm::vec3(0.0f, 0.0f, -5.0f);
					camera->m_angles = glm::vec3(0.0f, 0.0f, 0.0f);
					camera->m_offset = glm::vec3(0.0f);
				}
				
				if (inputs->isDown(VK_LBUTTON))
				{
					glm::vec2 drag(0.0f);
					drag = inputs->getCursorPosDelta() * 0.5f;
					camera->m_angles += glm::vec3(drag.y, -drag.x, 0.0f);
				}

				if (inputs->isDown(VK_RBUTTON))
				{
					glm::vec2 drag(0.0f);
					drag = inputs->getCursorPosDelta() * 0.5f;
					camera->m_angles += glm::vec3(0.0f, 0.0f, -drag.x);
				}

				transform->m_rotation = glm::quat(glm::vec3(-camera->m_angles.x, -camera->m_angles.y, 0.0f));
				break;
			}
		}
	}
}

std::tuple<glm::vec3, glm::vec3> CameraSystem::screenToWorld(glm::vec2 coords, Entity cameraEntity) const
{
	ResourcePtr<InputManager> inputs;
	if (coords.x == std::numeric_limits<float>::infinity() || coords.y == std::numeric_limits<float>::infinity())
		coords = inputs->getCursorPosNDC();

	CameraComponent* cameraComponent = nullptr;
	TransformComponent* cameraTransform = nullptr;

	bool findActiveCamera = !cameraEntity;
	EntityIterator<TransformComponent, CameraComponent> it(true);
	while (it.next())
	{
		CameraComponent* c = it.get<CameraComponent>();
		if ((findActiveCamera && (c->m_flags & CameraComponent::ActiveCamera) != 0) || (c->m_entity == cameraEntity))
		{
			cameraComponent = c;
			cameraTransform = it.get<TransformComponent>();
		}
	}
	CHECK_F(cameraComponent && cameraTransform); // couldn't find camera

	if ((cameraComponent->m_flags & CameraComponent::Orthographic) != 0)
	{
		// TODO: work camera rotation into this
		float left, right, top, bottom;
		getOrthographicBounds(cameraComponent, &left, &right, &top, &bottom);

		float width = glm::abs(left) + glm::abs(right), height = glm::abs(top) + glm::abs(bottom);

		glm::vec3 origin = cameraTransform->m_position;
		origin.x += (coords.x < 0.0f ? coords.x * -left : coords.x * right) * 2.0f;
		origin.y += (coords.y < 0.0f ? coords.y * bottom : coords.y * -top) * 2.0f;

		glm::vec3 ray = cameraTransform->m_rotation * glm::vec3(0.0f, 0.0f, 1.0f);
		return std::tuple<glm::vec3, glm::vec3>(origin, ray);
	}
	else if ((cameraComponent->m_flags & CameraComponent::Perspective) != 0)
	{
		const float fov = glm::radians(cameraComponent->m_fov);
		const glm::vec3 left = cameraTransform->m_rotation * glm::vec3(-1.0f, 0.0f, 0.0f);
		const glm::vec3 up = cameraTransform->m_rotation * glm::vec3(0.0f, 1.0f, 0.0f);
		glm::vec3 ray = cameraTransform->m_rotation * glm::vec3(0.0f, 0.0f, 1.0f); // straight forward
		ray = glm::rotate(ray, coords.x * fov, up);
		ray = glm::rotate(ray, coords.y * fov, left);

		return std::tuple<glm::vec3, glm::vec3>(cameraTransform->m_position, ray);
	}
	
	LOG_F(WARNING, "Unknown camera type\n");
	return {};
}

bool CameraSystem::getOrthographicBounds(CameraComponent* c, float* left, float* right, float* top, float* bottom) const
{
	CHECK_F(c != nullptr);
	if ((c->m_flags & CameraComponent::Orthographic) == 0)
		return false;

	if (!c->m_left && !c->m_right && !c->m_top && !c->m_bottom)
	{
		ResourcePtr<Rendering::Device> device;
		auto resolution = std::get<1>(device->getFrameBuffer());
		float halfWidth = resolution.x / 2.0f, halfHeight = resolution.y / 2.0f;
		*left = -halfWidth;
		*right = halfWidth;
		*top = halfHeight;
		*bottom = -halfHeight;
	}
	else
	{
		*left = c->m_left;
		*right = c->m_right;
		*top = c->m_top;
		*bottom = c->m_bottom;
	}

	return true;
}

bool CameraSystem::getActiveMatrices(glm::mat4* view, glm::mat4* projection) const
{
	EntityIterator<TransformComponent, CameraComponent> it(true);
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
	if (!projection) projection = (glm::mat4*)alloca(sizeof(glm::mat4));

	*view = glm::mat4(1.0f);
	if (t)
	{
		if (c->m_useModel)
		{
			*view = glm::rotate(*view, glm::radians(c->m_angles.x), glm::vec3(1.0f, 0.0f, 0.0f));
			*view = glm::rotate(*view, glm::radians(c->m_angles.y), glm::vec3(0.0f, 1.0f, 0.0f));
			*view = glm::rotate(*view, glm::radians(c->m_angles.z), glm::vec3(0.0f, 0.0f, 1.0f));
			*view = glm::translate(glm::mat4(1.0f), c->m_offset) * (*view);
		}
		else
		{
			(*view)[1][1] = -1.0f; // flip the y axis
			(*view)[2][2] = -1.0f; // flip the z axis
			(*view) = glm::mat4_cast(t->m_rotation) * glm::translate(*view, t->m_position);
		}
	}

	if (c->m_flags & CameraComponent::Perspective)
	{
		*projection = glm::perspective(c->m_fov, c->m_aspect, c->m_near, c->m_far);
	}
	else if (c->m_flags & CameraComponent::Orthographic)
	{
		float l, r, t, b;
		getOrthographicBounds(c, &l, &r, &t, &b);
		*projection = glm::ortho(l, r, b, t, c->m_near, c->m_far);
	}
	else
	{
		LOG_F(ERROR, "Invalid camera\n");
	}
}