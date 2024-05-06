#include <iostream>
#include <chrono>
#include <cstdlib>
#include <thread>
#include <ctime>
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>

#include "skiplist.h"
#include "ThreadPool.h"
#include "benchmark.h"
#include "ctpl_stl.h"

/* 引入外部库*/
#include "progressbar.hpp"
#include "document.h"

std::mutex mtx;     // 互斥锁，保护临界区资源
std::string delimiter = ":";    //  键值对之间的分隔符

int THREAD_NUM;      // 线程数量
int TEST_DATANUM;    // 测试的数据量
int MAX_LEVEL;       // 跳表的最大层数

std::random_device global_rd;          // 全局随机设备
std::atomic<int> completedTasks(0); // 用于跟踪已完成的插入任务数量
std::condition_variable cv;         // 线程池任务完成信号量
std::mutex mtx_task;                // 线程池任务互斥锁

const std::string configFilePath = "C:/SoftWare/VScode-dir/KVengine_cpp/config.json"; // 配置文件路径

bool ReadProgressBar(bool& useProgressBar)
{
    std::ifstream configFile(configFilePath);
    if (!configFile.is_open())
    {
        std::cerr << "Unable to open config file." << std::endl;
        return false;
    }
    
    std::string content((std::istreambuf_iterator<char>(configFile)), std::istreambuf_iterator<char>());
    rapidjson::Document doc;
    doc.Parse(content.c_str());

    if (!doc.IsObject() || !doc.HasMember("skipListBenchmark") || !doc["skipListBenchmark"].IsObject())
    {
        std::cerr << "Invalid config format." << std::endl;
        return false;
    }

    const rapidjson::Value& benchmark = doc["skipListBenchmark"];
    if (benchmark.HasMember("useProgressBar") && benchmark["useProgressBar"].IsBool())
    {
        useProgressBar = benchmark["useProgressBar"].GetBool();
        return true;
    }

    return false;
}

bool ReadUseRandRNG(bool& useRandRNG)
{
    std::ifstream configFile(configFilePath); // Replace with your actual config file path
    if (!configFile.is_open())
    {
        std::cerr << "Unable to open config file." << std::endl;
        return false;
    }
    
    std::string content((std::istreambuf_iterator<char>(configFile)), std::istreambuf_iterator<char>());
    rapidjson::Document doc;
    doc.Parse(content.c_str());

    if (!doc.IsObject() || !doc.HasMember("skipListBenchmark") || !doc["skipListBenchmark"].IsObject())
    {
        std::cerr << "Invalid config format." << std::endl;
        return false;
    }

    const rapidjson::Value& benchmark = doc["skipListBenchmark"];
    if (benchmark.HasMember("useRandRNG") && benchmark["useRandRNG"].IsBool())
    {
        useRandRNG = benchmark["useRandRNG"].GetBool();
        return true;
    }
    else
    {
        std::cerr << "Missing 'useRandRNG' field." << std::endl;
        return false;
    }
}

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

void printTestModeSelection()
{
    std::cout << "\n=============================\n";
    std::cout << "  请选择测试模式:\n";
    std::cout << "  1. ThreadPool\n";
    std::cout << "  2. Multi-thread\n";
    std::cout << "  3. CTPL\n";
    std::cout << "=============================\n";
    std::cout << "请输入选项: ";
}

void prepareSkipListForBenchmark(std::unique_ptr<SkipList<int, std::string>>& skipList)
{
    if (skipList->size() > 0)
    {
        std::cout << "检测到跳表中已存在数据，正在清除..." << std::endl;
        skipList->clear();
        std::cout << "数据清除完毕，开始新的基准测试..." << std::endl;
    }
    else
    {
        std::cout << "跳表为空，开始新的基准测试..." << std::endl;
    }
}

