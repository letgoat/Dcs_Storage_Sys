#include <iostream>
#include "skiplist.h"

std::mutex mtx;
std::string delimiter = ":"; //分隔符

template<typename K, typename V>
SkipList<K,V>::SkipList(int max_level){
    this->max_level_ = max_level;
    this->current_level_ = 0;
    this->node_count_ = 0;
    K k = K{};
    V v = V{};
    this->head_ = new Node<K,V>(k, v, max_level_);
}

template<typename K, typename V>
SkipList<K,V>::~SkipList(){
    clear(head_);
}

template<typename K, typename V>
Node<K,V>* SkipList<K,V>::create_node(const K k, const V v, int level){
    Node<K,V> *n = new Node<K,V>(k, v, level); 
    return n;
}

template<typename K, typename V>
bool SkipList<K,V>::search_element(K key){
    //定义一个指针current，初始化为跳表的头结点_header
    Node<K,V>* current = head_;
    //从跳表的最高层开始搜索
    for(int i = max_level_; i >= 0; i--){
        //遍历当前层级，直到下一个节点的键值大于或等于待查找的键值
        while(current->forward[i] && current->forward[i]->get_key() < key){
            //移动到当前层级的下一个节点
            current = current->forward[i];
        }
        // 当前节点的下一个节点的键值大于待查找的键值时，进行下沉到下一层
        // 下沉操作通过循环的i--实现
    }
    // 检查当前层(最底层)的下一个节点的键值是否为待查找的键值
    current = current->forward[0];
    if(current && current->get_key() == key){
        // 如果找到匹配的键值，返回true
        return true;
    }
    // 如果没有找到匹配的键值，返回false
    return false;
}

// 在跳表中插入一个新元素
// @param key 待插入节点的key
// @param value 待插入节点的value
// @return 如果元素已经存在，返回1, 否则更新value操作，并返回0

template<typename K, typename V>
int SkipList<K,V>::insert_element(const K key, const V value){
    mtx.lock();
    Node<K,V>* current = this->head_;
    Node<K,V>* update[max_level_ + 1]; //用于记录每层中待更新指针的节点
    memset(update, 0, sizeof(Node<K,V>*)*(max_level_ + 1));

    // 从最高层向下搜索插入位置
    for(int i = max_level_; i >= 0; i--){
        //寻找当前层中最接近且小于key的节点
        while(current->forward[i] != NULL && current->forward[i]->get_key() < key){
            current = current->forward[i]; //移动到下一节点
        }
        //保存每层中该节点，以便后续插入时更新指针
        update[i] = current;
    }

    // 移动到最底层的下一个节点，准备插入操作
    current = current->forward[0];
    // 检查待插入的节点的键是否已存在
    if(current != NULL && current->get_key() == key){
        // 如果键已存在，取消插入
        std::cout << "key:" << key << ", exists" << std::endl;
        mtx.unlock();
        return 1;
    }
    // 检查待插入的节点是否已存在于跳表中
    if(current == NULL || current->get_key() != key){
        // 通过随机函数决定新节点的层级高度
        int random_level = get_random_level();
        // 如果新节点的层级超出了跳表的当前最高层级
        if(random_level > max_level_){
            // 对所有新的更高层级，将头结点设置为它们的前驱节点
            for(int i = max_level_ + 1; i <= random_level; i++){
                update[i] = head_;
            }
            //更新跳表的当前最高层级为新节点的层级
            max_level_ = random_level;
        }

        Node<K,V> *inserted_node = create_node(key, value, random_level);
        // 在各层中插入新节点，同时更新前驱节点的forward指针
        for(int i = 0; i <= random_level; i++){
            // 新节点指向当前节点的下一个节点
            inserted_node->forward[i] = update[i]->forward[i];
            // 当前节点的下一个节点更新为新节点
            update[i]->forward[i] = inserted_node;
        }
        // 更新跳表的当前最高层级为新节点的层级
        max_level_ = random_level;
    }
    mtx.unlock(); // 函数执行完毕后解锁
    return 0;
}

