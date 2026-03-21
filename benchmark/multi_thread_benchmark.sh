#!/bin/bash
# multi_thread_benchmark.sh
# 多线程并发性能测试（生产环境版）

echo "=== 多线程并发性能测试（生产环境版）==="
echo "测试时间: $(date)"
echo ""

# 测试配置
THREADS=(1 4 8 16 32)  # 生产环境常见并发数
DATA_SIZES=(64 128 256)  # 生产环境常见数据大小
DURATION=30  # 秒

# 服务器配置
REDIS_LITE_HOST="127.0.0.1"
REDIS_LITE_PORT="6666"
REAL_REDIS_HOST="127.0.0.1"
REAL_REDIS_PORT="6379"

# 检查Redis-Lite是否运行
if ! pgrep -f "redis-list" > /dev/null; then
    echo "启动Redis-Lite..."
    ../build/redis-list &
    sleep 3
fi

# 检查真实Redis是否运行
if ! pgrep -f "redis-server" > /dev/null; then
    echo "启动真实Redis..."
    redis-server --daemonize yes
    sleep 3
fi

# 循环测试不同线程数和数据大小
for threads in "${THREADS[@]}"; do
    for size in "${DATA_SIZES[@]}"; do
        echo "线程数: $threads, 数据大小: $size bytes"
        echo "--------------------------------------------"
        
        # 测试Redis-Lite
        echo "Redis-Lite 性能:"
        redis-benchmark -h $REDIS_LITE_HOST -p $REDIS_LITE_PORT -c $threads -n 50000 -P 16 -d $size -t set,get -q
        echo ""
        
        # 测试真实Redis
        echo "真实Redis 性能:"
        redis-benchmark -h $REAL_REDIS_HOST -p $REAL_REDIS_PORT -c $threads -n 50000 -P 16 -d $size -t set,get -q
        echo ""
    done
done

echo "=== 测试完成 ==="