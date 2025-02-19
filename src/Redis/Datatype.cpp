
#include "DataType.h"

namespace toolkit
{


template <typename MapType>
std::vector<std::string> matchKeys(const MapType& container, const std::regex& regexPattern) {
    std::vector<std::string> result;
    for (const auto& [key, _] : container) {
        if (std::regex_match(key, regexPattern)) {
            result.push_back(key);
        }
    }
    return result;
}
// 
std::vector<std::string> RedisHash::keys(const std::regex& regexPattern) const {
    return matchKeys(hashData_, regexPattern);
}

std::vector<std::string> RedisSet::keys(const std::regex& regexPattern) const {
    return matchKeys(setData_, regexPattern);
}

std::vector<std::string> RedisList::keys(const std::regex& regexPattern) const {
    return matchKeys(listData_, regexPattern);
}

std::vector<std::string> RedisString::keys(const std::regex& regexPattern) const {
    std::vector<std::string> matchcingKeys;
    auto keys = skipList_->getAllKeys();
    for(const auto& key : keys) {
        if(std::regex_match(key, regexPattern)){
            matchcingKeys.push_back(key);
        }
    }
    return matchcingKeys;
}


// search 方法
bool RedisHash::search(const std::string& key) const {
    return hashData_.find(key) != hashData_.end();
}
bool RedisSet::search(const std::string& key) const {
    return setData_.find(key) != setData_.end();
}
bool RedisList::search(const std::string& key) const {
    return listData_.find(key) != listData_.end();
}
bool RedisString::search(const std::string& key) const {
    if(skipList_->search(key)){
        return true;
    }
    return false;
}

// erase 方法

bool RedisHash::erase(const std::string& key) {
    return hashData_.erase(key) > 0;
}

bool RedisSet::erase(const std::string& key) {
    return setData_.erase(key) > 0;
}

bool RedisList::erase(const std::string& key) {
    return listData_.erase(key) > 0;
}


bool RedisString::erase(const std::string& key) {
    return skipList_->erase(key);
}


/////////////////////////////////////////////////////////////////////////////////////////////

// 序列化：将哈希表中的内容序列化为字符串
std::string RedisHash::serialize() const {
    std::string serializedData;

    for (const auto& keyPair : hashData_) {
        const std::string& key = keyPair.first;
        const auto& fields = keyPair.second;

        for (const auto& fieldPair : fields) {
            const std::string& field = fieldPair.first;
            const std::string& value = fieldPair.second;

            // 将每个键值对以 "key:field:value" 的格式追加到字符串中，字段间用 `|` 分隔
            serializedData += key + "|" + field + "|" + value + "\n";
        }
    }

    return serializedData;
}

// 反序列化：从字符串恢复哈希表
void RedisHash::deserialize(const std::string& data) {
    // 清空当前数据
    hashData_.clear();

    // 按行拆分
    std::istringstream stream(data);
    std::string line;

    while (std::getline(stream, line)) {
        // 每一行应该是 "key|field|value" 格式
        size_t firstDelim = line.find('|');
        size_t secondDelim = line.find('|', firstDelim + 1);

        if (firstDelim == std::string::npos || secondDelim == std::string::npos) {
            // 跳过格式不正确的行
            continue;
        }

        std::string key = line.substr(0, firstDelim);
        std::string field = line.substr(firstDelim + 1, secondDelim - firstDelim - 1);
        std::string value = line.substr(secondDelim + 1);

        // 恢复到 hashData_
        hashData_[key][field] = value;
    }
}

// HSET: 设置字段值
void RedisHash::hset(const std::string& key, const std::string& field, const std::string& value) {
    hashData_[key][field] = value;
}
// HGET: 获取字段值
std::shared_ptr<std::string> RedisHash::hget(const std::string& key, const std::string& field) const {
    auto it = hashData_.find(key);
    if (it != hashData_.end()) {
        auto fieldIt = it->second.find(field);
        if (fieldIt != it->second.end()) {
            return std::make_shared<std::string>(fieldIt->second);
        }
    }
    return nullptr;
}
// HDEL: 删除字段
bool RedisHash::hdel(const std::string& key, const std::string& field) {
    auto it = hashData_.find(key);
    if (it != hashData_.end()) {
        return it->second.erase(field) > 0;
    }
    return false;
}
// HGETALL: 获取所有字段及其值
std::unordered_map<std::string, std::string> RedisHash::hgetall(const std::string& key) const {
    auto it = hashData_.find(key);
    if (it != hashData_.end()) {
        return it->second;
    }
    return {};
}
// 获取数据类型名称
std::string RedisHash::getType() const {
    return "HASH";
}


///////////////////////////////////////////////////////////////////////////////////////////

std::string RedisSet::serialize() const {
    std::string serializedData;

    for (const auto& keyPair : setData_) {
        const std::string& key = keyPair.first;
        const auto& values = keyPair.second;

        // 添加 key
        serializedData += key + "|";

        // 添加 values，用逗号分隔
        for (auto it = values.begin(); it != values.end(); ++it) {
            if (it != values.begin()) {
                serializedData += ",";
            }
            serializedData += *it;
        }

        // 每个键值对结束加换行符
        serializedData += "\n";
    }

    return serializedData;
}
void RedisSet::deserialize(const std::string& data) {
    // 清空当前数据
    setData_.clear();

    // 按行拆分
    std::istringstream stream(data);
    std::string line;

    while (std::getline(stream, line)) {
        // 找到第一个分隔符 '|'
        size_t delimPos = line.find('|');
        if (delimPos == std::string::npos) {
            // 跳过格式不正确的行
            continue;
        }

        // 提取 key 和 values
        std::string key = line.substr(0, delimPos);
        std::string valuesStr = line.substr(delimPos + 1);

        // 按逗号分隔解析 values
        std::unordered_set<std::string> values;
        std::istringstream valuesStream(valuesStr);
        std::string value;

        while (std::getline(valuesStream, value, ',')) {
            values.insert(value);
        }

        // 将解析结果加入到 setData_
        setData_[key] = std::move(values);
    }
}


// SADD: 添加元素到集合
void RedisSet::sadd(const std::string& key, const std::string& value) {
    setData_[key].insert(value);
}

// SREM: 从集合中删除元素
bool RedisSet::srem(const std::string& key, const std::string& value) {
    auto it = setData_.find(key);
    if (it != setData_.end()) {
        return it->second.erase(value) > 0;
    }
    return false;
}

// SMEMBERS: 获取集合中的所有元素
std::unordered_set<std::string> RedisSet::smembers(const std::string& key) const {
    auto it = setData_.find(key);
    if (it != setData_.end()) {
        return it->second;
    }
    return {};
}

// SISMEMBER: 检查元素是否在集合中
bool RedisSet::sismember(const std::string& key, const std::string& value) const {
    auto it = setData_.find(key);
    if (it != setData_.end()) {
        return it->second.find(value) != it->second.end();
    }
    return false;
}

// 获取数据类型名称
std::string RedisSet::getType() const {
    return "SET";
}


/////////////////////////////////////////////////////////////////////////////////////

// 序列化：将列表内容序列化为字符串
std::string RedisList::serialize() const {
    std::ostringstream oss;

    // 遍历所有键值对
    for (const auto& [key, deque] : listData_) {
        // 将每个列表序列化为 "key|value1,value2,value3" 格式
        oss << key << "|";
        for (size_t i = 0; i < deque.size(); ++i) {
            oss << deque[i];
            if (i != deque.size() - 1) {
                oss << ","; // 用逗号分隔列表的值
            }
        }
        oss << "\n"; // 每个列表用换行符分隔
    }

    return oss.str();
}

// 反序列化：从字符串恢复列表
void RedisList::deserialize(const std::string& data) {
    // 清空当前数据
    listData_.clear();

    // 按行拆分
    std::istringstream stream(data);
    std::string line;

    while (std::getline(stream, line)) {
        // 每一行应该是 "key|value1,value2,value3" 格式
        size_t delimPos = line.find('|');
        if (delimPos == std::string::npos) {
            // 跳过格式不正确的行
            continue;
        }

        std::string key = line.substr(0, delimPos);
        std::string values = line.substr(delimPos + 1);

        // 按逗号分割列表值
        std::istringstream valueStream(values);
        std::string value;
        std::deque<std::string> deque;

        while (std::getline(valueStream, value, ',')) {
            deque.push_back(value);
        }

        // 恢复到 listData_
        listData_[key] = std::move(deque);
    }
}


// LPUSH: 从左侧插入元素
void RedisList::lpush(const std::string& key, const std::string& value) {
    listData_[key].push_front(value);
}

// RPUSH: 从右侧插入元素
void RedisList::rpush(const std::string& key, const std::string& value) {
    listData_[key].push_back(value);
}

// LPOP: 从左侧弹出元素
std::string RedisList::lpop(const std::string& key) {
    auto it = listData_.find(key);
    if (it != listData_.end() && !it->second.empty()) {
        std::string value = it->second.front();
        it->second.pop_front();
        return value;
    }
    return "";
}

// RPOP: 从右侧弹出元素
std::string RedisList::rpop(const std::string& key) {
    auto it = listData_.find(key);
    if (it != listData_.end() && !it->second.empty()) {
        std::string value = it->second.back();
        it->second.pop_back();
        return value;
    }
    return "";
}

// LRANGE: 获取列表的范围
std::vector<std::string> RedisList::lrange(const std::string& key, int start, int end) const {
    auto it = listData_.find(key);
    std::vector<std::string> result;
    if (it != listData_.end()) {
        auto& deque = it->second;
        int size = static_cast<int>(deque.size());
        start = (start < 0) ? size + start : start;
        end = (end < 0) ? size + end : end;
        start = std::max(0, std::min(size, start));
        end = std::max(0, std::min(size - 1, end));

        for (int i = start; i <= end; ++i) {
            result.push_back(deque[i]);
        }
    }
    return result;
}

// 获取数据类型名称
std::string RedisList::getType() const {
    return "LIST";
}



////////////////////////////////////////////////////////////////////////////////////////////

// 序列化：将跳表内容序列化为字符串
std::string RedisString::serialize() const {
    std::ostringstream oss;
    for (const auto& [key, value] : skipList_->getAll()) {
        oss << key << "=" << value << ";";
    }
    return oss.str();
}

// 反序列化：从字符串恢复跳表
void RedisString::deserialize(const std::string& data) {
    skipList_->clear();
    std::istringstream iss(data);
    std::string pair;

    while (std::getline(iss, pair, ';')) {
        auto eqPos = pair.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key = pair.substr(0, eqPos);
        std::string value = pair.substr(eqPos + 1);
        skipList_->insert(key, value);
    }
}

// 插入或更新键值对
void RedisString::insert(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        throw std::invalid_argument("RedisString insert requires exactly 2 arguments: key and value");
    }
    skipList_->insert(args[0], args[1]);
}

