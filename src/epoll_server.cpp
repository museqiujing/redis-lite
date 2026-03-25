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
#include <sys/uio.h>

EpollServer::EpollServer(int port, AofPolicy aof_policy, int max_events)
    : port(port), epoll_fd(-1), server_fd(-1), max_events(max_events), handler(aof_policy)
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

    clients[client_fd] = ClientInfo{client_fd, SDS()};
    std::cout << " 新连接: " << inet_ntoa(client_addr.sin_addr)
              << ":" << ntohs(client_addr.sin_port) << "\n";
}

void EpollServer::handle_client_data(int client_fd)
{
    auto it = clients.find(client_fd);
    if (it == clients.end())
    {
        return;
    }

    ClientInfo &client = it->second;

    if (!client.write_buffer.empty())
    {
        struct iovec iov[1];
        iov[0].iov_base = (void *)client.write_buffer.c_str();
        iov[0].iov_len = client.write_buffer.size();

        ssize_t n = writev(client_fd, iov, 1);
        if (n == -1)
        {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                std::cerr << "发送响应失败: " << strerror(errno) << "\n";
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                close(client_fd);
                clients.erase(client_fd);
                return;
            }
            // 写缓冲区还没发出去，等下次 EPOLLOUT
        }
        else if (n > 0)
        {
            if (n < static_cast<ssize_t>(client.write_buffer.size()))
            {
                client.write_buffer.erase_prefix(static_cast<size_t>(n));
                client.writing = true;
            }
            else
            {
                client.write_buffer.clear();
                client.writing = false;

                struct epoll_event event;
                event.events = EPOLLIN | EPOLLET;
                event.data.fd = client_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &event);
            }
        }
    }

    // 2) ET 模式：必须一直读到 EAGAIN，不能因为 n < bufsize 就停
    while (true)
    {
        char tmp[64 * 1024];
        ssize_t n = read(client_fd, tmp, sizeof(tmp));

        if (n > 0)
        {
            client.buffer.append(tmp, n);
            continue;
        }

        if (n == 0)
        {
            std::cout << "客户端 " << client_fd << " 断开连接\n";
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
            close(client_fd);
            clients.erase(client_fd);
            return;
        }

        // n < 0
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            break; // 本轮已经读空
        }

        std::cerr << "读取客户端数据失败: " << strerror(errno) << "\n";
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        close(client_fd);
        clients.erase(client_fd);
        return;
    }

    if (client.buffer.empty())
    {
        return;
    }

    try
    {
        client.parser.set_buffer(client.buffer);

        std::vector<std::vector<SDS>> commands = client.parser.parse_batch(SDS());

        if (!commands.empty())
        {
            if (commands.size() == 1)
            {
                SDS response = handler.handle_command(commands[0]);
                send_response(client_fd, response);
            }
            else
            {
                std::vector<SDS> responses = handler.handle_commands_batch(commands);

                SDS combined_response;
                size_t total_size = 0;
                for (const auto &resp : responses)
                {
                    total_size += resp.size();
                }
                combined_response.reserve(total_size);

                for (const auto &resp : responses)
                {
                    combined_response += resp;
                }

                send_response(client_fd, combined_response);
            }
        }

        // 4) 只删除已消费的完整请求，残留半包继续保留
        size_t consumed = client.parser.get_consumed_bytes();
        if (consumed > 0)
        {
            client.buffer.erase_prefix(consumed);
        }
    }
    catch (const std::exception &e)
    {
        std::string msg = e.what();
        std::cerr << "Error parsing command: " << msg << "\n";

        // 半包：不回错，保留已有缓冲，等下一次 read 补齐
        if (msg.find("Incomplete") != std::string::npos)
        {
            return;
        }

        // 真协议错误：清空这次坏数据，避免死循环
        client.buffer.clear();
        client.parser.reset_parser();

        SDS error_response = RespSerializer::serialize_error(msg);
        send_response(client_fd, error_response);
    }
}

void EpollServer::send_response(int client_fd, const SDS &response)
{
    if (response.empty())
    {
        return;
    }

    // 新增：获取客户端信息
    auto it = clients.find(client_fd);
    if (it == clients.end())
        return;
    ClientInfo &client = it->second;

    // 如果有未完成的写入，追加到写入缓冲区
    if (!client.write_buffer.empty())
    {
        client.write_buffer += response;
        client.writing = true;

        struct epoll_event event;
        event.events = EPOLLIN | EPOLLOUT | EPOLLET;
        event.data.fd = client_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &event);
        return;
    }

    // iovec结构
    struct iovec iov[1];
    iov[0].iov_base = (void *)response.c_str();
    iov[0].iov_len = response.size();

    while (iov[0].iov_len > 0)
    {
        ssize_t n = writev(client_fd, iov, 1);
        if (n == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // 保存剩余未发送的数据
                size_t sent = response.size() - iov[0].iov_len;
                client.write_buffer = response.substr(sent);
                // 注册可写事件
                struct epoll_event event;
                event.events = EPOLLIN | EPOLLOUT | EPOLLET;
                event.data.fd = client_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &event);
                break;
            }
            else
            {
                std::cerr << " 发送响应失败: " << strerror(errno) << "\n";
                break;
            }
        }
        else if (n < static_cast<ssize_t>(iov[0].iov_len))
        {
            // 部分写入，保存剩余数据
            size_t sent = response.size() - iov[0].iov_len + n;
            client.write_buffer = response.substr(sent);
            client.writing = true;
            // 注册可写事件
            struct epoll_event event;
            event.events = EPOLLIN | EPOLLOUT | EPOLLET;
            event.data.fd = client_fd;
            epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &event);
            break;
        }
        else if (n == 0)
        {
            // 连接已关闭
            break;
        }

        iov[0].iov_base = (char *)iov[0].iov_base + n;
        iov[0].iov_len -= n;
    }
}

// 添加AOF管理方法
void EpollServer::set_aof_policy(AofPolicy policy)
{
    handler.set_aof_policy(policy);
}

AofPolicy EpollServer::get_aof_policy() const
{
    return handler.get_current_aof_policy();
}