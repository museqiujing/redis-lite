#!/bin/bash
# memory_monitor.sh
# 内存使用监控

echo "=== 内存使用监控 ==="
echo "测试时间: $(date)"
echo ""

# 启动Redis-Lite（如果未运行）
if ! pgrep -f "redis-list" > /dev/null; then
    echo "启动Redis-Lite..."
    ../build/redis-list &
    sleep 3
fi

# 获取PID
REDIS_PID=$(pgrep -f "redis-list")
echo "Redis-Lite PID: $REDIS_PID"
echo "--------------------------------------------"

# 监控内存使用
echo "时间,内存使用(MB),CPU使用率(%)" > memory_usage.csv

for i in {1..60}; do  # 监控60次，每次1秒
    # 获取内存使用
    MEM_USAGE=$(ps -o rss= -p $REDIS_PID | awk '{print $1/1024}')
    # 获取CPU使用率
    CPU_USAGE=$(ps -o %cpu= -p $REDIS_PID | awk '{print $1}')
    # 获取当前时间
    CURRENT_TIME=$(date +"%H:%M:%S")
    
    echo "$CURRENT_TIME,$MEM_USAGE,$CPU_USAGE" >> memory_usage.csv
    echo "$CURRENT_TIME - 内存: ${MEM_USAGE}MB, CPU: ${CPU_USAGE}%"
    
    sleep 1
done

echo "=== 监控完成 ==="
echo "结果已保存到 memory_usage.csv"
