#ifndef CMDPARSER_H
#define CMDPARSER_H

#include <vector>
#include <memory>
#include "CmdQueueManager.h"
#include "RedisHelper.h"
#include "RedisSession.h"


namespace toolkit
{
class CommandParser {
public:
    using Ptr = std::shared_ptr<CommandParser>;

    explicit CommandParser(const RedisHelper::Ptr helper) : redisHelper_(std::move(helper)) {}    
    CommandParser() = delete;
    virtual ~CommandParser() = default;

    // 解析并执行，执行并不是真正执行，而是将其压入命令队列管理器等待执行
    void parserAndExecuter(const std::vector<std::string> &args, Session::Ptr session) {
        if(!parserCommand(std::move(args), session)){
            return;
        }
        std::string cmd = args[0];
        auto dataStore = redisHelper_->getDataType(cmd);

        CommandQueueManager::Instance().pushCommand([args,this,session,dataStore]() {
            this->executeCommand(std::move(args),std::move(session),std::move(dataStore));
        });
    }

    // 解析并执行
    virtual void executeCommand(const std::vector<std::string>& command, Session::Ptr session, RedisDataType::Ptr dataStore) = 0;
    virtual bool parserCommand(const std::vector<std::string> &command, Session::Ptr session) = 0;

protected:
    RedisHelper::Ptr redisHelper_;
};

// SET 命令解析器 
class SetParser : public CommandParser {
public:
    explicit SetParser(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string> &command, Session::Ptr session) override{
        if(command.size() < 3) {
            ////DebugL << "Invalid SET command.";
            session->send(std::move("-ERR wrong number of arguments for 'set' command\r\n"));
            return false;
        }
        return true;
    }  
    // 执行 SET 命令的逻辑
    void executeCommand(const std::vector<std::string> &command, Session::Ptr session,RedisDataType::Ptr dataStore) override {
        // 动态类型转换
        auto redisString = std::dynamic_pointer_cast<RedisString>(dataStore);
        if(!redisString) {
            //DebugL << "Error: Data store is not a RedisString type.";
            session->send(std::move("-ERR operation against a key holding the wrong kind of value\r\n"));
            return;
        }
        // 调用 RedisString 的 insert 方法
        redisString->insert({command[1] ,command[2]});
        //DebugL << "Inserted key: " << command[1] << " with value: " << command[2];
        session->send(std::move("+OK\r\n"));
    }
};

// GET 命令解析器
class GetParser : public CommandParser {
public:
    explicit GetParser(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string> &command, Session::Ptr session) override{
        if(command.size() != 2) {
            //DebugL << "Invalid GET command.";
            session->send(std::move("-ERR wrong number of arguments for 'get' command\r\n"));
            return false;
        }
        return true;
    }  

    // 执行 Get 命令的逻辑
    void executeCommand(const std::vector<std::string> &command,Session::Ptr session, RedisDataType::Ptr dataStore) override {
        // 动态类型转换
        auto redisString = std::dynamic_pointer_cast<RedisString>(dataStore);
        if(!redisString) {
            //DebugL << "Error: Data store is not a RedisString type.";
            session->send(std::move("-ERR operation against a key holding the wrong kind of value\r\n"));
            return;
        }
        auto value = redisString->get({command[1]});
        if ( value != nullptr ) {
            //DebugL << "Found value for key " << command[1] << ": " << value;
            session->send(std::move("$" + std::to_string(value->size()) + "\r\n" + (*value) + "\r\n"));
        } else {
            session->send(std::move("$-1\r\n"));
        }
    }
};
// STRLEN 命令解析器
class StrlenParser : public CommandParser {
public:
    explicit StrlenParser(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string> &command, Session::Ptr session) override {
        if(command.size() != 2) {
            //DebugL << "Invalid STRLEN command.";
            session->send("-ERR wrong number of arguments for 'strlen' command\r\n");
            return false;
        }
        return true;
    }

