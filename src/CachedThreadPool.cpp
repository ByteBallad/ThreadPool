
#include <thread>
#include <mutex>
#include <condition_variable>
#include <list>
#include <functional>
#include <iostream>
#include <future>
#include <unordered_map>
using namespace std;

//const int MaxTaskCount = 200;
const int MaxTaskCount = 2;		// 同步队列中最大任务数（设置为2秒测试代码）
const int KeepAliveTime = 10;	// 线程最大空闲时间 60秒（设置为10秒测试代码）

template<class T>
class SyncQueue
{
private:
	std::list<T> m_queue;
	mutable std::mutex m_mutex;
	std::condition_variable m_notEmpty;	// 对应消费者
	std::condition_variable m_notFull;	// 对应生产者
	int m_maxSize;		// 同步队列上限
	bool m_needStop;	// true 同步队列停止工作

	bool isFull() const
	{
		bool full = m_queue.size() >= m_maxSize;
		if (full)
		{
			cout << "m_queue已经满了，需要等待..." << endl;
		}
		return full;
	}
	bool isEmpty() const
	{
		bool empty = m_queue.empty();
		if (empty)
		{
			cout << "m_queue已经空了，需要等待..." << endl;
		}
		return empty;
	}

	/*template<class F>
	void Add(F&& task)
	{
		std::unique_lock<std::mutex> locker(m_mutex);
		m_notFull.wait(locker, [this]()->bool {return m_needStop || !isFull(); });
		if (m_needStop)
		{
			return;
		}
		m_queue.push_back(std::forward<F>(task));
		m_notEmpty.notify_all();
	}*/

	// return 0; 成功
	// return 1; full
	// return 2; stop
	template<class F>
	int Add(F&& task)
	{
		std::unique_lock<std::mutex> locker(m_mutex);
		if (!m_notFull.wait_for(locker, std::chrono::seconds(1),
			[this]()->bool {return m_needStop || !isFull(); }))
		{
			return 1;
		}
		if (m_needStop)
		{
			return 2;
		}
		m_queue.push_back(std::forward<F>(task));
		m_notEmpty.notify_all();
		return 0;
	}
public:
	SyncQueue(int maxSize = 10)
		:m_maxSize(maxSize), m_needStop(false)
	{}
	~SyncQueue()
	{}
	int Put(const T& task)
	{
		return Add(task);
	}
	int Put(T&& task)
	{
		return Add(std::forward<T>(task));
	}
	void Take(std::list<T>& list)
	{
		std::unique_lock<std::mutex> locker(m_mutex);
		m_notEmpty.wait(locker, [this]()->bool {return m_needStop || !isEmpty(); });
		if (m_needStop)
		{
			cout << "同步队列停止工作" << endl;
			return;
		}
		list = std::move(m_queue);
		m_notFull.notify_all();
	}
	// return 0; 成功
	// return 1; empty
	// return 2; stop
	int Take(T& task)
	{
		std::unique_lock<std::mutex> locker(m_mutex);
		//m_notEmpty.wait(locker, [this]()->bool {return m_needStop || !isEmpty(); });
		if (!m_notEmpty.wait_for(locker, std::chrono::seconds(1), [this] {return m_needStop || !isEmpty(); }))
		{
			return 1;
		}
		if (m_needStop)
		{
			cout << "同步队列停止工作" << endl;
			return 2;
		}
		task = m_queue.front();
		m_queue.pop_front();
		m_notFull.notify_all();
		return 0;
	}
	void Stop()
	{
		{
			std::unique_lock<std::mutex> locker(m_mutex);
			m_needStop = true;
		}
		m_notEmpty.notify_all();
		m_notFull.notify_all();
	}
	bool Empty() const
	{
		std::unique_lock<std::mutex> locker(m_mutex);
		return m_queue.empty();
	}
	bool Full() const
	{
		std::unique_lock<std::mutex> locker(m_mutex);
		return m_queue.size() >= m_maxSize;
	}
	size_t Size() const
	{
		std::unique_lock<std::mutex> locker(m_mutex);
		return m_queue.size();
	}
	size_t Count() const
	{
		return m_queue.size();
	}
};

