#pragma once
#include <iostream>
#include <cstdlib> // 随机函数
#include <cmath>
#include <mutex>
#include <cstring>
#include <fstream> // 引入文件操作
#include "../node/node.h"
#define STORE_FILE "store/dumpFile" //存储文件路径

// 跳表的实现
template <typename K, typename V>
class SkipList{
public:
    SkipList(int);
    ~SkipList();
    int get_random_level();
    Node<K,V>* create_node(K, V, int);
    int insert_element(K, V);
    void display_list();
    bool search_element(K);
    void delete_element(K);
    void dump_file();
    bool is_valid_string(const std::string&);
    void get_key_value_from_string(const std::string&, std::string*, std::string*);
    void load_file();
    void clear(Node<K,V>*);
    int size();

private:
    Node<K,V>* head_; //头结点，作为跳表所有节点组织的入口点，类似与单链表
    int max_level_; //跳表中允许的最大层数
    int current_level_; //跳表当前的层数
    int node_count_; //跳表中节点的数量
    std::ofstream file_writer_;
    std::ifstream file_reader_;
};

template class SkipList<int, std::string>;