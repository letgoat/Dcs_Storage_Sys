# SkipList Redis 服务器 - 简洁架构图

## 整体架构

```
┌─────────────────────────────────────────────────────────────┐
│                    SkipList Redis Server                    │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        │
│  │   Clients   │  │   Network   │  │   Server    │        │
│  │             │  │             │  │             │        │
│  │ • Redis CLI │◄►│ • TCP Server│◄►│ • Main Loop │        │
│  │ • Web Apps  │  │ • Thread Pool│  │ • Signal Handler│    │
│  │ • Mobile    │  │ • Connection│  │ • Stats     │        │
│  └─────────────┘  └─────────────┘  └─────────────┘        │
│         │                │                │               │
│         ▼                ▼                ▼               │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        │
│  │   Business  │  │   Storage   │  │   Config    │        │
│  │             │  │             │  │             │        │
│  │ • Redis Handler│ • SkipList  │◄►│ • File Config│        │
│  │ • Command Exec│ • AOF Persist│  │ • Env Vars  │        │
│  │ • Protocol   │ • Data Recovery│ │ • Validation│        │
│  └─────────────┘  └─────────────┘  └─────────────┘        │
│         │                │                │               │
│         ▼                ▼                ▼               │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        │
│  │ Replication │  │   Utils     │  │   Logger    │        │
│  │             │  │             │  │             │        │
│  │ • Master    │  │ • String Utils│ • Log Files │        │
│  │ • Slave     │  │ • File Utils │ • Console    │        │
│  │ • Sync      │  │ • Network    │ • Rotation   │        │
│  └─────────────┘  └─────────────┘  └─────────────┘        │
└─────────────────────────────────────────────────────────────┘
```

## 核心数据流

```
Client Request → TCP Server → Thread Pool → Redis Handler → SkipList → Response
     │              │            │              │            │
     ▼              ▼            ▼              ▼            ▼
   RESP Protocol  Connection  Worker Loop   Command Exec  Data Storage
```

## 技术栈

```
┌─────────────────────────────────────────────────────────────┐
│                    Technology Stack                         │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  Language: C++17 | Modern C++ | RAII | Smart Pointers      │
│  Networking: TCP Sockets | POSIX API | Cross Platform      │
│  Concurrency: Thread Pool | Mutex | Condition Variables    │
│  Storage: SkipList | AOF | File I/O | Data Recovery        │
│  Protocol: RESP | Redis Compatible | Command Parser        │
│  Build: CMake | GCC/Clang | Cross Platform                  │
│  Error Handling: Custom Exceptions | Error Codes | Logging  │
└─────────────────────────────────────────────────────────────┘
```

## 性能指标

- **并发连接**: 1000+
- **查询复杂度**: O(log n)
- **内存管理**: RAII + Smart Pointers
- **数据安全**: AOF + Master-Slave Replication
- **可扩展性**: Modular Design

## 核心特性

1. **Redis协议兼容** - 支持标准RESP协议
2. **高性能存储** - 18层SkipList实现
3. **高并发处理** - 多线程TCP服务器
4. **数据持久化** - AOF机制保证数据安全
5. **主从复制** - 支持分布式部署
6. **优雅关闭** - 信号处理和资源清理
7. **监控统计** - 实时性能指标
8. **跨平台** - Windows/Linux支持 