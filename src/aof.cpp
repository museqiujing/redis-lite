#include "aof.h"
#include "resp_serializer.h"
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "resp_parser.h"
#include "command_handler.h"

// 线程局部序列化缓冲区（避免每次分配内存）
thread_local SDS AofManager::tls_serialize_buffer;

AofManager::AofManager(const SDS &file, AofPolicy policy)
    : filename(file), file_fd(-1), enabled(true), sync_policy(policy),
      bg_thread_running(false), rewrite_base_size(0),
      last_sync_time(std::chrono::steady_clock::now()),
      last_fsync_time(std::chrono::steady_clock::now())
{
    // 预分配双缓冲
    write_buffer[0].reserve(WRITE_BUFFER_SIZE);
    write_buffer[1].reserve(WRITE_BUFFER_SIZE);
}

AofManager::~AofManager()
{
    // 停止后台线程
    bg_thread_running = false;

    // 刷新所有剩余数据
    flush_all_buffers();

    if (fsync_thread.joinable())
    {
        fsync_thread.join();
    }
    if (write_thread.joinable())
    {
        write_thread.join();
    }
    if (bg_thread.joinable())
    {
        bg_thread.join();
    }

    close_file();
}

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
    if (sync_policy == AofPolicy::ALWAYS)
    {
        // ALWAYS 模式：启动专职写入和 fsync 线程
        bg_thread_running = true;
        write_thread = std::thread(&AofManager::background_write_thread, this);
        fsync_thread = std::thread(&AofManager::background_fsync_thread, this);
    }
    else if (sync_policy == AofPolicy::EVERYSEC)
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
        flush_all_buffers();
        close_file();
    }
}

// 快速序列化命令到缓冲区
inline void AofManager::append_command_to_buffer(SDS &buffer, const std::vector<SDS> &command)
{
    // 估算大小
    size_t estimated = 32;
    for (const auto &arg : command)
    {
        estimated += arg.size() + 16;
    }

    // 直接序列化
    buffer.append("*");
    buffer.append(std::to_string(command.size()));
    buffer.append("\r\n");

    for (const auto &arg : command)
    {
        buffer.append("$");
        buffer.append(std::to_string(arg.size()));
        buffer.append("\r\n");
        buffer.append(arg);
        buffer.append("\r\n");
    }
}

void AofManager::write_command(const std::vector<SDS> &command)
{
    if (!enabled || file_fd == -1)
    {
        return;
    }

    if (sync_policy == AofPolicy::ALWAYS)
    {
        // ALWAYS 模式：使用无锁队列 + 异步 fsync
        // 使用线程局部缓冲区序列化
        if (tls_serialize_buffer.empty())
        {
            tls_serialize_buffer.reserve(256);
        }
        tls_serialize_buffer.clear();
        append_command_to_buffer(tls_serialize_buffer, command);

        // 创建写入任务
        AofWriteTask task;
        task.data = tls_serialize_buffer;
        task.needs_fsync = true;

        // 尝试推入无锁队列
        int retries = 0;
        while (!write_queue.push(task) && retries < 50)
        {
            std::this_thread::yield();
            retries++;
        }

        if (retries >= 50)
        {
            // 队列满，降级为同步写入
            ssize_t written = write(file_fd, task.data.c_str(), task.data.size());
            if (written > 0 && file_fd != -1)
            {
                fsync(file_fd);
                total_fsyncs.fetch_add(1, std::memory_order_relaxed);
            }
        }

        total_writes.fetch_add(1, std::memory_order_relaxed);
        total_writes_since_rewrite.fetch_add(1, std::memory_order_relaxed);
        return;
    }

    // EVERYSEC 和 NO 模式：使用双缓冲机制
    size_t buf_idx = current_buffer.load(std::memory_order_relaxed);

    // 使用线程局部缓冲区预序列化
    if (tls_serialize_buffer.empty())
    {
        tls_serialize_buffer.reserve(256);
    }
    tls_serialize_buffer.clear();
    append_command_to_buffer(tls_serialize_buffer, command);

    size_t data_size = tls_serialize_buffer.size();

    // 尝试写入当前缓冲
    size_t used = buffer_used[buf_idx].load(std::memory_order_relaxed);

    if (used + data_size > WRITE_BUFFER_SIZE)
    {
        // 缓冲区满，直接写入文件
        ssize_t written = write(file_fd, tls_serialize_buffer.c_str(), data_size);
        if (written > 0)
        {
            buffer_used[buf_idx].fetch_add(written, std::memory_order_relaxed);
        }
    }
    else
    {
        // 写入缓冲区
        write_buffer[buf_idx].append(tls_serialize_buffer);
        buffer_used[buf_idx].fetch_add(data_size, std::memory_order_relaxed);

        // 缓冲区达到阈值时刷新
        if (buffer_used[buf_idx].load(std::memory_order_acquire) > WRITE_BUFFER_SIZE * 0.8)
        {
            flush_buffer();
        }
    }

    total_writes.fetch_add(1, std::memory_order_relaxed);
    total_writes_since_rewrite.fetch_add(1, std::memory_order_relaxed);
}

