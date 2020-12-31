#pragma once

class ThreadPool;
class EventManager;
#include "../Misc/StringView.h"

struct FileChangeEvent;
class Resource
{
protected:
	class Reloader;
	class Loader
	{
	public:
		enum Error {
			FileNotFound = 1,
			SystemError = 2
		};

	public:
		virtual ~Loader() {}
		virtual Resource* load(std::tuple<int, std::string>* error) = 0;
		virtual Reloader* createReloader() { return nullptr; }
		virtual std::string getDebugName() const { return std::string("<") + (const char*)getTypeName() + ">"; }
		virtual StringView getTypeName() const = 0;
	};
	friend class ResourceManager;
	friend struct ResourceStateChanged;

	class Reloader
	{
	public:
		virtual ~Reloader();
		void setReloadNeeded();
		virtual Loader* createLoader() const =0;

	private:
		int m_reload{ 0 };
		friend class ResourceManager;
	};

	class ReloaderWithEvent : public Reloader
	{
	public:
		template<typename Event> ReloaderWithEvent(const std::function<void(Event*, ReloaderWithEvent*)>&, const std::function<Loader*()>&, int priority = 0);
		~ReloaderWithEvent();
		Loader* createLoader() const;

	protected:
		ReloaderWithEvent();

	private:
		std::shared_ptr<bool> m_deleted;
		std::function<Loader*()> m_creator;
	};

	class ReloaderOnFileChange : public Reloader
	{
	public:
		ReloaderOnFileChange(StringView path, const std::function<Loader*()>&, int priority = 0);
		~ReloaderOnFileChange();
		Loader* createLoader() const;

	private:
		std::shared_ptr<bool> m_deleted;
		std::function<Loader* ()> m_creator;
	};

	template<typename> static Loader* createLoader() { static_assert(false, "Override Me!"); }
	static Resource* getSingleton() { return nullptr; } // prevent compile errors

	template<typename... Ts>
	static std::tuple<bool, std::size_t> getSharedHash(Ts...) { return{ false, 0 }; }

public:
	virtual ~Resource() {}

	friend struct ResourceData;
};

template<typename T>
class SingletonResource : public Resource
{
public:
	static T* getSingleton();
	static Loader* createLoader(...) { return nullptr; } // prevent compile errors
	static std::tuple<bool, std::size_t> getSharedHash();

protected:
	static std::mutex s_mutex;
};

struct ResourceData
{
	std::string m_debugName{};
	int m_refCount{ 1 };
	bool m_singleton{ false };

	bool m_owns{ true };
	Resource* m_resource{ nullptr };
	Resource* m_reloadedResource{ nullptr };

	enum class State {
		WAITING,
		LOADING,
		LOADED,
		FAILED,
		UNMANAGED // not managed by the resource manager
	};
	State m_state{ State::WAITING };
	std::size_t m_sharedHash{ 0 };
	std::tuple<int, std::string> m_error{ 0, {} };

	Resource::Reloader* m_reloader;
	
	std::recursive_mutex m_mutex; // recursive cuz loading a resource might try to get a singleton which locks the resource while searching for it

	void deleteResource()
	{
		if (m_owns)
		{
			delete m_resource;
			m_resource = nullptr;
		}
	}

	~ResourceData() { deleteResource(); delete m_reloader; }
};

struct NewPtr_t {};
struct EmptyPtr_t {};
struct NoOwnershipPtr_t {};
struct TakeOwnershipPtr_t {};

extern NewPtr_t NewPtr;
extern EmptyPtr_t EmptyPtr;
extern NoOwnershipPtr_t NoOwnershipPtr;
extern TakeOwnershipPtr_t TakeOwnershipPtr;

template<typename Resource>
class ResourcePtr
{
public:
	typedef ResourceData::State State;

public:
	ResourcePtr(EmptyPtr_t);
	ResourcePtr(NoOwnershipPtr_t, Resource*);
	ResourcePtr(TakeOwnershipPtr_t, Resource*, const char* name = "", std::size_t hash = 0);
	ResourcePtr(ResourcePtr<Resource>&&);
	ResourcePtr(const ResourcePtr<Resource>&);

	template<typename... Args> ResourcePtr(NewPtr_t = NewPtr, Args&&... args);
	~ResourcePtr();

	bool error(std::tuple<int, std::string>*) const;
	bool ready(std::tuple<int, std::string>*) const; // loading or error
	bool waitReady(std::tuple<int, std::string>*) const;

	void release();
	bool released() const;

	Resource* get() const;

	State getState() const;
	bool operator!() const;
	Resource* operator->() const;
	Resource& operator*() const;
	operator Resource*() const;

