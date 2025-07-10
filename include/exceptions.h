#pragma once

#include <exception>
#include <string>
#include <stdexcept>

namespace skiplist {

// 基础异常类
class SkipListException : public std::exception {
private:
    std::string message_;
    std::string error_code_;
    
public:
    SkipListException(const std::string& message, const std::string& error_code = "UNKNOWN")
        : message_(message), error_code_(error_code) {}
    
    const char* what() const noexcept override {
        return message_.c_str();
    }
    
    const std::string& getErrorCode() const {
        return error_code_;
    }
    
    const std::string& getMessage() const {
        return message_;
    }
};

// 配置相关异常
class ConfigException : public SkipListException {
public:
    ConfigException(const std::string& message, const std::string& error_code = "CONFIG_ERROR")
        : SkipListException(message, error_code) {}
};

class ConfigFileNotFoundException : public ConfigException {
public:
    ConfigFileNotFoundException(const std::string& filename)
        : ConfigException("Configuration file not found: " + filename, "CONFIG_FILE_NOT_FOUND") {}
};

class ConfigParseException : public ConfigException {
public:
    ConfigParseException(const std::string& message)
        : ConfigException("Configuration parse error: " + message, "CONFIG_PARSE_ERROR") {}
};

// 网络相关异常
class NetworkException : public SkipListException {
public:
    NetworkException(const std::string& message, const std::string& error_code = "NETWORK_ERROR")
        : SkipListException(message, error_code) {}
};

class ConnectionException : public NetworkException {
public:
    ConnectionException(const std::string& message)
        : NetworkException("Connection error: " + message, "CONNECTION_ERROR") {}
};

class SocketException : public NetworkException {
public:
    SocketException(const std::string& message)
        : NetworkException("Socket error: " + message, "SOCKET_ERROR") {}
};

class BindException : public NetworkException {
public:
    BindException(const std::string& address, int port)
        : NetworkException("Failed to bind to " + address + ":" + std::to_string(port), "BIND_ERROR") {}
};

// 数据存储相关异常
class StorageException : public SkipListException {
public:
    StorageException(const std::string& message, const std::string& error_code = "STORAGE_ERROR")
        : SkipListException(message, error_code) {}
};

class FileIOException : public StorageException {
public:
    FileIOException(const std::string& filename, const std::string& operation)
        : StorageException("File I/O error during " + operation + " on " + filename, "FILE_IO_ERROR") {}
};

class DataCorruptionException : public StorageException {
public:
    DataCorruptionException(const std::string& message)
        : StorageException("Data corruption detected: " + message, "DATA_CORRUPTION") {}
};

// Redis协议相关异常
class RedisProtocolException : public SkipListException {
public:
    RedisProtocolException(const std::string& message, const std::string& error_code = "REDIS_PROTOCOL_ERROR")
        : SkipListException(message, error_code) {}
};

class InvalidCommandException : public RedisProtocolException {
public:
    InvalidCommandException(const std::string& command)
        : RedisProtocolException("Invalid Redis command: " + command, "INVALID_COMMAND") {}
};

class ProtocolParseException : public RedisProtocolException {
public:
    ProtocolParseException(const std::string& message)
        : RedisProtocolException("Protocol parse error: " + message, "PROTOCOL_PARSE_ERROR") {}
};

// 复制相关异常
class ReplicationException : public SkipListException {
public:
    ReplicationException(const std::string& message, const std::string& error_code = "REPLICATION_ERROR")
        : SkipListException(message, error_code) {}
};

class MasterConnectionException : public ReplicationException {
public:
    MasterConnectionException(const std::string& master_address)
        : ReplicationException("Failed to connect to master: " + master_address, "MASTER_CONNECTION_ERROR") {}
};

class SyncException : public ReplicationException {
public:
    SyncException(const std::string& message)
        : ReplicationException("Sync error: " + message, "SYNC_ERROR") {}
};

// 内存相关异常
class MemoryException : public SkipListException {
public:
    MemoryException(const std::string& message, const std::string& error_code = "MEMORY_ERROR")
        : SkipListException(message, error_code) {}
};

class OutOfMemoryException : public MemoryException {
public:
    OutOfMemoryException(const std::string& operation)
        : MemoryException("Out of memory during " + operation, "OUT_OF_MEMORY") {}
};

// 线程相关异常
class ThreadException : public SkipListException {
public:
    ThreadException(const std::string& message, const std::string& error_code = "THREAD_ERROR")
        : SkipListException(message, error_code) {}
};

class ThreadCreationException : public ThreadException {
public:
    ThreadCreationException(const std::string& thread_name)
        : ThreadException("Failed to create thread: " + thread_name, "THREAD_CREATION_ERROR") {}
};

// 日志相关异常
class LoggingException : public SkipListException {
public:
    LoggingException(const std::string& message, const std::string& error_code = "LOGGING_ERROR")
        : SkipListException(message, error_code) {}
};

class LogFileException : public LoggingException {
public:
    LogFileException(const std::string& log_file)
        : LoggingException("Log file error: " + log_file, "LOG_FILE_ERROR") {}
};

// 工具函数：异常信息格式化
namespace ExceptionUtils {
    inline std::string formatExceptionInfo(const std::exception& e, const std::string& context = "") {
        std::string info = "Exception: " + std::string(e.what());
        if (!context.empty()) {
            info = "Context: " + context + " - " + info;
        }
        return info;
    }
    
    inline std::string formatExceptionWithCode(const SkipListException& e, const std::string& context = "") {
        std::string info = "Error Code: " + e.getErrorCode() + " - " + e.getMessage();
        if (!context.empty()) {
            info = "Context: " + context + " - " + info;
        }
        return info;
    }
}

} // namespace skiplist 