template<typename K, typename V>
int SkipList<K,V>::get_random_level(){
    int k = 1;
    while(rand() % 2){
        k++;
    }
    k = (k < max_level_) ? k : max_level_;
    return k;
}

template<typename K, typename V>
void SkipList<K,V>::delete_element(K key){
    mtx.lock();
    Node<K,V>* current = this->head_;
    Node<K,V>* update[max_level_ + 1];
    memset(update, 0, sizeof(Node<K,V>*) * (max_level_ + 1));

    // 从最高层开始向下搜索待删除节点
    for(int i = max_level_; i >= 0; i--){
        while(current->forward[i] != NULL && current->forward[i]->get_key() < key){
            current = current->forward[i];
        }
        update[i] = current; // 记录每一层待删除节点的前驱
    }

    current = current->forward[0];
    // 确认找到了待删除的节点
    if(current != NULL && current->get_key() == key){
        // 逐层更新指针，移除节点
        for(int i = 0; i <= max_level_; i++){
            if(update[i]->forward[i] != current) break;
            update[i]->forward[i] = current->forward[i];
        }
        // 调整跳表的层级
        while(max_level_ > 0 && head_->forward[max_level_] == NULL){
            max_level_--;
        }
        delete current;
        node_count_--;
    }
    mtx.unlock();
    return;
}

template<typename K, typename V>
void SkipList<K,V>::display_list(){
    // 从最上层开始向下遍历所有层
    for(int i = max_level_; i >= 0; i--){
        Node<K,V>* node = this->head_->forward[i]; // 获取当前层的头节点
        std::cout << "Level " << i << ": ";
        // 遍历当前层的所有节点
        while(node != nullptr){
            // 打印当前节点的键和值，键值之间用“：”分隔
            std::cout << node->get_key() << ":" << node->get_value() << ":";
            // 移动到当前层的下一个节点
            node = node->forward[i];
        }
        std::cout << std::endl; //当前层遍历结束，换行
    }
}

template<typename K, typename V>
void SkipList<K,V>::dump_file(){
    file_writer_.open(STORE_FILE); // 打开文件
    Node<K,V>* node = this->head_->forward[0]; // 从头节点开始遍历

    while(node != nullptr){
        file_writer_ << node->get_key() << ":" << node->get_value() << ";\n"; //写入键值对
        node = node->forward[0]; // 移动到下一个节点
    }

    file_writer_.flush(); // 刷新缓冲区，确保数据完全写入
    file_writer_.close(); // 关闭文件
}

// 该函数是否是有效字符串
template<typename K, typename V>
bool SkipList<K,V>::is_valid_string(const std::string& str){
    return !str.empty() && str.find(delimiter) != std::string::npos;
}

// 从字符串中获取键值对
template<typename K, typename V>
void SkipList<K,V>::get_key_value_from_string(const std::string &str, std::string *key, std::string *value){
    if(!is_valid_string(str)){
        return;
    }
    *key = str.substr(0, str.find(delimiter)); //substr函数是前闭后开
    *value = str.substr(str.find(delimiter) + 1, str.length());
}
template<typename K, typename V>
void SkipList<K,V>::load_file(){
    file_reader_.open(STORE_FILE);
    std::string line;
    std::string *key = new std::string();
    std::string *value = new std::string();

    while(getline(file_reader_, line)){
        get_key_value_from_string(line, key, value);
        if(key->empty() || value->empty()){
            continue;
        }
        // 将key定义为int类型
        insert_element(stoi(*key), *value);
        std::cout << "key:" << *key << "value:" << *value << std::endl;
    }

    delete key;
    delete value;
    file_reader_.close();
}

template<typename K, typename V>
void SkipList<K,V>::clear(Node<K,V>* node){
    if(node == nullptr) return;
    clear(node->forward[0]);
    delete node;
}

template<typename K, typename V>
int SkipList<K,V>::size(){
    return node_count_;
}