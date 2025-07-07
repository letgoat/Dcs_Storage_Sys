#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <csignal>
#include "../server/skiplist_server.h"
#include "../logger/logger.h"
#include "../config/config.h"

// 全局服务器实例
static SkipListServer* g_server = nullptr;

// 信号处理函数
void signalHandler(int signal) {
    if (g_server) {
        std::cout << "\nReceived signal " << signal << ", shutting down gracefully..." << std::endl;
        g_server->stop();
    }
}

// 显示帮助信息
void showHelp() {
    std::cout << "SkipList Redis Server v1.0.0\n";
    std::cout << "Usage: ./SkipListProject [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -c, --config <file>     Configuration file path\n";
    std::cout << "  -p, --port <port>       Server port (default: 6379)\n";
    std::cout << "  -h, --host <host>       Server host (default: 0.0.0.0)\n";
    std::cout << "  -l, --log-level <level> Log level (DEBUG|INFO|WARN|ERROR|FATAL)\n";
    std::cout << "  -d, --daemon            Run as daemon\n";
    std::cout << "  -v, --version           Show version information\n";
    std::cout << "  --help                  Show this help message\n\n";
    std::cout << "Examples:\n";
    std::cout << "  ./SkipListProject                    # Start with default settings\n";
    std::cout << "  ./SkipListProject -p 6380           # Start on port 6380\n";
    std::cout << "  ./SkipListProject -c config.conf    # Start with config file\n";
    std::cout << "  ./SkipListProject -l DEBUG          # Start with debug logging\n\n";
    std::cout << "Redis Commands Supported:\n";
    std::cout << "  PING, ECHO, SET, GET, DEL, EXISTS, KEYS, FLUSH\n";
    std::cout << "  SAVE, LOAD, INFO, CONFIG, SELECT, AUTH, QUIT\n\n";
    std::cout << "Configuration:\n";
    std::cout << "  Server can be configured via:\n";
    std::cout << "  1. Command line arguments\n";
    std::cout << "  2. Configuration file\n";
    std::cout << "  3. Environment variables\n\n";
    std::cout << "Environment Variables:\n";
    std::cout << "  SKIPLIST_PORT=6379\n";
    std::cout << "  SKIPLIST_HOST=0.0.0.0\n";
    std::cout << "  SKIPLIST_MAX_CONNECTIONS=1000\n";
    std::cout << "  SKIPLIST_THREAD_POOL_SIZE=4\n";
    std::cout << "  SKIPLIST_MAX_LEVEL=18\n";
    std::cout << "  SKIPLIST_LOG_LEVEL=INFO\n";
    std::cout << "  SKIPLIST_LOG_FILE=logs/skiplist.log\n";
}

// 显示版本信息
void showVersion() {
    std::cout << "SkipList Redis Server v1.0.0\n";
    std::cout << "Built with C++17\n";
    std::cout << "Features:\n";
    std::cout << "  - SkipList data structure implementation\n";
    std::cout << "  - Redis protocol compatibility (RESP)\n";
    std::cout << "  - Multi-threaded network server\n";
    std::cout << "  - Configurable logging system\n";
    std::cout << "  - Data persistence\n";
    std::cout << "  - Performance monitoring\n";
    std::cout << "  - Graceful shutdown\n";
}

// 解析命令行参数
bool parseArguments(int argc, char* argv[], std::string& config_file) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            showHelp();
            return false;
        } else if (arg == "--version" || arg == "-v") {
            showVersion();
            return false;
        } else if (arg == "--config" || arg == "-c") {
            if (i + 1 < argc) {
                config_file = argv[++i];
            } else {
                std::cerr << "Error: Missing config file path" << std::endl;
                return false;
            }
        } else if (arg == "--port" || arg == "-p") {
            if (i + 1 < argc) {
                int port = std::stoi(argv[++i]);
                Config::getInstance().setInt("port", port);
            } else {
                std::cerr << "Error: Missing port number" << std::endl;
                return false;
            }
        } else if (arg == "--host" || arg == "-h") {
            if (i + 1 < argc) {
                std::string host = argv[++i];
                Config::getInstance().setString("host", host);
            } else {
                std::cerr << "Error: Missing host address" << std::endl;
                return false;
            }
        } else if (arg == "--log-level" || arg == "-l") {
            if (i + 1 < argc) {
                std::string level = argv[++i];
                Config::getInstance().setString("log_level", level);
            } else {
                std::cerr << "Error: Missing log level" << std::endl;
                return false;
            }
        } else if (arg == "--daemon" || arg == "-d") {
            // TODO: Implement daemon mode
            std::cout << "Daemon mode not implemented yet" << std::endl;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            std::cerr << "Use --help for usage information" << std::endl;
            return false;
        }
    }
    
    return true;
}

