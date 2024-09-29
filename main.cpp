#include <iostream>
#include <memory>
#include "MysqlConn.h"
#include "ConnectionPool.h"
#include <thread>
using namespace std;

// 1. 单线程: 使用 /不使用连接池
// 2. 多线程: 使用 /不使用连接池

// 模拟不使用连接池的操作
void op1(int begin, int end)
{
    for (int i = begin; i < end; ++i)
    {
        // 要创建在内部，模拟多次进行数据库连接和销毁操作
        MysqlConn conn;
        conn.connect("root", "1", "mysql_linux", "192.168.10.30");
        char sql[1024] = {0};
        sprintf(sql, "insert into person values(%d, 25, 'boy', 'Tom')", i);
        conn.update(sql);
    }
}

// 模拟使用连接池的操作
void op2(ConnectionPool *pool, int begin, int end)
{
    for (int i = begin; i < end; ++i)
    {
        shared_ptr<MysqlConn> conn = pool->getConnection();
        char sql[1024] = {0};
        sprintf(sql, "insert into person values(%d, 25, 'boy', 'Tom')", i);
        conn->update(sql);
    }
}

void test1()
{
#if 1

    // 非连接池, 单线程,  用时: 160020372168 纳秒, 160020 毫秒
    steady_clock::time_point begin = steady_clock::now();
    op1(1, 5001);
    steady_clock::time_point end = steady_clock::now();

    auto length = end - begin; // 精度为纳秒，调用count方法得到有多少纳秒
    cout << "非连接池, 单线程,  用时: " << length.count() << " 纳秒, "
         << length.count() / 1000000 << " 毫秒" << endl;
#else

    // 连接池, 单线程,  用时: 27016210230 纳秒, 27016 毫秒
    ConnectionPool *pool = ConnectionPool::getConnectPool();
    steady_clock::time_point begin = steady_clock::now();
    op2(pool, 1, 5001);
    steady_clock::time_point end = steady_clock::now();

    auto length = end - begin;
    cout << "连接池, 单线程,  用时: " << length.count() << " 纳秒, "
         << length.count() / 1000000 << " 毫秒" << endl;

#endif
}

void test2()
{
#if 0
    // 非连接池, 单线程,  用时: 83936897329 纳秒, 83936 毫秒
    MysqlConn conn;
    conn.connect("root", "1", "mysql_linux", "192.168.10.30");

    steady_clock::time_point begin = steady_clock::now();
    thread t1(op1, 1, 1001);
    thread t2(op1, 1001, 2001);
    thread t3(op1, 2001, 3001);
    thread t4(op1, 3001, 4001);
    thread t5(op1, 4001, 5001);

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    steady_clock::time_point end = steady_clock::now();

    auto length = end - begin;
    cout << "非连接池, 单线程,  用时: " << length.count() << " 纳秒, "
         << length.count() / 1000000 << " 毫秒" << endl;

#else

    // 连接池, 单线程,  用时: 4799960855 纳秒, 4799 毫秒
    ConnectionPool *pool = ConnectionPool::getConnectPool();

    steady_clock::time_point begin = steady_clock::now();
    thread t1(op2, pool, 1, 1001);
    thread t2(op2, pool, 1001, 2001);
    thread t3(op2, pool, 2001, 3001);
    thread t4(op2, pool, 3001, 4001);
    thread t5(op2, pool, 4001, 5001);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    steady_clock::time_point end = steady_clock::now();

    auto length = end - begin;
    cout << "连接池, 单线程,  用时: " << length.count() << " 纳秒, "
         << length.count() / 1000000 << " 毫秒" << endl;

#endif
}

int query()
{
    MysqlConn conn;
    conn.connect("root", "1", "mysql_linux", "192.168.10.30");

    // 执行插入的 sql语句, id,age,gender,name
    string sql = "insert into person values(6, 25, 'boy', 'Tom')";
    bool flag = conn.update(sql);
    cout << "flag value:  " << flag << endl;

    // 执行查询 sql
    sql = "select * from person";
    conn.query(sql);
    while (conn.next())
    {
        cout << conn.value(0) << ", "
             << conn.value(1) << ", "
             << conn.value(2) << ", "
             << conn.value(3) << endl;
    }
    return 0;
}
int main()
{
    // query();

    test1();

    // test2();
    return 0;
}
