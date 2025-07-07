#include "utils.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <random>
#include <chrono>
#include <thread>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <regex>
#include <cctype>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <psapi.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <sys/sysinfo.h>
#include <pwd.h>
#include <netdb.h>
#endif

// 静态成员初始化
std::mt19937 Utils::random_generator_;
bool Utils::debug_mode_ = false;

void Utils::initRandomGenerator() {
    static bool initialized = false;
    if (!initialized) {
        random_generator_.seed(std::chrono::steady_clock::now().time_since_epoch().count());
        initialized = true;
    }
}

// 字符串工具
std::vector<std::string> Utils::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::string Utils::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::string Utils::toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string Utils::toUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

bool Utils::startsWith(const std::string& str, const std::string& prefix) {
    if (str.length() < prefix.length()) {
        return false;
    }
    return str.substr(0, prefix.length()) == prefix;
}

bool Utils::endsWith(const std::string& str, const std::string& suffix) {
    if (str.length() < suffix.length()) {
        return false;
    }
    return str.substr(str.length() - suffix.length()) == suffix;
}

std::string Utils::replace(const std::string& str, const std::string& from, const std::string& to) {
    std::string result = str;
    size_t pos = 0;
    while ((pos = result.find(from, pos)) != std::string::npos) {
        result.replace(pos, from.length(), to);
        pos += to.length();
    }
    return result;
}

// 数字工具
bool Utils::isNumber(const std::string& str) {
    if (str.empty()) {
        return false;
    }
    
    size_t start = 0;
    if (str[0] == '-' || str[0] == '+') {
        start = 1;
    }
    
    bool hasDigit = false;
    bool hasDot = false;
    
    for (size_t i = start; i < str.length(); ++i) {
        if (std::isdigit(str[i])) {
            hasDigit = true;
        } else if (str[i] == '.' && !hasDot) {
            hasDot = true;
        } else {
            return false;
        }
    }
    
    return hasDigit;
}

int Utils::stringToInt(const std::string& str, int default_value) {
    try {
        return std::stoi(str);
    } catch (...) {
        return default_value;
    }
}

double Utils::stringToDouble(const std::string& str, double default_value) {
    try {
        return std::stod(str);
    } catch (...) {
        return default_value;
    }
}

std::string Utils::intToString(int value) {
    return std::to_string(value);
}

std::string Utils::doubleToString(double value, int precision) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value;
    return oss.str();
}

// 时间工具
std::string Utils::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}

std::string Utils::getCurrentDate() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d");
    return ss.str();
}

std::string Utils::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    return ss.str();
}

std::string Utils::formatDuration(std::chrono::milliseconds duration) {
    auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration - hours);
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration - hours - minutes);
    auto ms = duration - hours - minutes - seconds;
    
    std::ostringstream oss;
    if (hours.count() > 0) {
        oss << hours.count() << "h ";
    }
    if (minutes.count() > 0 || hours.count() > 0) {
        oss << minutes.count() << "m ";
    }
    oss << seconds.count() << "s " << ms.count() << "ms";
    
    return oss.str();
}