	ResourcePtr<Resource>& operator=(const ResourcePtr<Resource>&);
	ResourcePtr<Resource>& operator=(ResourcePtr<Resource>&&);
	bool operator==(const ResourceData*) const;

	static ResourcePtr<Resource> fromResourceData(ResourceData*);

protected:
	ResourceData* m_data;
	template<typename R> friend class ResourcePtr;
};

template<typename T, typename ...Ts>
bool ready(std::tuple<int, std::string>* error, const ResourcePtr<T>&, const Ts&...);
template<typename T> bool ready(std::tuple<int, std::string>* error, const ResourcePtr<T>&);

struct ResourceStateChanged;
class ResourceManager
{
public:
	ResourceManager();
	~ResourceManager();

	void init();

	void startLoading();
	bool isLoading() const;

	void startReloading();

	void setAutoStartTasks(bool);

	void update();
	
	void setFreeResources(bool);
	void freeUnreferenced(bool freeSingleton = false);

	template<typename Resource> ResourcePtr<Resource> addSingletonResource(Resource*, const char* debugName, bool owns);
	template<typename Resource> ResourcePtr<Resource> addLoadedResource(Resource*, const char* debugName, std::size_t hash = 0);

	template<typename Resource, typename... Args> ResourceData* addRef(Args&&...);
	void release(ResourceData*);

	void imgui();

protected:
	struct Task
	{
		std::shared_ptr<Resource::Loader> m_loader;
		ResourceData* m_data;
	};

	template<typename Resource>
	ResourceData* addRefSpecialized(std::true_type, std::size_t);

	template<typename Resource, typename... Args>
	ResourceData* addRefSpecialized(std::false_type, std::size_t sharedHash, Args&&...);

	void setReloadDirty();
	void clearNotificationsFor(const Resource* resource);

protected:
	std::forward_list<ResourceData> m_resources;
	std::recursive_mutex m_resourceMutex;

	std::queue<Task> m_loadingTasks;
	std::mutex m_loadingTaskMutex;
	std::vector<ResourceStateChanged> m_notificationQueue;

	ResourcePtr<ThreadPool> m_threadPool;
	std::atomic<unsigned int> m_tasksInProgress;

	bool m_autoStartTasks;

	bool m_freeResources{ true };

	bool m_needsReload{ false };

	// imgui
	int m_resourceCurrentItem{ 0 };

	friend class Resource::Reloader;
	friend class ScriptManager; // access for clearNotificationsFor()
};

extern ResourceManager* g_resourceManager;

// ----------------------- IMPLEMENTATION ----------------------- 
template<typename Resource, typename... Args>
ResourceData* ResourceManager::addRef(Args&&... args)
{
	std::tuple<bool, std::size_t> shared = typename Resource::getSharedHash(std::forward<Args>(args)...);
	if (std::get<bool>(shared) == true)
	{
		std::lock_guard<std::recursive_mutex> l(m_resourceMutex);
		for (ResourceData& data : m_resources)
		{
			std::lock_guard<std::recursive_mutex> l(data.m_mutex);
			if (data.m_sharedHash == std::get<std::size_t>(shared))
			{
				data.m_refCount++;
				return &data;
			}
		}
	}

	return addRefSpecialized<Resource>(std::is_base_of<SingletonResource<Resource>, Resource>{}, std::get<std::size_t>(shared), std::forward<Args>(args)...);
}

template<typename Resource>
ResourceData* ResourceManager::addRefSpecialized(std::true_type, std::size_t)
{
	Resource* singleton = (Resource*)Resource::getSingleton();
	auto it = std::find_if(m_resources.begin(), m_resources.end(), [=](const ResourceData& d) { return d.m_resource == singleton; });
	if (it != m_resources.end())
		return &(*it);

	ResourcePtr<Resource> ptr = addSingletonResource(singleton, typeid(Resource).name(), true);
	ResourceData* data = &m_resources.front();
	data->m_refCount++; // don't free when ptr destructs
	return data;
}

template<typename Resource, typename... Args>
ResourceData* ResourceManager::addRefSpecialized(std::false_type, std::size_t sharedHash, Args&&... args)
{
	ResourceData* data = nullptr;
	{
		std::lock_guard<std::recursive_mutex> l(m_resourceMutex);
		m_resources.emplace_front();
		data = &m_resources.front();
	}

	std::shared_ptr<Resource::Loader> loader(typename Resource::createLoader(std::forward<Args>(args)...));

	{
		std::lock_guard<std::recursive_mutex> l(data->m_mutex);
		data->m_debugName = loader->getDebugName();
		data->m_sharedHash = sharedHash;
	}

	{
		std::lock_guard<std::mutex> l(m_loadingTaskMutex);
		m_loadingTasks.push(Task{ loader, data });
	}

	if (m_autoStartTasks && m_loadingTasks.size() == 1 && m_tasksInProgress <= 0)
	{
		startLoading();
	}
	return data;
}

