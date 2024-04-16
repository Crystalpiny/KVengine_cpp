#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <random>
#include <cstdint>
#include <limits>

#define MULTI_NUM_FOR_INPUT (1000000)   //  用户输入数据量的乘数,简化用户操作

/**
 * @brief 从配置文件中读取 useProgressBar 字段的值。
 * 
 * 此函数尝试打开并读取指定的配置文件，然后解析其内容以获取 useProgressBar 字段的值。
 * useProgressBar 字段指示是否展示进度条。
 * 
 * @param useProgressBar 引用传递，用于存储读取到的 useProgressBar 字段的值。
 * @return 如果成功读取到 useProgressBar 字段，并且字段类型为布尔型，则返回 true，否则返回 false。
 * 
 * @details
 * 函数首先尝试打开配置文件，如果文件成功打开，则读取文件内容并使用 RapidJSON 解析 JSON 结构。
 * 解析后，函数检查 "skipListBenchmark" 对象是否存在，及该对象中是否包含 "useProgressBar" 字段。
 * 如果存在且字段类型为布尔型，则将其值赋给 useProgressBar 参数，函数返回 true。
 * 如果文件无法打开、JSON 解析失败、缺少必要的字段或字段类型不正确，则函数会输出错误信息并返回 false。
 * 
 * @note
 * - 此函数假定配置文件遵循特定的格式，即包含名为 "skipListBenchmark" 的对象，且该对象中应包含 "useProgressBar" 字段。
 * - 如果配置文件格式不正确或缺少 "useProgressBar" 字段，函数会报告错误并返回 false。
 * - 函数只有在成功读取 "useProgressBar" 字段且该字段为布尔类型时才返回 true。
 */
bool ReadProgressBar(bool &useProgressBar);

/**
 * @brief 从配置文件中读取 useRandRNG 字段的值。
 * 
 * 此函数尝试打开并读取指定的配置文件，然后解析其内容以获取 useRandRNG 字段的值。
 * useRandRNG 字段指示是否使用标准的 rand() 随机数生成器。
 * 
 * @param useRandRNG 一个引用，用于存储读取到的 useRandRNG 字段的值。
 * @return bool 如果成功读取 useRandRNG 字段并且该字段为布尔类型，则返回 true。如果出现任何错误，则返回 false。
 * 
 * @details
 * 函数首先尝试打开给定路径的配置文件。如果文件成功打开，它会读取文件内容并使用 RapidJSON 解析 JSON 结构。
 * 解析后，函数检查是否存在 "skipListBenchmark" 对象以及该对象内是否存在 "useRandRNG" 字段。
 * 如果这些条件满足，并且 "useRandRNG" 字段的类型为布尔型，它的值就会被赋给 useRandRNG 参数。
 * 如果文件无法打开、JSON 解析失败、缺少必要的字段或字段类型不正确，函数将输出相应的错误消息并返回 false。
 * 
 * @note
 * - 此函数假设配置文件遵循特定的结构，即包含 "skipListBenchmark" 对象，且该对象内应包含 "useRandRNG" 字段。
 * - 如果配置文件的格式不正确或缺少 "useRandRNG" 字段，函数将报告错误并返回 false。
 * - 函数返回 true 仅当成功读取 "useRandRNG" 字段且其值为布尔类型。
 */
bool ReadUseRandRNG(bool &useRandRNG);

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
 * @brief 使用线程池进行跳表的并发插入性能测试。
 * 
 * @param skipList 要测试的跳表的智能指针。
 * @details
 * 本函数使用ThreadPool类来管理并发插入任务，测试跳表的插入性能。
 * 通过配置文件设置是否在测试过程中显示进度条。
 * 如果配置文件无法读取或缺少必要的字段，函数将输出错误信息并提前返回。
 * 若配置读取成功，依据配置决定是否使用进度条，并初始化ThreadPool和进度条。
 * 将插入任务提交给线程池，根据useProgressBar标志决定是否更新进度条。
 * 所有任务提交后，函数会等待所有线程池中的任务完成，然后计算并输出插入操作的耗时和每秒查询次数（QPS）。
 *
 * 函数的具体实现步骤如下：
 * 1. 从配置文件中读取useProgressBar设置。
 * 2. 创建ThreadPool实例，并根据线程数量初始化进度条（如果使用）。
 * 3. 遍历每个任务，将插入任务提交到线程池中。
 * 4. 如果启用进度条，确保在每次任务完成后更新进度条显示。
 * 5. 使用条件变量等待线程池中的所有任务完成。
 * 6. 计算并输出插入测试的总耗时和QPS。
 * 
 * @note
 * - 需要在外部定义THREAD_NUM、TEST_DATANUM等宏或常量，并确保它们有合理的值。
 * - 使用std::mutex和std::condition_variable确保线程同步和进度条的线程安全更新。
 * - 如果读取配置失败或配置项缺失，函数将不执行测试并输出错误信息。
 * - 函数结束前会打印出总耗时和每秒插入次数（QPS）。
 */
