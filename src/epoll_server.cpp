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
    : port(port), max_events(max_events), epoll_fd(-1), server_fd(-1)
{
    events.resize(max_events);
}

EpollServer::~EpollServer()
{
    if (server_fd != -1)
    {
        close(server_fd);
    }
    if (epoll_fd != -1)
    {
        close(epoll_fd);
    }
    for (auto &client : clients)
    {
        close(client.first);
    }
}

bool EpollServer::init_server()
{
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        std::cerr << "创建服务器socket失败: " << strerror(errno) << "\n";
        return false;
    }

    // 设置socket选项，允许地址重用.保证了在程序退出后，端口立即被重新使用,可以快速重启.
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        std::cerr << "设置服务器socket选项失败: " << strerror(errno) << "\n";
        close(server_fd);
        return false;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        std::cerr << "绑定服务器socket失败: " << strerror(errno) << "\n";
        close(server_fd);
        return false;
    }

    if (listen(server_fd, SOMAXCONN) == -1)
    {
        std::cerr << "监听服务器socket失败: " << strerror(errno) << "\n";
        close(server_fd);
        return false;
    }

    epoll_fd = epoll_create1(0); // 创建epoll实例
    if (epoll_fd == -1)
    {
        std::cerr << "创建epoll实例失败: " << strerror(errno) << "\n";
        close(server_fd);
        return false;
    }

    struct epoll_event event;  // 事件结构体
    event.events = EPOLLIN;    // 事件类型，EPOLLIN表示可读事件
    event.data.fd = server_fd; // 事件数据，服务器socket的文件描述符
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1)
    {
        std::cerr << "将服务器socket添加到epoll实例失败: " << strerror(errno) << "\n";
        close(server_fd);
        close(epoll_fd);
        return false;
    }

    std::cout << "服务器监听端口: " << port << "\n";
    return true;
}

void EpollServer::start()
{

    if (!init_server())
    {
        return;
    }

    while (true)
    {

        int nfds = epoll_wait(epoll_fd, events.data(), max_events, -1);
        if (nfds == -1)
        {
            std::cerr << "epoll等待事件失败: " << strerror(errno) << "\n";
            break;
        }

        for (int i = 0; i < nfds; i++)
        {
            if (events[i].data.fd == server_fd)
            {
                handle_new_connection();
            }
            else
            {
                handle_client_data(events[i].data.fd);
            }
        }
    }
}

void EpollServer::handle_new_connection()
{
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    // 接受新连接，三次握手完成
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_fd == -1)
    {
        std::cerr << "接受新连接失败: " << strerror(errno) << "\n";
        return;
    }

    // 设置客户端socket为非阻塞模式
    int flags = fcntl(client_fd, F_GETFL, 0);
    if (fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        std::cerr << "设置客户端socket为非阻塞模式失败: " << strerror(errno) << "\n";
        close(client_fd);
        return;
    }

    // 将客户端socket添加到epoll实例
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = client_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1)
    {
        std::cerr << "将客户端socket添加到epoll实例失败: " << strerror(errno) << "\n";
        close(client_fd);
        return;
    }

    clients[client_fd] = {client_fd, ""};
    std::cout << " 新连接: " << inet_ntoa(client_addr.sin_addr)
              << ":" << ntohs(client_addr.sin_port) << "\n";
}

