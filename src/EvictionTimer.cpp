
#include "../include/EvictionTimer.hpp"
#include <string.h>
#include <unistd.h>
#include <thread>
#include <iostream>
using namespace std;

// int m_epollfd;
// bool m_stop = false;
// std::unordered_map<int, Timer> timers;
// std::vector<epoll_event> m_events;
// static const int init_event = 16;

EvictionTimer::EvictionTimer():m_epollfd(-1)
{
    m_events.resize(init_event);
}
EvictionTimer::~EvictionTimer()
{
    set_stop();
    //cout<<"~EvictionTime"<<endl;
}
bool EvictionTimer::init(size_t timeout)
{
    bool res = false;
    m_stop = false;
    m_epollfd = epoll_create1(EPOLL_CLOEXEC);
    if(m_epollfd > 0)
    {
        try
        {
            work_thread = std::thread(&EvictionTimer::loop, this, timeout);
            res = true;
        }
        //catch(const std::system_error& e)
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            close(m_epollfd);
        }
    }
    //work_thread = std::thread(&EvictionTimer::loop, this, timeout);
    return res;
}
void EvictionTimer::add_timer(Timer &t)
{
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = t.get_fd();
    if(epoll_ctl(m_epollfd, EPOLL_CTL_ADD, t.get_fd(), &ev) < 0)
    {
        fprintf(stderr, " epoll_ctl EPOLL_CTL_ADD failed, error=%s\n", strerror(errno));
        return;
    }
    timers[t.get_fd()] = std::move(t);
}
void EvictionTimer::remove_timer(Timer &t)
{
    epoll_ctl(m_epollfd, EPOLL_CTL_DEL, t.get_fd(), nullptr);
    timers.erase(t.get_fd());
}
//  timeout 毫秒
void EvictionTimer::loop(int timeout)
{
    while(!m_stop)
    {
        //cout<<"loop:... "<<timeout<<endl;
        int n = epoll_wait(m_epollfd, m_events.data(), m_events.size(), timeout);
        //cout<<"epoll_wait: n "<<n<<endl;
        for(int i = 0; i < n; ++i)
        {
            int fd = m_events[i].data.fd;
            std::unordered_map<int, Timer>::iterator it = timers.find(fd);
            if(it != timers.end())
            {
                //cout<<"回调函数..."<<endl;
                auto& t = it->second;
                t.handle_event();
            }
        }
        if(n >= m_events.size())
        {
            m_events.resize(m_events.size() * 2);
        }
    }
}
void EvictionTimer::set_stop()
{
    if(m_stop) return;
    m_stop = true;
    if(work_thread.joinable())
    {
        work_thread.join();
    }
    if(m_epollfd > 0)
    {
        close(m_epollfd);
    }
    m_epollfd = -1;
}
