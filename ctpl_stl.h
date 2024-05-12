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

#include "logMod.h"



// 线程池用于运行用户的函数，其签名如下：
//      ret func(int id, other_params)
// 其中 id 是运行该函数的线程的索引
// ret 是某种返回类型


namespace ctpl
{

    namespace detail
    {
        /**
         * @brief 线程安全的队列实现。
         *
         * 该类提供了一个线程安全的队列，允许多个线程同时进行元素的入队和出队操作。
         *
         * @tparam T 队列中存储的元素类型。
         */
        template <typename T>
        class Queue
        {
        public:
            /**
             * @brief 将一个值添加到队列中。
             *
             * 该函数将一个值添加到队列的末尾。
             *
             * @param value 要添加到队列的值。
             * @return 如果成功添加值，则返回 true；否则返回 false。
             *
             * @note 该函数是线程安全的。
             */
            bool push(T const & value)
            {
                std::unique_lock<std::mutex> lock(this->mutex);
                this->q.push(value);
                return true;
            }

            /**
             * @brief 删除并检索队列的前端元素。
             *
             * 该函数从队列中删除并存储前端元素到给定的变量中。
             * 如果队列为空，则返回 false。
             *
             * @param v 用于存储检索到的元素的变量。
             * @return 如果成功检索到元素，则返回 true；如果队列为空，则返回 false。
             *
             * @note 该函数是线程安全的。
             * @note 该函数不适用于非整型类型。
             */
            bool pop(T & v)
            {
                std::unique_lock<std::mutex> lock(this->mutex);
                if (this->q.empty())
                    return false;
                v = this->q.front();
                this->q.pop();
                return true;
            }

            /**
             * @brief 检查队列是否为空。
             *
             * 该函数检查队列是否为空。
             *
             * @return 如果队列为空，则返回 true；否则返回 false。
             *
             * @note 该函数是线程安全的。
             */
            bool empty()
            {
                std::unique_lock<std::mutex> lock(this->mutex);
                return this->q.empty();
            }
        private:
            std::queue<T> q;    // 用于存储元素的底层容器。
            std::mutex mutex;   // 用于线程安全访问队列的互斥锁。
        };
    }

/**
 * @class thread_pool
 * @brief 线程池用于并行执行多个任务。
 * 
 * @details
 * 这个线程池类允许用户提交不同的任务（函数对象），这些任务将会被放入队列中，
 * 并由线程池中的工作线程并发执行。每个任务都会在单独的线程中执行，这样可以
 * 提高程序的执行效率，特别是在处理大量独立任务时。
 * 
 * @note
 * - 线程池的工作原理是预先创建一定数量的工作线程，并将这些线程保持在等待状态。
 * - 用户提交的任务会被封装成函数对象并放入一个任务队列中。线程池中的线程将会轮流从队列中取出任务并执行。
 * - 当线程执行完一个任务后，它会再次尝试从任务队列中取出任务。
 * - 如果任务队列为空，则线程会等待直到有新的任务被提交。
 * - 如果线程池被销毁或者收到停止的指令，所有工作线程将会完成当前任务后退出。
 * 
 * - 线程池中的每个线程都有一个关联的标志，该标志用于指示线程是否应该停止执行并退出。
 * - 当调用 `stop` 方法时，所有线程的标志都会被设置为 `true`，这样线程在完成当前任务后就会检查标志并退出。
 * - 如果 `stop` 方法中的 `isWait` 参数被设置为 `false`，线程池会清空任务队列并立即停止所有线程。
 * 
 */
    class thread_pool
    {

