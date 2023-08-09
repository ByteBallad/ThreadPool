#ifndef EVICTIONTIMER_HPP
#define EVICTIONTIMER_HPP
#include "Timer.hpp"
#include <unordered_map>
#include <vector>
#include <sys/epoll.h>
#include <thread>

class EvictionTimer
{
private:
    int m_epollfd;
    bool m_stop = true;
    std::unordered_map<int, Timer> timers;
    std::vector<epoll_event> m_events;
    static const int init_event = 16;
    std::thread work_thread;
public:
    EvictionTimer();
    ~EvictionTimer();
    EvictionTimer(const EvictionTimer&) = delete;
    EvictionTimer& operator=(const EvictionTimer&) = delete;
    bool init(size_t timeout);
    void add_timer(Timer &t);
    void remove_timer(Timer &t);
    void loop(int timeout);
    void set_stop();
};
#endif