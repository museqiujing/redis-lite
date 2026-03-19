#!/bin/bash

echo "=== 性能对比测试 ==="

# 测试标准Redis
echo "1. 标准Redis性能"
redis-benchmark -h 127.0.0.1 -p 6379 -c 5 -n 50000 -t set,get -q

# 测试Redis-Lite
echo "2. Redis-Lite性能" 
redis-benchmark -h 127.0.0.1 -p 6666 -c 5 -n 50000 -t set,get -q

# 管道性能测试
echo "3. 管道性能对比（管道大小16）"
echo "标准Redis:"
redis-benchmark -h 127.0.0.1 -p 6379 -c 3 -n 60000 -P 16 -t set,get -q
echo "Redis-Lite:"
redis-benchmark -h 127.0.0.1 -p 6666 -c 3 -n 60000 -P 16 -t set,get -q

# 单连接极限测试
echo "4. 单连接极限性能"
echo "标准Redis:"
redis-benchmark -h 127.0.0.1 -p 6379 -c 1 -n 20000 -P 32 -t set,get -q
echo "Redis-Lite:"
redis-benchmark -h 127.0.0.1 -p 6666 -c 1 -n 20000 -P 32 -t set,get -q


