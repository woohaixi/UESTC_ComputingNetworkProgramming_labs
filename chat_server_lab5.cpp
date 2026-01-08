#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define CHAT_PORT 8888
#define MAX_CLIENTS 20

using namespace std;

// 客户端信息结构体
struct ClientInfo {
    int socket_fd;
    string username;
    struct sockaddr_in address;
};

// 全局变量
map<int, ClientInfo> clients;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// 向所有客户端广播消息（除了发送者）
void broadcast_message(const string& message, int exclude_fd = -1) {
    pthread_mutex_lock(&clients_mutex);
    
    for (const auto& client : clients) {
        if (client.first != exclude_fd) {
            send(client.first, message.c_str(), message.length(), 0);
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

// 处理客户端连接的线程函数
void* handle_client(void* arg) {
    ClientInfo* client_info = (ClientInfo*)arg;
    int client_socket = client_info->socket_fd;
    char client_ip[INET_ADDRSTRLEN];
    
    inet_ntop(AF_INET, &(client_info->address.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_info->address.sin_port);
    
    char buffer[BUFFER_SIZE];
    
    // 获取用户名
    memset(buffer, 0, BUFFER_SIZE);
    int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        client_info->username = string(buffer);
        
        cout << "🟢 用户 '" << client_info->username << "' 加入聊天室 (" 
             << client_ip << ":" << client_port << ")" << endl;
        
        // 广播用户加入消息
        string join_msg = "📢 系统: 用户 '" + client_info->username + "' 加入了聊天室";
        broadcast_message(join_msg, client_socket);
        
        // 发送欢迎消息
        string welcome_msg = "👋 欢迎来到聊天室, " + client_info->username + "! 输入 'quit' 退出";
        send(client_socket, welcome_msg.c_str(), welcome_msg.length(), 0);
    } else {
        close(client_socket);
        delete client_info;
        return nullptr;
    }
    
    // 消息处理循环
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            string message(buffer);
            
            // 检查退出命令
            if (message == "quit" || message == "exit") {
                break;
            }
            
            // 构建聊天消息
            string chat_msg = "💬 " + client_info->username + ": " + message;
            cout << chat_msg << endl;
            
            // 广播消息给所有其他客户端
            broadcast_message(chat_msg, client_socket);
            
        } else if (bytes_received == 0) {
            cout << "🔌 用户 '" << client_info->username << "' 断开连接" << endl;
            break;
        } else {
            cerr << "❌ 接收用户 '" << client_info->username << "' 数据错误" << endl;
            break;
        }
    }
    
    // 用户退出处理
    string leave_msg = "📢 系统: 用户 '" + client_info->username + "' 离开了聊天室";
    cout << leave_msg << endl;
    broadcast_message(leave_msg, client_socket);
    
    // 从客户端列表中移除
    pthread_mutex_lock(&clients_mutex);
    clients.erase(client_socket);
    pthread_mutex_unlock(&clients_mutex);
    
    // 关闭连接
    close(client_socket);
    delete client_info;
    
    return nullptr;
}

int main() {
    int server_socket;
    struct sockaddr_in server_addr;
    
    cout << "╔═════════════════════════════════════════════════════╗" << endl;
    cout << "║                  TCP聊天服务器 (实验五)            ║" << endl;
    cout << "║                    端口: " << CHAT_PORT << "                      ║" << endl;
    cout << "║                最大用户数: " << MAX_CLIENTS << "                    ║" << endl;
    cout << "╚═════════════════════════════════════════════════════╝" << endl;
    
    // 创建TCP socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        cerr << "❌ 错误: socket创建失败" << endl;
        return -1;
    }
    cout << "✅ Socket创建成功" << endl;
    
    // 设置socket选项
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        cerr << "⚠️  警告: 设置socket选项失败" << endl;
    }
    
    // 设置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(CHAT_PORT);
    
    // 绑定socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "❌ 错误: 绑定端口 " << CHAT_PORT << " 失败" << endl;
        close(server_socket);
        return -1;
    }
    cout << "✅ 绑定端口 " << CHAT_PORT << " 成功" << endl;
    
    // 开始监听
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        cerr << "❌ 错误: 监听失败" << endl;
        close(server_socket);
        return -1;
    }
    cout << "✅ 服务器开始监听，等待用户连接..." << endl;
    cout << "💡 提示: 按 Ctrl+C 停止服务器" << endl;
    cout << "═══════════════════════════════════════════════════════" << endl;
    
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
        
        // 检查客户端数量
        pthread_mutex_lock(&clients_mutex);
        if (clients.size() >= MAX_CLIENTS) {
            string reject_msg = "❌ 服务器已满，请稍后重试";
            send(client_socket, reject_msg.c_str(), reject_msg.length(), 0);
            close(client_socket);
            pthread_mutex_unlock(&clients_mutex);
            continue;
        }
        pthread_mutex_unlock(&clients_mutex);
        
        // 创建客户端信息
        ClientInfo* client_info = new ClientInfo();
        client_info->socket_fd = client_socket;
        client_info->address = client_addr;
        
        // 添加到客户端列表
        pthread_mutex_lock(&clients_mutex);
        clients[client_socket] = *client_info;
        pthread_mutex_unlock(&clients_mutex);
        
        // 创建线程处理客户端
        pthread_t client_thread;
        if (pthread_create(&client_thread, nullptr, handle_client, (void*)client_info) != 0) {
            cerr << "❌ 错误: 创建客户端线程失败" << endl;
            
            pthread_mutex_lock(&clients_mutex);
            clients.erase(client_socket);
            pthread_mutex_unlock(&clients_mutex);
            
            close(client_socket);
            delete client_info;
            continue;
        }
        
        // 分离线程
        pthread_detach(client_thread);
    }
    
    close(server_socket);
    return 0;
}