    // 执行 STRLEN 命令的逻辑
    void executeCommand(const std::vector<std::string> &command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        // 动态类型转换
        auto redisString = std::dynamic_pointer_cast<RedisString>(dataStore);
        if (!redisString) {
            //DebugL << "Error: Data store is not a RedisString type.";
            session->send("-ERR operation against a key holding the wrong kind of value\r\n");
            return;
        }

        // 获取字符串长度
        auto value = redisString->get({command[1]});
        if (value != nullptr) {
            //DebugL << "Found value for key " << command[1] << ": " << *value;
            session->send(":" + std::to_string(value->size()) + "\r\n");
        } else {
            session->send(":0\r\n");  // 如果不存在该键，返回 0
        }
    }
};
// COMMAND 命令解析器
class CommandParserCommand : public CommandParser {
public:
    explicit CommandParserCommand(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string> &command, Session::Ptr session) override {
        if (command.size() != 1) {
            //DebugL << "Invalid COMMAND command.";
            session->send("-ERR wrong number of arguments for 'COMMAND' command\r\n");
            return false;
        }
        return true;
    }

    void executeCommand(const std::vector<std::string> &command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        // 构建 RESP 格式响应
        std::ostringstream response;
        response << "*" << commandMaps.size() << "\r\n";

        for (const auto &[cmd, enumVal] : commandMaps) {
            response << "*6\r\n";                 // 每个命令包含 6 部分
            response << "$" << cmd.size() << "\r\n" << cmd << "\r\n"; // 命令名称
            response << ":-1\r\n";                // 参数数量 -1 表示变长参数
            response << "*1\r\n$5\r\nwrite\r\n"; // 默认标志为 write
            response << ":1\r\n";                 // 第一个键位置
            response << ":1\r\n";                 // 最后一个键位置
            response << ":1\r\n";                 // 步长
        }

        session->send(response.str());
        //DebugL << "Sent COMMAND response.";
    }
};

// SELECT 命令解析器
class SelectParser : public CommandParser {
public:
    explicit SelectParser(std::shared_ptr<RedisHelper> redisHelper) 
        :CommandParser(std::move(redisHelper)) {}
private:
    bool parserCommand(const std::vector<std::string> &command, Session::Ptr session) override{
        if(command.size() != 2) {
            //DebugL << "Invalid SELECT command.";
            session->send("-ERR wrong number of arguments for 'sellect' command\r\n");
            return false;
        }
        return true;
    }
    void executeCommand(const std::vector<std::string> &command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        int dbIndex = std::stoi(command[1]);
        redisHelper_->selectDatabase(dbIndex);
        session->send("+OK\r\n");
        //DebugL << "Jump to db [" << dbIndex << "]";
    }
};

// KEYS 命令解析器
class KeysParaser : public CommandParser {
public:
    explicit KeysParaser(std::shared_ptr<RedisHelper> redisHelper) 
        :CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string> &command, Session::Ptr session) override{
        if(command.size() != 2) {
            //DebugL << "Invalid KEYS command.";
            session->send("-ERR wrong number of arguments for 'keys' command\r\n");
            return false;
        }
        return true;
    }
    void executeCommand(const std::vector<std::string> &command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        const std::string &pattern = command[1];

        // 获取所有匹配的键
        auto matched_keys = redisHelper_->keys(pattern);

        // 构建返回响应
        std::string response = "*" + std::to_string(matched_keys.size()) + "\r\n";
        for (const auto &key : matched_keys) {
            response += "$" + std::to_string(key.size()) + "\r\n" + key + "\r\n";
        }

        session->send(response);
        //DebugL << "Found " << matched_keys.size() << " keys matching pattern: " << pattern;
    }
};

// DEL 命令解析器
class DelParser : public CommandParser {
public:
    explicit DelParser(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string> &command, Session::Ptr session) override{
        if(command.size() != 2) {
            //DebugL << "Invalid DEL command.";
            session->send("-ERR wrong number of arguments for 'del' command\r\n");
            return false;
        }
        return true;
    }

    void executeCommand(const std::vector<std::string> &command, Session::Ptr session,RedisDataType::Ptr dataStore) override {
        bool deleted = redisHelper_->eraseKey(command[1]);
        if (deleted) {
            //DebugL << "Deleted key: " << command[1];
            session->send(":1\r\n");
        } else {
            //DebugL << "Key not found: " << command[1];
            session->send(":0\r\n");
        }
    }
};

// EXISTS 命令解析器
class ExistsParser : public CommandParser {
public:
    explicit ExistsParser(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string> &command, Session::Ptr session) override{
        if(command.size() != 2) {
            //DebugL << "Invalid EXISTS command.";
            session->send("-ERR wrong number of arguments for 'exists' command\r\n");
            return false;
        }
        return true;
    }
    void executeCommand(const std::vector<std::string> &command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        auto exists = redisHelper_->exists(command[1]);
        //DebugL << "Exists check for key: " << command[1] << ", result: " << exists;
        session->send(":" + std::to_string(exists ? 1 : 0) + "\r\n");
    }
};

