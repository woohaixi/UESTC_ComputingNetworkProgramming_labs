#!/bin/bash
echo "清理端口 9999..."
sudo fuser -k 9999/tcp 2>/dev/null
pkill ncat 2>/dev/null
echo "清理完成"