void AofManager::set_sync_policy(AofPolicy policy)
{
    sync_policy = policy;

    // 停止所有后台线程
    bg_thread_running = false;

    if (fsync_thread.joinable())
    {
        fsync_thread.join();
    }
    if (write_thread.joinable())
    {
        write_thread.join();
    }
    if (bg_thread.joinable())
    {
        bg_thread.join();
    }

    // 根据新策略重启线程
    bg_thread_running = true;
    if (sync_policy == AofPolicy::ALWAYS)
    {
        write_thread = std::thread(&AofManager::background_write_thread, this);
        fsync_thread = std::thread(&AofManager::background_fsync_thread, this);
    }
    else if (sync_policy == AofPolicy::EVERYSEC)
    {
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
    return AofPolicy::EVERYSEC;
}

// ALWAYS 模式：专职写入线程，批量写入
void AofManager::background_write_thread()
{
    SDS batch_buffer;
    batch_buffer.reserve(WRITE_BUFFER_SIZE);

    while (bg_thread_running)
    {
        batch_buffer.clear();
        bool has_data = false;
        int empty_count = 0;

        // 批量收集队列中的数据（最多收集 100μs 内的数据）
        auto start_time = std::chrono::steady_clock::now();

        while (bg_thread_running)
        {
            AofWriteTask task;
            if (write_queue.pop(task))
            {
                batch_buffer.append(task.data);
                has_data = true;
            }
            else
            {
                empty_count++;
                if (empty_count > 10)
                {
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time);
                    if (elapsed.count() >= 100 || !has_data)
                    {
                        break;
                    }
                    empty_count = 0;
                }
            }

            if (batch_buffer.size() >= WRITE_BUFFER_SIZE)
            {
                break;
            }
        }

        // 批量写入
        if (has_data && file_fd != -1)
        {
            ssize_t written = write(file_fd, batch_buffer.c_str(), batch_buffer.size());
            if (written < 0)
            {
                std::cerr << "AOF write error: " << strerror(errno) << std::endl;
            }
        }
    }
}

// ALWAYS 模式：专职 fsync 线程（每 100μs fsync 一次）
void AofManager::background_fsync_thread()
{
    while (bg_thread_running)
    {
        std::this_thread::sleep_for(std::chrono::microseconds(100));

        if (file_fd != -1 && sync_policy == AofPolicy::ALWAYS)
        {
            fsync(file_fd);
            total_fsyncs.fetch_add(1, std::memory_order_relaxed);
            last_fsync_time = std::chrono::steady_clock::now();
        }
    }
}

// EVERYSEC 模式：后台刷盘线程
void AofManager::background_sync_thread()
{
    while (bg_thread_running)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        if (file_fd != -1 && sync_policy == AofPolicy::EVERYSEC)
        {
            flush_buffer();
            fsync(file_fd);
            last_sync_time = std::chrono::steady_clock::now();
        }
    }
}

bool AofManager::load()
{
    struct stat st;
    if (stat(filename.c_str(), &st) != 0)
    {
        return false;
    }

    const bool old_enabled = enabled;
    enabled = false;

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

    enabled = old_enabled;

    auto end_time = std::chrono::steady_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);

    std::cout << "\nAOF loading completed: " << commands_loaded << " commands in "
              << total_time.count() << "ms\n";

    return ok;
}

