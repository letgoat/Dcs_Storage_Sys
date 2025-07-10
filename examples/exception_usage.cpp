#include "../include/exceptions.h"
#include <iostream>
#include <fstream>
#include <memory>
#include <thread>
#include <vector>
#include <sstream>
#include <algorithm>

using namespace skiplist;

// 示例1：配置加载中的异常处理
class ConfigLoader {
public:
    void loadConfig(const std::string& filename) {
        try {
            std::ifstream file(filename);
            if (!file.is_open()) {
                throw ConfigFileNotFoundException(filename);
            }
            
            std::string line;
            while (std::getline(file, line)) {
                if (line.empty() || line[0] == '#') {
                    continue;
                }
                
                size_t pos = line.find('=');
                if (pos == std::string::npos) {
                    throw ConfigParseException("Invalid config line format: " + line);
                }
                
                // 解析配置项
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                
                if (key.empty()) {
                    throw ConfigParseException("Empty key in config line: " + line);
                }
                
                // 处理配置项
                processConfigItem(key, value);
            }
            
        } catch (const ConfigException& e) {
            std::cerr << ExceptionUtils::formatExceptionWithCode(e, "ConfigLoader::loadConfig") << std::endl;
            throw; // 重新抛出异常
        }
    }
    
private:
    void processConfigItem(const std::string& key, const std::string& value) {
        // 配置项处理逻辑
        if (key == "max_connections") {
            int max_conn = std::stoi(value);
            if (max_conn <= 0) {
                throw ConfigParseException("Invalid max_connections value: " + value);
            }
        }
    }
};

// 示例2：网络服务器中的异常处理
class NetworkServer {
public:
    void bind(const std::string& address, int port) {
        try {
            // 模拟绑定失败
            if (port < 1024) {
                throw BindException(address, port);
            }
            
            // 模拟socket创建失败
            if (address == "invalid_address") {
                throw SocketException("Invalid address format: " + address);
            }
            
            std::cout << "Successfully bound to " << address << ":" << port << std::endl;
            
        } catch (const NetworkException& e) {
            std::cerr << ExceptionUtils::formatExceptionWithCode(e, "NetworkServer::bind") << std::endl;
            throw;
        }
    }
    
    void acceptConnection() {
        try {
            // 模拟连接接受失败
            if (shouldFailConnection()) {
                throw ConnectionException("Connection timeout");
            }
            
            std::cout << "Connection accepted successfully" << std::endl;
            
        } catch (const NetworkException& e) {
            std::cerr << ExceptionUtils::formatExceptionWithCode(e, "NetworkServer::acceptConnection") << std::endl;
            throw;
        }
    }
    
private:
    bool shouldFailConnection() {
        // 模拟随机失败
        return (rand() % 10) == 0;
    }
};

// 示例3：数据存储中的异常处理
class DataStorage {
public:
    void saveData(const std::string& filename, const std::string& data) {
        try {
            std::ofstream file(filename);
            if (!file.is_open()) {
                throw FileIOException(filename, "write");
            }
            
            file << data;
            if (file.fail()) {
                throw FileIOException(filename, "write");
            }
            
            std::cout << "Data saved successfully to " << filename << std::endl;
            
        } catch (const StorageException& e) {
            std::cerr << ExceptionUtils::formatExceptionWithCode(e, "DataStorage::saveData") << std::endl;
            throw;
        }
    }
    
    std::string loadData(const std::string& filename) {
        try {
            std::ifstream file(filename);
            if (!file.is_open()) {
                throw FileIOException(filename, "read");
            }
            
            std::string data;
            std::getline(file, data);
            
            // 模拟数据损坏检测
            if (data.find("CORRUPTED") != std::string::npos) {
                throw DataCorruptionException("Data contains corruption marker");
            }
            
            std::cout << "Data loaded successfully from " << filename << std::endl;
            return data;
            
        } catch (const StorageException& e) {
            std::cerr << ExceptionUtils::formatExceptionWithCode(e, "DataStorage::loadData") << std::endl;
            throw;
        }
    }
};

// 示例4：Redis协议处理中的异常处理
class RedisProtocolHandler {
public:
    void parseCommand(const std::string& command) {
        try {
            if (command.empty()) {
                throw InvalidCommandException("Empty command");
            }
            
            std::vector<std::string> parts = splitCommand(command);
            if (parts.empty()) {
                throw ProtocolParseException("Failed to parse command: " + command);
            }
            
            std::string cmd = parts[0];
            if (!isValidCommand(cmd)) {
                throw InvalidCommandException(cmd);
            }
            
            std::cout << "Command parsed successfully: " << cmd << std::endl;
            
        } catch (const RedisProtocolException& e) {
            std::cerr << ExceptionUtils::formatExceptionWithCode(e, "RedisProtocolHandler::parseCommand") << std::endl;
            throw;
        }
    }
    
private:
    std::vector<std::string> splitCommand(const std::string& command) {
        std::vector<std::string> parts;
        std::stringstream ss(command);
        std::string part;
        
        while (std::getline(ss, part, ' ')) {
            if (!part.empty()) {
                parts.push_back(part);
            }
        }
        
        return parts;
    }
    
    bool isValidCommand(const std::string& cmd) {
        std::vector<std::string> valid_commands = {"GET", "SET", "DEL", "PING", "QUIT"};
        return std::find(valid_commands.begin(), valid_commands.end(), cmd) != valid_commands.end();
    }
};