// 性能测试函数
void runPerformanceTest() {
    std::cout << "\n=== Performance Test ===\n";
    
    auto& server = *g_server;
    auto& skiplist = server.getRedisHandler().getSkipList();
    
    const int test_size = 10000;
    std::cout << "Testing with " << test_size << " elements...\n";
    
    // 插入测试
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < test_size; ++i) {
        skiplist.insert_element(i, "value_" + std::to_string(i));
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto insert_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Insert " << test_size << " elements: " << insert_time.count() << "ms\n";
    
    // 查找测试
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < test_size; ++i) {
        skiplist.search_element(i);
    }
    end = std::chrono::high_resolution_clock::now();
    auto search_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Search " << test_size << " elements: " << search_time.count() << "ms\n";
    
    // 删除测试
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < test_size; ++i) {
        skiplist.delete_element(i);
    }
    end = std::chrono::high_resolution_clock::now();
    auto delete_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Delete " << test_size << " elements: " << delete_time.count() << "ms\n";
    
    std::cout << "Performance test completed.\n\n";
}

int main(int argc, char* argv[]) {
    std::cout << "SkipList Redis Server Starting...\n";
    
    // 解析命令行参数
    std::string config_file;
    if (!parseArguments(argc, argv, config_file)) {
        return 0;
    }
    
    try {
        // 创建服务器实例
        g_server = new SkipListServer();
        
        // 初始化服务器
        if (!g_server->init(config_file)) {
            std::cerr << "Failed to initialize server" << std::endl;
            delete g_server;
            return 1;
        }
        
        // 设置信号处理
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);
        
        // 显示启动信息
        const auto& config = g_server->getConfig();
        const auto& server_config = config.getServerConfig();
        const auto& log_config = config.getLogConfig();
        
        std::cout << "Server Configuration:\n";
        std::cout << "  Host: " << server_config.host << "\n";
        std::cout << "  Port: " << server_config.port << "\n";
        std::cout << "  Max Connections: " << server_config.max_connections << "\n";
        std::cout << "  Thread Pool Size: " << server_config.thread_pool_size << "\n";
        std::cout << "  Log Level: " << log_config.log_level << "\n";
        std::cout << "  Log File: " << log_config.log_file << "\n";
        std::cout << "  Data File: " << config.getSkipListConfig().data_file << "\n\n";
        
        // 运行性能测试（可选）
        if (argc > 1 && std::string(argv[1]) == "--test") {
            runPerformanceTest();
        }
        
        // 启动服务器
        if (!g_server->start()) {
            std::cerr << "Failed to start server" << std::endl;
            delete g_server;
            return 1;
        }
        
        std::cout << "Server started successfully!\n";
        std::cout << "Use Ctrl+C to stop the server\n\n";
        
        // 主循环
        while (g_server->isRunning()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // 显示统计信息（每60秒）
            static int counter = 0;
            if (++counter >= 60) {
                counter = 0;
                auto stats = g_server->getStats();
                std::cout << "Server Stats - Uptime: " << stats.uptime_seconds 
                         << "s, Connections: " << stats.current_connections 
                         << ", Commands: " << stats.total_commands << "\n";
            }
        }
        
        std::cout << "Server stopped.\n";
        
        // 清理
        delete g_server;
        g_server = nullptr;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        if (g_server) {
            delete g_server;
        }
        return 1;
    }
    
    return 0;
}