#include "stdafx.h"
#include <string>
#include "ResourceManager.h"
#include "../Threading/ThreadPool.h"
#include "../imgui/ImGuiManager.h"
#include "../Managers/EventManager.h"
#include "../Files/FileManager.h"

NewPtr_t NewPtr;
EmptyPtr_t EmptyPtr;
NoOwnershipPtr_t NoOwnershipPtr;
TakeOwnershipPtr_t TakeOwnershipPtr;
ResourceManager* g_resourceManager = nullptr;
ResourceManager::ResourceManager():
m_resources(),
m_threadPool(EmptyPtr),
m_tasksInProgress(0),
m_autoStartTasks(false)
{
	CHECK_F(g_resourceManager == nullptr);
	g_resourceManager = this;

	m_threadPool = ResourcePtr<ThreadPool>();
}

ResourceManager::~ResourceManager()
{
	setFreeResources(true);
	while (m_tasksInProgress > 0)
		;

	while (!m_loadingTasks.empty())
	{
		m_loadingTasks.front().m_loader = nullptr;
		m_loadingTasks.pop();
	}

	m_threadPool.release();
	freeUnreferenced(true);
	for (auto& resource : m_resources)
	{
		if(!resource.m_singleton)
			LOG_F(WARNING, "Resource (%s) not freed, still has %d references\n", resource.m_debugName.c_str(), resource.m_refCount);
	}
	m_resources.clear();
}

void ResourceManager::init()
{
	ResourcePtr<EventManager> events;
	events->addListener<UpdateEvent>([this](UpdateEvent*) { update(); });
}

void ResourceManager::startLoading()
{
	auto loader = [](void* ud) {
		ResourceManager* rm = (ResourceManager*)ud;
		ResourceManager::Task task;

		while(true)
		{
			{
				std::lock_guard<std::mutex> l(rm->m_loadingTaskMutex);
				if (rm->m_loadingTasks.empty())
				{
					if (rm->m_tasksInProgress > 0)	continue;
					else							break;
				}
				task = rm->m_loadingTasks.front();
				rm->m_loadingTasks.pop();
			}

			rm->m_tasksInProgress++;

			{
				std::lock_guard<std::recursive_mutex> l(task.m_data->m_mutex);
				task.m_data->m_state = ResourceData::State::LOADING;
			}

			std::tuple<int, std::string> errors;
			Resource* resource = task.m_loader->load(&errors);
			if (resource)
			{
				// Resource Loaded
				std::lock_guard<std::recursive_mutex> l1(rm->m_resourceMutex);
				std::lock_guard<std::recursive_mutex> l2(task.m_data->m_mutex);
				if (task.m_data->m_resource)
				{
					task.m_data->m_state = ResourceData::State::LOADED; // use a specific reload state instead?
					task.m_data->m_reloadedResource = resource;
				}
				else
				{
					task.m_data->m_state = ResourceData::State::LOADED;
					task.m_data->m_resource = resource;
				}

				rm->m_notificationQueue.emplace_back(task.m_data, task.m_data->m_state, task.m_loader);
			}
			else if (std::get<int>(errors) != 0)
			{
				// Failed to load resource
				std::lock_guard<std::recursive_mutex> l1(rm->m_resourceMutex);
				std::lock_guard<std::recursive_mutex> l2(task.m_data->m_mutex);
				task.m_data->m_state = ResourceData::State::FAILED;
				task.m_data->m_error = errors;
				task.m_loader = nullptr;

				rm->m_notificationQueue.emplace_back(task.m_data, task.m_data->m_state);
			}
			else
			{
				// push back into queue
				std::lock(rm->m_loadingTaskMutex, task.m_data->m_mutex);
				std::lock_guard<std::mutex> l1(rm->m_loadingTaskMutex, std::adopt_lock_t{});
				std::lock_guard<std::recursive_mutex> l2(task.m_data->m_mutex, std::adopt_lock_t{});

				task.m_data->m_state = ResourceData::State::WAITING;
				rm->m_loadingTasks.push(task);
			}

			rm->m_tasksInProgress--;
		}
	};

	ThreadPool* pool = m_threadPool.get();
	for (std::size_t i = 0; i < pool->getThreadCount(); ++i)
	{
		pool->enqueue(loader, (void*)this);
	}
}

