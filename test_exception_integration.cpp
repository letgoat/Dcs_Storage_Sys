#include "include/exceptions.h"
#include <iostream>
#include <memory>

using namespace skiplist;

// 模拟配置加载器
class TestConfigLoader {
public:
    void loadConfig(const std::string& filename) {
        if (filename.empty()) {
            throw ConfigParseException("Empty filename");
        }
        
        if (filename == "nonexistent.conf") {
            throw ConfigFileNotFoundException(filename);
        }
        
        if (filename == "invalid.conf") {
            throw ConfigParseException("Invalid configuration format");
        }
        
        std::cout << "Config loaded successfully: " << filename << std::endl;
    }
};

// 模拟网络服务器
class TestNetworkServer {
public:
    void bind(const std::string& address, int port) {
        if (port < 1024) {
            throw BindException(address, port);
        }
        
        if (address == "invalid") {
            throw SocketException("Invalid address format");
        }
        
        std::cout << "Successfully bound to " << address << ":" << port << std::endl;
    }
    
    void acceptConnection() {
        if (shouldFail()) {
            throw ConnectionException("Connection timeout");
        }
        
        std::cout << "Connection accepted successfully" << std::endl;
    }
    
private:
    bool shouldFail() {
        return (rand() % 5) == 0; // 20% 失败率
    }
};

// 模拟数据存储
class TestDataStorage {
public:
    void saveData(const std::string& filename, const std::string& data) {
        if (filename.find("/invalid/") != std::string::npos) {
            throw FileIOException(filename, "write");
        }
        
        if (data.empty()) {
            throw StorageException("Empty data cannot be saved");
        }
        
        std::cout << "Data saved successfully to " << filename << std::endl;
    }
    
    std::string loadData(const std::string& filename) {
        if (filename.find("/invalid/") != std::string::npos) {
            throw FileIOException(filename, "read");
        }
        
        if (filename.find("corrupted") != std::string::npos) {
            throw DataCorruptionException("Data file is corrupted");
        }
        
        std::cout << "Data loaded successfully from " << filename << std::endl;
        return "test_data";
    }
};

// 模拟Redis协议处理器
class TestRedisHandler {
public:
    void parseCommand(const std::string& command) {
        if (command.empty()) {
            throw InvalidCommandException("Empty command");
        }
        
        if (command == "INVALID_COMMAND") {
            throw InvalidCommandException(command);
        }
        
        if (command.find("MALFORMED") != std::string::npos) {
            throw ProtocolParseException("Malformed command: " + command);
        }
        
        std::cout << "Command parsed successfully: " << command << std::endl;
    }
};

// 测试函数
void testConfigExceptions() {
    std::cout << "\n=== 测试配置异常 ===" << std::endl;
    
    TestConfigLoader loader;
    
    try {
        loader.loadConfig("nonexistent.conf");
    } catch (const ConfigFileNotFoundException& e) {
        std::cout << "捕获文件未找到异常: " << e.what() << std::endl;
        std::cout << "错误代码: " << e.getErrorCode() << std::endl;
    }
    
    try {
        loader.loadConfig("invalid.conf");
    } catch (const ConfigParseException& e) {
        std::cout << "捕获配置解析异常: " << e.what() << std::endl;
        std::cout << "错误代码: " << e.getErrorCode() << std::endl;
    }
    
    try {
        loader.loadConfig("");
    } catch (const ConfigParseException& e) {
        std::cout << "捕获空文件名异常: " << e.what() << std::endl;
    }
}

