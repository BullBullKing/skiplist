/* ************************************************************************
> File Name:     skiplist.h
> Author:        BullBullKing
> Created Time:  2026-04-27 20:21:41
> Description:   The header file for Skip List, containing the definitions of Node and SkipList classes
 ************************************************************************/

 #ifndef SKIPLIST_H
 #define SKIPLIST_H

#include <iostream>      // 引入输入输出流库，用于控制台打印
#include <cstdlib>       // 引入C标准库，包含rand()随机数函数
#include <cstring>       // 引入C字符串库，用于memset内存操作
#include <mutex>         // 引入互斥锁库，用于多线程安全
#include <fstream>       // 引入文件流库，用于持久化存储

#define STORE_FILE "store/dumpFile"  // 持久化文件路径

std::mutex mtx;                 // 定义全局互斥锁，保护跳表的并发写操作
std::string delimiter = ":";   // 定义键值对分隔符，用于文件存储格式






template<typename K, typename V>
class Node{
public:
    Node(){}   
    Node(K key, V value, int level); 
    ~Node();

    K getKey() const;           // 获取键
    V getValue() const;         // 获取值
    void setValue(V value);     // 设置值


public:
    Node<K, V> **forward = nullptr; // forward数组，存储该节点在不同层级的后继指针
    int nodeLevel = 0;              // 当前节点的层数（有效层数）

private:

    K key;      // 节点存储的键
    V value;    // 节点存储的值

};




template<typename K, typename V>
class SkipList{
public:
    SkipList(int maxLevel);
    ~SkipList();

    int getRandomLevel();                                   // 生成随机层数
    Node<K, V>* createNode(K key, V value, int level);      // 创建新节点
    int put(K key, V value);                        // 插入键值对，返回1表示键已存在，0表示插入成功
    bool find(K key);                                  // 查找指定键是否存在
    void remove(K key);                                // 删除指定键的节点
    void clear(Node<K, V>* cur);                       // 递归删除所有节点
    int size();                                        // 返回跳表中元素个数
    void displayList();                 // 打印整个跳表（所有层）
    void dumpFile();                    // 将跳表数据写入文件
    void loadFile();                    // 从文件加载数据到跳表

private:
    void getKeyValueFromString(const std::string& str, std::string* key, std::string* value);   // 从"key:value"字符串中解析出键和值
    bool isValidString(const std::string& str);                                                 // 检查字符串是否合法（非空且包含分隔符）


private:
    int m_maxLevel;                 // 跳表允许的最大层数
    int m_skipListLevel = 0;            // 当前跳表实际的最大层数（从0开始）
    int m_elementCount = 0;             // 跳表中实际存储的元素个数
    Node<K, V> *m_header;           // 头节点（哨兵），不存储实际数据
    std::ofstream m_fileWriter;     // 文件输出流，用于写入文件
    std::ifstream m_fileReader;     // 文件输入流，用于读取文件


};






template<typename K, typename V>
// Node带参构造函数实现
Node<K, V>::Node(K key, V value, int level): key(key), value(value), nodeLevel(level){

    // 分配forward数组，长度为level+1（索引0..level）
    this->forward = new Node<K, V>* [level+1];
    memset(this->forward, 0, sizeof(Node<K, V>*)* (level+1));
}


// Node析构函数：释放forward动态数组
template<typename K, typename V>
Node<K, V>::~Node(){
    delete []forward;   // 删除指针数组
}

// 获取键
template<typename K, typename V>
K Node<K, V>::getKey() const {
    return this->key; 
}

// 获取值
template<typename K, typename V>
V Node<K, V>::getValue() const {
    return this->value;  
}





// 跳表构造函数: 初始化最大层数, 当前层数, 元素计数, 头节点
template<typename K, typename V>
SkipList<K, V>::SkipList(int maxLevel): m_maxLevel(maxLevel){
    K k;
    V v;
    this->m_header = new Node<K, V>(k, v, m_maxLevel);   // 分配头节点
}


// 析构函数: 关闭文件, 递归删除所有节点, 最后删除头节点
template<typename K, typename V>
SkipList<K, V>::~SkipList() {
    if( this->m_fileWriter.is_open() ){
        this->m_fileWriter.close();   
    }
    if( this->m_fileReader.is_open() ){
        this->m_fileReader.close();   
    }

    // 如果头节点最底层有后续节点，递归清除整个链条
    if(this->m_header->forward[0]!=nullptr){
        clear(this->m_header->forward[0]);   // 递归删除所有实际节点
    }
    delete this->m_header;   // 删除头节点

}


