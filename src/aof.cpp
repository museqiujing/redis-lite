#include "aof.h"
#include "resp_serializer.h"
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "resp_parser.h"
#include "command_handler.h"
AofManager::AofManager(const std::string& file) : filename(file), file_fd(-1), enabled(true) {}

AofManager::~AofManager() {
    close_file();
}

bool AofManager::init(CommandHandler* cmd_handler) {
   
    handler = cmd_handler;
    return open_file();
}

void AofManager::set_enabled(bool enable) {
    enabled = enable;
    if (enable && file_fd == -1) {
        open_file();
    } else if (!enable && file_fd != -1) {
        close_file();
    }
}

void AofManager::write_command(const std::vector<std::string>& command) {
    if (!enabled || file_fd == -1) {
        return;
    }

    std::string resp = command_to_resp(command);
    ssize_t written = write(file_fd, resp.c_str(), resp.size());
    if (written != (ssize_t)resp.size()) {
        std::cerr << "Failed to write to AOF file" << std::endl;
    }

    // 强制刷新到磁盘
    fsync(file_fd);
}

bool AofManager::load() {
    std::ifstream file(filename,std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // 读取整个文件内容
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();

    // 解析RESP格式的命令
    if (!content.empty()) {
         size_t pos = 0;
        while (pos < content.size()) {
            RespParser parser;
            try {
                // 解析单个命令
                std::vector<std::string> command = parser.parse(content.substr(pos));
                if (!command.empty() && handler) {
                    // 执行命令
                    handler->handle_command(command);
                    
                    // 计算命令的长度，移动pos指针
                    // 这里简化处理，实际应该根据解析结果计算长度
                    // 找到下一个命令的开始位置
                    size_t next_pos = content.find("*", pos + 1);
                    if (next_pos != std::string::npos) {
                        pos = next_pos;
                    } else {
                        break;
                    }
                } else {
                    break;
                }
                 } catch (const std::exception& e) {
                std::cerr << "Error parsing AOF file: " << e.what() << std::endl;
                break;
            }
        }
    }
    return true;
}

bool AofManager::rewrite() {
    // 创建临时AOF文件
    std::string temp_filename = filename + ".tmp";
    int temp_fd = open(temp_filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (temp_fd == -1) {
        std::cerr << "Failed to create temporary AOF file: " << temp_filename << std::endl;
        return false;
    }

    // 遍历所有数据结构，生成最小命令集
    if (handler) {
        // 获取所有String数据
        auto string_data = handler->get_all_string_data();
        for (const auto& pair : string_data) {
            std::vector<std::string> command = {"SET", pair.first, pair.second};
            std::string resp = command_to_resp(command);
            write(temp_fd, resp.c_str(), resp.size());
        }
        
        // 获取所有List数据
        auto list_data = handler->get_all_list_data();
        for (const auto& pair : list_data) {
            const std::string& key = pair.first;
            const std::vector<std::string>& values = pair.second;
            // 对于List，使用RPUSH命令恢复所有元素
            if (!values.empty()) {
                std::vector<std::string> command = {"RPUSH", key};
                command.insert(command.end(), values.begin(), values.end());
                std::string resp = command_to_resp(command);
                write(temp_fd, resp.c_str(), resp.size());
            }
        }
        
        // 获取所有ZSet数据
        auto zset_data = handler->get_all_zset_data();
        for (const auto& pair : zset_data) {
            const std::string& key = pair.first;
            const std::vector<std::pair<double, std::string>>& members = pair.second;
            // 对于ZSet，使用ZADD命令恢复所有成员
            if (!members.empty()) {
                std::vector<std::string> command = {"ZADD", key};
                for (const auto& member : members) {
                    command.push_back(std::to_string(member.first));
                    command.push_back(member.second);
                }
                std::string resp = command_to_resp(command);
                write(temp_fd, resp.c_str(), resp.size());
            }
        }
    }

    // 强制刷新到磁盘
    fsync(temp_fd);
    
    // 关闭临时文件
    close(temp_fd);

    // 替换旧文件
    if (rename(temp_filename.c_str(), filename.c_str()) == -1) {
        std::cerr << "Failed to rename temporary AOF file" << std::endl;
        unlink(temp_filename.c_str());
        return false;
    }

    std::cout << "AOF rewrite completed successfully" << std::endl;
    return true;
}

bool AofManager::open_file() {
    // 以追加模式打开文件
    file_fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (file_fd == -1) {
        std::cerr << "Failed to open AOF file: " << filename << std::endl;
        return false;
    }
    return true;
}

void AofManager::close_file() {
    if (file_fd != -1) {
        close(file_fd);
        file_fd = -1;
    }
}

std::string AofManager::command_to_resp(const std::vector<std::string>& command) {
    return RespSerializer::serialize_array(command);
}