#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <random>

class Utils {
public:
    // 字符串工具
    static std::vector<std::string> split(const std::string& str, char delimiter);
    static std::string trim(const std::string& str);
    static std::string toLower(const std::string& str);
    static std::string toUpper(const std::string& str);
    static bool startsWith(const std::string& str, const std::string& prefix);
    static bool endsWith(const std::string& str, const std::string& suffix);
    static std::string replace(const std::string& str, const std::string& from, const std::string& to);
    
    // 数字工具
    static bool isNumber(const std::string& str);
    static int stringToInt(const std::string& str, int default_value = 0);
    static double stringToDouble(const std::string& str, double default_value = 0.0);
    static std::string intToString(int value);
    static std::string doubleToString(double value, int precision = 2);
    
    // 时间工具
    static std::string getCurrentTimestamp();
    static std::string getCurrentDate();
    static std::string getCurrentTime();
    static std::string formatDuration(std::chrono::milliseconds duration);
    static std::chrono::system_clock::time_point parseTimestamp(const std::string& timestamp);
    
    // 随机数工具
    static int randomInt(int min, int max);
    static double randomDouble(double min, double max);
    static std::string randomString(size_t length);
    static std::string generateUUID();
    
    // 文件工具
    static bool fileExists(const std::string& filename);
    static size_t getFileSize(const std::string& filename);
    static std::string getFileExtension(const std::string& filename);
    static std::string getFileName(const std::string& path);
    static std::string getDirectory(const std::string& path);
    static bool createDirectory(const std::string& path);
    static std::vector<std::string> listFiles(const std::string& directory);
    
    // 网络工具
    static bool isValidIP(const std::string& ip);
    static bool isValidPort(int port);
    static std::string getLocalIP();
    static std::string getHostname();
    
    // 内存工具
    static size_t getCurrentMemoryUsage();
    static std::string formatBytes(size_t bytes);
    
    // 性能工具
    static double getCurrentTimeSeconds();
    static void sleep(int milliseconds);
    static void sleepSeconds(double seconds);
    
    // 加密工具
    static std::string md5(const std::string& input);
    static std::string sha1(const std::string& input);
    static std::string base64Encode(const std::string& input);
    static std::string base64Decode(const std::string& input);
    
    // 系统工具
    static int getProcessId();
    static std::string getProcessName();
    static int getThreadId();
    static int getCpuCount();
    static std::string getOsName();
    static std::string getOsVersion();
    
    // 配置工具
    static std::string getEnvironmentVariable(const std::string& name, const std::string& default_value = "");
    static bool setEnvironmentVariable(const std::string& name, const std::string& value);
    
    // 日志工具
    static std::string getLogLevelString(int level);
    static std::string formatLogMessage(const std::string& level, const std::string& message, 
                                       const std::string& file = "", int line = 0);
    
    // 统计工具
    static double calculateAverage(const std::vector<double>& values);
    static double calculateMedian(const std::vector<double>& values);
    static double calculateStandardDeviation(const std::vector<double>& values);
    static double calculatePercentile(const std::vector<double>& values, double percentile);
    
    // 验证工具
    static bool isValidEmail(const std::string& email);
    static bool isValidUrl(const std::string& url);
    static bool isValidPhoneNumber(const std::string& phone);
    static bool isValidCreditCard(const std::string& card);
    
    // 格式化工具
    static std::string formatNumber(double number, int precision = 2);
    static std::string formatPercentage(double value, int precision = 2);
    static std::string formatCurrency(double amount, const std::string& currency = "USD");
    static std::string formatFileSize(size_t bytes);
    
    // 调试工具
    static void printStackTrace();
    static std::string getStackTrace();
    static void setDebugMode(bool enabled);
    static bool isDebugMode();
    
private:
    static std::mt19937 random_generator_;
    static bool debug_mode_;
    
    // 初始化随机数生成器
    static void initRandomGenerator();
}; 