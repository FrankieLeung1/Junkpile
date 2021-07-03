#pragma once

#include "../Resources/ResourceManager.h"
#include "../ECS/ECS.h"
#include "../Misc/Any.h"
#include "../Meta/Meta.h"
#include "../Scripts/ScriptManager.h"
#include "EntityIterator.h"

class ComponentManager;
struct Entity : public OpaqueHandle<ComponentManager, unsigned int> {
	friend class PhysicsSystem; 
	bool equals(const Entity& p) const { return (*this) == p; };
};
typedef std::size_t ComponentId;
#define INVALID_ENTITY 0

struct EntityIteratorArgs {
	int m_index;
	ComponentId m_id;
	std::size_t m_size;
	Entity m_entity;
	void*& m_vp;
};

template <typename... Ts>
class EntityIterator : protected std::tuple<Ts*...>
{
public:
	EntityIterator(ComponentManager* manager, bool allComponentsMustExist);
	~EntityIterator();

	bool next();
	template<typename T> T* get();
	template<std::size_t i = 0> bool valid() const;

	template<typename VFN> void accept(VFN);

	Entity getEntity() const;

	EntityIterator<Ts...>& operator++();
	EntityIterator<Ts...> operator++(int) const;

protected:
	template<std::size_t i, typename VFN> void accept(std::true_type, VFN);
	template<std::size_t i, typename VFN> void accept(std::false_type, VFN);

protected:
	ComponentManager* m_manager;
	bool m_allComponentsMustExist;
	Entity m_currentEntity;
	friend class ComponentManager;
};

class ComponentManager : public SingletonResource<ComponentManager>
{
public:
	typedef AnyWithSize<sizeof(std::vector<void*>)> ResizeableMemoryPool;
	struct ComponentPool
	{
		class BufferAccessor
		{
		public:
			virtual const void* front(const ResizeableMemoryPool&) = 0;
			virtual std::size_t size(const ResizeableMemoryPool&) = 0;
			virtual std::size_t elementSize() = 0;
			virtual bool empty(const ResizeableMemoryPool&) = 0;
			virtual void clear(ResizeableMemoryPool&) = 0;
			virtual void clearEntity(ResizeableMemoryPool&, Entity) = 0;
			virtual void printEntityIds(const ResizeableMemoryPool&) const = 0;
			virtual const char* getClassName() const = 0;
		};

		template<typename T>
		class BufferAccessorInstance : public BufferAccessor
		{
		public:
			static BufferAccessorInstance<T> s_instance;
			const void* front(const ResizeableMemoryPool& pool) { return &pool.get<std::vector<T>>().front(); }
			std::size_t size(const ResizeableMemoryPool& pool) { return pool.get<std::vector<T>>().size(); }
			std::size_t elementSize() { return sizeof(T); };
			bool empty(const ResizeableMemoryPool& pool) { return pool.get<std::vector<T>>().empty(); }
			void clear(ResizeableMemoryPool& pool) { pool.get<std::vector<T>>().clear(); }
			void clearEntity(ResizeableMemoryPool& pool, Entity e) {
				auto& v = pool.get<std::vector<T>>();
				for (auto it = v.begin(); it != v.end();)
				{
					if (it->m_entity == e) it = v.erase(it);
					else ++it;
				}
			};
			void printEntityIds(const ResizeableMemoryPool& pool) const
			{
				auto& v = pool.get<std::vector<T>>();
				std::stringstream ss;
				ss << (void*)&v << " " << getClassName() << " ";
				for (auto it = v.begin(); it != v.end(); ++it) ss << it->m_entity.m_value << " ";
				LOG_F(INFO, "%s\n", ss.str().c_str());
			}
			const char* getClassName() const { return typeid(T).name(); }
		};

		BufferAccessor* m_accessor;
		ResizeableMemoryPool m_buffer;

		template<typename Component> Component* findComponent(Entity);
	};

public:
	ComponentManager();
	~ComponentManager();

	template<typename T> void addComponentType(std::size_t reserve = 0);
	template<typename T> bool hasComponentType() const;
	template<typename T, typename T2, typename... Ts> bool hasComponentType() const;

