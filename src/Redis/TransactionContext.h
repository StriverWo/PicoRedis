#ifndef TRANSACTIONCONTEXT_H
#define TRANSACTIONCONTEXT_H
#include <vector>
#include <functional>
#include <iostream>

namespace toolkit
{
// 封装事务状态管理和命令队列逻辑
class TransactionContext {
public:
    void startTransaction() {
        _inTransation = true;
        _transcationQueue.clear();
    }

    void endTransaction() {
        _inTransation = false;
        _transcationQueue.clear();
    }

    bool isTransactionActive() const {
        return _inTransation;
    }

    void addCommandToQueue(const std::function<void(std::ostringstream&)>& command) {
        if(_inTransation) {
            _transcationQueue.push_back(command);
        }
    }

    const std::vector<std::function<void(std::ostringstream&)>> getTransactionQueue() const {
        return _transcationQueue;
    }

private:
    bool _inTransation = false;  // 是否开启事务标志
    std::vector<std::function<void(std::ostringstream&)>> _transcationQueue;    // 事务队列
    
};
} // namespace toolkit


#endif