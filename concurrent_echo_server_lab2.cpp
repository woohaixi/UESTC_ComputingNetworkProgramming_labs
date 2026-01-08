#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <vector>

#define BUFFER_SIZE 1024
#define SERVER_PORT 9999
#define MAX_CLIENTS 10

using namespace std;

// 客户端连接信息结构体
struct ClientInfo {
    int socket_fd;
    struct sockaddr_in address;
    int client_id;
};

// 全局变量：客户端计数
int client_count = 0;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

// 处理客户端连接的线程函数
void* handle_client(void* arg) {
    ClientInfo* client_info = (ClientInfo*)arg;
    int client_socket = client_info->socket_fd;
    char client_ip[INET_ADDRSTRLEN];
    
    // 获取客户端IP地址
    inet_ntop(AF_INET, &(client_info->address.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_info->address.sin_port);
    
    // 线程安全地增加客户端计数
    pthread_mutex_lock(&count_mutex);
    int current_client_id = ++client_count;
    pthread_mutex_unlock(&count_mutex);
    
    cout << "🎯 客户端 [" << current_client_id << "] 连接: " 
         << client_ip << ":" << client_port << endl;
    
    char buffer[BUFFER_SIZE];
    
    // 与客户端通信
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        
        // 接收客户端数据
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            cout << "📨 从客户端 [" << current_client_id << "] 收到: " 
                 << buffer << " (" << bytes_received << " 字节)" << endl;
            
            // 检查退出命令
            if (strcmp(buffer, "quit") == 0 || strcmp(buffer, "exit") == 0) {
                cout << "👋 客户端 [" << current_client_id << "] 请求断开连接" << endl;
                break;
            }
            
            // 回显数据给客户端
            int bytes_sent = send(client_socket, buffer, bytes_received, 0);
            if (bytes_sent > 0) {
                cout << "📤 向客户端 [" << current_client_id << "] 回显: " 
                     << buffer << " (" << bytes_sent << " 字节)" << endl;
            } else {
                cerr << "❌ 向客户端 [" << current_client_id << "] 发送失败" << endl;
                break;
            }
        } else if (bytes_received == 0) {
            cout << "🔌 客户端 [" << current_client_id << "] 断开连接" << endl;
            break;
        } else {
            cerr << "❌ 接收客户端 [" << current_client_id << "] 数据错误" << endl;
            break;
        }
    }
    
    // 关闭客户端socket
    close(client_socket);
    cout << "✅ 客户端 [" << current_client_id << "] 连接关闭" << endl;
    
    // 线程安全地减少客户端计数
    pthread_mutex_lock(&count_mutex);
    client_count--;
    cout << "📊 当前在线客户端: " << client_count << endl;
    pthread_mutex_unlock(&count_mutex);
    
    // 释放内存
    delete client_info;
    
    return nullptr;
}

int main() {
    int server_socket;
    struct sockaddr_in server_addr;
    
    cout << "=== 并发ECHO服务器 (实验二) ===" << endl;
    cout << "端口: " << SERVER_PORT << endl;
    cout << "最大客户端数: " << MAX_CLIENTS << endl;
    cout << "==============================" << endl;
    
    // 创建TCP socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        cerr << "❌ 错误: socket创建失败" << endl;
        return -1;
    }
    cout << "✅ Socket创建成功" << endl;
    
    // 设置socket选项，允许地址重用
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        cerr << "❌ 错误: 设置socket选项失败" << endl;
        close(server_socket);
        return -1;
    }
    
    // 设置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);
    
    // 绑定socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "❌ 错误: 绑定端口 " << SERVER_PORT << " 失败" << endl;
        close(server_socket);
        return -1;
    }
    cout << "✅ 绑定端口 " << SERVER_PORT << " 成功" << endl;
    
    // 开始监听
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        cerr << "❌ 错误: 监听失败" << endl;
        close(server_socket);
        return -1;
    }
    cout << "✅ 服务器开始监听，等待客户端连接..." << endl;
    cout << "💡 提示: 按 Ctrl+C 停止服务器" << endl;
    
    // 主循环：接受客户端连接
    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        
        // 接受客户端连接
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        
        if (client_socket < 0) {
            cerr << "❌ 错误: 接受客户端连接失败" << endl;
            continue;
        }
        
        // 创建客户端信息结构体
        ClientInfo* client_info = new ClientInfo();
        client_info->socket_fd = client_socket;
        client_info->address = client_addr;
        
        // 创建线程处理客户端
        pthread_t client_thread;
        if (pthread_create(&client_thread, nullptr, handle_client, (void*)client_info) != 0) {
            cerr << "❌ 错误: 创建客户端线程失败" << endl;
            close(client_socket);
            delete client_info;
            continue;
        }
        
        // 分离线程，使其结束后自动释放资源
        pthread_detach(client_thread);
        
        cout << "新的客户端连接已接受，创建处理线程" << endl;
    }
    
    // 关闭服务器socket（实际上不会执行到这里）
    close(server_socket);
    cout << "🛑 服务器关闭" << endl;
    
    return 0;
}