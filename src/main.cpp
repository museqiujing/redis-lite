// main.cpp 文件
#include "epoll_server.h"

int main(int argc, char* argv[]) {
    bool load_aof = true;
    int port = 6666;  // 改为6666端口，避免权限问题
    
    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--no-aof") == 0) {
            load_aof = false;
        } else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = std::atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = std::atoi(argv[i + 1]);
            i++;
        }
    }
    
    // 创建Epoll服务器，监听指定端口
    EpollServer server(port);
    std::cout << "EpollServer对象创建成功" << "\n";
    
    // 启动服务器
    std::cout << "开始启动服务器主循环..." << "\n";
    server.start();
    return 0;
}