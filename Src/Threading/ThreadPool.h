#pragma once

#include "../Resources/ResourceManager.h"

class ThreadPool : public SingletonResource<ThreadPool>
{
public:
	ThreadPool(size_t = std::thread::hardware_concurrency() - 1);
	~ThreadPool();

	void stop();
	std::size_t getThreadCount() const;

	template<class F, class... Args>
	auto enqueue(F&& f, Args&&... args)->std::future< typename std::result_of<F(Args...)>::type >;

protected:
	std::vector< std::thread > m_workers;
	std::queue< std::function<void()> > m_tasks;

	std::mutex m_queueMutex;
	std::condition_variable m_condition;
	bool m_stop;

	std::size_t m_threadCount;
};

// add new work item to the pool
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>
{
	using return_type = typename std::result_of<F(Args...)>::type;

	auto task = std::make_shared< std::packaged_task<return_type()> >(
		std::bind(std::forward<F>(f), std::forward<Args>(args)...));

	std::future<return_type> res = task->get_future();
	{
		std::unique_lock<std::mutex> lock(m_queueMutex);

		// don't allow enqueueing after stopping the pool
		if (m_stop)
			throw std::runtime_error("enqueue on stopped ThreadPool");

		m_tasks.emplace([task]() { (*task)(); });
	}
	m_condition.notify_one();
	return res;
}