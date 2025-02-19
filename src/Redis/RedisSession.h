#ifndef REDISSESSION_H
#define REDISSESSION_H

#include "Util/logger.h"
#include "Util/TimeTicker.h"
#include "RedisServer.h"
#include "Network/Session.h"
#include "CmdQueue.h"
#include "SkipList.h"
#include "CmdParserFactory.h"

namespace toolkit
{

class RedisSession : public Session {
public:
    using Ptr = std::shared_ptr<RedisSession>;
    RedisSession (const Socket::Ptr &sock) :
            Session(sock) {
        _cmdParserFactor = CmdParserFactory::Instance();
        //DebugL << "New RedisSession created: " << sock->get_local_ip() << ":" <<sock->get_local_port();
    }
    ~RedisSession () {
        //DebugL << "RedisSession destroyed.";
    }
    virtual void onRecv(const Buffer::Ptr &buf) override{
        //处理客户端发送过来的数据  [AUTO-TRANSLATED:c095b82e]
        // Handle data sent from the client
        std::string received_data = buf->toString();
        //DebugL << "Receive data: " << received_data;

        // 1. 初步命令解析：由 RESP 格式转化为vector存储的多个 string 格式
        std::vector<std::string> tokens = parseRESPCommand(received_data);
        if (tokens.empty()) {
            send("-Protocol error\r\n");
            return;
        }
        // 2. 得到相应命令的解析器
        const std::string cmd = tokens.front();
        std::shared_ptr<CommandParser> commandParser = _cmdParserFactor->getParser(cmd);
        if(commandParser == nullptr) {
            send("-ERR unknown command\r\n");
        }
        auto self = std::dynamic_pointer_cast<Session>(shared_from_this());
        // 3. 如果处于事务中且非事务控制命令，加入事务队列
        if(_transactionContext.isTransactionActive() && cmd != "exec" && cmd != "discard") {
            if(commandParser->parserCommand(tokens, self)){
                _transactionContext.addCommandToQueue([commandParser, tokens, self](std::ostringstream& result){
                    commandParser->parserAndExecuter(tokens, self);
                });
                send("+QUEUED\r\n");    // 注意一定要发送响应
                return;
            }
            _transactionContext.endTransaction(); 
            return;
        }
        // 4. 非事务模式或事务控制命令，解析器解析并将命令加入命令队列等待执行
        if(commandParser) {
            try{
                commandParser->parserAndExecuter(tokens, self);
            } catch (std::exception & ex){
                send("-ERR unknown command\r\n");
            } 
        }
    }
    virtual void onError(const SockException &err) override{
        //客户端断开连接或其他原因导致该对象脱离TCPServer管理  [AUTO-TRANSLATED:6b958a7b]
        // Client disconnects or other reasons cause the object to be removed from TCPServer management
        //WarnL << err;
    }
    virtual void onManager() override{
        //DebugL <<"Connect:" << this->get_peer_ip() <<" : " << this->get_peer_port() << " is alived !";
        // if(_ticker.elapsedTime() >= 60000){
        //     // 1 分钟持久化一次，持久化的过程也是异步执行的，避免阻塞当前Poller线程，这样能更好的做到负载均衡。
        //     auto timePoller = EventPollerPool::Instance().getPoller();
        //     timePoller->async_first([this]() {
        //         this->_cmdParserFactor->getRedisHelper()->persistChanges();
        //     });
        //     DebugL << "Data persisted to storage! ";
        //     _ticker.resetTime();    //重置计时器
        // }
    }

    // 提供事务上下文的接口
    virtual TransactionContext& getTransactionContext() override {
        return _transactionContext;
    }
    
private:
    Ticker _ticker;
    CmdParserFactory::Ptr _cmdParserFactor;
    TransactionContext _transactionContext;


    std::vector<std::string> parseRESPCommand(const std::string &data) {
        std::vector<std::string> result;
        std::istringstream stream(data);
        std::string line;

        std::function<void(std::istringstream &, int)> parseRESP = [&](std::istringstream &stream, int depth = 0) -> void {
            while (std::getline(stream, line)) {
                // 去除行尾的 \r
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }

                if (line.empty()) {
                    continue;
                }

                if (line[0] == '*') {
                    // 解析数组结构
                    int num_elements = std::stoi(line.substr(1));
                    while (num_elements-- > 0) {
                        parseRESP(stream, depth + 1);
                    }
                } else if (line[0] == '$') {
                    // 解析 Bulk String
                    int bulk_length = std::stoi(line.substr(1));
                    if (bulk_length > 0 && std::getline(stream, line)) {
                        if (!line.empty() && line.back() == '\r') {
                            line.pop_back();
                        }
                        result.push_back(line.substr(0, bulk_length));
                    }
                }
            }
        };

        parseRESP(stream, 0);
        return result;
    }
    
};
 
} // namespace toolkit

#endif