	template<typename... Components> EntityIterator<Components...> addEntity();
	Entity newEntity();
	void removeEntity(Entity);
	int debugId(Entity) const;
	void printAllEntityIds() const;

	template<typename Component, typename... Components> EntityIterator<Component, Components...> addComponents(Entity);
	template<typename Component, typename... Components> void removeComponents(Entity);

	template<typename... Components> EntityIterator<Components...> findEntity(Entity);

	template<typename... Components> EntityIterator<Components...> begin();
	template<typename... Components> bool next(EntityIterator<Components...>*);

	template<typename Component> ComponentPool* getPool();

	template<typename Component> void clearComponents();
	void clearAllComponents();

	void imgui();

protected:
	template<int = 0> void addComponents(Entity);
	template<int = 0> void removeComponents(Entity);
	bool validPointer(const ComponentPool&, void*) const;

	template<int = 0, typename... Ts> void setupIterator(std::true_type, EntityIterator<Ts...>&);
	template<int = 0, typename... Ts> void setupIterator(std::false_type, EntityIterator<Ts...>&);

	void onScriptUnloaded(ScriptUnloadedEvent*);
	void onScriptLoaded(ScriptLoadedEvent*);

protected:
	std::map< ComponentId, ComponentPool > m_pools;
	Entity m_nextFreeEntityId;
	std::size_t m_entityCount;

	struct ScriptData
	{
		// store entities 
		int m_nextEntityIndex{ 0 };
		std::vector<Entity> m_entities;
	};
	std::map<ScriptManager::Environment::Script, ScriptData> m_scriptData;

	template<typename... Ts> friend class EntityIterator;
	template<typename T> friend class ComponentPtr;
};

namespace Meta {
	template<> inline Object instanceMeta<ComponentManager>()
	{
		return Object("ComponentManager").
			func("newEntity", &ComponentManager::newEntity).
			func("removeEntity", &ComponentManager::removeEntity, { "entity" }).
			func("debugId", &ComponentManager::debugId, { "entity" });
	}

	template<> inline Object instanceMeta<Entity>()
	{
		return Object("Entity").
			func("equals", &Entity::equals, { "entity" });
	}
}

template<typename T>
class ComponentPtr
{
public:
	ComponentPtr(ComponentManager* cm, T* component = nullptr);
	ComponentPtr(const ComponentPtr<T>&);
	~ComponentPtr();

	ComponentPtr<T>& operator=(T*);
	T* get();
	T* operator*();

protected:
	int m_bufferVersion;
	Entity m_entity;
	ComponentManager::ComponentPool* m_pool;
	T* m_component;
};

#define COMPONENT_PROPERTY(type, name, ...) \
	private: type _##name { __VA_ARGS__ }; \
	public: void set##name(type value){ _##name = value; } void set##name(type& value){ _##name = value; } \
	const type& get##name() const { return _##name; } type get##name() { return _##name; }

#define COMPONENT_PROPERTY_PTR(type, name, ...) \
	private: type _##name { __VA_ARGS__ }; \
	public: void set##name(type value){ _##name = value; } \
	const type get##name() const { return _##name; } type get##name() { return _##name; }
	

template<typename Super>
struct ComponentBase
{
	Entity m_entity;
	static ComponentId componentId() { return typeid(Super).hash_code(); }
};

template<typename Super, typename System = void>
struct Component : public ComponentBase<Super>
{
	template<class T = System> static typename std::enable_if<std::is_same<T, void>::value, void>::type initSystem() { }
	template<class T = System> static typename std::enable_if<!std::is_same<T, void>::value, void>::type initSystem() { SingletonResource<T>::getSingleton(); }
};

struct NameComponent : public Component<NameComponent>
{
	typedef std::array<char, 32> NameArray;
	COMPONENT_PROPERTY(NameArray, name);
};

// ----------------------- IMPLEMENTATION ----------------------- 
template<typename T>
void ComponentManager::addComponentType(std::size_t reserve)
{
	CHECK_F(m_pools.find(T::componentId()) == m_pools.end());
	ComponentPool& pool = m_pools.insert(std::make_pair(T::componentId(), ComponentPool())).first->second;
	pool.m_accessor = &ComponentPool::BufferAccessorInstance<T>::s_instance;
	pool.m_buffer = std::vector<T>();
	pool.m_buffer.get<std::vector<T>>().reserve(reserve * sizeof(T));
}