    public:
        /**
         * @brief 默认构造函数，初始化线程池。
         * 
         * @details
         * 调用 init() 方法来初始化线程池的状态，包括设置等待的线程数量为 0，
         * 设置 isStop 和 isDone 标志为 false。不会预创建工作线程，线程池开始时为空。
         * 
         * @note
         * 线程池初始化后，默认处于空闲状态，直到提交了任务或者调用了 resize 方法，
         * 才会创建工作线程。线程池在初始化时不会立即开始执行任务，需要先通过 push 方法
         * 提交任务到线程池。
         */
        thread_pool() { this->init(); }
        /**
         * @brief 带参数的构造函数，初始化线程池并设置线程数量。
         * 
         * @details
         * 首先调用 init() 方法来初始化线程池的状态，然后调用 resize(nThreads)
         * 方法来设置线程池的线程数量。这允许用户在创建线程池时直接指定需要的工作线程数量。
         * 
         * @param nThreads 工作线程的数量。如果 nThreads 小于等于 0，则不会创建任何工作线程。
         * 
         * @note
         * 如果 nThreads 大于 0，则线程池会预创建指定数量的工作线程，并等待任务的提交。
         * 工作线程创建后会立即进入等待状态，一旦有任务被提交到线程池，它们就会开始执行任务。
         * 线程池在创建时会分配资源，包括线程和同步原语，因此在不需要时应该及时销毁线程池，
         * 以释放这些资源。
         */
        thread_pool(int nThreads)
        {
            LOG_INFO << "Initializing thread pool with " << nThreads << " threads.";
            this->init();
            this->resize(nThreads);
        }

        /**
         * @brief 析构函数，确保所有任务执行完毕后再销毁线程池。
         * 
         * @details
         * 在析构时调用 stop(true) 方法，它会等待队列中的所有任务执行完毕。
         * 这样可以保证所有的工作线程在销毁线程池之前都已经完成了它们的任务。
         * 
         * @note
         * 如果 stop 方法的参数 isWait 设置为 true，那么在线程池销毁前，
         * 会等待所有已经提交到线程池的任务完成执行。如果有大量长时间运行的任务，
         * 这可能会导致程序在析构时阻塞较长时间。
         * 
         * 如果在析构函数中不希望等待所有任务完成，可以在调用析构函数之前，
         * 手动调用 stop(false) 来快速停止所有工作线程并清空任务队列，
         * 但这可能导致一些任务没有被执行。
         */
        ~thread_pool()
        {
            LOG_INFO << "Destroying thread pool.";
            this->stop(true);
        }

        /**
         * @brief 获取线程池中正在运行的线程数量。
         * 
         * @details
         * 此方法返回线程池中当前活动（已创建且可能正在执行任务或等待任务）的线程数量。
         * 
         * @return 返回线程池中线程的数量，类型为 int。
         * 
         * @note
         * 此方法返回的是线程池当前的线程总数，不区分线程是否正在执行任务。
         * 如果需要知道有多少线程是空闲的，应当调用 n_idle() 方法。
         */
        int size() { return static_cast<int>(this->threads.size()); }

        /**
         * @brief 获取线程池中空闲（当前没有执行任务）的线程数量。
         * 
         * @details
         * 此方法返回当前没有执行任务的线程数量。这些线程已经创建并处于等待状态，
         * 准备接收并执行新的任务。
         * 
         * @return 返回线程池中空闲的线程数量，类型为 int。
         * 
         * @note
         * 空闲线程数量可以用来了解线程池的负载情况。如果空闲线程数量较少，
         * 可能表明线程池正在处理较多的任务或需要增加线程数量以提高并发处理能力。
         */
        int n_idle() { return this->nWaiting; }

        /**
         * @brief 获取线程池中指定索引位置的线程的引用。
         * 
         * @details
         * 此方法通过索引获取线程池中的线程。可以用于进一步的操作，比如获取线程状态。
         * 
         * @param i 线程的索引，从 0 开始计数。
         * 
         * @return 返回对应索引位置线程的引用，类型为 std::thread&。
         * 
         * @note
         * 在使用此方法时需要确保索引 i 不超出线程池大小，否则可能会引发越界错误。
         * 在多线程环境下获取线程引用应当谨慎，避免产生竞态条件。
         */
        std::thread & get_thread(int i) { return *this->threads[i]; }

