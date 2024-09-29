#include "MysqlConn.h"

// ��ʼ�����ݿ�����
MysqlConn::MysqlConn()
{
    m_conn = mysql_init(nullptr);

    // �����ַ�������ֹ������ȡ�����ַ�����
    mysql_set_character_set(m_conn, "utf8");
}

// �ͷ����ݿ�����
MysqlConn::~MysqlConn()
{
    if (m_conn != nullptr)
    {
        mysql_close(m_conn);
    }
    freeResult();
}

// �������ݿ�
bool MysqlConn::connect(string user, string passwd, string dbName, string ip, unsigned short port)
{
    // c_str() �� string ת��Ϊ char*
    MYSQL *ptr = mysql_real_connect(m_conn, ip.c_str(), user.c_str(), passwd.c_str(), dbName.c_str(), port, nullptr, 0);
    return ptr != nullptr;
}

// �������ݿ�: insert, update, delete
bool MysqlConn::update(string sql)
{
    if (mysql_query(m_conn, sql.c_str()))
    {
        return false;
    }
    return true;
}

// ��ѯ���ݿ� select ���
bool MysqlConn::query(string sql)
{
    freeResult();
    if (mysql_query(m_conn, sql.c_str()))
    {
        return false;
    }

    // ��ȡ�����
    m_result = mysql_store_result(m_conn);
    return true;
}

// ������ѯ�õ��Ľ����
bool MysqlConn::next()
{
    if (m_result != nullptr)
    {
        // ����һ�ζ�ȡһ��
        m_row = mysql_fetch_row(m_result);
        if (m_row != nullptr)
        {
            return true;
        }
    }
    return false;
}

// �õ�������е��ֶ�ֵ
string MysqlConn::value(int index)
{
    // ��ȡ��������еĸ���
    int rowCount = mysql_num_fields(m_result);
    if (index >= rowCount || index < 0)
    {
        return string();
    }

    // m_row ��һ������ָ�룬ָ��ָ�����飬Ҳ���ǵ�һ��Ԫ�ص�ַ��Ҳ����һ��ָ�룬�����ڲ�����Ϊ����ָ��
    // ָ�������ǣ�һ�������д�ŵ��Ƕ��ָ��
    // m_row[index] �ȼ��� *(m_row + index) �õ��ľ��� index ����ֵ��Ҳ����һ��ָ�룬ָ�� char* ��ָ��
    char *val = m_row[index];

    // ��ȡ index ����ֵ�ĳ��ȣ�����������ص���һ�� unsigned long*, �洢�������ֶεĶ�Ӧ��ֵ�ĳ���
    unsigned long length = mysql_fetch_lengths(m_result)[index];
    return string(val, length);
}

// �������
bool MysqlConn::transaction()
{
    // �����ֶ��ύ�����Զ��ύ����false ���ֶ��ύ
    return mysql_autocommit(m_conn, false);
}

// �ύ����
bool MysqlConn::commit()
{
    return mysql_commit(m_conn);
}

// ����ع�
bool MysqlConn::rollback()
{
    return mysql_rollback(m_conn);
}

// ˢ����ʼ�Ŀ���ʱ���
void MysqlConn::refreshAliveTime()
{
    // ����õ���ʱ���ܹ��ﵽ���뼰
    m_alivetime = steady_clock::now();
}

// �������Ӵ�����ʱ��
long long MysqlConn::getAliveTime()
{
    nanoseconds res = steady_clock::now() - m_alivetime;
    // ת��Ϊ�;��ȣ��� --> �� ʹ�� duration_cast ��
    milliseconds millsec = duration_cast<milliseconds>(res);
    return millsec.count();
}

// �ͷŽ�����ڴ�
void MysqlConn::freeResult()
{
    if (m_result)
    {
        mysql_free_result(m_result);
        m_result = nullptr;
    }
}
