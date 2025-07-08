#include "redis_protocol.h"
#include <sstream>
#include <algorithm>
#include <cctype>

RedisValuePtr RedisProtocol::parse(const std::string& data) {
    if (data.empty()) {
        return nullptr;
    }
    
    size_t pos = 0;
    char type = data[0]; //第一个字符判断RESP的类型
    
    switch (type) {
        case '+': return parseSimpleString(data, pos);
        case '-': return parseError(data, pos);
        case ':': return parseInteger(data, pos);
        case '$': return parseBulkString(data, pos);
        case '*': return parseArray(data, pos);
        default: return nullptr;
    }
}

std::string RedisProtocol::serialize(const RedisValue& value) {
    if (value.holds_alternative<std::string>()) {
        return createBulkString(value.get<std::string>());
    } else if (value.holds_alternative<int64_t>()) {
        return createInteger(value.get<int64_t>());
    } else if (value.holds_alternative<std::vector<RedisValuePtr>>()) {
        const auto& array = value.get<std::vector<RedisValuePtr>>();
        std::vector<std::string> strings;
        for (const auto& item : array) {
            if (item) {
                strings.push_back(serialize(*item));
            }
        }
        return createArray(strings);
    }
    return createNullBulkString();
}

RedisCommand RedisProtocol::parseCommand(const std::string& data) {
    RedisCommand cmd;
    auto value = parse(data);
    
    if (!value || !value->holds_alternative<std::vector<RedisValuePtr>>()) {
        return cmd;
    }
    
    const auto& array = value->get<std::vector<RedisValuePtr>>();
    if (array.empty()) {
        return cmd;
    }
    
    // 第一个元素是命令
    if (array[0] && array[0]->holds_alternative<std::string>()) {
        cmd.command = array[0]->get<std::string>();
        std::transform(cmd.command.begin(), cmd.command.end(), cmd.command.begin(), ::toupper);
    }
    
    // 其余元素是参数
    for (size_t i = 1; i < array.size(); ++i) {
        if (array[i] && array[i]->holds_alternative<std::string>()) {
            cmd.arguments.push_back(array[i]->get<std::string>());
        }
    }
    
    return cmd;
}

std::string RedisProtocol::createSimpleString(const std::string& str) {
    return "+" + str + "\r\n";
}

std::string RedisProtocol::createError(const std::string& error) {
    return "-" + error + "\r\n";
}

std::string RedisProtocol::createInteger(int64_t value) {
    return ":" + std::to_string(value) + "\r\n";
}

std::string RedisProtocol::createBulkString(const std::string& str) {
    return "$" + std::to_string(str.length()) + "\r\n" + str + "\r\n";
}

std::string RedisProtocol::createNullBulkString() {
    return "$-1\r\n";
}

std::string RedisProtocol::createArray(const std::vector<std::string>& values) {
    std::string result = "*" + std::to_string(values.size()) + "\r\n";
    for (const auto& value : values) {
        result += createBulkString(value);
    }
    return result;
}

std::string RedisProtocol::createEmptyArray() {
    return "*0\r\n";
}

bool RedisProtocol::isValidRESP(const std::string& data) {
    if (data.empty()) {
        return false;
    }
    
    char type = data[0];
    return type == '+' || type == '-' || type == ':' || type == '$' || type == '*';
}

std::string RedisProtocol::getTypeString(RedisType type) {
    switch (type) {
        case RedisType::SIMPLE_STRING: return "Simple String";
        case RedisType::ERROR: return "Error";
        case RedisType::INTEGER: return "Integer";
        case RedisType::BULK_STRING: return "Bulk String";
        case RedisType::ARRAY: return "Array";
        case RedisType::NULL_BULK_STRING: return "Null Bulk String";
        default: return "Unknown";
    }
}

RedisValuePtr RedisProtocol::parseSimpleString(const std::string& data, size_t& pos) {
    pos = 1; // 跳过'+'
    std::string str = readLine(data, pos);
    return std::make_shared<RedisValue>(str);
}

RedisValuePtr RedisProtocol::parseError(const std::string& data, size_t& pos) {
    pos = 1; // 跳过'-'
    std::string error = readLine(data, pos);
    return std::make_shared<RedisValue>(error);
}

RedisValuePtr RedisProtocol::parseInteger(const std::string& data, size_t& pos) {
    pos = 1; // 跳过':'
    std::string str = readLine(data, pos);
    try {
        int64_t value = std::stoll(str);
        return std::make_shared<RedisValue>(value);
    } catch (...) {
        return nullptr;
    }
}

RedisValuePtr RedisProtocol::parseBulkString(const std::string& data, size_t& pos) {
    pos = 1; // 跳过'$'
    
    // 读取长度
    std::string length_str = readLine(data, pos);
    if (length_str == "-1") {
        return nullptr; // null bulk string
    }
    
    try {
        int64_t length = std::stoll(length_str);
        if (length < 0 || pos + length + 2 > data.length()) {
            return nullptr;
        }
        
        std::string str = readBytes(data, pos, length);
        // 跳过\r\n
        if (pos + 2 <= data.length() && data[pos] == '\r' && data[pos + 1] == '\n') {
            pos += 2;
        }
        
        return std::make_shared<RedisValue>(str);
    } catch (...) {
        return nullptr;
    }
}

RedisValuePtr RedisProtocol::parseArray(const std::string& data, size_t& pos) {
    pos = 1; // 跳过'*'
    
    // 读取数组长度
    std::string length_str = readLine(data, pos);
    try {
        int64_t length = std::stoll(length_str);
        if (length < 0) {
            return nullptr;
        }
        
        std::vector<RedisValuePtr> array;
        for (int64_t i = 0; i < length; ++i) {
            if (pos >= data.length()) {
                break;
            }
            
            auto value = parse(data.substr(pos));
            if (value) {
                array.push_back(value);
                // 计算已解析的长度
                std::string serialized = serialize(*value);
                pos += serialized.length();
            } else {
                break;
            }
        }
        
        return std::make_shared<RedisValue>(array);
    } catch (...) {
        return nullptr;
    }
}

std::string RedisProtocol::readLine(const std::string& data, size_t& pos) {
    std::string line;
    while (pos < data.length() && data[pos] != '\r') {
        line += data[pos++];
    }
    
    // 跳过\r\n
    if (pos + 1 < data.length() && data[pos] == '\r' && data[pos + 1] == '\n') {
        pos += 2;
    }
    
    return line;
}

std::string RedisProtocol::readBytes(const std::string& data, size_t& pos, size_t length) {
    if (pos + length > data.length()) {
        return "";
    }
    
    std::string result = data.substr(pos, length);
    pos += length;
    return result;
} 