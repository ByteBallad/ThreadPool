
#include "../include/DBConnPool.hpp"
using namespace std;

// KeyedObjectPool<std::string, CDataConn> connPool;
// const std::string &m_host;
// const std::string &m_user;
// const std::string &m_passwd;
// const std::string &m_db;
// unsigned int m_port;
DBConnPool::DBConnPool(const std::string &host,
           const std::string &user,
           const std::string &passwd,
           const std::string &db,
           unsigned int port)
    :connPool(16, 16, 8, 60),
    m_host(host),
    m_user(user),
    m_passwd(passwd),
    m_db(db),
    m_port(port)
{

}
// 执行非select语句
int DBConnPool::executeSql(const std::string &sql)
{
    int res = 0;
    auto pa = connPool.acquire("Execute", m_host, m_user, m_passwd, m_db, m_port);
    if(pa)
    {
        res = pa->executeSql(sql);
    }
    return res;
}
// 执行select语句
int DBConnPool::getRecordSet(const std::string &sql, std::vector<vector<std::string>> &sset)
{
    int res = 0;
    auto pa = connPool.acquire("Select", m_host, m_user, m_passwd, m_db, m_port);
    if(pa)
    {
        res = pa->getRecordSet(sql);
        if(0 == res)
        {
            res = pa->getRecordSet(sset);
            pa->freeRecordSet();
        }
    }
    return res;
}