class CachedThreadPool
{
public:
	using Task = std::function<void(void)>;
private:
	//std::list<std::shared_ptr<std::thread>> m_threadGroup;
	std::unordered_map<std::thread::id, std::shared_ptr<std::thread>> m_threadGroup;
	int m_coreThreadSize;	// 线程数量下限 2
	int m_maxThreadSize;	// 线程数量上限
	std::atomic<int> m_idleThreadSize;	// 空闲线程数量
	std::atomic<int> m_curThreadSize;	// 当前线程数量
	std::mutex m_mutex;
	SyncQueue<Task> m_queue;
	std::atomic_bool m_running;	//true; false stop;
	std::once_flag m_flag;

	void start(int numthreads)
	{
		m_running = true;
		m_curThreadSize = numthreads;
		for (int i = 0; i < numthreads; ++i)
		{
			//m_threadGroup.push_back(std::make_shared<std::thread>(&FixedThreadPool::runInThread, this));
			auto tha = std::make_shared<std::thread>(std::thread(std::bind(&CachedThreadPool::runInThread, this)));
			std::thread::id tid = tha->get_id();
			m_threadGroup.emplace(tid, std::move(tha));
			m_idleThreadSize++;
		}
	}
	void runInThread()
	{
		auto tid = std::this_thread::get_id();
		auto startTime = std::chrono::high_resolution_clock().now();
		while (m_running)
		{
			Task task;
			if (m_queue.Size() == 0)
			{
				auto now = std::chrono::high_resolution_clock().now();
				auto intervalTime = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);
				std::lock_guard<std::mutex> lock(m_mutex);
				if (intervalTime.count() >= KeepAliveTime && m_curThreadSize > m_coreThreadSize)
				{
					m_threadGroup.find(tid)->second->detach();
					m_threadGroup.erase(tid);
					m_curThreadSize--;
					m_idleThreadSize--;
                    cout << "++++++++++++++++++" << endl;
					cout << "空闲线程销毁" << endl;
                    cout << "当前线程数量 " << m_curThreadSize << endl;
                    cout << "最小线程数量 " << m_coreThreadSize << endl;
                    cout << "++++++++++++++++++" << endl;
					return;
				}
                //std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
			}
			if (m_queue.Take(task) == 0 && m_running)
			{
				m_idleThreadSize--;
				task();
				m_idleThreadSize++;
				startTime = std::chrono::high_resolution_clock().now();
			}
		}
	}
	void stopThreadGroup()
	{
		m_queue.Stop();
		m_running = false;
		for (auto& tha : m_threadGroup)
		{
			if (tha.second->joinable())
			{
				tha.second->join();
			}
		}
		m_threadGroup.clear();
	}
public:
	CachedThreadPool(int numthreads, int taskPoolSize = MaxTaskCount)
		:m_coreThreadSize(numthreads),
		m_maxThreadSize(2*std::thread::hardware_concurrency()+1),
		m_idleThreadSize(0),
		m_curThreadSize(0),
		m_queue(taskPoolSize),
		m_running(false)
	{
		start(m_coreThreadSize);
	}
	~CachedThreadPool()
	{
		stop();
	}
	void stop()
	{
		std::call_once(m_flag, [this]()->void {stopThreadGroup(); });
	}

	/*void addTask(Task&& task)
	{
		if (m_queue.Put(std::forward<Task>(task)) != 0)
		{
			std::cerr << "task queue is full, add task fail." << std::endl;
			task();
		}
	}
	void addTask(const Task& task)
	{
		if (m_queue.Put(task) != 0)
		{
			std::cerr << "task queue is full, add task fail." << std::endl;
			task();
		}
	}*/

	template<class Func, class ...Args>
	auto addTask(Func&& func, Args&& ...args)->std::future<decltype(func(args...))>
	{
		using RetType = decltype(func(args...));
		//该方式将task放入到同步队列(Put)之后，函数结束，task会被销毁
		/*std::packaged_task<RetType()> task(std::bind(
										std::forward<Func>(func),
										std::forward<Args>(args)...)
		);
		std::future<RetType> result = task.get_future();*/
		//采用智能指针方式
		auto task = std::make_shared<std::packaged_task<RetType()>>(
			std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
		std::future<RetType> result = task->get_future();
		if (m_queue.Put([task] {(*task)(); }) != 0)
		{
			cout << "调用者运行策略" << endl;
			(*task)();
		}
		if (m_idleThreadSize <= 0 && m_curThreadSize < m_maxThreadSize)
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			auto tha = std::make_shared<std::thread>(std::thread(std::bind(&CachedThreadPool::runInThread, this)));
			std::thread::id tid = tha->get_id();
			m_threadGroup.emplace(tid, std::move(tha));
			m_idleThreadSize++;
			m_curThreadSize++;
		}
		return result;
	}
};



