
#include <cstdint>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include "../include/Timer.hpp"
#include <iostream>

//using TimerCallback = std::function<void(void)>;
//int m_fd;
//TimerCallback m_callback;

bool Timer::settime(size_t interval) // 设置定时时间　毫秒
{
    bool res = false;
    struct itimerspec new_value = {0};
    //秒级
    new_value.it_value.tv_sec = interval / 1000;
    //纳秒级
    new_value.it_value.tv_nsec = (interval % 1000) * 1000000;
    new_value.it_interval.tv_sec = interval / 1000;
    new_value.it_interval.tv_nsec = (interval % 1000) * 1000000;
    if(timerfd_settime(m_fd, 0, &new_value, nullptr) == 0)
    {
        res = true;
    }
    return res;
}
Timer::Timer()
    :m_fd(-1), m_callback(nullptr)
{}
Timer::~Timer()
{
    deleter_timer();
    //std::cout<<"~Timer"<<std::endl;
}
Timer::Timer(Timer&& other)
            :m_fd(other.m_fd), m_callback(other.m_callback)
{
    other.m_fd = -1;
    other.m_callback = nullptr;
}
Timer& Timer::operator=(Timer&& other)
{
    if(this == &other) return *this;
    //this->deleter_timer();
    if(this->m_fd > 0) close(m_fd);
    this->m_fd = other.m_fd;
    this->m_callback = other.m_callback;
    other.m_fd = -1;
    other.m_callback = nullptr;
    return *this;
}
bool Timer::init()
{
    bool res = true;
    if(m_fd > 0) return res;
    m_fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if(m_fd < 0) res = false;
    return res;

}
bool Timer::set_timer(const TimerCallback &cb, size_t interval)
{
    bool res = false;
    if(m_fd > 0 && settime(interval))
    {
        res = true;
        m_callback = cb;
    }
    return res;
}
bool Timer::reset_timer(size_t interval)
{
    bool res = false;
    if(m_fd > 0 && m_callback != nullptr && settime(interval))
    {
        res = true;
    }
    return res;
}
bool Timer::deleter_timer()
{
    bool res = false;
    if(m_fd > 0)
    {
        close(m_fd);
        m_fd = -1;
        m_callback = nullptr;
        res = true;
    }
    return res;
}
void Timer::handle_event()
{
    //此处必须一次性读取８字节数据，不然read函数返回-1
    //并设置errno为EINVAL(22)
    //char buff[1] = {};
    //int a = read(m_fd, buff, sizeof(buff));
    //cout<<"before m_callback a: "<<a<<endl;
    //if(a != sizeof(buff))
    uint64_t val = 0;
    if(read(m_fd, &val, sizeof(val)) != sizeof(val))
    {
        return;
    }
    if(m_callback != nullptr)
    {
        //std::cout<<"Before m_callback() val: "<<val<<endl;
        m_callback();
    }
}
int Timer::get_fd() const
{
    return m_fd;
}