void testNetworkExceptions() {
    std::cout << "\n=== 测试网络异常 ===" << std::endl;
    
    TestNetworkServer server;
    
    try {
        server.bind("127.0.0.1", 80); // 低端口
    } catch (const BindException& e) {
        std::cout << "捕获绑定异常: " << e.what() << std::endl;
        std::cout << "错误代码: " << e.getErrorCode() << std::endl;
    }
    
    try {
        server.bind("invalid", 8080);
    } catch (const SocketException& e) {
        std::cout << "捕获Socket异常: " << e.what() << std::endl;
    }
    
    for (int i = 0; i < 5; ++i) {
        try {
            server.acceptConnection();
        } catch (const ConnectionException& e) {
            std::cout << "捕获连接异常: " << e.what() << std::endl;
        }
    }
}

void testStorageExceptions() {
    std::cout << "\n=== 测试存储异常 ===" << std::endl;
    
    TestDataStorage storage;
    
    try {
        storage.saveData("/invalid/path/file.txt", "test data");
    } catch (const FileIOException& e) {
        std::cout << "捕获文件IO异常: " << e.what() << std::endl;
        std::cout << "错误代码: " << e.getErrorCode() << std::endl;
    }
    
    try {
        storage.saveData("valid.txt", "");
    } catch (const StorageException& e) {
        std::cout << "捕获存储异常: " << e.what() << std::endl;
    }
    
    try {
        storage.loadData("/invalid/path/file.txt");
    } catch (const FileIOException& e) {
        std::cout << "捕获文件读取异常: " << e.what() << std::endl;
    }
    
    try {
        storage.loadData("corrupted_file.txt");
    } catch (const DataCorruptionException& e) {
        std::cout << "捕获数据损坏异常: " << e.what() << std::endl;
    }
}

void testRedisExceptions() {
    std::cout << "\n=== 测试Redis协议异常 ===" << std::endl;
    
    TestRedisHandler handler;
    
    try {
        handler.parseCommand("");
    } catch (const InvalidCommandException& e) {
        std::cout << "捕获无效命令异常: " << e.what() << std::endl;
        std::cout << "错误代码: " << e.getErrorCode() << std::endl;
    }
    
    try {
        handler.parseCommand("INVALID_COMMAND");
    } catch (const InvalidCommandException& e) {
        std::cout << "捕获无效命令异常: " << e.what() << std::endl;
    }
    
    try {
        handler.parseCommand("MALFORMED_COMMAND");
    } catch (const ProtocolParseException& e) {
        std::cout << "捕获协议解析异常: " << e.what() << std::endl;
    }
}

void testExceptionHierarchy() {
    std::cout << "\n=== 测试异常层次结构 ===" << std::endl;
    
    try {
        throw ConfigFileNotFoundException("test.conf");
    } catch (const ConfigException& e) {
        std::cout << "捕获ConfigException: " << e.what() << std::endl;
    } catch (const SkipListException& e) {
        std::cout << "捕获SkipListException: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cout << "捕获std::exception: " << e.what() << std::endl;
    }
    
    try {
        throw NetworkException("Network error");
    } catch (const SkipListException& e) {
        std::cout << "捕获SkipListException: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cout << "捕获std::exception: " << e.what() << std::endl;
    }
}

void testExceptionUtils() {
    std::cout << "\n=== 测试异常工具函数 ===" << std::endl;
    
    try {
        throw ConfigParseException("Invalid configuration");
    } catch (const SkipListException& e) {
        std::string info = ExceptionUtils::formatExceptionWithCode(e, "test_function");
        std::cout << "格式化异常信息: " << info << std::endl;
    }
    
    try {
        throw std::runtime_error("Standard exception");
    } catch (const std::exception& e) {
        std::string info = ExceptionUtils::formatExceptionInfo(e, "test_function");
        std::cout << "格式化标准异常: " << info << std::endl;
    }
}

int main() {
    std::cout << "=== 自定义异常类集成测试 ===" << std::endl;
    
    try {
        testConfigExceptions();
        testNetworkExceptions();
        testStorageExceptions();
        testRedisExceptions();
        testExceptionHierarchy();
        testExceptionUtils();
        
        std::cout << "\n=== 所有测试完成 ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "未捕获的异常: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 