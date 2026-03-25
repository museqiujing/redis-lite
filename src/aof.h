#ifndef AOF_H
#define AOF_H

#include <string>
#include <vector>
#include "sds.h"
#include "resp_parser.h"
#include <atomic>
#include <thread>
#include <chrono>
#include <sys/mman.h>
#include <sys/stat.h>
#include <condition_variable>
#include <queue>
#include <mutex>

class CommandHandler; // 前向声明

// 专用 AOF 高性能解析器（避免 RespParser 的性能开销）
class AofFastParser
{
public:
    // 直接解析内存映射的 AOF 数据
    static std::vector<SDS> parse_command(const char *data, size_t size, size_t &consumed_bytes);

    // 批量解析多个命令（更高性能）
    static std::vector<std::vector<SDS>> parse_commands_batch(const char *data, size_t size, size_t &consumed_bytes);
};

// AOF 持久化策略枚举
enum class AofPolicy
{
    ALWAYS,   // 每次写入都刷盘
    EVERYSEC, // 每秒刷盘一次
    NO        // 不主动刷盘，依赖操作系统
};

// 无锁环形缓冲区 - 用于 AOF 写入队列
template <typename T, size_t Capacity>
class LockFreeRingBuffer
{
private:
    alignas(64) std::atomic<size_t> head_{0}; // 写指针
    alignas(64) std::atomic<size_t> tail_{0}; // 读指针
    T buffer_[Capacity];

public:
    bool push(const T &item)
    {
        const size_t head = head_.load(std::memory_order_relaxed);
        const size_t next_head = (head + 1) % Capacity;

        if (next_head == tail_.load(std::memory_order_acquire))
        {
            return false; // 缓冲区满
        }

        buffer_[head] = item;
        head_.store(next_head, std::memory_order_release);
        return true;
    }

    bool pop(T &item)
    {
        const size_t tail = tail_.load(std::memory_order_relaxed);

        if (tail == head_.load(std::memory_order_acquire))
        {
            return false; // 缓冲区空
        }

        item = buffer_[tail];
        tail_.store((tail + 1) % Capacity, std::memory_order_release);
        return true;
    }

    bool empty() const
    {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }

    size_t size() const
    {
        const size_t head = head_.load(std::memory_order_acquire);
        const size_t tail = tail_.load(std::memory_order_acquire);
        return (head >= tail) ? (head - tail) : (Capacity - tail + head);
    }
};

// AOF 写入任务
struct AofWriteTask
{
    SDS data;
    bool needs_fsync; // 标记是否需要 fsync
};

class AofManager
{
private:
    SDS filename;
    int file_fd;
    bool enabled;
    CommandHandler *handler;
    AofPolicy sync_policy;
    std::atomic<bool> bg_thread_running;

    // 双缓冲机制：当前写入缓冲和后台刷新缓冲
    static const size_t WRITE_BUFFER_SIZE = 32 * 1024; // 32KB 单缓冲区
    SDS write_buffer[2];
    std::atomic<size_t> current_buffer{0};
    std::atomic<size_t> buffer_used[2]{0, 0};

    // 无锁写入队列（用于 ALWAYS 模式）
    static const size_t AOF_QUEUE_SIZE = 4096;
    LockFreeRingBuffer<AofWriteTask, AOF_QUEUE_SIZE> write_queue;

    // 后台线程
    std::thread fsync_thread; // 专职 fsync 线程
    std::thread write_thread; // 专职写入线程
    std::thread bg_thread;    // EVERYSEC 模式使用

    // 同步原语（仅用于 ALWAYS 模式的等待）
    std::mutex fsync_mutex;
    std::condition_variable fsync_cv;

    // 写入统计
    std::atomic<size_t> total_writes{0};
    std::atomic<size_t> total_fsyncs{0};

    // 重写相关配置
    size_t rewrite_base_size;                                     // 上次重写时的文件大小
    static const size_t AUTO_REWRITE_MIN_SIZE = 64 * 1024 * 1024; // 64MB 最小重写大小
    static constexpr double AUTO_REWRITE_PERCENT = 1.0;           // 100% 增长触发重写
    static const size_t AUTO_REWRITE_WRITE_COUNT = 100000;        // 写入次数阈值

    // 重写状态控制
    std::atomic<bool> rewrite_in_progress{false};      // 防止并发重写
    std::atomic<size_t> total_writes_since_rewrite{0}; // 重写后的写入次数统计

    // 后台线程函数
    void background_fsync_thread();
    void background_write_thread();
    void background_sync_thread(); // EVERYSEC 模式使用
    void background_rewrite_thread();

    // 上次刷盘时间
    std::chrono::steady_clock::time_point last_sync_time;
    std::chrono::steady_clock::time_point last_fsync_time;

public:
    AofManager(const SDS &file = "appendonly.aof", AofPolicy policy = AofPolicy::EVERYSEC);
    ~AofManager();

    // 初始化 AOF
    bool init(CommandHandler *cmd_handler);

    // 启用/禁用 AOF
    void set_enabled(bool enable);

    // 写入命令到 AOF 文件（优化版本）
    void write_command(const std::vector<SDS> &command);

    // 加载 AOF 文件
    bool load();

    // 重写 AOF 文件（手动触发）
    bool rewrite();

    // 检查是否需要自动重写
    bool should_auto_rewrite() const;

    // 获取当前 AOF 文件大小
    size_t get_current_size() const;

    // 获取重写基准大小
    size_t get_rewrite_base_size() const;

    // 是否正在重写
    bool is_rewrite_in_progress() const;

    // 缓冲区刷盘
    void flush_buffer();

    // 设置持久化策略
    void set_sync_policy(AofPolicy policy);

    // 获取当前持久化策略
    AofPolicy get_sync_policy() const;

    // 强制刷盘所有缓冲区（服务器关闭时调用）
    void flush_all_buffers();

    // 策略字符串转换
    static SDS policy_to_string(AofPolicy policy);
    static AofPolicy string_to_policy(const SDS &policy_str);

private:
    RespParser parser;
    // 打开 AOF 文件
    bool open_file();

    // 关闭 AOF 文件
    void close_file();

    // 将命令转换为 RESP 格式（优化版本，直接写入缓冲区）
    void append_command_to_buffer(SDS &buffer, const std::vector<SDS> &command);

    // 预分配的序列化缓冲区（线程局部）
    static thread_local SDS tls_serialize_buffer;
};

#endif // AOF_H
