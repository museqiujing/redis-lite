#include "string_type.h"
#include <chrono>

// 检查键是否过期
bool String::is_expired(const SDS &key) const
{
    auto it = storage.find(key);
    if (it == storage.end())
    {
        return true; // 键不存在，视为过期
    }

    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();

    return it->second->expire_at != -1 && it->second->expire_at < now;
}

// 清理过期键
void String::clean_expired(const SDS &key)
{
    if (is_expired(key))
    {
        storage.erase(key);
    }
}

// 设置键值对
void String::set(const SDS &key, const SDS &value)
{ // 使用左值引用
    auto it = storage.find(key);
    if (it != storage.end())
    {
        it->second->data = value; // 使用赋值运算符
        it->second->expire_at = -1;
    }
    else
    {
        storage[key] = std::make_shared<Value>(value, -1);
    }
}

// 设置键值对并添加过期时间（秒）
void String::setex(const SDS &key, int seconds, const SDS &value)
{
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();
    int64_t expire_at = now + seconds * 1000LL;
    storage[key] = std::make_shared<Value>(value, expire_at);
}

// 获取值
const SDS &String::get(const SDS &key)
{
    clean_expired(key);
    auto it = storage.find(key);
    if (it == storage.end())
    {
        static SDS empty_sds; // 静态空对象，避免重复创建
        return empty_sds;
    }
    return it->second->data; // 返回const引用
}

// 设置过期时间（秒）
bool String::expire(const SDS &key, int seconds)
{
    clean_expired(key);
    auto it = storage.find(key);
    if (it == storage.end())
    {
        return false;
    }

    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();
    it->second->expire_at = now + seconds * 1000LL;
    return true;
}

// 递增操作
long long String::incr(const SDS &key)
{
    return incrby(key, 1);
}

// 递减操作
long long String::decr(const SDS &key)
{
    return decrby(key, 1);
}

// 递增指定值
long long String::incrby(const SDS &key, long long increment)
{
    clean_expired(key);
    auto it = storage.find(key);

    long long value = 0;
    if (it != storage.end())
    {
        try
        {
            value = std::stoll(it->second->data);
        }
        catch (...)
        {
            throw std::runtime_error("value is not an integer or out of range");
        }
    }

    value += increment;
    storage[key] = std::make_shared<Value>(SDS(std::to_string(value).c_str()), -1);
    return value;
}

// 递减指定值
long long String::decrby(const SDS &key, long long decrement)
{
    return incrby(key, -decrement);
}

// 检查键是否存在
bool String::exists(const SDS &key)
{
    clean_expired(key);
    return storage.find(key) != storage.end();
}

// 删除键
bool String::del(const SDS &key)
{
    clean_expired(key);
    auto it = storage.find(key);
    if (it == storage.end())
    {
        return false;
    }
    storage.erase(it);
    return true;
}

// 获取键的剩余过期时间（秒）
int String::ttl(const SDS &key)
{
    clean_expired(key);
    auto it = storage.find(key);
    if (it == storage.end())
    {
        return -2; // 键不存在
    }

    if (it->second->expire_at == -1)
    {
        return -1; // 永不过期
    }

    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();
    int64_t ttl_ms = it->second->expire_at - now;
    if (ttl_ms < 0)
    {
        return 0; // 已过期
    }
    int ttl_seconds = static_cast<int>(ttl_ms / 1000);
    return ttl_seconds;
}

// 获取所有键值对
std::vector<std::pair<SDS, SDS>> String::get_all_data()
{
    std::vector<std::pair<SDS, SDS>> result;
    for (const auto &pair : storage)
    {
        // 跳过过期的键
        if (!is_expired(pair.first))
        {
            result.emplace_back(pair.first, pair.second->data);
        }
    }
    return result;
}