std::chrono::system_clock::time_point Utils::parseTimestamp(const std::string& timestamp) {
    // 简化实现，实际应该解析各种时间格式
    std::tm tm = {};
    std::istringstream ss(timestamp);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

// 随机数工具
int Utils::randomInt(int min, int max) {
    initRandomGenerator();
    std::uniform_int_distribution<int> dist(min, max);
    return dist(random_generator_);
}

double Utils::randomDouble(double min, double max) {
    initRandomGenerator();
    std::uniform_real_distribution<double> dist(min, max);
    return dist(random_generator_);
}

std::string Utils::randomString(size_t length) {
    const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string result;
    result.reserve(length);
    
    initRandomGenerator();
    std::uniform_int_distribution<int> dist(0, chars.length() - 1);
    
    for (size_t i = 0; i < length; ++i) {
        result += chars[dist(random_generator_)];
    }
    
    return result;
}

std::string Utils::generateUUID() {
    // 简化实现，实际应该生成真正的UUID
    return randomString(8) + "-" + randomString(4) + "-" + randomString(4) + "-" + randomString(4) + "-" + randomString(12);
}

// 文件工具
bool Utils::fileExists(const std::string& filename) {
    return std::filesystem::exists(filename);
}

size_t Utils::getFileSize(const std::string& filename) {
    if (std::filesystem::exists(filename)) {
        return std::filesystem::file_size(filename);
    }
    return 0;
}

std::string Utils::getFileExtension(const std::string& filename) {
    std::filesystem::path path(filename);
    return path.extension().string();
}

std::string Utils::getFileName(const std::string& path) {
    std::filesystem::path filepath(path);
    return filepath.filename().string();
}

std::string Utils::getDirectory(const std::string& path) {
    std::filesystem::path filepath(path);
    return filepath.parent_path().string();
}

bool Utils::createDirectory(const std::string& path) {
    try {
        return std::filesystem::create_directories(path);
    } catch (...) {
        return false;
    }
}

std::vector<std::string> Utils::listFiles(const std::string& directory) {
    std::vector<std::string> files;
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            files.push_back(entry.path().string());
        }
    } catch (...) {
        // 忽略错误
    }
    return files;
}

// 网络工具
bool Utils::isValidIP(const std::string& ip) {
    std::regex ip_regex("^(?:[0-9]{1,3}\\.){3}[0-9]{1,3}$");
    if (!std::regex_match(ip, ip_regex)) {
        return false;
    }
    
    std::vector<std::string> parts = split(ip, '.');
    for (const auto& part : parts) {
        int num = stringToInt(part);
        if (num < 0 || num > 255) {
            return false;
        }
    }
    
    return true;
}

bool Utils::isValidPort(int port) {
    return port >= 1 && port <= 65535;
}

std::string Utils::getLocalIP() {
    // 简化实现
    return "127.0.0.1";
}

std::string Utils::getHostname() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        return std::string(hostname);
    }
    return "unknown";
}

// 内存工具
size_t Utils::getCurrentMemoryUsage() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
#else
    // Linux实现
    std::ifstream file("/proc/self/status");
    std::string line;
    while (std::getline(file, line)) {
        if (line.substr(0, 6) == "VmRSS:") {
            size_t pos = line.find_first_of("0123456789");
            if (pos != std::string::npos) {
                return stringToInt(line.substr(pos)) * 1024; // 转换为字节
            }
        }
    }
#endif
    return 0;
}

std::string Utils::formatBytes(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        unit++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit];
    return oss.str();
}

// 性能工具
double Utils::getCurrentTimeSeconds() {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count() / 1000000.0;
}

void Utils::sleep(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void Utils::sleepSeconds(double seconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(seconds * 1000)));
}

// 加密工具（简化实现）
std::string Utils::md5(const std::string& input) {
    // 简化实现，实际应该使用真正的MD5算法
    return "md5_" + input;
}

std::string Utils::sha1(const std::string& input) {
    // 简化实现，实际应该使用真正的SHA1算法
    return "sha1_" + input;
}

std::string Utils::base64Encode(const std::string& input) {
    // 简化实现，实际应该使用真正的Base64编码
    return "base64_" + input;
}

std::string Utils::base64Decode(const std::string& input) {
    // 简化实现，实际应该使用真正的Base64解码
    if (startsWith(input, "base64_")) {
        return input.substr(7);
    }
    return input;
}

// 系统工具
int Utils::getProcessId() {
#ifdef _WIN32
    return GetCurrentProcessId();
#else
    return getpid();
#endif
}

std::string Utils::getProcessName() {
    // 简化实现
    return "skiplist_server";
}

int Utils::getThreadId() {
    // 简化实现
    return 0;
}

int Utils::getCpuCount() {
#ifdef _WIN32
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    return sysInfo.dwNumberOfProcessors;
#else
    return get_nprocs();
#endif
}

std::string Utils::getOsName() {
#ifdef _WIN32
    return "Windows";
#else
    return "Linux";
#endif
}

std::string Utils::getOsVersion() {
    // 简化实现
    return "1.0";
}

