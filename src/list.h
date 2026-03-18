#ifndef LIST_H
#define LIST_H

#include <unordered_map>
#include <string>
#include <memory>
#include <list>
#include "sds.h"
#include <vector>

class List {
private:
    struct Value {
        std::list<SDS> data;  // 存储链表数据
        int64_t expire_at;    // 过期时间戳（毫秒），-1表示永不过期
        
        Value(int64_t expire = -1) : expire_at(expire) {}
    };
    
    std::unordered_map<std::string, std::shared_ptr<Value>> storage;  // 键值存储
    
    // 检查键是否过期
    bool is_expired(const std::string& key) const;
    
    // 清理过期键
    void clean_expired(const std::string& key);

public:
    List() = default;
    ~List() = default;
    
    // 在链表头部插入元素
    void lpush(const std::string& key, const std::string& value);
    
    // 在链表尾部插入元素
    void rpush(const std::string& key, const std::string& value);
    
    // 移除并返回链表头部元素
    std::string lpop(const std::string& key);
    
    // 移除并返回链表尾部元素
    std::string rpop(const std::string& key);
    
    // 返回链表指定范围的元素
    std::vector<std::string> lrange(const std::string& key, int start, int stop);
    
    // 返回链表长度
    long long llen(const std::string& key);
    
     // 获取所有数据
    std::vector<std::pair<std::string, std::vector<std::string>>> get_all_data();
    
    // 设置过期时间（秒）
    bool expire(const std::string& key, int seconds);
    
    // 检查键是否存在
    bool exists(const std::string& key);
    
    // 删除键
    bool del(const std::string& key);
    
    // 获取键的剩余过期时间（秒）
    int ttl(const std::string& key);
};

#endif // LIST_H