#ifndef CMDQUEUEMANAGER_H
#define CMDQUEUEMANAGER_H

#include <iostream>
#include <thread>
#include "CmdQueue.h"

namespace toolkit
{
    
// 命令队列管理器：管理一个后台线程持续从队列中获取命令并执行，同时提供启动和停止线程的机制。
class CommandQueueManager {
public:
    static CommandQueueManager &Instance() {
        static CommandQueueManager instance;    //C++11 线程安全的局部静态变量
        return instance;
    }
    // 禁用拷贝构造和赋值操作
    CommandQueueManager(const CommandQueueManager &) = delete;
    CommandQueueManager &operator=(const CommandQueueManager &) = delete;

    // 添加命令到队列
    void pushCommand(CommandQueue::Command cmd) {
        _commandQueue.push(std::move(cmd));
    }
    // 停止任务处理
    void stop() {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _stop = true;
        }
        _commandQueue.push([] {});  // 推入空任务以唤醒线程
        if(_workThread.joinable()) {
            _workThread.join();
        }
    }
private:
    // 构造与析构函数
    CommandQueueManager() : _stop(false) {
        // 启动后台线程
        _workThread = std::thread([this]() {
            this->processCommands();
        });
    }
    ~CommandQueueManager() {
        stop();
    }
    // 后台线程处理命令
    void processCommands() {
        while (true) {
            auto cmd = _commandQueue.pop();
            {
                std::lock_guard<std::mutex> lock(_mutex);
                if(_stop && _commandQueue.empty()) {
                    break;
                }
            }
            try {
                cmd();  //执行命令
            } catch (const std::exception &ex) {
                std::cerr << "Command execution error: " << ex.what() <<std::endl;
            }
        }
    }
    CommandQueue _commandQueue;
    std::thread _workThread;
    std::mutex _mutex;
    bool _stop;
};

} // namespace toolkit


#endif