// 配置工具
std::string Utils::getEnvironmentVariable(const std::string& name, const std::string& default_value) {
    const char* value = std::getenv(name.c_str());
    return value ? value : default_value;
}

bool Utils::setEnvironmentVariable(const std::string& name, const std::string& value) {
#ifdef _WIN32
    return _putenv_s(name.c_str(), value.c_str()) == 0;
#else
    return setenv(name.c_str(), value.c_str(), 1) == 0;
#endif
}

// 日志工具
std::string Utils::getLogLevelString(int level) {
    switch (level) {
        case 0: return "DEBUG";
        case 1: return "INFO";
        case 2: return "WARN";
        case 3: return "ERROR";
        case 4: return "FATAL";
        default: return "UNKNOWN";
    }
}

std::string Utils::formatLogMessage(const std::string& level, const std::string& message, 
                                   const std::string& file, int line) {
    std::ostringstream oss;
    oss << "[" << getCurrentTimestamp() << "] [" << level << "] ";
    if (!file.empty()) {
        oss << "[" << file << ":" << line << "] ";
    }
    oss << message;
    return oss.str();
}

// 统计工具
double Utils::calculateAverage(const std::vector<double>& values) {
    if (values.empty()) {
        return 0.0;
    }
    
    double sum = 0.0;
    for (double value : values) {
        sum += value;
    }
    return sum / values.size();
}

double Utils::calculateMedian(const std::vector<double>& values) {
    if (values.empty()) {
        return 0.0;
    }
    
    std::vector<double> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    
    size_t size = sorted.size();
    if (size % 2 == 0) {
        return (sorted[size / 2 - 1] + sorted[size / 2]) / 2.0;
    } else {
        return sorted[size / 2];
    }
}

double Utils::calculateStandardDeviation(const std::vector<double>& values) {
    if (values.size() < 2) {
        return 0.0;
    }
    
    double mean = calculateAverage(values);
    double sum = 0.0;
    
    for (double value : values) {
        double diff = value - mean;
        sum += diff * diff;
    }
    
    return std::sqrt(sum / (values.size() - 1));
}

double Utils::calculatePercentile(const std::vector<double>& values, double percentile) {
    if (values.empty()) {
        return 0.0;
    }
    
    std::vector<double> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    
    double index = (percentile / 100.0) * (sorted.size() - 1);
    size_t lower = static_cast<size_t>(std::floor(index));
    size_t upper = static_cast<size_t>(std::ceil(index));
    
    if (lower == upper) {
        return sorted[lower];
    }
    
    double weight = index - lower;
    return sorted[lower] * (1 - weight) + sorted[upper] * weight;
}

// 验证工具
bool Utils::isValidEmail(const std::string& email) {
    std::regex email_regex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
    return std::regex_match(email, email_regex);
}

bool Utils::isValidUrl(const std::string& url) {
    std::regex url_regex(R"(https?://[^\s/$.?#].[^\s]*)");
    return std::regex_match(url, url_regex);
}

bool Utils::isValidPhoneNumber(const std::string& phone) {
    std::regex phone_regex(R"(\+?[1-9]\d{1,14})");
    return std::regex_match(phone, phone_regex);
}

bool Utils::isValidCreditCard(const std::string& card) {
    // 简化实现，实际应该使用Luhn算法
    std::regex card_regex(R"(\d{13,19})");
    return std::regex_match(card, card_regex);
}

// 格式化工具
std::string Utils::formatNumber(double number, int precision) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << number;
    return oss.str();
}

std::string Utils::formatPercentage(double value, int precision) {
    return formatNumber(value * 100, precision) + "%";
}

std::string Utils::formatCurrency(double amount, const std::string& currency) {
    return currency + " " + formatNumber(amount, 2);
}

std::string Utils::formatFileSize(size_t bytes) {
    return formatBytes(bytes);
}

// 调试工具
void Utils::printStackTrace() {
    // 简化实现
    std::cout << "Stack trace not available" << std::endl;
}

std::string Utils::getStackTrace() {
    return "Stack trace not available";
}

void Utils::setDebugMode(bool enabled) {
    debug_mode_ = enabled;
}

bool Utils::isDebugMode() {
    return debug_mode_;
} 