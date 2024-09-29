#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include "MysqlConn.h"
#include <atomic>
using namespace std;

class ConnectionPool
{
public:
    // 设置静态方法，因为不能创建对象，所以设置为静态方法，通过类名获取这个数据库连接池实例
    static ConnectionPool *getConnectPool();

    // 删除拷贝构造函数和 "=" 操作符重载，因为要保证只有一个连接池实例
    ConnectionPool(const ConnectionPool &obj) = delete;
    ConnectionPool &operator=(const ConnectionPool &obj) = delete;

    // 外部调用这个，取出一个数据库连接
    shared_ptr<MysqlConn> getConnection();
    ~ConnectionPool();

private:
    // 构造函数私有化，不允许外部创建，因为保持一个对象，单例模式
    ConnectionPool();

    // 解析 json数据
    bool parseJsonFile();

    // 创建新的连接的线程处理动作函数
    void produceConnection();

    // 销毁连接线程的线程处理动作函数
    void recycleConnection();

    // 增加连接
    void addConnection();

    string m_ip;            // 数据发服务器的 IP地址
    string m_user;          // 数据库服务器用户名
    string m_passwd;        // 对应的密码
    string m_dbName;        // 数据库服务器上对应的数据库名
    unsigned short m_port;  // 数据库服务器端口号
    int m_minSize;          // 连接数量的最小值，这个是维护队列的最小值，保证突然的大量连接请求
    int m_maxSize;          // 连接数量的最大值，这个是队列数量 + m_busySize，表示最多只支持的连接数，包括了正在使用的
    atomic<int> m_busySize; // 正在忙的连接数，这个是共享资源，设置为原子变量
    int m_timeout;          // 超时时长，当连接数用完后等待的阻塞时长
    int m_maxIdleTime;      // 最大空闲时长，决定是否断开这个数据库连接

    // 连接池队列，存储的时是若干个数据库连接
    queue<MysqlConn *> m_connectionQ;
    mutex m_mutexQ;            // 连接池队列的互斥锁
    condition_variable m_cond; // 条件变量
};

