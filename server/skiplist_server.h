#pragma once
#include <memory>
#include <thread>
#include <atomic>
#include "../network/tcp_server.h"
#include "../server/redis_handler.h"
#include "../config/config.h"
#include "../logger/logger.h"

class SkipListServer {
public:
    SkipListServer();
    ~SkipListServer();
    
    // 初始化服务器
    bool init(const std::string& config_file = "");
    
    // 启动服务器
    bool start();
    
    // 停止服务器
    void stop();
    
    // 获取服务器状态
    bool isRunning() const { return running_; }
    
    // 获取配置
    const Config& getConfig() const { return config_; }
    
    // 获取Redis处理器
    RedisHandler& getRedisHandler() { return redis_handler_; }
    
    // 获取统计信息
    struct ServerStats {
        size_t total_connections = 0;
        size_t current_connections = 0;
        size_t total_commands = 0;
        double uptime_seconds = 0.0;
        std::string version = "1.0.0";
    };
    
    ServerStats getStats() const;

private:
    // 消息处理函数
    std::string handleMessage(const std::string& message, std::shared_ptr<ClientConnection> client);
    
    // 持久化线程函数
    void persistenceLoop();
    
    // 监控线程函数
    void monitorLoop();
    
    // 信号处理
    void setupSignalHandlers();
    
    // 优雅关闭
    void gracefulShutdown();
    
    // 加载配置
    bool loadConfiguration(const std::string& config_file);
    
    // 初始化日志系统
    void initLogging();
    
    // 初始化网络服务器
    bool initNetworkServer();
    
    // 创建必要的目录
    void createDirectories();
    
    Config& config_;
    std::unique_ptr<TCPServer> tcp_server_;
    RedisHandler redis_handler_;
    
    std::atomic<bool> running_;
    std::thread persistence_thread_;
    std::thread monitor_thread_;
    
    // 统计信息
    mutable std::mutex stats_mutex_;
    ServerStats server_stats_;
    std::chrono::steady_clock::time_point start_time_;
    
    // 持久化相关
    std::atomic<bool> persistence_enabled_;
    int persistence_interval_;
}; 