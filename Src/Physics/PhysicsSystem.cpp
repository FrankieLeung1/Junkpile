#include "stdafx.h"
#include "PhysicsSystem.h"
#include "../imgui/ImGuiManager.h"
#include "../Misc/Misc.h"
#include "../Scene/TransformSystem.h"
#include "../Managers/DebugManager.h"

btVector3 toBt(const glm::vec3& v) { return btVector3(v[0], v[1], v[2]); }
btVector4 toBt(const glm::vec4& v) { return btVector4(v[0], v[1], v[2], v[3]); }
glm::vec3 toGlm(const btVector3& v) { return glm::vec3(v[0], v[1], v[2]); }
glm::vec4 toGlm(const btVector4& v) { return glm::vec4(v[0], v[1], v[2], v[3]); }

struct PhysicsDebugDraw : public btIDebugDraw
{
	void drawLine(const btVector3& from, const btVector3& to, const btVector3& color);
	void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override;
	void reportErrorWarning(const char* warningString);
	void draw3dText(const btVector3& location, const char* textString);
	void setDebugMode(int debugMode);
	int getDebugMode() const;

	int m_debugMode{ 0 };
};

PhysicsSystem::PhysicsSystem():
m_components(),
m_events(),
m_collisions(),
m_world(nullptr)
{
	m_components->addComponentType<PhysicsComponent>(256);

	btAlignedAllocSetCustom([](std::size_t size) -> void* { return new char[size*sizeof(char)]; }, [](void* memblock) { return delete[] (char*)memblock; });

	m_configuration = decltype(m_configuration)(new btDefaultCollisionConfiguration());
	m_dispatcher = decltype(m_dispatcher)(new btCollisionDispatcher(m_configuration.get()));
	m_broadphase = decltype(m_broadphase)(new btDbvtBroadphase());
	m_constraintSolver = decltype(m_constraintSolver)(new btSequentialImpulseConstraintSolver());
	m_world = decltype(m_world)(new btDiscreteDynamicsWorld(m_dispatcher.get(), m_broadphase.get(), m_constraintSolver.get(), m_configuration.get()));

	m_debugDrawer = decltype(m_debugDrawer)(new PhysicsDebugDraw());
	m_debugDrawer->setDebugMode(btIDebugDraw::DBG_DrawWireframe + btIDebugDraw::DBG_DrawContactPoints /*+ btIDebugDraw::DBG_DrawAabb*/);
	m_world->setDebugDrawer(m_debugDrawer.get());
	m_world->setGravity(btVector3(0, -9.81f, 0));

	m_world->setInternalTickCallback([](btDynamicsWorld* world, btScalar timeStep) { ((PhysicsSystem*)world->getWorldUserInfo())->tickCallback(world, timeStep); }, this);

	gContactDestroyedCallback = [](void* ud) { ((CollisionEvent*)ud)->m_eventDeath = 0; return true; };

	ResourcePtr<EventManager> events;
	events->addListener<UpdateEvent>([this](UpdateEvent*) { processWorld(0.16f); process(0.16f); }, 10);
	events->addListener<RenderEvent>([this](RenderEvent* e) { render(e); });
}

PhysicsSystem::~PhysicsSystem()
{
	EntityIterator<PhysicsComponent> p(true);
	while (p.next())
	{
		auto* pc = p.get<PhysicsComponent>();
		m_world->removeRigidBody(pc->getBody());
		delete pc->getBody()->getMotionState();
		delete pc->getShape();
		delete pc->getBody();
		pc->setShape(nullptr);
		pc->setBody(nullptr);
	}
}

void PhysicsSystem::setGlobalGravity(const glm::vec3& v)
{
	m_world->setGravity(toBt(v));
}

glm::vec3 PhysicsSystem::getGlobalGravity() const
{
	btVector3 gravity = m_world->getGravity();
	return glm::vec3(gravity[0], gravity[1], gravity[2]);
}

