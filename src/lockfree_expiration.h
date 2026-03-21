#ifndef LOCKFREE_EXPIRATION_H
#define LOCKFREE_EXPIRATION_H

#include "sds.h"
#include <atomic>
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>

// 无锁过期管理器
class LockFreeExpirationManager
{
private:
    // 原子时间戳缓存
    std::atomic<int64_t> cached_time_{0};
    std::atomic<int64_t> last_update_{0};

    // 无锁过期队列
    struct ExpirationEntry
    {
        SDS key;
        std::atomic<int64_t> expire_at;
        std::atomic<bool> processed{false};

        ExpirationEntry(const SDS &k, int64_t expire)
            : key(k), expire_at(expire) {}
    };

    static constexpr size_t QUEUE_SIZE = 65536; // 64K队列大小
    std::vector<std::atomic<ExpirationEntry *>> expiration_queue_;
    std::atomic<size_t> queue_head_{0};
    std::atomic<size_t> queue_tail_{0};

    // 分片存储引用
    static constexpr int SHARD_COUNT = 16;

public:
    LockFreeExpirationManager();
    ~LockFreeExpirationManager();

    // 核心接口
    void add_expiration(const SDS &key, int64_t expire_at);
    bool check_expiration(const SDS &key, int64_t expire_at);
    void batch_cleanup();
    int64_t get_cached_time();

    // 统计信息
    size_t get_queue_size() const;
    size_t get_pending_cleanup_count() const;
};

#endif // LOCKFREE_EXPIRATION_H