void insert_test_threadpool(std::unique_ptr<SkipList<int, std::string>> &skipList);

/**
 * @brief 使用标准库多线程进行跳表的并发插入性能测试。
 * 
 * @param skipList 要测试的跳表的智能指针。
 * @details
 * 本函数使用std::thread进行跳表的并发插入测试，以评估插入操作的性能。
 * 函数开始时尝试从配置文件中读取是否应显示进度条的设置。如果读取成功，则根据配置决定是否初始化进度条。
 * 接着创建多个线程，并根据是否使用进度条将插入任务分配给每个线程执行。
 * 如果启用进度条，每次插入后线程会更新进度条显示。
 * 完成所有插入任务后，主线程会等待所有子线程完成，然后计算整个操作的耗时及每秒查询次数（QPS）。
 *
 * 函数的具体实现步骤如下：
 * 1. 从配置文件中读取useProgressBar设置。
 * 2. 创建std::thread对象的向量，并根据线程数量初始化进度条（如果使用）。
 * 3. 遍历每个任务，创建线程并将插入任务分配给它们。
 * 4. 如果启用进度条，确保在每次任务完成后同步更新进度条。
 * 5. 主线程使用join等待所有子线程完成任务。
 * 6. 计算并输出插入测试的总耗时和QPS。
 * 
 * @note
 * - 需要在外部定义THREAD_NUM、TEST_DATANUM等宏或常量，并确保它们有合理的值。
 * - 使用std::mutex确保进度条更新操作的线程安全。
 * - 如果读取配置失败或配置项缺失，函数将不执行测试并输出错误信息。
 * - 函数结束前会打印出总耗时和每秒插入次数（QPS）。
 */
void insert_test_multithread(std::unique_ptr<SkipList<int, std::string>>& skipList);

/**
 * @brief 使用CTPL线程池库进行跳表插入性能测试。
 * 
 * @param skipList 要测试的跳表的智能指针。
 * @details
 * 本函数旨在通过CTPL线程池并发地向跳表中插入元素，以评估插入操作的性能。
 * 函数开始时尝试从配置文件中读取是否应显示进度条的设置。如果读取成功，则根据配置决定是否初始化进度条。
 * 接着创建CTPL线程池，并根据是否使用进度条将插入任务分配到线程池中的线程。
 * 如果启用进度条，则每次插入后都会更新进度条显示。
 * 完成所有插入任务后，函数会等待所有线程池中的任务完成，并计算整个操作的耗时以及每秒查询次数（QPS）。
 *
 * 函数的具体实现步骤如下：
 * 1. 从配置文件中读取useProgressBar设置。
 * 2. 创建CTPL线程池，并根据线程数量初始化进度条（如果使用）。
 * 3. 遍历每个任务，将插入任务提交到线程池中。
 * 4. 如果启用进度条，确保在每次任务完成后同步更新进度条。
 * 5. 使用条件变量等待线程池中的所有任务完成。
 * 6. 计算并输出插入测试的总耗时和QPS。
 * 
 * @note
 * - 需要在外部定义THREAD_NUM、TEST_DATANUM等宏或常量，并确保它们有合理的值。
 * - 使用std::mutex和std::condition_variable确保线程同步和进度条的线程安全更新。
 * - 如果读取配置失败或配置项缺失，函数将不执行测试并输出错误信息。
 * - 函数结束前会打印出总耗时和每秒插入次数（QPS）。
 */
void insert_test_ctpl(std::unique_ptr<SkipList<int, std::string>> &skipList);

/**
 * @brief 使用线程池进行跳表的并发搜索性能测试。
 *
 * @param skipList 要测试的跳表的智能指针。
 * @details
 * 本函数使用自定义的ThreadPool类来管理并发搜索任务，测试跳表的搜索性能。
 * 通过配置文件设置是否在测试过程中显示进度条。
 * 如果配置文件无法读取或缺少必要的字段，函数将输出错误信息并提前返回。
 * 若配置读取成功，依据配置决定是否使用进度条，并初始化ThreadPool和进度条。
 * 将搜索任务提交给线程池，根据useProgressBar标志决定是否更新进度条。
 * 所有任务提交后，函数等待所有线程池中的任务完成，然后计算并输出搜索操作的耗时和QPS。
 *
 * 实现步骤如下：
 * 1. 尝试读取配置文件，获取useProgressBar设置。
 * 2. 创建ThreadPool实例，根据线程数量初始化进度条。
 * 3. 遍历每个任务，将搜索任务提交到线程池中。
 * 4. 如果使用进度条，确保在每次任务完成后更新进度条。
 * 5. 等待线程池中的所有任务完成。
 * 6. 计算并输出搜索测试的耗时和QPS。
 *
 * @note
 * - 需要在外部定义THREAD_NUM、TEST_DATANUM等宏或常量，并确保它们有合理的值。
 * - 使用std::mutex确保进度条更新操作的线程安全。
 * - 使用std::condition_variable等待所有线程池任务完成。
 * - 函数执行结束前，会输出总耗时和每秒查询次数（QPS）。
 */
