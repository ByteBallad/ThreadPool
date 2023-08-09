#ifndef TIMESTAMP_HPP
#define TIMESTAMP_HPP
#include<iostream>
#include<string>
using namespace std;
//精确到微秒级的时间戳类型
class Timestamp
{
private:
    std::int64_t msec_;
    static const int KMSP = 1000*1000;
    static const size_t BUFFLEN = 128;
public:
    Timestamp();
    explicit Timestamp(std::int64_t ms);
    ~Timestamp();
    Timestamp(const Timestamp& t);
    Timestamp& operator=(const Timestamp& t);
    const Timestamp& now();
    void swap(Timestamp& ts);
    std::string toString() const;
    std::string toFormattedString(bool showms = true) const;
public:
    static Timestamp Now();
    static Timestamp invalid();
    bool operator<(const Timestamp& other) const;
    bool operator==(const Timestamp& other) const;
    bool operator!=(const Timestamp& other) const;
    //相减，精确到秒级
    std::int64_t operator-(const Timestamp& other) const;
    //秒级上加一个整形数
    Timestamp operator+(const std::int64_t seconds) const;
    Timestamp operator+(const Timestamp& other) const;
    //相减，精确到微秒
    std::int64_t diffmse(const Timestamp& other) const;
};

#endif