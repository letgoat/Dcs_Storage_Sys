#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <memory>
#include <sstream>
#include <chrono>
#include <iomanip>

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4
};

class Logger {
public:
    static Logger& getInstance();
    
    // 禁止拷贝和赋值
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // 初始化日志系统
    void init(const std::string& log_file, LogLevel level = LogLevel::INFO, bool enable_console = true);
    
    // 设置日志级别
    void setLevel(LogLevel level);
    
    // 设置是否输出到控制台
    void setConsoleOutput(bool enable);
    
    // 设置日志文件
    void setLogFile(const std::string& log_file);
    
    // 日志记录方法
    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);
    void fatal(const std::string& message);
    
    // 格式化日志记录
    template<typename... Args>
    void debugf(const std::string& format, Args... args) {
        log(LogLevel::DEBUG, formatMessage(format, args...));
    }
    
    template<typename... Args>
    void infof(const std::string& format, Args... args) {
        log(LogLevel::INFO, formatMessage(format, args...));
    }
    
    template<typename... Args>
    void warnf(const std::string& format, Args... args) {
        log(LogLevel::WARN, formatMessage(format, args...));
    }
    
    template<typename... Args>
    void errorf(const std::string& format, Args... args) {
        log(LogLevel::ERROR, formatMessage(format, args...));
    }
    
    template<typename... Args>
    void fatalf(const std::string& format, Args... args) {
        log(LogLevel::FATAL, formatMessage(format, args...));
    }
    
    // 获取当前时间戳
    std::string getCurrentTimestamp() const;
    
    // 获取日志级别字符串
    std::string getLevelString(LogLevel level) const;
    
    // 检查是否需要轮转日志文件
    void checkRotation(const std::string& message);
    
    // 设置日志文件大小限制
    void setMaxFileSize(size_t max_size);
    
    // 设置最大日志文件数量
    void setMaxFiles(int max_files);

private:
    Logger() = default;
    ~Logger();
    
    // 核心日志记录方法
    void log(LogLevel level, const std::string& message);
    
    // 格式化消息
    template<typename... Args>
    std::string formatMessage(const std::string& format, Args... args) {
        
        std::ostringstream oss;
        formatMessageImpl(oss, format, args...);
        return oss.str();
    }
    
    // 递归循环调用，直到没有args后
    template<typename T, typename... Args>
    void formatMessageImpl(std::ostringstream& oss, const std::string& format, T value, Args... args) {
        size_t pos = format.find("{}");
        if (pos != std::string::npos) {
            oss << format.substr(0, pos) << value;
            formatMessageImpl(oss, format.substr(pos + 2), args...);
        } else {
            oss << format;
        }
    }
    
    // 递归结束条件
    void formatMessageImpl(std::ostringstream& oss, const std::string& format) {
        oss << format;
    }
    
    // 轮转日志文件
    void rotateLogFile();
    
    // 写入日志到文件
    void writeToFile(const std::string& message);
    
    // 写入日志到控制台
    void writeToConsole(const std::string& message);
    
    LogLevel current_level_;
    std::string log_file_;
    std::ofstream file_stream_;
    bool enable_console_;
    mutable std::mutex log_mutex_;
    size_t max_file_size_;
    size_t current_file_size_;
    int max_files_;
    bool initializing_ = false;
};

// 便捷的日志宏，打印日志使用宏即可
#define LOG_DEBUG(msg) Logger::getInstance().debug(msg)
#define LOG_INFO(msg) Logger::getInstance().info(msg)
#define LOG_WARN(msg) Logger::getInstance().warn(msg)
#define LOG_ERROR(msg) Logger::getInstance().error(msg)
#define LOG_FATAL(msg) Logger::getInstance().fatal(msg)

#define LOG_DEBUGF(format, ...) Logger::getInstance().debugf(format, ##__VA_ARGS__)
#define LOG_INFOF(format, ...) Logger::getInstance().infof(format, ##__VA_ARGS__)
#define LOG_WARNF(format, ...) Logger::getInstance().warnf(format, ##__VA_ARGS__)
#define LOG_ERRORF(format, ...) Logger::getInstance().errorf(format, ##__VA_ARGS__)
#define LOG_FATALF(format, ...) Logger::getInstance().fatalf(format, ##__VA_ARGS__) 