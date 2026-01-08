#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/types.h>
#include <netdb.h>

#define BUFFER_SIZE 1024
#define TIME_PORT 37
#define NTP_EPOCH 2208988800U

using namespace std;

// 可用的TIME服务器列表
const string TIME_SERVERS[] = {
    "time.google.com",      // Google时间服务器
    "time.windows.com",     // Microsoft时间服务器  
    "time.apple.com",       // Apple时间服务器
    "time.nist.gov",        // NIST时间服务器
    "pool.ntp.org",         // NTP池项目
    "ntp.aliyun.com",       // 阿里云NTP服务器
    "time1.tencent.com",    // 腾讯云时间服务器
    "time1.cloud.tencent.com",
    "cn.pool.ntp.org",      // 中国NTP池
    "ntp1.aliyun.com"
};

const int SERVER_COUNT = 10;

string format_time(uint32_t timestamp) {
    time_t unix_time = (time_t)(timestamp - NTP_EPOCH);
    return ctime(&unix_time);
}

int try_time_server(const string& server_host) {
    int udp_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    
    cout << "🔄 尝试服务器: " << server_host << " ..." << endl;
    
    // 创建UDP socket
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        cerr << "   ❌ Socket创建失败" << endl;
        return -1;
    }
    
    // 设置超时（2秒）
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    // 解析服务器地址
    struct hostent* server = gethostbyname(server_host.c_str());
    if (server == nullptr) {
        cerr << "   ❌ 无法解析主机名" << endl;
        close(udp_socket);
        return -1;
    }
    
    // 设置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TIME_PORT);
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    
    // 发送TIME请求
    int bytes_sent = sendto(udp_socket, "", 0, 0, 
                           (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (bytes_sent < 0) {
        cerr << "   ❌ 发送请求失败" << endl;
        close(udp_socket);
        return -1;
    }
    
    // 接收响应
    socklen_t server_len = sizeof(server_addr);
    memset(buffer, 0, BUFFER_SIZE);
    int bytes_received = recvfrom(udp_socket, buffer, sizeof(uint32_t), 0,
                                 (struct sockaddr*)&server_addr, &server_len);
    
    close(udp_socket);
    
    if (bytes_received == sizeof(uint32_t)) {
        uint32_t network_time;
        memcpy(&network_time, buffer, sizeof(uint32_t));
        uint32_t time_value = ntohl(network_time);
        
        cout << "   ✅ 成功获取时间!" << endl;
        cout << "   📊 原始时间戳: " << time_value << endl;
        cout << "   🕐 标准时间: " << format_time(time_value);
        
        // 显示本地时间对比
        time_t local_time = time(nullptr);
        cout << "   💻 本地时间: " << ctime(&local_time);
        return 0;
    } else {
        cerr << "   ❌ 无响应或响应无效" << endl;
        return -1;
    }
}

int main() {
    cout << "=== TIME服务UDP客户端 (实验三) ===" << endl;
    cout << "🔍 尝试连接 " << SERVER_COUNT << " 个时间服务器..." << endl;
    cout << "=======================================" << endl;
    
    bool success = false;
    
    for (int i = 0; i < SERVER_COUNT; i++) {
        cout << "\n尝试 " << (i + 1) << "/" << SERVER_COUNT << ":" << endl;
        if (try_time_server(TIME_SERVERS[i]) == 0) {
            success = true;
            break;
        }
    }
    
    if (!success) {
        cout << "\n❌ 所有服务器都不可用!" << endl;
        cout << "💡 建议解决方案:" << endl;
        cout << "1. 检查网络连接" << endl;
        cout << "2. 检查防火墙设置" << endl;
        cout << "3. 使用本地TIME服务器测试" << endl;
        cout << "4. 使用NTP端口(123)而不是TIME端口(37)" << endl;
    } else {
        cout << "\n🎉 TIME服务测试成功!" << endl;
    }
    
    return success ? 0 : 1;
}