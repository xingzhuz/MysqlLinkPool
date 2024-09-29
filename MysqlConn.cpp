#include "MysqlConn.h"

// 初始化数据库连接
MysqlConn::MysqlConn()
{
    m_conn = mysql_init(nullptr);

    // 设置字符集，防止后续读取中文字符乱码
    mysql_set_character_set(m_conn, "utf8");
}

// 释放数据库连接
MysqlConn::~MysqlConn()
{
    if (m_conn != nullptr)
    {
        mysql_close(m_conn);
    }
    freeResult();
}

// 连接数据库
bool MysqlConn::connect(string user, string passwd, string dbName, string ip, unsigned short port)
{
    // c_str() 将 string 转化为 char*
    MYSQL *ptr = mysql_real_connect(m_conn, ip.c_str(), user.c_str(), passwd.c_str(), dbName.c_str(), port, nullptr, 0);
    return ptr != nullptr;
}

// 更新数据库: insert, update, delete
bool MysqlConn::update(string sql)
{
    if (mysql_query(m_conn, sql.c_str()))
    {
        return false;
    }
    return true;
}

// 查询数据库 select 语句
bool MysqlConn::query(string sql)
{
    freeResult();
    if (mysql_query(m_conn, sql.c_str()))
    {
        return false;
    }

    // 获取结果集
    m_result = mysql_store_result(m_conn);
    return true;
}

// 遍历查询得到的结果集
bool MysqlConn::next()
{
    if (m_result != nullptr)
    {
        // 调用一次读取一行
        m_row = mysql_fetch_row(m_result);
        if (m_row != nullptr)
        {
            return true;
        }
    }
    return false;
}

// 得到结果集中的字段值
string MysqlConn::value(int index)
{
    // 获取结果集中列的个数
    int rowCount = mysql_num_fields(m_result);
    if (index >= rowCount || index < 0)
    {
        return string();
    }

    // m_row 是一个二级指针，指向指针数组，也就是第一个元素地址，也就是一个指针，所以内部定义为二级指针
    // 指针数组是：一个数组中存放的是多个指针
    // m_row[index] 等价与 *(m_row + index) 得到的就是 index 处的值，也就是一个指针，指向 char* 的指针
    char *val = m_row[index];

    // 获取 index 处的值的长度，这个函数返回的是一个 unsigned long*, 存储了所有字段的对应的值的长度
    unsigned long length = mysql_fetch_lengths(m_result)[index];
    return string(val, length);
}

// 事务操作
bool MysqlConn::transaction()
{
    // 设置手动提交还是自动提交事务，false 是手动提交
    return mysql_autocommit(m_conn, false);
}

// 提交事务
bool MysqlConn::commit()
{
    return mysql_commit(m_conn);
}

// 事务回滚
bool MysqlConn::rollback()
{
    return mysql_rollback(m_conn);
}

// 刷新起始的空闲时间点
void MysqlConn::refreshAliveTime()
{
    // 这个得到的时间能够达到纳秒及
    m_alivetime = steady_clock::now();
}

// 计算连接存活的总时长
long long MysqlConn::getAliveTime()
{
    nanoseconds res = steady_clock::now() - m_alivetime;
    // 转换为低精度，高 --> 低 使用 duration_cast 类
    milliseconds millsec = duration_cast<milliseconds>(res);
    return millsec.count();
}

// 释放结果集内存
void MysqlConn::freeResult()
{
    if (m_result)
    {
        mysql_free_result(m_result);
        m_result = nullptr;
    }
}
