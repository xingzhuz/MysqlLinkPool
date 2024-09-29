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
    // ���þ�̬��������Ϊ���ܴ���������������Ϊ��̬������ͨ��������ȡ������ݿ����ӳ�ʵ��
    static ConnectionPool *getConnectPool();

    // ɾ���������캯���� "=" ���������أ���ΪҪ��ֻ֤��һ�����ӳ�ʵ��
    ConnectionPool(const ConnectionPool &obj) = delete;
    ConnectionPool &operator=(const ConnectionPool &obj) = delete;

    // �ⲿ���������ȡ��һ�����ݿ�����
    shared_ptr<MysqlConn> getConnection();
    ~ConnectionPool();

private:
    // ���캯��˽�л����������ⲿ��������Ϊ����һ�����󣬵���ģʽ
    ConnectionPool();

    // ���� json����
    bool parseJsonFile();

    // �����µ����ӵ��̴߳���������
    void produceConnection();

    // ���������̵߳��̴߳���������
    void recycleConnection();

    // ��������
    void addConnection();

    string m_ip;            // ���ݷ��������� IP��ַ
    string m_user;          // ���ݿ�������û���
    string m_passwd;        // ��Ӧ������
    string m_dbName;        // ���ݿ�������϶�Ӧ�����ݿ���
    unsigned short m_port;  // ���ݿ�������˿ں�
    int m_minSize;          // ������������Сֵ�������ά�����е���Сֵ����֤ͻȻ�Ĵ�����������
    int m_maxSize;          // �������������ֵ������Ƕ������� + m_busySize����ʾ���ֻ֧�ֵ�������������������ʹ�õ�
    atomic<int> m_busySize; // ����æ��������������ǹ�����Դ������Ϊԭ�ӱ���
    int m_timeout;          // ��ʱʱ�����������������ȴ�������ʱ��
    int m_maxIdleTime;      // ������ʱ���������Ƿ�Ͽ�������ݿ�����

    // ���ӳض��У��洢��ʱ�����ɸ����ݿ�����
    queue<MysqlConn *> m_connectionQ;
    mutex m_mutexQ;            // ���ӳض��еĻ�����
    condition_variable m_cond; // ��������
};

