//
// Created by 80544 on 2023/8/26.
//

#ifndef KVENGINE_SKIPLIST_H
#define KVENGINE_SKIPLIST_H


#include <iostream>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <mutex>
#include <fstream>
#include <memory>

#define STORE_FILE "store/dumpFile" // 宏定义数据持久化文件路径和文件名

extern std::mutex mtx;           // 互斥锁，保护临界区资源
extern std::string delimiter;    //  键值对之间的分隔符

/**
 * @brief 模板类 Node，表示跳表的节点
 * 
 * @tparam K 键类型
 * @tparam V 值类型
 */
template<typename K, typename V>
class Node {

public:

    /**
     * @brief 默认构造函数
     * 
     * 创建一个空节点。
     */
    Node() {}

    /**
     * @brief 有参构造函数
     * 
     * 使用给定的键、值和层级创建一个节点。
     * 
     * @param k 节点的键
     * @param v 节点的值
     * @param level 节点的层级
     */
    Node(K k, V v, int);    //  有参构造函数

    /**
     * @brief 析构函数
     * 
     * 删除节点的指针数组，确保释放内存。
     */
    ~Node();    //  析构函数

    /**
     * @brief 获取节点的键
     * 
     * @return K 节点的键
     */
    K get_key() const;  //  获取键

    /**
     * @brief 获取节点的值
     * 
     * @return V 节点的值
     */
    V get_value() const;    //  获取值

    /**
     * @brief 设置节点的值
     * 
     * @param value 要设置的值
     */
    void set_value(V);  //  设定值

    //  指向下一节点的指针数组
    Node<K, V> **forward;

    int node_level;     // 节点所在层

private:
    K key;      // 节点的键，唯一标识节点
    V value;    //节点的值，键->数据
};

//  有参构造函数实现
template<typename K, typename V>
Node<K, V>::Node(const K k, const V v, int level) {
    this->key = k;
    this->value = v;
    this->node_level = level;

    // 数组大小为 0-level 的个数，level + 1
    this->forward = new Node<K, V>*[level+1];

    // 初始化指针数组 NULL(0)
    memset(this->forward, 0, sizeof(Node<K, V>*)*(level+1));
};

template<typename K, typename V>
Node<K, V>::~Node() {
    delete []forward;   // delete [] ，确保释放整个数组
};

//  获取键
template<typename K, typename V>
K Node<K, V>::get_key() const {
    return key;
};

//  获取键->值
template<typename K, typename V>
V Node<K, V>::get_value() const {
    return value;
};

//  设定节点的值为 value
template<typename K, typename V>
void Node<K, V>::set_value(V value) {
    this->value=value;
};

// 模板类 跳表
// 键值中的key用int型，如果用其他类型，需要自定义比较函数，同时需要修改skipList.load_file函数
template <typename K, typename V>
class SkipList {

public:
    SkipList(int);  //  构造函数，接受一个整数参数作为最大层级数
    ~SkipList();    //  析构函数，销毁skiplist类实例对象
    int get_random_level();    //  随机生成层数
    Node<K, V>* create_node(K, V, int);    //   创建节点，接受键、值、层数
    int insert_element(K, V);   //  插入元素
    void display_list();        //显示跳表
    bool search_element(K);     //  依靠键 搜索元素
    void delete_element(K);     //  依靠键 删除元素
    void dump_file();           //  数据持久化
    void load_file();           //  从文件加载跳表数据
    int size();                 //  跳表元素数量

private:
    void get_key_value_from_string(const std::string& str, std::string* key, std::string* value);   //  从字符串提取键值对
    bool is_valid_string(const std::string& str);   //  检查字符串是否有效

private:
    //  跳表最大层级数
    int _max_level;

    // 跳表当前层级数
    int _skip_list_level;

    // 指向跳表头结点的指针
    Node<K, V> *_header;

    // 输入输出流
    std::ofstream _file_writer;
    std::ifstream _file_reader;

    // 跳表中元素的数量
    int _element_count;
};

// 创建一个新节点
template<typename K, typename V>
Node<K, V>* SkipList<K, V>::create_node(const K k, const V v, int level) {
    Node<K, V> *n = new Node<K, V>(k, v, level);
    return n;
}

