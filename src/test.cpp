
#include "../include/DBConnPool.hpp"
#include "../include/Timestamp.hpp"

#include <thread>
using namespace std;

const int len = 128;
const int n = 10000;

void funa(DBConnPool &pool)
{
    char buff[len] = {};
    for(int i = 0; i < n; ++i)
    {
        sprintf(buff, "insert into student(sid, sname, ssex, sage) values('A0A00%d', 'lijian', '%s', %d)", i+1, (rand() % 2 ? "man":"woman"), rand()%10+15);
        pool.executeSql(buff);
    }
}

void funb(DBConnPool &pool)
{
    char buff[128] = {};
    for(int i = 0; i < n; ++i)
    {
        sprintf(buff, "insert into student(sid, sname, ssex, sage) values('B0B00%d', '李建', '%s', %d)", i+1, (rand() % 2 ? "man":"woman"), rand()%10+15);
        pool.executeSql(buff);
    }
}

void func(DBConnPool &pool)
{
    char buff[128] = {};
    for(int i = 0; i < n; ++i)
    {
        sprintf(buff, "insert into student(sid, sname, ssex, sage) values('C0C00%d', 'hello', '%s', %d)", i+1, (rand() % 2 ? "男":"女"), rand()%10+15);
        pool.executeSql(buff);
    }
}

void fund(DBConnPool &pool)
{
    char buff[128] = {};
    for(int i = 0; i < n; ++i)
    {
        sprintf(buff, "insert into student(sid, sname, ssex, sage) values('D0D00%d', 'world', '%s', %d)", i+1, (rand() % 2 ? "男":"女"), rand()%10+15);
        pool.executeSql(buff);
    }
}

void fune(DBConnPool &pool)
{
    char buff[128] = {};
    for(int i = 0; i < n; ++i)
    {
        sprintf(buff, "insert into student(sid, sname, ssex, sage) values('E0E00%d', 'newdata', '%s', %d)", i+1, (rand() % 2 ? "男":"女"), rand()%10+15);
        pool.executeSql(buff);
    }
}

int main()
{
    DBConnPool connPool("localhost", "root", "135799", "Student", 3306);

    Timestamp begin;
    begin.now();

    std::thread tha(funa, std::ref(connPool));
    std::thread thb(funb, std::ref(connPool));
    std::thread thc(func, std::ref(connPool));
    std::thread thd(fund, std::ref(connPool));
    std::thread the(fune, std::ref(connPool));

    tha.join();
    thb.join();
    thc.join();
    thd.join();
    the.join();

    auto sec = Timestamp::Now() - begin;
    cout<<"sec: "<<sec<<endl;

    return 0;
}


#if 0
#include "../include/MysqlApi3.hpp"

#include <string>
#include <iostream>
using namespace std;

int main()
{
    CDataConn dbConn;
    const int LEN = 256;
    char buff[256] = {};
    if(!dbConn.initConn("localhost", "root", "135799", "Student", 3306))
    {
        clog<<"initConn failed"<<endl;
        return 1;
    }
    std::vector<std::vector<std::string>> dbset;
    sprintf(buff, "select * from student");
    if(dbConn.getRecordSet(buff) == 0)
    {
        //dbConn.printRecordSet();
        dbConn.getRecordSet(dbset);
        dbConn.freeRecordSet();
    }
    int nrow = dbset.size();
    for(int i = 0; i < nrow; ++i)
    {
        int nfield = dbset[i].size();
        for(int j = 0; j < nfield; ++j)
        {
            cout<<dbset[i][j]<<"\t";
        }
        cout<<endl;
    }
    //执行一次mysql_real_query或mysql_query后，执行一次mysql_store_result可以获得结果集
    //多次执行mysql_store_result会返回nullptr
    //int value = dbConn.getRecordSet(sset);
    //cout<<"第二次获取结果集返回值: "<<value<<endl;
    return 0;
}

