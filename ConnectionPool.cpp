#include "ConnectionPool.h"
#include <json/json.h>
#include <fstream>
using namespace Json;

atomic<bool> isShutdown{false};

// ͨ��������ȡ������ݿ����ӳ�ʵ��
ConnectionPool *ConnectionPool::getConnectPool()
{
    // ������̬���ֱ��������ʷ�Χ��ֻ��������������򣬵������������ǵ��������
    // ��һ�ε��ã����ڴ����ڴ��ַ�����ǵڶ��ξ�ֱ��ʹ�������ַ�ˣ������ٴδ������ͷ���֮ǰ������ʵ����
    // ����ʹ�õ��ǵ���ģʽ�е�����ģʽ������ʱ����
    static ConnectionPool pool;
    return &pool;
}

// ���� json����
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

// �����µ�����
void ConnectionPool::produceConnection()
{
    while (true)
    {
        if (isShutdown)
            return;

        // ��С����������֤����������С����������ʹ��ʹ�ã�Ӧ��ͻȻ�ĸ���������
        unique_lock<mutex> locker(m_mutexQ);
        while (m_connectionQ.size() >= m_minSize || m_connectionQ.size() + m_busySize >= m_maxSize)
        {
            m_cond.wait(locker);

            // ����ǳ���Ҫ���������Ϊ���ӳعرջ��ѣ�ֱ���˳�
            if (isShutdown)
                return;
        }

        addConnection();     // �����Ӽ������
        m_cond.notify_all(); // ���������ڶ���Ϊ�յ��߳�
    }
}

// ��������
// ���ٵĿ������ӣ����㳬��һ��ʱ�����ǿ��о�����
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

            // ����ʱ��
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

// ��������
void ConnectionPool::addConnection()
{
    MysqlConn *conn = new MysqlConn;
    conn->connect(m_user, m_passwd, m_dbName, m_ip, m_port);
    conn->refreshAliveTime();
    m_connectionQ.push(conn);
}

// �ⲿ���������ȡ��һ�����ݿ�����
shared_ptr<MysqlConn> ConnectionPool::getConnection()
{
    // ������ж��������
    // 1. ���û�ﵽ������������󣬵��Ƕ���Ϊ�գ�����
    // 2. ���û�ﵽ������������󣬵��Ƕ��в�Ϊ�գ�������
    // 3. ����ﵽ������������󣬵��Ƕ��в�Ϊ�գ�����
    // 4. ����ﵽ�������s�����󣬵��Ƕ���Ϊ�գ�����
    // ֻҪû�ﵽ����������Լ����в�Ϊ�գ�����ȡ���ӣ���������
    // ���õĳ�ʱ��⣬����ʱ��û�����ѣ������ִ�� while �ж��Ƿ��������
    unique_lock<mutex> locker(m_mutexQ);
    while (m_connectionQ.empty() || (m_busySize >= m_maxSize))
    {
        // ����ָ����ʱ�䳤�ȣ�ʱ�䵽�˾ͽ������
        if (cv_status::timeout == m_cond.wait_for(locker, chrono::milliseconds(m_timeout)))
        {
            // ������� if ˵������ m_timeout ���ʱ��������û�б�����
            if (m_connectionQ.empty() || (m_busySize >= m_maxSize))
            {
                // ���ַ�ʽ��continue�������������� return�˳�
                // return nullptr;
                continue;
            }
        }
    }

    // ʹ�ù��������ָ�룬���յ�ǰ������������� Push�������У��Զ�����
    // ���������������ָ��Ĭ��ɾ�����������ǣ������������ָ��ָ����ڴ��ַ
    // �������ǲ���Ҫ���գ�������Ҫ���¼�����У��������ָ��ɾ��������
    // Ҳ���ǵڶ�������������ʹ�� lambda��ʽʵ��
    shared_ptr<MysqlConn> connptr(
        m_connectionQ.front(),
        [this](MysqlConn *conn)
        {
            unique_lock<mutex> locker(m_mutexQ);

            // ���¼�����е�ʱ��
            conn->refreshAliveTime();
            m_connectionQ.push(conn);

            m_busySize--;
            m_cond.notify_all(); // ����������������������߳�
        });

    m_connectionQ.pop();

    m_busySize++;

    // �����ǻ����������̣߳�Ҳ���ǻ��Ѵ��������ӵ��߳�
    // ��Ȼ�ỽ��ȡ���ӵ��̣߳����ǲ�Ӱ�죬���Ի��������
    m_cond.notify_all();
    return connptr;
}

ConnectionPool::~ConnectionPool()
{
    isShutdown = true;

    // ֪ͨ���еȴ����߳�
    m_cond.notify_all();

    while (!m_connectionQ.empty())
    {
        MysqlConn *conn = m_connectionQ.front();
        m_connectionQ.pop();
        delete conn;
    }

    printf("���ӳ�����...\n");
}

ConnectionPool::ConnectionPool()
{
    // ���������ļ�
    if (!parseJsonFile())
    {
        return;
    }

    for (int i = 0; i < m_minSize; ++i)
    {
        // �����Ӽ������
        addConnection();
    }

    // �����µ�����
    thread producer(&ConnectionPool::produceConnection, this);
    // ��������
    thread recycler(&ConnectionPool::recycleConnection, this);
    producer.detach();
    recycler.detach();
}
