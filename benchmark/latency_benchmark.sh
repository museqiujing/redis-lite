#!/bin/bash
# latency_benchmark.sh
# 延迟分布测试（生产环境版）

echo "=== 延迟分布测试（生产环境版）==="
echo "测试时间: $(date)"
echo ""

# 测试配置
CLIENTS=50
REQUESTS=100000
DURATION=60  # 秒

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

echo "测试延迟分布..."
echo "配置: 50客户端, 10万请求, 管道大小16"
echo "--------------------------------------------"

# 测试Redis-Lite
echo "Redis-Lite 延迟分布:"
redis-benchmark -h $REDIS_LITE_HOST -p $REDIS_LITE_PORT -c $CLIENTS -n $REQUESTS -P 16 -t set,get -d 128 --precision 3
echo ""

# 测试真实Redis
echo "真实Redis 延迟分布:"
redis-benchmark -h $REAL_REDIS_HOST -p $REAL_REDIS_PORT -c $CLIENTS -n $REQUESTS -P 16 -t set,get -d 128 --precision 3
echo ""

echo "=== 测试完成 ==="