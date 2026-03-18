// command_handler.cpp
#include "command_handler.h"
#include "resp_serializer.h"
#include "string_type.h"
#include "list.h"
#include "skiplist.h"
#include <stdexcept>

CommandHandler::CommandHandler() {
    aof_manager.init(this);
    // 加载AOF文件
    aof_manager.load();
}

std::string CommandHandler::handle_command(const std::vector<std::string>& command) {
    if (command.empty()) {
        return handle_error("Empty command");
    }
    
    std::string cmd = command[0];
    for (char& c : cmd) {
        c = toupper(c);
    }
    
    try {
        std::string response;
        
        if (cmd == "GET") {
            response = handle_get(command);
        } else if (cmd == "SET") {
            response = handle_set(command);
            aof_manager.write_command(command);
        } else if (cmd == "SETEX") {
            response = handle_setex(command);
            aof_manager.write_command(command);
        } else if (cmd == "EXPIRE") {
            response = handle_expire(command);
            aof_manager.write_command(command);
        } else if (cmd == "TTL") {
            response = handle_ttl(command);
        } else if (cmd == "INCR") {
            response = handle_incr(command);
            aof_manager.write_command(command);
        } else if (cmd == "DECR") {
            response = handle_decr(command);
            aof_manager.write_command(command);
        } else if (cmd == "INCRBY") {
            response = handle_incrby(command);
            aof_manager.write_command(command);
        } else if (cmd == "DECRBY") {
            response = handle_decrby(command);
            aof_manager.write_command(command);
        } else if (cmd == "EXISTS") {
            response = handle_exists(command);
        } else if (cmd == "DEL") {
            response = handle_del(command);
            aof_manager.write_command(command);
        } else if (cmd == "LPUSH") {
            response = handle_lpush(command);
            aof_manager.write_command(command);
        } else if (cmd == "RPUSH") {
            response = handle_rpush(command);
            aof_manager.write_command(command);
        } else if (cmd == "LPOP") {
            response = handle_lpop(command);
            aof_manager.write_command(command);
        } else if (cmd == "RPOP") {
            response = handle_rpop(command);
            aof_manager.write_command(command);
        } else if (cmd == "LRANGE") {
            response = handle_lrange(command);
        } else if (cmd == "LLEN") {
            response = handle_llen(command);
        } else if (cmd == "ZADD") {
            response = handle_zadd(command);
            aof_manager.write_command(command);
        } else if (cmd == "ZRANK") {
            response = handle_zrank(command);
        } else if (cmd == "ZRANGE") {
            response = handle_zrange(command);
        } else if (cmd == "ZREM") {
            response = handle_zrem(command);
            aof_manager.write_command(command);
        } else if (cmd == "BGREWRITEAOF") {
            response = handle_bgrewriteaof(command);
        } else if (cmd == "COMMAND") {
               response = "*0\r\n";// 返回一个空数组
         } else if (cmd == "PING") {
            response = handle_ping(command);
        } else {
            response = handle_error("Unknown command");
        }
        
        return response;
    } catch (const std::exception& e) {
        return handle_error(e.what());
    }
}

std::vector<std::pair<std::string, std::string>> CommandHandler::get_all_string_data()
{
    return string_type.get_all_data();
}

std::vector<std::pair<std::string, std::vector<std::string>>> CommandHandler::get_all_list_data()
{
    return list_type.get_all_data();
}

std::vector<std::pair<std::string, std::vector<std::pair<double, std::string>>>> CommandHandler::get_all_zset_data()
{
    return zset_manager.get_all_zset_data();
}

std::string CommandHandler::handle_bgrewriteaof(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        return handle_error("Wrong number of arguments for BGREWRITEAOF");
    }
    
    if (aof_manager.rewrite()) {
        return RespSerializer::serialize_simple_string("Background AOF rewrite started");
    } else {
        return handle_error("Failed to start AOF rewrite");
    }
}

std::string CommandHandler::handle_get(const std::vector<std::string> &args)
{
    if (args.size() != 2) {
        return handle_error("Wrong number of arguments for GET");
    }

    std::string value = string_type.get(args[1]);

    if (value.empty()) {
        return RespSerializer::serialize_null_bulk_string();
    }
    return RespSerializer::serialize_bulk_string(value);
}

std::string CommandHandler::handle_set(const std::vector<std::string>& args) {
    if (args.size() != 3) {
        return handle_error("Wrong number of arguments for SET");
    }
    
    string_type.set(args[1], args[2]);
    return RespSerializer::serialize_simple_string("OK");
}

std::string CommandHandler::handle_setex(const std::vector<std::string>& args) {
    if (args.size() != 4) {
        return handle_error("Wrong number of arguments for SETEX");
    }
    
    int seconds = std::stoi(args[2]);
    string_type.setex(args[1], seconds, args[3]);
    return RespSerializer::serialize_simple_string("OK");
}

std::string CommandHandler::handle_expire(const std::vector<std::string>& args) {
    if (args.size() != 3) {
        return handle_error("Wrong number of arguments for EXPIRE");
    }
    
    int seconds = std::stoi(args[2]);
    bool success = string_type.expire(args[1], seconds);
    return RespSerializer::serialize_integer(success ? 1 : 0);
}

std::string CommandHandler::handle_ttl(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return handle_error("Wrong number of arguments for TTL");
    }
    
    int ttl = string_type.ttl(args[1]);
    return RespSerializer::serialize_integer(ttl);
}

std::string CommandHandler::handle_incr(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return handle_error("Wrong number of arguments for INCR");
    }
    
    long long value = string_type.incr(args[1]);
    return RespSerializer::serialize_integer(value);
}

