#include "redis_handler.h"
#include "../logger/logger.h"
#include "../config/config.h"
#include <sstream>
#include <algorithm>
#include <chrono>
#include <fstream>

RedisHandler::RedisHandler()
    : current_db_(0)
    , authenticated_(true) // 默认不需要认证
    , password_("") {
}

RedisHandler::~RedisHandler() {
}

void RedisHandler::init(int max_level) {
    skiplist_ = std::make_unique<SkipList<int, std::string>>(max_level);
    registerCommands();
    
    // 加载AOF配置
    const auto& aof_config = Config::getInstance().getAOFConfig();
    aof_enabled_ = aof_config.enable_aof;
    aof_file_ = aof_config.aof_file;
    aof_fsync_ = aof_config.aof_fsync;
    aof_fsync_interval_ = aof_config.aof_fsync_interval;
    last_aof_fsync_ = std::chrono::system_clock::now();
    
    // 打开AOF文件
    if (aof_enabled_) {
        aof_stream_.open(aof_file_, std::ios::app);
        if (!aof_stream_.is_open()) {
            LOG_ERROR("Failed to open AOF file: " + aof_file_);
            aof_enabled_ = false;
        }
    }
    
    // 加载AOF
    if (aof_enabled_) {
        loadAOF();
    }
    
    // 加载现有数据
    loadData();
    
    // 初始化复制管理器
    initReplication();
    
    LOG_INFO("Redis handler initialized with max level: " + std::to_string(max_level));
}

std::string RedisHandler::handleCommand(const std::string& request, std::shared_ptr<ClientConnection> client) {
    try {
        RedisCommand cmd = RedisProtocol::parseCommand(request);
        
        if (cmd.command.empty()) {
            return createErrorResponse("ERR unknown command");
        }
        
        // 更新统计信息
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.total_commands++;
        }
        
        // 查找命令处理器
        auto it = command_handlers_.find(cmd.command);
        if (it != command_handlers_.end()) {
            return it->second(cmd.arguments, client);
        } else {
            return handleUnknown(cmd.arguments, client);
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error handling command: " + std::string(e.what()));
        return createErrorResponse("ERR internal error");
    }
}

void RedisHandler::registerCommands() {
    command_handlers_["PING"] = [this](const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
        return handlePing(args, client);
    };
    
    command_handlers_["ECHO"] = [this](const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
        return handleEcho(args, client);
    };
    
    command_handlers_["SET"] = [this](const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
        return handleSet(args, client);
    };
    
    command_handlers_["GET"] = [this](const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
        return handleGet(args, client);
    };
    
    command_handlers_["DEL"] = [this](const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
        return handleDel(args, client);
    };
    
    command_handlers_["EXISTS"] = [this](const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
        return handleExists(args, client);
    };
    
    command_handlers_["KEYS"] = [this](const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
        return handleKeys(args, client);
    };
    
    command_handlers_["FLUSH"] = [this](const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
        return handleFlush(args, client);
    };
    
    command_handlers_["SAVE"] = [this](const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
        return handleSave(args, client);
    };
    
    command_handlers_["LOAD"] = [this](const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
        return handleLoad(args, client);
    };
    
    command_handlers_["INFO"] = [this](const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
        return handleInfo(args, client);
    };
    
    command_handlers_["CONFIG"] = [this](const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
        return handleConfig(args, client);
    };
    
    command_handlers_["SELECT"] = [this](const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
        return handleSelect(args, client);
    };
    
    command_handlers_["AUTH"] = [this](const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
        return handleAuth(args, client);
    };
    
    command_handlers_["QUIT"] = [this](const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
        return handleQuit(args, client);
    };
}

std::string RedisHandler::handlePing(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.ping_commands++;
    
    if (args.empty()) {
        return RedisProtocol::createSimpleString("PONG");
    } else {
        return RedisProtocol::createBulkString(args[0]);
    }
}

std::string RedisHandler::handleEcho(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
    if (args.empty()) {
        return createErrorResponse("ERR wrong number of arguments for 'echo' command");
    }
    return RedisProtocol::createBulkString(args[0]);
}

std::string RedisHandler::handleSet(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
    if (args.size() < 2) {
        return createErrorResponse("ERR wrong number of arguments for 'set' command");
    }
    
    int key;
    if (!stringToInt(args[0], key)) {
        return createErrorResponse("ERR key must be an integer");
    }
    
    int result = skiplist_->insert_element(key, args[1]);
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.set_commands++;
    }
    
    if (result == 0) {
        // 追加AOF
        appendAOF("SET " + args[0] + " " + args[1]);
        
        // 复制命令到从节点
        if (replication_manager_ && replication_manager_->isMaster()) {
            replication_manager_->replicateCommand("SET " + args[0] + " " + args[1]);
        }
        
        return RedisProtocol::createSimpleString("OK");
    } else {
        return createErrorResponse("ERR failed to set key");
    }
}

