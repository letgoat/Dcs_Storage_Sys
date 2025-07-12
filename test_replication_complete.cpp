#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include "replication/replication_manager.h"

int main() {
    std::cout << "=== Testing Complete Replication System ===" << std::endl;
    
    // 创建主节点复制管理器
    ReplicationManager master;
    
    // 初始化为主节点
    master.init();
    
    // 添加一些测试从节点
    master.addSlave("127.0.0.1", 6380);
    master.addSlave("127.0.0.1", 6381);
    master.addSlave("127.0.0.1", 6382);
    
    std::cout << "Added " << master.getSlaves().size() << " slaves" << std::endl;
    
    // 启动复制
    if (!master.startReplication()) {
        std::cerr << "Failed to start replication" << std::endl;
        return 1;
    }
    
    std::cout << "Master replication started successfully" << std::endl;
    std::cout << "Master state: " << static_cast<int>(master.getState()) << std::endl;
    
    // 模拟一些复制命令
    std::cout << "\n=== Simulating Replication Commands ===" << std::endl;
    for (int i = 0; i < 10; ++i) {
        std::string command = "SET key" + std::to_string(i) + " value" + std::to_string(i);
        master.replicateCommand(command);
        
        // 获取统计信息
        const auto& stats = master.getStats();
        std::cout << "Command " << (i+1) << ": " << command << std::endl;
        std::cout << "  - Total commands: " << stats.total_commands_replicated 
                  << ", Total bytes: " << stats.total_bytes_replicated 
                  << ", Connected slaves: " << stats.connected_slaves 
                  << ", Replication lag: " << stats.replication_lag << std::endl;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    // 显示从节点信息
    std::cout << "\n=== Slave Information ===" << std::endl;
    auto slaves = master.getSlaves();
    for (const auto& slave : slaves) {
        std::cout << "Slave: " << slave.id 
                  << ", State: " << static_cast<int>(slave.state)
                  << ", Online: " << (slave.is_online ? "Yes" : "No")
                  << ", Offset: " << slave.replication_offset << std::endl;
    }
    
    // 运行主循环一段时间
    std::cout << "\n=== Running Master Loop ===" << std::endl;
    std::cout << "Master loop will run for 5 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    // 最终统计
    std::cout << "\n=== Final Statistics ===" << std::endl;
    const auto& final_stats = master.getStats();
    std::cout << "Final replication stats:" << std::endl;
    std::cout << "  - Total commands replicated: " << final_stats.total_commands_replicated << std::endl;
    std::cout << "  - Total bytes replicated: " << final_stats.total_bytes_replicated << std::endl;
    std::cout << "  - Connected slaves: " << final_stats.connected_slaves << std::endl;
    std::cout << "  - Average replication lag: " << final_stats.replication_lag << std::endl;
    std::cout << "  - Current replication offset: " << master.getReplicationOffset() << std::endl;
    
    // 停止复制
    master.stopReplication();
    
    std::cout << "\n=== Test Completed Successfully! ===" << std::endl;
    return 0;
} 