bool ResourceManager::isLoading() const
{
	return !m_loadingTasks.empty();
}

void ResourceManager::startReloading()
{
	if (!m_needsReload)
		return;

	m_needsReload = false;

	std::vector<ResourceData*> reloadingResources;
	for (auto it = m_resources.begin(); it != m_resources.end(); ++it)
	{
		if (it->m_reloader && it->m_reloader->m_reload > 0)
			reloadingResources.push_back(&(*it));
	}

	if (reloadingResources.empty())
		return;

	for (auto it = reloadingResources.begin(); it != reloadingResources.end(); ++it)
	{
		ResourceData* data = *it;
		std::shared_ptr<Resource::Loader> loader(data->m_reloader->createLoader());

		{
			std::lock_guard<std::mutex> l(m_loadingTaskMutex);
			m_loadingTasks.push(Task{ loader, data });
		}
	}

	if (m_autoStartTasks && m_loadingTasks.size() > 0 && m_tasksInProgress <= 0)
	{
		startLoading();
	}
}

void ResourceManager::setAutoStartTasks(bool b)
{
	m_autoStartTasks = b;
}

void ResourceManager::setFreeResources(bool b)
{
	m_freeResources = b;
	if (b)
	{
		freeUnreferenced();
	}
}

void ResourceManager::clearNotificationsFor(const Resource* resource)
{
	auto it = std::remove_if(m_notificationQueue.begin(), m_notificationQueue.end(), [=](const ResourceStateChanged& rsc) { return rsc.m_resourceData->m_resource == resource; });
	m_notificationQueue.erase(it, m_notificationQueue.end());
}

void ResourceManager::freeUnreferenced(bool freeSingleton)
{
	std::lock_guard<std::recursive_mutex> l(m_resourceMutex);

	if (m_resources.empty()) return;

	void* front = &m_resources.front();
	std::forward_list<ResourceData>::iterator it = m_resources.begin();
	std::forward_list<ResourceData>::iterator itBefore = m_resources.before_begin();
	for (; it != m_resources.end(); ++it, ++itBefore)
	{
		if (it->m_refCount <= 0 && (freeSingleton || it->m_singleton == false))
		{
			// don't delete if there's a pending notification for it
			if (std::find_if(m_notificationQueue.begin(), m_notificationQueue.end(), [=](const ResourceStateChanged& c) { return c.m_resourceData == &(*it); }) == m_notificationQueue.end())
			{
				// if you crash here, you might have a circular dependence in your ResourcePtr's
				m_resources.erase_after(itBefore);

				// the deconstructor of the thing we erased might erase more stuff, start loop back at the beginning
				freeUnreferenced(freeSingleton);
				return;
			}
		}
	}
}

void ResourceManager::release(ResourceData* data)
{
	int refCount;
	{
		std::lock_guard<std::recursive_mutex> l(data->m_mutex);
		refCount = --data->m_refCount;
	}

	if (refCount <= 0 && m_freeResources && !data->m_singleton)
	{
		freeUnreferenced();
	}
}

void ResourceManager::update()
{
	std::lock_guard<std::recursive_mutex> l(m_resourceMutex);
	for (const ResourceStateChanged& e : m_notificationQueue)
	{
		ResourcePtr<EventManager> events;
		auto* dest = events->addOneFrameEvent<ResourceStateChanged>();
		dest->m_resourceData = e.m_resourceData;
		dest->m_newState = e.m_newState;
		dest->m_reload = (e.m_resourceData->m_reloadedResource != nullptr);
		
		ResourceData* data = dest->m_resourceData;
		if (!data->m_reloader)
			data->m_reloader = e.m_loader->createReloader();

		if (data->m_reloadedResource)
		{
			data->deleteResource();
			data->m_resource = data->m_reloadedResource;
			data->m_reloadedResource = nullptr;
		}
	}
	m_notificationQueue.clear();
}

