#include <iostream>
#include <string>
#include <memory>
#include <thread>
#include <chrono>
#include <csignal>
#include <fstream>
#include <map>

// 简化的配置类
class SimpleConfig {
public:
    std::string host = "0.0.0.0";
    int port = 6379;
    int max_connections = 1000;
    int thread_pool_size = 4;
    std::string log_level = "INFO";
    std::string log_file = "logs/skiplist.log";
    bool enable_console = true;
    
    bool loadFromFile(const std::string& filename) {
        std::cout << "Loading config from: " << filename << std::endl;
        
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file: " << filename << std::endl;
            return false;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                
                // 去除空格
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                
                if (key == "host") host = value;
                else if (key == "port") port = std::stoi(value);
                else if (key == "max_connections") max_connections = std::stoi(value);
                else if (key == "thread_pool_size") thread_pool_size = std::stoi(value);
                else if (key == "log_level") log_level = value;
                else if (key == "log_file") log_file = value;
                else if (key == "enable_console") enable_console = (value == "true");
            }
        }
        
        std::cout << "Config loaded successfully:" << std::endl;
        std::cout << "  Host: " << host << std::endl;
        std::cout << "  Port: " << port << std::endl;
        std::cout << "  Max Connections: " << max_connections << std::endl;
        std::cout << "  Thread Pool Size: " << thread_pool_size << std::endl;
        std::cout << "  Log Level: " << log_level << std::endl;
        std::cout << "  Log File: " << log_file << std::endl;
        std::cout << "  Enable Console: " << (enable_console ? "true" : "false") << std::endl;
        
        return true;
    }
};

// 简化的服务器类
class FixedServer {
public:
    FixedServer() : running_(false) {}
    
    bool init(const std::string& config_file) {
        std::cout << "Initializing server..." << std::endl;
        
        try {
            // 加载配置
            if (!config_file.empty()) {
                if (!config_.loadFromFile(config_file)) {
                    std::cerr << "Failed to load configuration" << std::endl;
                    return false;
                }
            }
            
            // 创建必要的目录
            std::cout << "Creating directories..." << std::endl;
            system("mkdir -p logs store");
            
            // 初始化日志
            std::cout << "Initializing logging..." << std::endl;
            initLogging();
            
            std::cout << "Server initialized successfully!" << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Error initializing server: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool start() {
        if (running_) {
            std::cout << "Server is already running" << std::endl;
            return true;
        }
        
        std::cout << "Starting server on " << config_.host << ":" << config_.port << std::endl;
        
        try {
            running_ = true;
            
            // 启动监控线程
            monitor_thread_ = std::thread(&FixedServer::monitorLoop, this);
            
            std::cout << "Server started successfully!" << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Error starting server: " << e.what() << std::endl;
            return false;
        }
    }
    
    void stop() {
        if (!running_) return;
        
        std::cout << "Stopping server..." << std::endl;
        running_ = false;
        
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
        
        std::cout << "Server stopped." << std::endl;
    }
    
    bool isRunning() const { return running_; }
    
private:
    SimpleConfig config_;
    bool running_;
    std::thread monitor_thread_;
    
    void initLogging() {
        std::cout << "Log level: " << config_.log_level << std::endl;
        std::cout << "Log file: " << config_.log_file << std::endl;
        
        // 创建日志文件
        std::ofstream log_file(config_.log_file, std::ios::app);
        if (log_file.is_open()) {
            log_file << "Server started at " << getCurrentTime() << std::endl;
            log_file.close();
        }
    }
    
    void monitorLoop() {
        while (running_) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            if (running_) {
                std::cout << "Server is running... (Press Ctrl+C to stop)" << std::endl;
            }
        }
    }
    
    std::string getCurrentTime() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        return std::ctime(&time_t);
    }
};

// 信号处理
void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    exit(0);
}

int main(int argc, char* argv[]) {
    std::cout << "Fixed SkipList Redis Server Starting..." << std::endl;
    
    // 设置信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    std::string config_file;
    
    // 解析命令行参数
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--config" || arg == "-c") {
            if (i + 1 < argc) {
                config_file = argv[++i];
            }
        } else if (arg == "--port" || arg == "-p") {
            if (i + 1 < argc) {
                // 这里可以设置端口，但为了简化，我们使用配置文件
                std::cout << "Port will be set from config file" << std::endl;
                ++i;
            }
        }
    }
    
    try {
        // 创建服务器实例
        auto server = std::make_unique<FixedServer>();
        
        // 初始化服务器
        if (!server->init(config_file)) {
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
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 