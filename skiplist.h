#ifndef KVENGINE_SKIPLIST_H
#define KVENGINE_SKIPLIST_H

#include <algorithm>
#include <atomic>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <cctype>
#include <chrono>
#include <mutex>
#include <memory>
#include <iomanip>
#include <vector>
#include <Windows.h>
#include <sstream>
#include <string>
#include <thread>

/* 引入RapidJSON库头文件 */
#include <document.h>
#include <istreamwrapper.h>
#include <ostreamwrapper.h>
#include <writer.h>

#include "logMod.h"

#define STORE_FILE "store/dumpFile" // 宏定义数据持久化文件路径和文件名
#define CHRONO_STORE_FILE_NAME "chrono_dump_file"    //宏定义定时数据持久化基础文件名

extern std::mutex mtx;           // 互斥锁，保护临界区资源
extern std::string delimiter;    //  键值对之间的分隔符

/**
 * @brief 节点类
 * 
 * 表示跳表中的一个节点。
 * 
 * @tparam K 键的类型
 * @tparam V 值的类型
 */
template<typename K, typename V>
class Node
{
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
     * @brief 获取存储在节点中的值的引用。
     * 
     * 此方法返回一个引用，指向节点中存储的值。通过这个引用，可以直接读取或修改该值。这使得对节点值的操作更为直接和高效。
     * 
     * 注意：返回的引用直接关联于节点内部的值。因此，对返回的引用所做的任何修改都会直接影响到节点内部的值。使用时请谨慎，
     * 以避免不希望的数据修改。
     * 
     * @tparam K 节点键的类型。
     * @tparam V 节点值的类型。
     * 
     * @return V& 指向节点内部值的引用。通过这个引用，可以读取或修改节点的值。
     * 
     * @exception none 此方法不抛出异常。
     * 
     * @example
     * 
     * Node<int, std::string> myNode(1, "value");
     * std::string& valRef = myNode.get_value();
     * valRef = "new value"; // 直接修改节点中的值
     * 
     * 使用此方法时，必须确保节点在引用期间保持有效，以防止悬挂引用。
     */
    V &get_value();         // 非const版本的get_value方法，返回值的引用

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
Node<K, V>::Node(const K k, const V v, int level)
{
    this->key = k;
    this->value = v;
    this->node_level = level;

    // 数组大小为 0-level 的个数，level + 1
    this->forward = new Node<K, V>*[level+1];

    // 初始化指针数组 NULL(0)
    memset(this->forward, 0, sizeof(Node<K, V>*)*(level+1));
};

template<typename K, typename V>
Node<K, V>::~Node()
{
    delete []forward;   // delete [] ，确保释放整个数组
};

//  获取键
template<typename K, typename V>
K Node<K, V>::get_key() const
{
    return key;
};

//  获取键->值
template<typename K, typename V>
V Node<K, V>::get_value() const
{
    return value;
};

// 获取节点储存的值的引用
// 返回值类型为V的引用，允许直接修改节点储存的值
template<typename K, typename V>
V& Node<K, V>::get_value()
{
    return value;
};

//  设定节点的值为 value
template<typename K, typename V>
void Node<K, V>::set_value(V value)
{
    this->value=value;
};

/**
 * @brief 模板类跳表数据结构
 * 
 * SkipList 类是一个实现了跳表功能的数据结构，用于高效地存储和检索键值对元素。
 * 跳表是一种随机化数据结构，具有类似于平衡树的性能，但实现更加简单。
 * 
 * @tparam K 键的类型
 * @tparam V 值的类型
 * 
 * @note    键值中的key用int型，如果用其他类型，需要自定义比较函数，同时需要修改skipList.load_file函数
 */
template <typename K, typename V>
class SkipList
{
public:

    /**
     * @brief 构造函数：创建新的跳表对象
     * 
     * 构造一个新的跳表对象，指定跳表的最大层级数，并初始化跳表的各项属性。
     * 
     * @param max_level 跳表的最大层级数
     */
    SkipList(int);

    /**
     * @brief 销毁跳表对象
     * 
     * 清理跳表对象的资源，包括关闭文件流并释放所有节点所占用的内存。
     * 
     * 析构函数，销毁skiplist类实例对象。
     * 
     * 注意：调用该函数将会导致跳表对象及其所有节点被销毁，不再可用。
     */
    ~SkipList();

    /**
     * @brief 获取随机层级
     * 
     * 根据跳表的随机性质，生成一个随机层级数，用于新节点的插入。
     * 
     * @return 返回生成的随机层级数
     */
    int get_random_level();