PhysicsComponent* PhysicsSystem::createBox(Entity entity, const glm::vec3& size, float mass)
{
	// TODO: btGeneric6DofConstraint

	PhysicsComponent* component = m_components->addComponents<PhysicsComponent>(entity).get<PhysicsComponent>();
	component->setShape(new btBoxShape(btVector3(size[0], size[1], size[2])));

	btTransform startTransform;
	startTransform.setIdentity();

	auto it = m_components->findEntity<TransformComponent>(entity);
	if (it.valid())
		startTransform.setOrigin(toBt(it.get<TransformComponent>()->m_position));

	bool isDynamic = (mass != 0.f);

	btVector3 localInertia(0, 0, 0);
	if (isDynamic)
		component->getShape()->calculateLocalInertia(mass, localInertia);

	btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, component->getShape(), localInertia);
	component->setBody(new btRigidBody(rbInfo));
	component->getBody()->setUserIndex(entity.m_value);

	m_world->addRigidBody(component->getBody());
	return component;
}

PhysicsComponent* PhysicsSystem::createPlane(Entity entity, const glm::vec3& size, float mass)
{
	return nullptr;
}

void PhysicsSystem::impulse(Entity e, const glm::vec3& v)
{
	auto c = m_components->findEntity<PhysicsComponent>(e).get<PhysicsComponent>();
	if (c)
	{
		c->getBody()->activate();
		c->getBody()->applyImpulse(toBt(v), btVector3(0.0f, 0.0f, 0.0f));
	}
}

void PhysicsSystem::setGravity(Entity e, const glm::vec3& gravity)
{
	auto c = m_components->findEntity<PhysicsComponent>(e).get<PhysicsComponent>();
	if (c)
		c->getBody()->setGravity(toBt(gravity));
}

void PhysicsSystem::processWorld(float delta)
{
	m_world->stepSimulation(delta, 10);
}

void PhysicsSystem::process(float delta)
{
	EntityIterator<TransformComponent, PhysicsComponent> it(false);
	while (it.next())
	{
		PhysicsComponent* physics = it.get<PhysicsComponent>();
		if (!physics)
			continue;

		TransformComponent* position = it.get<TransformComponent>();
		if (position)
		{
			btMotionState* motionState = physics->getBody()->getMotionState();
			if (motionState)
			{
				btTransform transform;
				motionState->getWorldTransform(transform);
				position->m_position = toGlm(transform.getOrigin());
			}
		}
	}
}

void PhysicsSystem::tickCallback(btDynamicsWorld* world, btScalar timeStep)
{
	for (auto& collision : m_collisions)
		collision->m_eventDeath = collision->m_eventLife;

	for (int i = 0; i < m_dispatcher->getNumManifolds(); i++)
	{
		btPersistentManifold* contactManifold = m_dispatcher->getManifoldByIndexInternal(i);
		const btCollisionObject* obA = contactManifold->getBody0();
		const btCollisionObject* obB = contactManifold->getBody1();
		int numContacts = contactManifold->getNumContacts();
		if (numContacts <= 0)
			continue;

		CollisionEvent* event = nullptr;
		for (int j = 0; j < numContacts; j++)
		{
			event = (CollisionEvent*)contactManifold->getContactPoint(0).m_userPersistentData;
			if (event)
				break;
		}
		if (!event || event->m_eventDeath == 0.0f)
			event = m_events->addPersistentEvent<CollisionEvent>();

		auto count = countof(event->m_pointOn1);
		CHECK_F(countof(event->m_pointOn1) >= numContacts);
		event->m_entity[0].m_value = obA->getUserIndex();
		event->m_entity[1].m_value = obB->getUserIndex();
		event->m_eventDeath = std::numeric_limits<float>::infinity();
		
		memset(event->m_pointOn1, 0x00, sizeof(event->m_pointOn1));
		memset(event->m_pointOn2, 0x00, sizeof(event->m_pointOn2));

		for (int j = 0; j < numContacts; j++)
		{
			// F: do I need to check distance?
			btManifoldPoint& cp = contactManifold->getContactPoint(j);
			event->m_pointOn1[j] = toGlm(cp.getPositionWorldOnA());
			event->m_pointOn2[j] = toGlm(cp.getPositionWorldOnB());
			cp.m_userPersistentData = event;
		}
	}
}

