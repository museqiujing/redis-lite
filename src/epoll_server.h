// epoll_server.h
#ifndef EPOLL_SERVER_H
#define EPOLL_SERVER_H

#include <vector>
#include <unordered_map>
#include <string>
#include "command_handler.h"
#include "resp_parser.h"
#include "sds.h"
#include "aof.h"
#include "resp_serializer.h"
class EpollServer
{
public:
    EpollServer(int port, AofPolicy aof_policy = AofPolicy::NO, int max_events = 1024);
    ~EpollServer();
    void start();
    // 修改AOF管理方法
    void set_aof_policy(AofPolicy policy);
    AofPolicy get_aof_policy() const;

    CommandHandler &get_command_handler() { return handler; }

private:
    int port; // 服务器监听端口
    int epoll_fd;
    int server_fd;
    int max_events;
    std::vector<struct epoll_event> events;
    CommandHandler handler;
    struct ClientInfo
    {
        int fd;            // 客户端socket文件描述符
        SDS buffer;        // 接收缓冲区
        SDS write_buffer;  // 发送缓冲区，用于非阻塞写入
        RespParser parser; // 每个客户端使用一个RespParser实例
        bool writing;      // 标志位，用于判断是否正在写入响应
    };
    std::unordered_map<int, ClientInfo> clients;

    bool init_server();                     // 初始化服务器
    void handle_new_connection();           // 处理新连接
    void handle_client_data(int client_fd); // 处理客户端数据
    // 发送响应给客户端
    void send_response(int client_fd, const SDS &response);
};

#endif // EPOLL_SERVER_H