std::string RedisHandler::handleGet(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
    if (args.size() != 1) {
        return createErrorResponse("ERR wrong number of arguments for 'get' command");
    }
    
    int key;
    if (!stringToInt(args[0], key)) {
        return createErrorResponse("ERR key must be an integer");
    }
    
    bool exists = skiplist_->search_element(key);
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.get_commands++;
    }
    
    if (exists) {
        // 注意：这里简化了实现，实际应该返回存储的值
        return RedisProtocol::createBulkString("value_for_key_" + args[0]);
    } else {
        return RedisProtocol::createNullBulkString();
    }
}

std::string RedisHandler::handleDel(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
    if (args.size() != 1) {
        return createErrorResponse("ERR wrong number of arguments for 'del' command");
    }
    
    int key;
    if (!stringToInt(args[0], key)) {
        return createErrorResponse("ERR key must be an integer");
    }
    
    skiplist_->delete_element(key);
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.del_commands++;
    }
    
    // 追加AOF
    appendAOF("DEL " + args[0]);
    
    // 复制命令到从节点
    if (replication_manager_ && replication_manager_->isMaster()) {
        replication_manager_->replicateCommand("DEL " + args[0]);
    }
    
    return RedisProtocol::createInteger(1);
}

std::string RedisHandler::handleExists(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
    if (args.size() != 1) {
        return createErrorResponse("ERR wrong number of arguments for 'exists' command");
    }
    
    int key;
    if (!stringToInt(args[0], key)) {
        return createErrorResponse("ERR key must be an integer");
    }
    
    bool exists = skiplist_->search_element(key);
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.exists_commands++;
    }
    
    return RedisProtocol::createInteger(exists ? 1 : 0);
}

std::string RedisHandler::handleKeys(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
    // 简化实现，返回空数组
    return RedisProtocol::createEmptyArray();
}

std::string RedisHandler::handleFlush(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
    // 清空跳表
    // 注意：这里需要实现清空功能
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.flush_commands++;
    }
    
    // 追加AOF
    appendAOF("FLUSH");
    
    // 复制命令到从节点
    if (replication_manager_ && replication_manager_->isMaster()) {
        replication_manager_->replicateCommand("FLUSH");
    }
    
    return RedisProtocol::createSimpleString("OK");
}

std::string RedisHandler::handleSave(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
    saveData();
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.save_commands++;
    }
    
    return RedisProtocol::createSimpleString("OK");
}

std::string RedisHandler::handleLoad(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
    loadData();
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.load_commands++;
    }
    
    return RedisProtocol::createSimpleString("OK");
}

std::string RedisHandler::handleInfo(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.info_commands++;
    }
    
    return RedisProtocol::createBulkString(getServerInfo());
}

std::string RedisHandler::handleConfig(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
    if (args.size() < 1) {
        return createErrorResponse("ERR wrong number of arguments for 'config' command");
    }
    
    if (args[0] == "GET") {
        return RedisProtocol::createBulkString(getConfigInfo());
    }
    
    return createErrorResponse("ERR unknown subcommand");
}

std::string RedisHandler::handleSelect(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
    if (args.size() != 1) {
        return createErrorResponse("ERR wrong number of arguments for 'select' command");
    }
    
    int db;
    if (!stringToInt(args[0], db)) {
        return createErrorResponse("ERR invalid DB index");
    }
    
    if (db < 0 || db > 15) {
        return createErrorResponse("ERR DB index is out of range");
    }
    
    current_db_ = db;
    return RedisProtocol::createSimpleString("OK");
}

std::string RedisHandler::handleAuth(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
    if (args.size() != 1) {
        return createErrorResponse("ERR wrong number of arguments for 'auth' command");
    }
    
    if (password_.empty() || args[0] == password_) {
        authenticated_ = true;
        return RedisProtocol::createSimpleString("OK");
    } else {
        return createErrorResponse("ERR invalid password");
    }
}

std::string RedisHandler::handleQuit(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
    return RedisProtocol::createSimpleString("OK");
}

std::string RedisHandler::handleUnknown(const std::vector<std::string>& args, std::shared_ptr<ClientConnection> client) {
    return createErrorResponse("ERR unknown command");
}

std::string RedisHandler::createErrorResponse(const std::string& error) {
    return RedisProtocol::createError(error);
}

std::string RedisHandler::createOkResponse() {
    return RedisProtocol::createSimpleString("OK");
}

std::string RedisHandler::createIntegerResponse(int64_t value) {
    return RedisProtocol::createInteger(value);
}

std::string RedisHandler::createBulkStringResponse(const std::string& value) {
    return RedisProtocol::createBulkString(value);
}

std::string RedisHandler::createArrayResponse(const std::vector<std::string>& values) {
    return RedisProtocol::createArray(values);
}

bool RedisHandler::stringToInt(const std::string& str, int& value) {
    try {
        value = std::stoi(str);
        return true;
    } catch (...) {
        return false;
    }
}