        /**
         * @brief 调整线程池中的线程数量。
         * 
         * @details
         * 此方法用于增加或减少线程池中的工作线程数量。如果指定的线程数量大于当前数量，
         * 新线程将被创建并添加到线程池中；如果指定的线程数量小于当前数量，部分线程将会被停止并移除。
         * 
         * @param nThreads 想要设置的线程数量，必须大于或等于0。
         * 
         * @note
         * 当增加线程数量时，新线程将初始化并等待新的任务。当减少线程数量时，超出部分的线程将被通知停止，
         * 一旦它们完成了当前正在执行的任务，就会被销毁。这个方法应该谨慎调用，因为在操作过程中可能会影响正在执行的任务。
         * 特别是，不应该在多个线程中同时调用这个方法，同时也不应该与 `stop()` 方法交错执行，以免导致不可预料的结果。
         * 
         * 在调整线程数量时，线程池会锁定内部的互斥量来确保操作的原子性，因此，这个方法可能会阻塞调用线程，直到可以安全地
         * 修改线程数量为止。
         * 
         * 在减少线程数量时，将尝试安全地停止并分离多余的线程，保证它们完成当前任务后不再接收新任务。一旦线程被分离，
         * 它们所占用的资源将在线程自然结束时被系统回收。
         */
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

        /**
         * @brief 清空任务队列，删除所有等待执行的任务。
         * 
         * @details
         * 此方法会删除队列中的所有任务。这些任务是以 std::function<void(int id)> 类型的指针形式存储的。
         * 方法会遍历队列，弹出并删除每个任务指针。
         * 
         * @note
         * 使用此方法将丢失所有未执行的任务，请谨慎使用。在调用此方法后，线程池中将没有等待执行的任务。
         * 如果线程池正在执行 stop 方法，通常会在停止线程前调用此方法以清空队列。
         */
        void clear_queue()
        {
            std::function<void(int id)> * _f;
            while (this->q.pop(_f))
                delete _f;
        }

        /**
         * @brief 从任务队列中弹出一个任务并返回。
         * 
         * @details
         * 此方法会弹出队列中的下一个任务，该任务是一个接受 int 参数的函数包装器 std::function<void(int)>。
         * 如果队列为空，则返回的 std::function 对象将不包含任何任务（即默认构造的 std::function）。
         * 
         * @return 返回弹出的任务，类型为 std::function<void(int)>。如果队列为空，则返回值不包含任务。
         * 
         * @note
         * 在弹出任务后，原始的任务指针将被封装在 unique_ptr 中，确保无论因何种原因退出此方法，任务指针都会被正确删除，
         * 防止内存泄漏。此方法通常用在线程池内部的工作线程中，用于获取新的任务并执行。
         */
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

        /**
         * @brief 停止线程池中的所有工作线程，并根据 isWait 的值处理队列中的任务。
         * 
         * @param isWait 一个布尔标志，指示在停止线程前是否等待队列中的任务执行完毕。
         *        如果 isWait 为 true，则函数运行队列中的所有任务，否则队列将被清空。
         * 
         * @details
         * 如果 isWait 参数为 false，此方法将设置线程池的停止标志，使所有工作线程停止执行新任务，
         * 并清空任务队列。如果 isWait 为 true，则设置完成标志，让线程池完成队列中的所有任务后停止。
         * 
         * @note
         * 此方法可以异步调用，这样调用线程在等待线程池停止的过程中不会被阻塞。
         * 
         * 在调用此方法后，线程池将不再接受新的任务。如果线程池已经处于停止状态或完成状态，
         * 再次调用此方法将没有任何效果。
         * 
         * 在所有线程停止后，此方法还会清除所有未处理的任务，并清空线程及标志的向量。
         * 
         * 使用此方法时需要确保所有工作线程都能够响应停止信号，并且能够安全地停止当前正在执行的任务。
         * 否则，可能会导致不完整的任务执行或资源泄漏。
         */
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
            LOG_INFO << "Thread pool stopped. All threads have been joined.";
            this->threads.clear();
            this->flags.clear();
        }