template<typename T>
bool ComponentManager::hasComponentType() const
{
	return m_pools.find(typename T::componentId()) != m_pools.end();
}

template<typename T, typename T2, typename... Ts> 
bool ComponentManager::hasComponentType() const
{
	return hasComponentType<T>() && hasComponentType<T2, Ts...>();
}

template<typename... Components>
EntityIterator<Components...> ComponentManager::addEntity()
{
	Entity entity = newEntity();
	addComponents<Components...>(entity);

	EntityIterator<Components...> result(this, true);
	result.m_currentEntity.m_value = entity.m_value - 1; // set to the entity before and next()
	result.next();

	m_entityCount++;

	return std::move(result);
}
struct PhysicsComponent;
template<typename Component, typename... Components>
EntityIterator<Component, Components...> ComponentManager::addComponents(Entity eid)
{
	//static_assert(std::is_trivially_copyable<Component>::value, "Components must be a POD type");
	static_assert(std::is_base_of<::ComponentBase<Component>, Component>::value, "Components must inherit from Component<>");

	if (!eid)
		eid = newEntity();

	Component::initSystem();

	ComponentId cid = Component::componentId();
	ComponentPool& pool = m_pools[cid]; // make a new pool if it doesn't exist
	ResizeableMemoryPool& buffer = pool.m_buffer;
	if (!buffer)
	{
		pool.m_accessor = &ComponentPool::BufferAccessorInstance<Component>::s_instance;
		buffer = std::vector<Component>();
	}

	bool componentAdded = false;
	auto& vector = buffer.get<std::vector<Component>>();
	for(auto it = vector.begin(); it != vector.end(); ++it)
	{
		if (it->m_entity == eid)
		{
			LOG_F(WARNING, "adding component (%s) to an entity(%d) that already has it\n", typeid(Component).name(), eid);
			break;
		}
		else if (it->m_entity >= eid)
		{
			auto newComponentIt = vector.insert(it, Component{});
			newComponentIt->m_entity = eid;
			componentAdded = true;
			break;
		}
	}

	if (!componentAdded)
	{
		vector.emplace_back();
		vector.back().m_entity = eid;
	}

	addComponents<Components...>(eid);

	EntityIterator<Component, Components...> result(this, true);
	result.m_currentEntity.m_value = eid.m_value - 1; // set to the entity before and next()
	result.next();
	return std::move(result);
}

template<typename T>
ComponentPtr<T>::ComponentPtr(ComponentManager* cm, T* component):
m_bufferVersion(0),
m_entity(component ? component->m_entity : INVALID_ENTITY),
m_pool(cm->getPool<T>()),
m_component(component)
{
	m_bufferVersion = m_pool->m_version;
}

template<typename T>
ComponentPtr<T>::ComponentPtr(const ComponentPtr<T>& copy):
m_bufferVersion(copy.m_bufferVersion),
m_entity(copy.component),
m_pool(copy.m_pool),
m_component(copy.m_component)
{
}

template<typename T>
ComponentPtr<T>::~ComponentPtr()
{
}

template<typename T>
ComponentPtr<T>& ComponentPtr<T>::operator=(T* component)
{
	m_bufferVersion = m_pool->m_version;
	m_entity = component ? component->m_entity : INVALID_ENTITY;
	m_component = component;
	return *this;
}

template<typename T>
T* ComponentPtr<T>::get()
{
	return *(*this);
}

template<typename T>
T* ComponentPtr<T>::operator*()
{
	if (m_pool->m_version != m_bufferVersion)
	{
		m_component = m_pool->findComponent<T>(m_entity);
		m_bufferVersion = m_pool->m_version;
	}

	return m_component;
}

