#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include "server/redis_handler.h"
#include "network/redis_protocol.h"

void testMaster() {
    std::cout << "=== Starting Master Node ===" << std::endl;
    
    auto master = std::make_unique<RedisHandler>();
    master->init(18);
    
    // 启动复制
    master->startReplication();
    
    // 添加从节点
    master->addSlave("127.0.0.1", 6380);
    master->addSlave("127.0.0.1", 6381);
    
    std::cout << "Master is running. Press Enter to stop..." << std::endl;
    std::cin.get();
    
    master->stopReplication();
    std::cout << "Master stopped." << std::endl;
}

void testSlave(int port, const std::string& master_host, int master_port) {
    std::cout << "=== Starting Slave Node on port " << port << " ===" << std::endl;
    
    auto slave = std::make_unique<RedisHandler>();
    slave->init(18);
    
    // 初始化复制（从节点模式）
    slave->initReplication(master_host, master_port);
    
    // 启动复制
    slave->startReplication();
    
    std::cout << "Slave is running. Press Enter to stop..." << std::endl;
    std::cin.get();
    
    slave->stopReplication();
    std::cout << "Slave stopped." << std::endl;
}

void testReplicationCommands() {
    std::cout << "=== Testing Replication Commands ===" << std::endl;
    
    auto handler = std::make_unique<RedisHandler>();
    handler->init(18);
    
    // 测试主节点模式
    handler->initReplication(); // 不传参数，默认为主节点
    
    std::cout << "Is Master: " << (handler->isMaster() ? "Yes" : "No") << std::endl;
    std::cout << "Is Slave: " << (handler->isSlave() ? "Yes" : "No") << std::endl;
    
    // 添加从节点
    handler->addSlave("127.0.0.1", 6380);
    handler->addSlave("127.0.0.1", 6381);
    
    // 获取从节点列表
    auto slaves = handler->getSlaves();
    std::cout << "Slaves count: " << slaves.size() << std::endl;
    for (const auto& slave : slaves) {
        std::cout << "  - " << slave.host << ":" << slave.port << std::endl;
    }
    
    // 测试从节点模式
    auto slave_handler = std::make_unique<RedisHandler>();
    slave_handler->init(18);
    slave_handler->initReplication("127.0.0.1", 6379);
    
    std::cout << "Slave Is Master: " << (slave_handler->isMaster() ? "Yes" : "No") << std::endl;
    std::cout << "Slave Is Slave: " << (slave_handler->isSlave() ? "Yes" : "No") << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage:" << std::endl;
        std::cout << "  " << argv[0] << " master                    - Start master node" << std::endl;
        std::cout << "  " << argv[0] << " slave <port> <master_host> <master_port>  - Start slave node" << std::endl;
        std::cout << "  " << argv[0] << " test                      - Test replication commands" << std::endl;
        return 1;
    }
    
    std::string mode = argv[1];
    
    if (mode == "master") {
        testMaster();
    } else if (mode == "slave") {
        if (argc < 5) {
            std::cout << "Slave mode requires: port master_host master_port" << std::endl;
            return 1;
        }
        int port = std::stoi(argv[2]);
        std::string master_host = argv[3];
        int master_port = std::stoi(argv[4]);
        testSlave(port, master_host, master_port);
    } else if (mode == "test") {
        testReplicationCommands();
    } else {
        std::cout << "Unknown mode: " << mode << std::endl;
        return 1;
    }
    
    return 0;
} 