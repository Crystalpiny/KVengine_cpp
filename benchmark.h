#ifndef BENCHMARK_H
#define BENCHMARK_H

#define MULTI_NUM_FOR_INPUT (1000000)   //  用户输入数据量的乘数,简化用户操作

/**
 * @brief 初始化基准测试数据。
 * 
 * 该函数从标准输入中读取线程数量和测试数据量的值，并将它们分别赋给全局变量 THREAD_NUM 和 TEST_DATANUM。
 * 
 * @note 线程数量的最大值通常为16。
 * 
 * @return 返回指向 SkipList<int, std::string> 对象的 std::unique_ptr
 */
std::unique_ptr<SkipList<int, std::string>> init_benchmark_data();

/**
 * @brief 插入元素到跳表中的线程函数。
 * 
 * @param skipList 跳表对象的智能指针。
 * @param tid 当前线程的标识符。
 */
void insertElement(std::unique_ptr<SkipList<int, std::string>> &skipList, int tid);

/**
 * @brief 从跳表中搜索元素的线程函数。
 * 
 * @param skipList 跳表对象的智能指针。
 * @param tid 当前线程的标识符。
 */
void getElement(std::unique_ptr<SkipList<int, std::string>> &skipList, int tid);

/**
 * @brief 执行插入测试的函数。
 * 
 * @param skipList 跳表对象的智能指针，用于插入操作。
 */
void insert_test(std::unique_ptr<SkipList<int, std::string>>& skipList);

/**
 * @brief 执行搜索测试的函数。
 * 
 * @param skipList 跳表对象的智能指针，用于搜索操作。
 */
void search_test(std::unique_ptr<SkipList<int, std::string>>& skipList);

/**
 * @brief 典型的跳表使用示例。
 * 
 * 该函数演示了如何使用跳表数据结构进行插入、搜索、删除等操作，并展示了跳表的一些基本功能。
 */
void skiplist_usual_use();

/**
 * @brief 跳表性能基准测试。
 * 
 * 该函数执行跳表的性能基准测试，包括插入和搜索测试，并输出测试结果。
 */
void skiplist_benchmark();
#endif // BENCHMARK_H