void ResourceManager::imgui()
{
	ResourcePtr<ImGuiManager> im;
	using namespace ImGui;
	bool* opened = im->win("Resources");
	if ((*opened) == false)
		return;

	if (Begin("ResourceManager", opened))
	{
		Text("Loading Tasks: %d", m_tasksInProgress.load());

		Columns(3);

		ImGui::Separator();
		ImGui::Text("Ref"); ImGui::NextColumn();
		ImGui::Text("Name"); ImGui::NextColumn();
		ImGui::Text("State"); ImGui::NextColumn();
		ImGui::Separator();

		struct Display
		{
			int m_refCount;
			const char* m_debugName;
			ResourceData::State m_state;
		};

		std::vector<Display> display;
		{
			std::lock_guard<std::recursive_mutex> l(m_resourceMutex);
			for (const ResourceData& data : m_resources)
				display.push_back({ data.m_refCount, data.m_debugName.c_str(), data.m_state });
		}

		std::sort(display.begin(), display.end(), [](const Display& d1, const Display& d2) {return d1.m_refCount > d2.m_refCount; });

		int i = 0;
		for(Display& data : display)
		{
			PushID(i);

			char refCount[4];
			sprintf_s(refCount, "%d", data.m_refCount);
			if (ImGui::Selectable(refCount, false, ImGuiSelectableFlags_SpanAllColumns))
			{
				LOG_F(INFO, "clicked %d\n", i);
			}
			
			ImGui::NextColumn();

			Text(data.m_debugName); ImGui::NextColumn();

			typedef ResourceData::State State;
			if (data.m_state == State::WAITING) { Text("Waiting");  }
			else if (data.m_state == State::LOADING) { Text("Loading"); }
			else if (data.m_state == State::LOADED) { Text("Loaded"); }
			ImGui::NextColumn();

			PopID();
			i++;
		}
		Columns(1);
	}
	End();
}

void ResourceManager::setReloadDirty()
{
	m_needsReload = true;
}

Resource::Reloader::~Reloader()
{
}

void Resource::Reloader::setReloadNeeded()
{
	m_reload++;
	g_resourceManager->setReloadDirty();
}

Resource::ReloaderWithEvent::ReloaderWithEvent()
{

}

Resource::ReloaderWithEvent::~ReloaderWithEvent()
{
	*m_deleted = true;
}

Resource::Loader* Resource::ReloaderWithEvent::createLoader() const
{
	return m_creator();
}

Resource::ReloaderOnFileChange::ReloaderOnFileChange(StringView path, const std::function<Loader*()>& creator, int priority):
m_creator(creator)
{
	m_deleted = std::make_shared<bool>(false);
	std::shared_ptr<bool> deleted = m_deleted;

	std::string pathStr = path.str();

	ResourcePtr<EventManager> events;
	events->addListener<FileChangeEvent>([this, deleted, pathStr](FileChangeEvent* e) {
		if (*deleted)
		{
			e->discardListener();
			return;
		}

		std::vector<std::string> files = e->m_files; // BUG: why does it assert if I access e->m_files.begin() directly?
		for (auto it = files.begin(); it != files.end(); ++it)
		{
			if (endsWith(pathStr, it->c_str(), it->size())) // need a better way to resolve paths
			{
				setReloadNeeded();
				return;
			}
		}
	}, priority);
}

Resource::ReloaderOnFileChange::~ReloaderOnFileChange()
{

}

Resource::Loader* Resource::ReloaderOnFileChange::createLoader() const
{
	return m_creator();
}