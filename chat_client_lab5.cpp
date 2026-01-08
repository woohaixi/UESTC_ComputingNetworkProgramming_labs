#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define CHAT_PORT 8888

using namespace std;

// 全局变量
volatile bool keep_running = true;
int client_socket;

// 接收消息的线程函数
void* receive_messages(void* arg) {
    char buffer[BUFFER_SIZE];
    
    while (keep_running) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            cout << "\r" << buffer << endl;
            cout << "你: ";
            cout.flush();
        } else if (bytes_received == 0) {
            cout << "\r🔌 与服务器断开连接" << endl;
            keep_running = false;
            break;
        } else {
            // 接收错误，继续尝试
        }
    }
    
    return nullptr;
}

int main(int argc, char* argv[]) {
    string server_ip = "127.0.0.1";
    string username;
    
    if (argc > 1) {
        server_ip = argv[1];
    }
    
    cout << "╔═════════════════════════════════════════════════════╗" << endl;
    cout << "║                  TCP聊天客户端 (实验五)            ║" << endl;
    cout << "║                服务器: " << server_ip << ":" << CHAT_PORT << "               ║" << endl;
    cout << "╚═════════════════════════════════════════════════════╝" << endl;
    
    // 获取用户名
    cout << "请输入你的用户名: ";
    getline(cin, username);
    
    if (username.empty()) {
        username = "匿名用户";
    }
    
    // 创建TCP socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        cerr << "❌ 错误: socket创建失败" << endl;
        return -1;
    }
    
    // 设置服务器地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(CHAT_PORT);
    inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);
    
    // 连接服务器
    cout << "🔗 连接服务器中..." << endl;
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "❌ 错误: 连接服务器失败" << endl;
        close(client_socket);
        return -1;
    }
    
    // 发送用户名
    send(client_socket, username.c_str(), username.length(), 0);
    
    // 创建接收消息线程
    pthread_t recv_thread;
    if (pthread_create(&recv_thread, nullptr, receive_messages, nullptr) != 0) {
        cerr << "❌ 错误: 创建接收线程失败" << endl;
        close(client_socket);
        return -1;
    }
    
    cout << "✅ 连接成功！开始聊天吧～" << endl;
    cout << "💡 提示: 输入 'quit' 退出聊天室" << endl;
    cout << "═══════════════════════════════════════════════════════" << endl;
    
    // 消息发送循环
    string message;
    while (keep_running) {
        cout << "你: ";
        getline(cin, message);
        
        if (!keep_running) break;
        
        if (message == "quit" || message == "exit") {
            keep_running = false;
            break;
        }
        
        if (!message.empty()) {
            send(client_socket, message.c_str(), message.length(), 0);
        }
    }
    
    // 清理资源
    keep_running = false;
    close(client_socket);
    pthread_cancel(recv_thread);
    
    cout << "👋 再见！" << endl;
    return 0;
}