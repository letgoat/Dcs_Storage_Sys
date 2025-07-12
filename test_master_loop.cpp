#include <iostream>
#include <thread>
#include <chrono>
#include "replication/replication_manager.h"

int main() {
    std::cout << "Testing ReplicationManager masterLoop implementation..." << std::endl;
    
    // 创建主节点复制管理器
    ReplicationManager master;
    
    // 初始化为主节点
    master.init();
    
    // 添加一些测试从节点
    master.addSlave("127.0.0.1", 6380);
    master.addSlave("127.0.0.1", 6381);
    
    // 启动复制
    if (!master.startReplication()) {
        std::cerr << "Failed to start replication" << std::endl;
        return 1;
    }
    
    std::cout << "Master replication started successfully" << std::endl;
    
    // 模拟一些复制命令
    for (int i = 0; i < 5; ++i) {
        std::string command = "SET key" + std::to_string(i) + " value" + std::to_string(i);
        master.replicateCommand(command);
        std::cout << "Replicated command: " << command << std::endl;
        
        // 获取统计信息
        const auto& stats = master.getStats();
        std::cout << "Stats - Commands: " << stats.total_commands_replicated 
                  << ", Bytes: " << stats.total_bytes_replicated 
                  << ", Connected slaves: " << stats.connected_slaves << std::endl;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    // 运行一段时间
    std::cout << "Running master loop for 10 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(10));
    
    // 停止复制
    master.stopReplication();
    
    std::cout << "Test completed successfully!" << std::endl;
    return 0;
} 