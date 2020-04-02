#pragma once
#include "../Resources/ResourceManager.h"
#include "../ECS/ComponentManager.h"

struct CameraComponent : public Component<CameraComponent>
{
	static constexpr const char* m_cid = "Camera";
	float m_fov, m_aspect, m_near, m_far;

	CameraComponent() : m_fov(90.0f), m_aspect(16.0f / 9.0f), m_near(0.0f), m_far(1.0f) {}
};

class CameraSystem : public SingletonResource<CameraSystem>
{
public:
	CameraSystem();
	~CameraSystem();

	void addComponent(Entity);

protected:
	ResourcePtr<ComponentManager> m_components;
};