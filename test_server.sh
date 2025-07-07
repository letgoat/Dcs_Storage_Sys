#!/bin/bash

echo "=== SkipList Redis Server 使用指南 ==="
echo

# 1. 启动服务器
echo "1. 启动服务器..."
echo "   命令: ./bin/SkipListProject -c config/skiplist.conf -l DEBUG"
echo "   这将启动服务器，监听端口6379"
echo

# 2. 测试基本命令
echo "2. 测试基本Redis命令..."
echo "   使用redis-cli连接服务器:"
echo "   redis-cli -p 6379"
echo

echo "3. 支持的Redis命令:"
echo "   PING - 测试连接"
echo "   SET <key> <value> - 设置键值对"
echo "   GET <key> - 获取值"
echo "   DEL <key> - 删除键"
echo "   EXISTS <key> - 检查键是否存在"
echo "   KEYS - 列出所有键"
echo "   FLUSH - 清空数据库"
echo "   SAVE - 保存数据到文件"
echo "   LOAD - 从文件加载数据"
echo "   INFO - 获取服务器信息"
echo "   CONFIG GET <parameter> - 获取配置参数"
echo "   SELECT <db> - 选择数据库（0-15）"
echo "   AUTH <password> - 认证（如果启用）"
echo "   QUIT - 断开连接"
echo

echo "4. 示例操作:"
echo "   redis-cli -p 6379"
echo "   127.0.0.1:6379> PING"
echo "   127.0.0.1:6379> SET 1 \"hello world\""
echo "   127.0.0.1:6379> GET 1"
echo "   127.0.0.1:6379> EXISTS 1"
echo "   127.0.0.1:6379> INFO"
echo "   127.0.0.1:6379> QUIT"
echo

echo "5. 性能测试:"
echo "   ./bin/SkipListProject --test"
echo "   这将运行内置的性能测试"
echo

echo "6. 查看日志:"
echo "   tail -f logs/skiplist.log"
echo

echo "7. 停止服务器:"
echo "   Ctrl+C 或 pkill -f SkipListProject"
echo

echo "=== 项目特性 ==="
echo "- 基于跳表数据结构的高性能内存数据库"
echo "- 兼容Redis协议（RESP）"
echo "- 支持数据持久化（RDB + AOF）"
echo "- 多线程网络服务器"
echo "- 可配置的日志系统"
echo "- 性能监控和统计"
echo

echo "=== 配置文件 ==="
echo "配置文件位置: config/skiplist.conf"
echo "支持通过命令行参数、配置文件或环境变量进行配置"
echo

echo "=== 数据存储 ==="
echo "数据文件: store/dumpFile"
echo "AOF文件: store/appendonly.aof"
echo "日志文件: logs/skiplist.log"
echo

echo "现在你可以启动服务器并开始测试了！" 