
#ifndef OBJECTPOOL_HPP
#define OBJECTPOOL_HPP

#include "Timestamp.hpp"
#include "Timer.hpp"
#include "EvictionTimer.hpp"

#include <list>
#include <mutex>
#include <condition_variable>
#include <thread>
using namespace std;

template <typename TObject>
class ObjectPool
{
private:
    struct PoolObject:public TObject
    {
        template<class ...Args>
        PoolObject(Args &&...args)
            :TObject(std::forward<Args>(args)...),
            borrowed_at(Timestamp::Now())
        {}
        PoolObject(const PoolObject& obj)
            :TObject(obj), last_used(Timestamp::Now())
        {}
        Timestamp last_used;
        Timestamp borrowed_at;
        int tag;    // 1 idle; 2 active; 3 exile;
    };
private:
    size_t maxTotal;    // 池可以创建对象的最大个数
    size_t maxIdle;     // 池中的最大空闲对象个数
    size_t minIdle;     // 池中的最小空闲对象个数
    size_t maxIdleTime; // 池中空闲对象的最大空闲时间　　单位　秒
    size_t totalObject = 0; //当前对象总数
    size_t maxBorrTime = 10; // 对象最大借出时间　　单位　秒

    std::list<PoolObject*> pool;  //空闲池
    std::list<PoolObject*> actpool;  //活动池

    std::mutex mutex_;  // 互斥锁
    std::condition_variable cv_;   // 条件变量
    
    EvictionTimer evicTimer;
public:
    ObjectPool(size_t maxtotal = 16, size_t maxidle = 8, size_t minidle = 2,
                size_t maxidletime = 60)
                :maxTotal(maxtotal),
                maxIdle(maxidle),
                minIdle(minidle),
                maxIdleTime(maxidletime)
    {
        Timer timer;
        timer.init();
        timer.set_timer(std::bind(&ObjectPool::evictionLoop, this), maxIdleTime*1000);
        
        Timer ext;
        ext.init();
        ext.set_timer(std::bind(&ObjectPool::deportLoop, this), 10000);

        evicTimer.init(maxIdleTime*1000);
        evicTimer.add_timer(timer);
        evicTimer.add_timer(ext);
    }
    // 获取对象
    template <class ... Args>
    std::shared_ptr<PoolObject> acquire(Args&& ... args)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        // wait_for方法
        // 1. 先检查lambda
        // 2. 如果为假，释放锁，阻塞；如果为真，返回true
        // 3. 超时时间未到达之前，等待通知，获锁，判断lambda
        // 4. 如果为假，释放锁，阻塞；如果为真，返回true
        // 5. 超时时间到达，获锁，判断lambda
        // 6. 如果为假，返回false；如果为真，返回true
        // 注意：此处如果使用wait方法，会引起线程阻塞，进一步会导致死锁问题
        if(!cv_.wait_for(lock, std::chrono::milliseconds(1000),
            [&](){return !pool.empty() || totalObject < maxTotal;}))
        {
            return std::shared_ptr<PoolObject>(nullptr);
        }
        if(pool.empty())
        {
            ++totalObject;
            auto ptr = std::shared_ptr<PoolObject>(new PoolObject(std::forward<Args>(args)...),
                                            [&](PoolObject* p){release(p);});
            ptr->tag = 2;
            actpool.push_back(ptr.get());
            return ptr;
        }
        else
        {
            auto ptr = pool.front();
            pool.pop_front();
            ptr->borrowed_at = Timestamp::Now();
            ptr->tag = 2;
            actpool.push_back(ptr);
            return std::shared_ptr<PoolObject>(ptr, [&](PoolObject* p){release(p);});
        }
    }
    // 获取当前空闲对象个数
    size_t getIdleObjSize() const
    {
        return pool.size();
    }
    // 获取当前总的对象个数
    size_t getTotalObjSize() const
    {
        return totalObject;
    }
    // 获取当前活动对像个数
    size_t getActivateObjSize() const
    {
        return totalObject - pool.size();
    }
    void clear()
    {
        evicTimer.set_stop();
        for(auto& pObj:pool)
        {
            delete pObj;
        }
        pool.clear();
    }
    ~ObjectPool()
    {
        clear();
    }
private:
    void release(PoolObject* ptr)  // 借出对象的回收
    {
        std::unique_lock<mutex> lock(mutex_);
        if(ptr->tag == 3)
        {
            actpool.remove(ptr);
            delete ptr;
            cout<<"对象借出过久...放逐"<<endl;
        }
        else if(pool.size() < maxIdle)
        {
            ptr->last_used = Timestamp::Now();
            ptr->tag = 1;
            pool.push_back(ptr);
        }
        else
        {
            cout<<"对象池满了...释放对象"<<endl;
            --totalObject;
            delete ptr;
        }
        cv_.notify_one();
    }
    void deportLoop()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if(actpool.size() <= 0) return;
        auto it = actpool.begin();
        for(;it != actpool.end(); ++it)
        {
            //if((*it)->tag == 3) continue;
            auto actTime = Timestamp::Now() - (*it)->borrowed_at;
            if((*it)->tag != 3 && actTime > maxBorrTime)
            {
                (*it)->tag = 3;
                --totalObject;
                cout<<"标记对象为放逐状态"<<endl;
            }
        }
        cv_.notify_one();
    }
    void evictionLoop()
    {
        std::unique_lock<mutex> lock(mutex_);
        if(pool.size() <= minIdle) return;
        int num = pool.size() - minIdle;
        num = num > 3 ? 3 : num;
        auto it = pool.begin();
        for(; it != pool.end();)
        {
            auto idleTime = Timestamp::Now() - (*it)->last_used;
            if(idleTime >= maxIdleTime)
            {
                cout<<"超时 ... 驱逐对象"<<endl;
                --totalObject;
                delete (*it);
                pool.erase(it++);
                if(--num == 0) break;
            }
            else
            {
                ++it;
            }
        }
    }
};
#endif