#pragma once

#include "../Managers/EventManager.h"
#include "../ECS/ComponentManager.h"
#include "../ECS/System.h"
struct PhysicsComponent : public Component<PhysicsComponent>
{
	static constexpr const char* m_cid = "Physics";

	btCollisionShape* m_shape;
	btRigidBody* m_body;

	PhysicsComponent() : m_shape(nullptr), m_body(nullptr) {}
	PhysicsComponent(const PhysicsComponent&) { CHECK_F(false); }
	PhysicsComponent(PhysicsComponent&& p) : m_shape(p.m_shape), m_body(p.m_body) { m_shape = nullptr; m_body = nullptr; }
	PhysicsComponent& operator=(const PhysicsComponent&) { CHECK_F(false); return *this; }
	PhysicsComponent& operator=(PhysicsComponent&& p) { m_shape = p.m_shape; m_body = p.m_body;  p.m_shape = nullptr; p.m_body = nullptr; return *this; }
	~PhysicsComponent();
};

struct CollisionEvent : public PersistentEvent<CollisionEvent>
{
	Entity m_entity[2];
	glm::vec3 m_pointOn1[4];
	glm::vec3 m_pointOn2[4];
};

struct PhysicsDebugDraw;
class PhysicsSystem : public System, public SingletonResource<PhysicsSystem>
{
public:
	PhysicsSystem();
	~PhysicsSystem();

	void setGravity(const glm::vec3&);
	glm::vec3 getGravity() const;

	PhysicsComponent* createBox(Entity, const glm::vec3& size, float mass = 1.0f);
	PhysicsComponent* createPlane(Entity, const glm::vec3& size, float mass = 1.0f);

	void processWorld(float delta);
	void process(float delta);

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

	friend struct PhysicsComponent;
};