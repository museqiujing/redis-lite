#include "lockfree_expiration.h"
#include <iostream>
#include <thread>

LockFreeExpirationManager::LockFreeExpirationManager()
    : expiration_queue_(QUEUE_SIZE)
{
    // 初始化队列为空
    for (auto &entry : expiration_queue_)
    {
        entry.store(nullptr, std::memory_order_relaxed);
    }

    // 初始化时间缓存
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();
    cached_time_.store(now, std::memory_order_relaxed);
    last_update_.store(now, std::memory_order_relaxed);
}

LockFreeExpirationManager::~LockFreeExpirationManager()
{
    // 清理队列中的所有条目
    size_t head = queue_head_.load(std::memory_order_acquire);
    size_t tail = queue_tail_.load(std::memory_order_acquire);

    while (head != tail)
    {
        ExpirationEntry *entry = expiration_queue_[head].load(std::memory_order_acquire);
        if (entry)
        {
            delete entry;
            expiration_queue_[head].store(nullptr, std::memory_order_relaxed);
        }
        head = (head + 1) % QUEUE_SIZE;
    }
}

void LockFreeExpirationManager::add_expiration(const SDS &key, int64_t expire_at)
{
    if (expire_at == -1)
        return; // 永不过期

    ExpirationEntry *new_entry = new ExpirationEntry(key, expire_at);

    while (true)
    {
        size_t tail = queue_tail_.load(std::memory_order_acquire);
        size_t next_tail = (tail + 1) % QUEUE_SIZE;

        // 检查队列是否已满
        if (next_tail == queue_head_.load(std::memory_order_acquire))
        {
            // 队列满，触发批量清理
            batch_cleanup();
            continue;
        }

        // 尝试CAS操作
        ExpirationEntry *expected = nullptr;
        if (expiration_queue_[tail].compare_exchange_weak(
                expected, new_entry, std::memory_order_release, std::memory_order_relaxed))
        {
            // 成功添加，更新尾指针
            queue_tail_.store(next_tail, std::memory_order_release);
            break;
        }

        // CAS失败，重试
        std::this_thread::yield();
    }
}

bool LockFreeExpirationManager::check_expiration(const SDS &key, int64_t expire_at)
{
    if (expire_at == -1)
        return false; // 永不过期

    int64_t now = get_cached_time();
    return expire_at < now;
}

void LockFreeExpirationManager::batch_cleanup()
{
    size_t head = queue_head_.load(std::memory_order_acquire);
    size_t tail = queue_tail_.load(std::memory_order_acquire);
    int64_t now = get_cached_time();

    size_t cleaned_count = 0;

    while (head != tail && cleaned_count < 1000)
    { // 每次最多清理1000个
        ExpirationEntry *entry = expiration_queue_[head].load(std::memory_order_acquire);

        if (entry == nullptr)
        {
            // 空槽位，跳过
            head = (head + 1) % QUEUE_SIZE;
            continue;
        }

        if (entry->processed.load(std::memory_order_acquire))
        {
            // 已处理，清理并跳过
            delete entry;
            expiration_queue_[head].store(nullptr, std::memory_order_relaxed);
            head = (head + 1) % QUEUE_SIZE;
            cleaned_count++;
            continue;
        }

        // 检查是否过期
        int64_t expire_at = entry->expire_at.load(std::memory_order_acquire);
        if (expire_at < now)
        {
            // 标记为已处理
            entry->processed.store(true, std::memory_order_release);

            // 这里应该触发实际的键删除操作
            // 在实际实现中，这里会调用存储系统的删除方法
            std::cout << "[DEBUG] Key expired: " << entry->key.c_str()
                      << " at " << expire_at << " (now: " << now << ")" << std::endl;

            cleaned_count++;
        }

        head = (head + 1) % QUEUE_SIZE;
    }

    // 更新头指针
    if (head != queue_head_.load(std::memory_order_acquire))
    {
        queue_head_.store(head, std::memory_order_release);
    }
}

int64_t LockFreeExpirationManager::get_cached_time()
{
    int64_t current = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();

    // 每100ms更新一次缓存，减少系统调用
    if (current - last_update_.load(std::memory_order_acquire) > 100)
    {
        cached_time_.store(current, std::memory_order_release);
        last_update_.store(current, std::memory_order_release);
    }

    return cached_time_.load(std::memory_order_acquire);
}

size_t LockFreeExpirationManager::get_queue_size() const
{
    size_t head = queue_head_.load(std::memory_order_acquire);
    size_t tail = queue_tail_.load(std::memory_order_acquire);

    if (tail >= head)
    {
        return tail - head;
    }
    else
    {
        return QUEUE_SIZE - head + tail;
    }
}

size_t LockFreeExpirationManager::get_pending_cleanup_count() const
{
    size_t count = 0;
    size_t head = queue_head_.load(std::memory_order_acquire);
    size_t tail = queue_tail_.load(std::memory_order_acquire);

    while (head != tail)
    {
        ExpirationEntry *entry = expiration_queue_[head].load(std::memory_order_acquire);
        if (entry && !entry->processed.load(std::memory_order_acquire))
        {
            count++;
        }
        head = (head + 1) % QUEUE_SIZE;
    }

    return count;
}