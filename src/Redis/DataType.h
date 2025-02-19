#ifndef DATATYPE_H
#define DATATYPE_H

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <regex>
#include "SkipList.h"
namespace toolkit
{
// 抽象的 Redis 数据类型接口
class RedisDataType {

public:

    using Ptr = std::shared_ptr<RedisDataType>;
    virtual ~RedisDataType() = default;

    // // 插入数据
    // virtual void insert(const std::vector<std::string>& args) = 0;
    // // 查询数据
    // virtual std::string get(const std::vector<std::string>& args) const = 0;
    // // 删除数据
    // virtual bool remove(const std::vector<std::string>& args) = 0;
    
    // 获取类型名称（如 "hash", "set", "list"）
    virtual std::string getType() const = 0;
    // 序列化和反序列化
    virtual std::string serialize() const = 0;
    virtual void deserialize(const std::string& data) = 0;
    // 获取匹配的键
    virtual std::vector<std::string> keys(const std::regex& regexPattern) const = 0;
    // 得到所有键
    virtual std::vector<std::string> getAllKeys() const = 0;
    // 搜索键是否存在
    virtual bool search(const std::string & key) const = 0;
    // 删除键
    virtual bool erase(const std::string& key) = 0;
    // 获得相应类型的键总数
    virtual int getsize() const = 0;

};

class RedisHash : public RedisDataType{
public:
    virtual std::string getType() const override;
    virtual std::vector<std::string> keys(const std::regex& regexPattern) const;
    std::vector<std::string> getAllKeys() const;
    virtual bool search(const std::string & key) const override;
    virtual bool erase(const std::string& key) override;
    // 序列化：将哈希表内容序列化为字符串
    std::string serialize() const override;

    // 反序列化：从字符串恢复哈希表
    void deserialize(const std::string& data) override;

    // HSET: 设置字段值
    void hset(const std::string& key, const std::string& field, const std::string& value);

    // HGET: 获取字段值
    std::shared_ptr<std::string> hget(const std::string& key, const std::string& field) const ;

    // HDEL: 删除字段
    bool hdel(const std::string& key, const std::string& field);

    // HGETALL: 获取所有字段及其值
    std::unordered_map<std::string, std::string> hgetall(const std::string& key) const ;

    int getsize() const {
        return hashData_.size();
    }
private:
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> hashData_;

};


class RedisSet : public RedisDataType {
private:
    std::unordered_map<std::string, std::unordered_set<std::string>> setData_;
public:
    // 序列化：将跳表内容序列化为字符串
    std::string serialize() const override;
    // 反序列化：从字符串恢复跳表
    void deserialize(const std::string& data) override ;
    // SADD: 添加元素到集合
    void sadd(const std::string& key, const std::string& value);
    // SREM: 从集合中删除元素
    bool srem(const std::string& key, const std::string& value);
    // SMEMBERS: 获取集合中的所有元素
    std::unordered_set<std::string> smembers(const std::string& key) const;
    // SISMEMBER: 检查元素是否在集合中
    bool sismember(const std::string& key, const std::string& value) const;
    
    // 获取类型名称（如 "hash", "set", "list"）
    virtual std::string getType() const override;
    virtual std::vector<std::string> keys(const std::regex& regexPattern) const ;
    std::vector<std::string> getAllKeys() const;
    virtual bool search(const std::string & key) const override;
    virtual bool erase(const std::string& key) override;
    int getsize() const {
        return setData_.size();
    }
};


class RedisList : public RedisDataType {
private:
    std::unordered_map<std::string, std::deque<std::string>> listData_;

public:
    virtual std::string getType() const override;
    // 序列化：将跳表内容序列化为字符串
    std::string serialize() const override;

    // 反序列化：从字符串恢复跳表
    void deserialize(const std::string& data) override;

    // LPUSH: 从左侧插入元素
    void lpush(const std::string& key, const std::string& value);
    // RPUSH: 从右侧插入元素
    void rpush(const std::string& key, const std::string& value);
    // LPOP: 从左侧弹出元素
    std::string lpop(const std::string& key);
    // RPOP: 从右侧弹出元素
    std::string rpop(const std::string& key);
    // LRANGE: 获取列表的范围
    std::vector<std::string> lrange(const std::string& key, int start, int end) const;

    virtual std::vector<std::string> keys(const std::regex& regexPattern) const ;
    
    std::vector<std::string> getAllKeys() const;
    virtual bool search(const std::string & key) const override;
    virtual bool erase(const std::string& key) override;
    
    int getsize() const {
        return listData_.size();
    }
};


class RedisString : public RedisDataType {
public:
    using Ptr = std::shared_ptr<RedisString>;

    RedisString() {
        skipList_ = std::make_shared<SkipList<std::string, std::string>>();
    }
    // 序列化：将跳表内容序列化为字符串
    std::string serialize() const override;
    // 反序列化：从字符串恢复跳表
    void deserialize(const std::string& data) override;
    // 插入或更新键值对
    void insert(const std::vector<std::string>& args);
    // 获取键的值
    std::shared_ptr<std::string> get(const std::vector<std::string>& args) const;
    // 删除指定键
    bool remove(const std::vector<std::string>& args);
    // 获取所有键
    std::vector<std::string> getAllKeys() const;
    // 获取数据类型名称
    std::string getType() const override;
    // 清空存储
    void clear();
    // 打印存储内容（调试用）
    void print() const;

    virtual std::vector<std::string> keys(const std::regex& regexPattern) const ;
    virtual bool search(const std::string & key) const override;
    virtual bool erase(const std::string& key) override;
    int getsize() const {
        return skipList_->size();
    }
private:
    // 使用 SkipList 实现键值存储
    std::shared_ptr<SkipList<std::string, std::string>> skipList_;

};


} // namespace toolkit

#endif