std::string RedisHandler::getServerInfo() {
    std::ostringstream oss;
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    oss << "# Server\n";
    oss << "redis_version:1.0.0\n";
    oss << "os:Linux\n";
    oss << "arch_bits:64\n";
    oss << "multiplexing_api:epoll\n";
    oss << "process_id:" << getpid() << "\n";
    oss << "uptime_in_seconds:" << std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() << "\n";
    oss << "uptime_in_days:0\n";
    oss << "tcp_port:6379\n";
    oss << "connected_clients:" << 0 << "\n";
    oss << "used_memory:" << 0 << "\n";
    oss << "used_memory_human:0B\n";
    oss << "used_memory_rss:" << 0 << "\n";
    oss << "used_memory_peak:" << 0 << "\n";
    oss << "used_memory_peak_human:0B\n";
    oss << "used_memory_lua:0\n";
    oss << "mem_fragmentation_ratio:0.00\n";
    oss << "mem_allocator:libc\n";
    
    // 统计信息
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        oss << "# Stats\n";
        oss << "total_commands_processed:" << stats_.total_commands << "\n";
        oss << "instantaneous_ops_per_sec:0\n";
        oss << "total_connections_received:0\n";
        oss << "rejected_connections:0\n";
        oss << "expired_keys:0\n";
        oss << "evicted_keys:0\n";
        oss << "keyspace_hits:0\n";
        oss << "keyspace_misses:0\n";
        oss << "pubsub_channels:0\n";
        oss << "pubsub_patterns:0\n";
        oss << "latest_fork_usec:0\n";
    }
    
    return oss.str();
}

std::string RedisHandler::getConfigInfo() {
    std::ostringstream oss;
    oss << "maxmemory\n";
    oss << "maxmemory-policy\n";
    oss << "timeout\n";
    oss << "tcp-keepalive\n";
    oss << "databases\n";
    return oss.str();
}

void RedisHandler::resetStats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = Stats{};
}

void RedisHandler::saveData() {
    if (skiplist_) {
        skiplist_->dump_file();
        LOG_INFO("Data saved to file");
    }
}

void RedisHandler::loadData() {
    if (skiplist_) {
        skiplist_->load_file();
        LOG_INFO("Data loaded from file");
    }
}

bool RedisHandler::isAOFEnabled() const {
    return aof_enabled_;
}

void RedisHandler::appendAOF(const std::string& cmdline) {
    if (!aof_enabled_) return;
    std::lock_guard<std::mutex> lock(aof_mutex_);
    aof_stream_ << cmdline << "\n";
    if (aof_fsync_ == "always") {
        aof_stream_.flush();
    } else if (aof_fsync_ == "everysec") {
        auto now = std::chrono::system_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_aof_fsync_).count() >= aof_fsync_interval_) {
            aof_stream_.flush();
            last_aof_fsync_ = now;
        }
    }
}

void RedisHandler::flushAOF() {
    if (!aof_enabled_) return;
    std::lock_guard<std::mutex> lock(aof_mutex_);
    aof_stream_.flush();
}

void RedisHandler::reopenAOF() {
    if (!aof_enabled_) return;
    std::lock_guard<std::mutex> lock(aof_mutex_);
    if (aof_stream_.is_open()) aof_stream_.close();
    aof_stream_.open(aof_file_, std::ios::app);
}

void RedisHandler::loadAOF() {
    std::ifstream aof_in(aof_file_);
    if (!aof_in.is_open()) return;
    std::string line;
    while (std::getline(aof_in, line)) {
        if (line.empty()) continue;
        handleCommand(line, nullptr); // 直接重放命令
    }
    aof_in.close();
}

// 复制相关方法实现
void RedisHandler::initReplication(const std::string& master_host, int master_port) {
    replication_manager_ = std::make_unique<ReplicationManager>();
    replication_manager_->init(master_host, master_port);
    
    // 设置命令处理器回调
    replication_manager_->setCommandHandler([this](const std::string& command) {
        // 从节点接收到复制命令时，直接执行
        handleCommand(command, nullptr);
    });
}

bool RedisHandler::startReplication() {
    if (replication_manager_) {
        return replication_manager_->startReplication();
    }
    return false;
}

void RedisHandler::stopReplication() {
    if (replication_manager_) {
        replication_manager_->stopReplication();
    }
}

bool RedisHandler::isMaster() const {
    return replication_manager_ && replication_manager_->isMaster();
}

bool RedisHandler::isSlave() const {
    return replication_manager_ && replication_manager_->isSlave();
}

void RedisHandler::addSlave(const std::string& host, int port) {
    if (replication_manager_) {
        replication_manager_->addSlave(host, port);
    }
}

std::vector<SlaveInfo> RedisHandler::getSlaves() const {
    if (replication_manager_) {
        return replication_manager_->getSlaves();
    }
    return {};
} 