template<typename Component, typename... Components>
void ComponentManager::removeComponents(Entity eid)
{
	// does erase reallocate the buffer?
	ComponentId cid = Component::componentId();
	CHECK_F(m_pools.find(cid) != m_pools.end());

	ComponentPool& pool = m_pools[cid];
	ResizeableMemoryPool& buffer = pool.m_buffer;
	void* front = &buffer.front();
	ResizeableMemoryPool::iterator bytes = buffer.begin(), end = buffer.end();
	while (bytes != end)
	{
		Entity entity = *(Entity*)&(*bytes);
		if (entity == eid)
		{
			static_cast<Component*>(bytes)->~Component();
			buffer.erase(bytes, bytes + sizeof(Component));
			break;
		}
		else
		{
			bytes += sizeof(Component);
		}
	}

	removeComponents<Components...>(eid);

	// TODO: decrement m_entityCount if no more components
}

template<int> void ComponentManager::addComponents(Entity){}
template<int> void ComponentManager::removeComponents(Entity){}

template<typename... Components> EntityIterator<Components...> ComponentManager::findEntity(Entity e)
{
	EntityIterator<Components...> it(this, false);
	it.m_currentEntity.m_value = e.m_value - 1;
	it.next();

	return it.m_currentEntity == e ? it : EntityIterator<Components...>(this, false);
}

template<int i, typename... Ts>
void ComponentManager::setupIterator(std::true_type, EntityIterator< Ts...>& it)
{
	using ComponentType = std::tuple_element<i, std::tuple<Ts...> >::type;
	ComponentType*& component = std::get<i>(it);
	CHECK_F(component == nullptr, "component already setup");
	CHECK_F(m_pools.find(ComponentType::componentId()) != m_pools.end(), "couldn't find componentpool for %s", typeid(ComponentType).name());

	ComponentPool& pool = m_pools[ComponentType::componentId()];
	ComponentPool::BufferAccessor* accessor = pool.m_accessor;
	ResizeableMemoryPool& buffer = pool.m_buffer;
	component = accessor->empty(buffer) ? nullptr : (ComponentType*)accessor->front(buffer);

	setupIterator<i + 1>(std::integral_constant<bool, (i < sizeof...(Ts)-1)>{}, it);
}

template<int, typename... Ts> void ComponentManager::setupIterator(std::false_type, EntityIterator<Ts...>&) {}

template<typename... Components> 
EntityIterator<Components...> ComponentManager::begin()
{
	CHECK_F(hasComponentType<Components...>());
	EntityIterator<Components...> it(this);
	next(&it);
	return std::move(it);
}

template<typename... Components> 
bool ComponentManager::next(EntityIterator<Components...>* it)
{
	CHECK_F(it != nullptr);

	if (!it->m_currentEntity)
		it->m_currentEntity.m_value = 0;
	else
		it->m_currentEntity.m_value += 1;

	bool hasValidComponent = false;
	bool foundAllComponents = true;

	Entity highestEntity = it->m_currentEntity;
	auto fn = [&foundAllComponents, &hasValidComponent, &highestEntity, &it, this](EntityIteratorArgs& args)
	{
		// for each component pointer

		// dead component pointer, skip
		if (args.m_vp == nullptr)
		{
			foundAllComponents = false;
			return true;
		}

		Entity e = *(Entity*)args.m_vp;
		ComponentPool& pool = m_pools[args.m_id];
		while(e < it->m_currentEntity)
		{
			// while current pointer is lower than target entity id

			// advance the pointer by one
			args.m_vp = reinterpret_cast<char*>(args.m_vp) + args.m_size;

			// did we go beyond the buffer?
			bool valid = validPointer(pool, args.m_vp);
			hasValidComponent = hasValidComponent || valid;
			if (!valid)
			{
				// we're past the buffer now but continue onto the next components
				args.m_vp = nullptr;
				foundAllComponents = false;
				return true;
			}

			// update highestEntity
			Entity entity = *(Entity*)args.m_vp;
			highestEntity = std::max(highestEntity, entity);

			// if we're not pointing at or above the target entity, loop again
			e = *(Entity*)args.m_vp;
		}

		foundAllComponents = foundAllComponents && (e == it->m_currentEntity);
		hasValidComponent = true;
		return true;
	};
	it->accept(fn);

	if (it->m_allComponentsMustExist && !foundAllComponents && hasValidComponent)
	{
		// if we didn't find all the components but have at least one valid component,
		// try again (pushes all the not found component pointers up until it reaches the valid one)
		//it->m_currentEntity = highestEntity - 1;
		return next(it);
	}
	else
	{
		return it->m_allComponentsMustExist ? foundAllComponents && hasValidComponent : hasValidComponent;
	}
}

