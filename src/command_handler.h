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
class CommandHandler {
public:
    CommandHandler();
    std::string handle_command(const std::vector<std::string>& command);

     // 获取所有String数据
    std::vector<std::pair<std::string, std::string>> get_all_string_data(); 

     // 获取所有List数据
    std::vector<std::pair<std::string, std::vector<std::string>>> get_all_list_data();
    
    // 获取所有ZSet数据
    std::vector<std::pair<std::string, std::vector<std::pair<double, std::string>>>> get_all_zset_data();
  
private:

    String string_type;  
    List list_type; 
    SkipList skiplist_type;
    AofManager aof_manager;
    ZSetManager zset_manager;
    
    std::string handle_bgrewriteaof(const std::vector<std::string>& args);
    std::string handle_get(const std::vector<std::string>& args);
    std::string handle_set(const std::vector<std::string>& args);
    std::string handle_setex(const std::vector<std::string>& args);
    std::string handle_expire(const std::vector<std::string>& args);
    std::string handle_ttl(const std::vector<std::string>& args);
    std::string handle_incr(const std::vector<std::string>& args);
    std::string handle_decr(const std::vector<std::string>& args);
    std::string handle_incrby(const std::vector<std::string>& args);
    std::string handle_decrby(const std::vector<std::string>& args);
    std::string handle_exists(const std::vector<std::string>& args);
    std::string handle_del(const std::vector<std::string>& args);
    std::string handle_lpush(const std::vector<std::string>& args);
    std::string handle_rpush(const std::vector<std::string>& args);
    std::string handle_lpop(const std::vector<std::string>& args);
    std::string handle_rpop(const std::vector<std::string>& args);
    std::string handle_lrange(const std::vector<std::string>& args);
    std::string handle_llen(const std::vector<std::string>& args);
    std::string handle_zadd(const std::vector<std::string>& args);
    std::string handle_zrank(const std::vector<std::string>& args);
    std::string handle_zrange(const std::vector<std::string>& args);
    std::string handle_zrem(const std::vector<std::string>& args);
    std::string handle_ping(const std::vector<std::string>& args);
    std::string handle_error(const std::string& message);
};

#endif // COMMAND_HANDLER_H