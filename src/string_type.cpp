#include "string_type.h"
#include <chrono>

// 检查键是否过期
bool String::is_expired(const SDS &key)
{
    auto it = storage.find(key);
    if (it == storage.end())
    {
        return true; // 键不存在，视为过期
    }

    // 使用无锁过期检查（如果启用）
    if (use_lockfree_)
    {
        return lockfree_manager.check_expiration(key, it->second->expire_at);
    }

    static int64_t last_check_time = 0;
    static int64_t cached_time = 0;
    static const int check_interval = 10; // 10ms

    int64_t now = cached_time;
    int64_t current_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now().time_since_epoch())
                               .count();

    if (current_time - last_check_time > check_interval)
    {
        cached_time = current_time;
        last_check_time = current_time;
        now = current_time;
    }

    return it->second->expire_at != -1 && it->second->expire_at < now;
}

// 清理过期键
void String::clean_expired(const SDS &key)
{
    if (is_expired(key))
    {
        storage.erase(key);
        // 如果使用无锁模式，触发批量清理
        if (use_lockfree_)
        {
            lockfree_manager.batch_cleanup();
        }
        else
        {
            // 从时间轮中移除
            time_wheel.remove_key(key);
        }
    }
}

String::String() : time_wheel(), lockfree_manager(), running(true)
{
    start_expire_thread();
}

String::~String()
{
    running = false;
    if (expire_thread.joinable())
    {
        expire_thread.join();
    }
}

// 启动过期清理线程
void String::start_expire_thread()
{
    expire_thread = std::thread([this]()
                                {
        while (running)
        {
            // 每10ms检查一次
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (use_lockfree_)
            {
                // 无锁模式：触发批量清理
                lockfree_manager.batch_cleanup();
                
                // 注意：无锁管理器只负责标记过期，实际删除在GET操作时进行
                // 这样可以避免锁竞争，提高性能
            }
            else
            {
                // 时间轮模式：获取过期的键并批量删除
                auto expired_keys = time_wheel.get_expired_keys();
                
                // 批量删除过期键
                std::lock_guard<std::mutex> lock(storage_mutex);
                for (const auto &key : expired_keys)
                {
                    storage.erase(key);
                }
            }
        } });
}

// 设置键值对
void String::set(const SDS &key, const SDS &value)
{ // 使用左值引用
    std::lock_guard<std::mutex> lock(storage_mutex);
    auto it = storage.find(key);
    if (it != storage.end())
    {
        it->second->data = value; // 使用赋值运算符
        it->second->expire_at = -1;

        // 如果之前有过期时间，需要从管理器中移除
        if (use_lockfree_)
        {
            // 无锁管理器会自动处理永不过期的键
            // 这里不需要额外操作
        }
        else
        {
            // 从时间轮中移除
            time_wheel.remove_key(key);
        }
    }
    else
    {
        storage[key] = std::make_shared<Value>(value, -1);
    }
}

// 设置键值对并添加过期时间（秒）
void String::setex(const SDS &key, int seconds, const SDS &value)
{
    std::lock_guard<std::mutex> lock(storage_mutex);
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();
    int64_t expire_at = now + seconds * 1000LL;
    storage[key] = std::make_shared<Value>(value, expire_at);
    // 优先使用无锁管理器
    if (use_lockfree_)
    {
        lockfree_manager.add_expiration(key, expire_at);
    }
    else
    {
        // 回退到时间轮
        time_wheel.add_key(key, expire_at);
    }
}

// 获取值
const SDS &String::get(const SDS &key)
{
    auto it = storage.find(key);
    if (it == storage.end())
    {
        static SDS empty_sds; // 静态空对象，避免重复创建
        return empty_sds;
    }

    // 检查是否过期
    if (is_expired(key))
    {
        storage.erase(it);
        static SDS empty_sds;
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
    int64_t expire_at = now + seconds * 1000LL;
    it->second->expire_at = expire_at;
    // 优先使用无锁管理器
    if (use_lockfree_)
    {
        lockfree_manager.add_expiration(key, expire_at);
    }
    else
    {
        // 回退到时间轮
        time_wheel.add_key(key, expire_at);
    }
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
    std::lock_guard<std::mutex> lock(storage_mutex);
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
    // 从时间轮中移除（因为设置为永不过期）
    time_wheel.remove_key(key);
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
    std::lock_guard<std::mutex> lock(storage_mutex);
    clean_expired(key);
    return storage.find(key) != storage.end();
}

// 删除键
bool String::del(const SDS &key)
{
    std::lock_guard<std::mutex> lock(storage_mutex);
    auto it = storage.find(key);
    if (it == storage.end())
    {
        return false;
    }
    // 如果键有过期时间，需要从管理器中移除
    if (it->second->expire_at != -1)
    {
        if (use_lockfree_)
        {
            // 无锁管理器会自动处理删除的键
            // 这里不需要额外操作，因为键被删除后不会再次被检查
        }
        else
        {
            // 从时间轮中移除
            time_wheel.remove_key(key);
        }
    }

    storage.erase(it);

    return true;
}

// 获取键的剩余过期时间（秒）
int String::ttl(const SDS &key)
{
    // std::lock_guard<std::mutex> lock(storage_mutex);
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
    std::lock_guard<std::mutex> lock(storage_mutex);
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