// INCR 命令解析器
class IncrParser : public CommandParser {
public:
    explicit IncrParser(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}
private:
    bool parserCommand(const std::vector<std::string> &command, Session::Ptr session) override{
        if(command.size() != 2) {
            //DebugL << "Invalid INCR command.";
            session->send("-ERR wrong number of arguments for 'incr' command\r\n");
            return false;
        }
        return true;
    }  

    void executeCommand(const std::vector<std::string> &command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        // 动态类型转换
        auto redisString = std::dynamic_pointer_cast<RedisString>(dataStore);
        if(!redisString) {
            //DebugL << "Error: Data store is not a RedisString type.";
            session->send("-ERR operation against a key holding the wrong kind of value\r\n");
            return;
        }

        auto value = redisString->get({command[1]});
        int new_value = value ? std::stoi(*value) + 1 : 1;
        redisString->insert({command[1] ,std::to_string(new_value)});
        //DebugL << "Incremented key: " << command[1] << " to value: " << new_value;
        session->send(":" + std::to_string(new_value) + "\r\n");
    }
};

// DECR 命令解析器
class DecrParser : public CommandParser {
public:
    explicit DecrParser(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string> &command, Session::Ptr session) override{
        if(command.size() != 2) {
            //DebugL << "Invalid INCR command.";
            session->send("-ERR wrong number of arguments for 'incr' command\r\n");
            return false;
        }
        return true;
    }  

    void executeCommand(const std::vector<std::string> &command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        // 动态类型转换
        auto redisString = std::dynamic_pointer_cast<RedisString>(dataStore);
        if(!redisString) {
            //DebugL << "Error: Data store is not a RedisString type.";
            session->send("-ERR operation against a key holding the wrong kind of value\r\n");
            return;
        }

        auto value = redisString->get({command[1]});
        int new_value = value ? std::stoi(*value) - 1 : 1;
        redisString->insert({command[1] ,std::to_string(new_value)});
        //DebugL << "Decremented key: " << command[1] << " to value: " << new_value;
        session->send(":" + std::to_string(new_value) + "\r\n");
    }
};

// APPEND 命令解析器
class AppendParser : public CommandParser {
public:
    explicit AppendParser(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string>& command, Session::Ptr session) override {
        // APPEND 命令需要两个参数：命令名和键，键对应的值
        if (command.size() != 3) {
            //DebugL << "Invalid APPEND command.";
            session->send("-ERR wrong number of arguments for 'append' command\r\n");
            return false;
        }
        return true;
    }

    // 执行 APPEND 命令的逻辑
    void executeCommand(const std::vector<std::string>& command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        // 动态类型转换，将 dataStore 转换为 RedisString 类型
        auto redisString = std::dynamic_pointer_cast<RedisString>(dataStore);
        if (!redisString) {
            //DebugL << "Error: Data store is not a RedisString type.";
            session->send("-ERR operation against a key holding the wrong kind of value\r\n");
            return;
        }

        // 获取键当前的值，如果不存在则返回空字符串
        auto currentValue = redisString->get({command[1]});
        
        // 如果值存在，则将新值追加到现有值的末尾
        if (currentValue != nullptr) {
            std::string newValue = *currentValue + command[2];
            redisString->insert({command[1], newValue});
            //DebugL << "Appended to key: " << command[1] << " new value: " << newValue;
        } else {
            // 如果键不存在，将该值设置为传入的值
            redisString->insert({command[1], command[2]});
            //DebugL << "Set new key: " << command[1] << " to value: " << command[2];
        }

        // 返回追加后的字符串长度
        session->send(":" + std::to_string(command[1].size() + command[2].size()) + "\r\n");
    }
};

