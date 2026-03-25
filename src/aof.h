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

class AofManager
{
private:
    SDS filename;
    int file_fd;
    bool enabled;
    CommandHandler *handler;
    AofPolicy sync_policy;
    std::atomic<bool> bg_thread_running;
    std::thread bg_thread;

    // 高性能写入优化配置
    static const size_t BUFFER_SIZE_LIMIT = 64 * 1024;      // 64KB 缓冲区
    static const size_t BATCH_WRITE_THRESHOLD = 100;        // 批量写入阈值
    static const size_t MAX_THREAD_BUFFER_SIZE = 16 * 1024; // 每个线程 16KB 缓冲区

    // 主写入缓冲区（由后台线程管理）
    SDS write_buffer;
    size_t buffer_size;

    // 高性能写入优化：线程局部缓冲区池
    struct ThreadLocalBuffer
    {
        SDS buffer;
        size_t size;
        std::atomic<size_t> ref_count{0};
    };

    // 原子操作优化
    std::atomic<size_t> pending_writes{0};
    std::atomic<bool> flush_scheduled{false};

    // 重写相关配置
    size_t rewrite_base_size;                                     // 上次重写时的文件大小
    static const size_t AUTO_REWRITE_MIN_SIZE = 64 * 1024 * 1024; // 64MB 最小重写大小
    static constexpr double AUTO_REWRITE_PERCENT = 1.0;           // 100% 增长触发重写
    static const size_t AUTO_REWRITE_WRITE_COUNT = 100000;        // 写入次数阈值

    // 重写状态控制
    std::atomic<bool> rewrite_in_progress{false};      // 防止并发重写
    std::atomic<size_t> total_writes_since_rewrite{0}; // 重写后的写入次数统计

    // 后台刷盘线程
    void background_sync_thread();

    // 后台重写线程
    void background_rewrite_thread();

    // 上次刷盘时间
    std::chrono::steady_clock::time_point last_sync_time;

public:
    AofManager(const SDS &file = "appendonly.aof", AofPolicy policy = AofPolicy::EVERYSEC);
    ~AofManager();

    // 初始化 AOF
    bool init(CommandHandler *cmd_handler);

    // 启用/禁用 AOF
    void set_enabled(bool enable);

    // 写入命令到 AOF 文件
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

    // 将命令转换为 RESP 格式
    SDS command_to_resp(const std::vector<SDS> &command);
};

#endif // AOF_H
