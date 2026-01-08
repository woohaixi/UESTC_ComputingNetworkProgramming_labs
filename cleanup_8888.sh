#!/bin/bash
echo "🚀 快速清理聊天室服务端口..."
sudo fuser -k 8888/tcp 2>/dev/null
sudo fuser -k 8888/udp 2>/dev/null
pkill -f "udp_time_server" 2>/dev/null
echo "✅ 快速清理完成"