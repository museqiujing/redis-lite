#include "aof.h"
#include "resp_serializer.h"
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "resp_parser.h"
#include "command_handler.h"
#include <mutex>

AofManager::AofManager(const SDS &file, AofPolicy policy)
    : filename(file), file_fd(-1), enabled(true), sync_policy(policy),
      bg_thread_running(false), buffer_size(0), rewrite_base_size(0),
      last_sync_time(std::chrono::steady_clock::now()) {}

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

// 文件级 mutex，用于保护主写入缓冲区的合并
static std::mutex aof_main_buffer_mutex;

bool AofManager::init(CommandHandler *cmd_handler)
{
    handler = cmd_handler;

    // 初始化重写基准大小
    struct stat st;
    if (stat(filename.c_str(), &st) == 0)
    {
        rewrite_base_size = st.st_size;
        std::cout << "AOF rewrite base size initialized to: " << rewrite_base_size / (1024 * 1024) << "MB\n";
    }
    else
    {
        rewrite_base_size = 0;
        std::cout << "AOF file not found, rewrite base size set to 0\n";
    }

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

// 线程局部的写入缓冲区，避免锁竞争
thread_local SDS thread_local_buffer;
thread_local size_t thread_local_buffer_size = 0;

void AofManager::write_command(const std::vector<SDS> &command)
{
    if (!enabled || file_fd == -1)
    {
        return;
    }

    // 快速序列化（避免 RespSerializer 的开销）
    size_t estimated_size = 32; // 基础 RESP 开销
    for (const auto &arg : command)
    {
        estimated_size += arg.size() + 16; // 每个参数的开销
    }

    // 使用线程局部缓冲区（主要避免锁竞争）。当本地缓冲接近上限时，合并到主缓冲区
    if (thread_local_buffer_size + estimated_size > MAX_THREAD_BUFFER_SIZE) // 16KB 缓冲区限制
    {
        std::lock_guard<std::mutex> lock(aof_main_buffer_mutex);

        write_buffer.append(thread_local_buffer);
        buffer_size += thread_local_buffer_size;

        // 清空线程局部缓冲区
        thread_local_buffer.clear();
        thread_local_buffer_size = 0;

        // 检查主缓冲区是否需要刷盘
        if (buffer_size >= BUFFER_SIZE_LIMIT) // 64KB 主缓冲区
        {
            flush_buffer();
        }
    }

    // 快速序列化到线程局部缓冲区
    thread_local_buffer.append("*");
    thread_local_buffer.append(std::to_string(command.size()));
    thread_local_buffer.append("\r\n");

    for (const auto &arg : command)
    {
        thread_local_buffer.append("$");
        thread_local_buffer.append(std::to_string(arg.size()));
        thread_local_buffer.append("\r\n");
        thread_local_buffer.append(arg);
        thread_local_buffer.append("\r\n");
    }

    thread_local_buffer_size += estimated_size;

    // 如果当前策略要求尽快持久化（例如 ALWAYS），或采用 EVERYSEC 时
    // 希望在后台线程的 fsync 前能看到新数据，则需要把线程局部缓冲合并到主缓冲区。
    // 这里做一个保守的合并：当策略不是 NO 时，将本地缓冲合并到主缓冲区，
    // 主线程/后台线程会负责最终写磁盘和 fsync。
    if (sync_policy != AofPolicy::NO)
    {
        std::lock_guard<std::mutex> lock(aof_main_buffer_mutex);
        if (!thread_local_buffer.empty())
        {
            write_buffer.append(thread_local_buffer);
            buffer_size += thread_local_buffer_size;
            thread_local_buffer.clear();
            thread_local_buffer_size = 0;
        }

        if (sync_policy == AofPolicy::ALWAYS)
        {
            // 立即写入并刷盘
            flush_buffer();
            if (file_fd != -1)
                fsync(file_fd);
        }
    }

    // 更新写入计数（用于重写判断）
    total_writes_since_rewrite++;

    // 检查是否需要自动重写（降低频率）
    static thread_local size_t rewrite_check_counter = 0;
    if (++rewrite_check_counter >= 1000) // 每 1000 次写入检查一次（更频繁）
    {
        rewrite_check_counter = 0;
        if (should_auto_rewrite())
        {
            // 在后台线程中执行重写
            std::thread rewrite_thread(&AofManager::background_rewrite_thread, this);
            rewrite_thread.detach();
        }
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
    struct stat st;
    if (stat(filename.c_str(), &st) != 0)
    {
        return false;
    }

    const bool old_enabled = enabled;
    enabled = false; // 关键：回放期间禁止再次写 AOF

    std::cout << "Loading AOF file: " << filename << " ("
              << st.st_size / (1024 * 1024) << "MB)\n";

    int fd = open(filename.c_str(), O_RDONLY);
    if (fd == -1)
    {
        std::cerr << "Failed to open AOF file: " << filename << std::endl;
        enabled = old_enabled;
        return false;
    }

    static constexpr size_t READ_CHUNK_SIZE = 4 * 1024 * 1024;
    std::vector<char> read_buf(READ_CHUNK_SIZE);
    std::string pending;
    pending.reserve(READ_CHUNK_SIZE * 2);

    const size_t total_size = static_cast<size_t>(st.st_size);
    size_t file_bytes_read = 0;
    size_t commands_loaded = 0;
    size_t last_progress_display = 0;
    auto start_time = std::chrono::steady_clock::now();
    bool ok = true;

    while (true)
    {
        ssize_t nread = read(fd, read_buf.data(), read_buf.size());
        if (nread < 0)
        {
            std::cerr << "Failed to read AOF file: " << strerror(errno) << std::endl;
            ok = false;
            break;
        }
        if (nread == 0)
        {
            break;
        }

        pending.append(read_buf.data(), static_cast<size_t>(nread));
        file_bytes_read += static_cast<size_t>(nread);

        size_t pos = 0;
        while (pos < pending.size())
        {
            size_t consumed = 0;
            auto commands = AofFastParser::parse_commands_batch(
                pending.data() + pos,
                pending.size() - pos,
                consumed);

            if (!commands.empty() && consumed > 0)
            {
                if (handler)
                {
                    for (auto &command : commands)
                    {
                        handler->handle_command(command);
                        ++commands_loaded;
                    }
                }
                pos += consumed;
            }
            else
            {
                // 无更多可完整解析的命令，保留尾部（半包）
                break;
            }
        }

        if (pos > 0)
        {
            pending.erase(0, pos);
        }

        if (file_bytes_read - last_progress_display >= 1024 * 1024 ||
            commands_loaded % 5000 == 0 ||
            file_bytes_read == total_size)
        {
            int progress = total_size == 0 ? 100
                                           : static_cast<int>((file_bytes_read * 100) / total_size);

            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                current_time - start_time);

            std::cout << "Loading AOF: " << progress << "% ("
                      << file_bytes_read / (1024 * 1024) << "MB/"
                      << total_size / (1024 * 1024) << "MB, "
                      << commands_loaded << " commands, "
                      << elapsed.count() << "ms)\r" << std::flush;

            last_progress_display = file_bytes_read;
        }
    }

    close(fd);

    if (!pending.empty())
    {
        size_t consumed = 0;
        auto command = AofFastParser::parse_command(pending.data(), pending.size(), consumed);
        if (!command.empty() && consumed == pending.size())
        {
            if (handler)
            {
                handler->handle_command(command);
            }
            ++commands_loaded;
        }
        else
        {
            std::cerr << "Warning: AOF tail contains incomplete command, ignored ("
                      << pending.size() << " bytes)\n";
        }
    }

    enabled = old_enabled; // 恢复 AOF 开关

    auto end_time = std::chrono::steady_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);

    std::cout << "\nAOF loading completed: " << commands_loaded << " commands in "
              << total_time.count() << "ms\n";

    return ok;
}