    /**
     * @brief 创建一个节点对象
     * 
     * 创建一个新的节点对象，用于在跳表中存储键值对元素。
     * 
     * @param k 节点的键
     * @param v 节点的值
     * @param level 节点的层级（高度）
     * @return 返回指向新创建的节点对象的指针
     */
    Node<K, V>* create_node(K, V, int);

    /**
     * @brief 向跳表中插入新的键值对元素
     * 
     * 从跳表的顶层开始查找插入位置，并逐层向下查找，找到需要插入的位置后进行插入操作。
     * 如果插入的键已存在于跳表中，则不执行插入操作。
     * 
     * @param key 要插入的键
     * @param value 要插入的值
     * @return 如果成功插入元素，则返回 0；如果元素已存在于跳表中，则返回 1
     */
    int insert_element(K, V);

    /**
     * @brief 修改指定键的值。
     * 
     * 如果跳表中存在指定键的元素，则更新其值。
     * 
     * @param key 要修改的键
     * @param value 新的值
     * @return 如果跳表中存在键且值被成功修改，则返回 true；如果键不存在，则返回 false
     */
    bool update_element(K key, V value);

    /**
     * 更新跳表中的元素值。
     *
     * 根据给定的键（key），这个方法尝试在跳表中找到对应的元素，
     * 并用新的值（new_value）更新找到的元素的值。如果元素被成功找到并更新，
     * 方法将返回true；如果给定的键在跳表中不存在，方法将返回false。
     *
     * @param key 要更新的元素的键。这个键用于在跳表中定位元素。
     * @param new_value 用于更新元素值的新值。
     * @param old_value 一个引用参数，用于存储被更新的元素的旧值。
     *        如果方法成功找到并更新元素，旧值将通过这个参数返回。
     * @return bool 如果元素被成功找到并更新，返回true。
     *         如果给定的键在跳表中不存在，返回false。
     *
     * 注意：这个方法使用search_element_value来定位旧值，然后更新元素值。
     *       如果定位和更新操作成功，将通过标准输出打印更新信息。
     */
    bool update_element_value(K key, V new_vlaue, V &old_value);

    /**
     * @brief 显示跳表的内容。
     *
     * @details
     * 此方法遍历跳表的每个层级，打印出每一层的节点键值对。首先将每个层级的字符串表示收集到一个字符串向量中。
     * 接着确定最长的字符串长度，并据此创建一个居中的标题。最后，输出标题和每一层的内容，
     * 确保标题居中显示，并且每一层的末尾都有一个竖线 "|"。
     * 
     * 实现步骤如下：
     * 1. 通过一个向量存储每一层的字符串表示。
     * 2. 通过遍历这个向量来确定最长行的长度。
     * 3. 创建一个与最长行长度相匹配的居中标题。
     * 4. 如果标题长度不足，通过添加空格来确保它至少与最长行等长。
     * 5. 按顺序打印标题和各层级的内容。
     *
     * @note
     * - 该方法假定跳表中的节点已经按照从上到下、从左到右的顺序排列。
     * - 为了美观，每一层的开头都会有"Level x: "的前缀，x为层级编号。
     * - 每一层结束后都会打印一个额外的竖线 "|", 以标示该层级的结束。
     * - 输出结果中的标题"***** Skip List *****"将会根据最长行的长度动态居中。
     */
    void display_list();

    /**
     * @brief 搜索指定键的元素是否存在于跳表中
     * 
     * @param key 要搜索的键
     * @return 如果跳表中存在指定键的元素，则返回 true；否则返回 false
     */
    bool search_element(K);

    /**
     * @brief 查找元素值
     * 
     * 在跳表中搜索具有给定键的节点，并返回一个指向该节点值的指针。如果节点存在，您将可以通过返回的指针直接访问和修改值。
     * 如果找不到具有指定键的节点，则返回nullptr。
     * 
     * @tparam K 节点键值的类型。
     * @tparam V 节点存储值的类型。
     * 
     * @param key 要搜索的节点的键。
     * 
     * @return V* 如果找到了具有指定键的节点，返回指向节点值的指针；如果没有找到，返回nullptr。
     * 
     * @note 如果函数返回了一个非空指针，必须确保在使用指针期间跳表的内容不被修改，因为这样可能会使指针失效。
     * 
     * @exception none 此方法不抛出任何异常。
     */
    V* search_element_value(K key);

    /**
     * @brief 从跳表中删除指定键的元素
     * 
     * @param key 要删除的键
     */
    void delete_element(K);

