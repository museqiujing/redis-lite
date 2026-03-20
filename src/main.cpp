// main.cpp 简化版本
#include "epoll_server.h"
#include "aof.h"

int main(int argc, char *argv[])
{
    AofPolicy aof_policy = AofPolicy::NO;
    int port = 6666;

    // 解析参数
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--aof-policy") == 0 && i + 1 < argc)
        {
            // 直接解析策略
            if (strcmp(argv[i + 1], "always") == 0)
                aof_policy = AofPolicy::ALWAYS;
            else if (strcmp(argv[i + 1], "everysec") == 0)
                aof_policy = AofPolicy::EVERYSEC;
            else if (strcmp(argv[i + 1], "no") == 0)
                aof_policy = AofPolicy::NO;
            i++;
        }
    }

    // 创建服务器（服务器内部会处理AOF）
    EpollServer server(port, aof_policy);
    server.start();

    return 0;
}