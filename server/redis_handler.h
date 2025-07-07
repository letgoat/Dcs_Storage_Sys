#pragma once
#include <string>
#include <memory>
#include <map>
#include <functional>
#include "../skiplist/skiplist.h"
#include "../network/redis_protocol.h"
#include "../network/tcp_server.h"
#include <fstream>
#include <chrono>

class RedisHandler {
public:
    RedisHandler();
    ~RedisHandler();
    
    // 初始化处理器
    void init(int max_level = 18);
    
    // 处理Redis命令
    std::string handleCommand(const std::string& request, std::shared_ptr<ClientConnection> client);
    
    // 获取跳表实例
    SkipList<int, std::string>& getSkipList() { return *skiplist_; }
    
    // 获取统计信息
    struct Stats {
        size_t total_commands = 0;
        size_t get_commands = 0;
        size_t set_commands = 0;
        size_t del_commands = 0;
        size_t exists_commands = 0;
        size_t ping_commands = 0;
        size_t info_commands = 0;
        size_t flush_commands = 0;
        size_t save_commands = 0;
        size_t load_commands = 0;
    };
    
    const Stats& getStats() const { return stats_; }
    
    // 重置统计信息
    void resetStats();
    
    // 保存数据
    void saveData();
    
    // 加载数据
    void loadData();

    // AOF相关
    void appendAOF(const std::string& cmdline);
    void loadAOF();
    void flushAOF();
    void reopenAOF();
    bool isAOFEnabled() const;

private:
    // 命令处理函数类型
    using CommandHandler = std::function<std::string(const std::vector<std::string>&, std::shared_ptr<ClientConnection>)>;
    
    // 注册命令处理器
    void registerCommands();
    
    // 命令处理函数
    std::string handlePing(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client);
    std::string handleEcho(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client);
    std::string handleSet(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client);
    std::string handleGet(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client);
    std::string handleDel(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client);
    std::string handleExists(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client);
    std::string handleKeys(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client);
    std::string handleFlush(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client);
    std::string handleSave(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client);
    std::string handleLoad(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client);
    std::string handleInfo(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client);
    std::string handleConfig(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client);
    std::string handleSelect(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client);
    std::string handleAuth(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client);
    std::string handleQuit(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client);
    std::string handleUnknown(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client);
    
    // 辅助函数
    std::string createErrorResponse(const std::string& error);
    std::string createOkResponse();
    std::string createIntegerResponse(int64_t value);
    std::string createBulkStringResponse(const std::string& value);
    std::string createArrayResponse(const std::vector<std::string>& values);
    
    // 字符串转整数
    bool stringToInt(const std::string& str, int& value);
    
    // 获取服务器信息
    std::string getServerInfo();
    
    // 获取配置信息
    std::string getConfigInfo();
    
    std::unique_ptr<SkipList<int, std::string>> skiplist_;
    std::map<std::string, CommandHandler> command_handlers_;
    Stats stats_;
    std::mutex stats_mutex_;
    
    // 当前数据库编号（Redis支持多个数据库）
    int current_db_;
    
    // 认证状态
    bool authenticated_;
    std::string password_;

    // AOF相关
    std::ofstream aof_stream_;
    std::string aof_file_;
    bool aof_enabled_ = false;
    std::string aof_fsync_;
    int aof_fsync_interval_ = 1;
    std::mutex aof_mutex_;
    std::chrono::system_clock::time_point last_aof_fsync_;
}; 