// MSET 命令解析器
class MSetParser : public CommandParser {
public:
    explicit MSetParser(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string>& command, Session::Ptr session) override {
        if (command.size() < 3 || command.size() % 2 == 0) {
            //DebugL << "Invalid MSET command.";
            session->send("-ERR wrong number of arguments for 'mset' command\r\n");
            return false;
        }
        return true;
    }
    void executeCommand(const std::vector<std::string> &command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        // 动态类型转换
        auto redisString = std::dynamic_pointer_cast<RedisString>(dataStore);
        if (!redisString) {
            //DebugL << "Error: Data store is not a RedisString type.";
            session->send("-ERR operation against a key holding the wrong kind of value\r\n");
            return;
        }
        for (size_t i = 1; i < command.size(); i += 2) {
            redisString->insert({command[i],command[i+1]});
            //DebugL << "Inserted key: " << command[i] << " with value: " << command[i + 1];
        }
        
        session->send("+OK\r\n");
    }
};

// MGET 命令解析器
class MGetParser : public CommandParser {
public:
    explicit MGetParser(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string>& command, Session::Ptr session) override {
        if (command.size() < 2 ) {
            //DebugL << "Invalid MGET command.";
            session->send("-ERR wrong number of arguments for 'mget' command\r\n");
            return false;
        }
        return true;
    }
    void executeCommand(const std::vector<std::string> &command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        auto redisString = std::dynamic_pointer_cast<RedisString>(dataStore);
        if (!redisString) {
            //DebugL << "Error: Data store is not a RedisString type.";
            session->send("-ERR operation against a key holding the wrong kind of value\r\n");
            return;
        }
        std::string response = "*" + std::to_string(command.size() - 1) + "\r\n";
        for (size_t i = 1; i < command.size(); ++i) {
            auto value = redisString->get({command[i]});
            if (value) {
                response += "$" + std::to_string(value->size()) + "\r\n" + *value + "\r\n";
            } else {
                response += "$-1\r\n";
            }
        }
        session->send(response);
        //DebugL << "MGET response sent for keys: " << command.size() - 1;
    }
};

// HMSET 命令解析器
class HMSetParser : public CommandParser {
public:
    explicit HMSetParser(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string>& command, Session::Ptr session) override {
        if (command.size() < 3 || command.size() % 2 == 1) {
            //DebugL << "Invalid HMSET command.";
            session->send("-ERR wrong number of arguments for 'hmset' command\r\n");
            return false;
        }
        return true;
    }
    void executeCommand(const std::vector<std::string> &command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        // 动态类型转换
        auto redisHash = std::dynamic_pointer_cast<RedisHash>(dataStore);
        if(!redisHash) {
            //DebugL << "Error: Data store is not a RedisHash type.";
            session->send("-ERR operation against a key holding the wrong kind of value\r\n");
            return;
        }
        for (size_t i = 2; i < command.size(); i += 2) {
            redisHash->hset(command[1],command[i],command[i+1]);
            //DebugL << "Inserted field: " << command[i] << " with value: " << command[i + 1];
        }

        session->send("+OK\r\n");
    }
};
// HMGET 命令解析器
class HMGetParser : public CommandParser {
public:
    explicit HMGetParser(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string>& command, Session::Ptr session) override {
        if (command.size() < 3 ) {
            //DebugL << "Invalid HMGET command.";
            session->send("-ERR wrong number of arguments for 'hmget' command. At least one field is required\r\n");
            return false;
        }
        return true;
    }
    void executeCommand(const std::vector<std::string> &command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        // 动态类型转换
        auto redisHash = std::dynamic_pointer_cast<RedisHash>(dataStore);
        if(!redisHash) {
            //DebugL << "Error: Data store is not a RedisHash type.";
            session->send("-ERR operation against a key holding the wrong kind of value\r\n");
            return;
        }

        std::ostringstream response;
        response << "*" << (command.size() - 2) << "\r\n";
        for (size_t i = 2; i < command.size(); ++i) {
            auto value = redisHash->hget(command[1], command[i]);
            if (value) {
                response << "$" << value->size() << "\r\n" << *value << "\r\n";
            } else {
                response << "$-1\r\n";
            }
        }
        session->send(response.str());
        //DebugL << "HMGET response sent for keys: " << command.size() - 2;
    }
};

// HSET 命令解析器
class HSetParser : public CommandParser {
public:
    explicit HSetParser(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string> &command, Session::Ptr session) override{
        if(command.size() != 4) {
            //DebugL << "Invalid HSET command.";
            session->send("-ERR wrong number of arguments for 'hset' command\r\n");
            return false;
        }
        return true;
    }  

