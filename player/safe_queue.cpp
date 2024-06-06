//
// Created by zichen on 5/28/24.
//

#include "safe_queue.h"

//template<typename T>
//void SafeQueue<T>::enqueue(const T &item)
//{
//    std::unique_lock<std::mutex> lock(mtx);
//    queue.push(item);
//    cond.notify_one();
//}
//
//template<typename T>
//bool SafeQueue<T>::dequeue(T &item)
//{
//    std::unique_lock<std::mutex> lock(mtx);
//    cond.wait(lock, [this]()
//    { return !queue.empty() || stop; });
//    if (queue.empty())
//    {
//        return false;
//    }
//    item = queue.front();
//    queue.pop();
//    return true;
//}
//
//template<typename T>
//void SafeQueue<T>::stop_queue()
//{
//    std::unique_lock<std::mutex> lock(mtx);
//    stop = true;
//    cond.notify_all();
//}