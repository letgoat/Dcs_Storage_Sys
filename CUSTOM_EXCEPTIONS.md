# 自定义异常类使用说明

## 概述

本项目实现了完整的自定义异常类体系，提供了类型安全、信息丰富的异常处理机制。

## 异常类层次结构

```
std::exception
└── skiplist::SkipListException (基础异常类)
    ├── ConfigException (配置相关)
    │   ├── ConfigFileNotFoundException
    │   └── ConfigParseException
    ├── NetworkException (网络相关)
    │   ├── ConnectionException
    │   ├── SocketException
    │   └── BindException
    ├── StorageException (存储相关)
    │   ├── FileIOException
    │   └── DataCorruptionException
    ├── RedisProtocolException (Redis协议相关)
    │   ├── InvalidCommandException
    │   └── ProtocolParseException
    ├── ReplicationException (复制相关)
    │   ├── MasterConnectionException
    │   └── SyncException
    ├── MemoryException (内存相关)
    │   └── OutOfMemoryException
    ├── ThreadException (线程相关)
    │   └── ThreadCreationException
    └── LoggingException (日志相关)
        └── LogFileException
```

## 主要特性

### 1. 错误代码支持
每个异常都包含错误代码，便于程序化处理：
```cpp
try {
    // 可能抛出异常的代码
} catch (const skiplist::ConfigFileNotFoundException& e) {
    std::cout << "错误代码: " << e.getErrorCode() << std::endl;
    std::cout << "错误信息: " << e.getMessage() << std::endl;
}
```

### 2. 异常工具函数
提供格式化异常信息的工具函数：
```cpp
#include "include/exceptions.h"

try {
    // 代码
} catch (const skiplist::SkipListException& e) {
    std::string info = skiplist::ExceptionUtils::formatExceptionWithCode(e, "函数名");
    std::cout << info << std::endl;
}
```

### 3. 分层异常处理
支持按异常类型进行分层处理：
```cpp
try {
    // 代码
} catch (const skiplist::ConfigException& e) {
    // 处理配置相关异常
} catch (const skiplist::NetworkException& e) {
    // 处理网络相关异常
} catch (const skiplist::SkipListException& e) {
    // 处理所有自定义异常
} catch (const std::exception& e) {
    // 处理标准异常
}
```

## 使用示例

### 配置加载
```cpp
#include "include/exceptions.h"

void loadConfiguration(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw skiplist::ConfigFileNotFoundException(filename);
        }
        // 处理配置文件
    } catch (const skiplist::ConfigException& e) {
        std::cerr << "配置错误: " << e.what() << std::endl;
        throw; // 重新抛出
    }
}
```

### 网络操作
```cpp
void bindServer(const std::string& address, int port) {
    try {
        // 绑定操作
        if (bind(socket, addr, len) < 0) {
            throw skiplist::BindException(address, port);
        }
    } catch (const skiplist::NetworkException& e) {
        std::cerr << "网络错误: " << e.what() << std::endl;
        throw;
    }
}
```

### 文件操作
```cpp
void saveData(const std::string& filename, const std::string& data) {
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw skiplist::FileIOException(filename, "write");
        }
        file << data;
    } catch (const skiplist::StorageException& e) {
        std::cerr << "存储错误: " << e.what() << std::endl;
        throw;
    }
}
```

## 编译和测试

### 编译
```bash
g++ -std=c++17 -I. test_exception_integration.cpp -o test_exception_integration
```

### 运行测试
```bash
./test_exception_integration
```

## 集成到现有项目

1. 包含头文件：
```cpp
#include "include/exceptions.h"
```

2. 替换现有的错误处理：
```cpp
// 旧方式
if (!file.is_open()) {
    std::cerr << "Failed to open file" << std::endl;
    return false;
}

// 新方式
if (!file.is_open()) {
    throw skiplist::FileIOException(filename, "read");
}
```

3. 添加异常处理：
```cpp
try {
    // 可能抛出异常的代码
} catch (const skiplist::SkipListException& e) {
    std::cerr << skiplist::ExceptionUtils::formatExceptionWithCode(e, "函数名") << std::endl;
    return false;
}
```

## 优势

1. **类型安全**：编译时检查异常类型
2. **信息丰富**：包含错误代码和详细消息
3. **层次清晰**：按功能模块分类异常
4. **易于扩展**：可以轻松添加新的异常类型
5. **调试友好**：提供格式化工具函数
6. **标准兼容**：继承自 std::exception

## 注意事项

1. 异常处理应该在最合适的层次进行
2. 避免在析构函数中抛出异常
3. 确保异常安全（RAII原则）
4. 合理使用异常层次结构进行捕获 