#include "config.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <algorithm>

Config& Config::getInstance() {
    static Config instance;
    return instance;
}

bool Config::loadFromFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << filename << std::endl;
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // 跳过注释和空行
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // 去除前后空格
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            custom_config_[key] = value;
        }
    }
    
    // 应用配置到各个配置结构
    applyCustomConfig();
    
    return true;
}

void Config::loadFromEnvironment() {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    // 服务器配置
    if (const char* port = std::getenv("SKIPLIST_PORT")) {
        server_config_.port = std::atoi(port);
    }
    if (const char* host = std::getenv("SKIPLIST_HOST")) {
        server_config_.host = host;
    }
    if (const char* max_conn = std::getenv("SKIPLIST_MAX_CONNECTIONS")) {
        server_config_.max_connections = std::atoi(max_conn);
    }
    if (const char* thread_pool = std::getenv("SKIPLIST_THREAD_POOL_SIZE")) {
        server_config_.thread_pool_size = std::atoi(thread_pool);
    }
    if (const char* cluster = std::getenv("SKIPLIST_ENABLE_CLUSTER")) {
        server_config_.enable_cluster = (std::string(cluster) == "true");
    }
    if (const char* nodes = std::getenv("SKIPLIST_CLUSTER_NODES")) {
        server_config_.cluster_nodes = nodes;
    }
    
    // 跳表配置
    if (const char* max_level = std::getenv("SKIPLIST_MAX_LEVEL")) {
        skiplist_config_.max_level = std::atoi(max_level);
    }
    if (const char* data_file = std::getenv("SKIPLIST_DATA_FILE")) {
        skiplist_config_.data_file = data_file;
    }
    if (const char* persistence = std::getenv("SKIPLIST_ENABLE_PERSISTENCE")) {
        skiplist_config_.enable_persistence = (std::string(persistence) == "true");
    }
    if (const char* interval = std::getenv("SKIPLIST_PERSISTENCE_INTERVAL")) {
        skiplist_config_.persistence_interval = std::atoi(interval);
    }
    
    // 日志配置
    if (const char* log_level = std::getenv("SKIPLIST_LOG_LEVEL")) {
        log_config_.log_level = log_level;
    }
    if (const char* log_file = std::getenv("SKIPLIST_LOG_FILE")) {
        log_config_.log_file = log_file;
    }
    if (const char* console = std::getenv("SKIPLIST_ENABLE_CONSOLE")) {
        log_config_.enable_console = (std::string(console) == "true");
    }
}

void Config::setServerConfig(const ServerConfig& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    server_config_ = config;
}

void Config::setSkipListConfig(const SkipListConfig& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    skiplist_config_ = config;
}

void Config::setLogConfig(const LogConfig& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    log_config_ = config;
}

void Config::setAOFConfig(const AOFConfig& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    aof_config_ = config;
}

bool Config::saveToFile(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to create config file: " << filename << std::endl;
        return false;
    }
    
    file << "# SkipList Configuration File\n\n";
    
    // 服务器配置
    file << "[Server]\n";
    file << "port=" << server_config_.port << "\n";
    file << "host=" << server_config_.host << "\n";
    file << "max_connections=" << server_config_.max_connections << "\n";
    file << "thread_pool_size=" << server_config_.thread_pool_size << "\n";
    file << "enable_cluster=" << (server_config_.enable_cluster ? "true" : "false") << "\n";
    file << "cluster_nodes=" << server_config_.cluster_nodes << "\n\n";
    
    // 跳表配置
    file << "[SkipList]\n";
    file << "max_level=" << skiplist_config_.max_level << "\n";
    file << "data_file=" << skiplist_config_.data_file << "\n";
    file << "enable_persistence=" << (skiplist_config_.enable_persistence ? "true" : "false") << "\n";
    file << "persistence_interval=" << skiplist_config_.persistence_interval << "\n\n";
    
    // 日志配置
    file << "[Log]\n";
    file << "log_level=" << log_config_.log_level << "\n";
    file << "log_file=" << log_config_.log_file << "\n";
    file << "enable_console=" << (log_config_.enable_console ? "true" : "false") << "\n";
    file << "max_file_size=" << log_config_.max_file_size << "\n";
    file << "max_files=" << log_config_.max_files << "\n\n";
    
    // 自定义配置
    if (!custom_config_.empty()) {
        file << "[Custom]\n";
        for (const auto& pair : custom_config_) {
            file << pair.first << "=" << pair.second << "\n";
        }
    }
    
    return true;
}