void skiplist_benchmark()
{
    std::unique_ptr<SkipList<int, std::string>> skipList = init_benchmark_data();

    int testMode = 0;
    while (true)
    {
        printTestModeSelection();
        std::cin >> testMode;
        if (std::cin.fail() || (testMode < 1 || testMode > 3))  // 更新有效选项范围
        {
            std::cout << "无效选项，请重新输入。\n";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        else
        {
            break;
        }
    }

    prepareSkipListForBenchmark(skipList);

    switch (testMode)
    {
        case 1:
            insert_test_threadpool(skipList);
            completedTasks = 0; // 重置计数器
            search_test_threadpool(skipList);
            break;
        case 2:
            insert_test_multithread(skipList);
            completedTasks = 0; // 重置计数器
            search_test_multithread(skipList);
            break;
        case 3:
            insert_test_ctpl(skipList);
            completedTasks = 0; // 重置计数器
            search_test_ctpl(skipList);
            break;
        default:
            std::cout << "未知的测试模式。" << std::endl;
            break;
    }
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
    bool useRandRNG = true;    // 默认使用rand随机数生成器
    if (!ReadUseRandRNG(useRandRNG))
    {   // 如果无法读取配置文件或配置项缺失，输出错误信息并返回
        std::cerr << "Error reading config or missing useRandRNG field." << std::endl;
        return;
    }

    // 计算每个线程插入操作次数
    int tmp = TEST_DATANUM / THREAD_NUM;

    // 每个线程执行插入操作
    if(useRandRNG)
    {
        for (int i = tid * tmp, count = 0; count < tmp; i++)
        {
            count++;
            skipList->insert_element(std::rand() % TEST_DATANUM, "a");
        }
    }
    else
    {
        for (int i = tid * tmp, count = 0; count < tmp; i++)
        {
            // 使用Xorshift64随机数生成器,随机生成一个键值对
            Xorshift64 rng(getSafeSeed());
            skipList->insert_element(rng.nextInRange(0, TEST_DATANUM - 1), "a");
        }
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
    bool useRandRNG = true;    // 默认使用rand随机数生成器
    if (!ReadUseRandRNG(useRandRNG))
    {   // 如果无法读取配置文件或配置项缺失，输出错误信息并返回
        std::cerr << "Error reading config or missing useRandRNG field." << std::endl;
        return;
    }

    // 计算每个线程搜索操作次数
    int tmp = TEST_DATANUM / THREAD_NUM; 
    if(useRandRNG)
    {
        for (int i = tid * tmp, count = 0; count < tmp; i++)
        {
            count++;
            // 使用std::rand()生成随机搜索键
            skipList->search_element(std::rand() % TEST_DATANUM);
        }
    }
    else
    {
        auto& gen = getThreadLocalMt19937();
        std::uniform_int_distribution<> dis(0, TEST_DATANUM - 1);

        for (int i = tid * tmp, count = 0; count < tmp; i++)
        {
            count++;
            skipList->search_element(dis(gen));
        }
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

void insert_test_threadpool(std::unique_ptr<SkipList<int, std::string>>& skipList)
{
    bool useProgressBar = false;    // 默认不显示进度条
    if (!ReadProgressBar(useProgressBar))
    {   // 如果无法读取配置文件或配置项缺失，输出错误信息并返回
        std::cerr << "Error reading config or missing useProgressBar field." << std::endl;
        return;
    }
    // ThreadPool的插入测试逻辑
    ThreadPool pool(THREAD_NUM);
    std::mutex progress_mtx;
    progressbar bar(THREAD_NUM);    // 使用线程数量初始化progress bar

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < THREAD_NUM; i++)
    {
        if(!useProgressBar)
        {   /* 在无进度条模式下将任务交给线程池的方法 */
            // 提交插入任务给线程池
            pool.enqueue(insertElement, std::ref(skipList), i);
        }
        else
        {   /* 基于progress bar库的进度条*/
            pool.enqueue([&skipList, &bar, &progress_mtx, i]() {
                insertElement(skipList, i);
                // 同步访问进度条
                {
                    std::lock_guard<std::mutex> lock(progress_mtx);
                    bar.update(); // 每次插入后更新进度条
                }
            });
        }
    }

    // 等待所有任务执行完毕
    std::unique_lock<std::mutex> lock(mtx_task);
    cv.wait(lock, [&](){ return completedTasks == THREAD_NUM; });

    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    std::cout << std::endl;
    std::cout << "ThreadPool insert elapsed: " << elapsed.count() << "\n";
    std::cout << "ThreadPool Insert QPS:" << (TEST_DATANUM / 10000) / elapsed.count() << "w" << "\n";
    std::cout << std::endl; // 在输出耗时后添加换行
}

void insert_test_multithread(std::unique_ptr<SkipList<int, std::string>>& skipList)
{
    bool useProgressBar = false;    // 默认不显示进度条
    if (!ReadProgressBar(useProgressBar))
    {   // 如果无法读取配置文件或配置项缺失，输出错误信息并返回
        std::cerr << "Error reading config or missing useProgressBar field." << std::endl;
        return;
    }
    // 多线程的插入测试逻辑
    std::vector<std::thread> threads;
    std::mutex progress_mtx;
    progressbar bar(THREAD_NUM); // 使用线程数量初始化进度条

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < THREAD_NUM; i++)
    {
        if(!useProgressBar)
        {   /* 无进度条模式 */
            threads.emplace_back(insertElement, std::ref(skipList), i);
        }
        else
        {   /* 基于progress bar库的进度条*/
            threads.emplace_back([&skipList, &bar, &progress_mtx, i]() {
                insertElement(skipList, i);
                {
                    std::lock_guard<std::mutex> lock(progress_mtx);
                    bar.update(); // 每次插入后更新进度条
                }
            });
        }
    }
    for (auto &thread : threads)
    {
        thread.join();
    }

    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    std::cout << std::endl; // 在输出耗时前添加换行
    std::cout << "Multi-thread insert elapsed: " << elapsed.count() << "\n";
    std::cout << "Multi-thread Insert QPS:" << (TEST_DATANUM / 10000) / elapsed.count() << "w" << "\n";
    std::cout << std::endl; // 在输出耗时后添加换行
}

void insert_test_ctpl(std::unique_ptr<SkipList<int, std::string>>& skipList)
{
    bool useProgressBar = false;    // 默认不显示进度条
    if (!ReadProgressBar(useProgressBar))
    {   // 如果无法读取配置文件或配置项缺失，输出错误信息并返回
        std::cerr << "Error reading config or missing useProgressBar field." << std::endl;
        return;
    }
    ctpl::thread_pool pool(THREAD_NUM); // 创建线程池
    std::mutex progress_mtx;
    progressbar bar(THREAD_NUM); // 使用线程数量初始化进度条

    auto start = std::chrono::high_resolution_clock::now();
    std::mutex mtx_task;
    std::condition_variable cv;
    int completedTasks = 0;

    for (int i = 0; i < THREAD_NUM; i++)
    {
        if(!useProgressBar)
        {   /* 无进度条模式 */
            pool.push([&skipList, &mtx_task, &cv, &completedTasks, i](int)
            {
                insertElement(skipList, i);
                std::lock_guard<std::mutex> lock(mtx_task);
                completedTasks++;
                cv.notify_one();
            });
        }
        else
        {   /* 基于progress bar库的进度条*/
            pool.push([&skipList, &mtx_task, &cv, &bar, &progress_mtx, &completedTasks, i](int) {
                insertElement(skipList, i);
                {
                    std::lock_guard<std::mutex> lock(progress_mtx);
                    bar.update(); // 每次插入后更新进度条
                }
                std::lock_guard<std::mutex> lock(mtx_task);
                completedTasks++;
                cv.notify_one();
            });
        }
    }

    // 等待所有任务执行完毕
    std::unique_lock<std::mutex> lock(mtx_task);
    cv.wait(lock, [&](){ return completedTasks == THREAD_NUM; });

    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    std::cout << std::endl; // 在输出耗时前添加换行
    std::cout << "CTPL insert elapsed: " << elapsed.count() << " seconds\n";
    std::cout << "CTPL Insert QPS:" << (TEST_DATANUM / 10000) / elapsed.count() << "w" << "\n";
    std::cout << std::endl; // 在输出耗时后添加换行
}

void search_test_threadpool(std::unique_ptr<SkipList<int, std::string>>& skipList)
{
    bool useProgressBar = false;    // 默认不显示进度条
    if (!ReadProgressBar(useProgressBar))
    {   // 如果无法读取配置文件或配置项缺失，输出错误信息并返回
        std::cerr << "Error reading config or missing useProgressBar field." << std::endl;
        return;
    }
    // ThreadPool的插入测试逻辑
    ThreadPool pool(THREAD_NUM);
    std::mutex progress_mtx;
    progressbar bar(THREAD_NUM); // 使用线程数量初始化进度条

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < THREAD_NUM; i++)
    {
        if(!useProgressBar)
        {   /* 无进度条模式 */
            // 提交搜索任务给线程池
            pool.enqueue(getElement, std::ref(skipList), i);
        }
        else
        {   /* 基于progress bar库的进度条*/
            pool.enqueue([&skipList, &bar, &progress_mtx, i]() {
                getElement(skipList, i);
                {
                    std::lock_guard<std::mutex> lock(progress_mtx);
                    bar.update(); // 每次搜索后更新进度条
                }
            });
        }
    }

    // 等待所有任务执行完毕
    std::unique_lock<std::mutex> lock(mtx_task);
    cv.wait(lock, [&](){ return completedTasks == THREAD_NUM; });

    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    std::cout << std::endl; // 在输出耗时前添加换行
    std::cout << "ThreadPool search elapsed: " << elapsed.count() << "\n";
    std::cout << "ThreadPool search QPS:" << (TEST_DATANUM / 10000) / elapsed.count() << "w" << "\n";
    std::cout << std::endl; // 在输出耗时后添加换行
}

void search_test_multithread(std::unique_ptr<SkipList<int, std::string>>& skipList)
{
    bool useProgressBar = false;    // 默认不显示进度条
    if (!ReadProgressBar(useProgressBar))
    {   // 如果无法读取配置文件或配置项缺失，输出错误信息并返回
        std::cerr << "Error reading config or missing useProgressBar field." << std::endl;
        return;
    }
    // 多线程的插入测试逻辑
    std::vector<std::thread> threads;
    std::mutex progress_mtx;
    progressbar bar(THREAD_NUM); // 使用线程数量初始化进度条
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < THREAD_NUM; i++)
    {
        if(!useProgressBar)
        {   /* 无进度条模式 */
            threads.emplace_back(getElement, std::ref(skipList), i);
        }
        else
        {   /* 基于progress bar库的进度条*/
            threads.emplace_back([&skipList, &bar, &progress_mtx, i]() {
                getElement(skipList, i);
                {
                    std::lock_guard<std::mutex> lock(progress_mtx);
                    bar.update(); // 每次搜索后更新进度条
                }
            });
        }
    }
    for (auto &thread : threads)
    {
        thread.join();
    }

    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    std::cout << std::endl; // 在输出耗时前添加换行
    std::cout << "Multi-thread search elapsed: " << elapsed.count() << "\n";
    std::cout << "Multi-thread search QPS:" << (TEST_DATANUM / 10000) / elapsed.count() << "w" << "\n";
    std::cout << std::endl; // 在输出耗时后添加换行
}

