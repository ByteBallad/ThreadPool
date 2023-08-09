
#ifndef DBCONNPOOL_HPP
#define DBCONNPOOL_HPP

#include "keyedObjectPool.hpp"
#include "MysqlApi3.hpp"

#include <mysql/mysql.h>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

class DBConnPool
{
private:
    KeyedObjectPool<std::string, CDataConn> connPool;
    const std::string &m_host;
    const std::string &m_user;
    const std::string &m_passwd;
    const std::string &m_db;
    unsigned int m_port;
public:
    DBConnPool(const std::string &host,
               const std::string &user,
               const std::string &passwd,
               const std::string &db,
               unsigned int port);
    //执行非select语句
    int executeSql(const std::string &sql);
    //执行select语句
    int getRecordSet(const std::string &sql, std::vector<vector<std::string>> &sset);

};
#endif