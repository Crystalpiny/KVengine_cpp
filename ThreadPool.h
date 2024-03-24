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
 * @brief 线程池类
 * 
 * 实现一个简单的线程池，用于管理线程的创建、任务的提交和执行。
 * 
 * @note 
 * - 该类禁用了拷贝构造函数、移动构造函数、拷贝赋值运算符和移动赋值运算符。
 * - 提供了构造函数、析构函数和任务提交函数。
 * - 构造函数用于创建一个新的线程池对象，并指定线程数量。
 * - 任务提交函数将函数及其参数添加到线程池的任务队列中，并返回与任务关联的 future 对象，用于获取任务的结果。
 * - 析构函数用于销毁线程池对象，释放线程资源。
 * - 线程池内部使用互斥锁和条件变量实现线程同步和任务调度。
 * - 线程池中的工作线程不断从任务队列中获取任务并执行，直到线程池被停止。
 */
class ThreadPool {
public:

    // 删除拷贝和移动构造函数以及拷贝和移动赋值运算符
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(const ThreadPool&&) = delete;
    ThreadPool& operator=(const ThreadPool&&) = delete;

    /**
     * @brief 构造函数
     * 
     * 创建一个新的线程池对象，并指定线程数量。
     * 
     * @param threads 线程数量
     * 
     * @note 
     * - 使用初始化列表初始化 stop 成员变量为 false。
     * - 循环创建指定数量的工作线程，并将其加入 workers 容器中。
     * - 每个工作线程会不断从任务队列中获取任务并执行，直到线程池被停止。
     * - 工作线程的执行逻辑包括：
     *   - 等待条件变量 condition，直到满足 stop 为 true 或任务队列非空。
     *   - 如果线程池已停止且任务队列为空，则线程退出循环。
     *   - 从任务队列中取出一个任务，并将其移动到 task 对象中。
     *   - 从任务队列中移除已取出的任务。
     *   - 执行任务，调用存储在 task 对象中的函数。
     */
    ThreadPool(size_t);
    
    /**
     * @brief 任务提交函数
     * 
     * 将函数及其参数加入线程池的任务队列中，并返回与任务关联的 future 对象，用于获取任务的结果。
     * 
     * @tparam F 函数类型
     * @tparam Args 参数类型
     * @param f 要执行的函数
     * @param args 函数参数
     * @return std::future<typename std::result_of<F(Args...)>::type> 与任务关联的 future 对象，用于获取任务的结果
     * 
     * @note 
     * - 使用可变参数模板函数，接收函数类型和参数类型。
     * - 使用 std::result_of 模板元编程技术，推导出函数 f 的返回类型 return_type。
     * - 创建一个 shared_ptr<packaged_task<return_type()>> 对象 task，并使用 bind 函数将函数 f 及其参数绑定到 task 对象。
     * - 获取与任务关联的 future 对象 res。
     * - 使用互斥锁保护任务队列的访问，将 lambda 函数添加到任务队列中。
     * - 唤醒一个等待中的工作线程，通知工作线程有新的任务可用。
     * - 返回保存任务结果的 future 对象 res。
     */
    template<class F, class... Args>    //可变参数模板函数。F是可调用对象的类型，Args 是参数包的类型
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;  //std::future对象，获取任务的返回值。 std::result_of表示对象F在给定参数Args...下具体的返回类型

    /**
     * @brief 析构函数
     * 
     * 销毁线程池对象，释放线程资源。
     * 
     * @note 
     * - 使用互斥锁保护对 stop 成员变量的修改，将 stop 设置为 true。
     * - 通过条件变量通知所有等待中的工作线程，检查 stop 标志决定是否退出。
     * - 循环遍历所有工作线程，如果线程可被加入，则等待线程结束，实现正常退出。
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