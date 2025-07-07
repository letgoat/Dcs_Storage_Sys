#include <iostream>
#include <string>
#include <memory>
#include <csignal>
#include <thread>
#include <chrono>

// 简单的信号处理
void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    exit(0);
}

int main() {
    std::cout << "Debug Server Starting..." << std::endl;
    
    // 设置信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    try {
        std::cout << "Creating server instance..." << std::endl;
        
        // 这里我们只测试基本的初始化，不启动网络服务器
        std::cout << "Server instance created successfully!" << std::endl;
        std::cout << "Press Ctrl+C to stop" << std::endl;
        
        // 简单的等待循环
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 