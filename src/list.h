#ifndef LIST_H
#define LIST_H

#include <unordered_map>
#include <string>
#include <memory>
#include <list>
#include "sds.h"
#include <vector>

class List
{
private:
    struct Value
    {
        std::list<SDS> data; // 存储链表数据
        int64_t expire_at;   // 过期时间戳（毫秒），-1表示永不过期

        Value(int64_t expire = -1) : expire_at(expire) {}
    };

    std::unordered_map<SDS, std::shared_ptr<Value>> storage; // 键值存储

    // 检查键是否过期
    bool is_expired(const SDS &key) const;

    // 清理过期键
    void clean_expired(const SDS &key);

public:
    List() = default;
    ~List() = default;

    // 在链表头部插入元素
    void lpush(const SDS &key, const SDS &value);

    // 在链表尾部插入元素
    void rpush(const SDS &key, const SDS &value);

    // 移除并返回链表头部元素
    SDS lpop(const SDS &key);

    // 移除并返回链表尾部元素
    SDS rpop(const SDS &key);

    // 返回链表指定范围的元素
    std::vector<SDS> lrange(const SDS &key, int start, int stop);

    // 返回链表长度
    long long llen(const SDS &key);

    // 获取所有数据
    std::vector<std::pair<SDS, std::vector<SDS>>> get_all_data();

    // 设置过期时间（秒）
    bool expire(const SDS &key, int seconds);

    // 检查键是否存在
    bool exists(const SDS &key);

    // 删除键
    bool del(const SDS &key);

    // 获取键的剩余过期时间（秒）
    int ttl(const SDS &key);
};

#endif // LIST_H