// 根据给定键值对，将元素插入到跳表中
// return 1 意味着 元素已在跳表中
// return 0 意味着 元素插入成功
template<typename K, typename V>
int SkipList<K, V>::insert_element(const K key, const V value) {

    mtx.lock(); // 加互斥锁，保障并发安全
    Node<K, V> *current = this->_header;

    //  update数组保存插入节点的前一个节点
    Node<K, V> *update[_max_level+1];
    memset(update, 0, sizeof(Node<K, V>*)*(_max_level+1));  //  初始化NULL

    // 从跳表的最高层级开始
    for(int i = _skip_list_level; i >= 0; i--) {
        //  当前层下一节点存在 且 下一节点的key 小于 参数key
        while(current->forward[i] != NULL && current->forward[i]->get_key() < key) {
            //  向右走
            current = current->forward[i];
        }
        update[i] = current;    //  更新update数组 当前层的节点
    }

    // 达到最底层，将current指向插入位置右侧节点
    current = current->forward[0];

    // 如果当前节点存在 且 key==传入的参数key
    if (current != NULL && current->get_key() == key) {
        //std::cout << "key: " << key << ", exists" << std::endl;
        mtx.unlock();   //  解锁互斥量
        return 1;   //  元素已在跳表中(根据key判断)
    }

    // 如果 current 为 NULL ，说明到达当前层尾部
    //  如果 current 的键值不等于 key，意味着 需要在 update[0] 和 current 节点之间插入新节点
    if (current == NULL || current->get_key() != key ) {

        // 生成一个随机层级用于新节点
        int random_level = get_random_level();

        // 如果随机层级大于跳表的当前层级，将 update 数组的值初始化为指向头节点
        if (random_level > _skip_list_level) {
            for (int i = _skip_list_level+1; i < random_level+1; i++) {
                update[i] = _header;
            }
            _skip_list_level = random_level;    //  更新跳表层数
        }

        // 创建一个具有随机层级的新节点
        Node<K, V>* inserted_node = create_node(key, value, random_level);

        // 插入节点
        for (int i = 0; i <= random_level; i++) {
            inserted_node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = inserted_node;
        }
        //std::cout << "Successfully inserted key:" << key << ", value:" << value << std::endl;
        _element_count ++;
    }
    mtx.unlock();
    return 0;   //  表示插入成功
}

// 可视化跳表
template<typename K, typename V>
void SkipList<K, V>::display_list() {

    std::cout << "\n*****Skip List*****"<<"\n";
    //  遍历所有层级
    for (int i = 0; i <= _skip_list_level; i++) {
        Node<K, V> *node = this->_header->forward[i];
        std::cout << "Level " << i << ": ";
        //  遍历当前层节点
        while (node != NULL) {
            std::cout << node->get_key() << ":" << node->get_value() << ";";
            node = node->forward[i];
        }
        std::cout << std::endl;
    }
}

// 内存中数据持久化到本地磁盘中的文件
template<typename K, typename V>
void SkipList<K, V>::dump_file() {

    std::cout << "dump_file-----------------" << std::endl;
    //  打开文件
    _file_writer.open(STORE_FILE);
    //  从跳表最底层开始遍历节点
    Node<K, V> *node = this->_header->forward[0];

    //  遍历节点并将键值写入文件
    while (node != NULL) {
        _file_writer << node->get_key() << ":" << node->get_value() << "\n";
        std::cout << node->get_key() << ":" << node->get_value() << ";\n";
        node = node->forward[0];
    }

    _file_writer.flush();   //  刷新文件输出流
    _file_writer.close();   //  关闭文件
}

// 加载本地磁盘 文件中的数据
template<typename K, typename V>
void SkipList<K, V>::load_file() {

    _file_reader.open(STORE_FILE);
    std::cout << "load_file-----------------" << std::endl;
    std::string line;       //  保存从文件中读取的数据
    /*std::string* key = new std::string();
    std::string* value = new std::string();*/
    std::unique_ptr<std::string> key(new std::string());    //  使用智能指针管理，防止内存泄露
    std::unique_ptr<std::string> value(new std::string());
    //  逐行读取文件的内容
    while (getline(_file_reader, line)) {
        get_key_value_from_string(line, key, value);    // 从每一行数据中提取键和值
        // 如果键或值为空，跳过该行数据
        if (key->empty() || value->empty()) {
            continue;
        }
        insert_element(*key, *value);
        std::cout << "key:" << *key << "value:" << *value << std::endl;
    }
    /*//  释放临时申请的内存
    delete key;
    delete value;*/
    //关闭文件输入流
    _file_reader.close();
}

//  获取跳表中元素的数量
template<typename K, typename V>
int SkipList<K, V>::size() {
    return _element_count;
}

