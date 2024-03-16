#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

/**
 * @brief 线程池类，用于管理和调度多个工作线程执行任务。
 */
class ThreadPool {
public:
    // 删除拷贝和移动构造函数以及拷贝和移动赋值运算符
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(const ThreadPool&&) = delete;
    ThreadPool& operator=(const ThreadPool&&) = delete;

    /**
     * @brief 构造一个新的线程池对象
     * 
     * @param threads 线程数目
     */
    ThreadPool(size_t);
    
    /**
     * @brief 将函数及其参数加入线程池的任务队列中并返回与任务关联的 future 对象
     * 
     * @tparam F 函数类型
     * @tparam Args 参数类型
     * @param f 要执行的函数
     * @param args 函数参数
     * @return std::future<typename std::result_of<F(Args...)>::type> 与任务关联的 future 对象，用于获取任务的结果
     */
    template<class F, class... Args>    //可变参数模板函数。F是可调用对象的类型，Args 是参数包的类型
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;  //std::future对象，获取任务的返回值。 std::result_of表示对象F在给定参数Args...下具体的返回类型

    /**
     * @brief 线程池析构函数
     * 
     */
    ~ThreadPool();
private:
    // 需要确保对线程资源的追踪，以确保能正确的回收它们
    std::vector< std::thread > workers; //存储线程对象的容器
    // 任务队列
    std::queue< std::function<void()> > tasks;  //任务队列，存储待执行的任务，外部调用enqueue()函数添加任务，任务被加入到该队列
    
    // 线程池的同步机制
    std::mutex queue_mutex;                 //互斥锁，保护任务队列的访问
    std::condition_variable condition;      //任务队列为空，工作线程等待条件变量同志，有新的任务加入时被唤醒
    bool stop;                              //指示线程池是否停止，true表示线程池停止
};

//构造函数仅启动一定数量的工作线程
inline ThreadPool::ThreadPool(size_t threads)
    :   stop(false)
{
    for(size_t i = 0;i<threads;++i)
        workers.emplace_back(
            [this]
            {
                for(;;)
                {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock,
                            [this]{ return this->stop || !this->tasks.empty(); });  //  等待，直到满足stop==true 或 任务队列非空
                        if(this->stop && this->tasks.empty())       //如果停止标志为真且任务队列为空，则工作线程退出循环
                            return;
                        task = std::move(this->tasks.front());      //从任务队列中取出一个任务，并将其移动到 task 对象中
                        this->tasks.pop();
                    }

                    task();    //执行任务，调用存储在 task 对象中的函数
                }
            }
        );
}

//向线程池中添加新的任务
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;  //result_of 模板元编程技术，推导出函数 f 的返回类型 return_type

    auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

    std::future<return_type> res = task->get_future();              //获取与任务关联的 std::future 对象，用于获取任务的结果
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        // don't allow enqueueing after stopping the pool
        if(stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

        tasks.emplace([task](){ (*task)(); });//lambda 函数中调用 (*task)() 执行任务，使用 task 的共享指针来确保任务对象在执行完之前不会被销毁
    }
    condition.notify_one(); //唤醒一个等待中的工作线程，通知工作线程有新的任务可用
    return res;             //返回保存任务结果的 std::future 对象
}

//析构函数,回收所有线程资源
inline ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();                 //向所有在等待条件变量的工作线程通知，检查stop标志决定是否退出
    for(std::thread &worker: workers)
        if(worker.joinable()){
            worker.join();                  //等待工作线程结束，正常退出
        }
}

#endif