std::string Config::getString(const std::string& key, const std::string& default_value) const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    auto it = custom_config_.find(key);
    return (it != custom_config_.end()) ? it->second : default_value;
}

int Config::getInt(const std::string& key, int default_value) const {
    std::string value = getString(key);
    if (value.empty()) {
        return default_value;
    }
    try {
        return std::stoi(value);
    } catch (...) {
        return default_value;
    }
}

bool Config::getBool(const std::string& key, bool default_value) const {
    std::string value = getString(key);
    if (value.empty()) {
        return default_value;
    }
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    return (value == "true" || value == "1" || value == "yes");
}

void Config::setString(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    custom_config_[key] = value;
}

void Config::setInt(const std::string& key, int value) {
    setString(key, std::to_string(value));
}

void Config::setBool(const std::string& key, bool value) {
    setString(key, value ? "true" : "false");
}

void Config::applyCustomConfig() {
    // 应用自定义配置到各个配置结构
    if (custom_config_.find("port") != custom_config_.end()) {
        server_config_.port = getInt("port", server_config_.port);
    }
    if (custom_config_.find("host") != custom_config_.end()) {
        server_config_.host = getString("host", server_config_.host);
    }
    if (custom_config_.find("max_connections") != custom_config_.end()) {
        server_config_.max_connections = getInt("max_connections", server_config_.max_connections);
    }
    if (custom_config_.find("thread_pool_size") != custom_config_.end()) {
        server_config_.thread_pool_size = getInt("thread_pool_size", server_config_.thread_pool_size);
    }
    if (custom_config_.find("enable_cluster") != custom_config_.end()) {
        server_config_.enable_cluster = getBool("enable_cluster", server_config_.enable_cluster);
    }
    if (custom_config_.find("cluster_nodes") != custom_config_.end()) {
        server_config_.cluster_nodes = getString("cluster_nodes", server_config_.cluster_nodes);
    }
    
    if (custom_config_.find("max_level") != custom_config_.end()) {
        skiplist_config_.max_level = getInt("max_level", skiplist_config_.max_level);
    }
    if (custom_config_.find("data_file") != custom_config_.end()) {
        skiplist_config_.data_file = getString("data_file", skiplist_config_.data_file);
    }
    if (custom_config_.find("enable_persistence") != custom_config_.end()) {
        skiplist_config_.enable_persistence = getBool("enable_persistence", skiplist_config_.enable_persistence);
    }
    if (custom_config_.find("persistence_interval") != custom_config_.end()) {
        skiplist_config_.persistence_interval = getInt("persistence_interval", skiplist_config_.persistence_interval);
    }
    
    if (custom_config_.find("log_level") != custom_config_.end()) {
        log_config_.log_level = getString("log_level", log_config_.log_level);
    }
    if (custom_config_.find("log_file") != custom_config_.end()) {
        log_config_.log_file = getString("log_file", log_config_.log_file);
    }
    if (custom_config_.find("enable_console") != custom_config_.end()) {
        log_config_.enable_console = getBool("enable_console", log_config_.enable_console);
    }

    // AOF配置
    if (custom_config_.find("enable_aof") != custom_config_.end()) {
        aof_config_.enable_aof = getBool("enable_aof", aof_config_.enable_aof);
    }
    if (custom_config_.find("aof_file") != custom_config_.end()) {
        aof_config_.aof_file = getString("aof_file", aof_config_.aof_file);
    }
    if (custom_config_.find("aof_fsync") != custom_config_.end()) {
        aof_config_.aof_fsync = getString("aof_fsync", aof_config_.aof_fsync);
    }
    if (custom_config_.find("aof_fsync_interval") != custom_config_.end()) {
        aof_config_.aof_fsync_interval = getInt("aof_fsync_interval", aof_config_.aof_fsync_interval);
    }
} 