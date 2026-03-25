#!/usr/bin/env bash
set -eu

HOST="127.0.0.1"
PORT="6666"

# Choose nc option to close the connection on stdin EOF.
# OpenBSD netcat supports -N; traditional netcat supports -q 0.
if nc -h 2>&1 | grep -q -- "-N"; then
  NC_OPTS='-N'
else
  NC_OPTS='-q 0'
fi

echo "== 半包测试 =="
{
  printf '*3\r\n$3\r\nSET\r\n$4\r\nte'
  sleep 1
  printf 'xt\r\n$5\r\nhello\r\n'
  sleep 1
  printf '*2\r\n$3\r\nGET\r\n$4\r\ntext\r\n'
  sleep 1
} | nc $NC_OPTS "$HOST" "$PORT"

echo
echo "== 粘包测试 =="
{
  printf '*1\r\n$4\r\nPING\r\n*2\r\n$3\r\nGET\r\n$4\r\ntext\r\n'
  sleep 1
} | nc $NC_OPTS "$HOST" "$PORT"