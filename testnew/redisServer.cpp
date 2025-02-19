

#include <csignal>
#include <iostream>
#include <unistd.h>

#include "Redis/RedisSession.h"
using namespace std;
using namespace toolkit;



// 打印欢迎信息和服务器启动信息
void printWelcomeMessage() {
    cout << R"(
    _______ _________ _______  _______  _______  _______  ______  _________ _______ 
    (  ____ )\__   __/(  ____ \(  ___  )(  ____ )(  ____ \(  __  \ \__   __/(  ____ \
    | (    )|   ) (   | (    \/| (   ) || (    )|| (    \/| (  \  )   ) (   | (    \/
    | (____)|   | |   | |      | |   | || (____)|| (__    | |   ) |   | |   | (_____ 
    |  _____)   | |   | |      | |   | ||     __)|  __)   | |   | |   | |   (_____  )
    | (         | |   | |      | |   | || (\ (   | (      | |   ) |   | |         ) |
    | )      ___) (___| (____/\| (___) || ) \ \__| (____/\| (__/  )___) (___/\____) |
    |/       \_______/(_______/(_______)|/   \__/(_______/(______/ \_______/\_______)
                                                                                    
)" << endl;
    cout << "Welcome to PicoRedis - A lightweight Redis server implemented in C++!" << endl;
    cout << "Starting PicoRedis server..." << endl;
}

// 打印服务器停止信息
void printShutdownMessage() {
    cout << "Shutting down PicoRedis server..." << endl;
}


int main() {
    // 打印欢迎信息
    printWelcomeMessage();

    // 启动 Redis 服务器
    RedisServer::Ptr server(new RedisServer());
    server->Start<RedisSession>(6380, false); // 监听 6380 端口
    cout << "PicoRedis server is listening on port 6380..." << endl;

    //退出程序事件处理
    static semaphore sem;
    signal(SIGINT, [](int) { 
        sem.post(); 
        printShutdownMessage();
    });// 设置退出信号
    sem.wait();

    return 0;
}


 // // 初始化日志模块  [AUTO-TRANSLATED:fd9321b2]
    // // Initialize the log module
    // Logger::Instance().add(std::make_shared<ConsoleChannel>());
    // Logger::Instance().setWriter(std::make_shared<AsyncLogWriter>());