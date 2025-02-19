#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include <optional>
#include <regex>

namespace toolkit
{
    
template <typename K, typename V>
class SkipList {
private:
    struct Node {
        K key;
        V value;
        std::vector<std::shared_ptr<Node>> forward;

        Node(const K &k, const V &v, int level)
            : key(k), value(v), forward(level, nullptr) {}
    };

    std::shared_ptr<Node> head;
    int maxLevel;
    int level;
    float p;
    std::mt19937 generator;
    std::uniform_real_distribution<float> distribution;

    int randomLevel() {
        int lvl = 1;
        while (distribution(generator) < p && lvl < maxLevel) {
            lvl++;
        }
        return lvl;
    }
    

public:
    SkipList(int maxLevel = 16, float p = 0.5)
        : maxLevel(maxLevel), level(1), p(p), generator(std::random_device{}()), distribution(0.0, 1.0) {
        head = std::make_shared<Node>(K(), V(), maxLevel);
    }
    static std::shared_ptr<SkipList<K, V>> instance() {
        static std::shared_ptr<SkipList<K, V>> instance(new SkipList<K, V>());
        return instance;
    }
    void insert(const K &key, const V &value) {
        std::vector<std::shared_ptr<Node>> update(maxLevel);
        auto x = head;
        for (int i = level - 1; i >= 0; i--) {
            while (x->forward[i] && x->forward[i]->key < key) {
                x = x->forward[i];
            }
            update[i] = x;
        }
        // 检查 key 是否已存在
        x = x->forward[0];
        if (x && x->key == key) {
            // Key 已存在，直接更新值
            x->value = value;
            return;
        }

        int newLevel = randomLevel();
        if (newLevel > level) {
            for (int i = level; i < newLevel; i++) {
                update[i] = head;
            }
            level = newLevel;
        }

        auto newNode = std::make_shared<Node>(key, value, newLevel);
        for (int i = 0; i < newLevel; i++) {
            newNode->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = newNode;
        }
    }

    // Function to search for a key
    std::shared_ptr<V> search(const K &key) const {
        std::shared_ptr<Node> x = head;
        for (int i = level - 1; i >= 0; i--) {
            while (x->forward[i] && x->forward[i]->key < key) {
                x = x->forward[i];
            }
        }
        x = x->forward[0];
        if (x && x->key == key) {
            return std::make_shared<V>(x->value);  // Return a shared_ptr to the value
        }
        return nullptr;  // Return nullptr if not found
    }
    
    bool erase(const K &key) {
        std::vector<std::shared_ptr<Node>> update(maxLevel);
        auto x = head;
        for (int i = level - 1; i >= 0; i--) {
            while (x->forward[i] && x->forward[i]->key < key) {
                x = x->forward[i];
            }
            update[i] = x;
        }

        x = x->forward[0];
        if (x && x->key == key) {
            for (int i = 0; i < level; i++) {
                if (update[i]->forward[i] != x) {
                    break;
                }
                update[i]->forward[i] = x->forward[i];
            }

            while (level > 1 && !head->forward[level - 1]) {
                level--;
            }
            return true;
        }
        return false;
    }

    std::vector<K> getAllKeys() const {
        std::vector<K> keys;
        auto x = head->forward[0];
        while (x) {
            keys.emplace_back(x->key);
            x = x->forward[0];
        }
        return keys;
    }

    std::vector<std::pair<K, K>> getAll() const {
        std::vector<std::pair<K, K>> keys;
        auto x = head->forward[0];
        while (x) {
            keys.emplace_back(x->key,x->value);
            x = x->forward[0];
        }
        return keys;
    }
    void clear() {
        head = std::make_shared<Node>(K(), V(), maxLevel);
        level = 1;
    }

    bool isEmpty() const {
        return !head->forward[0];
    }

    void print() const {
        auto x = head->forward[0];
        while (x) {
            std::cout << x->key << ": " << x->value << std::endl;
            x = x->forward[0];
        }
    }
    // 新增：获取键的总数
    int size() const {
        int count = 0;
        auto x = head->forward[0];
        while (x) {
            count++;
            x = x->forward[0];
        }
        return count;
    }
};



} // namespace toolkit


#endif
