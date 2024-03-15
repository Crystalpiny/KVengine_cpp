#ifndef BENCHMARK_H
#define BENCHMARK_H

#define MULTI_NUM_FOR_INPUT (1000000)   //  用户输入数据量的乘数,简化用户操作

/**
 * @brief 初始化基准测试数据。
 * 
 * 该函数从标准输入中读取线程数量和测试数据量的值，并将它们分别赋给全局变量 THREAD_NUM 和 TEST_DATANUM。
 * 
 * @note 线程数量的最大值通常为16。
 */
void init_benchmark_data();

/**
 * @brief 向跳表中插入元素。
 * 
 * 该函数根据给定的线程标识符向跳表中插入元素。
 * 
 * @param tid 线程标识符，用于确定插入操作的范围。
 */
void insertElement(int tid);

/**
 * @brief 从跳表中获取元素。
 * 
 * 该函数根据给定的线程标识符从跳表中搜索元素。
 * 
 * @param tid 线程标识符，用于确定搜索操作的范围。
 */
void getElement(int tid);

/**
 * @brief 执行插入测试。
 * 
 * 该函数用于执行插入测试，向跳表中插入数据，并计算插入操作的耗时和每秒插入的元素数量。
 */
void insert_test();

/**
 * @brief 执行搜索测试。
 * 
 * 该函数用于执行搜索测试，从跳表中搜索数据，并计算搜索操作的耗时和每秒搜索的元素数量。
 */
void search_test();

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