template<typename Resource>
ResourcePtr<Resource> ResourceManager::addSingletonResource(Resource* resource, const char* debugName, bool owns)
{
	std::lock_guard<std::recursive_mutex> l(m_resourceMutex);

	bool b;
	std::size_t hash;
	std::tie(b, hash) = Resource::getSharedHash();
	LOG_IF_F(ERROR, b == false, "Singleton Resources(%s) must have a shared hash\n", typeid(Resource).name());

	// TODO: only get hash for SAME types
	auto it = std::find_if(m_resources.begin(), m_resources.end(), [=](const ResourceData& d) {return d.m_sharedHash == hash; });
	LOG_IF_F(ERROR, it != m_resources.end(), "Multiple singleton resources (%s)", typeid(Resource).name());

	m_resources.emplace_front();
	ResourceData& data = m_resources.front();
	data.m_state = ResourceData::State::LOADED;
	data.m_sharedHash = hash;
	data.m_debugName = debugName;
	data.m_owns = owns;
	data.m_resource = resource;
	data.m_singleton = true;
	return ResourcePtr<Resource>::fromResourceData(&data);
}

template<typename Resource> 
ResourcePtr<Resource> ResourceManager::addLoadedResource(Resource* resource, const char* debugName, std::size_t hash)
{
	std::lock_guard<std::recursive_mutex> l(m_resourceMutex);

	m_resources.emplace_front();
	ResourceData& data = m_resources.front();
	data.m_state = ResourceData::State::LOADED;
	data.m_sharedHash = hash;
	data.m_debugName = debugName;
	data.m_owns = true;
	data.m_resource = resource;
	return ResourcePtr<Resource>::fromResourceData(&data);
}

template<typename Resource>
ResourcePtr<Resource>::ResourcePtr(EmptyPtr_t):
m_data(nullptr)
{

}

template<typename Resource>
ResourcePtr<Resource>::ResourcePtr(NoOwnershipPtr_t, Resource* resource):
m_data(new ResourceData())
{
	m_data->m_state = State::UNMANAGED;
	m_data->m_owns = false;
	m_data->m_refCount = 1;
	m_data->m_resource = resource;
}

template<typename Resource>
ResourcePtr<Resource>::ResourcePtr(TakeOwnershipPtr_t, Resource* resource, const char* name, std::size_t hash):
m_data(nullptr)
{
	*this = g_resourceManager->addLoadedResource(resource, name, hash);
}

template<typename Resource>
ResourcePtr<Resource>::ResourcePtr(ResourcePtr<Resource>&& m):
m_data(m.m_data)
{
	m.m_data = nullptr;
}

template<typename Resource>
ResourcePtr<Resource>::ResourcePtr(const ResourcePtr<Resource>& copy):
m_data(copy.m_data)
{
	if (m_data)
	{
		std::lock_guard<std::recursive_mutex> l(m_data->m_mutex);
		m_data->m_refCount++;
	}
}

template<typename Resource>
template<typename... Args>
ResourcePtr<Resource>::ResourcePtr(NewPtr_t, Args&&... args)
{
	CHECK_F(g_resourceManager != nullptr);
	m_data = g_resourceManager->addRef<Resource>(std::forward<Args>(args)...);
}

template<typename Resource>
ResourcePtr<Resource>::~ResourcePtr()
{
	release();
}

template<typename Resource>
bool ResourcePtr<Resource>::error(std::tuple<int, std::string>* error) const
{
	std::lock_guard<std::recursive_mutex> l(m_data->m_mutex);
	if(error)
		*error = m_data->m_error;

	return std::get<int>(m_data->m_error) != 0;
}

template<typename Resource>
bool ResourcePtr<Resource>::ready(std::tuple<int, std::string>* error) const
{
	std::lock_guard<std::recursive_mutex> l(m_data->m_mutex);
	if(std::get<int>(m_data->m_error) != 0) *error = m_data->m_error;
	return m_data->m_resource != nullptr;
}

template<typename Resource>
bool ResourcePtr<Resource>::waitReady(std::tuple<int, std::string>* error) const
{
	//TODO: prioritize resources that are being waited on

	std::tuple<int, std::string> placeholder;
	if (ready(&placeholder)) return true;
	if(!error) error = &placeholder;

	using namespace std::chrono;
	auto start = high_resolution_clock::now();
	while (!ready(error) && std::get<int>(*error) == 0)
	{
		std::this_thread::sleep_for(seconds(0));

		static bool printed = false;
		auto elapsed = high_resolution_clock::now() - start;
		if (!printed && duration_cast<std::chrono::minutes>(elapsed) > minutes(1))
		{
			LOG_F(WARNING, "waiting on resource for over a minute\n");
			printed = true;
		}
	}

	return std::get<0>(*error) == 0;
}

