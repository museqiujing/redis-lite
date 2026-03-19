#!/bin/bash

echo "=== Redis-Lite 关键性能测试 ==="

# 启动服务器
cd /home/autumn/write_cpp/redis-list/build
./redis-list &
SERVER_PID=$!
echo "服务器启动,PID: $SERVER_PID"
sleep 2

echo "1. 基础SET/GET性能 (50并发,10万请求)"
redis-benchmark -h 127.0.0.1 -p 6380 -c 50 -n 100000 -t set,get -q

echo "2. 不同并发级别测试"
for conn in 10 50 100 200; do
    echo "并发数: $conn"
    redis-benchmark -h 127.0.0.1 -p 6380 -c $conn -n 50000 -q | grep -E "(SET|GET)"
done

echo "3. 管道性能测试 (管道大小16)"
redis-benchmark -h 127.0.0.1 -p 6380 -c 50 -n 100000 -P 16 -q | grep -E "(SET|GET)"

echo "4. 数据结构操作性能"
redis-benchmark -h 127.0.0.1 -p 6380 -c 50 -n 50000 -t lpush,lpop,zadd,zrange -q

# 停止服务器
kill $SERVER_PID
wait $SERVER_PID

echo "=== 关键性能测试完成 ==="