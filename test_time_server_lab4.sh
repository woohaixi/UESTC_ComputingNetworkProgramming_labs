#!/bin/bash

echo "=== 实验四：无连接TIME服务器测试 ==="
echo "=================================="
echo "开始时间: $(date '+%Y-%m-%d %H:%M:%S')"

# 清理函数
cleanup() {
    echo "🛑 清理环境..."
    sudo pkill -f udp_time_server_lab4 2>/dev/null
    sleep 2
}

# 编译
echo ""
echo "1. 编译TIME服务器和客户端..."
g++ -std=c++11 -pthread -o udp_time_server_lab4 udp_time_server_lab4.cpp
g++ -std=c++11 -o test_time_client_lab4 test_time_client_lab4.cpp

if [ $? -eq 0 ]; then
    echo "✅ 编译成功"
else
    echo "❌ 编译失败"
    exit 1
fi

# 清理
cleanup

# 启动服务器
echo ""
echo "2. 启动TIME服务器（后台运行）..."
./udp_time_server_lab4 > server.log 2>&1 &
SERVER_PID=$!
sleep 3

if ps -p $SERVER_PID > /dev/null; then
    echo "✅ 服务器启动成功 (PID: $SERVER_PID)"
else
    echo "❌ 服务器启动失败"
    cat server.log
    exit 1
fi

# 测试
echo ""
echo "3. 测试TIME服务..."
echo "----------------------------------------"

# 使用修复后的客户端进行测试
./test_time_client_lab4 127.0.0.1

# 显示服务器日志
echo ""
echo "----------------------------------------"
echo "📋 服务器日志:"
echo "----------------------------------------"
cat server.log

# 清理
echo ""
echo "4. 清理..."
cleanup
rm -f server.log udp_time_server_lab4 test_time_client_lab4
echo "✅ 测试完成"
echo "结束时间: $(date '+%Y-%m-%d %H:%M:%S')"