#ifndef REDISHELPER_H
#define REDISHELPER_H

#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <unordered_set>
#include "SkipList.h"
#include "PersistenceManager.h"
#include "DataManager.h"
#include "Global.h"

namespace toolkit {

class RedisHelper {
public:
    using Ptr = std::shared_ptr<RedisHelper>;
    // 获取 RedisHelper 单例
    static Ptr instance() {
        static Ptr instance(new RedisHelper());
        return instance;
    }

    // 根据传入的命令返回需要操作的相应数据类型（string/hash/set等）
    RedisDataType::Ptr getDataType(const std::string& cmd) {
        std::string type = cmdDataTypeMaps[cmd];
        if(type == "ALL") {
            return nullptr;
        }
        return dataManager_[currentDbIndex_]->getDataType(type);
    }


    // 获取匹配模式的所有键
    std::vector<std::string> keys(const std::string &pattern) {
        auto ret = dataManager_[currentDbIndex_]->keys(pattern);
        return ret;     // 按值返回：std::vector 支持移动语义，返回局部变量时编译器会优化为移动操作，不会有性能损失。
    }

    // 获取键总数（dbsize)
    int dbsize() {
        return dataManager_[currentDbIndex_]->dbsize();
    }
    // 删除键
    bool eraseKey(const std::string& key) {
        return dataManager_[currentDbIndex_]->eraseKey(key);
    }

    // 搜索建
    bool exists(const std::string& key) {
        return dataManager_[currentDbIndex_]->searchKey(key);
    }
    void persistChanges(bool sync = false) {
        dataManager_[currentDbIndex_]->persistDataToDisk(sync);
    }

    void loadFromStorage() {
        dataManager_[currentDbIndex_]->loadDataFromDisk();
    }

    // 切换当前数据库
    void selectDatabase(int dbIndex) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(dbIndex < 0 || dbIndex >= 16) {
            return ;
        }
        currentDbIndex_ = dbIndex;
        dataManager_[currentDbIndex_]->loadDataFromDisk();
    }

    ~RedisHelper() { };

private:
    // 因为辅助类 RedisHelper 只会存在单个实例，
    RedisHelper() : currentDbIndex_(0) {
        // 初始化16个数据库的数据管理器
        dataManager_.resize(16);
        for(int i = 0; i < 16; ++i) {
            dataManager_[i] = std::make_shared<DataManager>(i);
        }
    }

    int currentDbIndex_ = 0;
    std::vector<DataManager::Ptr> dataManager_;
    std::mutex mutex_;
    // DataManager::Ptr dataManager_;      // 数据管理器
};

} // namespace toolkit

#endif
