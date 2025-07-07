#pragma once
#include <string>
#include <functional>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET SocketType;
#define INVALID_SOCKET_VALUE INVALID_SOCKET
#define CLOSE_SOCKET(s) closesocket(s)
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
typedef int SocketType;
#define INVALID_SOCKET_VALUE -1
#define CLOSE_SOCKET(s) close(s)
#endif

// 客户端连接类
class ClientConnection {
public:
    ClientConnection(SocketType socket, const std::string& client_addr);
    ~ClientConnection();
    
    // 获取客户端地址
    std::string getClientAddress() const { return client_address_; }
    
    // 获取socket
    SocketType getSocket() const { return socket_; }
    
    // 发送数据
    bool send(const std::string& data);
    
    // 接收数据
    std::string receive();
    
    // 关闭连接
    void close();
    
    // 检查连接是否有效
    bool isValid() const { return socket_ != INVALID_SOCKET_VALUE; }

private:
    SocketType socket_;
    std::string client_address_;
    std::mutex send_mutex_;
};

// 任务结构
struct Task {
    std::shared_ptr<ClientConnection> client;
    std::string data;
    
    Task(std::shared_ptr<ClientConnection> c, const std::string& d)
        : client(c), data(d) {}
};

// TCP服务器类
class TCPServer {
public:
    using MessageHandler = std::function<std::string(const std::string&, std::shared_ptr<ClientConnection>)>;
    
    TCPServer();
    ~TCPServer();
    
    // 初始化服务器
    bool init(const std::string& host, int port, int thread_pool_size = 4);
    
    // 启动服务器
    bool start();
    
    // 停止服务器
    void stop();
    
    // 设置消息处理器
    void setMessageHandler(MessageHandler handler);
    
    // 获取当前连接数
    size_t getConnectionCount() const;
    
    // 获取服务器状态
    bool isRunning() const { return running_; }

private:
    // 接受连接的线程函数
    void acceptLoop();
    
    // 工作线程函数
    void workerLoop();
    
    // 处理客户端连接
    void handleClient(std::shared_ptr<ClientConnection> client);
    
    // 设置socket为非阻塞模式
    bool setNonBlocking(SocketType socket);
    
    // 初始化网络库
    bool initNetwork();
    
    // 清理网络库
    void cleanupNetwork();
    
    std::string host_;
    int port_;
    SocketType server_socket_;
    std::atomic<bool> running_;
    
    // 线程池
    std::vector<std::thread> worker_threads_;
    std::thread accept_thread_;
    
    // 任务队列
    std::queue<Task> task_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    // 消息处理器
    MessageHandler message_handler_;
    
    // 连接管理
    std::vector<std::shared_ptr<ClientConnection>> connections_;
    mutable std::mutex connections_mutex_;
    
    // 网络库初始化标志
    bool network_initialized_;
}; 