#pragma once
#include "../Resources/ResourceManager.h"
#include "../ECS/ComponentManager.h"

struct UpdateEvent;
class CameraSystem;
struct CameraComponent : public Component<CameraComponent, CameraSystem>
{
	static constexpr const char* m_cid = "Camera";

	static const int Perspective = (1);
	static const int Orthographic = (1) << 1;
	static const int ActiveCamera = (1) << 2;
	int m_flags;

	enum ControlType { None, WASD, Orbit };
	ControlType m_controlType;

	union {
		struct { float m_fov, m_aspect, m_near, m_far; }; // perspective
		struct { float m_left, m_right, m_bottom, m_top; }; // orthographic
	};

	// Arcball
	glm::vec3 m_angles;
	glm::vec3 m_offset;

	CameraComponent() : m_flags(Perspective), m_fov(90.0f), m_aspect(16.0f / 9.0f), m_near(0.1f), m_far(10000.0f), m_angles(0.0f), m_offset(0.0f) {}
	//CameraComponent() : m_flags(Orthographic), m_controlType(None), m_left(0.0f), m_right(0.0f), m_bottom(0.0f), m_top(0.0f), m_lookAt(0.0f) {}
};

class CameraSystem : public SingletonResource<CameraSystem>
{
public:
	CameraSystem();
	~CameraSystem();

	CameraComponent* addComponentPerspective(Entity e, float fov = 90.0f);
	CameraComponent* addComponentOrthographic(Entity e);
	void setCameraActive(Entity);
	void setWASDInput(Entity);
	void setNoInput(Entity);

	void update(const UpdateEvent*);

	bool getActiveMatrices(glm::mat4* view, glm::mat4* projection) const;
	void getMatrices(Entity, glm::mat4* view, glm::mat4* projection) const;

protected:
	ResourcePtr<ComponentManager> m_components;
};

namespace Meta
{
	template<> inline Object instanceMeta<CameraComponent>()
	{
		return Object("CameraComponent").
			var("m_flags", &CameraComponent::m_flags).
			//var("m_controlType", &CameraComponent::m_controlType).
			var("m_angles", &CameraComponent::m_angles).
			var("m_offset", &CameraComponent::m_offset);
	}

	template<> inline Object instanceMeta<CameraSystem>()
	{
		return Object("CameraSystem").
			func("addComponentPerspective", &CameraSystem::addComponentPerspective, { "entity", "fov" }, { 90.0f }).
			func("addComponentOrthographic", &CameraSystem::addComponentOrthographic, { "entity" }).
			func("setCameraActive", &CameraSystem::setCameraActive, { "entity" }).
			func("setWASDInput", &CameraSystem::setWASDInput, { "entity" }).
			func("setNoInput", &CameraSystem::setNoInput, { "entity" });
		
	}
}