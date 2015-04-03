#ifndef SYNCHRONIZEDQUEUE_H
#define SYNCHRONIZEDQUEUE_H

#include <vector>
#include <deque>
#include <unordered_map>
#include <map>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <functional>
#include "headers/handle_IO.h"


template <typename T>
struct SynchronizedMap {
    std::unordered_map<std::string, T *> map;
    std::mutex mtx;
    bool being_used;

    SynchronizedMap(): being_used(false) {}

    void push(T *item) {
        remove(item);
        mtx.lock();
        map.insert(
                    std::make_pair(item->toString(), item));
        mtx.unlock();
    }

    T *get(std::string key) {
        T *item = nullptr;
        mtx.lock();
        try {
            item = map.at(key);
        } catch (std::out_of_range) { }
        mtx.unlock();
        return item;
    }

    bool contains (std::string key) {
        if (get(key) == nullptr) {
            return false;
        }
        return true;
    }

    std::vector<T *> getValues() {
        std::vector<T *> values;
        mtx.lock();
        for (auto &v : map) {
            values.push_back(v.second);
        }
        mtx.unlock();
        return values;
    }

    bool remove(T *item) {
        mtx.lock();
        int32_t size_before = map.size();
        map.erase(item->toString());
        int32_t size_after = map.size();
        mtx.unlock();
        if (size_before != size_after) {
            return true;
        }
        return false;
    }

    void applyTo(std::function<void (T *)> func) {
        mtx.lock();
        std::for_each(map.begin(), map.end(),
                      [&](std::pair<std::string, T *> entry) { func(entry.second); });
        mtx.unlock();
        return false;
    }

    bool removeIf(std::function<bool (T *)> func) {
        mtx.lock();
        map.erase(
                    std::remove_if(map.begin(), map.end(),
                                   [&](std::pair<std::string, T *> entry) {
                        return func(entry.second);
                    }), map.end());
        mtx.unlock();
        return false;
    }

    void clear() {
        mtx.lock();
        map.clear();
        mtx.unlock();
    }
};

template <typename T>
struct SynchronizedQueue {
    std::deque<T *> queue;
    std::mutex mtx;
    std::condition_variable cond;
    bool being_used;

    SynchronizedQueue(): being_used(false) {}

    void push(T *item) {
        if (contains(item)) {
            return;
        }
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

    std::vector<T *> getValues() {
        std::vector<T *> values;
        mtx.lock();
        for (auto &v : queue) {
            values.push_back(v);
        }
        mtx.unlock();
        return values;
    }

    int32_t getSize() {
        int32_t size;
        std::unique_lock<std::mutex> lck(mtx, std::defer_lock);
        lck.lock();
        while (being_used) {
            cond.wait(lck);
        }
        being_used = true;
        size = queue.size();
        being_used = false;
        lck.unlock();
        cond.notify_one();
        return size;
    }

    T *pop() {
        T *item = nullptr;
        std::unique_lock<std::mutex> lck(mtx, std::defer_lock);
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

    void clear() {
        std::unique_lock<std::mutex> lck(mtx, std::defer_lock);
        lck.lock();
        queue.clear();
        lck.unlock();
    }

    bool remove(T *item) {
        std::unique_lock<std::mutex> lck(mtx, std::defer_lock);
        lck.lock();
        while (being_used) {
            cond.wait(lck);
        }
        being_used = true;
        int32_t size_before = queue.size();
        queue.erase(
            std::remove_if(queue.begin(), queue.end(), [=](T *it) {
                return (item->equalsTo(it));
            }), queue.end());
        int32_t size_after = queue.size();
        being_used = false;
        lck.unlock();
        cond.notify_one();
        return (size_before != size_after);
    }

    bool contains(T *item) {
        bool cont = false;
        std::unique_lock<std::mutex> lck(mtx, std::defer_lock);
        lck.lock();
        cont = !(std::find(
                    queue.begin(), queue.end(), item) == queue.end());
        lck.unlock();
        return cont;
    }

    bool removeIf(std::function<bool (T *)> func) {
        std::unique_lock<std::mutex> lck(mtx, std::defer_lock);
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
        std::unique_lock<std::mutex> lck(mtx, std::defer_lock);
        lck.lock();
        lck.unlock();
        cond.notify_one();
    }
};

#endif // SYNCHRONIZEDQUEUE_H
