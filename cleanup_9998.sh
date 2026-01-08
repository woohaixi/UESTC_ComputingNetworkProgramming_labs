#!/bin/bash
echo "🚀 快速清理TIME服务端口..."
sudo fuser -k 9998/tcp 2>/dev/null
sudo fuser -k 9998/udp 2>/dev/null
pkill -f "udp_time_server" 2>/dev/null
echo "✅ 快速清理完成"