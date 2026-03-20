// command_handler.cpp
#include "command_handler.h"
#include "resp_serializer.h"
#include "string_type.h"
#include "list.h"
#include "skiplist.h"
#include <stdexcept>

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
    for (char &c : cmd)
    {
        c = toupper(c);
    }

    try
    {
        if (cmd == "GET")
        {
            response = handle_get(command);
        }
        else if (cmd == "SET")
        {
            response = handle_set(command);
            if (aof_policy != AofPolicy::NO)
            {
                aof_manager.write_command(command);
            }
        }
        else if (cmd == "SETEX")
        {
            response = handle_setex(command);
            if (aof_policy != AofPolicy::NO)
            {
                aof_manager.write_command(command);
            }
        }
        else if (cmd == "EXPIRE")
        {
            response = handle_expire(command);
            if (aof_policy != AofPolicy::NO)
            {
                aof_manager.write_command(command);
            }
        }
        else if (cmd == "TTL")
        {
            response = handle_ttl(command);
        }
        else if (cmd == "INCR")
        {
            response = handle_incr(command);
            if (aof_policy != AofPolicy::NO)
            {
                aof_manager.write_command(command);
            }
        }
        else if (cmd == "DECR")
        {
            response = handle_decr(command);
            if (aof_policy != AofPolicy::NO)
            {
                aof_manager.write_command(command);
            }
        }
        else if (cmd == "INCRBY")
        {
            response = handle_incrby(command);
            if (aof_policy != AofPolicy::NO)
            {
                aof_manager.write_command(command);
            }
        }
        else if (cmd == "DECRBY")
        {
            response = handle_decrby(command);
            if (aof_policy != AofPolicy::NO)
            {
                aof_manager.write_command(command);
            }
        }
        else if (cmd == "EXISTS")
        {
            response = handle_exists(command);
        }
        else if (cmd == "DEL")
        {
            response = handle_del(command);
            if (aof_policy != AofPolicy::NO)
            {
                aof_manager.write_command(command);
            }
        }
        else if (cmd == "LPUSH")
        {
            response = handle_lpush(command);
            if (aof_policy != AofPolicy::NO)
            {
                aof_manager.write_command(command);
            }
        }
        else if (cmd == "RPUSH")
        {
            response = handle_rpush(command);
            if (aof_policy != AofPolicy::NO)
            {
                aof_manager.write_command(command);
            }
        }
        else if (cmd == "LPOP")
        {
            response = handle_lpop(command);
            if (aof_policy != AofPolicy::NO)
            {
                aof_manager.write_command(command);
            }
        }
        else if (cmd == "RPOP")
        {
            response = handle_rpop(command);
            if (aof_policy != AofPolicy::NO)
            {
                aof_manager.write_command(command);
            }
        }
        else if (cmd == "LRANGE")
        {
            response = handle_lrange(command);
        }
        else if (cmd == "LLEN")
        {
            response = handle_llen(command);
        }
        else if (cmd == "ZADD")
        {
            response = handle_zadd(command);
            if (aof_policy != AofPolicy::NO)
            {
                aof_manager.write_command(command);
            }
        }
        else if (cmd == "ZRANK")
        {
            response = handle_zrank(command);
        }
        else if (cmd == "ZRANGE")
        {
            response = handle_zrange(command);
        }
        else if (cmd == "ZREM")
        {
            response = handle_zrem(command);
            if (aof_policy != AofPolicy::NO)
            {
                aof_manager.write_command(command);
            }
        }
        else if (cmd == "BGREWRITEAOF")
        {
            response = handle_bgrewriteaof(command);
        }
        else if (cmd == "COMMAND")
        {
            response = "*0\r\n"; // 返回一个空数组
        }
        else if (cmd == "PING")
        {
            response = handle_ping(command);
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