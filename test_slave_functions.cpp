#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include "replication/replication_manager.h"

int main() {
    std::cout << "=== Testing Slave Functions Implementation ===" << std::endl;
    
    // 测试从节点功能
    std::cout << "\n1. Testing Slave Mode..." << std::endl;
    ReplicationManager slave;
    
    // 初始化为从节点
    slave.init("127.0.0.1", 16379);
    std::cout << "Slave initialized, master: " << slave.getMasterAddress() << std::endl;
    std::cout << "Slave role: " << (slave.isSlave() ? "SLAVE" : "NOT SLAVE") << std::endl;
    
    // 设置命令处理器
    slave.setCommandHandler([](const std::string& command) {
        std::cout << "Slave received command: " << command << std::endl;
    });
    
    // 启动从节点复制
    if (!slave.startReplication()) {
        std::cerr << "Failed to start slave replication" << std::endl;
        return 1;
    }
    
    std::cout << "Slave replication started" << std::endl;
    std::cout << "Slave state: " << static_cast<int>(slave.getState()) << std::endl;
    
    // 运行从节点一段时间
    std::cout << "\n2. Running slave for 10 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(10));
    
    // 测试主节点功能
    std::cout << "\n3. Testing Master Mode..." << std::endl;
    ReplicationManager master;
    
    // 初始化为主节点
    master.init();
    std::cout << "Master initialized" << std::endl;
    
    // 添加从节点
    master.addSlave("127.0.0.1", 6380);
    master.addSlave("127.0.0.1", 6381);
    
    // 启动主节点复制
    if (!master.startReplication()) {
        std::cerr << "Failed to start master replication" << std::endl;
        return 1;
    }
    
    std::cout << "Master replication started" << std::endl;
    
    // 模拟一些复制命令
    std::cout << "\n4. Simulating replication commands..." << std::endl;
    for (int i = 0; i < 5; ++i) {
        std::string command = "SET master_key" + std::to_string(i) + " master_value" + std::to_string(i);
        master.replicateCommand(command);
        
        const auto& stats = master.getStats();
        std::cout << "Command " << (i+1) << ": " << command << std::endl;
        std::cout << "  - Connected slaves: " << stats.connected_slaves << std::endl;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    // 显示从节点信息
    std::cout << "\n5. Slave Information:" << std::endl;
    auto slaves = master.getSlaves();
    for (const auto& slave_info : slaves) {
        std::cout << "  - Slave: " << slave_info.id 
                  << ", State: " << static_cast<int>(slave_info.state)
                  << ", Online: " << (slave_info.is_online ? "Yes" : "No")
                  << ", Offset: " << slave_info.replication_offset << std::endl;
    }
    
    // 运行主节点一段时间
    std::cout << "\n6. Running master for 5 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    // 停止复制
    std::cout << "\n7. Stopping replication..." << std::endl;
    slave.stopReplication();
    master.stopReplication();
    
    std::cout << "\n=== All Tests Completed Successfully! ===" << std::endl;
    return 0;
} 