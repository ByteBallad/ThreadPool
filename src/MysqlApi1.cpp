
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

    mysql_close(mysql);
    mysql = nullptr;

    return 0;
}