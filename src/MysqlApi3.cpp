
#include "../include/MysqlApi3.hpp"
//MYSQL mysqlConn;
//MYSQL *mysql;
//CDataConn::MYSQL_RES *recordSet;

CDataConn::CDataConn()
    :mysqlConn{}, mysql(nullptr), recordSet(nullptr)
{
    if(mysql_library_init(0, nullptr, nullptr) != 0)
    {
        clog<<"mysql_library_init failed"<<endl;
        exit(EXIT_FAILURE);
    }
    if((mysql = mysql_init(&mysqlConn)) == nullptr)
    {
        clog<<"mysql_init failed"<<endl;
        exit(EXIT_FAILURE);
    }
}
CDataConn::CDataConn(const std::string &host,
                     const std::string &user,
                    const std::string &passwd,
                    const std::string &db,
                    unsigned int port)  // Mysql默认端口号3306
    :CDataConn()    // 委托构造函数 初始化列表中不能再初始化任何其他成员变量
{
    if(!initConn(host, user, passwd, db, port))
    {
        exit(EXIT_FAILURE);
    }
}
CDataConn::~CDataConn()
{
    exitConn();
    mysql_library_end();
}
bool CDataConn::initConn(const std::string &host,
              const std::string &user,
              const std::string &passwd,
              const std::string &db,
              unsigned int port)
{
    bool res = true;
    if(mysql_real_connect(mysql, host.c_str(), user.c_str(), passwd.c_str(), db.c_str(), port, nullptr, 0) == nullptr)
    {
        clog<<"mysql_real_connect failed to "<<db<<endl;
        clog<<mysql_errno(mysql)<<endl;
        res = false;
    }
    return res;
}
void CDataConn::exitConn()
{
    mysql_close(mysql);
    mysql = nullptr;
}
// 执行非select语句
int CDataConn::executeSql(const std::string &sql)
{
    if(sql.find("select") == 0)
    {
        clog<<"非insert drop delete...语句, select查询语句调用getRecordSet方法"<<endl;
        return -1;
    }
    //if(mysql_query(mysql, sql.c_str()))
    // mysql_real_query执行成功返回０，失败返回非０值
    if(mysql_real_query(mysql, sql.c_str(), sql.size()))
    {
        clog<<"mysql_real_query failed"<<endl;
        clog<<mysql_errno(mysql)<<endl;
        return -2;
    }
    return mysql_affected_rows(mysql);
}
// 执行select语句
int CDataConn::getRecordSet(const std::string &sql)
{
    if(sql.find("select") != 0)
    {
        clog<<"非select语句"<<endl;
        return -1;
    }
    //if(mysql_query(mysql, sql.c_str()))
    if(mysql_real_query(mysql, sql.c_str(), sql.size()))
    {
        clog<<"mysql_real_query select failed"<<endl;
        clog<<mysql_errno(mysql)<<endl;
        return -2;
    }
    //recordSet = mysql_store_result(mysql);
    return 0;
}
int CDataConn::getRecordSet(std::vector<std::vector<std::string>>& strset)
{
    strset.clear();
    recordSet = mysql_store_result(mysql);
    if(nullptr == recordSet)
    {
        clog<<"结果集为空"<<endl;
        return -1;
    }
    int rownum = mysql_num_rows(recordSet);
    int fieldnum = mysql_field_count(mysql);
    strset.resize(rownum);
    for(int i = 0; i < rownum; ++i)
    {
        strset[i].reserve(fieldnum);
        MYSQL_ROW rs = mysql_fetch_row(recordSet);
        for(int j = 0; j < fieldnum; ++j)
        {
            strset[i].emplace_back(rs[j]);
        }
    }
    return 0;
}
void CDataConn::freeRecordSet()
{
    if(recordSet != nullptr)
    {
        mysql_free_result(recordSet);
    }
    recordSet = nullptr;
}
// 执行printRecordSet之前先执行getRecordSet
void CDataConn::printRecordSet()
{
    recordSet = mysql_store_result(mysql);
    if(nullptr == recordSet) return;
    int rownum = mysql_num_rows(recordSet);
    int fieldnum = mysql_field_count(mysql);
    for(int i = 0; i < rownum; ++i)
    {
        MYSQL_ROW rs = mysql_fetch_row(recordSet);
        for(int j = 0; j < fieldnum; ++j)
        {
            cout<<rs[j]<<"\t";
        }
        cout<<endl;
    }
}
