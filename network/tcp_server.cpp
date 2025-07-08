#include "tcp_server.h"
#include <iostream>
#include <algorithm>

ClientConnection::ClientConnection(SocketType socket, const std::string& client_addr)
    : socket_(socket), client_address_(client_addr) {
}

ClientConnection::~ClientConnection() {
    close();
}

bool ClientConnection::send(const std::string& data) {
    std::lock_guard<std::mutex> lock(send_mutex_);
    
    if (socket_ == INVALID_SOCKET_VALUE) {
        return false;
    }
    
    size_t total_sent = 0;
    //TCP的send不能保证一次发完所有数据，可能由于缓冲区慢，网络状况等等
    while (total_sent < data.length()) {
        int sent = ::send(socket_, data.c_str() + total_sent, 
                         static_cast<int>(data.length() - total_sent), 0);
        if (sent <= 0) {
            return false;
        }
        total_sent += sent;
    }
    
    return true;
}

std::string ClientConnection::receive() {
    char buffer[4096];
    std::string data;
    
    while (true) {
        int received = ::recv(socket_, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) {
            break;
        }
        
        buffer[received] = '\0';
        data += buffer;
        
        // 检查是否接收到完整的RESP消息
        if (data.find("\r\n") != std::string::npos) {
            break;
        }
    }
    
    return data;
}

void ClientConnection::close() {
    if (socket_ != INVALID_SOCKET_VALUE) {
        ::close(socket_);
        socket_ = INVALID_SOCKET_VALUE;
    }
}

TCPServer::TCPServer()
    : server_socket_(INVALID_SOCKET_VALUE)
    , running_(false)
    , network_initialized_(false) {
}

TCPServer::~TCPServer() {
    stop();
    cleanupNetwork();
}

bool TCPServer::init(const std::string& host, int port, int thread_pool_size) {
    if (!initNetwork()) {
        return false;
    }
    
    host_ = host;
    port_ = port;
    
    // 创建socket
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ == INVALID_SOCKET_VALUE) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }
    
    // 设置socket选项
    // 允许服务器socket端口被快速重用，避免重启服务器端口被占用的问题
    int opt = 1;
    if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, 
                   reinterpret_cast<char*>(&opt), sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options" << std::endl;
        return false;
    }
    
    // 绑定地址
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (host == "0.0.0.0") {
        server_addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        server_addr.sin_addr.s_addr = inet_addr(host.c_str());
    }
    
    if (bind(server_socket_, reinterpret_cast<struct sockaddr*>(&server_addr), 
             sizeof(server_addr)) < 0) {
        std::cerr << "Failed to bind socket" << std::endl;
        return false;
    }
    
    // 监听连接
    if (listen(server_socket_, SOMAXCONN) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        return false;
    }
    
    // 创建工作线程
    for (int i = 0; i < thread_pool_size; ++i) {
        worker_threads_.emplace_back(&TCPServer::workerLoop, this);//原地构建新的thread，所以直接传参即可
    }
    
    return true;
}

bool TCPServer::start() {
    if (server_socket_ == INVALID_SOCKET_VALUE) {
        return false;
    }
    
    running_ = true;
    accept_thread_ = std::thread(&TCPServer::acceptLoop, this);
    
    return true;
}

void TCPServer::stop() {
    running_ = false;
    
    // 关闭服务器socket
    if (server_socket_ != INVALID_SOCKET_VALUE) {
        CLOSE_SOCKET(server_socket_);
        server_socket_ = INVALID_SOCKET_VALUE;
    }
    
    // 等待接受线程结束
    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }
    
    // 通知所有工作线程
    queue_cv_.notify_all();
    
    // 等待工作线程结束
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    worker_threads_.clear();
    
    // 关闭所有客户端连接
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        for (auto& conn : connections_) {
            conn->close();
        }
        connections_.clear();
    }
}

void TCPServer::setMessageHandler(MessageHandler handler) {
    message_handler_ = handler;
}

size_t TCPServer::getConnectionCount() const {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    return connections_.size();
}

void TCPServer::acceptLoop() {
    while (running_) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        
        SocketType client_socket = accept(server_socket_, 
                                        reinterpret_cast<struct sockaddr*>(&client_addr), 
                                        &client_addr_len);
        
        if (client_socket == INVALID_SOCKET_VALUE) {
            if (running_) {
                std::cerr << "Failed to accept connection" << std::endl;
            }
            continue;
        }
        
        std::string client_address = inet_ntoa(client_addr.sin_addr) + 
                                   std::string(":") + std::to_string(ntohs(client_addr.sin_port));
        
        auto client = std::make_shared<ClientConnection>(client_socket, client_address);
        
        // 添加到连接列表
        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            connections_.push_back(client);
        }
        
        // 启动客户端处理线程
        std::thread(&TCPServer::handleClient, this, client).detach();
    }
}

void TCPServer::workerLoop() {
    while (running_) {
        Task task(nullptr, "");
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this] { return !task_queue_.empty() || !running_; });
            
            if (!running_) {
                break;
            }
            
            if (!task_queue_.empty()) {
                task = task_queue_.front();
                task_queue_.pop();
            }
        }
        
        if (task.client && message_handler_) { //是否有效的客户端连接，并且消息处理器
            std::string response = message_handler_(task.data, task.client);
            if (!response.empty()) {
                task.client->send(response);
            }
        }
    }
}

void TCPServer::handleClient(std::shared_ptr<ClientConnection> client) {
    while (running_ && client->isValid()) {
        std::string data = client->receive();
        if (data.empty()) {
            break;
        }
        
        if (message_handler_) {
            std::string response = message_handler_(data, client);
            if (!response.empty()) {
                client->send(response);
            }
        }
    }
    
    // 从连接列表中移除
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.erase(
            std::remove(connections_.begin(), connections_.end(), client),
            connections_.end()
        );
    }
    
    client->close();
}

bool TCPServer::setNonBlocking(SocketType socket) {
#ifdef _WIN32
    u_long mode = 1;
    return ioctlsocket(socket, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(socket, F_GETFL, 0);
    return fcntl(socket, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
}

bool TCPServer::initNetwork() {
#ifdef _WIN32
    if (!network_initialized_) {
        WSADATA wsa_data;
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
            std::cerr << "Failed to initialize Winsock" << std::endl;
            return false;
        }
        network_initialized_ = true;
    }
#endif
    return true;
}

void TCPServer::cleanupNetwork() {
#ifdef _WIN32
    if (network_initialized_) {
        WSACleanup();
        network_initialized_ = false;
    }
#endif
} 