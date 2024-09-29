#include <iostream>
#include <memory>
#include "MysqlConn.h"
#include "ConnectionPool.h"
#include <thread>
using namespace std;

// 1. ���߳�: ʹ�� /��ʹ�����ӳ�
// 2. ���߳�: ʹ�� /��ʹ�����ӳ�

// ģ�ⲻʹ�����ӳصĲ���
void op1(int begin, int end)
{
    for (int i = begin; i < end; ++i)
    {
        // Ҫ�������ڲ���ģ���ν������ݿ����Ӻ����ٲ���
        MysqlConn conn;
        conn.connect("root", "1", "mysql_linux", "192.168.10.30");
        char sql[1024] = {0};
        sprintf(sql, "insert into person values(%d, 25, 'boy', 'Tom')", i);
        conn.update(sql);
    }
}

// ģ��ʹ�����ӳصĲ���
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

    // �����ӳ�, ���߳�,  ��ʱ: 160020372168 ����, 160020 ����
    steady_clock::time_point begin = steady_clock::now();
    op1(1, 5001);
    steady_clock::time_point end = steady_clock::now();

    auto length = end - begin; // ����Ϊ���룬����count�����õ��ж�������
    cout << "�����ӳ�, ���߳�,  ��ʱ: " << length.count() << " ����, "
         << length.count() / 1000000 << " ����" << endl;
#else

    // ���ӳ�, ���߳�,  ��ʱ: 27016210230 ����, 27016 ����
    ConnectionPool *pool = ConnectionPool::getConnectPool();
    steady_clock::time_point begin = steady_clock::now();
    op2(pool, 1, 5001);
    steady_clock::time_point end = steady_clock::now();

    auto length = end - begin;
    cout << "���ӳ�, ���߳�,  ��ʱ: " << length.count() << " ����, "
         << length.count() / 1000000 << " ����" << endl;

#endif
}

void test2()
{
#if 0
    // �����ӳ�, ���߳�,  ��ʱ: 83936897329 ����, 83936 ����
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
    cout << "�����ӳ�, ���߳�,  ��ʱ: " << length.count() << " ����, "
         << length.count() / 1000000 << " ����" << endl;

#else

    // ���ӳ�, ���߳�,  ��ʱ: 4799960855 ����, 4799 ����
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
    cout << "���ӳ�, ���߳�,  ��ʱ: " << length.count() << " ����, "
         << length.count() / 1000000 << " ����" << endl;

#endif
}

int query()
{
    MysqlConn conn;
    conn.connect("root", "1", "mysql_linux", "192.168.10.30");

    // ִ�в���� sql���, id,age,gender,name
    string sql = "insert into person values(6, 25, 'boy', 'Tom')";
    bool flag = conn.update(sql);
    cout << "flag value:  " << flag << endl;

    // ִ�в�ѯ sql
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