// 随机生成层数: 每次抛硬币(rand()%2), 连续正面则k++, 限制不超过m_maxLevel
template<typename K, typename V>
int SkipList<K, V>::getRandomLevel(){
    int k = 1;  // 初始层数为1

    while( rand()%2 ){
        k++;
    }
    k =  k<this->m_maxLevel ? k : this->m_maxLevel ;
    return k;
}


// 创建新节点, 新节点在堆内存中分配
template<typename K, typename V>
Node<K, V>* SkipList<K, V>::createNode(K key, V value, int level){
    Node<K, V>* node = new Node<K, V>(key, value, level);
    return  node;
}


// 插入元素的核心逻辑（线程安全）
template<typename K, typename V>
int SkipList<K, V>::put(K key, V value){
    mtx.lock();
    Node<K, V>* current = this->m_header;

    // 创建update数组，长度为m_maxLevel+1，用于记录每层需要更新的前驱节点
    Node<K, V>* update[this->m_maxLevel+1];
    memset(update, 0, sizeof(Node<K, V>*)* (this->m_maxLevel+1)); 

    // 从当前最高层向下查找，记录每层中小于key的最大节点
    for(int i = this->m_skipListLevel; i>=0; i--){
        while (current->forward[i] != nullptr && current->forward[i]->getKey() < key){
            current = current->forward[i];
        }
        update[i] = current;   // 记录每层的前驱节点
    }

    // 移动到最底层（第0层）的下一个节点
    current = current->forward[0];

    // 如果当前节点存在且键相等，则表示键已存在
    if (current != nullptr && current->getKey() == key){
        printf("Put: [%d, %s], exists\n", key, current->getValue().c_str()); 
        mtx.unlock();
        return 1;   // 返回1表示键已存在
    }

/*
Level 3: HEAD → 9
Level 2: HEAD → 5 → 9
Level 1: HEAD → 3 → 5 → 7 → 9
Level 0: HEAD → 1 → 2 → 3 → 4 → 5 → 6 → 7 → 8 → 9
*/

    // 键不存在，执行插入
    if( current == nullptr || current->getKey() != key ){
        int randomLevel = this->getRandomLevel();
        // 如果随机层数大于当前跳表的最大层数，则需要更新update数组的更高层
        if( randomLevel > this->m_skipListLevel ){
            for( int i = this->m_skipListLevel+1; i<randomLevel+1; i++ ){
                update[i] = this->m_header;   // 将多出的层的前驱指向头节点
            }
            this->m_skipListLevel = randomLevel;   // 更新当前跳表的最大层数
        }

        // 创建新节点
        Node<K, V>* newNode = createNode(key, value, randomLevel);
        // 在每个层的前驱节点后插入新节点: 修改前驱和后继指针
        for( int i = 0; i<=randomLevel; i++ ){
            newNode->forward[i] = update[i]->forward[i];   // 新节点的下一个指向前驱的下一个
            update[i]->forward[i] = newNode;               // 前驱的下一个指向新节点
        }
        printf("Put: [%d, %s], randomLevel: %d\n", key, value.c_str(), randomLevel);
        this->m_elementCount++;
    }
    mtx.unlock();
    return 0;
}


// 查找元素（不加锁，因为只读）
template<typename K, typename V>
bool SkipList<K, V>::find(K key){
    Node<K, V>* current = m_header;

    // 从当前最高层向下查找
    for( int i = m_skipListLevel; i>=0; i-- ){
        while( current->forward[i] != nullptr && current->forward[i]->getKey() < key ){
            current = current->forward[i];   // 同层向右移动
        }
    }
    // 移动到最底层（第0层）的下一个节点
    current = current->forward[0];

    if( current != nullptr && current->getKey() == key ){
        printf("Find: [%d, %s]\n", key, current->getValue().c_str());   // 找到，打印值
        return true;
    }
    printf("Find: %d not found\n", key);
    return false;
}


