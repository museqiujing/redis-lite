#ifndef STRING_H
#define STRING_H

#include <unordered_map>
#include <string>
#include <memory>
#include "sds.h"
#include <vector>
#include "timewheel.h"
#include <thread>

class String
{
private:
    struct Value
    {
        SDS data;          // 存储字符串数据
        int64_t expire_at; // 过期时间戳（毫秒），-1表示永不过期

        Value(SDS str, int64_t expire = -1)
            : data(std::move(str)), expire_at(expire) {}
    };

    std::unordered_map<SDS, std::shared_ptr<Value>> storage; // 键值存储

    // 检查键是否过期
    bool is_expired(const SDS &key) const;

    // 清理过期键
    void clean_expired(const SDS &key);

    // 新增成员
    TimeWheel time_wheel;
    std::thread expire_thread;
    std::mutex storage_mutex;
    bool running;

    // 启动过期线程
    void start_expire_thread();

public:
    String();
    ~String();

    // 设置键值对
    void set(const SDS &key, const SDS &value); // 使用左值引用

    // 设置键值对并添加过期时间（秒）
    void setex(const SDS &key, int seconds, const SDS &value);

    // 获取值
    const SDS &get(const SDS &key);

    // 设置过期时间（秒）
    bool expire(const SDS &key, int seconds);

    // 递增操作
    long long incr(const SDS &key);

    // 递减操作
    long long decr(const SDS &key);

    // 递增指定值
    long long incrby(const SDS &key, long long increment);

    // 递减指定值
    long long decrby(const SDS &key, long long decrement);

    // 检查键是否存在
    bool exists(const SDS &key);

    // 删除键
    bool del(const SDS &key);

    // 获取键的剩余过期时间（秒）
    int ttl(const SDS &key);

    // 获取所有String数据
    std::vector<std::pair<SDS, SDS>> get_all_data();
};

#endif // STRING_H