# 主从复制功能使用指南

## 概述

本项目实现了Redis兼容的主从复制功能，支持一主多从的架构。主节点负责处理写操作并将命令复制到从节点，从节点负责接收主节点的复制命令并执行。

## 功能特性

- **主从复制**：支持一主多从架构
- **自动同步**：从节点自动与主节点同步数据
- **故障检测**：主节点定期检测从节点状态
- **命令复制**：写操作自动复制到所有从节点
- **配置灵活**：支持配置文件和环境变量配置

## 架构设计

### 复制管理器 (ReplicationManager)

核心组件，负责管理主从复制的所有功能：

- **角色管理**：主节点/从节点角色切换
- **连接管理**：管理主从节点间的连接
- **命令复制**：将写命令复制到从节点
- **状态监控**：监控复制状态和从节点健康状态

### 复制流程

1. **初始化**：根据配置确定角色（主/从）
2. **连接建立**：从节点连接到主节点
3. **数据同步**：从节点与主节点同步数据
4. **命令复制**：主节点将写命令复制到从节点
5. **状态维护**：定期检测连接状态

## 配置说明

### 配置文件 (config/skiplist.conf)

```ini
[Replication]
# 启用复制功能
enable_replication=true
# 复制角色: master, slave
replication_role=master
# 主节点地址（从节点使用）
master_host=127.0.0.1
master_port=6379
# 复制端口
replication_port=16379
# 从节点列表（主节点使用，逗号分隔）
slave_nodes=127.0.0.1:6380,127.0.0.1:6381
```

### 环境变量

```bash
export SKIPLIST_REPLICATION_ENABLED=true
export SKIPLIST_REPLICATION_ROLE=master
export SKIPLIST_MASTER_HOST=127.0.0.1
export SKIPLIST_MASTER_PORT=6379
export SKIPLIST_REPLICATION_PORT=16379
```

## 使用方法

### 1. 启动主节点

```bash
# 编译项目
mkdir build && cd build
cmake ..
make

# 启动主节点
./bin/SkipListProject
```

### 2. 启动从节点

```bash
# 在另一个终端启动从节点
./bin/SkipListProject --slave --master-host=127.0.0.1 --master-port=6379
```

### 3. 使用测试程序

```bash
# 测试复制命令
./bin/replication_test test

# 启动主节点测试
./bin/replication_test master

# 启动从节点测试
./bin/replication_test slave 6380 127.0.0.1 6379
```

## API 接口

### RedisHandler 复制接口

```cpp
// 初始化复制
void initReplication(const std::string& master_host = "", int master_port = 0);

// 启动复制
bool startReplication();

// 停止复制
void stopReplication();

// 检查角色
bool isMaster() const;
bool isSlave() const;

// 管理从节点（主节点使用）
void addSlave(const std::string& host, int port);
std::vector<SlaveInfo> getSlaves() const;
```

### ReplicationManager 接口

```cpp
// 基本操作
void init(const std::string& master_host = "", int master_port = 0);
bool startReplication();
void stopReplication();

// 角色管理
void setRole(ReplicationRole role);
ReplicationRole getRole() const;

// 从节点管理（主节点）
bool addSlave(const std::string& host, int port);
void removeSlave(const std::string& slave_id);
std::vector<SlaveInfo> getSlaves() const;

// 命令复制
void replicateCommand(const std::string& command);
void applyReplicationCommand(const std::string& command);

// 状态查询
ReplicationState getState() const;
int64_t getReplicationOffset() const;
```

## 复制协议

### 命令格式

复制命令使用简单的序列化格式：

```
长度:命令内容
```

例如：
```
7:SET 1 2
```

### 复制流程

1. **连接阶段**：从节点连接到主节点
2. **握手阶段**：交换复制参数
3. **同步阶段**：从节点同步主节点数据
4. **复制阶段**：主节点持续发送写命令

## 监控和调试

### 日志输出

复制管理器会输出详细的日志信息：

```
[INFO] Initialized as MASTER
[INFO] Master replication started on port 16379
[INFO] Added slave: 127.0.0.1:6380
[INFO] Replicated command to slave: 127.0.0.1:6380
```

### 状态监控

可以通过以下方式监控复制状态：

```cpp
// 获取复制统计信息
const auto& stats = replication_manager->getStats();
std::cout << "Total commands replicated: " << stats.total_commands_replicated << std::endl;
std::cout << "Connected slaves: " << stats.connected_slaves << std::endl;
```

## 故障处理

### 常见问题

1. **从节点连接失败**
   - 检查主节点是否运行
   - 检查网络连接
   - 检查防火墙设置

2. **复制延迟**
   - 检查网络带宽
   - 调整复制缓冲区大小
   - 监控系统负载

3. **数据不一致**
   - 检查复制日志
   - 重新同步从节点
   - 验证命令序列

### 故障恢复

1. **从节点重启**：自动重新连接主节点
2. **主节点重启**：从节点等待主节点恢复
3. **网络中断**：自动重连机制

## 性能优化

### 配置建议

1. **复制缓冲区**：根据数据量调整缓冲区大小
2. **网络优化**：使用专用网络连接
3. **并发控制**：调整复制线程数量

### 监控指标

- 复制延迟
- 复制吞吐量
- 连接状态
- 错误率

## 扩展功能

### 计划中的功能

1. **哨兵模式**：自动故障检测和切换
2. **集群模式**：多主多从架构
3. **读写分离**：从节点处理读请求
4. **数据分片**：水平扩展支持

## 总结

主从复制功能为项目提供了高可用性和数据冗余能力。通过合理的配置和监控，可以构建稳定可靠的分布式存储系统。

## 相关文件

- `replication/replication_manager.h` - 复制管理器头文件
- `replication/replication_manager.cpp` - 复制管理器实现
- `server/redis_handler.h` - Redis处理器（集成复制功能）
- `server/redis_handler.cpp` - Redis处理器实现
- `config/skiplist.conf` - 配置文件
- `replication_test.cpp` - 测试程序 