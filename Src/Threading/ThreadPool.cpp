#include "stdafx.h"
#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t threads):
m_stop(false),
m_threadCount(threads)
{
	for (size_t i = 0; i < threads; ++i)
	{
		m_workers.emplace_back( [this]
			{
				for (;;)
				{
					std::function<void()> task;

					{
						std::unique_lock<std::mutex> lock(this->m_queueMutex);
						m_condition.wait(lock, [this] { return m_stop || !m_tasks.empty(); });
						if (m_stop && m_tasks.empty())
							return;

						task = std::move(m_tasks.front());
						m_tasks.pop();
					}

					task();
				}
			}
		);
	}
}

ThreadPool::~ThreadPool()
{
	{
		std::unique_lock<std::mutex> lock(m_queueMutex);
		m_stop = true;
	}

	m_condition.notify_all();
	for (std::thread& worker : m_workers)
		worker.join();
}

void ThreadPool::stop()
{
	{
		std::unique_lock<std::mutex> lock(m_queueMutex);
		m_stop = true;
	}

	m_condition.notify_all();
}

std::size_t ThreadPool::getThreadCount() const
{
	return m_threadCount;
}