#include "logger.h"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <sstream>
#include <iomanip>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::~Logger() {
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
}

void Logger::init(const std::string& log_file, LogLevel level, bool enable_console) {
    {
        std::lock_guard<std::mutex> lock(log_mutex_);
        initializing_ = true;
        
        current_level_ = level;
        log_file_ = log_file;
        enable_console_ = enable_console;
        max_file_size_ = 100 * 1024 * 1024; // 100MB
        max_files_ = 10;
        current_file_size_ = 0;
        
        // 创建日志目录
        std::filesystem::path log_path(log_file);
        if (log_path.has_parent_path()) {
            std::filesystem::create_directories(log_path.parent_path());
        }
        
        // 打开日志文件
        file_stream_.open(log_file, std::ios::app);
        if (!file_stream_.is_open()) {
            std::cerr << "Failed to open log file: " << log_file << std::endl;
        }
        
        // 获取当前文件大小
        if (file_stream_.is_open()) {
            file_stream_.seekp(0, std::ios::end);
            current_file_size_ = file_stream_.tellp();
        }
        
        initializing_ = false;
    }
    // 锁释放后再输出日志，避免死锁
    info("Logger initialized");
}

void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    current_level_ = level;
}

void Logger::setConsoleOutput(bool enable) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    enable_console_ = enable;
}

void Logger::setLogFile(const std::string& log_file) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
    
    log_file_ = log_file;
    file_stream_.open(log_file, std::ios::app);
    
    if (file_stream_.is_open()) {
        file_stream_.seekp(0, std::ios::end);
        current_file_size_ = file_stream_.tellp();
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    if (initializing_) {
        return;
    }
    if (level < current_level_) {
        return;
    }
    
    std::string timestamp = getCurrentTimestamp();
    std::string level_str = getLevelString(level);
    std::string formatted_message = timestamp + " [" + level_str + "] " + message + "\n";
    
    {
        std::lock_guard<std::mutex> lock(log_mutex_);
        
        if (enable_console_) {
            writeToConsole(formatted_message);
        }
        
        if (file_stream_.is_open()) {
            writeToFile(formatted_message);
        }
    }
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warn(const std::string& message) {
    log(LogLevel::WARN, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::fatal(const std::string& message) {
    log(LogLevel::FATAL, message);
}

std::string Logger::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}

std::string Logger::getLevelString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

void Logger::checkRotation() {
    std::lock_guard<std::mutex> lock(log_mutex_);
    
    if (current_file_size_ >= max_file_size_) {
        rotateLogFile();
    }
}

void Logger::setMaxFileSize(size_t max_size) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    max_file_size_ = max_size;
}

void Logger::setMaxFiles(int max_files) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    max_files_ = max_files;
}

void Logger::rotateLogFile() {
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
    
    std::filesystem::path log_path(log_file_);
    std::string base_name = log_path.stem().string();
    std::string extension = log_path.extension().string();
    std::string parent_dir = log_path.parent_path().string();
    
    // 删除最老的日志文件
    for (int i = max_files_ - 1; i >= 0; --i) {
        std::string old_file = parent_dir + "/" + base_name + "." + std::to_string(i) + extension;
        if (std::filesystem::exists(old_file)) {
            std::filesystem::remove(old_file);
        }
    }
    
    // 重命名现有日志文件
    for (int i = max_files_ - 2; i >= 0; --i) {
        std::string old_file = parent_dir + "/" + base_name + "." + std::to_string(i) + extension;
        std::string new_file = parent_dir + "/" + base_name + "." + std::to_string(i + 1) + extension;
        if (std::filesystem::exists(old_file)) {
            std::filesystem::rename(old_file, new_file);
        }
    }
    
    // 重命名当前日志文件
    std::string current_backup = parent_dir + "/" + base_name + ".0" + extension;
    if (std::filesystem::exists(log_file_)) {
        std::filesystem::rename(log_file_, current_backup);
    }
    
    // 重新打开日志文件
    file_stream_.open(log_file_, std::ios::app);
    current_file_size_ = 0;
    
    info("Log file rotated");
}

void Logger::writeToFile(const std::string& message) {
    if (file_stream_.is_open()) {
        file_stream_ << message;
        file_stream_.flush();
        current_file_size_ += message.length();
    }
}

void Logger::writeToConsole(const std::string& message) {
    std::cout << message;
    std::cout.flush();
} 