        /**
         * @brief 将函数提交到线程池中执行，并提供能够获取函数返回值的 future。
         * 
         * @tparam F 函数类型。
         * @tparam Rest 函数参数类型的包扩展。
         * @param f 函数对象，该函数将被提交到线程池。
         * @param rest 函数对象的参数。
         * 
         * @return 返回一个 std::future 对象，用户可以通过它获取函数的返回值或捕获异常。
         * 
         * @details
         * 此模板函数允许用户提交任何可调用的函数对象到线程池中异步执行。
         * 函数对象和其参数首先被绑定为一个 std::packaged_task 对象，然后被封装到一个
         * std::function<void(int)> 对象中，并被添加到任务队列中。
         * 
         * @note
         * 提交的函数必须能够接受一个 int 类型的参数，该参数为线程的 id。
         * 提交函数后，用户不需要等待函数执行完成，可以继续其他操作。
         * 通过返回的 std::future 对象，用户可以在适当的时候获取函数的执行结果。
         * 
         * 使用 std::bind 和 std::placeholders::_1 来绑定线程的 id 作为函数的第一个参数。
         * 使用 std::forward 来完美转发函数对象和其参数，保持它们的值类别和原有状态。
         */
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

        /**
         * @brief 将不接受额外参数的函数提交到线程池中执行，并提供能够获取函数返回值的 future。
         * 
         * @tparam F 函数类型。
         * @param f 函数对象，该函数将被提交到线程池。
         * 
         * @return 返回一个 std::future 对象，用户可以通过它获取函数的返回值或捕获异常。
         * 
         * @details
         * 此重载版本的 push 方法用于提交不需要额外参数（除了线程 id）的函数对象。
         * 函数对象被封装到一个 std::packaged_task 对象中，并被添加到任务队列中。
         * 
         * @note
         * 提交的函数必须能够接受一个 int 类型的参数，该参数为线程的 id。
         * 通过返回的 std::future 对象，用户可以在适当的时候获取函数的执行结果。
         * 
         * 使用 std::forward 来完美转发函数对象，保持其值类别和原有状态。
         */
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

        // 禁止拷贝和移动构造函数以及赋值操作符，确保线程池对象不可拷贝和移动。
        thread_pool(const thread_pool &);// = delete;
        thread_pool(thread_pool &&);// = delete;
        thread_pool & operator=(const thread_pool &);// = delete;
        thread_pool & operator=(thread_pool &&);// = delete;

        /**
         * @brief 初始化并启动线程池中的线程。
         * 
         * @param i 线程索引。
         * 
         * @details
         * 此方法用于启动线程池中的一个新线程。它将创建一个新线程并将其添加到线程列表中。
         * 新线程将运行一个循环，不断从任务队列中取出任务并执行。
         * 
         * @note
         * 在新线程的运行循环中，如果线程被标记为停止或线程池被销毁，则线程将退出循环并结束。
         * 否则，线程将等待新任务的到来并执行它们。
         */
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
            LOG_INFO << "Creating thread with index: " << i;
            this->threads[i].reset(new std::thread(f));
        }

        /**
         * @brief 初始化线程池的状态变量。
         * 
         * @details
         * 此方法将设置线程池的等待线程数为 0，并将停止和完成标志设置为 false。
         * 这个方法通常在线程池构造函数中调用，以初始化线程池的状态。
         */
        void init() { this->nWaiting = 0; this->isStop = false; this->isDone = false; }

        // 线程池的线程列表
        std::vector<std::unique_ptr<std::thread>> threads;
        // 标志列表，用于通知线程停止
        std::vector<std::shared_ptr<std::atomic<bool>>> flags;
        // 任务队列
        detail::Queue<std::function<void(int id)> *> q;
        // 完成标志，用于通知线程所有任务都已完成
        std::atomic<bool> isDone;
        // 停止标志，用于通知线程停止执行并退出
        std::atomic<bool> isStop;
        // 等待的线程数
        std::atomic<int> nWaiting;

        std::mutex mutex;
        std::condition_variable cv;
    };

}

#endif // __ctpl_stl_thread_pool_H__