    void executeCommand(const std::vector<std::string> &command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        // 动态类型转换
        auto redisHash = std::dynamic_pointer_cast<RedisHash>(dataStore);
        if(!redisHash) {
            //DebugL << "Error: Data store is not a RedisHash type.";
            session->send("-ERR operation against a key holding the wrong kind of value\r\n");
            return;
        }
        redisHash->hset(command[1],command[2],command[3]);
        //DebugL << "hset key: " << command[1] << " to value: " << command[2] << ": "<< command[3];
        session->send("+OK\r\n");
    }
};

// HGET 命令解析器
class HGetParser : public CommandParser {
public:
    explicit HGetParser(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string> &command, Session::Ptr session) override{
        if(command.size() != 3) {
            //DebugL << "Invalid HGET command.";
            session->send("-ERR wrong number of arguments for 'hget' command\r\n");
            return false;
        }
        return true;
    }  

    void executeCommand(const std::vector<std::string> &command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        // 动态类型转换
        auto redisHash = std::dynamic_pointer_cast<RedisHash>(dataStore);
        if(!redisHash) {
            //DebugL << "Error: Data store is not a RedisHash type.";
            session->send("-ERR operation against a key holding the wrong kind of value\r\n");
            return;
        }
        auto value = redisHash->hget(command[1],command[2]);
        if (value) {
            session->send("$" + std::to_string(value->size()) + "\r\n" + *value + "\r\n");
        } else {
            session->send("$-1\r\n");
        }
        //DebugL << "HGET response sent for field: " << command[2];
    }
    
};

// HDEL 命令解析器
class HDelParser : public CommandParser {
public:
    explicit HDelParser(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string> &command, Session::Ptr session) override{
        if (command.size() < 3) {
            //DebugL << "Invalid HDEL command.";
            session->send("-ERR wrong number of arguments for 'hdel' command\r\n");
            return false;
        }
        return true;
    }  
    void executeCommand(const std::vector<std::string> &command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        auto redisHash = std::dynamic_pointer_cast<RedisHash>(dataStore);
        if(!redisHash) {
            //DebugL << "Error: Data store is not a RedisHash type.";
            session->send("-ERR operation against a key holding the wrong kind of value\r\n");
            return;
        }
        bool deleted = redisHash->hdel(command[1], command[2]);
        if (deleted) {
            //DebugL << "Deleted field: " << command[2];
            session->send(":1\r\n");
        } else {
            //DebugL << "Field not found: " << command[2];
            session->send(":0\r\n");
        }
    }
};

// HGETALL
class HGetAllParser : public CommandParser {
public:
    explicit HGetAllParser(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string>& command, Session::Ptr session) override {
        // HGETALL 命令需要两个参数，第一个是命令名，第二个是哈希表的键
        if (command.size() != 2) {
            //DebugL << "Invalid HGETALL command.";
            session->send("-ERR wrong number of arguments for 'hgetall' command\r\n");
            return false;
        }
        return true;
    }

    void executeCommand(const std::vector<std::string>& command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        // 动态类型转换，将 dataStore 转换为 RedisHash 类型
        auto redisHash = std::dynamic_pointer_cast<RedisHash>(dataStore);
        if (!redisHash) {
            //DebugL << "Error: Data store is not a RedisHash type.";
            session->send("-ERR operation against a key holding the wrong kind of value\r\n");
            return;
        }

        // 获取指定键的所有字段和值
        auto allFields = redisHash->hgetall(command[1]);
        if (allFields.empty()) {
            // 如果没有找到字段，返回空的哈希
            session->send("*0\r\n");
        } else {
            // 格式化返回值为 Redis 响应格式
            std::string response = "*" + std::to_string(allFields.size() * 2) + "\r\n"; // 每个字段和值一对，共2个元素
            for (const auto& fieldValuePair : allFields) {
                response += "$" + std::to_string(fieldValuePair.first.size()) + "\r\n" + fieldValuePair.first + "\r\n";
                response += "$" + std::to_string(fieldValuePair.second.size()) + "\r\n" + fieldValuePair.second + "\r\n";
            }
            session->send(response);
        }
        //DebugL << "HGETALL response sent for key: " << command[1];
    }
};

