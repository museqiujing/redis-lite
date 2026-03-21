// command_handler.cpp
#include "command_handler.h"
#include "resp_serializer.h"
#include "string_type.h"
#include "list.h"
#include "skiplist.h"
#include <stdexcept>
// 命令映射表
std::unordered_map<SDS, CommandHandler::CommandHandlerFunc> CommandHandler::command_map = {
    {"GET", &CommandHandler::handle_get},
    {"SET", &CommandHandler::handle_set},
    {"SETEX", &CommandHandler::handle_setex},
    {"EXPIRE", &CommandHandler::handle_expire},
    {"TTL", &CommandHandler::handle_ttl},
    {"INCR", &CommandHandler::handle_incr},
    {"DECR", &CommandHandler::handle_decr},
    {"INCRBY", &CommandHandler::handle_incrby},
    {"DECRBY", &CommandHandler::handle_decrby},
    {"EXISTS", &CommandHandler::handle_exists},
    {"DEL", &CommandHandler::handle_del},
    {"LPUSH", &CommandHandler::handle_lpush},
    {"RPUSH", &CommandHandler::handle_rpush},
    {"LPOP", &CommandHandler::handle_lpop},
    {"RPOP", &CommandHandler::handle_rpop},
    {"LRANGE", &CommandHandler::handle_lrange},
    {"LLEN", &CommandHandler::handle_llen},
    {"ZADD", &CommandHandler::handle_zadd},
    {"ZRANK", &CommandHandler::handle_zrank},
    {"ZRANGE", &CommandHandler::handle_zrange},
    {"ZREM", &CommandHandler::handle_zrem},
    {"BGREWRITEAOF", &CommandHandler::handle_bgrewriteaof},
    {"PING", &CommandHandler::handle_ping}};

std::unordered_set<SDS> CommandHandler::write_commands = {
    "SET", "SETEX", "EXPIRE", "INCR", "DECR",
    "INCRBY", "DECRBY", "DEL", "LPUSH", "RPUSH",
    "LPOP", "RPOP", "ZADD", "ZREM"};

// 修改构造函数
CommandHandler::CommandHandler(AofPolicy aof_policy)
    : aof_policy(aof_policy)
{

    if (aof_policy != AofPolicy::NO)
    {
        aof_manager.init(this);
        aof_manager.set_sync_policy(aof_policy);
        aof_manager.load();
    }
}

// 设置AOF开关
void CommandHandler::set_aof_policy(AofPolicy policy)
{
    aof_policy = policy;

    if (policy != AofPolicy::NO)
    {
        aof_manager.set_enabled(true);
        aof_manager.set_sync_policy(policy);
    }
    else
    {
        aof_manager.set_enabled(false);
    }
}

SDS CommandHandler::handle_command(const std::vector<SDS> &command)
{
    if (command.empty())
    {
        return handle_error("Empty command");
    }
    SDS cmd = command[0];
    /*for (char &c : cmd)
    {
        c = toupper(c);
    }*/
    try
    {
        auto it = command_map.find(cmd);
        if (it != command_map.end())
        {
            // 执行命令处理函数
            CommandHandlerFunc handler_func = it->second;
            response = (this->*handler_func)(command);

            handle_write_command_aof(command);
        }
        else if (cmd == "COMMAND")
        {
            response = "*0\r\n"; // 返回一个空数组
        }
        else
        {
            response = handle_error("Unknown command");
        }

        return response;
    }
    catch (const std::exception &e)
    {
        return handle_error(e.what());
    }
}

std::vector<std::pair<SDS, SDS>> CommandHandler::get_all_string_data()
{
    return string_type.get_all_data();
}

std::vector<std::pair<SDS, std::vector<SDS>>> CommandHandler::get_all_list_data()
{
    return list_type.get_all_data();
}

std::vector<std::pair<SDS, std::vector<std::pair<double, SDS>>>> CommandHandler::get_all_zset_data()
{
    return zset_manager.get_all_zset_data();
}

SDS CommandHandler::handle_bgrewriteaof(const std::vector<SDS> &args)
{
    if (args.size() != 1)
    {
        return handle_error("Wrong number of arguments for BGREWRITEAOF");
    }

    if (aof_manager.rewrite())
    {
        return RespSerializer::serialize_simple_string("Background AOF rewrite started");
    }
    else
    {
        return handle_error("Failed to start AOF rewrite");
    }
}

SDS CommandHandler::handle_get(const std::vector<SDS> &args)
{
    if (args.size() != 2)
    {
        return handle_error("Wrong number of arguments for GET");
    }

    SDS value = string_type.get(args[1]);

    if (value.empty())
    {
        return RespSerializer::serialize_null_bulk_string();
    }
    return RespSerializer::serialize_bulk_string(value);
}

SDS CommandHandler::handle_set(const std::vector<SDS> &args)
{
    if (args.size() != 3)
    {
        return handle_error("Wrong number of arguments for SET");
    }

    string_type.set(args[1], args[2]);

    return RespSerializer::serialize_simple_string("OK");
}