    /**
     * @brief 将内存中的数据持久化到本地磁盘文件中
     * 
     * 遍历跳表并将其中的键值对写入文件中，用于数据持久化。
     */
    void dump_file();

    /**
     * @brief 从文件加载数据到跳表中
     * 
     * 打开指定文件并从中读取数据，将键值对插入到跳表中。
     */
    void load_file();

    /**
     * @brief 返回跳表中元素的数量
     * 
     * @return 返回跳表中元素的数量
     */
    int size();

    /**
     * @brief 清空跳表
     * 
     * 清空跳表中的所有节点，并重置跳表的状态。
     * 
     * @tparam K 键的类型
     * @tparam V 值的类型
     */
    void clear();

    /**
     * @brief 从指定的 JSON 文件中加载数据，并将这些数据插入跳表中。
     *
     * 此方法打开一个 JSON 文件，读取键值对数组，并依次将它们插入到跳表中。
     * 键值对应该是 JSON 对象的形式，并且每个对象都应该有 "key" 和 "value" 两个字段。
     * 如果文件无法打开，或者 JSON 格式不正确（例如，不是一个数组，或者数组中的元素缺少 "key" 或 "value" 字段），
     * 将输出错误信息并返回。
     *
     * @param file_name JSON 文件的路径和名称。
     * @tparam K 跳表中键的类型。注意，此类型必须与 JSON 文件中的键类型兼容。
     * @tparam V 跳表中值的类型。注意，此类型必须与 JSON 文件中的值类型兼容。
     * @note 此方法假设键类型 K 可以从 JSON 的 int 类型转换而来，值类型 V 可以从 JSON 的 string 类型转换而来。
     *       如果 K 和 V 的类型与此不同，需要对代码进行适当修改。
     */
    void load_from_json(const std::string &file_name);

    /**
     * @brief 保存跳表内容到JSON文件。此函数将当前跳表的键值对以JSON格式保存到指定的文件中。
     *        文件名将包含基础文件名与当前时间戳，确保每个文件都是唯一的。文件将被保存到指定的绝对路径下。
     * 
     * @tparam K 跳表键的类型。
     * @tparam V 跳表值的类型。
     * @param basic_file_name 基础文件名，不包含路径和文件扩展名。函数将在此基础上添加时间戳和.json扩展名。
     * 
     * @details
     * 函数执行流程如下：
     * 1. 获取当前时间，并转化为字符串形式，用以生成唯一的文件名。
     * 2. 构造完整的文件名，包含事先设定的绝对路径和".json"扩展名。
     * 3. 打开文件输出流，准备写入数据。
     *    - 如果无法打开文件，函数将输出错误信息并提前返回。
     * 4. 利用RapidJSON库创建一个空的JSON数组。
     * 5. 遍历跳表的所有节点，为每一个键值对创建JSON对象，并添加到JSON数组中。
     * 6. 将JSON数组写入文件。
     * 7. 关闭文件输出流。
     * 
     * 注意：确保有写权限到指定的文件夹路径，并且RapidJSON库已经被正确引入到项目中。
     * 
     * 使用示例：
     * SkipList<int, std::string> skiplist;
     * skiplist.save_to_json("my_skiplist_data");
     * 将生成文件名类似于 "C:/SoftWare/VScode-dir/KVengine_cpp/store/my_skiplist_data_2024-01-01_12-00-00.json"
     */
    void save_to_json(const std::string &basic_file_name);