// INCRBY 命令解析器
class IncrByParser : public CommandParser {
public:
    explicit IncrByParser(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string>& command, Session::Ptr session) override {
        if (command.size() != 3) {
            //DebugL << "Invalid INCRBY command.";
            session->send("-ERR wrong number of arguments for 'incrby' command\r\n");
            return false;
        }
        return true;
    }
    void executeCommand(const std::vector<std::string> &command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        auto redisString = std::dynamic_pointer_cast<RedisString>(dataStore);

        int increment = std::stoi(command[2]);
        auto value = redisString->get({command[1]});
        int new_value = value ? std::stoi(*value) + increment : increment;
        redisString->insert({command[1], std::to_string(new_value)});

        session->send(":" + std::to_string(new_value) + "\r\n");
        //DebugL << "Incremented key: " << command[1] << " to value: " << new_value;
    }
};

// DECRBY 命令解析器
class DecrByParser : public CommandParser {
public:
    explicit DecrByParser(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string>& command, Session::Ptr session) override {
        if (command.size() != 3) {
            //DebugL << "Invalid DECRBY command.";
            session->send("-ERR wrong number of arguments for 'decrby' command\r\n");
            return false;
        }
        return true;
    }
    void executeCommand(const std::vector<std::string> &command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        auto redisString = std::dynamic_pointer_cast<RedisString>(dataStore);

        int decrement = std::stoi(command[2]);
        auto value = redisString->get({command[1]});
        int new_value = value ? std::stoi(*value) - decrement : -decrement;
        redisString->insert({command[1], std::to_string(new_value)});

        session->send(":" + std::to_string(new_value) + "\r\n");
        //DebugL << "Decremented key: " << command[1] << " to value: " << new_value;
    }
};

// MULTI 事务命令解析器
// MultiParser
class MultiParser : public CommandParser {
public:
    explicit MultiParser(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string>& command, Session::Ptr session) override {
        if (command.size() != 1) {
            session->send("-ERR wrong number of arguments for 'multi' command\r\n");
            return false;
        }
        return true;
    }

    void executeCommand(const std::vector<std::string>& command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        //auto redisSession = std::dynamic_pointer_cast<RedisSession>(session);
        auto& transactionContext = session->getTransactionContext();
        if (transactionContext.isTransactionActive()) {
            session->send("-ERR MULTI calls can't be nested\r\n");
            return;
        }

        transactionContext.startTransaction();
        session->send("+OK\r\n");
    }
};

// ExecParser
class ExecParser : public CommandParser {
public:
    explicit ExecParser(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string>& command, Session::Ptr session) override {
        if (command.size() != 1) {
            session->send("-ERR wrong number of arguments for 'exec' command\r\n");
            return false;
        }
        return true;
    }

    void executeCommand(const std::vector<std::string>& command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        //auto redisSession = std::dynamic_pointer_cast<RedisSession>(session);
        auto& transactionContext = session->getTransactionContext();
        if (!transactionContext.isTransactionActive()) {
            session->send("-ERR EXEC without MULTI\r\n");
            return;
        }

        auto& transactionQueue = transactionContext.getTransactionQueue();
        std::string response = "*" + std::to_string(transactionQueue.size()) + "\r\n";
        // 迭代处理事务队列中的所有命令
        for (const auto& cmd : transactionQueue) {
            std::ostringstream result;
            cmd(result);
            response += result.str();
        }

        transactionContext.endTransaction();
        session->send(response);
    }
};

// DiscardParser
class DiscardParser : public CommandParser {
public:
    explicit DiscardParser(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string>& command, Session::Ptr session) override {
        if (command.size() != 1) {
            session->send("-ERR wrong number of arguments for 'discard' command\r\n");
            return false;
        }
        return true;
    }

    void executeCommand(const std::vector<std::string>& command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        //auto redisSession = std::dynamic_pointer_cast<RedisSession>(session);
        auto& transactionContext = session->getTransactionContext();

        if (!transactionContext.isTransactionActive()) {
            session->send("-ERR DISCARD without MULTI\r\n");
            return;
        }

        transactionContext.endTransaction();
        session->send("+OK\r\n");
    }
};
// LPUSH 命令解析器
class LPushParser : public CommandParser {
public:
    explicit LPushParser(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string> &command, Session::Ptr session) override {
        if (command.size() < 3) { // 至少需要 key 和一个 value
            DebugL << "Invalid LPUSH command.";
            session->send("-ERR wrong number of arguments for 'lpush' command\r\n");
            return false;
        }
        return true;
    }