SDS CommandHandler::handle_setex(const std::vector<SDS> &args)
{
    if (args.size() != 4)
    {
        return handle_error("Wrong number of arguments for SETEX");
    }

    int seconds = std::stoi(args[2]);
    string_type.setex(args[1], seconds, args[3]);

    return RespSerializer::serialize_simple_string("OK");
}

SDS CommandHandler::handle_expire(const std::vector<SDS> &args)
{
    if (args.size() != 3)
    {
        return handle_error("Wrong number of arguments for EXPIRE");
    }

    int seconds = std::stoi(args[2]);
    bool success = string_type.expire(args[1], seconds);

    return RespSerializer::serialize_integer(success ? 1 : 0);
}

SDS CommandHandler::handle_ttl(const std::vector<SDS> &args)
{
    if (args.size() != 2)
    {
        return handle_error("Wrong number of arguments for TTL");
    }

    int ttl = string_type.ttl(args[1]);
    return RespSerializer::serialize_integer(ttl);
}

SDS CommandHandler::handle_incr(const std::vector<SDS> &args)
{
    if (args.size() != 2)
    {
        return handle_error("Wrong number of arguments for INCR");
    }

    long long value = string_type.incr(args[1]);

    return RespSerializer::serialize_integer(value);
}

SDS CommandHandler::handle_decr(const std::vector<SDS> &args)
{
    if (args.size() != 2)
    {
        return handle_error("Wrong number of arguments for DECR");
    }

    long long value = string_type.decr(args[1]);
    return RespSerializer::serialize_integer(value);
}

SDS CommandHandler::handle_incrby(const std::vector<SDS> &args)
{
    if (args.size() != 3)
    {
        return handle_error("Wrong number of arguments for INCRBY");
    }

    long long increment = std::stoll(args[2]);
    long long value = string_type.incrby(args[1], increment);
    return RespSerializer::serialize_integer(value);
}

SDS CommandHandler::handle_decrby(const std::vector<SDS> &args)
{
    if (args.size() != 3)
    {
        return handle_error("Wrong number of arguments for DECRBY");
    }
    long long decrement = std::stoll(args[2]);
    long long value = string_type.decrby(args[1], decrement);
    return RespSerializer::serialize_integer(value);
}

SDS CommandHandler::handle_exists(const std::vector<SDS> &args)
{
    if (args.size() != 2)
    {
        return handle_error("Wrong number of arguments for EXISTS");
    }

    bool exists = string_type.exists(args[1]);
    return RespSerializer::serialize_integer(exists ? 1 : 0);
}

SDS CommandHandler::handle_del(const std::vector<SDS> &args)
{
    if (args.size() != 2)
    {
        return handle_error("Wrong number of arguments for DEL");
    }

    bool success = string_type.del(args[1]);

    return RespSerializer::serialize_integer(success ? 1 : 0);
}

SDS CommandHandler::handle_lpush(const std::vector<SDS> &args)
{
    if (args.size() < 3)
    {
        return handle_error("Wrong number of arguments for LPUSH");
    }

    for (size_t i = 2; i < args.size(); i++)
    {
        list_type.lpush(args[1], args[i]);
    }

    long long len = list_type.llen(args[1]);

    return RespSerializer::serialize_integer(len);
}

SDS CommandHandler::handle_rpush(const std::vector<SDS> &args)
{
    if (args.size() < 3)
    {
        return handle_error("Wrong number of arguments for RPUSH");
    }

    for (size_t i = 2; i < args.size(); i++)
    {
        list_type.rpush(args[1], args[i]);
    }

    long long len = list_type.llen(args[1]);
    return RespSerializer::serialize_integer(len);
}

SDS CommandHandler::handle_lpop(const std::vector<SDS> &args)
{
    if (args.size() != 2)
    {
        return handle_error("Wrong number of arguments for LPOP");
    }

    SDS value = list_type.lpop(args[1]);
    if (value.empty())
    {
        return RespSerializer::serialize_null_bulk_string();
    }
    return RespSerializer::serialize_bulk_string(value);
}

SDS CommandHandler::handle_rpop(const std::vector<SDS> &args)
{
    if (args.size() != 2)
    {
        return handle_error("Wrong number of arguments for RPOP");
    }

    SDS value = list_type.rpop(args[1]);
    if (value.empty())
    {
        return RespSerializer::serialize_null_bulk_string();
    }
    return RespSerializer::serialize_bulk_string(value);
}

SDS CommandHandler::handle_lrange(const std::vector<SDS> &args)
{
    if (args.size() != 4)
    {
        return handle_error("Wrong number of arguments for LRANGE");
    }

    int start = std::stoi(args[2]);
    int stop = std::stoi(args[3]);
    std::vector<SDS> values = list_type.lrange(args[1], start, stop);
    return RespSerializer::serialize_array(values);
}

SDS CommandHandler::handle_llen(const std::vector<SDS> &args)
{
    if (args.size() != 2)
    {
        return handle_error("Wrong number of arguments for LLEN");
    }

    long long len = list_type.llen(args[1]);
    return RespSerializer::serialize_integer(len);
}

