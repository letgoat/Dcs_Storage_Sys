# SkipList Redis 服务器架构图

## 整体架构概览

```
┌─────────────────────────────────────────────────────────────────┐
│                        SkipList Redis Server                    │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────────┐    ┌─────────────────┐    ┌──────────────┐ │
│  │   Client Layer  │    │   Network Layer │    │  Server Core │ │
│  │                 │    │                 │    │              │ │
│  │ • Redis Clients │◄──►│ • TCP Server    │◄──►│ • Main Server│ │
│  │ • RESP Protocol │    │ • Connection Mgmt│    │ • Signal Handler│
│  │ • Command Parser│    │ • Thread Pool   │    │ • Stats Monitor│
│  └─────────────────┘    └─────────────────┘    └──────────────┘ │
│           │                       │                       │     │
│           ▼                       ▼                       ▼     │
│  ┌─────────────────┐    ┌─────────────────┐    ┌──────────────┐ │
│  │  Business Layer │    │   Storage Layer │    │  Config Layer│ │
│  │                 │    │                 │    │              │ │
│  │ • Redis Handler │◄──►│ • SkipList      │◄──►│ • Config Mgmt│ │
│  │ • Command Exec  │    │ • AOF Persistence│   │ • Env Vars   │ │
│  │ • Protocol Gen  │    │ • Data Recovery │    │ • File Config│ │
│  └─────────────────┘    └─────────────────┘    └──────────────┘ │
│           │                       │                       │     │
│           ▼                       ▼                       ▼     │
│  ┌─────────────────┐    ┌─────────────────┐    ┌──────────────┐ │
│  │ Replication Layer│   │   Utility Layer │    │  Log Layer   │ │
│  │                 │    │                 │    │              │ │
│  │ • Master/Slave  │    │ • String Utils  │    │ • Logger     │ │
│  │ • Sync Protocol │    │ • File Utils    │    │ • Log Rotation│
│  │ • State Mgmt    │    │ • Network Utils │    │ • Console Out│ │
│  └─────────────────┘    └─────────────────┘    └──────────────┘ │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

## 详细模块架构

### 1. 网络层 (Network Layer)
```
┌─────────────────────────────────────────────────────────────┐
│                    Network Layer                            │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────────┐    ┌─────────────────┐                │
│  │   TCPServer     │    │ ClientConnection│                │
│  │                 │    │                 │                │
│  │ • Socket Mgmt   │◄──►│ • Socket Wrapper│                │
│  │ • Accept Loop   │    │ • Send/Receive  │                │
│  │ • Thread Pool   │    │ • Connection State│              │
│  │ • Message Queue │    │ • Client Address│                │
│  └─────────────────┘    └─────────────────┘                │
│           │                       │                        │
│           ▼                       ▼                        │
│  ┌─────────────────┐    ┌─────────────────┐                │
│  │   Worker Loop   │    │  Message Handler│                │
│  │                 │    │                 │                │
│  │ • Task Processing│   │ • Command Routing│               │
│  │ • Response Gen  │    │ • Error Handling│                │
│  │ • Thread Safety │    │ • Protocol Parse│                │
│  └─────────────────┘    └─────────────────┘                │
└─────────────────────────────────────────────────────────────┘
```

### 2. 业务层 (Business Layer)
```
┌─────────────────────────────────────────────────────────────┐
│                   Business Layer                            │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────────┐    ┌─────────────────┐                │
│  │  RedisHandler   │    │ RedisProtocol   │                │
│  │                 │    │                 │                │
│  │ • Command Exec  │◄──►│ • RESP Parser   │                │
│  │ • Data Mgmt     │    │ • Response Gen  │                │
│  │ • AOF Handler   │    │ • Protocol Utils│                │
│  │ • Stats Tracking│    │ • Error Response│                │
│  └─────────────────┘    └─────────────────┘                │
│           │                       │                        │
│           ▼                       ▼                        │
│  ┌─────────────────┐    ┌─────────────────┐                │
│  │  Command Router │    │  Response Cache │                │
│  │                 │    │                 │                │
│  │ • GET/SET/DEL   │    │ • Success Resp  │                │
│  │ • PING/ECHO     │    │ • Error Resp    │                │
│  │ • INFO/SAVE     │    │ • Bulk Response │                │
│  └─────────────────┘    └─────────────────┘                │
└─────────────────────────────────────────────────────────────┘
```

### 3. 存储层 (Storage Layer)
```
┌─────────────────────────────────────────────────────────────┐
│                   Storage Layer                             │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────────┐    ┌─────────────────┐                │
│  │    SkipList     │    │   Node System   │                │
│  │                 │    │                 │                │
│  │ • 18 Levels     │◄──►│ • SkipListNode  │                │
│  │ • O(log n)      │    │ • Forward Array │                │
│  │ • Insert/Delete │    │ • Key/Value     │                │
│  │ • Search        │    │ • Level Mgmt    │                │
│  └─────────────────┘    └─────────────────┘                │
│           │                       │                        │
│           ▼                       ▼                        │
│  ┌─────────────────┐    ┌─────────────────┐                │
│  │   AOF System    │    │  Data Recovery  │                │
│  │                 │    │                 │                │
│  │ • Append Only   │    │ • Startup Load  │                │
│  │ • Fsync Policy  │    │ • Data Validate │                │
│  │ • Command Log   │    │ • Corruption Check│              │
│  └─────────────────┘    └─────────────────┘                │
└─────────────────────────────────────────────────────────────┘
```

### 4. 复制层 (Replication Layer)
```
┌─────────────────────────────────────────────────────────────┐
│                 Replication Layer                           │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────────┐    ┌─────────────────┐                │
│  │ ReplicationMgr  │    │  Master Role    │                │
│  │                 │    │                 │                │
│  │ • Role Mgmt     │◄──►│ • Command Repl  │                │
│  │ • State Control │    │ • Slave Mgmt    │                │
│  │ • Sync Protocol │    │ • Offset Track  │                │
│  └─────────────────┘    └─────────────────┘                │
│           │                       │                        │
│           ▼                       ▼                        │
│  ┌─────────────────┐    ┌─────────────────┐                │
│  │   Slave Role    │    │  Sync Protocol  │                │
│  │                 │    │                 │                │
│  │ • Master Connect│    │ • PSYNC Command │                │
│  │ • Command Apply │    │ • Offset Sync   │                │
│  │ • State Sync    │    │ • Data Transfer │                │
│  └─────────────────┘    └─────────────────┘                │
└─────────────────────────────────────────────────────────────┘
```

## 数据流图

### 1. 客户端请求处理流程
```
Client Request
      │
      ▼
