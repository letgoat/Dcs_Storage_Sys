#include <iostream>
#include <string>
#include <memory>
#include <thread>
#include <chrono>
#include <csignal>

// 简单的TCP服务器模拟
class SimpleServer {
public:
    SimpleServer() : running_(false) {}
    
    bool init(const std::string& host, int port) {
        std::cout << "Initializing server on " << host << ":" << port << std::endl;
        host_ = host;
        port_ = port;
        return true;
    }
    
    bool start() {
        std::cout << "Starting server..." << std::endl;
        running_ = true;
        std::cout << "Server started successfully on " << host_ << ":" << port_ << std::endl;
        return true;
    }
    
    void stop() {
        std::cout << "Stopping server..." << std::endl;
        running_ = false;
    }
    
    bool isRunning() const { return running_; }
    
private:
    std::string host_;
    int port_;
    bool running_;
};

// 信号处理
void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    exit(0);
}

int main() {
    std::cout << "Simple Server Test Starting..." << std::endl;
    
    // 设置信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    try {
        // 创建服务器实例
        auto server = std::make_unique<SimpleServer>();
        
        // 初始化服务器
        if (!server->init("0.0.0.0", 6379)) {
            std::cerr << "Failed to initialize server" << std::endl;
            return 1;
        }
        
        // 启动服务器
        if (!server->start()) {
            std::cerr << "Failed to start server" << std::endl;
            return 1;
        }
        
        std::cout << "Server is running. Press Ctrl+C to stop." << std::endl;
        
        // 主循环
        while (server->isRunning()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        std::cout << "Server stopped." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 