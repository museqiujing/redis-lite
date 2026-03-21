#include "timewheel.h"
#include <chrono>
#include <thread>

TimeWheel::TimeWheel(int granularity_ms, int wheel_size)
    : buckets(wheel_size),
      current_pos(0),
      current_time(std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count()),
      granularity(granularity_ms),
      wheel_size(wheel_size),
      running(false)
{
}

TimeWheel::~TimeWheel()
{
    stop();
}

void TimeWheel::add_key(const SDS &key, int64_t expire_at)
{
    std::lock_guard<std::mutex> lock(mutex);

    // 计算过期时间与当前时间的差值
    int64_t delta = expire_at - current_time;
    if (delta <= 0)
    {
        // 已经过期，直接返回
        return;
    }

    // 计算在时间轮中的位置
    size_t pos = (current_pos + delta / granularity) % wheel_size;

    // 添加到对应桶中
    buckets[pos].keys.insert(key);
}

void TimeWheel::remove_key(const SDS &key)
{
    std::lock_guard<std::mutex> lock(mutex);

    // 遍历所有桶查找并删除
    for (auto &bucket : buckets)
    {
        auto it = bucket.keys.find(key);
        if (it != bucket.keys.end())
        {
            bucket.keys.erase(it);
            break;
        }
    }
}

std::vector<SDS> TimeWheel::get_expired_keys()
{
    std::lock_guard<std::mutex> lock(mutex);

    // 更新当前时间
    current_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();

    // 获取当前桶中的所有键
    std::vector<SDS> expired_keys;
    for (const auto &key : buckets[current_pos].keys)
    {
        expired_keys.push_back(key);
    }

    // 清空当前桶
    buckets[current_pos].keys.clear();

    // 移动到下一个位置
    current_pos = (current_pos + 1) % wheel_size;

    return expired_keys;
}

void TimeWheel::start()
{
    if (running)
    {
        return;
    }

    running = true;

    // 启动时间轮线程
    std::thread([this]()
                {
        while (running) {
            // 等待一个时间粒度
            std::this_thread::sleep_for(std::chrono::milliseconds(granularity));
            
            // 获取过期键（在实际使用中，这个方法会被外部线程调用）
            // 这里只是保持时间轮转动
            get_expired_keys();
        } })
        .detach();
}

void TimeWheel::stop()
{
    running = false;
}
