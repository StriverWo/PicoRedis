#ifndef CMDPARSERFACTORY_H
#define CMDPARSERFACTORY_H

#include "CmdParser.h"
#include "Global.h"
#include "Util/onceToken.h"

namespace toolkit
{
// 享元模式工厂
class CmdParserFactory {
private:
    std::unordered_map<std::string, std::shared_ptr<CommandParser>> parserMaps;
    RedisHelper::Ptr redisHelper_;

    CmdParserFactory() : redisHelper_(RedisHelper::instance()) {
        static onceToken token([this]() {
            redisHelper_->loadFromStorage();
            DebugL << "Data loaded from Disk !";
        });
    };  // 初始化RedisHelper对象

    std::shared_ptr<CommandParser> createCommandParser(const std::string& command);

public:
    using Ptr = std::shared_ptr<CmdParserFactory>;
    static CmdParserFactory::Ptr Instance() {
        static std::shared_ptr<CmdParserFactory> instance(new CmdParserFactory());        
        return instance;
    }

    RedisHelper::Ptr& getRedisHelper() {
        return redisHelper_;
    }

    std::shared_ptr<CommandParser> getParser(const std::string& command) {
        if(!parserMaps.empty() && parserMaps.find(command) != parserMaps.end()) {
            return parserMaps[command];
        }
        return createCommandParser(command);
    }
};

std::shared_ptr<CommandParser> CmdParserFactory::createCommandParser(const std::string& command){
        Command op;
        if(commandMaps.find(command)  == commandMaps.end()) {
            op = INVALID_COMMAND;
        } else {
            op = commandMaps[command];
        }

        switch(op) {
            case SET:{
                parserMaps[command] = std::make_shared<SetParser>(redisHelper_);
                break;
            }
            case GET:{
                parserMaps[command] = std::make_shared<GetParser>(redisHelper_);
                break;
            }
            case DEL:{
                parserMaps[command] = std::make_shared<DelParser>(redisHelper_);
                break;
            }
            case EXISTS:{
                parserMaps[command] = std::make_shared<ExistsParser>(redisHelper_);
                break;
            }
            case INCR:{
                parserMaps[command] = std::make_shared<IncrParser>(redisHelper_);
                break;
            }
            case DECR:{
                parserMaps[command] = std::make_shared<DecrParser>(redisHelper_);
                break;
            }
            case APPEND:{
                parserMaps[command] = std::make_shared<AppendParser>(redisHelper_);
                break;
            }
            case KEYS:{
                parserMaps[command] = std::make_shared<KeysParaser>(redisHelper_);
                break;
            }
            case MSET:{
                parserMaps[command] = std::make_shared<MSetParser>(redisHelper_);
                break;               
            }
            case MGET:{
                parserMaps[command] = std::make_shared<MGetParser>(redisHelper_);
                break;
            }
            case HSET:{
                parserMaps[command] = std::make_shared<HSetParser>(redisHelper_);
                break;
            }
            case HMSET:{
                parserMaps[command] = std::make_shared<HMSetParser>(redisHelper_);
                break;
            }
            case HMGET:{
                parserMaps[command] = std::make_shared<HMGetParser>(redisHelper_);
                break;
            }
            case HGET:{
                parserMaps[command] = std::make_shared<HGetParser>(redisHelper_);
                break;
            }
            case HDEL:{
                parserMaps[command] = std::make_shared<HDelParser>(redisHelper_);
                break;
            }
            case HGETALL:{
                parserMaps[command] = std::make_shared<HGetAllParser>(redisHelper_);
                break;
            }
            case INCRBY:{
                parserMaps[command] = std::make_shared<IncrByParser>(redisHelper_);
                break;
            }
            case DECRBY:{
                parserMaps[command] = std::make_shared<DecrByParser>(redisHelper_);
                break;
            }
            case MULTI:{
                parserMaps[command] = std::make_shared<MultiParser>(redisHelper_);               
                break;
            }
            case EXEC:{
                parserMaps[command] = std::make_shared<ExecParser>(redisHelper_);               
                break;
            }
            case DISCARD:{
                parserMaps[command] = std::make_shared<DiscardParser>(redisHelper_);
                break;
            }
            case STRLEN:{
                parserMaps[command] = std::make_shared<StrlenParser>(redisHelper_);
                break;
            }
            case SELECT:{
                parserMaps[command] = std::make_shared<SelectParser>(redisHelper_);
                break;
            }
            case COMMAND:{
                parserMaps[command] = std::make_shared<CommandParserCommand>(redisHelper_);
                break;
            }
            case LPUSH:{
                parserMaps[command] = std::make_shared<LPushParser>(redisHelper_);                
                break;
            }
            case RPUSH:{
                parserMaps[command] = std::make_shared<RPushParser>(redisHelper_);
                break;
            }
            case LPOP:{
                parserMaps[command] = std::make_shared<LPopParser>(redisHelper_);
                break;
            }
            case RPOP:{
                parserMaps[command] = std::make_shared<RPopParser>(redisHelper_);
                break;
            }
            case LRANGE:{
                parserMaps[command] = std::make_shared<LRangeParser>(redisHelper_);
                break;
            }
            case SADD:{
                parserMaps[command] = std::make_shared<SAddParser>(redisHelper_);            
                break;
            }
            case SREM:{
                parserMaps[command] = std::make_shared<SRemParser>(redisHelper_);
                break;
            }
            case SMEMBERS:{
                parserMaps[command] = std::make_shared<SMembersParser>(redisHelper_);
                break;
            }
            case SISMEMEBER:{
                parserMaps[command] = std::make_shared<SIsMemberParser>(redisHelper_);
                break;
            }
            case DBSIZE:{
                parserMaps[command] = std::make_shared<DBsizeParaser>(redisHelper_);
                break;
            }
            default:{
                return nullptr;
            }
        }
        return parserMaps[command];
    };

} // namespace toolkit


#endif