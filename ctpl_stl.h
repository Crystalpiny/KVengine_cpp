#ifndef __ctpl_stl_thread_pool_H__
#define __ctpl_stl_thread_pool_H__

#include <functional>
#include <thread>
#include <atomic>
#include <vector>
#include <memory>
#include <exception>
#include <future>
#include <mutex>
#include <queue>



// 线程池用于运行用户的函数，其签名如下：
//      ret func(int id, other_params)
// 其中 id 是运行该函数的线程的索引
// ret 是某种返回类型


namespace ctpl
{

    namespace detail
    {
        template <typename T>
        class Queue
        {
        public:
            // 添加一个值到队列中
            bool push(T const & value)
            {
                std::unique_lock<std::mutex> lock(this->mutex);
                this->q.push(value);
                return true;
            }
            // 删除检索到的元素，不要用于非整型类型
            bool pop(T & v)
            {
                std::unique_lock<std::mutex> lock(this->mutex);
                if (this->q.empty())
                    return false;
                v = this->q.front();
                this->q.pop();
                return true;
            }
            // 判断队列是否为空
            bool empty()
            {
                std::unique_lock<std::mutex> lock(this->mutex);
                return this->q.empty();
            }
        private:
            std::queue<T> q;
            std::mutex mutex;
        };
    }

    class thread_pool
    {

    public:

        thread_pool() { this->init(); }
        thread_pool(int nThreads) { this->init(); this->resize(nThreads); }

        // 析构函数等待队列中的所有函数都执行完毕
        ~thread_pool()
        {
            this->stop(true);
        }

        // 获取线程池中正在运行的线程数量
        int size() { return static_cast<int>(this->threads.size()); }

        // 获取空闲线程的数量
        int n_idle() { return this->nWaiting; }
        std::thread & get_thread(int i) { return *this->threads[i]; }

        // 改变线程池中的线程数量
        // 应该由一个线程调用，否则要注意不要和 this->stop() 函数交错执行
        // nThreads 必须 >= 0
        void resize(int nThreads)
        {
            // 如果线程数量增加
            if (!this->isStop && !this->isDone)
            {
                int oldNThreads = static_cast<int>(this->threads.size());
                if (oldNThreads <= nThreads)
                {  // if the number of threads is increased
                    this->threads.resize(nThreads);
                    this->flags.resize(nThreads);

                    for (int i = oldNThreads; i < nThreads; ++i)
                    {
                        this->flags[i] = std::make_shared<std::atomic<bool>>(false);
                        this->set_thread(i);
                    }
                }
                else{  // 如果线程数量减少
                    for (int i = oldNThreads - 1; i >= nThreads; --i)
                    {
                        *this->flags[i] = true;  // this thread will finish
                        this->threads[i]->detach();
                    }
                    {
                        // 停止正在等待的已分离线程
                        std::unique_lock<std::mutex> lock(this->mutex);
                        this->cv.notify_all();
                    }
                    // 因为线程已经脱离，所以安全删除
                    this->threads.resize(nThreads);
                    // 因为线程有指向原始标志的shared_ptr的副本，所以安全删除
                    this->flags.resize(nThreads);
                }
            }
        }

        // 清空队列
        void clear_queue()
        {
            std::function<void(int id)> * _f;
            while (this->q.pop(_f))
                delete _f;
        }

        // 弹出原函数的函数包装器
        std::function<void(int)> pop()
        {
            std::function<void(int id)> * _f = nullptr;
            this->q.pop(_f);
            std::unique_ptr<std::function<void(int id)>> func(_f); // 返回时，即使发生异常也删除函数
            std::function<void(int)> f;
            if (_f)
                f = *_f;
            return f;
        }

