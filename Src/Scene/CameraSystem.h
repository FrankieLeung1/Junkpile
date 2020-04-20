#pragma once
#include "../Resources/ResourceManager.h"
#include "../ECS/ComponentManager.h"

struct UpdateEvent;
struct CameraComponent : public Component<CameraComponent>
{
	static constexpr const char* m_cid = "Camera";

	static const int Perspective = (1);
	static const int Orthographic = (1) << 1;
	static const int ActiveCamera = (1) << 2;
	int m_flags;

	union {
		struct { float m_fov, m_aspect, m_near, m_far; };
		struct { float m_left, m_right, m_bottom, m_top; };
	};

	//CameraComponent() : m_flags(Perspective), m_fov(90.0f), m_aspect(16.0f / 9.0f), m_near(0.0f), m_far(1.0f) {}
	CameraComponent() : m_flags(Orthographic), m_left(0.0f), m_right(0.0f), m_bottom(0.0f), m_top(0.0f) {}
};

class CameraSystem : public SingletonResource<CameraSystem>
{
public:
	CameraSystem();
	~CameraSystem();

	void update(const UpdateEvent*);

	glm::mat4x4 getMatrix(Entity) const;

protected:
	ResourcePtr<ComponentManager> m_components;
};