bool AofManager::rewrite()
{
    // 防止并发重写
    bool expected = false;
    if (!rewrite_in_progress.compare_exchange_strong(expected, true))
    {
        std::cout << "AOF rewrite already in progress, skipping\n";
        return false;
    }

    std::cout << "Starting AOF rewrite...\n";

    // 先刷新缓冲区确保所有数据已写入
    flush_buffer();

    // 创建临时 AOF 文件
    std::string temp_filename = filename + ".tmp";
    int temp_fd = open(temp_filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (temp_fd == -1)
    {
        std::cerr << "Failed to create temporary AOF file: " << temp_filename << std::endl;
        rewrite_in_progress = false;
        return false;
    }

    // 遍历所有数据结构，生成最小命令集
    if (handler)
    {
        size_t keys_written = 0;

        // 获取所有 String 数据
        auto string_data = handler->get_all_string_data();
        for (const auto &pair : string_data)
        {
            std::vector<SDS> command = {"SET", pair.first, pair.second};
            SDS resp = command_to_resp(command);
            ssize_t n = write(temp_fd, resp.c_str(), resp.size());
            if (n == -1)
            {
                std::cerr << "AOF 重写时写入失败：" << strerror(errno) << std::endl;
                close(temp_fd);
                unlink(temp_filename.c_str());
                rewrite_in_progress = false;
                return false;
            }
            keys_written++;
        }

        // 获取所有 List 数据
        auto list_data = handler->get_all_list_data();
        for (const auto &pair : list_data)
        {
            const SDS &key = pair.first;
            const std::vector<SDS> &values = pair.second;
            // 对于 List，使用 RPUSH 命令恢复所有元素
            if (!values.empty())
            {
                std::vector<SDS> command = {"RPUSH", key};
                command.insert(command.end(), values.begin(), values.end());
                SDS resp = command_to_resp(command);
                ssize_t n = write(temp_fd, resp.c_str(), resp.size());
                if (n == -1)
                {
                    std::cerr << "AOF 重写时写入失败：" << strerror(errno) << std::endl;
                    close(temp_fd);
                    unlink(temp_filename.c_str());
                    rewrite_in_progress = false;
                    return false;
                }
                keys_written++;
            }
        }

        // 获取所有 ZSet 数据
        auto zset_data = handler->get_all_zset_data();
        for (const auto &pair : zset_data)
        {
            const SDS &key = pair.first;
            const std::vector<std::pair<double, SDS>> &members = pair.second;
            // 对于 ZSet，使用 ZADD 命令恢复所有成员
            if (!members.empty())
            {
                std::vector<SDS> command = {"ZADD", key};
                for (const auto &member : members)
                {
                    command.push_back(std::to_string(member.first));
                    command.push_back(member.second);
                }
                SDS resp = command_to_resp(command);
                ssize_t n = write(temp_fd, resp.c_str(), resp.size());
                if (n == -1)
                {
                    std::cerr << "AOF 重写时写入失败：" << strerror(errno) << std::endl;
                    close(temp_fd);
                    unlink(temp_filename.c_str());
                    rewrite_in_progress = false;
                    return false;
                }
                keys_written++;
            }
        }

        std::cout << "AOF rewrite: " << keys_written << " keys written\n";
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
        rewrite_in_progress = false;
        return false;
    }

    struct stat new_st;
    if (stat(filename.c_str(), &new_st) == 0)
    {
        size_t old_size = rewrite_base_size;
        rewrite_base_size = new_st.st_size;

        // 重置写入计数
        total_writes_since_rewrite = 0;

        std::cout << "AOF rewrite completed: "
                  << old_size / (1024 * 1024) << "MB -> "
                  << rewrite_base_size / (1024 * 1024) << "MB\n";
    }

    // 标记重写完成
    rewrite_in_progress = false;
    return true;
}

bool AofManager::should_auto_rewrite() const
{
    struct stat st;
    if (stat(filename.c_str(), &st) != 0)
    {
        return false;
    }

    size_t current_size = st.st_size;

    // 防止并发重写
    if (rewrite_in_progress.load())
    {
        return false;
    }

    // Redis 风格的自动重写逻辑：
    // 1. 如果 rewrite_base_size 为 0（首次启动），当文件超过最小重写阈值时触发
    // 2. 否则，当文件增长超过设定百分比时触发
    // 3. 或者写入次数超过阈值时触发

    bool should_rewrite = false;

    if (rewrite_base_size == 0)
    {
        // 首次启动，文件从无到有增长超过阈值
        if (current_size >= AUTO_REWRITE_MIN_SIZE)
        {
            should_rewrite = true;
        }
    }
    else
    {
        // 文件增长超过设定百分比
        if (current_size >= rewrite_base_size * (1.0 + AUTO_REWRITE_PERCENT))
        {
            should_rewrite = true;
        }
    }

    // 或者写入次数超过阈值（即使文件大小未达到阈值）
    if (total_writes_since_rewrite.load() >= AUTO_REWRITE_WRITE_COUNT)
    {
        should_rewrite = true;
    }

    return should_rewrite;
}

size_t AofManager::get_current_size() const
{
    struct stat st;
    if (stat(filename.c_str(), &st) != 0)
    {
        return 0;
    }
    return st.st_size;
}

size_t AofManager::get_rewrite_base_size() const
{
    return rewrite_base_size;
}

bool AofManager::is_rewrite_in_progress() const
{
    return rewrite_in_progress.load();
}

void AofManager::flush_buffer()
{
    if (write_buffer.empty() || file_fd == -1)
    {
        return;
    }

    ssize_t written = write(file_fd, write_buffer.c_str(), write_buffer.size());
    if (written != (ssize_t)write_buffer.size())
    {
        std::cerr << "Failed to write buffer to AOF file\n";
    }

    // 清空缓冲区
    write_buffer.clear();
    buffer_size = 0;
}

// 强制刷盘所有线程局部缓冲区（在服务器关闭时调用）
void AofManager::flush_all_buffers()
{
    // 这里无法直接访问其他线程的 thread_local 变量
    // 但我们可以确保在服务器正常关闭时，主线程的缓冲区被刷入
    flush_buffer();

    // 对于 ALWAYS 策略，确保数据落盘
    if (sync_policy == AofPolicy::ALWAYS && file_fd != -1)
    {
        fsync(file_fd);
    }
}

void AofManager::background_rewrite_thread()
{
    std::cout << "Starting automatic AOF rewrite...\n";
    if (rewrite())
    {
        std::cout << "Automatic AOF rewrite completed\n";
    }
    else
    {
        std::cerr << "Automatic AOF rewrite failed or skipped\n";
    }
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

        // 每秒刷盘：先刷新缓冲区，然后 fsync
        if (file_fd != -1 && sync_policy == AofPolicy::EVERYSEC)
        {
            // 先把主缓冲区写入文件，确保主缓冲区中的数据被持久化
            flush_buffer();
            fsync(file_fd);
            last_sync_time = std::chrono::steady_clock::now();
        }
    }
}

