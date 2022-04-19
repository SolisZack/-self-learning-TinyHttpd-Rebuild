//
// Created by zack solis on 2022/1/29.
//

#ifndef THREADPOOL_THREADPOOL_H
#define THREADPOOL_THREADPOOL_H
#include "funcQueue.h"
#include <future>
#include <vector>
#include <thread>
#include <mutex>
#include <unistd.h>

class ThreadPool {
private:
    bool m_pool_shut;  // 决定线程池是否开启，进而影响ThreadWorker和析构函数
    std::vector<std::thread> m_threads;  // 存储ThreadWorker，以便析构的时候等待线程结束
    FuncQueue m_funcQueue;  // 线程安全的函数队列，用于获取待ThreadWorker执行的函数
    std::mutex m_thread_mtx;  // 互斥锁，用于保证线程安全，防止多个ThreadWorker同时访问函数队列
    std::condition_variable m_thread_cv;  // 条件变量，用于休眠和唤醒ThreadWorker

    // 线程池的基本单位，用于以线程形式执行函数
    class ThreadWorker {
    private:
        int m_id;
        ThreadPool* m_pool;
    public:
        ThreadWorker() = delete;
        ThreadWorker(ThreadPool* pool, int id):m_id(id), m_pool(pool){};

        // 重载运算符()
        void operator()() {
            Func m_func;
            bool get_func;
            while (!m_pool->m_pool_shut) {
                // sleep防止某一线程运行过快一直抢锁
                // std::this_thread::sleep_for(std::chrono::milliseconds(10));
                // 也可以使用在此部分改成代码块（加个大括号）的形式解决，效果更好
                // 以此纪念我debug了一个半小时的问题⬆️
                {
                    std::unique_lock<std::mutex> lock(m_pool->m_thread_mtx);
                    // 如何函数队列为空，则解除锁，同时等待（wait会自动解除锁）
                    if (m_pool->m_funcQueue.funcQueueEmpty())
                        m_pool->m_thread_cv.wait(lock);
                    get_func = m_pool->m_funcQueue.dequeue(m_func);
                }

                if (!get_func)
                    printf("thread id %i get function failed\n", m_id);
                m_func();
            }
        }
    };
public:
    explicit ThreadPool(const int thread_num): m_pool_shut(false), m_threads(std::vector<std::thread>(thread_num)){};
    explicit ThreadPool(): m_pool_shut(false), m_threads(std::vector<std::thread>(sysconf(_SC_NPROCESSORS_ONLN))){};
    ~ThreadPool() = default;

    void init() {
        for (int i = 0; i < m_threads.size(); i++) {
            // at()在遇到无效索引时会抛出out_of_range异常.
            // thread的构造函数会自动调用第一个参数，故重载了ThreadWorker的（）运算符以便让线程自动执行
            m_threads.at(i) = std::thread(ThreadWorker(this, i));
        }
    }

    void shutPool() {
        m_pool_shut = true;
        m_thread_cv.notify_all();  // 因为m_pool_shut被设置成true，所以ThreadWorker将跳过while循环，最终执行完毕
        for (auto& m_thread : m_threads) {
            // 检查当前的线程对象是否表示了一个活动的执行线程
            if (m_thread.joinable())
                m_thread.join();  // 等待线程执行完毕
        }
    }

    // 可变长参数
    template<typename F, typename... Args>
    // "auto 函数(参数)->类型"的形式学名叫"函数返回类型后置"，auto作为占位符，用于自动推导函数的返回值类型
    // 函数的返回值是，储存了f(args...)运行结果的数据类型的future，可以通过futurexxx.get()异步获取该值
    // 思路如果自己忘了可以参考main.cpp里的myTest函数
    auto submitFunc(F&& f, Args&&... args)-> std::future<decltype(f(args...))> {
        // 将传入的函数和参数利用std::bind 打包成一个xxx()形式的函数
        // 注意变长参数的书写形式
        std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        // 将上述函数打包成期物，同时利用智能指针方便后序打包成void()形式
        auto ptr_func = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);
        // 打包成void()形式，这样就可以传入到funcQueue中了
        std::function<void()> wrapped_func = [ptr_func](){(*ptr_func)();};
        // 将打包好的函数传入funcQueue
        m_funcQueue.enqueue(wrapped_func);
        // 唤醒一个线程
        m_thread_cv.notify_one();
        // 返回期物
        return ptr_func->get_future();
    }
};


#endif //THREADPOOL_THREADPOOL_H
