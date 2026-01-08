#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <time.h>
#include <iomanip>

#define BUFFER_SIZE 1024
#define TIME_PORT 9998
#define NTP_EPOCH 2208988800U

using namespace std;

// 获取格式化的时间字符串
string get_formatted_time() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return string(buffer);
}

string format_time(uint32_t timestamp) {
    time_t unix_time = (time_t)(timestamp - NTP_EPOCH);
    string time_str = ctime(&unix_time);
    // 移除换行符
    if (!time_str.empty() && time_str[time_str.length()-1] == '\n') {
        time_str.erase(time_str.length()-1);
    }
    return time_str;
}

void print_test_header(int test_num, const string& description) {
    cout << endl;
    cout << "🧪 测试 " << test_num << ": " << description << endl;
    cout << "────────────────────────────────────────" << endl;
}

int main(int argc, char* argv[]) {
    string server_ip = "127.0.0.1";  // 默认本地服务器
    
    if (argc > 1) {
        server_ip = argv[1];
    }
    
    cout << "=== TIME服务测试客户端 (实验四) ===" << endl;
    cout << "服务器: " << server_ip << ":" << TIME_PORT << endl;
    cout << "开始时间: " << get_formatted_time() << endl;
    cout << "=================================" << endl;
    
    // 测试1：基本连接测试
    print_test_header(1, "基本连接测试");
    
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        cerr << "❌ UDP socket创建失败" << endl;
        return -1;
    }
    
    // 设置超时
    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    // 设置服务器地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TIME_PORT);
    inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);
    
    // 发送TIME请求
    cout << "[" << get_formatted_time() << "] " << "📤 发送TIME请求..." << endl;
    int bytes_sent = sendto(udp_socket, "", 0, 0, 
                           (struct sockaddr*)&server_addr, sizeof(server_addr));
    
    if (bytes_sent < 0) {
        cerr << "[" << get_formatted_time() << "] " << "❌ 发送请求失败" << endl;
        close(udp_socket);
        return -1;
    }
    
    // 接收响应
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(server_addr);
    int bytes_received = recvfrom(udp_socket, buffer, sizeof(uint32_t), 0,
                                 (struct sockaddr*)&server_addr, &addr_len);
    
    if (bytes_received == sizeof(uint32_t)) {
        uint32_t network_time;
        memcpy(&network_time, buffer, sizeof(uint32_t));
        uint32_t time_value = ntohl(network_time);
        
        cout << "[" << get_formatted_time() << "] " << "✅ 收到TIME服务器响应!" << endl;
        cout << "[" << get_formatted_time() << "] " << "📊 原始时间戳: " << time_value << endl;
        cout << "[" << get_formatted_time() << "] " << "🕐 服务器时间: " << format_time(time_value) << endl;
        
        // 显示本地时间对比
        time_t local_time = time(nullptr);
        cout << "[" << get_formatted_time() << "] " << "💻 本地时间: " << ctime(&local_time);
    } else {
        cerr << "[" << get_formatted_time() << "] " << "❌ 无响应或响应无效" << endl;
        cerr << "[" << get_formatted_time() << "] " << "💡 请确保TIME服务器正在运行" << endl;
    }
    
    close(udp_socket);
    
    // 测试2：再次请求测试
    print_test_header(2, "再次请求测试");
    
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        cerr << "❌ UDP socket创建失败" << endl;
        return -1;
    }
    
    setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    cout << "[" << get_formatted_time() << "] " << "📤 发送第二次TIME请求..." << endl;
    bytes_sent = sendto(udp_socket, "", 0, 0, 
                       (struct sockaddr*)&server_addr, sizeof(server_addr));
    
    if (bytes_sent >= 0) {
        bytes_received = recvfrom(udp_socket, buffer, sizeof(uint32_t), 0,
                                 (struct sockaddr*)&server_addr, &addr_len);
        
        if (bytes_received == sizeof(uint32_t)) {
            uint32_t network_time;
            memcpy(&network_time, buffer, sizeof(uint32_t));
            uint32_t time_value = ntohl(network_time);
            
            cout << "[" << get_formatted_time() << "] " << "✅ 收到第二次响应!" << endl;
            cout << "[" << get_formatted_time() << "] " << "📊 时间戳: " << time_value << endl;
            cout << "[" << get_formatted_time() << "] " << "🕐 服务器时间: " << format_time(time_value) << endl;
        }
    }
    
    close(udp_socket);
    
    // 测试3：快速连续请求测试
    print_test_header(3, "快速连续请求测试");
    cout << "[" << get_formatted_time() << "] " << "启动3个并发客户端..." << endl;
    
    int success_count = 0;
    for (int i = 0; i < 3; i++) {
        int client_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if (client_socket >= 0) {
            setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
            
            if (sendto(client_socket, "", 0, 0, (struct sockaddr*)&server_addr, sizeof(server_addr)) >= 0) {
                if (recvfrom(client_socket, buffer, sizeof(uint32_t), 0, 
                           (struct sockaddr*)&server_addr, &addr_len) == sizeof(uint32_t)) {
                    success_count++;
                    cout << "[" << get_formatted_time() << "] " << "✅ 客户端 " << (i+1) << " 成功" << endl;
                }
            }
            close(client_socket);
        }
        // 微小延迟，避免请求完全同时
        usleep(100000); // 100ms
    }
    
    cout << "[" << get_formatted_time() << "] " << "📊 成功响应: " << success_count << "/3" << endl;
    
    cout << endl << "🎉 所有测试完成!" << endl;
    cout << "结束时间: " << get_formatted_time() << endl;
    
    return 0;
}