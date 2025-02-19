#ifndef CMDQUEUE_H
#define CMDQUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace toolkit
{
    
class CommandQueue {
public:
    using Command = std::function<void()>;

    // 添加命令到队列
    void push(Command cmd) {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _queue.push(std::move(cmd));
        }
        _condition.notify_one();
    }

    // 从队列中取出命令
    Command pop() {
        std::unique_lock<std::mutex> lock(_mutex);
        _condition.wait(lock, [this]() { return !_queue.empty(); });
        Command cmd = std::move(_queue.front());
        _queue.pop();
        return cmd;
    }

    // 判断当前任务队列是否为空
    bool empty() {
        return _queue.empty();
    }

private:
    std::queue<Command> _queue;
    std::mutex _mutex;
    std::condition_variable _condition;
};

} // namespace toolkit


#endif