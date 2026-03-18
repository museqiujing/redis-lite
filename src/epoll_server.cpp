// 修改后的epoll_server.cpp
#include "epoll_server.h"
#include "resp_parser.h"
#include "command_handler.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>


EpollServer::EpollServer(int port, int max_events) 
    : port(port), max_events(max_events), epoll_fd(-1), server_fd(-1) {
    events.resize(max_events);
}

EpollServer::~EpollServer() {
    if (server_fd != -1) {
        close(server_fd);
    }
    if (epoll_fd != -1) {
        close(epoll_fd);
    }
    for (auto& client : clients) {
        close(client.first);
    }
}

bool EpollServer::init_server() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "创建服务器socket失败: " << strerror(errno) << std::endl;
        return false;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        std::cerr << "设置服务器socket选项失败: " << strerror(errno) << std::endl;
        close(server_fd);
        return false;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        std::cerr << "绑定服务器socket失败: " << strerror(errno) << std::endl;
        close(server_fd);
        return false;
    }

    if (listen(server_fd, SOMAXCONN) == -1) {
        std::cerr << "监听服务器socket失败: " << strerror(errno) << std::endl;
        close(server_fd);
        return false;
    }

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        std::cerr << "创建epoll实例失败: " << strerror(errno) << std::endl;
        close(server_fd);
        return false;
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        std::cerr << "将服务器socket添加到epoll实例失败: " << strerror(errno) << std::endl;
        close(server_fd);
        close(epoll_fd);
        return false;
    }

    std::cout << "服务器监听端口: " << port << std::endl;
    return true;
}

void EpollServer::start() {
    if (!init_server()) {
        return;
    }

    while (true) {
        int nfds = epoll_wait(epoll_fd, events.data(), max_events, -1);
        if (nfds == -1) {
            std::cerr << "epoll等待事件失败: " << strerror(errno) << std::endl;
            break;
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == server_fd) {
                handle_new_connection();
            } else {
                handle_client_data(events[i].data.fd);
            }
        }
    }
}

void EpollServer::handle_new_connection() {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
    if (client_fd == -1) {
        std::cerr << "接受新连接失败: " << strerror(errno) << std::endl;
        return;
    }

    int flags = fcntl(client_fd, F_GETFL, 0);
    if (fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "设置客户端socket为非阻塞模式失败: " << strerror(errno) << std::endl;
        close(client_fd);
        return;
    }

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = client_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
        std::cerr << "将客户端socket添加到epoll实例失败: " << strerror(errno) << std::endl;
        close(client_fd);
        return;
    }

    clients[client_fd] = {client_fd, ""};
    std::cout << " 新连接: " << inet_ntoa(client_addr.sin_addr) 
              << ":" << ntohs(client_addr.sin_port) << std::endl;
}

void EpollServer::handle_client_data(int client_fd) {
    char buffer[4096];
    ssize_t n = read(client_fd, buffer, sizeof(buffer));
    if (n <= 0) {
        // 连接关闭或错误处理
        if (n < 0) {
            std::cerr << " 读取客户端数据失败: " << strerror(errno) << std::endl;
        } else {
            std::cout << " 客户端 " << client_fd << " 断开连接" << std::endl;
        }
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        close(client_fd);
        clients.erase(client_fd);
    } else {
        // 处理接收到的数据
        clients[client_fd].buffer.append(buffer, n);
        
        try {
            // 使用客户端的parser实例
            std::vector<std::string> command = clients[client_fd].parser.parse(clients[client_fd].buffer);
            
            if (!command.empty()) {
                // 打印接收到的命令
                std::cout << " 接收到命令: ";
                for (const auto& arg : command) {
                    std::cout << arg << " ";
                }
                std::cout << std::endl;
                
                // 处理命令
                std::string response = handler.handle_command(command);
                // 打印响应
                std::cout << " 响应: " << response << std::endl;
                // 发送响应
                send_response(client_fd, response);
                
                 // 清空缓冲区，因为解析已经完成
                clients[client_fd].buffer.clear();
            }
        } catch (const std::exception& e) {
            // 解析失败，保留未解析的数据
            clients[client_fd].buffer = clients[client_fd].parser.get_remaining_data();
            std::cerr << "Error parsing command: " << e.what() << std::endl;
        }
    }
}

void EpollServer::send_response(int client_fd, const std::string& response) {
    ssize_t n = write(client_fd, response.c_str(), response.size());
    if (n == -1) {
        std::cerr << " 发送响应失败: " << strerror(errno) << std::endl;
    }
}