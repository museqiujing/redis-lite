#ifndef AOF_H
#define AOF_H

#include <string>
#include <vector>


class CommandHandler; // 前向声明

class AofManager {
private:
    std::string filename;
    int file_fd;
    bool enabled;
    CommandHandler* handler;
   // std::string sync_policy;  // 持久化策略："always", "everysec", "no"

public:
    
    //AofManager(const std::string& file = "appendonly.aof", const std::string& policy = "everysec");
    AofManager(const std::string& file = "appendonly.aof");
    ~AofManager();

    // 初始化AOF
    bool init(CommandHandler* cmd_handler);

    // 启用/禁用AOF
    void set_enabled(bool enable);

    // 写入命令到AOF文件
    void write_command(const std::vector<std::string>& command);

    // 加载AOF文件
    bool load();

    // 重写AOF文件
    bool rewrite();

    // 设置持久化策略
    //void set_sync_policy(const std::string& policy);
private:
    // 打开AOF文件
    bool open_file();

    // 关闭AOF文件
    void close_file();

    // 将命令转换为RESP格式
    std::string command_to_resp(const std::vector<std::string>& command);
};

#endif // AOF_H