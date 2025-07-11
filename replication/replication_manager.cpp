#include "replication_manager.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <thread>

ReplicationManager::ReplicationManager()
    : role_(ReplicationRole::UNKNOWN)
    , state_(ReplicationState::DISCONNECTED)
    , running_(false)
    , replication_offset_(0)
    , replication_port_(16379)
    , ping_interval_ms_(1000)
    , sync_timeout_ms_(5000)
    , max_replication_log_size_(10000) {
}

ReplicationManager::~ReplicationManager() {
    stopReplication();
}

void ReplicationManager::init(const std::string& master_host, int master_port) {
    if (!master_host.empty()) { // 如果主节点地址不为空，说明当前节点不是主节点，而是需要去连接主节点；当前节点为从节点
        // 从节点模式
        setRole(ReplicationRole::SLAVE);
        setMasterAddress(master_host, master_port);
        std::cout << "Initialized as SLAVE, master: " << master_host << ":" << master_port << std::endl;
    } else { // 主节点为空，说明不需要去连接主节点；当前节点就是主节点
        // 主节点模式
        setRole(ReplicationRole::MASTER);
        std::cout << "Initialized as MASTER" << std::endl;
    }
}

bool ReplicationManager::startReplication() {
    if (running_) {
        std::cout << "Replication is already running" << std::endl;
        return true;
    }
    
    running_ = true;
    
    if (isMaster()) {
        // 启动主节点复制服务器
        replication_server_ = std::make_unique<TCPServer>();
        if (!replication_server_->init("0.0.0.0", replication_port_, 2)) {
            std::cerr << "Failed to initialize replication server" << std::endl;
            return false;
        }
        
        if (!replication_server_->start()) {
            std::cerr << "Failed to start replication server" << std::endl;
            return false;
        }
        
        // 启动主节点线程
        master_thread_ = std::thread(&ReplicationManager::masterLoop, this);
        ping_thread_ = std::thread(&ReplicationManager::pingSlaves, this);
        
        updateState(ReplicationState::ONLINE);
        std::cout << "Master replication started on port " << replication_port_ << std::endl;
        
    } else if (isSlave()) {
        // 启动从节点线程
        slave_thread_ = std::thread(&ReplicationManager::slaveLoop, this);
        
        updateState(ReplicationState::CONNECTING);
        std::cout << "Slave replication started, connecting to master" << std::endl;
    }
    
    return true;
}

void ReplicationManager::stopReplication() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    // 停止复制服务器
    if (replication_server_) {
        replication_server_->stop();
    }
    
    // 等待线程结束
    if (master_thread_.joinable()) {
        master_thread_.join();
    }
    if (slave_thread_.joinable()) {
        slave_thread_.join();
    }
    if (ping_thread_.joinable()) {
        ping_thread_.join();
    }
    
    updateState(ReplicationState::DISCONNECTED);
    std::cout << "Replication stopped" << std::endl;
}

void ReplicationManager::setRole(ReplicationRole role) {
    role_ = role;
}

bool ReplicationManager::addSlave(const std::string& host, int port) {
    if (!isMaster()) {
        std::cerr << "Only master can add slaves" << std::endl;
        return false;
    }
    
    std::lock_guard<std::mutex> lock(slaves_mutex_);
    
    // 检查是否已存在
    for (const auto& slave : slaves_) {
        if (slave->host == host && slave->port == port) {
            std::cout << "Slave already exists: " << host << ":" << port << std::endl;
            return false;
        }
    }
    
    auto slave = std::make_shared<SlaveInfo>(host, port);
    slaves_.push_back(slave);
    
    std::cout << "Added slave: " << host << ":" << port << std::endl;
    return true;
}

void ReplicationManager::removeSlave(const std::string& slave_id) {
    if (!isMaster()) {
        std::cerr << "Only master can remove slaves" << std::endl;
        return;
    }
    
    std::lock_guard<std::mutex> lock(slaves_mutex_);
    
    slaves_.erase(
        std::remove_if(slaves_.begin(), slaves_.end(),
            [&slave_id](const std::shared_ptr<SlaveInfo>& slave) {
                return slave->id == slave_id;
            }),
        slaves_.end()
    );
    
    std::cout << "Removed slave: " << slave_id << std::endl;
}

std::vector<SlaveInfo> ReplicationManager::getSlaves() const {
    std::lock_guard<std::mutex> lock(slaves_mutex_);
    
    std::vector<SlaveInfo> result;
    for (const auto& slave : slaves_) {
        result.push_back(*slave);
    }
    return result;
}

void ReplicationManager::setMasterAddress(const std::string& host, int port) {
    master_host_ = host;
    master_port_ = port;
}

std::string ReplicationManager::getMasterAddress() const {
    return master_host_ + ":" + std::to_string(master_port_);
}

