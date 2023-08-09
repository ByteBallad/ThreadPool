
#include <mysql/mysql.h>
#include <iostream>
using namespace std;

int main()
{
    MYSQL mysqlconn;
    MYSQL* mysql = mysql_init(&mysqlconn);
    //MYSQL* mysql = mysql_init(nullptr);
    if(nullptr == mysql)
    {
        cout<<"mysql init error"<<endl;
        return 1;
    }
    cout<<"mysql init success "<<mysql<<" "<<&mysqlconn<<endl;
    if(mysql_real_connect(mysql,
                        "localhost",
                        "root",
                        "135799",
                        "Student",
                        3306,
                        nullptr,
                        0) == nullptr)
    {
        cout<<"connection error"<<endl;
        mysql_close(mysql);
        return 1;
    }
    cout<<"mysql connection success"<<endl;
    const char* sqlstr = "create table student(sid varchar(10), sname varchar(10), ssex varchar(4), sage int)";
    if(mysql_query(mysql, sqlstr) != 0)
    {
        cout<<"create table error"<<endl;
        return 1;
    }

    

    mysql_close(mysql);
    mysql = nullptr;

    return 0;
}