        // 等待所有计算线程结束并停止所有线程
        // 可以异步调用，以便在等待时不暂停调用线程
        // 如果 isWait == true，则队列中的所有函数都将被运行，否则队列将被清空而不运行函数
        void stop(bool isWait = false)
        {
            if (!isWait)
            {
                if (this->isStop)
                    return;
                this->isStop = true;
                for (int i = 0, n = this->size(); i < n; ++i)
                {
                    *this->flags[i] = true;  // 命令线程停止
                }
                this->clear_queue();  // 清空队列
            }
            else
            {
                if (this->isDone || this->isStop)
                    return;
                this->isDone = true;  // 通知等待的线程结束
            }
            {
                std::unique_lock<std::mutex> lock(this->mutex);
                this->cv.notify_all();  // 停止所有等待的线程
            }
            for (int i = 0; i < static_cast<int>(this->threads.size()); ++i) {  // 等待计算线程结束
                    if (this->threads[i]->joinable())
                        this->threads[i]->join();
            }
            // 如果线程池中没有线程，但队列中有一些函数，则这些函数不会被线程删除
            // 因此，在这里删除它们
            this->clear_queue();
            this->threads.clear();
            this->flags.clear();
        }

        // 向线程池提交函数
        template<typename F, typename... Rest>
        auto push(F && f, Rest&&... rest) ->std::future<decltype(f(0, rest...))> {
            auto pck = std::make_shared<std::packaged_task<decltype(f(0, rest...))(int)>>(
                std::bind(std::forward<F>(f), std::placeholders::_1, std::forward<Rest>(rest)...)
                );
            auto _f = new std::function<void(int id)>([pck](int id) {
                (*pck)(id);
            });
            this->q.push(_f);
            std::unique_lock<std::mutex> lock(this->mutex);
            this->cv.notify_one();
            return pck->get_future();
        }

        // 运行用户的函数，该函数接受 int 类型的参数 - 运行线程的 id。返回值是模板化的
        // 操作符返回 std::future，在那里用户可以获取结果和重新抛出捕获的异常
        template<typename F>
        auto push(F && f) ->std::future<decltype(f(0))> {
            auto pck = std::make_shared<std::packaged_task<decltype(f(0))(int)>>(std::forward<F>(f));
            auto _f = new std::function<void(int id)>([pck](int id) {
                (*pck)(id);
            });
            this->q.push(_f);
            std::unique_lock<std::mutex> lock(this->mutex);
            this->cv.notify_one();
            return pck->get_future();
        }


    private:

        // 已删除
        thread_pool(const thread_pool &);// = delete;
        thread_pool(thread_pool &&);// = delete;
        thread_pool & operator=(const thread_pool &);// = delete;
        thread_pool & operator=(thread_pool &&);// = delete;

        void set_thread(int i)
        {
            // 复制指向标志的 shared_ptr
            std::shared_ptr<std::atomic<bool>> flag(this->flags[i]);
            auto f = [this, i, flag/* 复制指向标志的 shared_ptr */]()
            {
                std::atomic<bool> & _flag = *flag;
                std::function<void(int id)> * _f;
                bool isPop = this->q.pop(_f);
                while (true)
                {
                    // 如果队列中有东西
                    while (isPop)
                    {  
                        // 如果发生异常，返回时删除函数
                        std::unique_ptr<std::function<void(int id)>> func(_f);
                        (*_f)(i);
                        if (_flag)
                            return;  // 如果需要停止线程，则即使队列尚未清空也返回
                        else
                            isPop = this->q.pop(_f);
                    }
                    // 队列在这里为空，等待下一个命令
                    std::unique_lock<std::mutex> lock(this->mutex);
                    ++this->nWaiting;
                    // 等待直到有东西进入队列或者线程需要结束
                    this->cv.wait(lock, [this, &_f, &isPop, &_flag](){ isPop = this->q.pop(_f); return isPop || this->isDone || _flag; });
                    --this->nWaiting;
                    if (!isPop)
                        return;  // 如果队列为空并且 this->isDone == true 或者 *flag，则返回
                }
            };
            this->threads[i].reset(new std::thread(f));
        }

        void init() { this->nWaiting = 0; this->isStop = false; this->isDone = false; }

        std::vector<std::unique_ptr<std::thread>> threads;
        std::vector<std::shared_ptr<std::atomic<bool>>> flags;
        detail::Queue<std::function<void(int id)> *> q;
        std::atomic<bool> isDone;
        std::atomic<bool> isStop;
        std::atomic<int> nWaiting;  // 等待的线程数

        std::mutex mutex;
        std::condition_variable cv;
    };

}

#endif // __ctpl_stl_thread_pool_H__
