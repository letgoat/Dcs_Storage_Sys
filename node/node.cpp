#include <iostream>
#include <cstring>
#include "node.h"

template <typename K, typename V>
Node<K,V>::Node(K k, V v, int level){
    this->key = k;
    this->value = v;
    this->node_level = level;
    this->forward = new Node<K,V>* [level + 1];
    memset(this->forward, 0, sizeof(Node<K,V>*)*(level + 1));
}

template <typename K, typename V>
Node<K,V>::~Node(){
    delete [] forward;
}


template <typename K, typename V>
K Node<K,V>::get_key() const{
    return key;
}

template <typename K, typename V>
V Node<K,V>::get_value() const{
    return value;
}

template <typename K, typename V>
void Node<K,V>::set_value(V v){
    value = v;
}