#ifndef PERSISTENCEMANAGER_H
#define PERSISTENCEMANAGER_H

#include <string>
#include <unordered_set>
#include <memory>
#include <mutex>
#include "leveldb/db.h"
#include "leveldb/write_batch.h"
#include "DataType.h"

namespace toolkit {

class PersistenceManager {
public:
    using Ptr = std::shared_ptr<PersistenceManager>;


    ~PersistenceManager() {
        delete db_;
    }
    PersistenceManager(int dbIndex) {
        leveldb::Options options;
        options.create_if_missing = true;
        std::string dbName = "./data/db"  + std::to_string(dbIndex);
        leveldb::Status status = leveldb::DB::Open(options, dbName, &db_);
        if (!status.ok()) {
            throw std::runtime_error("Failed to open LevelDB");
        }
    }
    // // 获取PersistenceManager单例
    // static Ptr instance() {
    //     static Ptr instance(new PersistenceManager());
    //     return instance;
    // }

    // 持久化数据
    void persistToDisk(std::unordered_map<std::string, std::string>& dataStore, bool sync = false) {
        DebugL << "persistToDisk is executed!";
        std::lock_guard<std::mutex> lock(mutex_);
        // 使用 WriteBatch 批量写入
        leveldb::WriteBatch batch;

        for (const auto& [key, data] : dataStore) {
            batch.Put(key, data);  // 将数据序列化后存入batch
        }

        leveldb::WriteOptions options;
        options.sync = sync;

        // 执行批量操作
        leveldb::Status status = db_->Write(options, &batch);
        if (!status.ok()) {
            throw std::runtime_error("Failed to persist data to LevelDB");
        }
    }

    // 从磁盘加载数据
    void loadFromDisk(std::unordered_map<std::string, std::shared_ptr<RedisDataType>>& dataStore) {
        std::lock_guard<std::mutex> lock(mutex_);
        leveldb::Iterator* it = db_->NewIterator(leveldb::ReadOptions());

        for (it->SeekToFirst(); it->Valid(); it->Next()) {
            std::string key = it->key().ToString();
            std::string serializeData = it->value().ToString();
            
            // 后续可以增加类型
            if(key == "STRING") {
                auto redisString = std::make_shared<RedisString>();
                redisString->deserialize(serializeData);
                dataStore[key] = redisString;
            }else if(key == "HASH") {
                auto redisHash = std::make_shared<RedisHash>();
                redisHash->deserialize(serializeData);
                dataStore[key] = redisHash;
            }else if(key == "SET") {
                auto redisSet = std::make_shared<RedisSet>();
                redisSet->deserialize(serializeData);
                dataStore[key] = redisSet;
            }else if(key == "LIST") {
                auto redisList = std::make_shared<RedisList>();
                redisList->deserialize(serializeData);
                dataStore[key] = redisList;
            }else {
                throw std::runtime_error("Failed to load data form disk.");
            }
        }
        delete it;
    }

private:
    

    leveldb::DB* db_;
    std::mutex mutex_;
};

} // namespace toolkit

#endif