// 获取键的值
std::shared_ptr<std::string> RedisString::get(const std::vector<std::string>& args) const {
    if (args.size() != 1) {
        throw std::invalid_argument("RedisString get requires exactly 1 argument: key");
    }
    return skipList_->search(args[0]);
}



// 删除指定键
bool RedisString::remove(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        throw std::invalid_argument("RedisString remove requires exactly 1 argument: key");
    }
    return skipList_->erase(args[0]);
}


// RedisList::getAllKeys
std::vector<std::string> RedisList::getAllKeys() const {
    std::vector<std::string> keys;
    for (const auto& [key, _] : listData_) {
        keys.push_back(key);
    }
    return keys;
}

// RedisSet::getAllKeys
std::vector<std::string> RedisSet::getAllKeys() const {
    std::vector<std::string> keys;
    for (const auto& [key, _] : setData_) {
        keys.push_back(key);
    }
    return keys;
}

// RedisHash::getAllKeys
std::vector<std::string> RedisHash::getAllKeys() const {
    std::vector<std::string> keys;
    for (const auto& [key, _] : hashData_) {
        keys.push_back(key);
    }
    return keys;
}

// 获取所有键
std::vector<std::string> RedisString::getAllKeys() const {
    return skipList_->getAllKeys();
}

// 获取数据类型名称
std::string RedisString::getType() const {
    return "STRING";
}

// 清空存储
void RedisString::clear() {
    skipList_->clear();
}

// 打印存储内容（调试用）
void RedisString::print() const {
    skipList_->print();
}

    
} // namespace toolkit