std::string CommandHandler::handle_decr(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return handle_error("Wrong number of arguments for DECR");
    }
    
    long long value = string_type.decr(args[1]);
    return RespSerializer::serialize_integer(value);
}

std::string CommandHandler::handle_incrby(const std::vector<std::string>& args) {
    if (args.size() != 3) {
        return handle_error("Wrong number of arguments for INCRBY");
    }

    long long increment = std::stoll(args[2]);
    long long value = string_type.incrby(args[1], increment);
    return RespSerializer::serialize_integer(value);
}

std::string CommandHandler::handle_decrby(const std::vector<std::string>& args) {
    if (args.size() != 3) {
        return handle_error("Wrong number of arguments for DECRBY");
    }
    long long decrement = std::stoll(args[2]);
    long long value = string_type.decrby(args[1], decrement);
    return RespSerializer::serialize_integer(value);
}

std::string CommandHandler::handle_exists(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return handle_error("Wrong number of arguments for EXISTS");
    }
    
    bool exists = string_type.exists(args[1]);
    return RespSerializer::serialize_integer(exists ? 1 : 0);
}

std::string CommandHandler::handle_del(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return handle_error("Wrong number of arguments for DEL");
    }
    
    bool success = string_type.del(args[1]);
    return RespSerializer::serialize_integer(success ? 1 : 0);
}

std::string CommandHandler::handle_lpush(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        return handle_error("Wrong number of arguments for LPUSH");
    }
    
    for (size_t i = 2; i < args.size(); i++) {
        list_type.lpush(args[1], args[i]);
    }
    
    long long len = list_type.llen(args[1]);
    return RespSerializer::serialize_integer(len);
}

std::string CommandHandler::handle_rpush(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        return handle_error("Wrong number of arguments for RPUSH");
    }
    
    for (size_t i = 2; i < args.size(); i++) {
        list_type.rpush(args[1], args[i]);
    }
    
    long long len = list_type.llen(args[1]);
    return RespSerializer::serialize_integer(len);
}

std::string CommandHandler::handle_lpop(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return handle_error("Wrong number of arguments for LPOP");
    }

    std::string value = list_type.lpop(args[1]);
    if (value.empty()) {
        return RespSerializer::serialize_null_bulk_string();
    }
    return RespSerializer::serialize_bulk_string(value);
}

std::string CommandHandler::handle_rpop(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return handle_error("Wrong number of arguments for RPOP");
    }
    
    std::string value = list_type.rpop(args[1]);
    if (value.empty()) {
        return RespSerializer::serialize_null_bulk_string();
    }
    return RespSerializer::serialize_bulk_string(value);
}

std::string CommandHandler::handle_lrange(const std::vector<std::string>& args) {
    if (args.size() != 4) {
        return handle_error("Wrong number of arguments for LRANGE");
    }
    
    int start = std::stoi(args[2]);
    int stop = std::stoi(args[3]);
    std::vector<std::string> values = list_type.lrange(args[1], start, stop);
    return RespSerializer::serialize_array(values);
}

std::string CommandHandler::handle_llen(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return handle_error("Wrong number of arguments for LLEN");
    }
    
    long long len = list_type.llen(args[1]);
    return RespSerializer::serialize_integer(len);
}

std::string CommandHandler::handle_zadd(const std::vector<std::string>& args) {
    if (args.size() < 4 || args.size() % 2 != 0) {
        return handle_error("Wrong number of arguments for ZADD");
    }
    
    const std::string& key = args[1];
    SkipList* zset = zset_manager.get_zset(key);
    
    int count = 0;
    for (size_t i = 2; i < args.size(); i += 2) {
        double score = std::stod(args[i]);
        zset->insert(score, args[i+1]);
        count++;
    }
    
    return RespSerializer::serialize_integer(count);
}

std::string CommandHandler::handle_zrank(const std::vector<std::string>& args) {
    if (args.size() != 3) {
        return handle_error("Wrong number of arguments for ZRANK");
    }
    
    const std::string& key = args[1];
    SkipList* zset = zset_manager.get_zset(key);
    
    int rank = zset->get_rank(args[2]);
    if (rank == -1) {
        return RespSerializer::serialize_null_bulk_string();
    }
    return RespSerializer::serialize_integer(rank);
}

std::string CommandHandler::handle_zrange(const std::vector<std::string>& args) {
    if (args.size() != 4) {
        return handle_error("Wrong number of arguments for ZRANGE");
    }
    
    try{
    const std::string& key = args[1];
    SkipList* zset = zset_manager.get_zset(key);
    
    int start = std::stoi(args[2]);
    int stop = std::stoi(args[3]);
    auto pairs = zset->range(start, stop);
    std::vector<std::string> values;
    for (const auto& pair : pairs) {
        values.push_back(pair.second);
    }
    return RespSerializer::serialize_array(values);
}catch(const std::invalid_argument& e){
    return handle_error("Error: " + std::string(e.what()));
}
}

std::string CommandHandler::handle_zrem(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        return handle_error("Wrong number of arguments for ZREM");
    }
    
    const std::string& key = args[1];
    SkipList* zset = zset_manager.get_zset(key);
    
    int count = 0;
    for (size_t i = 2; i < args.size(); i++) {
        bool success = zset->remove(args[i]);
        if (success) {
            count++;
        }
    }
    
    return RespSerializer::serialize_integer(count);
}

std::string CommandHandler::handle_ping(const std::vector<std::string>& args) {
    return RespSerializer::serialize_simple_string("PONG");
}

std::string CommandHandler::handle_error(const std::string& message) {
    return RespSerializer::serialize_error(message);
}