SDS CommandHandler::handle_zadd(const std::vector<SDS> &args)
{
    if (args.size() < 4 || args.size() % 2 != 0)
    {
        return handle_error("Wrong number of arguments for ZADD");
    }

    const SDS &key = args[1];
    SkipList *zset = zset_manager.get_zset(key);

    int count = 0;
    for (size_t i = 2; i < args.size(); i += 2)
    {
        double score = std::stod(args[i]);
        zset->insert(score, args[i + 1]);
        count++;
    }

    return RespSerializer::serialize_integer(count);
}

SDS CommandHandler::handle_zrank(const std::vector<SDS> &args)
{
    if (args.size() != 3)
    {
        return handle_error("Wrong number of arguments for ZRANK");
    }

    const SDS &key = args[1];
    SkipList *zset = zset_manager.get_zset(key);

    int rank = zset->get_rank(args[2]);
    if (rank == -1)
    {
        return RespSerializer::serialize_null_bulk_string();
    }
    return RespSerializer::serialize_integer(rank);
}

SDS CommandHandler::handle_zrange(const std::vector<SDS> &args)
{
    if (args.size() != 4)
    {
        return handle_error("Wrong number of arguments for ZRANGE");
    }

    try
    {
        const SDS &key = args[1];
        SkipList *zset = zset_manager.get_zset(key);

        int start = std::stoi(args[2]);
        int stop = std::stoi(args[3]);
        auto pairs = zset->range(start, stop);
        std::vector<SDS> values;
        for (const auto &pair : pairs)
        {
            values.push_back(pair.second);
        }
        return RespSerializer::serialize_array(values);
    }
    catch (const std::invalid_argument &e)
    {
        return handle_error("Error: " + SDS(e.what()));
    }
}

SDS CommandHandler::handle_zrem(const std::vector<SDS> &args)
{
    if (args.size() < 3)
    {
        return handle_error("Wrong number of arguments for ZREM");
    }

    const SDS &key = args[1];
    SkipList *zset = zset_manager.get_zset(key);

    int count = 0;
    for (size_t i = 2; i < args.size(); i++)
    {
        bool success = zset->remove(args[i]);
        if (success)
        {
            count++;
        }
    }

    return RespSerializer::serialize_integer(count);
}

SDS CommandHandler::handle_ping(const std::vector<SDS> &args)
{
    return RespSerializer::serialize_simple_string("PONG");
}

SDS CommandHandler::handle_error(const SDS &message)
{
    return RespSerializer::serialize_error(message);
}

// 判断是否为写命令（需要写入AOF的命令）
bool CommandHandler::is_write_command(const SDS &cmd) const
{

    return write_commands.find(cmd) != write_commands.end();
}

// 判断是否应该写入AOF
bool CommandHandler::should_write_aof() const
{
    return aof_policy != AofPolicy::NO;
}

// 处理写命令的AOF写入
void CommandHandler::handle_write_command_aof(const std::vector<SDS> &command)
{
    if (command.empty())
        return;

    SDS cmd = command[0];
    for (char &c : cmd)
    {
        c = toupper(c);
    }

    // 双重检查：既是写命令且AOF已启用
    if (is_write_command(cmd) && should_write_aof())
    {
        aof_manager.write_command(command);
    }
}

// 初始化响应缓冲区池
thread_local std::vector<SDS> CommandHandler::response_buffer_pool;

// 初始化缓冲区池
void CommandHandler::init_buffer_pool()
{
    if (response_buffer_pool.empty())
    {
        // 预分配10个缓冲区
        for (int i = 0; i < 10; i++)
        {
            response_buffer_pool.push_back(SDS());
        }
    }
}

// 从池获取缓冲区
SDS CommandHandler::get_buffer_from_pool()
{
    init_buffer_pool();
    if (!response_buffer_pool.empty())
    {
        SDS buffer = response_buffer_pool.back();
        response_buffer_pool.pop_back();
        buffer.clear();
        return buffer;
    }
    return SDS();
}

// 归还缓冲区到池
void CommandHandler::return_buffer_to_pool(SDS &buffer)
{
    buffer.clear();
    response_buffer_pool.push_back(buffer);
}

std::vector<SDS> CommandHandler::handle_commands_batch(const std::vector<std::vector<SDS>> &commands)
{
    std::vector<SDS> responses;
    responses.reserve(commands.size());

    for (const auto &command : commands)
    {
        if (command.empty())
        {
            responses.push_back(handle_error("Empty command"));
            continue;
        }

        SDS cmd = command[0];
        for (char &c : cmd)
        {
            c = toupper(c);
        }

        try
        {
            auto it = command_map.find(cmd);
            if (it != command_map.end())
            {
                // 执行命令处理函数
                CommandHandlerFunc handler_func = it->second;
                SDS response = (this->*handler_func)(command);
                responses.push_back(response);

                // 处理AOF写入
                handle_write_command_aof(command);
            }
            else if (cmd == "COMMAND")
            {
                responses.push_back("*0\r\n"); // 返回一个空数组
            }
            else
            {
                responses.push_back(handle_error("Unknown command"));
            }
        }
        catch (const std::exception &e)
        {
            responses.push_back(handle_error(e.what()));
        }
    }

    return responses;
}