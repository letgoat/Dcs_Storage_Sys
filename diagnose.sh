#!/bin/bash

echo "=== SkipListProject 诊断脚本 ==="
echo

# 1. 检查可执行文件
echo "1. 检查可执行文件..."
if [ -f "./bin/SkipListProject" ]; then
    echo "   ✓ 可执行文件存在"
    ls -la ./bin/SkipListProject
else
    echo "   ✗ 可执行文件不存在"
    exit 1
fi
echo

# 2. 检查依赖库
echo "2. 检查依赖库..."
ldd ./bin/SkipListProject
echo

# 3. 检查目录权限
echo "3. 检查目录权限..."
echo "   当前目录: $(pwd)"
echo "   logs目录: $(ls -la logs/)"
echo "   store目录: $(ls -la store/)"
echo

# 4. 检查端口占用
echo "4. 检查端口占用..."
ss -tulpn | grep 6379 || echo "   端口6379未被占用"
echo

# 5. 检查配置文件
echo "5. 检查配置文件..."
if [ -f "./config/skiplist.conf" ]; then
    echo "   ✓ 配置文件存在"
    echo "   配置文件内容预览:"
    head -10 ./config/skiplist.conf
else
    echo "   ✗ 配置文件不存在"
fi
echo

# 6. 尝试启动服务器（短暂运行）
echo "6. 尝试启动服务器（5秒测试）..."
timeout 5s ./bin/SkipListProject -p 6379 -l DEBUG &
SERVER_PID=$!
sleep 2

# 检查进程是否还在运行
if kill -0 $SERVER_PID 2>/dev/null; then
    echo "   ✓ 服务器进程正在运行 (PID: $SERVER_PID)"
    
    # 检查端口是否被监听
    if ss -tulpn | grep 6379 > /dev/null; then
        echo "   ✓ 端口6379正在被监听"
    else
        echo "   ✗ 端口6379未被监听"
    fi
    
    # 停止服务器
    kill $SERVER_PID 2>/dev/null
    wait $SERVER_PID 2>/dev/null
else
    echo "   ✗ 服务器进程已退出"
fi
echo

# 7. 检查日志文件
echo "7. 检查日志文件..."
if [ -f "./logs/skiplist.log" ]; then
    echo "   日志文件大小: $(wc -c < ./logs/skiplist.log) 字节"
    if [ -s "./logs/skiplist.log" ]; then
        echo "   日志文件内容:"
        cat ./logs/skiplist.log
    else
        echo "   日志文件为空"
    fi
else
    echo "   日志文件不存在"
fi
echo

echo "=== 诊断完成 ==="
echo
echo "建议的解决方案:"
echo "1. 如果服务器无法启动，检查是否有权限问题"
echo "2. 如果端口无法绑定，检查是否有其他程序占用端口"
echo "3. 如果日志为空，可能是日志系统初始化失败"
echo "4. 尝试使用不同的端口: ./bin/SkipListProject -p 6380"
echo "5. 尝试使用配置文件: ./bin/SkipListProject -c config/skiplist.conf" 