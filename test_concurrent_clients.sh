#!/bin/bash
echo "=== 测试真正并发连接 ==="

# 同时启动多个客户端
./test_client "客户端1" &
./test_client "客户端2" &
./test_client "客户端3" &
./test_client "客户端4" &

# 等待所有客户端完成
wait
echo "所有客户端测试完成"