template<typename Component> 
ComponentManager::ComponentPool* ComponentManager::getPool()
{
	auto it = m_pools.find(Component::componentId());
	return it != m_pools.end() ? &(it->second) : nullptr;
}

template<typename Component> 
Component* ComponentManager::ComponentPool::findComponent(Entity entity)
{
	Component* component = (Component*)&m_buffer.front();
	Component* back = (Component*)&m_buffer.back();
	while (component <= back)
	{
		if (component->m_entity == entity)
			return component;

		component++;
	}
	return nullptr;
}

template<typename Component>
void ComponentManager::clearComponents()
{
	auto pool = m_pools.find(Component::componentId());
	if (pool != m_pools.end())
		pool->second.m_accessor->clear(pool->second.m_buffer);
}

// EntityIterator
template <typename... Ts>
EntityIterator<Ts...>::EntityIterator(ComponentManager* manager, bool allComponentsMustExist) :
m_manager(manager),
m_allComponentsMustExist(allComponentsMustExist),
m_currentEntity()
{
	CHECK_F(manager->hasComponentType<Ts...>());
	manager->setupIterator(std::true_type(), *this);
}

template <typename... Ts>
EntityIterator<Ts...>::~EntityIterator()
{

}

template <typename... Ts>
template <typename T>
T* EntityIterator<Ts...>::get()
{
	// TODO: static_assert T is in this tuple

	auto value = std::get<T*>(*this);
	return value && value->m_entity == m_currentEntity ? value : nullptr;
}

template <typename... Ts>
bool EntityIterator<Ts...>::next()
{
	return m_manager->next(this);
}

template<typename... Ts>
template<typename VFN>
void EntityIterator<Ts...>::accept(VFN fn)
{
	accept<0>(std::true_type(), fn);
}

template<typename... Ts>
Entity EntityIterator<Ts...>::getEntity() const
{
	return m_currentEntity;
}

template <typename... Ts>
template<std::size_t i>
bool EntityIterator<Ts...>::valid() const
{
	bool isValid = true;
	auto callback = [&isValid](EntityIteratorArgs& args) -> bool
	{
		isValid = (args.m_vp != nullptr);
		return !isValid;
	};
	const_cast< EntityIterator<Ts...>* >(this)->accept(callback);

	return isValid;
}

template <typename... Ts>
EntityIterator<Ts...>& EntityIterator<Ts...>::operator++()
{
	next();
	return *this;
}

template <typename... Ts>
EntityIterator<Ts...> EntityIterator<Ts...>::operator++(int) const
{
	EntityIterator<Ts...> it(*this);
	next();
	return it;
}

template <typename... Ts>
template<std::size_t i, typename VFN>
void EntityIterator<Ts...>::accept(std::true_type, VFN fn)
{
	auto& p = std::get<i>(*this);
	void* vp = reinterpret_cast<void*>(p);
	using ComponentType = std::tuple_element<i, std::tuple<Ts...> >::type;

	EntityIteratorArgs args = {
	/*args.m_index*/	i,
	/*args.m_id*/		ComponentType::componentId(),
	/*args.m_size*/		sizeof(ComponentType),
	/*args.m_entity*/	p ? p->m_entity : Entity(),
	/*args.m_vp*/		vp
	};
	
	if (fn(args))
		accept<i + 1>(std::integral_constant<bool, (i < sizeof...(Ts)-1)>{}, fn);

	p = reinterpret_cast< decltype(p) >(args.m_vp);
}

template <typename... Ts>
template<std::size_t i, typename VFN>
void EntityIterator<Ts...>::accept(std::false_type, VFN)
{
	// empty
}

template<typename T>
ComponentManager::ComponentPool::BufferAccessorInstance<T> ComponentManager::ComponentPool::BufferAccessorInstance<T>::s_instance;