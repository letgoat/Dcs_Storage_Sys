#pragma once
#include <string>
#include <map>
#include <memory>
#include <mutex>

class Config {
public:
    static Config& getInstance();
    
    // 禁止拷贝和赋值
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    
    // 配置项
    struct ServerConfig {
        int port = 6379;
        std::string host = "0.0.0.0";
        int max_connections = 1000;
        int thread_pool_size = 4;
        bool enable_cluster = false;
        std::string cluster_nodes;
    };
    
    struct SkipListConfig {
        int max_level = 18;
        std::string data_file = "store/dumpFile";
        bool enable_persistence = true;
        int persistence_interval = 60; // seconds
    };
    
    struct LogConfig {
        std::string log_level = "INFO";
        std::string log_file = "logs/skiplist.log";
        bool enable_console = true;
        int max_file_size = 100 * 1024 * 1024; // 100MB
        int max_files = 10;
    };
    
    struct AOFConfig {
        bool enable_aof = false;
        std::string aof_file = "store/appendonly.aof";
        std::string aof_fsync = "everysec"; // always, everysec, no
        int aof_fsync_interval = 1; // 秒
    };
    
    // 加载配置文件
    bool loadFromFile(const std::string& filename);
    
    // 从环境变量加载配置
    void loadFromEnvironment();
    
    // 获取配置
    const ServerConfig& getServerConfig() const { return server_config_; }
    const SkipListConfig& getSkipListConfig() const { return skiplist_config_; }
    const LogConfig& getLogConfig() const { return log_config_; }
    const AOFConfig& getAOFConfig() const { return aof_config_; }
    
    // 设置配置
    void setServerConfig(const ServerConfig& config);
    void setSkipListConfig(const SkipListConfig& config);
    void setLogConfig(const LogConfig& config);
    void setAOFConfig(const AOFConfig& config);
    
    // 保存配置到文件
    bool saveToFile(const std::string& filename) const;
    
    // 获取配置字符串
    std::string getString(const std::string& key, const std::string& default_value = "") const;
    int getInt(const std::string& key, int default_value = 0) const;
    bool getBool(const std::string& key, bool default_value = false) const;
    
    // 设置配置
    void setString(const std::string& key, const std::string& value);
    void setInt(const std::string& key, int value);
    void setBool(const std::string& key, bool value);

private:
    Config() = default;
    ~Config() = default;
    
    ServerConfig server_config_;
    SkipListConfig skiplist_config_;
    LogConfig log_config_;
    AOFConfig aof_config_;
    
    std::map<std::string, std::string> custom_config_;
    mutable std::mutex config_mutex_;
    
    // 应用自定义配置
    void applyCustomConfig();
}; 