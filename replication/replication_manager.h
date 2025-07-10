#pragma once
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <map>
#include <functional>
#include "../network/tcp_server.h"
#include "../logger/logger.h"

// 复制角色枚举
enum class ReplicationRole {
    MASTER,
    SLAVE,
    UNKNOWN
};

// 复制状态枚举
enum class ReplicationState {
    CONNECTING,
    CONNECTED,
    SYNCING,
    ONLINE,
    DISCONNECTED,
    ERROR
};

// 从节点信息
struct SlaveInfo {
    // 唯一标识一个从节点(host:port)
    std::string id;
    // 记录从节点的网络地址(主节点建立连接，发送数据，检查状态)
    std::string host;
    int port;

    ReplicationState state;
    // 记录主节点上次与该从节点通信的时间点
    std::chrono::system_clock::time_point last_ping;
    // 记录从节点已经同步到的复制偏移量
    int64_t replication_offset;
    // 记录从节点是否在线
    bool is_online;
    
    SlaveInfo(const std::string& h, int p) 
        : host(h), port(p), state(ReplicationState::CONNECTING), 
          replication_offset(0), is_online(false) {
        id = host + ":" + std::to_string(port);
    }
};

// 复制日志条目
struct ReplicationLogEntry {
    int64_t offset;
    std::string command;
    // 记录该条日志条目的生成时间
    std::chrono::system_clock::time_point timestamp;
    
    ReplicationLogEntry(int64_t off, const std::string& cmd)
        : offset(off), command(cmd), timestamp(std::chrono::system_clock::now()) {}
};

// 主从复制管理器
class ReplicationManager {
public:
    ReplicationManager();
    ~ReplicationManager();
    
    // 初始化复制管理器
    void init(const std::string& master_host = "", int master_port = 0);
    
    // 启动复制
    bool startReplication();
    
    // 停止复制
    void stopReplication();
    
    // 设置复制角色
    void setRole(ReplicationRole role);
    
    // 获取复制角色
    ReplicationRole getRole() const { return role_; }
    
    // 添加从节点（主节点使用）
    bool addSlave(const std::string& host, int port);
    
    // 移除从节点（主节点使用）
    void removeSlave(const std::string& slave_id);
    
    // 获取所有从节点
    std::vector<SlaveInfo> getSlaves() const;
    
    // 设置主节点地址（从节点使用）
    void setMasterAddress(const std::string& host, int port);
    
    // 获取主节点地址
    std::string getMasterAddress() const;
    
    // 复制命令（主节点使用）
    void replicateCommand(const std::string& command);
    
    // 应用复制命令（从节点使用）
    void applyReplicationCommand(const std::string& command);
    
    // 获取复制偏移量
    int64_t getReplicationOffset() const { return replication_offset_; }
    
    // 设置复制偏移量
    void setReplicationOffset(int64_t offset) { replication_offset_ = offset; }
    
    // 检查是否为主节点
    bool isMaster() const { return role_ == ReplicationRole::MASTER; }
    
    // 检查是否为从节点
    bool isSlave() const { return role_ == ReplicationRole::SLAVE; }
    
    // 获取复制状态
    ReplicationState getState() const { return state_; }
    
    // 设置命令处理器回调
    void setCommandHandler(std::function<void(const std::string&)> handler) {
        command_handler_ = handler;
    }
    
    // 获取复制统计信息
    struct ReplicationStats {
        int64_t total_commands_replicated = 0;
        int64_t total_bytes_replicated = 0;
        int64_t replication_lag = 0;
        int connected_slaves = 0;
        std::chrono::system_clock::time_point last_sync_time;
    };
    
    const ReplicationStats& getStats() const { return stats_; }

private:
    // 主节点相关方法
    void masterLoop();
    void handleSlaveConnection(std::shared_ptr<ClientConnection> client);
    void replicateToSlaves(const std::string& command);
    void pingSlaves();
    void cleanupDeadSlaves();
    
    // 从节点相关方法
    void slaveLoop();
    void connectToMaster();
    void syncWithMaster();
    void handleMasterCommands();
    
    // 通用方法
    void updateState(ReplicationState new_state);
    void logReplicationEntry(const std::string& command);
    std::string serializeReplicationCommand(const std::string& command);
    std::string deserializeReplicationCommand(const std::string& data);
    
    // 成员变量
    ReplicationRole role_;
    ReplicationState state_;
    std::atomic<bool> running_;
    
    // 主节点相关
    std::vector<std::shared_ptr<SlaveInfo>> slaves_;
    mutable std::mutex slaves_mutex_;
    std::unique_ptr<TCPServer> replication_server_;
    
    // 从节点相关
    std::string master_host_;
    int master_port_;
    std::shared_ptr<ClientConnection> master_connection_;
    std::mutex master_connection_mutex_;
    
    // 复制日志
    // 待同步的命令队列
    std::queue<ReplicationLogEntry> replication_log_;
    std::mutex replication_log_mutex_;
    // 主节点最新的命令编号
    int64_t replication_offset_;
    
    // 线程
    std::thread master_thread_;
    std::thread slave_thread_;
    std::thread ping_thread_;
    
    // 统计信息
    ReplicationStats stats_;
    mutable std::mutex stats_mutex_;
    
    // 命令处理器回调
    std::function<void(const std::string&)> command_handler_;
    
    // 配置
    // 主节点监听端口
    int replication_port_;
    // 主从之间发送心跳包的时间间隔
    int ping_interval_ms_;
    // 主从操作的超时时间
    int sync_timeout_ms_;
    // 主节点保留的最大复制目录条目数
    int max_replication_log_size_;
}; 