// 专用 AOF 高性能解析器实现
std::vector<SDS> AofFastParser::parse_command(const char *data, size_t size, size_t &consumed_bytes)
{
    std::vector<SDS> result;
    consumed_bytes = 0;

    if (size < 3)
        return result; // 最小 RESP 命令长度

    // 快速解析 RESP 数组格式（AOF 命令都是数组格式）
    if (data[0] == '*')
    {
        // 解析数组长度
        size_t array_len = 0;
        size_t i = 1;
        while (i < size && data[i] != '\r')
        {
            if (data[i] >= '0' && data[i] <= '9')
            {
                array_len = array_len * 10 + (data[i] - '0');
            }
            i++;
        }

        if (i + 1 >= size || data[i] != '\r' || data[i + 1] != '\n')
        {
            return result; // 格式错误
        }

        i += 2; // 跳过\r\n
        consumed_bytes = i;

        // 解析数组元素
        for (size_t elem = 0; elem < array_len && i < size; elem++)
        {
            if (data[i] == '$')
            {
                // 解析批量字符串
                i++; // 跳过'$'
                size_t str_len = 0;
                while (i < size && data[i] != '\r')
                {
                    if (data[i] >= '0' && data[i] <= '9')
                    {
                        str_len = str_len * 10 + (data[i] - '0');
                    }
                    i++;
                }

                if (i + 2 + str_len >= size || data[i] != '\r' || data[i + 1] != '\n')
                {
                    return result; // 格式错误
                }

                i += 2; // 跳过\r\n

                // 提取字符串内容（避免 SDS 构造函数中的内存分配）
                if (str_len > 0)
                {
                    SDS str;
                    str.append(data + i, str_len); // 直接 append，避免拷贝
                    result.push_back(std::move(str));
                    i += str_len;
                }
                else
                {
                    result.emplace_back(""); // 空字符串
                }

                if (i + 1 >= size || data[i] != '\r' || data[i + 1] != '\n')
                {
                    return result; // 格式错误
                }
                i += 2; // 跳过\r\n
            }
            else
            {
                return result; // 不支持的类型
            }
        }

        consumed_bytes = i;
    }

    return result;
}

std::vector<std::vector<SDS>> AofFastParser::parse_commands_batch(const char *data, size_t size, size_t &consumed_bytes)
{
    std::vector<std::vector<SDS>> result;
    consumed_bytes = 0;

    size_t pos = 0;
    while (pos < size)
    {
        size_t cmd_consumed = 0;
        auto command = parse_command(data + pos, size - pos, cmd_consumed);

        if (!command.empty() && cmd_consumed > 0)
        {
            result.push_back(std::move(command));
            pos += cmd_consumed;
        }
        else
        {
            // 解析失败，尝试跳过错误数据
            size_t next_cmd = pos + 1;
            while (next_cmd < size && data[next_cmd] != '*')
            {
                next_cmd++;
            }

            if (next_cmd < size)
            {
                pos = next_cmd;
            }
            else
            {
                break;
            }
        }
    }

    consumed_bytes = pos;
    return result;
}