int main()
{
    CDataConn dbConn;
    const int LEN = 256;
    char buff[256] = {};
    if(!dbConn.initConn("localhost", "root", "135799", "Student", 3306))
    {
        clog<<"initConn failed"<<endl;
        return 1;
    }
    for(int i = 0; i < 10; ++i)
    {
        sprintf(buff, "insert into student(sid, sname, ssex, sage) values('0900%d', 'lijian', '%s', %d)", i, (rand()%2 ? "man":"woman"), rand()%10+15);
        cout<<buff<<endl;
        dbConn.executeSql(buff);
    }


    return 0;
}

#include "../include/Timestamp.hpp"
#include "../include/Timer.hpp"
#include "../include/EvictionTimer.hpp"
#include "../include/keyedObjectPool.hpp"

#include <thread>
#include <iostream>
#include <map>
#include <list>
using namespace std;

class TcpConn
{
    std::string ipaddr;
    short port;
public:
    TcpConn(const std::string ip = "nullptr", short p = 0)
        :ipaddr(ip), port(p)
    {
        cout<<"Connect ip : port  "<<ipaddr<<" : "<<port<<endl;
    }
    void init(const std::string ip = "nullptr", short p = 0)
    {
        ipaddr = ip;
        port = p;
        cout<<"Connect ..."<<endl;
        std::this_thread::sleep_for(chrono::seconds(rand()%20));
    }
    bool is_conn() const {return true;}
};

int main()
{
    KeyedObjectPool<std::string, TcpConn> objectPool(8,6,2,5);
    {
        auto spa = objectPool.acquire("192.168.12.11:8900");
        if(spa) spa->init("192.168.12.11", 8900);
        auto spb = objectPool.acquire("192.168.12.11:8900");
        if(spb) spb->init("192.168.12.11", 8900);
        auto spc = objectPool.acquire("192.168.22.23:8900");
        if(spc) spc->init("192.168.22.23", 8900);
        auto spd = objectPool.acquire("192.168.22.23:8900");
        if(spd) spd->init("192.168.22.23", 8900);
    }
    auto pa = objectPool.acquire("192.168.22.23:8900");
    if(pa && !pa->is_conn())
    {
        pa->init("192.168.22.23", 8900);
    }
    cout<<"Total Object size:        "<<objectPool.getTotalObjSize()<<endl;
    cout<<"Activate Object size:     "<<objectPool.getActivateObjSize()<<endl;
    cout<<"Idle Object (pool) size:  "<<objectPool.getIdleObjSize()<<endl;
    // 等待一段时间，让驱逐策略生效
    std::this_thread::sleep_for(std::chrono::seconds(6));
    cout<<"Total Object size:        "<<objectPool.getTotalObjSize()<<endl;
    cout<<"Activate Object size:     "<<objectPool.getActivateObjSize()<<endl;
    cout<<"Idle Object (pool) size:  "<<objectPool.getIdleObjSize()<<endl;
    return 0;
}

int main()
{
    std::map<int, std::list<std::string>> islistmap;
    islistmap[5].push_back(std::string("tulun"));
    islistmap[5].push_back(std::string("lijian"));
    islistmap[7].push_back(std::string("newdata"));
    cout<<"islistmap size: "<<islistmap.size()<<endl;
    return 0;
}

class Point
{
private:
    float _x;
    float _y;
public:
    Point():_x(0.0), _y(0.0) {cout<<"Create Point()"<<endl;}
    Point(float x, float y):_x(x), _y(y) {cout<<"Create Point(float,float)"<<endl;}
    Point(const Point&) = default;
    Point& operator=(const Point&) = default;
    ~Point() {cout<<"Destroy Point"<<endl;}
    float& pointX() {return _x;}
    const float& pointX() const {return _x;}
    float& pointY() {return _y;}
    const float& pointY() const {return _y;}
    void printpoint() const
    {
        cout<<_x<<" : "<<_y<<endl;
    }
};

