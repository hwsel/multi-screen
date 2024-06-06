//
// Created by zichen on 5/28/24.
//

#ifndef CLIENT_SAFE_QUEUE_H
#define CLIENT_SAFE_QUEUE_H


#include <atomic>
#include <queue>
#include <condition_variable>

template<typename T>
class SafeQueue
{
public:
    SafeQueue() : stop(false)
    {}

    void enqueue(const T &item)
    {
        std::unique_lock<std::mutex> lock(mtx);
        queue.push(item);
        cond.notify_one();
    }

    bool dequeue(T &item)
    {
        std::unique_lock<std::mutex> lock(mtx);
        cond.wait(lock, [this]()
        { return !queue.empty() || stop; });
        if (queue.empty())
        {
            return false;
        }
        item = queue.front();
        queue.pop();
        return true;
    }

    void stop_queue()
    {
        std::unique_lock<std::mutex> lock(mtx);
        stop = true;
        cond.notify_all();
    }

    size_t queue_size()
    {
        std::unique_lock<std::mutex> lock(mtx);
        return queue.size();
    }


private:
    std::queue<T> queue;
    std::mutex mtx;
    std::condition_variable cond;
    std::atomic<bool> stop;
};


#endif //CLIENT_SAFE_QUEUE_H
