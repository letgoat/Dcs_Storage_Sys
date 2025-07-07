# SkipListProject 使用指南

## 🚀 项目概述

SkipListProject 是一个基于跳表数据结构的 Redis 兼容服务器，支持基本的 Redis 命令和协议。

## 📋 系统要求

- Linux 系统（推荐 Ubuntu 20.04+）
- GCC 7.0+ 或 Clang 5.0+
- CMake 3.10+
- 至少 100MB 可用内存
- 至少 50MB 可用磁盘空间

## 🔧 编译项目

```bash
# 1. 创建构建目录
mkdir -p build
cd build

# 2. 运行 CMake 配置
cmake ..

# 3. 编译项目
make -j4

# 4. 检查编译结果
ls -la ../bin/
```

## 🏃‍♂️ 运行服务器

### 基本启动

```bash
# 使用默认配置启动
./bin/SkipListProject

# 指定端口启动
./bin/SkipListProject -p 6379

# 指定配置文件和日志级别
./bin/SkipListProject -c config/skiplist.conf -l DEBUG
```

### 启动参数

- `-c, --config <file>`: 指定配置文件路径
- `-p, --port <port>`: 指定服务器端口
- `-h, --host <host>`: 指定服务器主机地址
- `-l, --log-level <level>`: 指定日志级别 (DEBUG, INFO, WARN, ERROR)
- `--help`: 显示帮助信息
- `--version`: 显示版本信息

### 配置文件示例

```ini
# config/skiplist.conf
[Server]
host=0.0.0.0
port=6379
max_connections=1000
thread_pool_size=4

[SkipList]
max_level=16
data_file=store/skiplist.dat
enable_persistence=true
persistence_interval=60

[Log]
log_level=INFO
log_file=logs/skiplist.log
enable_console=true
max_file_size=10485760
max_files=5
```

## 🔌 连接和测试

### 使用 redis-cli 连接

```bash
# 连接到服务器
redis-cli -p 6379

# 测试连接
redis-cli -p 6379 ping

# 执行命令
redis-cli -p 6379 set mykey "Hello World"
redis-cli -p 6379 get mykey
```

### 使用 telnet 连接

```bash
# 连接到服务器
telnet 127.0.0.1 6379

# 发送 Redis 命令
PING
SET test "Hello"
GET test
```

## 📝 支持的 Redis 命令

### 基本命令
- `PING` - 测试连接
- `SET <key> <value>` - 设置键值对
- `GET <key>` - 获取值
- `DEL <key>` - 删除键
- `EXISTS <key>` - 检查键是否存在
- `KEYS` - 列出所有键
- `FLUSH` - 清空数据库

### 数据管理
- `SAVE` - 保存数据到文件
- `LOAD` - 从文件加载数据
- `INFO` - 获取服务器信息

### 配置管理
- `CONFIG GET <parameter>` - 获取配置参数
- `SELECT <db>` - 选择数据库（0-15）
- `AUTH <password>` - 认证（如果启用）

### 连接管理
- `QUIT` - 断开连接

## 📊 监控和管理

### 查看服务器状态

```bash
# 查看进程
ps aux | grep SkipListProject

# 查看端口监听
ss -tulpn | grep 6379

# 查看日志
tail -f logs/skiplist.log
```

### 性能监控

服务器会定期输出统计信息：
- 运行时间
- 当前连接数
- 总命令数
- 内存使用情况

## 🛠️ 故障排除

### 常见问题

1. **服务器无法启动**
   ```bash
   # 检查依赖库
   ldd bin/SkipListProject
   
   # 检查权限
   ls -la bin/SkipListProject
   
   # 检查端口占用
   ss -tulpn | grep 6379
   ```

2. **无法连接服务器**
   ```bash
   # 检查服务器是否运行
   ps aux | grep SkipListProject
   
   # 检查防火墙设置
   sudo ufw status
   
   # 尝试不同端口
   ./bin/SkipListProject -p 6380
   ```

3. **日志文件为空**
   ```bash
   # 检查日志目录权限
   ls -la logs/
   
   # 使用 DEBUG 级别启动
   ./bin/SkipListProject -l DEBUG
   ```

### 诊断脚本

项目包含诊断脚本 `diagnose.sh`：

```bash
# 运行诊断
./diagnose.sh
```

## 📁 目录结构

```
skiplist_cpp/
├── bin/                    # 可执行文件
├── build/                  # 构建文件
├── config/                 # 配置文件
├── logs/                   # 日志文件
├── store/                  # 数据存储
├── src/                    # 源代码
│   ├── node/              # 节点实现
│   ├── skiplist/          # 跳表实现
│   ├── network/           # 网络模块
│   ├── server/            # 服务器模块
│   ├── logger/            # 日志模块
│   ├── config/            # 配置模块
│   └── utils/             # 工具模块
└── test/                  # 测试文件
```

## 🔒 安全考虑

1. **网络安全**
   - 默认监听所有接口 (0.0.0.0)
   - 建议在生产环境中限制访问
   - 考虑使用防火墙规则

2. **数据安全**
   - 数据文件存储在 `store/` 目录
   - 定期备份数据文件
   - 设置适当的文件权限

3. **日志安全**
   - 日志文件可能包含敏感信息
   - 定期轮转和清理日志
   - 设置适当的日志文件权限

## 🚀 性能优化

1. **内存优化**
   - 调整跳表最大层数
   - 监控内存使用情况
   - 定期清理无用数据

2. **网络优化**
   - 调整线程池大小
   - 优化连接池配置
   - 使用适当的缓冲区大小

3. **磁盘优化**
   - 使用 SSD 存储
   - 调整持久化间隔
   - 优化文件 I/O 操作

## 📞 支持和反馈

如果遇到问题或有建议，请：

1. 查看日志文件 `logs/skiplist.log`
2. 运行诊断脚本 `./diagnose.sh`
3. 检查配置文件是否正确
4. 尝试使用不同的参数启动

## 📄 许可证

本项目采用 MIT 许可证，详见 LICENSE 文件。 