// 删除元素的核心逻辑（线程安全）
template<typename K, typename V>
void SkipList<K, V>::remove(K key){
    mtx.lock();
    Node<K, V>* current = this->m_header;
    Node<K, V>* update[this->m_maxLevel+1];
    memset(update, 0, sizeof(Node<K, V>*)* (this->m_maxLevel+1));

    // 从最高层向下查找，记录每层的前驱节点
    for( int i = this->m_skipListLevel; i>=0; i-- ){
        while( current->forward[i] != nullptr && current->forward[i]->getKey() < key ){
            current = current->forward[i];   // 同层向右移动
        }
        update[i] = current;   // 记录该层的前驱节点
    }
    current = current->forward[0];   // 到达最底层待删除节点的位置

/*
Level 3: HEAD → 9
Level 2: HEAD → 5 → 9
Level 1: HEAD → 3 → 5 → 7 → 9
Level 0: HEAD → 1 → 2 → 3 → 4 → 5 → 6 → 7 → 8 → 9
*/

    if( current != nullptr && current->getKey() == key ){
        // 从第0层到最高层, 如果该层前驱指向的就是待删节点, 则调整指针
        for( int i = 0; i<=this->m_skipListLevel; i++ ){
            if( update[i]->forward[i] != current )
                break;  // 如果该层前驱的下一个不是待删节点, 说明高层已不再指向它, 直接退出
            update[i]->forward[i] = current->forward[i];   // 前驱指向待删节点的下一个节点
        }
        // 删除后, 如果最高层已经为空, 则降低m_skipListLevel
        while( this->m_skipListLevel > 0 && this->m_header->forward[this->m_skipListLevel] == nullptr ){
            this->m_skipListLevel--;
        }
        printf("Remove: %d\n", key);   // 删除成功提示
        delete current;   // 释放节点内存
        this->m_elementCount--;
    }
    mtx.unlock();
    return;
}


// 递归删除所有节点
template<typename K, typename V>
void SkipList<K, V>::clear(Node<K, V>* cur){
    if( cur->forward[0] != nullptr ){
        clear(cur->forward[0]);   // 先递归到尾部
    }
    delete cur;   // 释放当前节点内存
}


// 返回跳表中元素个数
template<typename K, typename V>
int SkipList<K, V>::size(){
    return this->m_elementCount;
}


// 打印跳表
template<typename K, typename V>
void SkipList<K, V>::displayList(){
    std::cout << "\n*****Skip List*****"<<"\n";
    for( int i = this->m_skipListLevel; i>=0; i-- ){
        Node<K, V>* cur = this->m_header->forward[i];   // 第i层的第一个实际节点
        std::cout << "Level " << i << ": ";   // 打印层数
        while( cur != nullptr ){
            std::cout << cur->getKey() << ":" << cur->getValue() << " ";   // 打印键值对，分号分隔
            cur = cur->forward[i];   // 同一层向右移动
        }
        std::cout << std::endl;   // 每层打印完换行
    }
}


// 将跳表写入磁盘文件(只写最底层数据)
template<typename K, typename V>
void SkipList<K, V>::dumpFile(){
    std::cout << "Dumping skip list to file..." << std::endl;
    this->m_fileWriter.open(STORE_FILE, std::ios::out | std::ios::trunc);   // 以输出模式打开文件，清空原有内容
    Node<K, V>* cur = this->m_header->forward[0];

    while( cur != nullptr ){
        this->m_fileWriter << cur->getKey() << ":" << cur->getValue() << std::endl;   // 写入键值对
        cur = cur->forward[0];   // 同一层向右移动
    }
    this->m_fileWriter.flush();
    this->m_fileWriter.close();
    return;
}


// 从磁盘文件加载数据到跳表
template<typename K, typename V>
void SkipList<K, V>::loadFile(){
    this->m_fileReader.open(STORE_FILE, std::ios::in);
    std::cout << "Loading skip list from file..." << std::endl;
    std::string line;
    std::string* key = new std::string();
    std::string* value = new std::string();
        while (getline(this->m_fileReader, line)) {   // 逐行读取
        getKeyValueFromString(line, key, value);   // 解析该行得到键和值
        if (key->empty() || value->empty()) {
            continue;   // 非法行跳过
        }
        // 注意：这里假设键是整型，调用stoi转换，值保持字符串
        this->put(stoi(*key), *value);   // 插入到跳表
        std::cout << "key:" << *key << "value:" << *value << std::endl;   // 打印提示
    }
    delete key;  
    delete value; 
    this->m_fileReader.close();  
}


// 解析"key:value"字符串，分隔符为":"
template<typename K, typename V>
void SkipList<K, V>::getKeyValueFromString(const std::string& str, std::string* key, std::string* value){
    if( !isValidString(str) ){
        return ;
    }
    *key = str.substr(0, str.find(delimiter));
    *value = str.substr(str.find(delimiter)+1, str.length());
    return;
}


// 检查字符串是否合法（非空且包含分隔符）
template<typename K, typename V>
bool SkipList<K, V>::isValidString(const std::string& str){
    if( str.empty() ){
        return false;  
    }
    if( str.find(delimiter) == std::string::npos ){
        return false;  
    }
    return true;
}





#endif