void search_test_threadpool(std::unique_ptr<SkipList<int, std::string>> &skipList);

/**
 * @brief 使用多线程进行跳表搜索性能测试。
 *
 * @param skipList 要进行测试的跳表智能指针引用。
 * @details
 * 本函数使用标准库中的多线程进行跳表的并发搜索测试，并可选地使用进度条显示搜索进度。
 * 函数首先尝试从配置文件中读取是否显示进度条的设置。
 * 如果读取配置成功，则根据设置初始化进度条，并创建多线程进行搜索操作。
 * 然后并发地在每个线程中调用getElement函数进行搜索操作，并根据配置更新进度条。
 * 完成所有搜索任务后，函数会计算并输出整个搜索操作的耗时以及每秒查询次数（QPS）。
 * 如果无法读取配置文件或配置项缺失，函数将输出错误信息并返回。
 *
 * 实现步骤如下：
 * 1. 尝试从配置文件中读取是否显示进度条的设置。
 * 2. 如果配置读取成功，根据配置决定是否显示进度条。
 * 3. 创建多线程，为每个线程分配搜索任务，并可选地同步更新进度条。
 * 4. 使用线程的join方法等待所有搜索任务完成。
 * 5. 计算并输出整个搜索操作的耗时和QPS。
 *
 * @note
 * - 如果配置文件读取失败或缺少useProgressBar字段，将不会执行测试并输出错误信息。
 * - 使用std::mutex确保进度条更新操作的线程安全。
 * - 函数结束前会输出所有任务的执行时间和性能指标。
 * - THREAD_NUM和TEST_DATANUM需要在外部定义，并确保它们有合理的值。
 */
void search_test_multithread(std::unique_ptr<SkipList<int, std::string>> &skipList);

/**
 * @brief 使用CTPL线程池进行跳表搜索性能测试。
 *
 * @param skipList 要进行测试的跳表智能指针引用。
 * @details
 * 本函数使用CTPL线程池库对跳表进行并发搜索测试，并可选地使用进度条显示搜索进度。
 * 函数首先尝试从配置文件中读取是否显示进度条的设置。
 * 如果读取配置成功，根据设置初始化进度条，并创建CTPL线程池。
 * 然后并发地在每个线程中调用getElement函数进行搜索操作，并根据配置更新进度条。
 * 完成所有搜索任务后，函数会计算并输出整个搜索操作的耗时以及每秒查询次数（QPS）。
 * 如果无法读取配置文件或配置项缺失，函数将输出错误信息并返回。
 *
 * 实现步骤如下：
 * 1. 尝试从配置文件中读取是否显示进度条的设置。
 * 2. 创建CTPL线程池，并根据设置决定是否显示进度条。
 * 3. 为每个线程分配搜索任务，并可选地同步更新进度条。
 * 4. 使用条件变量等待所有搜索任务完成。
 * 5. 计算并输出整个搜索操作的耗时和QPS。
 *
 * @note
 * - 如果配置文件读取失败或缺少useProgressBar字段，将不会执行测试并输出错误信息。
 * - 使用std::mutex确保进度条更新操作的线程安全。
 * - 函数结束前会输出所有任务的执行时间和性能指标。
 */
void search_test_ctpl(std::unique_ptr<SkipList<int, std::string>> &skipList);

/**
 * @brief 演示跳表的常规使用方法。
 *
 * @details
 * 本函数展示了如何使用跳表进行基本操作，包括元素的插入、大小获取、持久化、搜索、显示以及删除操作。
 * 首先，创建一个整数键和字符串值的跳表对象，并使用std::unique_ptr进行智能指针管理。
 * 然后，向跳表中插入多个键值对，并展示跳表的大小。
 * 接着，将跳表内容持久化到文件中，并尝试搜索某些元素以检查它们是否存在。
 * 最后，展示完整的跳表，执行几次删除操作，并再次展示更新后的跳表和其大小。
 * 如果在执行过程中出现异常，则捕获并输出异常信息。
 *
 * 实现步骤如下：
 * 1. 使用std::make_unique创建跳表对象。
 * 2. 通过insert_element方法插入不同的键值对到跳表中。
 * 3. 使用size方法获取并打印跳表的大小。
 * 4. 使用dump_file方法将跳表状态持久化到文件中。
 * 5. 使用search_element方法搜索特定键的元素。
 * 6. 使用display_list方法打印跳表的当前状态。
 * 7. 使用delete_element方法删除特定键的元素。
 * 8. 再次使用size和display_list方法获取并打印跳表的更新后大小和状态。
 * 9. 使用try-catch结构处理可能的异常。
 *
 * @note
 * - 跳表对象是通过std::unique_ptr管理，确保资源能够自动释放。
 * - dump_file、search_element、delete_element等方法的具体实现依赖于SkipList类的定义。
 * - 如果操作过程中出现异常，会输出异常信息并终止程序的其他操作。
 */
void skiplist_usual_use();

#endif // BENCHMARK_H