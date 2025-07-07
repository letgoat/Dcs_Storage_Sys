#include "skiplist_server.h"
#include <iostream>
#include <chrono>
#include <filesystem>
#include <csignal>

// 全局服务器实例，用于信号处理
static SkipListServer* g_server = nullptr;

// 信号处理函数声明（在main.cpp中定义）
void signalHandler(int signal);

SkipListServer::SkipListServer()
    : config_(Config::getInstance())
    , running_(false)
    , persistence_enabled_(true)
    , persistence_interval_(60) {
    g_server = this;
}

SkipListServer::~SkipListServer() {
    stop();
    g_server = nullptr;
}

bool SkipListServer::init(const std::string& config_file) {
    try {
        // 加载配置
        if (!loadConfiguration(config_file)) {
            std::cerr << "Failed to load configuration" << std::endl;
            return false;
        }
        
        // 创建必要的目录
        createDirectories();
        
        // 初始化日志系统
        initLogging();
        
        // 初始化Redis处理器
        const auto& skiplist_config = config_.getSkipListConfig();
        redis_handler_.init(skiplist_config.max_level);
        
        // 初始化网络服务器
        if (!initNetworkServer()) {
            LOG_ERROR("Failed to initialize network server");
            return false;
        }
        
        // 设置信号处理
        setupSignalHandlers();
        
        // 初始化统计信息
        start_time_ = std::chrono::steady_clock::now();
        
        LOG_INFO("SkipList server initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error initializing server: " << e.what() << std::endl;
        return false;
    }
}

bool SkipListServer::start() {
    if (running_) {
        LOG_WARN("Server is already running");
        return true;
    }
    
    try {
        // 启动网络服务器
        if (!tcp_server_->start()) {
            LOG_ERROR("Failed to start TCP server");
            return false;
        }
        
        running_ = true;
        
        // 启动持久化线程
        if (persistence_enabled_) {
            persistence_thread_ = std::thread(&SkipListServer::persistenceLoop, this);
        }
        
        // 启动监控线程
        monitor_thread_ = std::thread(&SkipListServer::monitorLoop, this);
        
        const auto& server_config = config_.getServerConfig();
        LOG_INFOF("SkipList server started on {}:{}", server_config.host, server_config.port);
        
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error starting server: " + std::string(e.what()));
        return false;
    }
}

void SkipListServer::stop() {
    if (!running_) {
        return;
    }
    
    LOG_INFO("Stopping SkipList server...");
    running_ = false;
    
    // 停止网络服务器
    if (tcp_server_) {
        tcp_server_->stop();
    }
    
    // 等待持久化线程结束
    if (persistence_thread_.joinable()) {
        persistence_thread_.join();
    }
    
    // 等待监控线程结束
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
    
    // 保存数据
    redis_handler_.saveData();
    
    LOG_INFO("SkipList server stopped");
}

std::string SkipListServer::handleMessage(const std::string& message, std::shared_ptr<ClientConnection> client) {
    try {
        // 更新统计信息
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            server_stats_.total_commands++;
        }
        
        // 处理Redis命令
        return redis_handler_.handleCommand(message, client);
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error handling message: " + std::string(e.what()));
        return RedisProtocol::createError("ERR internal error");
    }
}

void SkipListServer::persistenceLoop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(persistence_interval_));
        
        if (running_) {
            try {
                redis_handler_.saveData();
                LOG_DEBUG("Data persisted successfully");
            } catch (const std::exception& e) {
                LOG_ERROR("Error during persistence: " + std::string(e.what()));
            }
        }
    }
}

void SkipListServer::monitorLoop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(30)); // 每30秒更新一次统计信息
        
        if (running_) {
            try {
                // 更新运行时间
                auto now = std::chrono::steady_clock::now();
                auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
                
                {
                    std::lock_guard<std::mutex> lock(stats_mutex_);
                    server_stats_.uptime_seconds = uptime.count();
                    server_stats_.current_connections = tcp_server_->getConnectionCount();
                }
                
                // 检查日志轮转
                Logger::getInstance().checkRotation();
                
            } catch (const std::exception& e) {
                LOG_ERROR("Error in monitor loop: " + std::string(e.what()));
            }
        }
    }
}

void SkipListServer::setupSignalHandlers() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
#ifdef _WIN32
    signal(SIGBREAK, signalHandler);
#else
    signal(SIGQUIT, signalHandler);
#endif
}

void SkipListServer::gracefulShutdown() {
    LOG_INFO("Graceful shutdown initiated");
    stop();
}

bool SkipListServer::loadConfiguration(const std::string& config_file) {
    // 从环境变量加载配置
    config_.loadFromEnvironment();
    
    // 如果指定了配置文件，则从文件加载
    if (!config_file.empty()) {
        if (!config_.loadFromFile(config_file)) {
            std::cerr << "Warning: Failed to load config file: " << config_file << std::endl;
        }
    }
    
    // 设置持久化参数
    const auto& skiplist_config = config_.getSkipListConfig();
    persistence_enabled_ = skiplist_config.enable_persistence;
    persistence_interval_ = skiplist_config.persistence_interval;
    
    return true;
}

void SkipListServer::initLogging() {
    const auto& log_config = config_.getLogConfig();
    
    LogLevel level = LogLevel::INFO;
    if (log_config.log_level == "DEBUG") level = LogLevel::DEBUG;
    else if (log_config.log_level == "WARN") level = LogLevel::WARN;
    else if (log_config.log_level == "ERROR") level = LogLevel::ERROR;
    else if (log_config.log_level == "FATAL") level = LogLevel::FATAL;
    
    Logger::getInstance().init(log_config.log_file, level, log_config.enable_console);
    Logger::getInstance().setMaxFileSize(log_config.max_file_size);
    Logger::getInstance().setMaxFiles(log_config.max_files);
}

bool SkipListServer::initNetworkServer() {
    const auto& server_config = config_.getServerConfig();
    
    tcp_server_ = std::make_unique<TCPServer>();
    
    if (!tcp_server_->init(server_config.host, server_config.port, server_config.thread_pool_size)) {
        return false;
    }
    
    // 设置消息处理器
    tcp_server_->setMessageHandler([this](const std::string& message, std::shared_ptr<ClientConnection> client) {
        return handleMessage(message, client);
    });
    
    return true;
}

void SkipListServer::createDirectories() {
    const auto& log_config = config_.getLogConfig();
    const auto& skiplist_config = config_.getSkipListConfig();
    
    // 创建日志目录
    std::filesystem::path log_path(log_config.log_file);
    if (log_path.has_parent_path()) {
        std::filesystem::create_directories(log_path.parent_path());
    }
    
    // 创建数据存储目录
    std::filesystem::path data_path(skiplist_config.data_file);
    if (data_path.has_parent_path()) {
        std::filesystem::create_directories(data_path.parent_path());
    }
}

SkipListServer::ServerStats SkipListServer::getStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return server_stats_;
} 