void funa(ObjectPool<Point>& objectPool)
{
    auto obj1 = objectPool.acquire(1, 2);
    std::this_thread::sleep_for(std::chrono::seconds(rand()%10));
    if(obj1) (obj1)->printpoint();
    auto obj2 = objectPool.acquire(3, 4);
    std::this_thread::sleep_for(std::chrono::seconds(rand()%10));
    if(obj2) obj2->printpoint();
    {
        auto obj3 = objectPool.acquire(3, 4);
        std::this_thread::sleep_for(std::chrono::seconds(rand()%10));
        if(obj3) obj3->printpoint();
        auto obj4 = objectPool.acquire(3, 4);
        std::this_thread::sleep_for(std::chrono::seconds(rand()%10));
        if(obj4) obj4->printpoint();
    }
    cout<<"Total Object size:        "<<objectPool.getTotalObjSize()<<endl;
    cout<<"Activate Object size:     "<<objectPool.getActivateObjSize()<<endl;
    cout<<"Idle Object (pool) size:  "<<objectPool.getIdleObjSize()<<endl;
}

void funb(ObjectPool<Point>& objectPool)
{
    auto obj1 = objectPool.acquire(1, 2);
    std::this_thread::sleep_for(std::chrono::seconds(rand()%10));
    if(obj1) obj1->printpoint();
    {
        auto obj2 = objectPool.acquire(3, 4);
        std::this_thread::sleep_for(std::chrono::seconds(rand()%10));
        if(obj2) obj2->printpoint();
        auto obj3 = objectPool.acquire(3, 4);
        std::this_thread::sleep_for(std::chrono::seconds(rand()%10));
        if(obj3) obj3->printpoint();
    }
    auto obj4 = objectPool.acquire(3, 4);
    std::this_thread::sleep_for(std::chrono::seconds(rand()%10));
    if(obj4) obj4->printpoint();
    cout<<"Total Object size:        "<<objectPool.getTotalObjSize()<<endl;
    cout<<"Activate Object size:     "<<objectPool.getActivateObjSize()<<endl;
    cout<<"Idle Object (pool) size:  "<<objectPool.getIdleObjSize()<<endl;
}

int main()
{
    ObjectPool<Point> objectPool(6, 4, 2, 5);

    std::thread tha(funa, std::ref(objectPool));
    std::thread thb(funb, std::ref(objectPool));

    tha.join();
    thb.join();
    cout<<"Total Object size:        "<<objectPool.getTotalObjSize()<<endl;
    cout<<"Activate Object size:     "<<objectPool.getActivateObjSize()<<endl;
    cout<<"Idle Object (pool) size:  "<<objectPool.getIdleObjSize()<<endl;
    // 等待一段时间，让驱逐策略生效
    std::this_thread::sleep_for(std::chrono::seconds(6));

    cout<<"Total Object size:        "<<objectPool.getTotalObjSize()<<endl;
    cout<<"Activate Object size:     "<<objectPool.getActivateObjSize()<<endl;
    cout<<"Idle Object (pool) size:  "<<objectPool.getIdleObjSize()<<endl;
    return 0;
}

void funa()
{
    static int num = 0;
    cout<<"funa 2s "<<++num<<endl;
}

void funb()
{
    static int num = 0;
    cout<<"funb 6s "<<++num<<endl;
}

int main()
{
    EvictionTimer evicTimer;
    {
        Timer t1, t2;
        t1.init();
        t2.init();
        t1.set_timer(funa, 2000);
        t2.set_timer(funb, 6000);

        evicTimer.init(1000);
        evicTimer.add_timer(t1);
        evicTimer.add_timer(t2);
    }
    std::this_thread::sleep_for(std::chrono::seconds(60));
    //evicTimer.set_stop();
}

void funa()
{
    static int num = 0;
    cout<<"funa num: "<<++num<<endl;
}

void funb()
{
    static int num = 100;
    cout<<"funb num: "<<--num<<endl;
}

int main()
{
    Timer t1, t2;
    EvictionTimer evictime;
    t1.init();
    t1.set_timer(funa, 5000);
    t2.init();
    t2.set_timer(funb, 2000);
    evictime.init();
    evictime.add_timer(t1);
    evictime.add_timer(t2);
    evictime.loop(1000);
}

int main()
{
    Timestamp tp;
    cout<<tp.toString()<<endl;
    cout<<tp.toFormattedString()<<endl;
    tp.now();
    cout<<tp.toString()<<endl;
    cout<<tp.toFormattedString()<<endl;
    tp = tp + 120;
    cout<<tp.toString()<<endl;
    cout<<tp.toFormattedString()<<endl;
    return 0;
}
#endif