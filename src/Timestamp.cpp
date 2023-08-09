
#include <sys/time.h>
#include <time.h>
#include "../include/Timestamp.hpp"
#include <iostream>

//std::int64_t msec_;
Timestamp::Timestamp():msec_(0) {}
Timestamp::Timestamp(std::int64_t ms):msec_(ms)
{
    //std::cout<<"Create Timestamp"<<std::endl;
}
Timestamp::~Timestamp()
{
    //std::cout<<"~Timestamp"<<std::endl;
}
Timestamp::Timestamp(const Timestamp& t):msec_(t.msec_)
{
    //std::cout<<"Copy Timestamp"<<std::endl;
}
Timestamp& Timestamp::operator=(const Timestamp& t)
{
    if(this != &t)
    {
        this->msec_ = t.msec_;
    }
    //std::cout<<"Timestamp::operator="<<std::endl;
    return *this;
}
const Timestamp& Timestamp::now()
{
    *this = Timestamp::Now();
    return *this;
}
void Timestamp::swap(Timestamp& ts)
{
    std::swap(this->msec_, ts.msec_);
}
std::string Timestamp::toString() const
{
    char buff[BUFFLEN] = {0};
    std::int64_t seconds = msec_ / KMSP;
    std::int64_t microseconds = msec_ % KMSP;
    snprintf(buff, BUFFLEN, "%ld.%06ld", seconds, microseconds);
    return std::string(buff);
}
std::string Timestamp::toFormattedString(bool showms) const
{
    char buff[BUFFLEN] = {0};
    std::int64_t seconds = msec_ / KMSP;
    std::int64_t microseconds = msec_ % KMSP;
    struct tm tm_time;
    //二者对时区有差别，gmtime_r 0时区，　localtime_r 本地时区
    //gmtime_r(&seconds, &tm_time);
    localtime_r(&seconds, &tm_time);
    int pos = snprintf(buff, BUFFLEN, "%4d/%02d/%02d/-%02d:%02d:%02d",
                                    tm_time.tm_year + 1900,
                                    tm_time.tm_mon + 1,
                                    tm_time.tm_mday,
                                    tm_time.tm_hour,
                                    tm_time.tm_min,
                                    tm_time.tm_sec);
    if(showms)
    {
        snprintf(buff+pos, BUFFLEN-pos, ".%ldZ", microseconds);
    }
    return std::string(buff);
}
Timestamp Timestamp::Now()
{
    struct timeval tv = {};
    gettimeofday(&tv, nullptr);
    std::int64_t seconds = tv.tv_sec;
    return Timestamp(seconds*KMSP + tv.tv_usec);
}
Timestamp Timestamp::invalid()
{
    return Timestamp();
}
bool Timestamp::operator<(const Timestamp& other) const
{
    return this->msec_ < other.msec_;
}
bool Timestamp::operator==(const Timestamp& other) const
{
    return this->msec_ == other.msec_;
}
bool Timestamp::operator!=(const Timestamp& other) const
{
    return this->msec_ != other.msec_;
}
//相减，精确到秒级
std::int64_t Timestamp::operator-(const Timestamp& other) const
{
    std::int64_t diff = this->msec_ - other.msec_;
    return diff/KMSP;
}
//秒级上加一个整形数
Timestamp Timestamp::operator+(const std::int64_t seconds) const
{
    return Timestamp(this->msec_ + seconds*KMSP);
}
Timestamp Timestamp::operator+(const Timestamp& other) const
{
    return Timestamp(this->msec_ + other.msec_);
}
std::int64_t Timestamp::diffmse(const Timestamp& other) const
{
    return this->msec_ - other.msec_;
}
