#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>

#define BUFFER_SIZE 1024
#define ECHO_PORT 9999

using namespace std;

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    string server_ip = "127.0.0.1";
    
    cout << "=== ECHO服务TCP客户端 (端口: " << ECHO_PORT << ") ===" << endl;
    
    // 创建TCP socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        cerr << "错误: socket创建失败" << endl;
        return -1;
    }
    cout << "✓ Socket创建成功" << endl;
    
    // 设置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(ECHO_PORT);
    
    if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
        cerr << "错误: 无效的服务器地址" << endl;
        close(client_socket);
        return -1;
    }
    
    // 连接服务器
    cout << "正在连接ECHO服务器 " << server_ip << ":" << ECHO_PORT << "..." << endl;
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "错误: 连接服务器失败" << endl;
        cerr << "请确保运行: ncat -v -l 9999 -k --exec \"/bin/cat\"" << endl;
        close(client_socket);
        return -1;
    }
    cout << "✓ 连接服务器成功!" << endl;
    
    // 获取用户输入
    string message;
    cout << "请输入要发送的消息: ";
    getline(cin, message);
    
    // 发送数据
    int bytes_sent = send(client_socket, message.c_str(), message.length(), 0);
    if (bytes_sent < 0) {
        cerr << "错误: 发送数据失败" << endl;
        close(client_socket);
        return -1;
    }
    cout << "✓ 已发送: " << message << " (长度: " << bytes_sent << " 字节)" << endl;
    
    // 接收回显数据
    memset(buffer, 0, BUFFER_SIZE);
    int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        cout << "✓ 收到回显: " << buffer << " (长度: " << bytes_received << " 字节)" << endl;
        
        // 验证功能
        if (message == string(buffer)) {
            cout << "🎉 测试成功! ECHO服务功能正常" << endl;
        } else {
            cout << "❌ 测试失败! 回显内容不一致" << endl;
            cout << "原始: \"" << message << "\"" << endl;
            cout << "回显: \"" << buffer << "\"" << endl;
        }
    } else if (bytes_received == 0) {
        cout << "服务器关闭了连接" << endl;
    } else {
        cerr << "错误: 接收数据失败" << endl;
    }
    
    // 关闭连接
    close(client_socket);
    cout << "连接已关闭" << endl;
    
    return 0;
}