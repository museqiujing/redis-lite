// 修改 command_handler.h 文件
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

class CommandHandler
{
public:
    CommandHandler(AofPolicy policy = AofPolicy::NO);
    SDS handle_command(const std::vector<SDS> &command);

    // AOF管理方法
    void set_aof_policy(AofPolicy policy);
    AofPolicy get_aof_policy() const { return aof_policy; }

    // 获取所有String数据
    std::vector<std::pair<SDS, SDS>> get_all_string_data();

    // 获取所有List数据
    std::vector<std::pair<SDS, std::vector<SDS>>> get_all_list_data();

    // 获取所有ZSet数据
    std::vector<std::pair<SDS, std::vector<std::pair<double, SDS>>>> get_all_zset_data();

private:
    String string_type;
    List list_type;
    SkipList skiplist_type;
    AofManager aof_manager;
    ZSetManager zset_manager;
    AofPolicy aof_policy; // AOF策略
    SDS response;         // 响应字符串

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