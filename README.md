# PicoRedis
一款C++11标准实现的高性能Redis服务器，且使用LevelDB进行数据持久化。其具体实现借鉴使用了开源项目ZLToolKit: https://github.com/ZLMediaKit/ZLToolKit/tree/master，有关其源码笔记参见另一仓库：。  
可以完美的与 Redis 自带的客户端Redis-cli实现交互，且可以使用Redis-benchmark进行性能测试。

**附注**：本项目仅仅是个人学习项目，难免错误很多，还在不断的完善补充。

## 编译
```Bash
cd PicoRedis
make clean
make
```
## 运行
```Bash
./bin/redisServer
```
运行后会在终端显示：
```Bash

    _______ _________ _______  _______  _______  _______  ______  _________ _______ 
    (  ____ )\__   __/(  ____ \(  ___  )(  ____ )(  ____ \(  __  \ \__   __/(  ____ \
    | (    )|   ) (   | (    \/| (   ) || (    )|| (    \/| (  \  )   ) (   | (    \/
    | (____)|   | |   | |      | |   | || (____)|| (__    | |   ) |   | |   | (_____ 
    |  _____)   | |   | |      | |   | ||     __)|  __)   | |   | |   | |   (_____  )
    | (         | |   | |      | |   | || (\ (   | (      | |   ) |   | |         ) |
    | )      ___) (___| (____/\| (___) || ) \ \__| (____/\| (__/  )___) (___/\____) |
    |/       \_______/(_______/(_______)|/   \__/(_______/(______/ \_______/\_______)
                                                                                    

Welcome to PicoRedis - A lightweight Redis server implemented in C++!
Starting PicoRedis server...
```                                                                            


## 客户端连接
这里客户端的代码还不太完善，所以目前可以直接使用 redis-cli 与之交互（需要事先安装Redis)，默认 PicoRedis 服务器端口是：6380。
```Bash
redis-cli -p 6380
```
## 性能测试
测试我直接使用的是 Redis 提供的工具：redis-benchmark。如在终端输入：
```Bash
redis-benchmark -p 6380 -t set,get,hset,hget -c 100 -n 10000
```
在终端会输出测试结果，下面是在我系统上执行的输出结果：
```Bash
====== SET ======
  10000 requests completed in 0.18 seconds
  100 parallel clients
  3 bytes payload
  keep alive: 1

88.69% <= 1 milliseconds
100.00% <= 1 milliseconds
55248.62 requests per second

====== GET ======
  10000 requests completed in 0.18 seconds
  100 parallel clients
  3 bytes payload
  keep alive: 1

92.35% <= 1 milliseconds
100.00% <= 1 milliseconds
56818.18 requests per second

====== HSET ======
  10000 requests completed in 0.18 seconds
  100 parallel clients
  3 bytes payload
  keep alive: 1

89.45% <= 1 milliseconds
99.93% <= 7 milliseconds
99.97% <= 14 milliseconds
100.00% <= 14 milliseconds
55248.62 requests per second
```
可以与标准的 Redis 对比测试性能，但受限于个人水平，和Redis本身版本更迭性能卓越，PicoRedis难以达到Redis本身的高性能。  

## 目前支持的命令

| 命令  | 支持的数据类型  | 命令描述  |
| --- | --- | --- |
| `set` | STRING | 设置指定键的值，如果键已存在则覆盖旧值。 |
| `setnx` | STRING | 仅当键不存在时设置键的值。 |
| `setex` | STRING | 设置键的值并同时设置过期时间。 |
| `get` | STRING | 获取指定键的值。 |
| `select` | ALL | 切换到指定的数据库。 |
| `dbsize` | ALL | 返回当前数据库中键的数量。 |
| `exists` | ALL | 检查给定键是否存在。 |
| `del` | ALL | 删除指定的键。 |
| `incr` | STRING | 将键所存储的值递增 1。 |
| `incrby` | STRING | 将键所存储的值按指定的增量递增。 |
| `decr` | STRING | 将键所存储的值递减 1。 |
| `decrby` | STRING | 将键所存储的值按指定的减量递减。 |
| `mset` | STRING | 同时设置一个或多个键值对。 |
| `mget` | STRING | 获取一个或多个键的值。 |
| `hmset` | HASH | 同时设置哈希表中多个字段的值。 |
| `hmget` | HASH | 获取哈希表中一个或多个字段的值。 |
| `strlen` | STRING | 返回键所存储的字符串值的长度。 |
| `append` | STRING | 如果键已经存在并且是一个字符串，将指定值追加到该键原有值的末尾。 |
| `keys` | ALL | 查找所有符合给定模式的键。 |
| `lpush` | LIST | 将一个或多个值插入到列表的头部。 |
| `rpush` | LIST | 将一个或多个值插入到列表的尾部。 |
| `lpop` | LIST | 移除并返回列表的第一个元素。 |
| `rpop` | LIST | 移除并返回列表的最后一个元素。 |
| `lrange` | LIST | 返回列表中指定区间内的元素。 |
| `hset` | HASH | 设置哈希表中指定字段的值。 |
| `hget` | HASH | 获取哈希表中指定字段的值。 |
| `hdel` | HASH | 删除哈希表中一个或多个指定字段。 |
| `hgetall` | HASH | 获取哈希表中所有的字段和值。 |
| `multi` | ALL | 开启一个事务块。 |
| `exec` | ALL | 执行事务块内的所有命令。 |
| `discard` | ALL | 取消当前事务。 |
| `command` | ALL | 获取 Redis 命令的相关信息。 |
| `sadd` | SET | 向集合中添加一个或多个成员。 |
| `srem` | SET | 移除集合中一个或多个成员。 |
| `smembers` | SET | 返回集合中的所有成员。 |
| `sismember` | SET | 判断成员是否是集合的成员。 |

