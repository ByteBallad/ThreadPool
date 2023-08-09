
#include <thread>
#include <mutex>
#include <condition_variable>
#include <list>
#include <functional>
#include <iostream>
#include <future>
using namespace std;

const int MaxTaskCount = 200;

template<class T>
class SyncQueue
{
private:
	std::list<T> m_queue;
	std::mutex m_mutex;
	std::condition_variable m_notEmpty;	//对应消费者
	std::condition_variable m_notFull;	//对应生产者
	int m_maxSize;		//同步队列上限
	bool m_needStop;	//true 同步队列停止工作

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
#if 0
	template<class F>
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
	}
#endif
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
	SyncQueue(int maxSize)
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
			return;
		}
		list = std::move(m_queue);
		m_notFull.notify_all();
	}
	void Take(T& task)
	{
		std::unique_lock<std::mutex> locker(m_mutex);
		m_notEmpty.wait(locker, [this]()->bool {return m_needStop || !isEmpty(); });
		if (m_needStop)
		{
			return;
		}
		task = m_queue.front();
		m_queue.pop_front();
		m_notFull.notify_all();
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

class FixedThreadPool
{
public:
	using Task = std::function<void(void)>;
private:
	std::list<std::shared_ptr<std::thread>> m_threadGroup;
	SyncQueue<Task> m_queue;
	std::atomic_bool m_running;	//true; false stop;
	std::once_flag m_flag;

	void start(int numthreads)
	{
		m_running = true;
		for (int i = 0; i < numthreads; ++i)
		{
			m_threadGroup.push_back(std::make_shared<std::thread>(&FixedThreadPool::runInThread, this));
		}
	}
	void runInThread()
	{
		while (m_running)
		{
			Task task;
			m_queue.Take(task);
			if (task && m_running)
			{
				task();
			}
		}
	}
	void stopThreadGroup()
	{
		m_queue.Stop();
		m_running = false;
		for (auto& tha : m_threadGroup)
		{
			if (tha && tha->joinable())
			{
				tha->join();
			}
		}
		m_threadGroup.clear();
	}
public:
	FixedThreadPool(int numthreads = std::thread::hardware_concurrency())
		:m_queue(MaxTaskCount), m_running(false)
	{
		start(numthreads);
	}
	~FixedThreadPool()
	{
		stop();
	}
	void stop()
	{
		std::call_once(m_flag, [this]()->void {stopThreadGroup(); });
	}
#if 0
	void addTask(Task&& task)
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
	}
#endif
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
		if (m_queue.Put([task] {(*task)();}) != 0)
		{
			(*task)();
		}
		return result;
	}
};

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