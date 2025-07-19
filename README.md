# SkipList Redis Server

一个基于跳表数据结构的高性能内存数据库服务器，兼容Redis协议。

## 项目特性

### 🚀 核心功能
- **跳表数据结构**: 实现高效的O(log n)时间复杂度操作
- **Redis协议兼容**: 支持RESP协议，可与Redis客户端兼容
- **多线程网络服务器**: 高并发处理能力
- **数据持久化**: 支持数据保存和恢复（RDB快照+新增AOF持久化）
- **AOF持久化**: 写操作实时追加日志，重启可恢复全部数据，兼容Redis机制
- **配置管理**: 灵活的配置系统，支持文件和环境变量

### 🔧 技术特性
- **C++17**: 现代C++特性
- **多线程**: 线程池处理客户端连接
- **网络编程**: 原生Socket编程
- **日志系统**: 分级日志记录和文件轮转
- **性能监控**: 实时性能统计
- **优雅关闭**: 信号处理和资源清理

### 📊 性能特性
- **高并发**: 支持1000+并发连接
- **低延迟**: 跳表数据结构提供快速查询
- **内存效率**: 优化的内存使用
- **可扩展**: 模块化设计，易于扩展

## 快速开始

### 编译项目

```bash
# 创建构建目录
mkdir build && cd build

# 配置CMake
cmake ..

# 编译
make -j$(nproc)

# 运行服务器
./bin/SkipListProject
```

### 基本使用

```bash
# 启动服务器（默认端口6379）
./bin/SkipListProject

# 指定端口启动
./bin/SkipListProject -p 6380

# 使用配置文件启动
./bin/SkipListProject -c config/skiplist.conf

# 启用调试日志
./bin/SkipListProject -l DEBUG

# 查看帮助
./bin/SkipListProject --help

# 查看版本信息
./bin/SkipListProject --version
```

### 使用Redis客户端连接

```bash
# 使用redis-cli连接
redis-cli -p 6379

# 测试基本命令
127.0.0.1:6379> PING
PONG
127.0.0.1:6379> SET 1 "hello"
OK
127.0.0.1:6379> GET 1
"hello"
127.0.0.1:6379> EXISTS 1
(integer) 1
127.0.0.1:6379> DEL 1
(integer) 1
127.0.0.1:6379> INFO
# Server
redis_version:1.0.0
...
```

## 支持的Redis命令

### 基本命令
- `PING` - 测试连接
- `ECHO <message>` - 回显消息
- `QUIT` - 断开连接

### 数据操作
- `SET <key> <value>` - 设置键值对
- `GET <key>` - 获取值
- `DEL <key>` - 删除键
- `EXISTS <key>` - 检查键是否存在

### 数据库管理
- `SAVE` - 保存数据到文件
- `LOAD` - 从文件加载数据
- `FLUSH` - 清空数据库
- `SELECT <db>` - 选择数据库（0-15）

### 信息查询
- `INFO` - 获取服务器信息
- `CONFIG GET <parameter>` - 获取配置参数

## 配置说明

### 配置文件格式

配置文件使用INI格式，支持以下配置段：

```ini
[Server]
host=0.0.0.0
port=6379
max_connections=1000
thread_pool_size=4

[SkipList]
max_level=18
data_file=store/dumpFile
enable_persistence=true
persistence_interval=60

[Log]
log_level=INFO
log_file=logs/skiplist.log
enable_console=true
max_file_size=104857600
max_files=10

[AOF]
# 启用AOF持久化
enable_aof=true
# AOF文件路径
aof_file=store/appendonly.aof
# AOF同步策略: always, everysec, no
aof_fsync=everysec
# AOF同步间隔（秒，everysec时生效）
aof_fsync_interval=1
```

### 环境变量

支持通过环境变量配置：

```bash
export SKIPLIST_PORT=6379
export SKIPLIST_HOST=0.0.0.0
export SKIPLIST_MAX_CONNECTIONS=1000
export SKIPLIST_THREAD_POOL_SIZE=4
export SKIPLIST_MAX_LEVEL=18
export SKIPLIST_LOG_LEVEL=INFO
export SKIPLIST_LOG_FILE=logs/skiplist.log
```

## 性能测试

### 基准测试

项目包含内置性能测试：

```bash
# 运行性能测试
./bin/SkipListProject --test
```

测试结果示例：
```
=== Performance Test ===
Testing with 10000 elements...
Insert 10000 elements: 45ms
Search 10000 elements: 12ms
Delete 10000 elements: 38ms
Performance test completed.
```

## 🤝 贡献

欢迎提交Issue和Pull Request！

1. Fork 项目
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 打开Pull Request


⭐ 如果这个项目对您有帮助，请给它一个星标！