int add(int a, int b, int s)
{
	std::this_thread::sleep_for(std::chrono::seconds(s));
	int c = a + b;
	cout << "add begin ..." << endl;
	return c;
}

void add_a(CachedThreadPool& pool)
{
	auto r = pool.addTask(add, 10, 20, 8);
	cout << "add_a: " << r.get() << endl;
}

void add_b(CachedThreadPool& pool)
{
	auto r = pool.addTask(add, 15, 20, 8);
	cout << "add_b: " << r.get() << endl;
}

void add_c(CachedThreadPool& pool)
{
	auto r = pool.addTask(add, 20, 30, 6);
	cout << "add_c: " << r.get() << endl;
}

void add_d(CachedThreadPool& pool)
{
	auto r = pool.addTask(add, 40, 50, 9);
	cout << "add_d: " << r.get() << endl;
}

int main()
{
	CachedThreadPool pool(2);
	std::thread tha(add_a, std::ref(pool));
	std::thread thb(add_b, std::ref(pool));
	std::thread thc(add_c, std::ref(pool));
	std::thread thd(add_d, std::ref(pool));
	std::thread thz(add_d, std::ref(pool));

	tha.join();
	thb.join();
	thc.join();
	thd.join();
	thz.join();

	std::this_thread::sleep_for(std::chrono::seconds(20));

	std::thread the(add_a, std::ref(pool));
	std::thread thf(add_b, std::ref(pool));

	the.join();
	thf.join();
	return 0;
}

#if 0
int* m_malloc(int size)
{
	int* p = (int*)malloc(size);
	return p;
}

void my_free(int* p)
{
	free(p);
}

int Add(int a, int b)
{
	cout << "Add begin ..." << endl;
	std::this_thread::sleep_for(std::chrono::seconds(2));
	int c = a + b;
	std::this_thread::sleep_for(std::chrono::seconds(1));
	cout << "Add end ..." << endl;
	return c;
}

void add_a(FixedThreadPool& pool)
{
	auto rx = pool.addTask(Add, 10, 20);
	cout << rx.get() << endl;
}

void add_b(FixedThreadPool& pool)
{
	auto rx = pool.addTask(Add, 20, 30);
	cout << rx.get() << endl;
}

void add_c(FixedThreadPool& pool)
{
	int n = 10;
	auto px = pool.addTask(m_malloc, sizeof(int) * n);
	int* ip = px.get();
	for (int i = 0; i < n; ++i) { ip[i] = i; }
	for (int i = 0; i < n; ++i) { cout << ip[i] << " "; }
	cout << endl;
	pool.addTask(my_free, ip);
}

int main()
{
	FixedThreadPool pool;
	std::thread tha(add_a, std::ref(pool));
	std::thread thb(add_b, std::ref(pool));
	std::thread thc(add_c, std::ref(pool));

	tha.join();
	thb.join();
	thc.join();
	return 0;
}
#endif