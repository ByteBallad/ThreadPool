
#ifndef TIMER_HPP
#define TIMER_HPP
#include<functional>
using namespace std;

class Timer
{
public:
    using TimerCallback = std::function<void(void)>;
private:
    int m_fd;
    TimerCallback m_callback;
    bool settime(size_t interval);  //设置定时时间　毫秒
public:
    Timer();
    ~Timer();
    //避免多个对象拥有同一个文件描述符，浅拷贝和深拷贝都不行
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
    Timer(Timer&&);
    Timer& operator=(Timer&&);
    bool init();
    bool set_timer(const TimerCallback& cb, size_t interval);
    bool reset_timer(size_t interval);
    bool deleter_timer();
    void handle_event();
    int get_fd() const;
};
#endif