void ReplicationManager::replicateCommand(const std::string& command) {
    if (!isMaster()) {
        std::cerr << "Only master can replicate commands" << std::endl;
        return;
    }
    
    // 记录复制日志
    logReplicationEntry(command);
    
    // 复制到所有从节点
    replicateToSlaves(command);
    
    // 更新统计信息
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_commands_replicated++;
        stats_.total_bytes_replicated += command.length();
        stats_.last_sync_time = std::chrono::system_clock::now();
    }
}

void ReplicationManager::applyReplicationCommand(const std::string& command) {
    if (!isSlave()) {
        std::cerr << "Only slave can apply replication commands" << std::endl;
        return;
    }
    
    // 调用命令处理器
    if (command_handler_) {
        command_handler_(command);
    }
    
    // 更新复制偏移量
    replication_offset_++;
}

// 还未实现
void ReplicationManager::masterLoop() {
    while (running_) {
        try {
            // 处理从节点连接
            // 这里还未实现，实际应该处理从节点的连接和命令同步
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } catch (const std::exception& e) {
            std::cerr << "Error in master loop: " << e.what() << std::endl;
        }
    }
}

void ReplicationManager::slaveLoop() {
    while (running_) {
        try {
            if (state_ == ReplicationState::CONNECTING || 
                state_ == ReplicationState::DISCONNECTED) {
                connectToMaster();
            } else if (state_ == ReplicationState::CONNECTED) {
                syncWithMaster();
            } else if (state_ == ReplicationState::ONLINE) {
                handleMasterCommands();
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } catch (const std::exception& e) {
            std::cerr << "Error in slave loop: " << e.what() << std::endl;
            updateState(ReplicationState::ERROR);
        }
    }
}

void ReplicationManager::connectToMaster() {
    std::cout << "Connecting to master: " << getMasterAddress() << std::endl;
    
    // 这里简化实现，实际应该建立TCP连接
    // 在实际实现中，需要创建到主节点的TCP连接
    
    updateState(ReplicationState::CONNECTED);
    std::cout << "Connected to master" << std::endl;
}

void ReplicationManager::syncWithMaster() {
    std::cout << "Syncing with master..." << std::endl;
    
    // 发送PSYNC命令进行同步
    // 这里简化实现
    
    updateState(ReplicationState::ONLINE);
    std::cout << "Sync completed, now online" << std::endl;
}

void ReplicationManager::handleMasterCommands() {
    // 处理来自主节点的命令
    // 这里简化实现，实际应该从TCP连接读取命令
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void ReplicationManager::replicateToSlaves(const std::string& command) {
    std::lock_guard<std::mutex> lock(slaves_mutex_);
    
    std::string serialized = serializeReplicationCommand(command);
    
    for (auto& slave : slaves_) {
        if (slave->is_online) {
            // 这里简化实现，实际应该通过TCP连接发送命令
            // slave->connection->send(serialized);
            std::cout << "Replicated command to slave: " << slave->id << std::endl;
        }
    }
}

// 还未实现
void ReplicationManager::pingSlaves() {
    while (running_) {
        try {
            cleanupDeadSlaves();
            
            std::lock_guard<std::mutex> lock(slaves_mutex_);
            for (auto& slave : slaves_) {
                // 发送ping命令
                // 这里还未实现
                slave->last_ping = std::chrono::system_clock::now();
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(ping_interval_ms_));
        } catch (const std::exception& e) {
            std::cerr << "Error in ping loop: " << e.what() << std::endl;
        }
    }
}

void ReplicationManager::cleanupDeadSlaves() {
    std::lock_guard<std::mutex> lock(slaves_mutex_);
    
    auto now = std::chrono::system_clock::now();
    // 超时时间是Ping间隔的3倍
    auto timeout = std::chrono::milliseconds(ping_interval_ms_ * 3);
    
    slaves_.erase(
        std::remove_if(slaves_.begin(), slaves_.end(),
            [&now, &timeout](const std::shared_ptr<SlaveInfo>& slave) {
                return (now - slave->last_ping) > timeout;
            }),
        slaves_.end()
    );
}

void ReplicationManager::updateState(ReplicationState new_state) {
    state_ = new_state;
    std::cout << "Replication state changed to: " << static_cast<int>(new_state) << std::endl;
}

void ReplicationManager::logReplicationEntry(const std::string& command) {
    std::lock_guard<std::mutex> lock(replication_log_mutex_);
    
    replication_log_.emplace(replication_offset_, command);
    replication_offset_++;
    
    // 限制日志大小
    while (replication_log_.size() > max_replication_log_size_) {
        replication_log_.pop();
    }
}

std::string ReplicationManager::serializeReplicationCommand(const std::string& command) {
    // 简单的序列化格式：长度+命令
    std::string result = std::to_string(command.length()) + ":" + command;
    return result;
}

std::string ReplicationManager::deserializeReplicationCommand(const std::string& data) {
    // 简单的反序列化
    size_t pos = data.find(':');
    if (pos == std::string::npos) {
        return "";
    }
    
    int length = std::stoi(data.substr(0, pos));
    return data.substr(pos + 1, length);
} 