    void executeCommand(const std::vector<std::string> &command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        auto redisList = std::dynamic_pointer_cast<RedisList>(dataStore);
        if (!redisList) {
            session->send("-ERR operation against a key holding the wrong kind of value\r\n");
            return;
        }

        const std::string& key = command[1];
        for (size_t i = 2; i < command.size(); ++i) {
            redisList->lpush(key, command[i]);
        }

        session->send(":" + std::to_string(command.size() - 2) + "\r\n"); // 返回插入的元素数量
    }
};

// RPUSH 命令解析器
class RPushParser : public CommandParser {
public:
    explicit RPushParser(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string> &command, Session::Ptr session) override {
        if (command.size() < 3) { // 至少需要 key 和一个 value
            DebugL << "Invalid RPUSH command.";
            session->send("-ERR wrong number of arguments for 'rpush' command\r\n");
            return false;
        }
        return true;
    }

    void executeCommand(const std::vector<std::string> &command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        auto redisList = std::dynamic_pointer_cast<RedisList>(dataStore);
        if (!redisList) {
            session->send("-ERR operation against a key holding the wrong kind of value\r\n");
            return;
        }

        const std::string& key = command[1];
        for (size_t i = 2; i < command.size(); ++i) {
            redisList->rpush(key, command[i]);
        }

        session->send(":" + std::to_string(command.size() - 2) + "\r\n"); // 返回插入的元素数量
    }
};

// LPOP 命令解析器
class LPopParser : public CommandParser {
public:
    explicit LPopParser(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string> &command, Session::Ptr session) override {
        if (command.size() != 2) { // 需要 key
            DebugL << "Invalid LPOP command.";
            session->send("-ERR wrong number of arguments for 'lpop' command\r\n");
            return false;
        }
        return true;
    }

    void executeCommand(const std::vector<std::string> &command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        auto redisList = std::dynamic_pointer_cast<RedisList>(dataStore);
        if (!redisList) {
            session->send("-ERR operation against a key holding the wrong kind of value\r\n");
            return;
        }

        const std::string& key = command[1];
        auto value = redisList->lpop(key);
        if (!value.empty()) {
            session->send("$" + std::to_string(value.size()) + "\r\n" + value + "\r\n");
        } else {
            session->send("$-1\r\n"); // 列表为空或不存在
        }
    }
};

// RPOP 命令解析器
class RPopParser : public CommandParser {
public:
    explicit RPopParser(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string> &command, Session::Ptr session) override {
        if (command.size() != 2) { // 需要 key
            DebugL << "Invalid RPOP command.";
            session->send("-ERR wrong number of arguments for 'rpop' command\r\n");
            return false;
        }
        return true;
    }

    void executeCommand(const std::vector<std::string> &command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        auto redisList = std::dynamic_pointer_cast<RedisList>(dataStore);
        if (!redisList) {
            session->send("-ERR operation against a key holding the wrong kind of value\r\n");
            return;
        }

        const std::string& key = command[1];
        auto value = redisList->rpop(key);
        if (!value.empty()) {
            session->send("$" + std::to_string(value.size()) + "\r\n" + value + "\r\n");
        } else {
            session->send("$-1\r\n"); // 列表为空或不存在
        }
    }
};

// LRANGE 命令解析器
class LRangeParser : public CommandParser {
public:
    explicit LRangeParser(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string> &command, Session::Ptr session) override {
        if (command.size() != 4) { // 需要 key, start, end
            DebugL << "Invalid LRANGE command.";
            session->send("-ERR wrong number of arguments for 'lrange' command\r\n");
            return false;
        }
        return true;
    }

    void executeCommand(const std::vector<std::string> &command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        auto redisList = std::dynamic_pointer_cast<RedisList>(dataStore);
        if (!redisList) {
            session->send("-ERR operation against a key holding the wrong kind of value\r\n");
            return;
        }

        const std::string& key = command[1];
        int start = std::stoi(command[2]);
        int end = std::stoi(command[3]);
        auto values = redisList->lrange(key, start, end);

        session->send("*" + std::to_string(values.size()) + "\r\n");
        for (const auto& value : values) {
            session->send("$" + std::to_string(value.size()) + "\r\n" + value + "\r\n");
        }
    }
};
// SADD
class SAddParser : public CommandParser {
public:
    explicit SAddParser(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string> &command, Session::Ptr session) override {
        if (command.size() < 3) {
            session->send(std::move("-ERR wrong number of arguments for 'sadd' command\r\n"));
            return false;
        }
        return true;
    }

