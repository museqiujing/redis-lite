#ifndef AOF_H
#define AOF_H

#include <string>
#include <vector>
#include "sds.h"
#include "resp_parser.h"
#include <atomic>
#include <thread>
#include <chrono>
class CommandHandler; // 前向声明

// AOF持久化策略枚举
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

    // 后台刷盘线程
    void background_sync_thread();

    // 上次刷盘时间
    std::chrono::steady_clock::time_point last_sync_time;

public:
    AofManager(const SDS &file = "appendonly.aof", AofPolicy policy = AofPolicy::EVERYSEC);
    ~AofManager();

    // 初始化AOF
    bool init(CommandHandler *cmd_handler);

    // 启用/禁用AOF
    void set_enabled(bool enable);

    // 写入命令到AOF文件
    void write_command(const std::vector<SDS> &command);

    // 加载AOF文件
    bool load();

    // 重写AOF文件
    bool rewrite();

    // 设置持久化策略
    void set_sync_policy(AofPolicy policy);

    // 获取当前持久化策略
    AofPolicy get_sync_policy() const;

    // 策略字符串转换
    static SDS policy_to_string(AofPolicy policy);
    static AofPolicy string_to_policy(const SDS &policy_str);

private:
    RespParser parser;
    // 打开AOF文件
    bool open_file();

    // 关闭AOF文件
    void close_file();

    // 将命令转换为RESP格式
    SDS command_to_resp(const std::vector<SDS> &command);
};

#endif // AOF_H