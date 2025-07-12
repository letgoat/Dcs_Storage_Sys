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
        
        // 设置消息处理器来处理从节点连接
        replication_server_->setMessageHandler(
            [this](const std::string& message, std::shared_ptr<ClientConnection> client) {
                return handleReplicationMessage(message, client);
            }
        );
        
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

void ReplicationManager::masterLoop() {
    std::cout << "Master loop started" << std::endl;
    
    while (running_) {
        try {
            // 1. 处理从节点连接
            handleNewSlaveConnections();
            
            // 2. 处理复制命令队列
            processReplicationQueue();
            
            // 3. 更新从节点状态统计
            updateSlaveStats();
            
            // 4. 检查从节点健康状态
            checkSlaveHealth();
            
            // 5. 清理断开的从节点
            cleanupDeadSlaves();
            
            // 主循环间隔
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
        } catch (const std::exception& e) {
            std::cerr << "Error in master loop: " << e.what() << std::endl;
            // 短暂等待后继续，避免频繁错误日志
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
    
    std::cout << "Master loop stopped" << std::endl;
}

void ReplicationManager::handleNewSlaveConnections() {
    // 由于TCPServer使用消息处理器模式，新连接的处理在setMessageHandler中完成
    // 这里主要处理从节点的状态更新和统计信息
    
    // 更新统计信息
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        int online_count = 0;
        for (const auto& slave : slaves_) {
            if (slave->is_online) {
                online_count++;
            }
        }
        stats_.connected_slaves = online_count;
    }
    
    // 检查是否有新的从节点需要处理
    // 这里可以添加一些定期清理或状态检查的逻辑
}

void ReplicationManager::processReplicationQueue() {
    // 处理复制日志队列中的命令
    std::lock_guard<std::mutex> lock(replication_log_mutex_);
    
    if (!replication_log_.empty()) {
        // 获取最新的复制偏移量
        int64_t current_offset = replication_offset_;
        
        // 处理队列中的所有命令
        while (!replication_log_.empty()) {
            const auto& entry = replication_log_.front();
            
            // 只处理未同步的命令
            if (entry.offset > current_offset) {
                // 复制到所有从节点
                replicateToSlaves(entry.command);
                current_offset = entry.offset;
            }
            
            replication_log_.pop();
        }
        
        // 更新复制偏移量
        replication_offset_ = current_offset;
    }
}

void ReplicationManager::updateSlaveStats() {
    std::lock_guard<std::mutex> lock(slaves_mutex_);
    
    int online_slaves = 0;
    int64_t total_lag = 0;
    
    for (const auto& slave : slaves_) {
        if (slave->is_online) {
            online_slaves++;
            // 计算复制延迟
            int64_t lag = replication_offset_ - slave->replication_offset;
            total_lag += lag;
        }
    }
    
    // 更新统计信息
    {
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        stats_.connected_slaves = online_slaves;
        stats_.replication_lag = online_slaves > 0 ? total_lag / online_slaves : 0;
    }
}

void ReplicationManager::checkSlaveHealth() {
    std::lock_guard<std::mutex> lock(slaves_mutex_);
    
    auto now = std::chrono::system_clock::now();
    auto timeout = std::chrono::milliseconds(ping_interval_ms_ * 2); // 2倍ping间隔作为健康检查超时
    
    for (auto& slave : slaves_) {
        // 检查从节点是否超时
        if (slave->is_online && (now - slave->last_ping) > timeout) {
            std::cout << "Slave " << slave->id << " appears to be unhealthy, marking as offline" << std::endl;
            slave->is_online = false;
            slave->state = ReplicationState::DISCONNECTED;
        }
    }
}

void ReplicationManager::handleSlaveConnection(std::shared_ptr<ClientConnection> client) {
    if (!client || !client->isValid()) {
        std::cerr << "Invalid client connection" << std::endl;
        return;
    }
    
    std::cout << "New slave connection from: " << client->getClientAddress() << std::endl;
    
    try {
        // 1. 接收从节点的握手消息
        std::string handshake = client->receive();
        if (handshake.empty()) {
            std::cerr << "Failed to receive handshake from slave" << std::endl;
            return;
        }
        
        // 2. 解析从节点信息（简化实现）
        // 实际应该解析更复杂的协议，这里假设格式为 "SLAVE:host:port"
        std::string slave_host = client->getClientAddress();
        int slave_port = 0; // 实际应该从握手消息中解析
        
        // 3. 创建或更新从节点信息
        std::lock_guard<std::mutex> lock(slaves_mutex_);
        
        // 查找是否已存在该从节点
        auto existing_slave = std::find_if(slaves_.begin(), slaves_.end(),
            [&slave_host](const std::shared_ptr<SlaveInfo>& slave) {
                return slave->host == slave_host;
            });
        
        std::shared_ptr<SlaveInfo> slave_info;
        if (existing_slave != slaves_.end()) {
            // 更新现有从节点
            slave_info = *existing_slave;
            std::cout << "Updating existing slave: " << slave_info->id << std::endl;
        } else {
            // 创建新的从节点
            slave_info = std::make_shared<SlaveInfo>(slave_host, slave_port);
            slaves_.push_back(slave_info);
            std::cout << "Added new slave: " << slave_info->id << std::endl;
        }
        
        // 4. 更新从节点状态
        slave_info->state = ReplicationState::CONNECTED;
        slave_info->is_online = true;
        slave_info->last_ping = std::chrono::system_clock::now();
        
        // 5. 发送握手响应
        std::string response = "MASTER:OK:" + std::to_string(replication_offset_);
        if (!client->send(response)) {
            std::cerr << "Failed to send handshake response to slave" << std::endl;
            slave_info->is_online = false;
            slave_info->state = ReplicationState::DISCONNECTED;
            return;
        }
        
        std::cout << "Slave " << slave_info->id << " connected successfully" << std::endl;
        
        // 6. 开始同步数据（如果有需要）
        if (slave_info->replication_offset < replication_offset_) {
            slave_info->state = ReplicationState::SYNCING;
            std::cout << "Starting sync with slave " << slave_info->id << std::endl;
            
            // 发送需要同步的命令
            sendSyncCommands(client, slave_info);
            
            slave_info->state = ReplicationState::ONLINE;
            std::cout << "Sync completed for slave " << slave_info->id << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error handling slave connection: " << e.what() << std::endl;
    }
}

void ReplicationManager::sendSyncCommands(std::shared_ptr<ClientConnection> client, 
                                         std::shared_ptr<SlaveInfo> slave) {
    // 发送从节点缺失的命令
    std::lock_guard<std::mutex> lock(replication_log_mutex_);
    
    // 这里简化实现，实际应该从复制日志中获取缺失的命令
    // 并按照正确的顺序发送给从节点
    
    std::cout << "Sending sync commands to slave " << slave->id 
              << " (offset: " << slave->replication_offset 
              << " -> " << replication_offset_ << ")" << std::endl;
    
    // 实际实现中应该通过client发送命令
    if (client && client->isValid()) {
        // 发送同步命令的示例
        std::string sync_command = "SYNC:" + std::to_string(slave->replication_offset) + ":" + std::to_string(replication_offset_);
        client->send(sync_command);
    }
    
    // 更新从节点的复制偏移量
    slave->replication_offset = replication_offset_;
}

std::string ReplicationManager::handleReplicationMessage(const std::string& message, 
                                                        std::shared_ptr<ClientConnection> client) {
    if (!client || !client->isValid()) {
        return "ERROR:Invalid connection";
    }
    
    std::cout << "Received replication message from " << client->getClientAddress() 
              << ": " << message << std::endl;
    
    // 解析消息类型
    if (message.find("SLAVE_CONNECT") == 0) {
        // 从节点连接请求
        handleSlaveConnection(client);
        return "MASTER:OK:" + std::to_string(replication_offset_);
        
    } else if (message.find("PING") == 0) {
        // 从节点心跳
        std::string slave_id = client->getClientAddress();
        updateSlavePing(slave_id);
        return "PONG";
        
    } else if (message.find("SYNC_REQUEST") == 0) {
        // 从节点同步请求
        // 解析同步偏移量
        size_t pos = message.find(':');
        if (pos != std::string::npos) {
            std::string offset_str = message.substr(pos + 1);
            try {
                int64_t slave_offset = std::stoll(offset_str);
                return handleSyncRequest(client, slave_offset);
            } catch (const std::exception& e) {
                return "ERROR:Invalid sync offset";
            }
        }
        return "ERROR:Invalid sync request format";
        
    } else if (message.find("COMMAND_ACK") == 0) {
        // 从节点命令确认
        // 解析确认的偏移量
        size_t pos = message.find(':');
        if (pos != std::string::npos) {
            std::string offset_str = message.substr(pos + 1);
            try {
                int64_t ack_offset = std::stoll(offset_str);
                updateSlaveOffset(client->getClientAddress(), ack_offset);
                return "OK";
            } catch (const std::exception& e) {
                return "ERROR:Invalid ack offset";
            }
        }
        return "ERROR:Invalid ack format";
        
    } else {
        // 未知消息类型
        std::cout << "Unknown replication message: " << message << std::endl;
        return "ERROR:Unknown message type";
    }
}

void ReplicationManager::updateSlavePing(const std::string& slave_id) {
    std::lock_guard<std::mutex> lock(slaves_mutex_);
    
    for (auto& slave : slaves_) {
        if (slave->id == slave_id || slave->host + ":" + std::to_string(slave->port) == slave_id) {
            slave->last_ping = std::chrono::system_clock::now();
            slave->is_online = true;
            break;
        }
    }
}

std::string ReplicationManager::handleSyncRequest(std::shared_ptr<ClientConnection> client, 
                                                 int64_t slave_offset) {
    std::lock_guard<std::mutex> lock(replication_log_mutex_);
    
    // 计算需要同步的命令数量
    int64_t commands_to_sync = replication_offset_ - slave_offset;
    
    if (commands_to_sync <= 0) {
        return "SYNC:OK:0";
    }
    
    // 这里简化实现，实际应该从复制日志中获取具体的命令
    std::string sync_response = "SYNC:START:" + std::to_string(commands_to_sync);
    
    // 发送同步数据
    if (client && client->isValid()) {
        client->send(sync_response);
    }
    
    return sync_response;
}

void ReplicationManager::updateSlaveOffset(const std::string& slave_id, int64_t offset) {
    std::lock_guard<std::mutex> lock(slaves_mutex_);
    
    for (auto& slave : slaves_) {
        if (slave->id == slave_id || slave->host + ":" + std::to_string(slave->port) == slave_id) {
            slave->replication_offset = offset;
            break;
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
    
    try {
        // 创建TCP客户端连接
        // 这里简化实现，实际应该使用TCP客户端库
        // 在实际实现中，需要创建到主节点的TCP连接
        
        // 模拟连接过程
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 发送连接握手消息
        std::string handshake = "SLAVE_CONNECT:" + std::to_string(replication_offset_);
        // 实际应该通过TCP连接发送: master_connection_->send(handshake);
        
        // 接收主节点响应
        std::string response = "MASTER:OK:" + std::to_string(replication_offset_);
        // 实际应该从TCP连接接收: std::string response = master_connection_->receive();
        
        if (response.find("MASTER:OK") == 0) {
            updateState(ReplicationState::CONNECTED);
            std::cout << "Connected to master successfully" << std::endl;
        } else {
            throw std::runtime_error("Failed to establish connection with master");
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to connect to master: " << e.what() << std::endl;
        updateState(ReplicationState::ERROR);
        std::this_thread::sleep_for(std::chrono::seconds(5)); // 重连延迟
    }
}

void ReplicationManager::syncWithMaster() {
    std::cout << "Syncing with master..." << std::endl;
    
    try {
        // 发送同步请求
        std::string sync_request = "SYNC_REQUEST:" + std::to_string(replication_offset_);
        // 实际应该通过TCP连接发送: master_connection_->send(sync_request);
        
        // 接收同步响应
        std::string sync_response = "SYNC:START:0"; // 模拟响应
        // 实际应该从TCP连接接收: std::string sync_response = master_connection_->receive();
        
        if (sync_response.find("SYNC:START") == 0) {
            // 解析需要同步的命令数量
            size_t pos = sync_response.find(':');
            if (pos != std::string::npos) {
                std::string count_str = sync_response.substr(pos + 1);
                try {
                    int commands_to_sync = std::stoi(count_str);
                    
                    if (commands_to_sync > 0) {
                        std::cout << "Need to sync " << commands_to_sync << " commands" << std::endl;
                        
                        // 接收并应用同步命令
                        for (int i = 0; i < commands_to_sync; ++i) {
                            std::string command = "SET synced_key" + std::to_string(i) + " synced_value" + std::to_string(i);
                            // 实际应该从TCP连接接收: std::string command = master_connection_->receive();
                            
                            // 应用命令
                            applyReplicationCommand(command);
                            
                            // 发送确认
                            std::string ack = "COMMAND_ACK:" + std::to_string(replication_offset_);
                            // 实际应该通过TCP连接发送: master_connection_->send(ack);
                        }
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Failed to parse sync count: " << e.what() << std::endl;
                    throw;
                }
            }
            
            updateState(ReplicationState::ONLINE);
            std::cout << "Sync completed, now online" << std::endl;
        } else {
            throw std::runtime_error("Invalid sync response from master");
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to sync with master: " << e.what() << std::endl;
        updateState(ReplicationState::ERROR);
    }
}

void ReplicationManager::handleMasterCommands() {
    // 处理来自主节点的命令
    try {
        // 发送心跳
        std::string ping = "PING";
        // 实际应该通过TCP连接发送: master_connection_->send(ping);
        
        // 接收主节点响应
        std::string response = "PONG"; // 模拟响应
        // 实际应该从TCP连接接收: std::string response = master_connection_->receive();
        
        if (response == "PONG") {
            // 心跳正常，继续处理命令
            std::string command = ""; // 模拟命令
            // 实际应该从TCP连接接收: std::string command = master_connection_->receive();
            
            if (!command.empty()) {
                // 解析并应用命令
                std::string deserialized = deserializeReplicationCommand(command);
                if (!deserialized.empty()) {
                    std::cout << "Received command from master: " << deserialized << std::endl;
                    applyReplicationCommand(deserialized);
                    
                    // 发送确认
                    std::string ack = "COMMAND_ACK:" + std::to_string(replication_offset_);
                    // 实际应该通过TCP连接发送: master_connection_->send(ack);
                }
            }
        } else {
            // 心跳失败，可能需要重连
            std::cout << "Heartbeat failed, may need to reconnect" << std::endl;
            updateState(ReplicationState::DISCONNECTED);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error handling master commands: " << e.what() << std::endl;
        updateState(ReplicationState::ERROR);
    }
    
    // 短暂等待，避免过于频繁的处理
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
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

void ReplicationManager::pingSlaves() {
    std::cout << "Ping slaves loop started" << std::endl;
    
    while (running_) {
        try {
            // 清理断开的从节点
            cleanupDeadSlaves();
            
            // 向所有从节点发送心跳
            std::lock_guard<std::mutex> lock(slaves_mutex_);
            for (auto& slave : slaves_) {
                if (slave->is_online) {
                    // 发送ping命令
                    std::string ping_message = "PING";
                    // 实际应该通过TCP连接发送: slave->connection->send(ping_message);
                    
                    // 模拟ping响应
                    std::string pong_response = "PONG";
                    // 实际应该从TCP连接接收: std::string pong_response = slave->connection->receive();
                    
                    if (pong_response == "PONG") {
                        // 更新最后ping时间
                        slave->last_ping = std::chrono::system_clock::now();
                        std::cout << "Ping successful for slave: " << slave->id << std::endl;
                    } else {
                        // Ping失败，标记为离线
                        slave->is_online = false;
                        slave->state = ReplicationState::DISCONNECTED;
                        std::cout << "Ping failed for slave: " << slave->id << std::endl;
                    }
                } else {
                    // 从节点离线，尝试重新连接
                    std::cout << "Slave " << slave->id << " is offline, attempting to reconnect..." << std::endl;
                    slave->state = ReplicationState::CONNECTING;
                }
            }
            
            // 等待下一次ping
            std::this_thread::sleep_for(std::chrono::milliseconds(ping_interval_ms_));
            
        } catch (const std::exception& e) {
            std::cerr << "Error in ping loop: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
    
    std::cout << "Ping slaves loop stopped" << std::endl;
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