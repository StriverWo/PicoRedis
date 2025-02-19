#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include <memory>
#include <unordered_map>
#include "DataType.h"

namespace toolkit
{
class DataManager {
public:
    using Ptr = std::shared_ptr<DataManager>;

    // // 获取 DataManager 单例
    // static Ptr instance() {
    //     static Ptr instance(new DataManager());
    //     return instance;
    // }

    ~DataManager() {
        persistDataToDisk(true);     // 析构时同步刷盘
    }

    DataManager(int dbIndex){
        persistenceManager_ = std::make_shared<PersistenceManager>(dbIndex);
    }

public:

    // 根据数据类型获取相应的 RedisDataType 的实例化指针
    RedisDataType::Ptr& getDataType(std::string& type) {
        auto it = dataStore_.find(type);
        if(it == dataStore_.end() || !it->second) {
            // 如果 type 对应的键不存在或为空，则根据类型创建
            if (type == "STRING") {
                createKey<RedisString>(type);
            } else if (type == "HASH") {
                createKey<RedisHash>(type);
            } else if (type == "LIST") {
                createKey<RedisList>(type);
            } else if (type == "SET") {
                createKey<RedisSet>(type);
            } else {
                throw std::invalid_argument("Unsupported Redis data type: " + type);
            }
        }
        return dataStore_[type];
    } 

    // 持久化所有数据到磁盘
    void persistDataToDisk(bool sync) {
        std::unordered_map<std::string, std::string> validiSnapshot;
        for (const auto& [key,value] : dataStore_) {
            validiSnapshot[key] = value->serialize();
        }
        persistenceManager_->persistToDisk(validiSnapshot, sync);
    }

    // 从磁盘加载数据到内存
    void loadDataFromDisk() {
        persistenceManager_->loadFromDisk(dataStore_);
    }

    // 创建一个新的键，指定类型
    template <typename T>
    void createKey(const std::string& key) {
        if (dataStore_.count(key)) {
            throw std::runtime_error("Key already exists with a different type");
        }
        dataStore_[key] = std::make_shared<T>();
    }

    // KEYS 命令调用
    std::vector<std::string> keys(const std::string& pattern) {
        std::vector<std::string> result;

        // 特殊模式 "*" 处理
        if (pattern == "*") {
            for (const auto& [key, data] : dataStore_) {
                auto allKeys = data->getAllKeys();
                result.insert(result.end(), allKeys.begin(), allKeys.end());
            }
            return result;
        }

        // 通配符转正则
        std::regex regexPattern(wildcardToRegex(pattern));

        // 正则匹配
        for (const auto& [key, data] : dataStore_) {
            auto matchedKeys = data->keys(regexPattern);
            result.insert(result.end(), matchedKeys.begin(), matchedKeys.end());
        }
        return result;
    }

    // DEL命令调用
    bool eraseKey(const std::string& key) {
        for (const auto& [type, data] : dataStore_) {
            if(data->search(key)) {
                data->erase(key);
                return true;
            }    
        }
        return false;
    }
    // 键总数（dbsize）
    int dbsize() {
        int sum = 0;
        for (const auto& [type, data] : dataStore_) {
            sum += data->getsize();
        }
        return sum;
    }
    // 搜索键
    bool searchKey(const std::string& key) {
        for (const auto& [type, data] : dataStore_) {
            if(data->search(key)) {
                return true;
            }    
        }
        return false;
    }
private:
    
    
    // 存储键值对，值是抽象的数据类型
    std::unordered_map<std::string, std::shared_ptr<RedisDataType>> dataStore_;
    PersistenceManager::Ptr persistenceManager_;    // 持久化管理器

    std::string regex_escape(const std::string& str) {
        static const std::unordered_set<char> specialChars = {
            '.', '\\', '+', '*', '?', '^', '$', '(', ')', '[', ']', '{', '}', '|'
        };
        
        std::string escapedStr;
        for (char c : str) {
            if (specialChars.count(c)) {
                escapedStr += '\\';  // 在特殊字符前添加反斜杠
            }
            escapedStr += c;
        }
        return escapedStr;
    }

    std::string wildcardToRegex(const std::string& pattern) {
        std::string regexPattern;
        for (char c : pattern) {
            if (c == '*') {
                regexPattern += ".*";  // '*' 转换为正则表达式的 '.*'
            } else if (c == '?') {
                regexPattern += '.';  // '?' 转换为正则表达式的 '.'
            } else {
                regexPattern += regex_escape(std::string(1, c));
            }
        }
        return regexPattern;
    }
};    
} // namespace toolkit


#endif