// main.cpp 文件
#include "epoll_server.h"

int main() {
    // 创建Epoll服务器，监听默认端口6379
    EpollServer server;
    // 启动服务器
    server.start();
    return 0;
}
