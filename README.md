# PicoRedis
一款C++11标准实现的高性能Redis服务器，且使用LevelDB进行数据持久化，LevelDB 是谷歌开源的键值数据库，以牺牲部分读性能的方式具备优秀的写性能。而 PicoRedis 只是将LevelDB 作为数据持久化的介质，和 Redis一样，读取和写入数据仍然始终是先写入内存。
然后PicoRedis可以设置定期的触发持久化，事先数据的持久化。有关 LevelDB 源码的阅读笔记参见我的另一仓库：https://github.com/StriverWo/LevelDB-NOTE 。

其有关网络部分的具体实现借鉴了开源项目ZLToolKit: https://github.com/ZLMediaKit/ZLToolKit/tree/master ，有关其源码笔记参考我的另一仓库：https://github.com/StriverWo/ZLToolKit-NOTE 。  
PicoRedis 可以完美的与 Redis 自带的客户端Redis-cli实现交互，且可以使用Redis-benchmark进行性能测试。

**附注**：本项目仅仅是个人学习项目，难免错误很多，还在不断的完善补充。

## 设计动机
主要是在学习 Redis 之后有的这个想法：  
* 传统 Redis 是基于 C 语言设计，而如今 C++ 有许多优秀的特性，比如说智能指针，STL标准库，多态，模板等，结合 C++ 的优势能否使得操作更加高效？
* 传统 Redis 使用 AOF 或 RDB 方式进行持久化，但是他们始终是存储到文件中，文件内容最终落盘后才能实现永久的持久化。而 LevelDB 只需追加写日志的形式就能保证数据安全，追加写具有很高的性能。
* 虽然 在Redis 中使用了多线程，但在较老版本的Redis中只是对比如说数据刷盘/数据删除等操作利用后台线程来异步完成操作。较新的版本 Redis6/7 中
进一步采用多个IO线程来处理网络IO，因为 Redis 的主要瓶颈在于内存大小和网络IO速度。但是读写操作命令仍然使用单线程来处理。
也就是说主线程只负责接受建立连接请求，并为其分配网络IO线程，通过全局等待队列和状态标志来进行网络IO线程和主线程的通信。当网络IO线程读取请求并解析完毕后，
再通知主线程进行执行命令。如果需要回写socket，那么主线程再将该任务交给网络IO线程来处理。
**但是，Redis6/7都是由单个线程完成socket的连接请求建立的，PicoRedis 的设计借鉴了 ZLToolKit 的设计，采用多线程监听，抢占式accept连接方式进行设计。**

## 主要模块
### 网络部分：
**多线程监听+抢占式accept**  
在PicoRedis服务器启动的时候，会创建一个socket，经过命名绑定(bind)，然后调用listen系统调用创建一个监听队列以存放待处理的客户连接。然后将这个监听socket fd下发给多个线程进行监听，当socket可读时会触发accept事件回调，由于accept系统调用本身是线程安全的，多线程accept并不会出现竞态条件，因为内核会保证同一时刻只有一个线程能够成功接受连接。
**多线程多epoll，一个线程一个事件轮询（one pthead one loop)**  
### 命令解析部分：
全局维护一个命令解析工厂（享元工厂模式），用unordered_map来维护<命令,命令解析器>的映射。用智能指针加以管理每个命令解析器。保证全局只存在相应命令对应的一个命令解析器。



## 编译
```Bash
cd PicoRedis
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
这里客户端的代码还不太完善，所以目前可以直接使用 redis-cli 与之交互（需要事先安装Redis)，默认 PicoRedis 服务器端口是：6380。打开另一终端输入：
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
但是通过 redis-benchmark 测试可知 PicoRedis 仍然是一个高性能的设计

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

