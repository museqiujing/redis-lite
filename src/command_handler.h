// 修改 command_handler.h 文件
#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <vector>
#include <string>
#include "string_type.h"
#include "list.h"
#include "skiplist.h"

class CommandHandler {
public:
    CommandHandler();
    std::string handle_command(const std::vector<std::string>& command);

private:

    String string_type;  
    List list_type; 
    SkipList skiplist_type;
    
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