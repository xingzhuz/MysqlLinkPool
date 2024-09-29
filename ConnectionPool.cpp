#include "ConnectionPool.h"
#include <json/json.h>
#include <fstream>
using namespace Json;

atomic<bool> isShutdown{false};

// 通过类名获取这个数据库连接池实例
ConnectionPool *ConnectionPool::getConnectPool()
{
    // 创建静态布局变量，访问范围就只有这个函数作用域，但是生命周期是到程序结束
    // 第一次调用，会在创建内存地址，但是第二次就直接使用这个地址了，不会再次创建，就返回之前创建的实例了
    // 这里使用的是单例模式中的懒汉模式，调用时创建
    static ConnectionPool pool;
    return &pool;
}

// 解析 json数据
bool ConnectionPool::parseJsonFile()
{
    ifstream ifs("dbconf.json");
    Reader rd;
    Value root;
    rd.parse(ifs, root);
    if (root.isObject())
    {
        m_ip = root["ip"].asString();
        m_port = root["port"].asInt();
        m_user = root["userName"].asString();
        m_passwd = root["password"].asString();
        m_dbName = root["dbName"].asString();
        m_minSize = root["minSize"].asInt();
        m_maxSize = root["maxSize"].asInt();
        m_maxIdleTime = root["maxIdleTime"].asInt();
        m_timeout = root["timeout"].asInt();
        m_busySize = 0;
        return true;
    }
    return false;
}

// 创建新的连接
void ConnectionPool::produceConnection()
{
    while (true)
    {
        if (isShutdown)
            return;

        // 最小连接数，保证队列里有最小连接数，即使不使用，应对突然的高数量访问
        unique_lock<mutex> locker(m_mutexQ);
        while (m_connectionQ.size() >= m_minSize || m_connectionQ.size() + m_busySize >= m_maxSize)
        {
            m_cond.wait(locker);

            // 这个非常重要，如果是因为连接池关闭唤醒，直接退出
            if (isShutdown)
                return;
        }

        addConnection();     // 将连接加入队列
        m_cond.notify_all(); // 唤醒阻塞在队列为空的线程
    }
}

// 销毁连接
// 销毁的空闲连接，满足超过一定时长还是空闲就销毁
void ConnectionPool::recycleConnection()
{
    while (true)
    {
        if (isShutdown)
            return;

        this_thread::sleep_for(chrono::milliseconds(500));
        unique_lock<mutex> locker(m_mutexQ);
        while (m_connectionQ.size() > m_minSize)
        {
            MysqlConn *conn = m_connectionQ.front();

            // 存活的时间
            if (conn->getAliveTime() >= m_maxIdleTime)
            {
                m_connectionQ.pop();
                delete conn;
            }
            else
            {
                break;
            }
        }
    }
}

// 增加连接
void ConnectionPool::addConnection()
{
    MysqlConn *conn = new MysqlConn;
    conn->connect(m_user, m_passwd, m_dbName, m_ip, m_port);
    conn->refreshAliveTime();
    m_connectionQ.push(conn);
}

// 外部调用这个，取出一个数据库连接
shared_ptr<MysqlConn> ConnectionPool::getConnection()
{
    // 这个能判断四种情况
    // 1. 如果没达到最大连接数请求，但是队列为空，阻塞
    // 2. 如果没达到最大连接数请求，但是队列不为空，不阻塞
    // 3. 如果达到最大连接数请求，但是队列不为空，阻塞
    // 4. 如果达到最大连接s数请求，但是队列为空，阻塞
    // 只要没达到最大连接数以及队列不为空，才能取连接，否则阻塞
    // 设置的超时检测，超过时长没被唤醒，则继续执行 while 判断是否继续阻塞
    unique_lock<mutex> locker(m_mutexQ);
    while (m_connectionQ.empty() || (m_busySize >= m_maxSize))
    {
        // 阻塞指定的时间长度，时间到了就解除阻塞
        if (cv_status::timeout == m_cond.wait_for(locker, chrono::milliseconds(m_timeout)))
        {
            // 进入这个 if 说明阻塞 m_timeout 这个时长，还是没有被唤醒
            if (m_connectionQ.empty() || (m_busySize >= m_maxSize))
            {
                // 两种方式，continue继续阻塞，或者 return退出
                // return nullptr;
                continue;
            }
        }
    }

    // 使用共享的智能指针，回收当前连接用完后，重新 Push进队列中，自动回收
    // 由于这个共享智能指针默认删除器处理动作是：回收这个智能指针指向的内存地址
    // 但是我们不是要回收，我们是要重新加入队列，因此重新指定删除器函数
    // 也就是第二个参数，这里使用 lambda方式实现
    shared_ptr<MysqlConn> connptr(
        m_connectionQ.front(),
        [this](MysqlConn *conn)
        {
            unique_lock<mutex> locker(m_mutexQ);

            // 更新加入队列的时间
            conn->refreshAliveTime();
            m_connectionQ.push(conn);

            m_busySize--;
            m_cond.notify_all(); // 唤醒因最大连接数阻塞的线程
        });

    m_connectionQ.pop();

    m_busySize++;

    // 本意是唤醒生产者线程，也就是唤醒创建新连接的线程
    // 虽然会唤醒取连接的线程，但是不影响，它仍会继续阻塞
    m_cond.notify_all();
    return connptr;
}

ConnectionPool::~ConnectionPool()
{
    isShutdown = true;

    // 通知所有等待的线程
    m_cond.notify_all();

    while (!m_connectionQ.empty())
    {
        MysqlConn *conn = m_connectionQ.front();
        m_connectionQ.pop();
        delete conn;
    }

    printf("连接池销毁...\n");
}

ConnectionPool::ConnectionPool()
{
    // 加载配置文件
    if (!parseJsonFile())
    {
        return;
    }

    for (int i = 0; i < m_minSize; ++i)
    {
        // 将连接加入队列
        addConnection();
    }

    // 创建新的连接
    thread producer(&ConnectionPool::produceConnection, this);
    // 销毁连接
    thread recycler(&ConnectionPool::recycleConnection, this);
    producer.detach();
    recycler.detach();
}
