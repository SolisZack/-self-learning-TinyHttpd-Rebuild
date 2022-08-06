//
// Created by zack solis on 2022/1/29.
//

#ifndef THREADPOOL_FUNCQUEUE_H
#define THREADPOOL_FUNCQUEUE_H

#include <queue>
#include <mutex>
#include <functional>

using Func = std::function<void()>;

class FuncQueue {
private:
    int m_capacity;
    std::queue<Func> m_queue;
    std::mutex m_mutex;
public:
    FuncQueue() {
        m_capacity = 50;
    }

    ~FuncQueue() = default;

    // 将函数装入队列中，为保证
    void enqueue(const Func& func) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_queue.size() > m_capacity)
            m_queue.pop();
        m_queue.emplace(func);
        printf("func enqueue, queue.size:%i\n", m_queue.size());
    }

    bool dequeue(Func& func) {
        std::unique_lock<std::mutex> lock(m_mutex);
        printf("trying to get func, queue.size:%i\n", m_queue.size());
//        std::cout << "trying to get func\n";
        if (m_queue.empty())
            return false;
        func = std::move(m_queue.front());  // move提升性能
        m_queue.pop();
        return true;
    }

    bool funcQueueEmpty() {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    unsigned long size() {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.size();
    }
};

#endif //THREADPOOL_FUNCQUEUE_H