    void executeCommand(const std::vector<std::string> &command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        auto redisSet = std::dynamic_pointer_cast<RedisSet>(dataStore);
        if (!redisSet) {
            session->send(std::move("-ERR operation against a key holding the wrong kind of value\r\n"));
            return;
        }

        const std::string &key = command[1];
        int addedCount = 0;

        for (size_t i = 2; i < command.size(); ++i) {
            redisSet->sadd(key, command[i]);
            ++addedCount;
        }

        session->send(std::move(":" + std::to_string(addedCount) + "\r\n"));
    }
};
// SREM
class SRemParser : public CommandParser {
public:
    explicit SRemParser(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string> &command, Session::Ptr session) override {
        if (command.size() < 3) {
            session->send(std::move("-ERR wrong number of arguments for 'srem' command\r\n"));
            return false;
        }
        return true;
    }

    void executeCommand(const std::vector<std::string> &command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        auto redisSet = std::dynamic_pointer_cast<RedisSet>(dataStore);
        if (!redisSet) {
            session->send(std::move("-ERR operation against a key holding the wrong kind of value\r\n"));
            return;
        }

        const std::string &key = command[1];
        int removedCount = 0;

        for (size_t i = 2; i < command.size(); ++i) {
            if (redisSet->srem(key, command[i])) {
                ++removedCount;
            }
        }

        session->send(std::move(":" + std::to_string(removedCount) + "\r\n"));
    }
};
// SMEMBERS
class SMembersParser : public CommandParser {
public:
    explicit SMembersParser(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string> &command, Session::Ptr session) override {
        if (command.size() != 2) {
            session->send(std::move("-ERR wrong number of arguments for 'smembers' command\r\n"));
            return false;
        }
        return true;
    }

    void executeCommand(const std::vector<std::string> &command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        auto redisSet = std::dynamic_pointer_cast<RedisSet>(dataStore);
        if (!redisSet) {
            session->send(std::move("-ERR operation against a key holding the wrong kind of value\r\n"));
            return;
        }

        const std::string &key = command[1];
        auto members = redisSet->smembers(key);

        session->send(std::move("*" + std::to_string(members.size()) + "\r\n"));
        for (const auto &member : members) {
            session->send(std::move("$" + std::to_string(member.size()) + "\r\n" + member + "\r\n"));
        }
    }
};
// SISMEMEBER
class SIsMemberParser : public CommandParser {
public:
    explicit SIsMemberParser(std::shared_ptr<RedisHelper> redisHelper)
        : CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string> &command, Session::Ptr session) override {
        if (command.size() != 3) {
            session->send(std::move("-ERR wrong number of arguments for 'sismember' command\r\n"));
            return false;
        }
        return true;
    }

    void executeCommand(const std::vector<std::string> &command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        auto redisSet = std::dynamic_pointer_cast<RedisSet>(dataStore);
        if (!redisSet) {
            session->send(std::move("-ERR operation against a key holding the wrong kind of value\r\n"));
            return;
        }

        const std::string &key = command[1];
        const std::string &value = command[2];

        bool isMember = redisSet->sismember(key, value);
        session->send(std::move(":" + std::to_string(isMember ? 1 : 0) + "\r\n"));
    }
};

// DBSIZE
class DBsizeParaser : public CommandParser {
public:
    explicit DBsizeParaser(std::shared_ptr<RedisHelper> redisHelper) 
        :CommandParser(std::move(redisHelper)) {}

private:
    bool parserCommand(const std::vector<std::string> &command, Session::Ptr session) override{
        if(command.size() != 1) {
            //DebugL << "Invalid DBSIZE command.";
            session->send("-ERR wrong number of arguments for 'dbsize' command\r\n");
            return false;
        }
        return true;
    }
    void executeCommand(const std::vector<std::string> &command, Session::Ptr session, RedisDataType::Ptr dataStore) override {
        auto response = redisHelper_->dbsize();
        session->send(":" + std::to_string(response) + "\r\n");
        //DebugL << "Found " << matched_keys.size() << " keys matching pattern: " << pattern;
    }
};


} // namespace toolkit

#endif