┌─────────────┐
│ TCP Server  │ ── Accept Connection
└─────────────┘
      │
      ▼
┌─────────────┐
│ Message Queue│ ── Add to Worker Queue
└─────────────┘
      │
      ▼
┌─────────────┐
│ Worker Loop │ ── Process Task
└─────────────┘
      │
      ▼
┌─────────────┐
│Redis Protocol│ ── Parse RESP
└─────────────┘
      │
      ▼
┌─────────────┐
│Redis Handler│ ── Execute Command
└─────────────┘
      │
      ▼
┌─────────────┐
│  SkipList   │ ── Data Operation
└─────────────┘
      │
      ▼
┌─────────────┐
│ Response Gen│ ── Generate Response
└─────────────┘
      │
      ▼
┌─────────────┐
│ TCP Client  │ ── Send Response
└─────────────┘
```

### 2. 数据持久化流程
```
Write Command
      │
      ▼
┌─────────────┐
│Redis Handler│ ── Process Command
└─────────────┘
      │
      ▼
┌─────────────┐
│  SkipList   │ ── Update Data
└─────────────┘
      │
      ▼
┌─────────────┐
│ AOF System  │ ── Append Command
└─────────────┘
      │
      ▼
┌─────────────┐
│ Fsync Policy│ ── Flush to Disk
└─────────────┘
```

### 3. 主从复制流程
```
Master Write
      │
      ▼
