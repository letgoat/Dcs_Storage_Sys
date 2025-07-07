#pragma once
#include <iostream>

// 节点的实现
template <typename K, typename V>
class Node{
public:
    Node() {};
    Node(K k, V v, int);
    ~Node();
    K get_key() const;
    V get_value() const;
    void set_value(V);
    Node<K,V> **forward;

    int node_level;
private:
    K key;
    V value;
};

template class Node<int, std::string>;