void search_test_ctpl(std::unique_ptr<SkipList<int, std::string>>& skipList)
{
    bool useProgressBar = false;    // 默认不显示进度条
    if (!ReadProgressBar(useProgressBar))
    {   // 如果无法读取配置文件或配置项缺失，输出错误信息并返回
        std::cerr << "Error reading config or missing useProgressBar field." << std::endl;
        return;
    }
    ctpl::thread_pool pool(THREAD_NUM); // 创建线程池
    std::mutex progress_mtx;
    progressbar bar(THREAD_NUM); // 使用线程数量初始化进度条

    auto start = std::chrono::high_resolution_clock::now();
    std::mutex mtx_task;
    std::condition_variable cv;
    int completedTasks = 0;

    for (int i = 0; i < THREAD_NUM; i++)
    {
        if(!useProgressBar)
        {   /* 无进度条模式 */
            pool.push([&skipList, &mtx_task, &cv, &completedTasks, i](int) {
                getElement(skipList, i);
                std::lock_guard<std::mutex> lock(mtx_task);
                completedTasks++;
                cv.notify_one();
            });
        }
        else
        {   /* 基于progress bar库的进度条*/
            pool.push([&skipList, &bar, &progress_mtx, &mtx_task, &cv, &completedTasks, i](int)
            {
                getElement(skipList, i);
                {
                    std::lock_guard<std::mutex> lock(progress_mtx);
                    bar.update(); // 每次搜索后更新进度条
                }
                {
                    std::lock_guard<std::mutex> lock(mtx_task);
                    completedTasks++;
                    cv.notify_one();
                }
            });
        }
    }

    // 等待所有任务执行完毕
    std::unique_lock<std::mutex> lock(mtx_task);
    cv.wait(lock, [&](){ return completedTasks == THREAD_NUM; });

    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    std::cout << std::endl; // 在输出耗时前添加换行
    std::cout << "CTPL search elapsed: " << elapsed.count() << " seconds\n";
    std::cout << "CTPL search QPS:" << (TEST_DATANUM / 10000) / elapsed.count() << "w" << "\n";
    std::cout << std::endl; // 在输出耗时后添加换行
}

void skiplist_usual_use()
{
    try
    {
        // 实例化跳表对象并使用std::unique_ptr进行管理
        std::unique_ptr<SkipList<int, std::string>> skipList_implement = std::make_unique<SkipList<int, std::string>>(16);

        // 写入元素
        skipList_implement->insert_element(1, "I");
        skipList_implement->insert_element(2, "II");
        skipList_implement->insert_element(3, "III");
        skipList_implement->insert_element(4, "IV");
        skipList_implement->insert_element(5, "V");
        skipList_implement->insert_element(7, "VII");
        skipList_implement->insert_element(8, "VIII");
        skipList_implement->insert_element(9, "IX");
        skipList_implement->insert_element(12, "XII");
        skipList_implement->insert_element(17, "XVII");
        skipList_implement->insert_element(18, "XVIII");
        skipList_implement->insert_element(19, "XIX");
        skipList_implement->insert_element(20, "XX");
        skipList_implement->insert_element(21, "XXI");
        skipList_implement->insert_element(22, "XXII");
        skipList_implement->insert_element(23, "XXIII");
        skipList_implement->update_element(23, "XXIIIC");

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