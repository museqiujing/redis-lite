#include "aof.h"
#include "resp_serializer.h"
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "resp_parser.h"
#include "command_handler.h"
AofManager::AofManager(const SDS &file, AofPolicy policy)
    : filename(file), file_fd(-1), enabled(true), sync_policy(policy),
      bg_thread_running(false), last_sync_time(std::chrono::steady_clock::now()) {}

AofManager::~AofManager()
{
    // 停止后台线程
    bg_thread_running = false;
    if (bg_thread.joinable())
    {
        bg_thread.join();
    }
    close_file();
}

bool AofManager::init(CommandHandler *cmd_handler)
{
    handler = cmd_handler;
    // 根据策略启动后台线程
    if (sync_policy == AofPolicy::EVERYSEC)
    {
        bg_thread_running = true;
        bg_thread = std::thread(&AofManager::background_sync_thread, this);
    }
    return open_file();
}

void AofManager::set_enabled(bool enable)
{
    enabled = enable;
    if (enable && file_fd == -1)
    {
        open_file();
    }
    else if (!enable && file_fd != -1)
    {
        close_file();
    }
}

void AofManager::write_command(const std::vector<SDS> &command)
{
    if (!enabled || file_fd == -1)
    {
        return;
    }

    SDS resp = command_to_resp(command);
    ssize_t written = write(file_fd, resp.c_str(), resp.size());
    if (written != (ssize_t)resp.size())
    {
        std::cerr << "Failed to write to AOF file\n";
        return;
    }

    // 根据策略决定是否立即刷盘
    switch (sync_policy)
    {
    case AofPolicy::ALWAYS:
        fsync(file_fd); // 立即刷盘
        break;
    case AofPolicy::EVERYSEC:
        // 每秒刷盘由后台线程处理
        break;
    case AofPolicy::NO:
        // 不主动刷盘
        break;
    }
}

void AofManager::set_sync_policy(AofPolicy policy)
{
    sync_policy = policy;

    // 重启后台线程
    if (bg_thread.joinable())
    {
        bg_thread_running = false;
        bg_thread.join();
    }

    if (sync_policy == AofPolicy::EVERYSEC && enabled)
    {
        bg_thread_running = true;
        bg_thread = std::thread(&AofManager::background_sync_thread, this);
    }
}

SDS AofManager::policy_to_string(AofPolicy policy)
{
    switch (policy)
    {
    case AofPolicy::ALWAYS:
        return "always";
    case AofPolicy::EVERYSEC:
        return "everysec";
    case AofPolicy::NO:
        return "no";
    default:
        return "everysec";
    }
}

AofPolicy AofManager::string_to_policy(const SDS &policy_str)
{
    if (policy_str == "always")
        return AofPolicy::ALWAYS;
    if (policy_str == "everysec")
        return AofPolicy::EVERYSEC;
    if (policy_str == "no")
        return AofPolicy::NO;
    return AofPolicy::EVERYSEC; // 默认值
}

bool AofManager::load()
{
    std::ifstream file(filename.c_str(), std::ios::binary);
    if (!file.is_open())
    {
        return false;
    }

    // 读取整个文件内容
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();

    // 解析RESP格式的命令
    if (!content.empty())
    {
        size_t pos = 0;
        while (pos < content.size())
        {
            try
            {
                // 解析单个命令
                SDS chunk(content.data() + pos, content.size() - pos);
                std::vector<SDS> command = parser.parse(chunk);
                if (!command.empty() && handler)
                {
                    // 执行命令
                    handler->handle_command(command);

                    // 移动pos指针
                    size_t consumed = parser.get_consumed_bytes();
                    if (consumed == 0)
                    {
                        // 防止死循环
                        break;
                    }
                    pos += consumed;
                }
                else
                {
                    break;
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error parsing AOF file: " << e.what() << "\n";
                break;
            }
        }
    }

    return true;
}

bool AofManager::rewrite()
{
    // 创建临时AOF文件
    std::string temp_filename = filename + ".tmp";
    int temp_fd = open(temp_filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (temp_fd == -1)
    {
        std::cerr << "Failed to create temporary AOF file: " << temp_filename << std::endl;
        return false;
    }

    // 遍历所有数据结构，生成最小命令集
    if (handler)
    {
        // 获取所有String数据
        auto string_data = handler->get_all_string_data();
        for (const auto &pair : string_data)
        {
            std::vector<SDS> command = {"SET", pair.first, pair.second};
            SDS resp = command_to_resp(command);
            write(temp_fd, resp.c_str(), resp.size());
        }

        // 获取所有List数据
        auto list_data = handler->get_all_list_data();
        for (const auto &pair : list_data)
        {
            const SDS &key = pair.first;
            const std::vector<SDS> &values = pair.second;
            // 对于List，使用RPUSH命令恢复所有元素
            if (!values.empty())
            {
                std::vector<SDS> command = {"RPUSH", key};
                command.insert(command.end(), values.begin(), values.end());
                SDS resp = command_to_resp(command);
                write(temp_fd, resp.c_str(), resp.size());
            }
        }

        // 获取所有ZSet数据
        auto zset_data = handler->get_all_zset_data();
        for (const auto &pair : zset_data)
        {
            const SDS &key = pair.first;
            const std::vector<std::pair<double, SDS>> &members = pair.second;
            // 对于ZSet，使用ZADD命令恢复所有成员
            if (!members.empty())
            {
                std::vector<SDS> command = {"ZADD", key};
                for (const auto &member : members)
                {
                    command.push_back(std::to_string(member.first));
                    command.push_back(member.second);
                }
                SDS resp = command_to_resp(command);
                write(temp_fd, resp.c_str(), resp.size());
            }
        }
    }

    // 强制刷新到磁盘
    fsync(temp_fd);

    // 关闭临时文件
    close(temp_fd);

    // 替换旧文件
    if (rename(temp_filename.c_str(), filename.c_str()) == -1)
    {
        std::cerr << "Failed to rename temporary AOF file" << std::endl;
        unlink(temp_filename.c_str());
        return false;
    }

    std::cout << "AOF rewrite completed successfully\n";
    return true;
}

bool AofManager::open_file()
{
    // 以追加模式打开文件
    file_fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (file_fd == -1)
    {
        std::cerr << "Failed to open AOF file: " << filename << std::endl;
        return false;
    }
    return true;
}

void AofManager::close_file()
{
    if (file_fd != -1)
    {
        close(file_fd);
        file_fd = -1;
    }
}

SDS AofManager::command_to_resp(const std::vector<SDS> &command)
{
    return RespSerializer::serialize_array(command);
}

// 实现后台刷盘线程
void AofManager::background_sync_thread()
{
    while (bg_thread_running)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        if (file_fd != -1)
        {
            fsync(file_fd); // 每秒刷盘一次
            last_sync_time = std::chrono::steady_clock::now();
        }
    }
}