bool AofManager::rewrite()
{
    bool expected = false;
    if (!rewrite_in_progress.compare_exchange_strong(expected, true))
    {
        std::cout << "AOF rewrite already in progress, skipping\n";
        return false;
    }

    std::cout << "Starting AOF rewrite...\n";

    flush_buffer();

    std::string temp_filename = filename + ".tmp";
    int temp_fd = open(temp_filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (temp_fd == -1)
    {
        std::cerr << "Failed to create temporary AOF file: " << temp_filename << std::endl;
        rewrite_in_progress = false;
        return false;
    }

    if (handler)
    {
        size_t keys_written = 0;

        auto string_data = handler->get_all_string_data();
        for (const auto &pair : string_data)
        {
            std::vector<SDS> command = {"SET", pair.first, pair.second};
            SDS resp = RespSerializer::serialize_array(command);
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

        auto list_data = handler->get_all_list_data();
        for (const auto &pair : list_data)
        {
            const SDS &key = pair.first;
            const std::vector<SDS> &values = pair.second;
            if (!values.empty())
            {
                std::vector<SDS> command = {"RPUSH", key};
                command.insert(command.end(), values.begin(), values.end());
                SDS resp = RespSerializer::serialize_array(command);
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

        auto zset_data = handler->get_all_zset_data();
        for (const auto &pair : zset_data)
        {
            const SDS &key = pair.first;
            const std::vector<std::pair<double, SDS>> &members = pair.second;
            if (!members.empty())
            {
                std::vector<SDS> command = {"ZADD", key};
                for (const auto &member : members)
                {
                    command.push_back(std::to_string(member.first));
                    command.push_back(member.second);
                }
                SDS resp = RespSerializer::serialize_array(command);
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

    fsync(temp_fd);
    close(temp_fd);

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

        total_writes_since_rewrite = 0;

        std::cout << "AOF rewrite completed: "
                  << old_size / (1024 * 1024) << "MB -> "
                  << rewrite_base_size / (1024 * 1024) << "MB\n";
    }

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

    if (rewrite_in_progress.load())
    {
        return false;
    }

    bool should_rewrite = false;

    if (rewrite_base_size == 0)
    {
        if (current_size >= AUTO_REWRITE_MIN_SIZE)
        {
            should_rewrite = true;
        }
    }
    else
    {
        if (current_size >= rewrite_base_size * (1.0 + AUTO_REWRITE_PERCENT))
        {
            should_rewrite = true;
        }
    }

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
    size_t buf_idx = current_buffer.load(std::memory_order_acquire);
    size_t other_idx = (buf_idx + 1) % 2;

    size_t used = buffer_used[other_idx].load(std::memory_order_acquire);
    if (used > 0 && file_fd != -1)
    {
        ssize_t written = write(file_fd, write_buffer[other_idx].c_str(), used);
        if (written > 0)
        {
            buffer_used[other_idx].store(0, std::memory_order_release);
            write_buffer[other_idx].clear();
        }
    }
}

void AofManager::flush_all_buffers()
{
    for (size_t i = 0; i < 2; i++)
    {
        size_t used = buffer_used[i].load(std::memory_order_acquire);
        if (used > 0 && file_fd != -1)
        {
            ssize_t written = write(file_fd, write_buffer[i].c_str(), used);
            if (written > 0)
            {
                buffer_used[i].store(0, std::memory_order_release);
                write_buffer[i].clear();
            }
        }
    }

    // 处理无锁队列中的剩余数据
    AofWriteTask task;
    while (write_queue.pop(task))
    {
        if (file_fd != -1)
        {
            ssize_t written = write(file_fd, task.data.c_str(), task.data.size());
            (void)written;
        }
    }

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

std::vector<SDS> AofFastParser::parse_command(const char *data, size_t size, size_t &consumed_bytes)
{
    std::vector<SDS> result;
    consumed_bytes = 0;

    if (size < 3)
        return result;

    if (data[0] == '*')
    {
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
            return result;
        }

        i += 2;
        consumed_bytes = i;

        for (size_t elem = 0; elem < array_len && i < size; elem++)
        {
            if (data[i] == '$')
            {
                i++;
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
                    return result;
                }

                i += 2;

                if (str_len > 0)
                {
                    SDS str;
                    str.append(data + i, str_len);
                    result.push_back(std::move(str));
                    i += str_len;
                }
                else
                {
                    result.emplace_back("");
                }

                if (i + 1 >= size || data[i] != '\r' || data[i + 1] != '\n')
                {
                    return result;
                }
                i += 2;
            }
            else
            {
                return result;
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
