// command_handler.cpp
// 修改 command_handler.cpp 文件
#include "command_handler.h"
#include "resp_serializer.h"
#include "string_type.h"
#include "list.h"
#include "skiplist.h"
#include <stdexcept>

CommandHandler::CommandHandler() {}

std::string CommandHandler::handle_command(const std::vector<std::string>& command) {
    if (command.empty()) {
        return handle_error("Empty command");
    }
    
    std::string cmd = command[0];
    for (char& c : cmd) {
        c = toupper(c);
    }
    
    try {
        if (cmd == "GET") {
            return handle_get(command);
        } else if (cmd == "SET") {
            return handle_set(command);
        } else if (cmd == "SETEX") {
            return handle_setex(command);
        } else if (cmd == "EXPIRE") {
            return handle_expire(command);
        } else if (cmd == "TTL") {
            return handle_ttl(command);
        } else if (cmd == "INCR") {
            return handle_incr(command);
        } else if (cmd == "DECR") {
            return handle_decr(command);
        } else if (cmd == "INCRBY") {
            return handle_incrby(command);
        } else if (cmd == "DECRBY") {
            return handle_decrby(command);
        } else if (cmd == "EXISTS") {
            return handle_exists(command);
        } else if (cmd == "DEL") {
            return handle_del(command);
        } else if (cmd == "LPUSH") {
            return handle_lpush(command);
        } else if (cmd == "RPUSH") {
            return handle_rpush(command);
        } else if (cmd == "LPOP") {
            return handle_lpop(command);
        } else if (cmd == "RPOP") {
            return handle_rpop(command);
        } else if (cmd == "LRANGE") {
            return handle_lrange(command);
        } else if (cmd == "LLEN") {
            return handle_llen(command);
        } else if (cmd == "ZADD") {
            return handle_zadd(command);
        } else if (cmd == "ZRANK") {
            return handle_zrank(command);
        } else if (cmd == "ZRANGE") {
            return handle_zrange(command);
        } else if (cmd == "ZREM") {
            return handle_zrem(command);
        } else if (cmd == "COMMAND") {
            // 返回一个包含命令名的数组，每个元素都是字符串
               return "*0\r\n";
            // return "*11\r\n*2\r\n$3\r\nGET\r\n*0\r\n*2\r\n$3\r\nSET\r\n*0\r\n*2\r\n$5\r\nSETEX\r\n*0\r\n*2\r\n$6\r\nEXPIRE\r\n*0\r\n*2\r\n$3\r\nTTL\r\n*0\r\n*2\r\n$4\r\nINCR\r\n*0\r\n*2\r\n$4\r\nDECR\r\n*0\r\n*2\r\n$6\r\nINCRBY\r\n*0\r\n*2\r\n$6\r\nDECRBY\r\n*0\r\n*2\r\n$6\r\nEXISTS\r\n*0\r\n*2\r\n$3\r\nDEL\r\n*0\r\n";
        } else if (cmd == "PING") {
            return handle_ping(command);
        } else {
            return handle_error("Unknown command");
        }
    } catch (const std::exception& e) {
        return handle_error(e.what());
    }
}

std::string CommandHandler::handle_get(const std::vector<std::string>& args) {
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
    
    int count = 0;
    for (size_t i = 2; i < args.size(); i += 2) {
        double score = std::stod(args[i]);
        skiplist_type.insert(score, args[i+1]);
        count++;
    }
    
    return RespSerializer::serialize_integer(count);
}

std::string CommandHandler::handle_zrank(const std::vector<std::string>& args) {
    if (args.size() != 3) {
        return handle_error("Wrong number of arguments for ZRANK");
    }
    
    int rank = skiplist_type.get_rank(args[2]);
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
    int start = std::stoi(args[2]);
    int stop = std::stoi(args[3]);
    auto pairs = skiplist_type.range(start, stop);
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
    
    int count = 0;
    for (size_t i = 2; i < args.size(); i++) {
        bool success = skiplist_type.remove(args[i]);
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
