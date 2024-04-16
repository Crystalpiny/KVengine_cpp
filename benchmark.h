#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <random>
#include <cstdint>
#include <limits>

#define MULTI_NUM_FOR_INPUT (1000000)   //  用户输入数据量的乘数,简化用户操作

/**
 * @brief 读取配置文件并获取进度条使用标志。
 * 
 * 此函数尝试打开配置文件，并解析JSON以检索 'useProgressBar' 字段。
 * 如果配置文件不存在、无法打开或JSON格式不正确，函数将返回false。
 * 如果一切正常，函数将设置 'useProgressBar' 并返回true。
 * 
 * @param useProgressBar 一个引用布尔值，用于存储是否使用进度条的标志。
 * @return bool 如果成功读取配置文件并获取 'useProgressBar' 字段，则返回true，否则返回false。
 */
bool ReadConfig(bool &useProgressBar);

/**
 * @brief Xorshift64 伪随机数生成器类
 * 
 */
class Xorshift64
{
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
    uint64_t next()
    {
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
    uint32_t nextInRange(uint32_t min, uint32_t max)
    {
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
 * @brief 打印测试模式选择菜单
 * 
 * 此函数用于打印测试模式选择菜单，提供用户选择线程池或多线程的选项。
 * 
 * @note 
 * - 该函数依赖 iostream 库中的 std::cout 对象进行输出。
 * - 用户需要根据菜单提示输入相应选项。
 * - 本函数不会处理用户输入，只负责打印菜单。
 */
void printTestModeSelection();

/**
 * @brief 准备跳表用于基准测试
 * 
 * 此函数用于准备跳表以进行基准测试。如果跳表中已存在数据，则清除数据；否则，直接开始新的基准测试。
 * 
 * @param skipList 跳表对象的智能指针
 * 
 * @note 
 * - 该函数依赖 iostream 库中的 std::cout 对象进行输出。
 * - 如果跳表中已存在数据，则清除所有数据并打印清除信息。
 * - 如果跳表为空，则直接打印开始新的基准测试信息。
 */
void prepareSkipListForBenchmark(std::unique_ptr<SkipList<int, std::string>> &skipList);

/**
 * @brief 执行跳表基准测试
 * 
 * 此函数用于执行跳表的基准测试，根据用户选择的测试模式进行插入和搜索操作。
 * 
 * @note 
 * - 该函数依赖 iostream 库中的 std::cout 和 std::cin 对象进行输入输出。
 * - 调用 init_benchmark_data() 函数初始化跳表对象。
 * - 用户需要根据打印的测试模式选择菜单输入相应选项。
 * - 如果用户输入无效选项，会要求重新输入。
 * - 根据用户选择的测试模式，调用相应的插入和搜索函数。
 * - 在多线程测试模式下，会重置完成任务的计数器。
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
 * @brief 插入元素到跳表
 * 
 * 此函数用于将元素插入到跳表中。每个线程负责插入一部分元素。
 * 
 * @param skipList 跳表对象的智能指针
 * @param tid 线程的ID
 * 
 * @note 
 * - 该函数依赖于 iostream 库和 mutex 库中的 std::cout、std::unique_lock 和 std::mutex 对象。
 * - 根据线程的ID和总共的线程数量，计算每个线程需要执行的插入操作次数。
 * - 每个线程通过循环执行插入操作，使用 Xorshift64 随机数生成器生成键值对。
 * - 在插入完成后，增加计数器的值。
 * - 当所有插入任务完成时，通过互斥锁和条件变量通知等待的线程。
 */
void insertElement(std::unique_ptr<SkipList<int, std::string>> &skipList, int tid);

/**
 * @brief 获取元素从跳表中
 * 
 * 此函数用于从跳表中获取元素。每个线程负责执行一部分搜索操作。
 * 
 * @param skipList 跳表对象的智能指针
 * @param tid 线程的ID
 * 
 * @note 
 * - 该函数依赖于 iostream 库和 mutex 库中的 std::cout、std::unique_lock 和 std::mutex 对象，以及 random 库中的 std::uniform_int_distribution 和 std::mt19937 对象。
 * - 根据线程的ID和总共的线程数量，计算每个线程需要执行的搜索操作次数。
 * - 每个线程通过循环执行搜索操作，使用 std::uniform_int_distribution 和 std::mt19937 生成随机键，然后在跳表中进行搜索操作。
 * - 在搜索完成后，增加计数器的值。
 * - 当所有搜索任务完成时，通过互斥锁和条件变量通知等待的线程。
 */
void getElement(std::unique_ptr<SkipList<int, std::string>> &skipList, int tid);

/**
 * @brief 使用线程池进行插入测试
 * 
 * 此函数用于使用线程池进行插入测试。多个线程同时向跳表中插入元素。
 * 
 * @param skipList 跳表对象的智能指针
 * 
 * @note 
 * - 该函数依赖于 iostream 库中的 std::cout 对象，以及 chrono 库中的 std::chrono::high_resolution_clock、std::chrono::duration 和 std::chrono::duration::count 对象。
 * - 创建线程池对象，并设置线程数量为 THREAD_NUM。
 * - 获取当前时间作为插入操作开始时间。
 * - 循环提交插入任务给线程池，每个线程负责插入一部分元素。
 * - 等待所有任务执行完毕，使用互斥锁和条件变量实现等待机制。
 * - 获取当前时间作为插入操作结束时间。
 * - 计算插入操作的耗时，并输出耗时信息。
 * - 计算插入操作的每秒执行次数（QPS）并输出。
 */
void insert_test_threadpool(std::unique_ptr<SkipList<int, std::string>> &skipList);

/**
 * @brief 使用多线程进行插入测试
 * 
 * 此函数用于使用多线程进行插入测试。多个线程同时向跳表中插入元素。
 * 
 * @param skipList 跳表对象的智能指针
 * 
 * @note 
 * - 该函数依赖于 iostream 库中的 std::cout 对象，以及 chrono 库中的 std::chrono::high_resolution_clock、std::chrono::duration 和 std::chrono::duration::count 对象。
 * - 创建线程容器，并设置容器大小为 THREAD_NUM。
 * - 获取当前时间作为插入操作开始时间。
 * - 循环创建线程，每个线程负责插入一部分元素。
 * - 等待所有线程执行完毕，使用 std::thread::join() 实现等待机制。
 * - 获取当前时间作为插入操作结束时间。
 * - 计算插入操作的耗时，并输出耗时信息。
 * - 计算插入操作的每秒执行次数（QPS）并输出。
 */
void insert_test_multithread(std::unique_ptr<SkipList<int, std::string>>& skipList);

/**
 * @brief 插入测试函数（使用CTPL线程池）
 * 
 * 该函数使用CTPL线程池对给定的跳表对象执行并发插入操作的测试。
 * 
 * @param skipList 跳表对象的智能指针，用于进行插入操作的目标跳表。
 * 
 * @note
 * 该函数通过以下步骤执行并发插入操作：
 * - 创建一个具有预定义线程数量的CTPL线程池。
 * - 记录插入操作开始时间。
 * - 创建互斥锁和条件变量，用于线程同步和任务完成的跟踪。
 * - 循环创建并提交任务到线程池，每个任务负责向跳表中插入元素。
 * - 每个任务完成插入操作后，使用互斥锁保护共享的任务完成计数器，并通知等待的线程。
 * - 主线程等待所有任务完成，即等待任务完成计数器达到预定义线程数量。
 * - 记录插入操作结束时间，并计算插入操作的耗时。
 * - 输出插入操作的耗时。
 * - 计算并输出每秒插入的数量（QPS，每万条数据）。
 */ 
void insert_test_ctpl(std::unique_ptr<SkipList<int, std::string>> &skipList);

/**
 * @brief 使用线程池进行搜索测试
 * 
 * 此函数用于使用线程池进行搜索测试。多个线程同时从跳表中搜索元素。
 * 
 * @param skipList 跳表对象的智能指针
 * 
 * @note 
 * - 该函数依赖于 iostream 库中的 std::cout 对象，以及 chrono 库中的 std::chrono::high_resolution_clock、std::chrono::duration 和 std::chrono::duration::count 对象。
 * - 创建线程池对象，并设置线程数量为 THREAD_NUM。
 * - 获取当前时间作为搜索操作开始时间。
 * - 循环提交搜索任务给线程池，每个线程负责搜索一部分元素。
 * - 等待所有任务执行完毕，使用互斥锁和条件变量实现等待机制。
 * - 获取当前时间作为搜索操作结束时间。
 * - 计算搜索操作的耗时，并输出耗时信息。
 * - 计算搜索操作的每秒执行次数（QPS）并输出。
 */
void search_test_threadpool(std::unique_ptr<SkipList<int, std::string>> &skipList);

/**
 * @brief 使用多线程进行搜索测试
 * 
 * 此函数用于使用多线程进行搜索测试。多个线程同时从跳表中搜索元素。
 * 
 * @param skipList 跳表对象的智能指针
 * 
 * @note 
 * - 该函数依赖于 iostream 库中的 std::cout 对象，以及 chrono 库中的 std::chrono::high_resolution_clock、std::chrono::duration 和 std::chrono::duration::count 对象。
 * - 创建线程容器，并设置容器大小为 THREAD_NUM。
 * - 获取当前时间作为搜索操作开始时间。
 * - 循环创建线程，每个线程负责搜索一部分元素。
 * - 等待所有线程执行完毕，使用 std::thread::join() 实现等待机制。
 * - 获取当前时间作为搜索操作结束时间。
 * - 计算搜索操作的耗时，并输出耗时信息。
 * - 计算搜索操作的每秒执行次数（QPS）并输出。
 */
void search_test_multithread(std::unique_ptr<SkipList<int, std::string>> &skipList);

/**
 * @brief 搜索测试函数（使用CTPL线程池）
 * 
 * 该函数使用CTPL线程池对给定的跳表对象执行并发搜索操作的测试。
 * 
 * @param skipList 跳表对象的智能指针，用于进行搜索操作的目标跳表。
 * 
 * @note
 * 该函数通过以下步骤执行并发搜索操作：
 * - 创建一个具有预定义线程数量的CTPL线程池。
 * - 记录搜索操作开始时间。
 * - 创建互斥锁和条件变量，用于线程同步和任务完成的跟踪。
 * - 循环创建并提交任务到线程池，每个任务负责在跳表中执行元素搜索。
 * - 每个任务完成搜索操作后，使用互斥锁保护共享的任务完成计数器，并通知等待的线程。
 * - 主线程等待所有任务完成，即等待任务完成计数器达到预定义线程数量。
 * - 记录搜索操作结束时间，并计算搜索操作的耗时。
 * - 输出搜索操作的耗时。
 * - 计算并输出每秒搜索的数量（QPS，每万条数据）。
 */ 
void search_test_ctpl(std::unique_ptr<SkipList<int, std::string>> &skipList);

/**
 * @brief 跳表的常规用法示例
 * 
 * 此函数用于展示跳表的常规用法示例，包括插入元素、检索元素、删除元素等操作。
 * 
 * @note 
 * - 该函数依赖于 iostream 库中的 std::cout 和 std::endl 对象。
 * - 实例化跳表对象并使用 std::unique_ptr 进行管理。
 * - 插入一系列的键值对到跳表中。
 * - 输出跳表的元素个数。
 * - 将跳表的元素持久化到文件。
 * - 根据键值进行元素检索，并输出结果。
 * - 显示跳表的内容。
 * - 根据键值进行元素删除。
 * - 输出跳表的元素个数。
 * - 再次显示跳表的内容。
 * - 如果发生异常，将异常信息输出到标准错误流。
 */
void skiplist_usual_use();

#endif // BENCHMARK_H