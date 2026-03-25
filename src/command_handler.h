#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <vector>
#include <string>
#include "string_type.h"
#include "list.h"
#include "skiplist.h"
#include "aof.h"
#include "zset.h"
#include "sds.h"
#include <unordered_map>
#include <unordered_set>
class CommandHandler
{
public:
    CommandHandler(AofPolicy policy = AofPolicy::NO);
    SDS handle_command(const std::vector<SDS> &command);

    std::vector<SDS> handle_commands_batch(const std::vector<std::vector<SDS>> &commands);

    // AOF管理方法
    void set_aof_policy(AofPolicy policy);
    // AOF写入控制方法
    bool is_aof_enabled() const { return aof_policy != AofPolicy::NO; }
    AofPolicy get_current_aof_policy() const { return aof_policy; }

    // 获取所有String数据
    std::vector<std::pair<SDS, SDS>> get_all_string_data();

    // 获取所有List数据
    std::vector<std::pair<SDS, std::vector<SDS>>> get_all_list_data();

    // 获取所有ZSet数据
    std::vector<std::pair<SDS, std::vector<std::pair<double, SDS>>>> get_all_zset_data();

private:
    // 命令处理函数类型
    typedef SDS (CommandHandler::*CommandHandlerFunc)(const std::vector<SDS> &);

    // 静态命令映射表声明
    static std::unordered_map<SDS, CommandHandlerFunc> command_map;

    // 写命令集合声明
    static std::unordered_set<SDS> write_commands;

    String string_type;
    List list_type;
    SkipList skiplist_type;
    AofManager aof_manager;
    ZSetManager zset_manager;
    AofPolicy aof_policy; // AOF策略
    SDS response;         // 响应字符串

    // 响应缓冲区池
    static thread_local std::vector<SDS> response_buffer_pool;
    static SDS get_buffer_from_pool();
    static void return_buffer_to_pool(SDS &buffer);
    static void init_buffer_pool();

    // 辅助方法：处理写命令的AOF写入
    void handle_write_command_aof(const std::vector<SDS> &command);
    bool should_write_aof() const;
    bool is_write_command(const SDS &cmd) const;

    // 处理命令
    SDS handle_bgrewriteaof(const std::vector<SDS> &args);
    SDS handle_get(const std::vector<SDS> &args);
    SDS handle_set(const std::vector<SDS> &args);
    SDS handle_setex(const std::vector<SDS> &args);
    SDS handle_expire(const std::vector<SDS> &args);
    SDS handle_ttl(const std::vector<SDS> &args);
    SDS handle_incr(const std::vector<SDS> &args);
    SDS handle_decr(const std::vector<SDS> &args);
    SDS handle_incrby(const std::vector<SDS> &args);
    SDS handle_decrby(const std::vector<SDS> &args);
    SDS handle_exists(const std::vector<SDS> &args);
    SDS handle_del(const std::vector<SDS> &args);
    SDS handle_lpush(const std::vector<SDS> &args);
    SDS handle_rpush(const std::vector<SDS> &args);

    SDS handle_lpop(const std::vector<SDS> &args);
    SDS handle_rpop(const std::vector<SDS> &args);

    SDS handle_lrange(const std::vector<SDS> &args);
    SDS handle_llen(const std::vector<SDS> &args);
    SDS handle_zadd(const std::vector<SDS> &args);
    SDS handle_zrank(const std::vector<SDS> &args);
    SDS handle_zrange(const std::vector<SDS> &args);
    SDS handle_zrem(const std::vector<SDS> &args);
    SDS handle_ping(const std::vector<SDS> &args);
    SDS handle_error(const SDS &message);
};

#endif // COMMAND_HANDLER_H