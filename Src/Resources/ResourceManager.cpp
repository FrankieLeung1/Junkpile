#include "stdafx.h"
#include <string>
#include "ResourceManager.h"
#include "../Threading/ThreadPool.h"
#include "../imgui/ImGuiManager.h"

NewPtr_t NewPtr;
ResourceManager* g_resourceManager = nullptr;
ResourceManager::ResourceManager():
m_resources(),
m_threadPool(ResourcePtr<ThreadPool>::EmptyPtr{}),
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
		delete m_loadingTasks.front().m_loader;
		m_loadingTasks.pop();
	}

	m_threadPool.release();
	freeUnreferenced(true);
	for (auto& resource : m_resources)
	{
		LOG_IF_F(WARNING, !resource.m_singleton, "Resource (%s) not freed, still has %d references\n", resource);
	}
	m_resources.clear();
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
				std::lock_guard<std::mutex> l(task.m_data->m_mutex);
				task.m_data->m_state = ResourceData::State::LOADING;
			}

			std::tuple<int, std::string> errors;
			Resource* resource = task.m_loader->load(&errors);
			if (resource)
			{
				// Resource Loaded
				std::lock_guard<std::mutex> l(task.m_data->m_mutex);
				task.m_data->m_state = ResourceData::State::LOADED;
				task.m_data->m_resource = resource;
				delete task.m_loader;
			}
			else if (std::get<int>(errors) != 0)
			{
				// Failed to load resource
				std::lock_guard<std::mutex> l(task.m_data->m_mutex);
				task.m_data->m_state = ResourceData::State::FAILED;
				task.m_data->m_error = errors;
				delete task.m_loader;
			}
			else
			{
				// push back into queue
				std::lock(rm->m_loadingTaskMutex, task.m_data->m_mutex);
				std::lock_guard<std::mutex> l1(rm->m_loadingTaskMutex, std::adopt_lock_t{});
				std::lock_guard<std::mutex> l2(task.m_data->m_mutex, std::adopt_lock_t{});

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
			m_resources.erase_after(itBefore);

			// the deconstructor of the thing we erased might erase more stuff, start loop back at the beginning
			freeUnreferenced(freeSingleton);
			return;
		}
	}
}

void ResourceManager::release(ResourceData* data)
{
	int refCount;
	{
		std::lock_guard<std::mutex> l(data->m_mutex);
		refCount = --data->m_refCount;
	}

	if (refCount <= 0 && m_freeResources && !data->m_singleton)
	{
		freeUnreferenced();
	}
}

void ResourceManager::process()
{

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