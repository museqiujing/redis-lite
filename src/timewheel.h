#ifndef TIMEWHEEL_H
#define TIMEWHEEL_H

#include <unordered_set>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include "sds.h"

class TimeWheel
{
private:
    struct Bucket
    {
        std::unordered_set<SDS> keys;
    };

    std::vector<Bucket> buckets;
    size_t current_pos;
    int64_t current_time; // 毫秒时间戳
    int granularity;      // 时间粒度（毫秒）
    int wheel_size;       // 时间轮大小

    // 线程安全相关
    std::mutex mutex;
    std::condition_variable cv;
    bool running;

public:
    TimeWheel(int granularity_ms = 10, int wheel_size = 6000); // 10ms 粒度，10分钟轮
    ~TimeWheel();

    void add_key(const SDS &key, int64_t expire_at);
    void remove_key(const SDS &key);
    std::vector<SDS> get_expired_keys();
    void start();
    void stop();
};

#endif