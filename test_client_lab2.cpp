#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>

#define BUFFER_SIZE 1024
#define SERVER_PORT 9999

using namespace std;

void test_echo_client(const string& client_name) {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    
    cout << "[" << client_name << "] 启动..." << endl;
    
    // 创建TCP socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        cerr << "[" << client_name << "] socket创建失败" << endl;
        return;
    }
    
    // 设置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    
    // 连接服务器
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "[" << client_name << "] 连接服务器失败" << endl;
        close(client_socket);
        return;
    }
    cout << "[" << client_name << "] 连接服务器成功!" << endl;
    
    // 发送测试消息
    string messages[] = {
        client_name + ": Hello Server!",
        client_name + ": 测试并发",
        client_name + ": 第二条消息",
        "quit"
    };
    
    for (const auto& message : messages) {
        // 发送数据
        send(client_socket, message.c_str(), message.length(), 0);
        cout << "[" << client_name << "] 发送: " << message << endl;
        
        if (message == "quit") break;
        
        // 接收回显
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            cout << "[" << client_name << "] 收到: " << buffer << endl;
        }
        
        sleep(1); // 等待1秒
    }
    
    close(client_socket);
    cout << "[" << client_name << "] 连接关闭" << endl;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "用法: " << argv[0] << " <客户端名称>" << endl;
        return 1;
    }
    
    test_echo_client(argv[1]);
    return 0;
}