void EpollServer::handle_client_data(int client_fd)
{
    // 边缘触发模式下的智能读取策略
    int consecutive_empty_reads = 0;
    const int max_consecutive_empty = 3; // 连续3次空读取才确认没有数据

    while (consecutive_empty_reads < max_consecutive_empty)
    {
        char buffer[4096];
        ssize_t n = read(client_fd, buffer, sizeof(buffer));

        if (n > 0)
        {
            consecutive_empty_reads = 0; // 重置计数器，因为有数据
            clients[client_fd].buffer.append(buffer, n);

            // 如果读取的数据量小于缓冲区大小，可能接近数据末尾
            if (n < static_cast<ssize_t>(sizeof(buffer)))
            {
                consecutive_empty_reads++; // 接近结束，但还要再试一次
            }
        }
        else if (n == 0)
        {
            // 连接关闭
            std::cout << " 客户端 " << client_fd << " 断开连接" << "\n";
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
            close(client_fd);
            clients.erase(client_fd);
            return;
        }
        else if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                consecutive_empty_reads++; // 没有数据可读
            }
            else
            {
                // 真正的读取错误
                std::cerr << " 读取客户端数据失败: " << strerror(errno) << "\n";
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                close(client_fd);
                clients.erase(client_fd);
                return;
            }
        }
    }

    // 调试信息
    if (consecutive_empty_reads >= max_consecutive_empty)
    {
        std::cout << " 边缘触发读取完成，缓冲区大小: "
                  << clients[client_fd].buffer.size() << " 字节" << std::endl;
    }

    // 解析命令 - 使用更智能的退出条件
    if (clients[client_fd].buffer.empty())
    {
        return; // 没有数据需要解析
    }

    // 保存初始缓冲区大小用于进度检测
    size_t initial_buffer_size = clients[client_fd].buffer.size();
    int no_progress_count = 0;
    const int max_no_progress = 3; // 连续3次没有进展则退出

    while (!clients[client_fd].buffer.empty() && no_progress_count < max_no_progress)
    {
        size_t buffer_size_before = clients[client_fd].buffer.size();

        try
        {
            // 检查是否有完整命令
            if (!clients[client_fd].parser.has_complete_request())
            {
                break; // 没有完整命令，退出循环
            }

            // 解析命令
            std::vector<std::string> command = clients[client_fd].parser.parse(clients[client_fd].buffer);

            if (!command.empty())
            {
                // 处理命令
                std::string response = handler.handle_command(command);
                // 发送响应
                send_response(client_fd, response);

                // 使用get_consumed_bytes()来获取实际消耗的字节数
                size_t consumed = clients[client_fd].parser.get_consumed_bytes();
                if (consumed > 0)
                {
                    // 保留未解析的数据
                    clients[client_fd].buffer = clients[client_fd].buffer.substr(consumed);
                    no_progress_count = 0; // 有进展，重置计数器

                    std::cout << " 成功解析命令，消耗 " << consumed << " 字节，剩余 "
                              << clients[client_fd].buffer.size() << " 字节" << std::endl;
                }
                else
                {
                    // 解析成功但未消耗字节，可能是协议问题
                    std::cerr << "警告：解析成功但未消耗字节，清空缓冲区" << "\n";
                    clients[client_fd].buffer.clear();
                    break;
                }
            }
            else
            {
                // 解析返回空命令，清空缓冲区避免死循环
                std::cerr << "警告：解析返回空命令数组，清空缓冲区" << std::endl;
                clients[client_fd].buffer.clear();
                break;
            }
        }
        catch (const std::exception &e)
        {
            // 解析失败，保留未解析的数据
            clients[client_fd].buffer = clients[client_fd].parser.get_remaining_data();
            std::cerr << "Error parsing command: " << e.what() << "\n";

            // 检查是否有进展
            if (clients[client_fd].buffer.size() >= buffer_size_before)
            {
                no_progress_count++; // 没有进展
            }
            else
            {
                no_progress_count = 0; // 有进展
            }
        }
    }

    if (no_progress_count >= max_no_progress)
    {
        std::cerr << "警告：解析没有进展，保留 " << clients[client_fd].buffer.size()
                  << " 字节未处理数据" << std::endl;
    }
}

void EpollServer::send_response(int client_fd, const std::string &response)
{
    ssize_t n = write(client_fd, response.c_str(), response.size());
    if (n == -1)
    {
        std::cerr << " 发送响应失败: " << strerror(errno) << "\n";
    }
}