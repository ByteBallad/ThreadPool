
#ifndef MYSQLAPI3_HPP
#define MYSQLAPI3_HPP

#include <mysql/mysql.h>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

class CDataConn
{
private:
    MYSQL mysqlConn;
    MYSQL* mysql;
    MYSQL_RES* recordSet;
public:
    CDataConn();
    CDataConn(const std::string& host,
              const std::string& user,
              const std::string& passwd,
              const std::string& db,
              unsigned int port);
    ~CDataConn();
    CDataConn(const CDataConn&) = delete;
    CDataConn& operator=(const CDataConn&) = delete;
public:
    bool initConn(const std::string& host,
                  const std::string& user,
                  const std::string& passwd,
                  const std::string& db,
                  unsigned int port);
    void exitConn();
    // executeSql方法和getRecordSet方法调用的都是mysql_real_query方法
    // 前者返回影响的行数，后者返回状态码：0 表示正常执行
    // 执行非select语句
    int executeSql(const std::string& sql);
    // 执行select语句
    int getRecordSet(const std::string& sql);
    int getRecordSet(std::vector<std::vector<std::string>>& strset);
    void freeRecordSet();
    void printRecordSet();
};

#endif