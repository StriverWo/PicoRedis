

#include <csignal>
#include <iostream>

#ifndef _WIN32
#include <unistd.h>
#endif

#include "Redis/RedisSession.h"
using namespace std;
using namespace toolkit;



int main() {
    // // 初始化日志模块  [AUTO-TRANSLATED:fd9321b2]
    // // Initialize the log module
    // Logger::Instance().add(std::make_shared<ConsoleChannel>());
    // Logger::Instance().setWriter(std::make_shared<AsyncLogWriter>());



    RedisServer::Ptr server(new RedisServer());
    server->Start<RedisSession>(6380,false);//监听6380端口

    //退出程序事件处理  [AUTO-TRANSLATED:80065cb7]
    // Exit program event handling
    static semaphore sem;
    signal(SIGINT, [](int) { 
        sem.post(); 
    });// 设置退出信号
    sem.wait();
    return 0;
}
