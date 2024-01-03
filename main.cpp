#include <iostream>
#include <chrono>
#include <cstdlib>
#include <thread>
#include <ctime>
#include <vector>
#include <random>

#include "skiplist.h"
#include "ThreadPool.h"

#define NUM_THREADS 16                   //线程数量
#define TEST_COUNT 10000000              //测试数据量
std::atomic<int> completedTasks(0);    //用于跟踪已完成的插入任务数量
std::condition_variable cv;              //线程池任务完成信号量
std::mutex mtx_task;                     //线程池任务互斥锁

SkipList<int, std::string> skipList(18);    //实例化跳表对象，根据传入参数确定跳表最大层级

void insertElement(int tid) {
    //std::cout << tid << std::endl;
    int tmp = TEST_COUNT/NUM_THREADS;   //计算每个线程插入操作次数
    for (int i=tid*tmp, count=0; count<tmp; i++) {
        count++;
        skipList.insert_element(rand() % TEST_COUNT, "a");  //rand随机生成键，与字符串“a” 插入跳表
    }
    completedTasks++;  //插入任务完成，增加计数器的值
    if (completedTasks == NUM_THREADS) {
        std::unique_lock<std::mutex> lock(mtx_task);
        cv.notify_all();  //通知唤醒等待的线程
    }
}

void getElement(int tid) {
    //std::cout << tid << std::endl;

    int tmp = TEST_COUNT/NUM_THREADS;   //计算每个线程搜索操作次数
    for (int i=tid*tmp, count=0; count<tmp; i++) {
        count++;
        skipList.search_element(rand() % TEST_COUNT);           //rand随机生成键，在跳表进行 搜索操作
    }
//    completedTasks++;  //搜索任务完成，增加计数器的值
//    if (completedTasks == NUM_THREADS) {
//        std::unique_lock<std::mutex> lock(mtx_task);
//        cv.notify_all();  //通知唤醒等待的线程
//    }
}

void insert_test(){
    srand(time(NULL));
    //创建线程池对象
    ThreadPool pool(NUM_THREADS);
    //std::vector<std::thread> threads;
    auto start = std::chrono::high_resolution_clock::now();
//    for (int i = 0; i < NUM_THREADS; i++) {
//        //std::cout << "main(): creating thread, " << i << std::endl;
//        threads.emplace_back(insertElement, i);
//    }
//    for (auto &thread: threads) {
//        thread.join();
//    }
    for (int i = 0; i < NUM_THREADS; i++) {
        pool.enqueue(insertElement, i);  // 提交插入任务给线程池
    }
    //等待所有任务执行完毕
    std::unique_lock<std::mutex> lock(mtx_task);
    cv.wait(lock, [&]() {
        return completedTasks == NUM_THREADS;
    });
    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    std::cout << "insert elapsed: " << elapsed.count() << "\n";
    std::cout << " Insert QPS:" << (TEST_COUNT/10000)/elapsed.count() << "w" << "\n";
}
void search_test(){
    srand(time(NULL));
    std::vector<std::thread> threads;
    //ThreadPool pool(NUM_THREADS);
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_THREADS; i++) {
        //std::cout << "main(): creating thread, " << i << std::endl;
        threads.emplace_back(getElement, i);
    }

    for (auto &thread: threads) {
        thread.join();
    }
//    for (int i = 0; i < NUM_THREADS; i++) {
//        pool.enqueue(getElement, i);  //提交搜索任务给线程池
//    }
//
//    //等待所有任务执行完毕
//    std::unique_lock<std::mutex> lock(mtx_task);
//    cv.wait(lock, [&]() {
//        return completedTasks == NUM_THREADS;
//    });
    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    std::cout << "search elapsed: " << elapsed.count() << "\n";
    std::cout << " Search QPS:" << (TEST_COUNT/10000)/elapsed.count() << "w" << "\n";
}
void usual_use(){
    //实例化跳表对象 设定最大层级
    SkipList<int, std::string> skipList_implement(16);
    //写入元素
    skipList_implement.insert_element(1,"one");
    skipList_implement.insert_element(2,"two");
    skipList_implement.insert_element(3,"three");
    skipList_implement.insert_element(4,"four");
    skipList_implement.insert_element(5,"five");
    skipList_implement.insert_element(7,"seven");
    skipList_implement.insert_element(8,"eight");
    skipList_implement.insert_element(9,"nine");
    skipList_implement.insert_element(12,"twelve");
    skipList_implement.insert_element(17,"seventeen");
    skipList_implement.insert_element(18,"eighteen");
    skipList_implement.insert_element(19,"nineteen");
    skipList_implement.insert_element(20,"twenty");
    skipList_implement.insert_element(21,"twenty-one");
    skipList_implement.insert_element(22,"twenty-two");
    skipList_implement.insert_element(23,"twenty-three");
    skipList_implement.insert_element(25,"twenty-five");
    skipList_implement.insert_element(27,"twenty-seven");
    //输出跳表元素个数
    std::cout << "skipList size:" << skipList_implement.size() << std::endl;
    //跳表元素持久化到文件
    skipList_implement.dump_file();
    //按键值检索
    skipList_implement.search_element(9);
    skipList_implement.search_element(18);
    skipList_implement.search_element(27);
    //显示跳表
    skipList_implement.display_list();
    //按键值删除
    skipList_implement.delete_element(3);
    skipList_implement.delete_element(7);
    skipList_implement.delete_element(17);
    //输出跳表元素个数
    std::cout << "skipList size:" << skipList_implement.size() << std::endl;
    //显示跳表
    skipList_implement.display_list();
}
int main() {
    insert_test();          //随机写入测试，计算QPS
    //completedTasks = 0;   //重置计数器
    search_test();          //随机读取测试，计算QPS
    //usual_use();          //函数接口使用
    return 0;
}