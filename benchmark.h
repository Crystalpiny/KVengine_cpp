#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <random>
#include <cstdint>

#define MULTI_NUM_FOR_INPUT (1000000)   //  用户输入数据量的乘数,简化用户操作

/**
 * @brief Xorshift64 伪随机数生成器类
 * 
 */
class Xorshift64 {
private:
    uint64_t state;

public:
    /**
     * @brief 构造一个 Xorshift64 对象
     * 
     * @param seed 随机数种子，默认使用随机设备生成
     */
    Xorshift64(uint64_t seed = std::random_device{}()) : state(seed) {}

    /**
     * @brief 生成下一个伪随机数
     * 
     * @return uint64_t 返回下一个伪随机数
     */
    uint64_t next() {
        state ^= state >> 12;
        state ^= state << 25;
        state ^= state >> 27;
        return state * 0x2545F4914F6CDD1D;
    }

    /**
     * @brief 生成一个在指定范围内的伪随机数
     * 
     * @param min 最小值
     * @param max 最大值
     * @return uint32_t 返回在指定范围内的伪随机数
     */
    uint32_t nextInRange(uint32_t min, uint32_t max) {
        uint64_t range = max - min + 1;
        return min + static_cast<uint32_t>(next() % range);
    }
};

/**
 * @brief 获取一个线程安全的随机种子
 * 
 * 使用静态互斥锁保护全局随机设备以实现线程安全，并返回一个随机种子。
 * 
 * @return unsigned long long 返回一个线程安全的随机种子
 */
unsigned long long getSafeSeed();

/**
 * @brief 获取线程局部的 Mersenne Twister 伪随机数生成器
 * 
 * 为每个线程生成一个独立的种子，并使用其生成一个 Mersenne Twister 伪随机数生成器。
 * 
 * @return std::mt19937& 返回一个线程局部的 Mersenne Twister 伪随机数生成器的引用
 */
std::mt19937& getThreadLocalMt19937();

/**
 * @brief 跳表性能基准测试。
 * 
 * 该函数执行跳表的性能基准测试，包括插入和搜索测试，并输出测试结果。
 */
void skiplist_benchmark();

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

#endif // BENCHMARK_H