    /**
     * @brief 比较当前跳表与另一个跳表的最低层中的键值对是否完全一致。
     * 
     * @details
     * 此方法执行一个逐节点对比的操作，比较每个节点的键和值：
     * - 如果所有对应节点的键和值均相等，则认为两个跳表在最低层是一致的。
     * - 如果在任何点上发现键或值不相等，或者两个跳表的节点数量不同，则认为它们不一致。
     * 
     * @tparam K 键的类型。
     * @tparam V 值的类型。
     * @param other 与当前跳表进行对比的另一个跳表对象的引用。
     * @return 如果两个跳表在最低层完全一致，则返回true；否则返回false。
     */
    bool skiplist_equals(const SkipList<K, V>& other) const;

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
Node<K, V>* SkipList<K, V>::create_node(const K k, const V v, int level)
{
    Node<K, V> *n = new Node<K, V>(k, v, level);
    return n;
}

// 根据给定键值对，将元素插入到跳表中
// return 1 意味着 元素已在跳表中
// return 0 意味着 元素插入成功
template<typename K, typename V>
int SkipList<K, V>::insert_element(const K key, const V value)
{

    mtx.lock(); // 加互斥锁，保障并发安全
    Node<K, V> *current = this->_header;

    //  update数组保存插入节点的前一个节点
    Node<K, V> *update[_max_level+1];
    memset(update, 0, sizeof(Node<K, V>*)*(_max_level+1));  //  初始化NULL

    // 从跳表的最高层级开始
    for(int i = _skip_list_level; i >= 0; i--)
    {
        //  当前层下一节点存在 且 下一节点的key 小于 参数key
        while(current->forward[i] != NULL && current->forward[i]->get_key() < key)
        {
            //  向右走
            current = current->forward[i];
        }
        update[i] = current;    //  更新update数组 当前层的节点
    }

    // 达到最底层，将current指向插入位置右侧节点
    current = current->forward[0];

    // 如果当前节点存在 且 key==传入的参数key
    if (current != NULL && current->get_key() == key)
    {
        //std::cout << "key: " << key << ", exists" << std::endl;
        mtx.unlock();   //  解锁互斥量
        return 1;   //  元素已在跳表中(根据key判断)
    }

    // 如果 current 为 NULL ，说明到达当前层尾部
    //  如果 current 的键值不等于 key，意味着 需要在 update[0] 和 current 节点之间插入新节点
    if (current == NULL || current->get_key() != key )
    {

        // 生成一个随机层级用于新节点
        int random_level = get_random_level();

        // 如果随机层级大于跳表的当前层级，将 update 数组的值初始化为指向头节点
        if (random_level > _skip_list_level)
        {
            for (int i = _skip_list_level+1; i < random_level+1; i++)
            {
                update[i] = _header;
            }
            _skip_list_level = random_level;    //  更新跳表层数
        }

        // 创建一个具有随机层级的新节点
        Node<K, V>* inserted_node = create_node(key, value, random_level);

        // 插入节点
        for (int i = 0; i <= random_level; i++)
        {
            inserted_node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = inserted_node;
        }
        //std::cout << "Successfully inserted key:" << key << ", value:" << value << std::endl;
        _element_count ++;
    }
    mtx.unlock();
    return 0;   //  表示插入成功
}

// 更新跳表中键为key的节点的值为value
// 如果跳表中不存在该键，则返回false
template<typename K, typename V>
bool SkipList<K, V>::update_element(K key, V value)
{
    Node<K, V> *current = _header;

    // 从最高层开始向下搜索需要更新的节点
    for (int i = _skip_list_level; i >= 0; i--)
    {
        while (current->forward[i] && current->forward[i]->get_key() < key)
        {
            current = current->forward[i];
        }
    }
    
    // 到达第0层，前进到下一个节点
    current = current->forward[0];

    // 如果当前节点的键与搜索的键匹配，则更新值
    if (current != nullptr && current->get_key() == key)
    {
        current->set_value(value);
        return true;
    }

    // 如果跳表中不存在该键，则返回false
    return false;
}

//更新跳表中键位key的节点的值，并显示详细信息
template<typename K, typename V>
bool SkipList<K, V>::update_element_value(K key, V new_value, V &old_value)
{
    V* found_value = this->search_element_value(key); // 使用search_element_value方法来定位旧值

    if (found_value != nullptr)
    {
        old_value = *found_value;  // 存储旧值
        *found_value = new_value;  // 更新为新值

        // 如有需要，可以在这里输出详细信息
        std::cout << "Updated key " << key 
                  << " from: " << old_value 
                  << " to: " << new_value << std::endl;

        return true; // 成功找到并更新了元素
    }

    return false; // 未找到元素
}

// 可视化跳表
template<typename K, typename V>
void SkipList<K, V>::display_list()
{
    LOG_INFO << "Beginning display of SkipList.";
    std::vector<std::string> lines; // 用于存储每一层的字符串
  
    // 遍历所有层级
    for (int level = _skip_list_level; level >= 0; --level)
    {
        std::ostringstream oss;
        oss << "Level " << level << ": ";

        Node<K, V> *node = this->_header->forward[level];
        while (node != nullptr)
        {
            oss << "|" << node->get_key() << ":" << node->get_value() << " ";
            node = node->forward[level];
        }
        oss << "|"; // 每层最后添加 "|"
        lines.push_back(oss.str()); // 将每层的字符串添加到lines中
    }

    // 找到最长的行
    std::string title = "***** Skip List *****";
    size_t max_length = title.size();
    for (const auto& line : lines)
    {
        if (line.size() > max_length)
        {
            max_length = line.size();
        }
    }

    // 创建标题，并确保它与最长行一样长且居中
    size_t title_padding = (max_length - title.size()) / 2;
    std::string full_title = std::string(title_padding, ' ') + title;

    // 如果标题不够长，确保它至少与最长行一样长
    while (full_title.size() < max_length)
    {
        full_title += " "; // 在标题末尾添加空格以确保长度足够
    }

    std::cout << full_title << '\n';

    // 输出每一层
    for (const auto& line : lines)
    {
        std::cout << line << '\n';
    }
    LOG_INFO << "Display of SkipList completed.";
}

// 内存中数据持久化到本地磁盘中的文件
template<typename K, typename V>
void SkipList<K, V>::dump_file()
{
    LOG_INFO << "Starting dump of SkipList to " << STORE_FILE;
    std::cout << "dump_file-----------------" << std::endl;
    //  打开文件
    _file_writer.open(STORE_FILE);
    //  从跳表最底层开始遍历节点
    Node<K, V> *node = this->_header->forward[0];

    //  遍历节点并将键值写入文件
    while (node != NULL)
    {
        _file_writer << node->get_key() << ":" << node->get_value() << "\n";
        std::cout << node->get_key() << ":" << node->get_value() << ";\n";
        node = node->forward[0];
    }

    _file_writer.flush();   //  刷新文件输出流
    _file_writer.close();   //  关闭文件
}

// 加载本地磁盘 文件中的数据
template<typename K, typename V>
void SkipList<K, V>::load_file()
{
    LOG_INFO << "Starting load data from SkipList " << STORE_FILE;
    _file_reader.open(STORE_FILE);
    std::cout << "load_file-----------------" << std::endl;
    //  保存从文件中读取的数据
    std::string line;

    /*std::string* key = new std::string();
    std::string* value = new std::string();*/

    //  使用智能指针管理，防止内存泄露
    std::unique_ptr<std::string> key(new std::string());
    std::unique_ptr<std::string> value(new std::string());
    //  逐行读取文件的内容
    while (getline(_file_reader, line))
    {
        get_key_value_from_string(line, key, value);    // 从每一行数据中提取键和值
        // 如果键或值为空，跳过该行数据
        if (key->empty() || value->empty())
        {
            continue;
        }
        insert_element(*key, *value);
        std::cout << "key:" << *key << "value:" << *value << std::endl;
    }

    //关闭文件输入流
    _file_reader.close();
}

//  获取跳表中元素的数量
template<typename K, typename V>
int SkipList<K, V>::size()
{
    return _element_count;
}

//  从字符串中提取key:value
template<typename K, typename V>
void SkipList<K, V>::get_key_value_from_string(const std::string& str, std::string* key, std::string* value)
{
    //  验证字符串是否有效
    if(!is_valid_string(str))
    {
        return;
    }
    *key = str.substr(0, str.find(delimiter));  //  获取key部分
    *value = str.substr(str.find(delimiter)+1, str.length());   //  获取value部分
}

//  验证字符串是否有效
template<typename K, typename V>
bool SkipList<K, V>::is_valid_string(const std::string& str)
{
    // str为空
    if (str.empty())
    {
        return false;
    }
    //  str中不包含 : 分隔符
    if (str.find(delimiter) == std::string::npos)
    {
        return false;
    }
    return true;
}

// 从跳表中删除元素
template<typename K, typename V>
void SkipList<K, V>::delete_element(K key)
{

    mtx.lock(); //  互斥锁，保障并发安全性
    Node<K, V> *current = this->_header;
    Node<K, V> *update[_max_level+1];   //  存储需要删除的节点前一个节点
    memset(update, 0, sizeof(Node<K, V>*)*(_max_level+1));  //初始化为NULL

    // 从跳表的最高层级开始查找需要删除的元素
    for (int i = _skip_list_level; i >= 0; i--)
    {
        //  当前层下一节点存在 且 下一节点key值 小于目标key值
        while (current->forward[i] !=NULL && current->forward[i]->get_key() < key)
        {
            current = current->forward[i];  //向右走
        }
        update[i] = current;    //  更新update数组
    }

    current = current->forward[0];
    //  不为空且键值相等 找到要删除的节点
    if (current != NULL && current->get_key() == key)
    {

        //  从删除节点的层级开始删除，向更低层级遍历，保证在更低的层级一定满足删除条件
        for(int i=current->node_level;i>=0;i--)
        {
            update[i]->forward[i] = current->forward[i];
        }

        // 删除无元素的层级,从最高层级开始遍历
        while (_skip_list_level > 0 && _header->forward[_skip_list_level] == 0)
        {
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
bool SkipList<K, V>::search_element(K key)
{
    Node<K, V> *current = _header;  //  初始化current为跳表头节点

    // 从跳表最高层级开始遍历
    for (int i = _skip_list_level; i >= 0; i--)
    {
        //  如果当前层 下一节点存在 且 key值<参数key值
        while (current->forward[i] && current->forward[i]->get_key() < key)
        {
            current = current->forward[i];  //向右走
        }
    }

    //  到达最底层，将节点移动到搜索元素的位置
    current = current->forward[0];

    // 如果当前节点的key值与参数key值相等，我们就找到了要搜索的节点
    if (current and current->get_key() == key)
    {
        return true;
    }
    return false;
}

// 在跳表中根据给定key值搜索元素，并将对应值返回
template<typename K, typename V>
V* SkipList<K, V>::search_element_value(K key)
{
    Node<K, V> *current = _header;

    for (int i = _skip_list_level; i >= 0; i--)
    {
        while (current->forward[i] && current->forward[i]->get_key() < key)
        {
            current = current->forward[i];
        }
    }

    current = current->forward[0];

    if (current != nullptr && current->get_key() == key) {
        return &(current->get_value()); //返回指向找到的元素值的指针
    }
    return nullptr; // 如果未找到，返回null指针
}

// 跳表 构造函数
template<typename K, typename V>
SkipList<K, V>::SkipList(int max_level)
{

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
SkipList<K, V>::~SkipList()
{
    LOG_INFO << "Destroying skiplist";
    if (_file_writer.is_open())
    {
        _file_writer.close();
    }
    if (_file_reader.is_open())
    {
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
int SkipList<K, V>::get_random_level()
{

    int k = 0;  //  初始化层级为0
    while (rand() % 2)
    {
        k++;
    }
    k = (k < _max_level) ? k : _max_level;  // 将层级数限制在 _max_level 范围内
    return k;
};

template <typename K, typename V>
void SkipList<K, V>::clear()
{
    LOG_INFO << "Starting SkipList clear operation.";
    // 遍历跳表的每一层，从最底层开始
    Node<K, V>* current = _header->forward[0];
    while (current != nullptr)
    {
        Node<K, V>* temp = current;
        current = current->forward[0]; // 移动到下一个节点
        delete temp; // 删除当前节点
    }

    // 重置头节点的每一层指向
    for (int i = 0; i < _max_level; ++i)
    {
        _header->forward[i] = nullptr;
    }

    // 重置跳表的当前层级和元素计数
    _skip_list_level = 0; // 假设跳表初始化时至少有一层
    _element_count = 0;
    LOG_INFO << "SkipList cleared successfully.";
}

template<typename K, typename V>
void SkipList<K, V>::load_from_json(const std::string& file_name)
{
    LOG_INFO << "Loading SkipList from JSON file: " << file_name;
    std::ifstream ifs(file_name);
    if (!ifs.is_open())
    {
        LOG_ERROR << "Error: Cannot open file " << file_name;
        std::cerr << "Error: Cannot open file " << file_name << std::endl;
        return;
    }

    rapidjson::IStreamWrapper isw(ifs);
    rapidjson::Document doc;
    doc.ParseStream(isw);
    ifs.close();

    if (!doc.IsArray())
    {
        LOG_ERROR << "Error: JSON file must contain an array of key-value pairs.";
        std::cerr << "Error: JSON file must contain an array of key-value pairs" << std::endl;
        return;
    }

    for (rapidjson::SizeType i = 0; i < doc.Size(); i++)
    {
        if (!doc[i].IsObject() || !doc[i].HasMember("key") || !doc[i].HasMember("value"))
        {
            std::cerr << "Error: Each key-value pair must be an object with 'key' and 'value'." << std::endl;
            continue;
        }
        try {
            if (!doc[i]["key"].IsInt() || !doc[i]["value"].IsString()) {
                std::cerr << "Error: Key or value type mismatch in element " << i << "." << std::endl;
                continue;
            }

            K key = doc[i]["key"].GetInt();
            V value = doc[i]["value"].GetString();
            insert_element(key, value);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error: Exception caught while processing key-value pair at index " << i << ": " << e.what() << std::endl;
            continue;
        }
    }
    LOG_INFO << "Successfully loaded " << doc.Size() << " elements from JSON.";
}

template<typename K, typename V>
void SkipList<K, V>::save_to_json(const std::string& basic_file_name)
{
    // 获取当前时间点
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);

    // 将当前时间转换为字符串
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d_%H-%M-%S");
    std::string time_str = ss.str();

    // 创建带有时间标签的文件名
    std::string file_name_with_time = "C:/SoftWare/VScode-dir/KVengine_cpp/store/" + basic_file_name + "_" + time_str + ".json";
    LOG_INFO << "Saving SkipList to JSON file: " << file_name_with_time;

    // 使用新的文件名打开文件输出流
    std::ofstream ofs(file_name_with_time);
    if (!ofs.is_open())
    {
        LOG_ERROR << "Error: Cannot open file " << file_name_with_time;
        std::cerr << "Error: Cannot open file " << file_name_with_time << std::endl;
        return;
    }

    // 写JSON数据到文件
    rapidjson::OStreamWrapper osw(ofs);
    rapidjson::Document doc;
    doc.SetArray();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

    // 遍历跳表节点，并将它们存储到JSON文件中
    Node<K, V>* node = this->_header->forward[0];
    while (node != nullptr)
    {
        rapidjson::Value obj(rapidjson::kObjectType);

        obj.AddMember("key", rapidjson::Value().SetInt(node->get_key()), allocator);
        obj.AddMember("value", rapidjson::Value().SetString(node->get_value().c_str(), allocator), allocator);

        doc.PushBack(obj, allocator);
        node = node->forward[0];
    }

    // 利用Writer类快速写入数据到文档流中
    rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
    doc.Accept(writer);

    // 关闭文件流
    ofs.close();

    LOG_INFO << "SkipList successfully saved to JSON.";
}

template<typename K, typename V>
bool SkipList<K, V>::skiplist_equals(const SkipList<K, V>& other) const
{
    Node<K, V>* currentThis = this->_header->forward[0];
    Node<K, V>* currentOther = other._header->forward[0];

    // 同时遍历两个跳表的最低层
    while (currentThis != nullptr && currentOther != nullptr)
    {
        // 检查键和值是否一致
        if (currentThis->get_key() != currentOther->get_key() || currentThis->get_value() != currentOther->get_value())
        {
            return false; // 发现不一致的键值对，立即返回false
        }
        
        // 移动到下一个节点
        currentThis = currentThis->forward[0];
        currentOther = currentOther->forward[0];
    }

    // 如果两个跳表的最低层同时遍历完毕，则它们一致；否则，不一致
    return currentThis == nullptr && currentOther == nullptr;
}

/**
 * SkipListConsole类提供了一个用于操作SkipList类的命令行接口。 
 * 你可以使用它来插入、删除、更新、搜索跳表中的元素以及显示跳表、清空跳表和退出程序。
 *
 * @tparam K SkipList中元素的键（Key）类型。
 * @tparam V SkipList中元素的值（Value）类型。
 */
template<typename K, typename V>
class SkipListConsole
{
public:
    /**
     * 构造函数，初始化SkipListConsole类的实例。
     *
     * @param list 一个引用，指向我们要操作的SkipList实例。
     */
    SkipListConsole(SkipList<K, V>& list) : _list(list) {}

    /**
     * run方法启动命令行接口并接受用户命令，直到用户选择退出。
     * 目前支持的命令包括INSERT, DELETE, UPDATE, SEARCH, DISPLAY, SIZE, CLEAR, EXIT。
     */
    void run()
    {
        std::string input;
        K key;
        V value;
        std::cout << "SkipList Console Interface" << std::endl;
        std::cout << "Available commands: INSERT <key> <value>, DELETE <key>, UPDATE <key> <value>, SEARCH <key>, DISPLAY, SIZE, CLEAR, EXIT" << std::endl;
        
        while (true)
        {
            std::cout << "> ";
            std::getline(std::cin, input);
            // 使用trim函数处理input，并直接更新input变量
            input = trim(input);
            
            if (input.empty()) continue; // 如果修剪后的输入为空，则跳过当前循环迭代

            std::istringstream iss(input);
            std::string command;
            iss >> command;

            if (command == "INSERT")
            {
                iss >> key >> value;
                _list.insert_element(key, value) ? std::cout << "Key already exists.\n" : std::cout << "Element inserted.\n";
            }
            else if (command == "DELETE")
            {
                iss >> key;
                _list.delete_element(key);
                std::cout << "Element deleted (if it existed).\n";
            }
            else if (command == "UPDATE")
            {
                K key;
                V new_value;
                V old_value;

                iss >> key >> new_value;
                if (_list.update_element_value(key, new_value, old_value))
                {
                    std::cout << "Element with key \"" << key
                            << "\" has been updated from \"" << old_value
                            << "\" to \"" << new_value << "\".\n";
                }
                else
                {
                    std::cout << "Element with key \"" << key << "\" not found.\n";
                }
            }
            else if (command == "SEARCH")
            {
                iss >> key;
                auto foundValue = _list.search_element_value(key);
                if (foundValue != nullptr)
                {
                    std::cout << "Element found. Key: " << key << ", Value: " << *foundValue << ".\n";
                }
                else
                {
                    std::cout << "Element not found.\n";
                }
            }
            else if (command == "DISPLAY")
            {
                _list.display_list();
            }
            else if (command == "SIZE")
            {
                std::cout << "Size: " << _list.size() << std::endl;
            }
            else if (command == "CLEAR")
            {
                _list.clear();
                std::cout << "List cleared.\n";
            }
            else if (command == "EXIT")
            {
                std::cout << "Exiting...\n";
                Sleep(1000); // 参数单位为毫秒

                break;
            }
            else
            {
                std::cout << "Unknown command.\n";
            }
        }
    }
private:
    SkipList<K, V>& _list;          // 要操作的SkipList实例的引用。

    /**
     * 静态函数trim用于删除字符串的开头和结尾的空格。
     *
     * @param str 待处理的字符串。
     * @return 删除开头和结尾的空格后的新字符串。
     */
    static std::string trim(const std::string &str)
    {
        size_t first = str.find_first_not_of(' ');
        if (std::string::npos == first)
        {
            return str;
        }
        size_t last = str.find_last_not_of(' ');
        return str.substr(first, (last - first + 1));
    }
};

/**
 * @class AutoSaveSkipList
 * @brief 继承自SkipList，增加了自动保存功能的跳表。
 * @details 该类扩展了SkipList，通过后台线程定期将跳表数据自动保存到JSON文件，以实现数据的持久化。
 * @tparam K SkipList中键的类型。
 * @tparam V SkipList中值的类型。
 */
template<typename K, typename V>
class AutoSaveSkipList : public SkipList<K, V>
{
    std::thread autoSaveThread;
    std::atomic<bool> stopAutoSaveThread = false;

    /**
     * @class AutoSaveSkipList
     * @brief 继承自SkipList，增加了自动保存功能的跳表。
     * @details 该类扩展了SkipList，通过后台线程定期将跳表数据自动保存到JSON文件，以实现数据的持久化。
     * @tparam K SkipList中键的类型。
     * @tparam V SkipList中值的类型。
     */
    void autoSaveRoutine(const std::string& filename, unsigned int intervalSeconds)
    {
        while (!stopAutoSaveThread.load())
        {
            std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));
            this->save_to_json(filename + "_autosave");
        }
    }

public:
    /**
     * @brief 构造函数，初始化跳表并启动自动保存线程。
     * @details 创建一个AutoSaveSkipList对象，同时启动一个后台线程，定期将跳表数据保存到指定文件。构造函数接受跳表的最大层级、保存文件的名字和自动保存的间隔时间作为参数。
     * @param maxLevel 跳表的最大层数。
     * @param filename 用于保存跳表数据的文件名。
     * @param intervalSeconds 自动保存到文件的时间间隔（秒）。
     */
    AutoSaveSkipList(int maxLevel, const std::string& filename, unsigned int intervalSeconds) 
        : SkipList<K, V>(maxLevel) {
        autoSaveThread = std::thread(&AutoSaveSkipList::autoSaveRoutine, this, filename, intervalSeconds);
    }

    /**
     * @brief 析构函数，确保自动保存线程被正确终止。
     * @details 在对象析构时，设置`stopAutoSaveThread`为true，并等待后台线程结束，确保资源被正确释放。
     * @note 如果自动保存线程处于活动状态，这将阻塞直至线程完成。
     */
    ~AutoSaveSkipList()
    {
        stopAutoSaveThread.store(true);
        if (autoSaveThread.joinable())
        {
            autoSaveThread.join();
        }
    }

    /**
     * @brief 拷贝构造函数。已删除以防止实例被拷贝。
     * @details 删除拷贝构造函数以防止跳过列表实例被不正确地拷贝，这可能会导致资源竞争等问题。
     */
    AutoSaveSkipList(const AutoSaveSkipList&) = delete;

    /**
     * @brief 拷贝赋值操作。已删除以防止实例之间的赋值。
     * @details 删除拷贝赋值操作以保证每个跳过列表实例是唯一的，避免不一致性和资源竞争问题。
     */
    AutoSaveSkipList& operator=(const AutoSaveSkipList&) = delete;
};

#endif //KVENGINE_SKIPLIST_H
