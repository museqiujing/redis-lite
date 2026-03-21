#!/bin/bash
# stability_benchmark.sh
# 长时间稳定性测试（生产环境版）

echo "=== 长时间稳定性测试（生产环境版）==="
echo "测试时间: $(date)"
echo "预计持续时间: 30分钟"
echo ""

# 测试配置
CLIENTS=20
REQUESTS=1000000  # 100万请求
DURATION=1800  # 30分钟

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

echo "开始长时间稳定性测试..."
echo "配置: 20客户端, 100万请求, 30分钟"
echo "--------------------------------------------"

# 测试Redis-Lite
echo "测试Redis-Lite 稳定性..."
redis-benchmark -h $REDIS_LITE_HOST -p $REDIS_LITE_PORT -c $CLIENTS -n $REQUESTS -P 16 -t set,get -d 128 --csv > redis_lite_stability_results.csv

# 测试真实Redis
echo "测试真实Redis 稳定性..."
redis-benchmark -h $REAL_REDIS_HOST -p $REAL_REDIS_PORT -c $CLIENTS -n $REQUESTS -P 16 -t set,get -d 128 --csv > real_redis_stability_results.csv

echo "=== 测试完成 ==="
echo "Redis-Lite 结果已保存到 redis_lite_stability_results.csv"
echo "真实Redis 结果已保存到 real_redis_stability_results.csv"