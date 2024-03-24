#include <iostream>
#include <chrono>
#include <cstdlib>
#include <thread>
#include <ctime>
#include <vector>

#include "skiplist.h"
#include "ThreadPool.h"
#include "benchmark.h"

std::mutex mtx;     // 互斥锁，保护临界区资源
std::string delimiter = ":";    //  键值对之间的分隔符

int THREAD_NUM;      // 线程数量
int TEST_DATANUM;    // 测试的数据量
int MAX_LEVEL;       // 跳表的最大层数

std::random_device global_rd;          // 全局随机设备
std::atomic<int> completedTasks(0); // 用于跟踪已完成的插入任务数量
std::condition_variable cv;         // 线程池任务完成信号量
std::mutex mtx_task;                // 线程池任务互斥锁

unsigned long long getSafeSeed()
{
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx); // 保护global_rd以实现线程安全
    return global_rd();
}

std::mt19937& getThreadLocalMt19937()
{
    // 为每个线程生成独立的种子
    unsigned long long seed = getSafeSeed();
    thread_local std::mt19937 gen(seed);
    return gen;
}

void skiplist_benchmark()
{
    // 使用智能指针来管理动态分配的内存，帮助避免内存泄漏。
    std::unique_ptr<SkipList<int, std::string>> skipList = init_benchmark_data();

    if (skipList->size() > 0)
    {
        std::cout << "检测到跳表中已存在数据，正在清除..." << std::endl;
        // 清除跳表中的所有元素
        skipList->clear();
        std::cout << "数据清除完毕，开始新的基准测试..." << std::endl;
    }
    else
    {
        std::cout << "跳表的元素个数： " << skipList->size() << std::endl;
        std::cout << "跳表为空，开始新的基准测试..." << std::endl;
    }

    insert_test(skipList);      // 进行插入测试,测试QPS.
    completedTasks = 0;                     // 重置计数器
    search_test(skipList);      // 进行搜索测试,测试QPS.
}

std::unique_ptr<SkipList<int, std::string>> init_benchmark_data()
{
    // 从标准输入中读取线程数量和测试数据量的值，并赋给全局变量 THREAD_NUM 和 TEST_DATANUM
    std::cout << "请输入线程数量(通常最大值为16) :";
    std::cin >> THREAD_NUM;

    int data_input;
    std::cout << "请输入测试的数据量（百万）：";
    std::cin >> data_input;

    // 将用户输入的百万单位转换为实际的数据量
    TEST_DATANUM = data_input * MULTI_NUM_FOR_INPUT;

    std::cout << "请输入跳表的最大层级(推荐设为18) :";
    std::cin >> MAX_LEVEL;

    // 可以根据传入参数确定跳表最大层级
    // 使用make_unique创建智能指针
    return std::make_unique<SkipList<int, std::string>>(MAX_LEVEL);
}

void insertElement(std::unique_ptr<SkipList<int, std::string>> &skipList, int tid)
{
    // 计算每个线程插入操作次数
    int tmp = TEST_DATANUM / THREAD_NUM;

    // 每个线程执行插入操作
    for (int i = tid * tmp, count = 0; count < tmp; i++)
    {
        count++;
        // 使用Xorshift64随机数生成器,随机生成一个键值对
        Xorshift64 rng(getSafeSeed());
        skipList->insert_element(rng.nextInRange(0, TEST_DATANUM - 1), "a");
    }

    // 插入任务完成，增加计数器的值
    completedTasks++;
    if (completedTasks == THREAD_NUM)
    {
        std::unique_lock<std::mutex> lock(mtx_task);
        // 通知唤醒等待的线程
        cv.notify_all();
    }
}

void getElement(std::unique_ptr<SkipList<int, std::string>> &skipList, int tid)
{
    // 计算每个线程搜索操作次数
    int tmp = TEST_DATANUM / THREAD_NUM; 

    auto& gen = getThreadLocalMt19937();
    std::uniform_int_distribution<> dis(0, TEST_DATANUM - 1);

    for (int i = tid * tmp, count = 0; count < tmp; i++)
    {
        count++;
        // 使用<random>库生成随机键，在跳表进行搜索操作
        skipList->search_element(dis(gen));
    }

    //搜索任务完成，增加计数器的值
    completedTasks++;
    if (completedTasks == THREAD_NUM)
    {
        std::unique_lock<std::mutex> lock(mtx_task);
        //通知唤醒等待的线程
        cv.notify_all();
    }
}

