#pragma once
#include <iostream>
#include <mysql.h>
#include <chrono>
using namespace std;
using namespace chrono;
class MysqlConn
{
public:
    // 初始化数据库连接
    MysqlConn();

    // 释放数据库连接
    ~MysqlConn();

    // 连接数据库
    bool connect(string user, string passwd, string dbName, string ip, unsigned short port = 3306);

    // 更新数据库: insert, update, delete
    bool update(string sql);

    // 查询数据库 select 语句
    bool query(string sql);

    // 遍历查询得到的结果集
    bool next();

    // 得到结果集中的字段值
    string value(int index);

    // 事务操作
    bool transaction();

    // 提交事务
    bool commit();

    // 事务回滚
    bool rollback();

    // 刷新起始的空闲时间点
    void refreshAliveTime();

    // 计算连接存活的总时长
    long long getAliveTime();

private:
    void freeResult();                    // 释放结果集内存
    MYSQL *m_conn = nullptr;              // 数据库对象
    MYSQL_RES *m_result = nullptr;        // select 查询后的结果集
    MYSQL_ROW m_row = nullptr;            // 用于遍历结果集，保存一行数据
    steady_clock::time_point m_alivetime; // 创建绝对时钟
};

