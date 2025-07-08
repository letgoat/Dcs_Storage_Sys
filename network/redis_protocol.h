#pragma once
#include <string>
#include <vector>
#include <memory>
#include <variant>

// 命令: SET 1 "Hello world" --> RESP格式： *3\r\n$3\r\nSET\r\n$1\r\n1\r\n$11\r\nHello world\r\n

// Redis数据类型
enum class RedisType {
    SIMPLE_STRING,
    ERROR,
    INTEGER,
    BULK_STRING,
    ARRAY,
    NULL_BULK_STRING
};

// Redis值类型 - 使用variant避免循环引用
struct RedisValue {
    // value可以是字符串，64为整数，或者一个RedisValue智能指针数组
    std::variant<std::string, int64_t, std::vector<std::shared_ptr<RedisValue>>> value;
    
    // 构造函数
    RedisValue() : value(std::string{}) {}
    RedisValue(const std::string& str) : value(str) {}
    RedisValue(int64_t num) : value(num) {}
    RedisValue(const std::vector<std::shared_ptr<RedisValue>>& arr) : value(arr) {}
    
    // 访问器   
    template<typename T>
    bool holds_alternative() const {
        return std::holds_alternative<T>(value);
    }
    
    template<typename T>
    const T& get() const {
        return std::get<T>(value);
    }
    
    template<typename T>
    T& get() {
        return std::get<T>(value);
    }
};

using RedisValuePtr = std::shared_ptr<RedisValue>;

// Redis命令结构
struct RedisCommand {
    std::string command;
    std::vector<std::string> arguments;
};

class RedisProtocol {
public:
    // 解析RESP协议
    static RedisValuePtr parse(const std::string& data);
    
    // 序列化为RESP协议
    static std::string serialize(const RedisValue& value);
    
    // 解析Redis命令
    static RedisCommand parseCommand(const std::string& data);
    
    // 创建简单字符串响应
    static std::string createSimpleString(const std::string& str);
    
    // 创建错误响应
    static std::string createError(const std::string& error);
    
    // 创建整数响应
    static std::string createInteger(int64_t value);
    
    // 创建批量字符串响应
    static std::string createBulkString(const std::string& str);
    
    // 创建空批量字符串响应
    static std::string createNullBulkString();
    
    // 创建数组响应
    static std::string createArray(const std::vector<std::string>& values);
    
    // 创建空数组响应
    static std::string createEmptyArray();
    
    // 检查是否是有效的RESP格式
    static bool isValidRESP(const std::string& data);
    
    // 获取RESP类型的字符串表示
    static std::string getTypeString(RedisType type);

private:
    // 解析简单字符串
    static RedisValuePtr parseSimpleString(const std::string& data, size_t& pos);
    
    // 解析错误
    static RedisValuePtr parseError(const std::string& data, size_t& pos);
    
    // 解析整数
    static RedisValuePtr parseInteger(const std::string& data, size_t& pos);
    
    // 解析批量字符串
    static RedisValuePtr parseBulkString(const std::string& data, size_t& pos);
    
    // 解析数组
    static RedisValuePtr parseArray(const std::string& data, size_t& pos);
    
    // 读取一行（到\r\n）
    static std::string readLine(const std::string& data, size_t& pos);
    
    // 读取指定长度的数据
    static std::string readBytes(const std::string& data, size_t& pos, size_t length);
}; 