void insert_test(std::unique_ptr<SkipList<int, std::string>>& skipList)
{
    srand(time(nullptr));
    // 创建线程池对象
    ThreadPool pool(THREAD_NUM);
    // std::vector<std::thread> threads;
    auto start = std::chrono::high_resolution_clock::now();
    //    for (int i = 0; i < THREAD_NUM; i++) {
    //        //std::cout << "main(): creating thread, " << i << std::endl;
    //        threads.emplace_back(insertElement, i);
    //    }
    //    for (auto &thread: threads) {
    //        thread.join();
    //    }
    for (int i = 0; i < THREAD_NUM; i++)
    {
        pool.enqueue(insertElement, std::ref(skipList), i); // 提交插入任务给线程池
    }
    // 等待所有任务执行完毕
    std::unique_lock<std::mutex> lock(mtx_task);
    cv.wait(lock, [&]()
            { return completedTasks == THREAD_NUM; });
    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    std::cout << "insert elapsed: " << elapsed.count() << "\n";
    std::cout << " Insert QPS:" << (TEST_DATANUM / 10000) / elapsed.count() << "w"
              << "\n";
}

void search_test(std::unique_ptr<SkipList<int, std::string>>& skipList)
{
    srand(time(nullptr));
    // std::vector<std::thread> threads;
    ThreadPool pool(THREAD_NUM);
    auto start = std::chrono::high_resolution_clock::now();

    // for (int i = 0; i < THREAD_NUM; i++)
    // {
    //     // std::cout << "main(): 正在创建线程 " << i << std::endl;
    //     threads.emplace_back(getElement, i);
    // }

    // for (auto &thread : threads)
    // {
    //     thread.join();
    // }
    for (int i = 0; i < THREAD_NUM; i++)
    {
        // 提交搜索任务给线程池
        pool.enqueue(getElement, std::ref(skipList), i);
    }
    
    //等待所有任务执行完毕
    std::unique_lock<std::mutex> lock(mtx_task);
    cv.wait(lock, [&]() {
        return completedTasks == THREAD_NUM;
    });
    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    std::cout << "search elapsed: " << elapsed.count() << "\n";
    std::cout << " Search QPS:" << (TEST_DATANUM / 10000) / elapsed.count() << "w"
              << "\n";
}

void skiplist_usual_use()
{
    try
    {
        // 实例化跳表对象并使用std::unique_ptr进行管理
        std::unique_ptr<SkipList<int, std::string>> skipList_implement = std::make_unique<SkipList<int, std::string>>(16);

        // 写入元素
        skipList_implement->insert_element(1, "one");
        skipList_implement->insert_element(2, "two");
        skipList_implement->insert_element(3, "three");
        skipList_implement->insert_element(4, "four");
        skipList_implement->insert_element(5, "five");
        skipList_implement->insert_element(7, "seven");
        skipList_implement->insert_element(8, "eight");
        skipList_implement->insert_element(9, "nine");
        skipList_implement->insert_element(12, "twelve");
        skipList_implement->insert_element(17, "seventeen");
        skipList_implement->insert_element(18, "eighteen");
        skipList_implement->insert_element(19, "nineteen");
        skipList_implement->insert_element(20, "twenty");
        skipList_implement->insert_element(21, "twenty-one");
        skipList_implement->insert_element(22, "twenty-two");
        skipList_implement->insert_element(23, "twenty-three");
        skipList_implement->insert_element(25, "twenty-five");
        skipList_implement->insert_element(27, "twenty-seven");

        // 输出跳表元素个数
        std::cout << "skipList size:" << skipList_implement->size() << std::endl;

        // 跳表元素持久化到文件
        skipList_implement->dump_file();

        // 按键值检索
        if (skipList_implement->search_element(9))
        {
            std::cout << "Element found. "<< std::endl;
        }
        else
        {
            std::cout << "Element not found." << std::endl;
        }

        if (skipList_implement->search_element(18))
        {
            std::cout << "Element found. "<< std::endl;
        }
        else
        {
            std::cout << "Element not found." << std::endl;
        }
        if (skipList_implement->search_element(27))
        {
            std::cout << "Element found. "<< std::endl;
        }
        else
        {
            std::cout << "Element not found." << std::endl;
        }

        // 显示跳表
        skipList_implement->display_list();

        // 按键值删除
        skipList_implement->delete_element(3);
        skipList_implement->delete_element(7);
        skipList_implement->delete_element(17);

        // 输出跳表元素个数
        std::cout << "skipList size:" << skipList_implement->size() << std::endl;

        // 显示跳表
        skipList_implement->display_list();
    }
    catch(const std::exception& e)
    {
        std::cerr << "An exception occurred: " << e.what() << std::endl;
    }

}