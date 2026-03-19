// epoll_server.h
#ifndef EPOLL_SERVER_H
#define EPOLL_SERVER_H

#include <vector>
#include <unordered_map>
#include <string>
#include "command_handler.h"
#include "resp_parser.h"
class EpollServer {
public:
    EpollServer(int port , int max_events = 1024);
    ~EpollServer();
    void start();

private:
    int port;
    int epoll_fd;
    int server_fd;
    int max_events;
    std::vector<struct epoll_event> events; 
    CommandHandler handler;
    struct ClientInfo {
    int fd; // 客户端socket文件描述符
    std::string buffer; // 接收缓冲区
    RespParser parser;  // 每个客户端使用一个RespParser实例
};
    std::unordered_map<int, ClientInfo> clients;

    bool init_server();
    void handle_new_connection();
    void handle_client_data(int client_fd);
    void send_response(int client_fd, const std::string& response);
};

#endif // EPOLL_SERVER_H
