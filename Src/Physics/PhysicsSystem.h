#pragma once

#include "../Managers/EventManager.h"
#include "../ECS/ComponentManager.h"
#include "../ECS/System.h"
#include "../Meta/Meta.h"
struct PhysicsComponent : public Component<PhysicsComponent>
{
	COMPONENT_PROPERTY_PTR(btCollisionShape*, Shape);
	COMPONENT_PROPERTY_PTR(btRigidBody*, Body);
	//btCollisionShape* m_shape;
	//btRigidBody* m_body;

	PhysicsComponent() : _Shape(nullptr), _Body(nullptr) { }
	PhysicsComponent(const PhysicsComponent& p) : _Shape(p._Shape), _Body(p._Body) { }
	PhysicsComponent& operator=(const PhysicsComponent& p) {  reset(); _Shape = p._Shape; _Body = p._Body; return *this; }
	PhysicsComponent& operator=(PhysicsComponent&& p) { reset(); _Shape = p._Shape; _Body = p._Body; p._Body = nullptr; p._Shape = nullptr; Component<PhysicsComponent>::operator=(p); return *this; }
	~PhysicsComponent();
	void reset();
};

struct CollisionEvent : public PersistentEvent<CollisionEvent>
{
	Entity m_entity[2];
	glm::vec3 m_pointOn1[4];
	glm::vec3 m_pointOn2[4];

	Entity getEntity1() { return m_entity[0]; }
	Entity getEntity2() { return m_entity[1]; }
};

struct PhysicsDebugDraw;
class PhysicsSystem : public System, public SingletonResource<PhysicsSystem>
{
public:
	PhysicsSystem();
	~PhysicsSystem();

	void setGlobalGravity(const glm::vec3&);
	glm::vec3 getGlobalGravity() const;

	PhysicsComponent* createBox(Entity, const glm::vec3& size, float mass = 1.0f);
	PhysicsComponent* createPlane(Entity, const glm::vec3& size, float mass = 1.0f);

	void impulse(Entity, const glm::vec3&);
	void setGravity(Entity, const glm::vec3&);

	void processWorld(float delta);
	void process(float delta);
	void render(RenderEvent* e);

	void imgui();

protected:
	void tickCallback(btDynamicsWorld* world, btScalar timeStep);

protected:
	ResourcePtr<ComponentManager> m_components;
	ResourcePtr<EventManager> m_events;

	std::vector<CollisionEvent*> m_collisions;
	
	std::unique_ptr<btCollisionConfiguration> m_configuration{ nullptr };
	std::unique_ptr<btBroadphaseInterface> m_broadphase{ nullptr };
	std::unique_ptr<btConstraintSolver> m_constraintSolver{ nullptr };
	std::unique_ptr<btDispatcher> m_dispatcher{ nullptr };
	std::unique_ptr<btDynamicsWorld> m_world{ nullptr };
	std::unique_ptr<PhysicsDebugDraw> m_debugDrawer{ nullptr };

	bool m_doDebugDraw{ false };

	friend struct PhysicsComponent;
};

namespace Meta
{
	template<>
	inline Object instanceMeta<PhysicsComponent>()
	{
		return Object("PhysicsComponent");
	}

	template<>
	inline Object instanceMeta<CollisionEvent>()
	{
		return Object("CollisionEvent").
			func("getEntity1", &CollisionEvent::getEntity1).
			func("getEntity2", &CollisionEvent::getEntity2);
	}

	template<>
	inline Object instanceMeta<PhysicsSystem>()
	{
		return Object("PhysicsSystem").
			func<PhysicsSystem, void, const glm::vec3&>("setGlobalGravity", &PhysicsSystem::setGlobalGravity, { "vector" }, { {0.0f, 0.0f, 0.0f} }).
			func("getGlobalGravity", &PhysicsSystem::getGlobalGravity).
			func("setGlobalGravity", &PhysicsSystem::setGlobalGravity, { "vector" }).
			func("setGravity", &PhysicsSystem::setGravity, { "entity", "vector" }).
			func("createBox", &PhysicsSystem::createBox, { "entity", "size", "mass" }, { 1.0f }).
			func("createPlane", &PhysicsSystem::createPlane, { "entity", "size", "mass" }, { 1.0f }).
			func("impulse", &PhysicsSystem::impulse, { "entity", "vector" });
	}
}