┌─────────────┐
│Master Role  │ ── Execute Command
└─────────────┘
      │
      ▼
┌─────────────┐
│ Command Repl│ ── Send to Slaves
└─────────────┘
      │
      ▼
┌─────────────┐
│ Slave Role  │ ── Receive Command
└─────────────┘
      │
      ▼
┌─────────────┐
│ Command Apply│ ── Apply to Local
└─────────────┘
```

## 技术栈架构

```
┌─────────────────────────────────────────────────────────────┐
│                    Technology Stack                         │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────────┐    ┌─────────────────┐                │
│  │   Language      │    │   Libraries     │                │
│  │                 │    │                 │                │
│  │ • C++17         │    │ • STL           │                │
│  │ • Modern C++    │    │ • Threading     │                │
│  │ • RAII          │    │ • Filesystem    │                │
│  │ • Smart Pointers│    │ • Chrono        │                │
│  └─────────────────┘    └─────────────────┘                │
│           │                       │                        │
│           ▼                       ▼                        │
│  ┌─────────────────┐    ┌─────────────────┐                │
│  │   Networking    │    │   Build System  │                │
│  │                 │    │                 │                │
│  │ • TCP Sockets   │    │ • CMake         │                │
│  │ • POSIX API     │    │ • Make          │                │
│  │ • Cross Platform│    │ • GCC/Clang     │                │
│  └─────────────────┘    └─────────────────┘                │
│           │                       │                        │
│           ▼                       ▼                        │
│  ┌─────────────────┐    ┌─────────────────┐                │
│  │   Concurrency   │    │   Error Handling│                │
│  │                 │    │                 │                │
│  │ • Thread Pool   │    │ • Custom Exceptions│             │
│  │ • Mutex/Locks   │    │ • Exception Hierarchy│           │
│  │ • Condition Vars│    │ • Error Codes   │                │
│  └─────────────────┘    └─────────────────┘                │
└─────────────────────────────────────────────────────────────┘
```

## 部署架构

```
┌─────────────────────────────────────────────────────────────┐
│                    Deployment Architecture                   │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────────┐    ┌─────────────────┐                │
│  │   Client Apps   │    │   Load Balancer │                │
│  │                 │    │                 │                │
│  │ • Redis Clients │◄──►│ • Nginx/HAProxy │                │
│  │ • Web Apps      │    │ • Health Check  │                │
│  │ • Mobile Apps   │    │ • Failover      │                │
│  └─────────────────┘    └─────────────────┘                │
│           │                       │                        │
│           ▼                       ▼                        │
│  ┌─────────────────┐    ┌─────────────────┐                │
│  │  Master Server  │    │  Slave Servers  │                │
│  │                 │    │                 │                │
│  │ • Primary Node  │    │ • Replica Nodes │                │
│  │ • Write Access  │    │ • Read Access   │                │
│  │ • Data Sync     │    │ • Failover      │                │
│  └─────────────────┘    └─────────────────┘                │
│           │                       │                        │
│           ▼                       ▼                        │
│  ┌─────────────────┐    ┌─────────────────┐                │
│  │  Storage Layer  │    │  Monitoring     │                │
│  │                 │    │                 │                │
│  │ • AOF Files     │    │ • Metrics       │                │
│  │ • Data Files    │    │ • Logs          │                │
│  │ • Config Files  │    │ • Alerts        │                │
│  └─────────────────┘    └─────────────────┘                │
└─────────────────────────────────────────────────────────────┘
```

## 性能特性

- **并发处理**：支持1000+并发连接
- **响应时间**：O(log n)查询复杂度
- **内存效率**：智能指针管理，RAII资源管理
- **数据安全**：AOF持久化，主从复制
- **可扩展性**：模块化设计，易于扩展新功能 