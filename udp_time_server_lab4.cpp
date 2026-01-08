#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <time.h>
#include <signal.h>
#include <syslog.h>
#include <mutex>
#include <iomanip>
#include <sstream>

#define BUFFER_SIZE 1024
#define TIME_PORT 9998
#define NTP_EPOCH 2208988800U

using namespace std;

// 全局变量，用于优雅退出
volatile sig_atomic_t keep_running = 1;
std::mutex output_mutex;

// 信号处理函数
void signal_handler(int signum) {
    keep_running = 0;
    std::lock_guard<std::mutex> lock(output_mutex);
    cout << "\n🛑 收到退出信号，正在关闭服务器..." << endl;
}

// 获取格式化的当前时间字符串
string get_formatted_time() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return string(buffer);
}

// 获取当前TIME协议时间戳
uint32_t get_current_time() {
    // 确保每次调用都获取最新的时间
    return (uint32_t)(time(nullptr) + NTP_EPOCH);
}

// 输出客户端信息
void log_client_connection(const struct sockaddr_in& client_addr, int request_id) {
    std::lock_guard<std::mutex> lock(output_mutex);
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_addr.sin_port);
    
    cout << "[" << get_formatted_time() << "] "
         << "📍 请求#" << request_id << " 来自 " << client_ip << ":" << client_port << endl;
}

// 记录服务器响应
void log_server_response(uint32_t time_value, int bytes_sent, int request_id) {
    std::lock_guard<std::mutex> lock(output_mutex);
    time_t unix_time = (time_t)(time_value - NTP_EPOCH);
    cout << "[" << get_formatted_time() << "] "
         << "   📤 请求#" << request_id << " 发送时间: " << time_value 
         << " (" << bytes_sent << " 字节)" << endl;
    cout << "[" << get_formatted_time() << "] "
         << "   🕐 对应时间: " << ctime(&unix_time);
}

int main() {
    int server_socket;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    
    cout << "=== 无连接TIME服务器 (实验四) ===" << endl;
    cout << "端口: " << TIME_PORT << endl;
    cout << "协议: UDP" << endl;
    cout << "启动时间: " << get_formatted_time() << endl;
    cout << "================================" << endl;
    
    // 注册信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 创建UDP socket
    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0) {
        cerr << "❌ 错误: UDP socket创建失败" << endl;
        return -1;
    }
    cout << "✅ UDP socket创建成功" << endl;
    
    // 设置socket选项，允许地址重用
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        cerr << "⚠️  警告: 设置地址重用失败" << endl;
    }
    
    // 设置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // 监听所有接口
    server_addr.sin_port = htons(TIME_PORT);
    
    // 绑定socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "❌ 错误: 绑定端口 " << TIME_PORT << " 失败" << endl;
        cerr << "💡 提示: 可能需要sudo权限或端口已被占用" << endl;
        close(server_socket);
        return -1;
    }
    cout << "✅ 绑定端口 " << TIME_PORT << " 成功" << endl;
    
    // 获取服务器IP信息
    char hostname[256];
    char server_ip[INET_ADDRSTRLEN];
    gethostname(hostname, sizeof(hostname));
    
    cout << "🏠 服务器主机名: " << hostname << endl;
    cout << "🌐 监听地址: 0.0.0.0:" << TIME_PORT << " (所有网络接口)" << endl;
    cout << "💡 本地测试: 127.0.0.1:" << TIME_PORT << endl;
    cout << "🚀 TIME服务器已启动，等待客户端请求..." << endl;
    cout << "⏹️  按 Ctrl+C 停止服务器" << endl;
    cout << "----------------------------------------" << endl;
    
    int request_count = 0;
    
    // 主服务循环
    while (keep_running) {
        socklen_t client_len = sizeof(client_addr);
        memset(buffer, 0, BUFFER_SIZE);
        
        // 接收客户端请求（阻塞等待）
        int bytes_received = recvfrom(server_socket, buffer, BUFFER_SIZE, 0,
                                     (struct sockaddr*)&client_addr, &client_len);
        
        if (bytes_received < 0) {
            if (keep_running) {
                cerr << "[" << get_formatted_time() << "] "
                     << "❌ 接收数据错误" << endl;
            }
            continue;
        }
        
        request_count++;
        
        // 记录客户端连接
        log_client_connection(client_addr, request_count);
        
        {
            std::lock_guard<std::mutex> lock(output_mutex);
            cout << "[" << get_formatted_time() << "] "
                 << "   📨 收到TIME请求 #" << request_count << " (" << bytes_received << " 字节)" << endl;
        }
        
        // 获取当前时间
        uint32_t current_time = get_current_time();
        uint32_t network_time = htonl(current_time);
        
        // 发送时间响应
        int bytes_sent = sendto(server_socket, &network_time, sizeof(network_time), 0,
                               (struct sockaddr*)&client_addr, client_len);
        
        if (bytes_sent == sizeof(network_time)) {
            log_server_response(current_time, bytes_sent, request_count);
        } else {
            std::lock_guard<std::mutex> lock(output_mutex);
            cerr << "[" << get_formatted_time() << "] "
                 << "   ❌ 请求#" << request_count << " 发送响应失败" << endl;
        }
        
        {
            std::lock_guard<std::mutex> lock(output_mutex);
            cout << "[" << get_formatted_time() << "] "
                 << "   ────────────────────────" << endl;
        }
    }
    
    // 清理资源
    close(server_socket);
    cout << "[" << get_formatted_time() << "] " << "✅ 服务器socket已关闭" << endl;
    cout << "[" << get_formatted_time() << "] " << "📊 总计处理请求: " << request_count << " 个" << endl;
    cout << "[" << get_formatted_time() << "] " << "🎉 TIME服务器正常退出" << endl;
    
    return 0;
}