void PhysicsSystem::imgui()
{
	ResourcePtr<ImGuiManager> im;
	bool* opened = im->win("Physics");
	if (*opened == false)
		return;

	if (ImGui::Begin("Physics", opened))
	{
		ImGui::Text("Collision Objects: %d", m_world->getNumCollisionObjects());
		ImGui::Text("Contacts: %d", m_world->getDispatcher()->getNumManifolds());

		ImGui::Columns(3);

		ImGui::Separator();
		ImGui::Text("Entity1"); ImGui::NextColumn();
		ImGui::Text("Entity2"); ImGui::NextColumn();
		ImGui::Text("Lifetime"); ImGui::NextColumn();
		ImGui::Separator();

		for (int i = 0; i < m_dispatcher->getNumManifolds(); i++)
		{
			btPersistentManifold* contactManifold = m_dispatcher->getManifoldByIndexInternal(i);
			const btCollisionObject* obA = contactManifold->getBody0();
			const btCollisionObject* obB = contactManifold->getBody1();

			int largestLifetime = 0;
			int numContacts = contactManifold->getNumContacts();
			for (int j = 0; j < numContacts; j++)
			{
				btManifoldPoint& cp = contactManifold->getContactPoint(j);
				largestLifetime = std::max(largestLifetime, cp.getLifeTime());
			}

			ImGui::Text("%d", obA->getUserIndex()); ImGui::NextColumn();
			ImGui::Text("%d", obB->getUserIndex()); ImGui::NextColumn();
			ImGui::Text("%d", largestLifetime); ImGui::NextColumn();
		}

		ImGui::Separator();
		ImGui::Columns(1);

		int drawFlags = m_debugDrawer->getDebugMode();
		bool wireframe = (drawFlags & btIDebugDraw::DBG_DrawWireframe) != 0;
		if (ImGui::Checkbox("Wireframe", &wireframe))
			drawFlags = wireframe ? drawFlags | btIDebugDraw::DBG_DrawWireframe : drawFlags & ~btIDebugDraw::DBG_DrawWireframe;

		bool aabb = (drawFlags & btIDebugDraw::DBG_DrawAabb) != 0;
		if (ImGui::Checkbox("AABB", &aabb))
			drawFlags = aabb ? drawFlags | btIDebugDraw::DBG_DrawAabb : drawFlags & ~btIDebugDraw::DBG_DrawAabb;

		bool contacts = (drawFlags & btIDebugDraw::DBG_DrawContactPoints) != 0;
		if (ImGui::Checkbox("Contact Points", &contacts))
			drawFlags = contacts ? drawFlags | btIDebugDraw::DBG_DrawContactPoints : drawFlags & ~btIDebugDraw::DBG_DrawContactPoints;

		m_debugDrawer->setDebugMode(drawFlags);
	}
	ImGui::End();

	int flags = ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground;
	if (ImGui::Begin("PhysicsDebugDraw", nullptr, flags))
	{
		m_doDebugDraw = true;
		m_world->debugDrawWorld();
	}
	else
	{
		m_doDebugDraw = false;
	}
	ImGui::End();
}

void PhysicsSystem::render(RenderEvent* e)
{
	if (m_doDebugDraw)
	{
		//DEBUG_VAR(m_debugDrawer->m_projection);
	}
}

void PhysicsComponent::reset()
{
	if (getBody())
	{
		ResourcePtr<PhysicsSystem> p;
		delete getBody()->getMotionState();
		p->m_world->removeCollisionObject(getBody());
		delete getBody();
		delete getShape();

		setBody(nullptr);
		setShape(nullptr);
	}
}

PhysicsComponent::~PhysicsComponent()
{
	reset();
}

void PhysicsDebugDraw::drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
{
	ResourcePtr<DebugManager> debugManager;
	debugManager->addLine3D(glm::vec4(toGlm(from), 1), glm::vec4(toGlm(to), 1), glm::vec4(toGlm(color), 1));
}

void PhysicsDebugDraw::drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color)
{
	ResourcePtr<DebugManager> debugManager;
	debugManager->addLine3D(glm::vec4(toGlm(PointOnB), 1), glm::vec4(toGlm(PointOnB) + toGlm(normalOnB) * 50.0f, 1), glm::vec4(toGlm(color), 1));
}

void PhysicsDebugDraw::reportErrorWarning(const char* warningString)
{
	LOG_F(ERROR, warningString);
}

void PhysicsDebugDraw::draw3dText(const btVector3& location, const char* textString)
{
	//LOG_F(INFO, textString);
}

void PhysicsDebugDraw::setDebugMode(int debugMode)
{
	m_debugMode = debugMode;
}

int PhysicsDebugDraw::getDebugMode() const
{
	return m_debugMode;
}