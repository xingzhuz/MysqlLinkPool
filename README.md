# MysqlLinkPool
基于 C++11 的数据库连接池

> 具体实现思路查看: [https://xingzhu.top/archives/shu-ju-ku-lian-jie-chi-shi-xian](https://xingzhu.top/archives/shu-ju-ku-lian-jie-chi-shi-xian)

## 介绍

- 一个基于 Linux 环境下实现的数据库连接池
- Linux 下相关的环境配置查看: [https://xingzhu.top/archives/shu-ju-ku-lian-jie-chi-huan-jing-pei-zhi](https://xingzhu.top/archives/shu-ju-ku-lian-jie-chi-huan-jing-pei-zhi)


```SQL
--- 此程序使用的数据库以及表头设置
create database mysql_linux;
use mysql_linux;

CREATE TABLE person (
    id INT PRIMARY KEY AUTO_INCREMENT,
    age INT,
    gender ENUM('boy', 'girl'),
    name VARCHAR(100)
);
```


```SHELL
# 编译运行
g++ *.cpp -o main -lmysqlclient -ljson
./main
```