class FileManager;
template<typename Resource>
void ResourcePtr<Resource>::release()
{
	if (m_data)
	{
		if (m_data->m_state == State::UNMANAGED)
		{
			m_data->m_refCount--;
			if (m_data->m_refCount <= 0)
				delete m_data;
		}
		else
			g_resourceManager->release(m_data);

		m_data = nullptr;
	}
}

template<typename Resource>
bool ResourcePtr<Resource>::released() const
{
	return m_data == nullptr;
}

template<typename Resource>
Resource* ResourcePtr<Resource>::get() const
{
	waitReady(nullptr);

	std::lock_guard<std::recursive_mutex> l(m_data->m_mutex);
	return (Resource*)m_data->m_resource;
}

template<typename Resource>
typename ResourcePtr<Resource>::State ResourcePtr<Resource>::getState() const
{
	std::lock_guard<std::recursive_mutex> l(m_data->m_mutex);
	return m_data->m_state;
}

template<typename Resource>
bool ResourcePtr<Resource>::operator!() const
{
	return !m_data;
}

template<typename Resource>
Resource* ResourcePtr<Resource>::operator->() const
{
	waitReady(nullptr);

	std::lock_guard<std::recursive_mutex> l(m_data->m_mutex);
	return static_cast<Resource*>(m_data->m_resource);
}

template<typename Resource>
Resource& ResourcePtr<Resource>::operator*() const
{
	waitReady(nullptr);

	std::lock_guard<std::recursive_mutex> l(m_data->m_mutex);
	return static_cast<Resource&>(*m_data->m_resource);
}

template<typename Resource>
ResourcePtr<Resource>::operator Resource*() const
{
	waitReady(nullptr);

	std::lock_guard<std::recursive_mutex> l(m_data->m_mutex);
	return static_cast<Resource*>(m_data->m_resource);
}


template<typename Resource>
ResourcePtr<Resource>& ResourcePtr<Resource>::operator=(const ResourcePtr<Resource>& copy)
{
	release();
	m_data = copy.m_data;
	if(m_data)
	{
		std::lock_guard<std::recursive_mutex> l(m_data->m_mutex);
		m_data->m_refCount++;
	}
	return *this;
}

template<typename Resource>
ResourcePtr<Resource>& ResourcePtr<Resource>::operator=(ResourcePtr<Resource>&& moved)
{
	release();
	m_data = moved.m_data;
	moved.m_data = nullptr;
	return *this;
}

template<typename Resource>
bool ResourcePtr<Resource>::operator==(const ResourceData* data) const
{
	return m_data == data;
}

template<typename Resource>
ResourcePtr<Resource> ResourcePtr<Resource>::fromResourceData(ResourceData* data)
{
	ResourcePtr<Resource> result(EmptyPtr);
	result.m_data = data;
	return result;
}

template<typename Event>
Resource::ReloaderWithEvent::ReloaderWithEvent(const std::function<void(Event*, ReloaderWithEvent*)>& pred, const std::function<Loader*()>& creator, int priority):
m_creator(creator)
{
	m_deleted = std::make_shared<bool>(false);
	auto predPtr = std::make_shared<std::decay<decltype(pred)>::type>(pred);
	std::shared_ptr<bool> deleted = m_deleted;
	ResourcePtr<EventManager> events;
	events->addListener<Event>([this, deleted, predPtr](Event* e) {
		if (*deleted)
		{
			e->discardListener();
			return;
		}
		(*predPtr)(e, this);
	}, priority);
}

template<typename T>
std::mutex SingletonResource<T>::s_mutex;

template<typename T>
T* SingletonResource<T>::getSingleton()
{
	std::lock_guard<std::mutex> lock(s_mutex);
	static T* singleton = new T();
	return singleton;
}

template<typename T>
std::tuple<bool, std::size_t> SingletonResource<T>::getSharedHash()
{
	return { true, typeid(T).hash_code() };
}

template<typename T, typename ...Ts>
bool ready(std::tuple<int, std::string>* error, const ResourcePtr<T>& resource, const Ts&... others)
{
	return resource.ready(error) && ready(error, others...);
}

template<typename T> 
bool ready(std::tuple<int, std::string>* error, const ResourcePtr<T>& resource)
{
	return resource.ready(error);
}