//  从字符串中提取key:value
template<typename K, typename V>
void SkipList<K, V>::get_key_value_from_string(const std::string& str, std::string* key, std::string* value) {
    //  验证字符串是否有效
    if(!is_valid_string(str)) {
        return;
    }
    *key = str.substr(0, str.find(delimiter));  //  获取key部分
    *value = str.substr(str.find(delimiter)+1, str.length());   //  获取value部分
}

//  验证字符串是否有效
template<typename K, typename V>
bool SkipList<K, V>::is_valid_string(const std::string& str) {
    // str为空
    if (str.empty()) {
        return false;
    }
    //  str中不包含 : 分隔符
    if (str.find(delimiter) == std::string::npos) {
        return false;
    }
    return true;
}

// 从跳表中删除元素
template<typename K, typename V>
void SkipList<K, V>::delete_element(K key) {

    mtx.lock(); //  互斥锁，保障并发安全性
    Node<K, V> *current = this->_header;
    Node<K, V> *update[_max_level+1];   //  存储需要删除的节点前一个节点
    memset(update, 0, sizeof(Node<K, V>*)*(_max_level+1));  //初始化为NULL

    // 从跳表的最高层级开始查找需要删除的元素
    for (int i = _skip_list_level; i >= 0; i--) {
        //  当前层下一节点存在 且 下一节点key值 小于目标key值
        while (current->forward[i] !=NULL && current->forward[i]->get_key() < key) {
            current = current->forward[i];  //向右走
        }
        update[i] = current;    //  更新update数组
    }

    current = current->forward[0];
    //  不为空且键值相等 找到要删除的节点
    if (current != NULL && current->get_key() == key) {

        //  从删除节点的层级开始删除，向更低层级遍历，保证在更低的层级一定满足删除条件
        for(int i=current->node_level;i>=0;i--){
            update[i]->forward[i] = current->forward[i];
        }

        // 删除无元素的层级,从最高层级开始遍历
        while (_skip_list_level > 0 && _header->forward[_skip_list_level] == 0) {
            _skip_list_level --;
        }

        std::cout << "Successfully deleted key "<< key << std::endl;
        delete current;     // 释放删除的节点的内存
        _element_count --;  // 元素计数减一
    }
    mtx.unlock();   //  解锁互斥量
}

// 在跳表中根据给定key值搜索元素
template<typename K, typename V>
bool SkipList<K, V>::search_element(K key) {

    //std::cout << "search_element-----------------" << std::endl;
    Node<K, V> *current = _header;  //  初始化current为跳表头节点

    // 从跳表最高层级开始遍历
    for (int i = _skip_list_level; i >= 0; i--) {
        //  如果当前层 下一节点存在 且 key值<参数key值
        while (current->forward[i] && current->forward[i]->get_key() < key) {
            current = current->forward[i];  //向右走
        }
    }

    //  到达最底层，将节点移动到搜索元素的位置
    current = current->forward[0];

    // 如果当前节点的key值与参数key值相等，我们就找到了要搜索的节点
    if (current and current->get_key() == key) {
        //std::cout << "Found key: " << key << ", value: " << current->get_value() << std::endl;
        return true;
    }

    //std::cout << "Not Found Key:" << key << std::endl;
    return false;
}

// 跳表 构造函数
template<typename K, typename V>
SkipList<K, V>::SkipList(int max_level) {

    this->_max_level = max_level;   // 设置跳表的最大层级数
    this->_skip_list_level = 0;     // 初始化跳表的层级数为0
    this->_element_count = 0;       // 初始化跳表的元素计数为0

    // 创建头节点并将键和值初始化为 null
    K k;
    V v;
    this->_header = new Node<K, V>(k, v, _max_level);   // 创建头节点，指定最大层级数
};

//  跳表 析构函数，回收内存空间
template<typename K, typename V>
SkipList<K, V>::~SkipList() {

    if (_file_writer.is_open()) {
        _file_writer.close();
    }
    if (_file_reader.is_open()) {
        _file_reader.close();
    }

    //  循环删除节点
    Node<K, V>* current = _header->forward[0];
    while(current) {
        Node<K, V>* tmp = current->forward[0];
        delete current;
        current = tmp;
    }
    delete _header; //  删除头节点
}

//  随机生成层级数
template<typename K, typename V>
int SkipList<K, V>::get_random_level(){

    int k = 0;  //  初始化层级为0
    while (rand() % 2) {
        k++;
    }
    k = (k < _max_level) ? k : _max_level;  // 将层级数限制在 _max_level 范围内
    return k;
};

#endif //KVENGINE_SKIPLIST_H
