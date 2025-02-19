#ifndef REDISSERVER_H
#define REDISSERVER_H


#include "Network/TcpServer.h"
#include "CmdParserFactory.h"  

namespace toolkit
{

class RedisServer : public TcpServer {
public:
    using Ptr = std::shared_ptr<RedisServer>;
    /**
    * @brief 创建Redis Server
    * @param poller 是否给Redis Server指定唯一的Poller线程监听（是否需要开启多线程监听），默认为nullptr（开启）
    * @param periodic_flush 是否开启定期刷盘操作（持久化数据到LevelDB中），默认为true（开启）
    * @param seconds 定期刷盘周期
    */
    explicit RedisServer(const EventPoller::Ptr &poller = nullptr) : TcpServer(poller) {
        _cmdParserFactor = CmdParserFactory::Instance();    // 内部会执行数据的加载(disk->memory)
        _redisServerTimer = nullptr;
    }
    /**
    * @brief 开始Redis server
    * @param port 本机端口，0则随机
    * @param host 监听网卡ip
    * @param backlog tcp listen backlog
    * @param periodic_flush 是否开启定期刷盘（默认开启）
    * @param seconds 刷盘周期（默认 1 min）
    */
    template <typename SessionType>
    void Start(uint16_t port, bool periodic_flush = true, const std::string &host = "::", uint32_t backlog = 1024, const std::function<void(std::shared_ptr<SessionType> &)> &cb = nullptr,  float seconds = 60.0) {
        if (periodic_flush == true) {
            // 新建定时任务
            std::weak_ptr<RedisServer> weak_self = std::static_pointer_cast<RedisServer>(shared_from_this());
            _redisServerTimer = std::make_shared<Timer>(seconds,[weak_self](){
                auto strong_self = weak_self.lock();
                if (!strong_self) {
                    return false;
                }
                strong_self->_cmdParserFactor->getRedisHelper()->persistChanges();
                return true;

                }, _poller);
        }
        this->start(port, host, backlog, cb);
    }
private:
    CmdParserFactory::Ptr _cmdParserFactor;     // 命令解析工厂
    Timer::Ptr _redisServerTimer;       // 全局的redis Server的时间定时刷盘器
};   
} // namespace toolkit



#endif