// 示例5：内存管理中的异常处理
class MemoryManager {
public:
    void* allocateMemory(size_t size) {
        try {
            void* ptr = malloc(size);
            if (ptr == nullptr) {
                throw OutOfMemoryException("malloc(" + std::to_string(size) + ")");
            }
            
            std::cout << "Memory allocated successfully: " << size << " bytes" << std::endl;
            return ptr;
            
        } catch (const MemoryException& e) {
            std::cerr << ExceptionUtils::formatExceptionWithCode(e, "MemoryManager::allocateMemory") << std::endl;
            throw;
        }
    }
    
    void deallocateMemory(void* ptr) {
        if (ptr != nullptr) {
            free(ptr);
            std::cout << "Memory deallocated successfully" << std::endl;
        }
    }
};

// 示例6：线程管理中的异常处理
class ThreadManager {
public:
    void createWorkerThread(const std::string& thread_name) {
        try {
            std::thread worker([this, thread_name]() {
                try {
                    workerFunction(thread_name);
                } catch (const std::exception& e) {
                    std::cerr << "Worker thread exception: " << e.what() << std::endl;
                }
            });
            
            if (!worker.joinable()) {
                throw ThreadCreationException(thread_name);
            }
            
            worker.detach();
            std::cout << "Worker thread created successfully: " << thread_name << std::endl;
            
        } catch (const ThreadException& e) {
            std::cerr << ExceptionUtils::formatExceptionWithCode(e, "ThreadManager::createWorkerThread") << std::endl;
            throw;
        }
    }
    
private:
    void workerFunction(const std::string& thread_name) {
        std::cout << "Worker thread " << thread_name << " started" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "Worker thread " << thread_name << " finished" << std::endl;
    }
};

// 示例7：日志系统中的异常处理
class LogManager {
public:
    void writeLog(const std::string& log_file, const std::string& message) {
        try {
            std::ofstream log_stream(log_file, std::ios::app);
            if (!log_stream.is_open()) {
                throw LogFileException(log_file);
            }
            
            log_stream << getCurrentTimestamp() << " " << message << std::endl;
            if (log_stream.fail()) {
                throw LogFileException(log_file);
            }
            
            std::cout << "Log written successfully to " << log_file << std::endl;
            
        } catch (const LoggingException& e) {
            std::cerr << ExceptionUtils::formatExceptionWithCode(e, "LogManager::writeLog") << std::endl;
            throw;
        }
    }
    
private:
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        return std::ctime(&time_t);
    }
};

// 主函数：演示异常处理
int main() {
    std::cout << "=== 自定义异常类使用示例 ===" << std::endl;
    
    try {
        // 测试配置加载
        std::cout << "\n1. 测试配置加载异常处理:" << std::endl;
        ConfigLoader config_loader;
        try {
            config_loader.loadConfig("nonexistent.conf");
        } catch (const ConfigException& e) {
            std::cout << "捕获配置异常: " << e.what() << std::endl;
        }
        
        // 测试网络服务器
        std::cout << "\n2. 测试网络服务器异常处理:" << std::endl;
        NetworkServer network_server;
        try {
            network_server.bind("127.0.0.1", 80); // 低端口，会抛出异常
        } catch (const NetworkException& e) {
            std::cout << "捕获网络异常: " << e.what() << std::endl;
        }
        
        // 测试数据存储
        std::cout << "\n3. 测试数据存储异常处理:" << std::endl;
        DataStorage storage;
        try {
            storage.saveData("/invalid/path/file.txt", "test data");
        } catch (const StorageException& e) {
            std::cout << "捕获存储异常: " << e.what() << std::endl;
        }
        
        // 测试Redis协议处理
        std::cout << "\n4. 测试Redis协议异常处理:" << std::endl;
        RedisProtocolHandler redis_handler;
        try {
            redis_handler.parseCommand("INVALID_COMMAND");
        } catch (const RedisProtocolException& e) {
            std::cout << "捕获Redis协议异常: " << e.what() << std::endl;
        }
        
        // 测试内存管理
        std::cout << "\n5. 测试内存管理异常处理:" << std::endl;
        MemoryManager mem_manager;
        try {
            // 尝试分配一个非常大的内存块
            void* ptr = mem_manager.allocateMemory(SIZE_MAX);
            mem_manager.deallocateMemory(ptr);
        } catch (const MemoryException& e) {
            std::cout << "捕获内存异常: " << e.what() << std::endl;
        }
        
        // 测试线程管理
        std::cout << "\n6. 测试线程管理异常处理:" << std::endl;
        ThreadManager thread_manager;
        try {
            thread_manager.createWorkerThread("test_worker");
            std::this_thread::sleep_for(std::chrono::seconds(3));
        } catch (const ThreadException& e) {
            std::cout << "捕获线程异常: " << e.what() << std::endl;
        }
        
        // 测试日志系统
        std::cout << "\n7. 测试日志系统异常处理:" << std::endl;
        LogManager log_manager;
        try {
            log_manager.writeLog("/invalid/log/path/app.log", "Test log message");
        } catch (const LoggingException& e) {
            std::cout << "捕获日志异常: " << e.what() << std::endl;
        }
        
        std::cout << "\n=== 所有异常处理测试完成 ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "未捕获的异常: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 