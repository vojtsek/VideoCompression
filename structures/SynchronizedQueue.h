#ifndef SYNCHRONIZEDQUEUE_H
#define SYNCHRONIZEDQUEUE_H
#include <deque>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <functional>
#include "headers/handle_IO.h"

template <typename T>
struct SynchronizedQueue {
    std::deque<T *> queue;
    std::mutex mtx;
    std::condition_variable cond;
    bool being_used;

    SynchronizedQueue(): being_used(false) {}

    void push(T *item) {
        std::unique_lock<std::mutex> lck(mtx, std::defer_lock);
        lck.lock();
        while (being_used) {
            cond.wait(lck);
        }
        being_used = true;
        queue.push_back(item);
        being_used = false;
        lck.unlock();
        cond.notify_one();
    }

    T *pop() {
        T *item = nullptr;
        std::unique_lock<std::mutex> lck(
                    mtx, std::defer_lock);
        lck.lock();
        while (being_used || !queue.size()) {
            cond.wait(lck);
        }
        being_used = true;
        item = queue.front();
        queue.pop_front();
        being_used = false;
        lck.unlock();
        cond.notify_one();
        return item;
    }

    bool remove(T *item) {
        std::unique_lock<std::mutex> lck(
                    mtx, std::defer_lock);
        lck.lock();
        while (being_used) {
            cond.wait(lck);
        }
        being_used = true;
        int size_before = queue.size();
        queue.erase(
            std::remove_if(queue.begin(), queue.end(), [=](T *it) {
                return (it->getHash() == item->getHash());
            }), queue.end());
        int size_after = queue.size();
        being_used = false;
        lck.unlock();
        cond.notify_one();
        return (size_before != size_after);
    }

    bool removeIf(std::function<bool (T *)> func) {
        std::unique_lock<std::mutex> lck(
                    mtx, std::defer_lock);
        lck.lock();
        while (being_used) {
            cond.wait(lck);
        }
        being_used = true;
        queue.erase(
            std::remove_if(queue.begin(), queue.end(), func), queue.end());
        being_used = false;
        lck.unlock();
        cond.notify_one();
        return false;
    }

    void signal() {
         std::unique_lock<std::mutex> lck(
                    mtx, std::defer_lock);
        lck.lock();
        lck.unlock();
        cond.notify_one();
    }
};

#endif // SYNCHRONIZEDQUEUE_H
