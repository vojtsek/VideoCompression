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
/*!
 * \brief The SynchronizedMap struct
 * Map serving as a storage with synchronized access.
 */
struct SynchronizedMap {
    //! map to actually hold the data
    std::unordered_map<std::string, T *> map;
    //! mutex to synchronize access
    std::mutex mtx;

    /*!
     * \brief push inserts the item to the map
     * \param item pointer to the item to be inserted
     */
    void push(T *item) {
        if (contains(item->toString())) {
            return;
        }
        mtx.lock();
        map.insert(
                    std::make_pair(item->toString(), item));
        mtx.unlock();
    }

    /*!
     * \brief get gets pointer to the item associated with the given key
     * \param key of the item
     * \return pointer if the key is associated with a value, nullptr otherwise
     */
    T *get(std::string key) {
        T *item = nullptr;
        mtx.lock();
        try {
            // try to get an item, fails silently
            item = map.at(key);
        } catch (std::out_of_range) { }
        mtx.unlock();
        return item;
    }

    /*!
     * \brief contains indicates whether the given key is associated with a value
     * \param key to get info about
     * \return true if the key is associated with a value
     */
    bool contains (std::string key) {
        // synchronization handled by get
        if (get(key) == nullptr) {
            return false;
        }
        return true;
    }

    /*!
     * \brief getValues returns vector of all values contained in the map
     * \return vector of values
     */
    std::vector<T *> getValues() {
        std::vector<T *> values;
        mtx.lock();
        for (auto &v : map) {
            values.push_back(v.second);
        }
        mtx.unlock();
        return values;
    }

    /*!
     * \brief remove removes item
     * \param item, its key is used for comparison
     * \return true if an item was removed
     */
    bool remove(T *item) {
        mtx.lock();
        // to determine, whether size changed
        int64_t size_before = map.size();
        // removal based on key
        map.erase(item->toString());
        int64_t size_after = map.size();
        mtx.unlock();
        // size changed
        if (size_before != size_after) {
            return true;
        }
        return false;
    }

    /*!
     * \brief applyTo applies passed funtion to all items
     * \param func funtion to apply
     */
    void applyTo(std::function<void (T *)> func) {
        mtx.lock();
        std::for_each(map.begin(), map.end(),
                      [&](std::pair<std::string, T *> entry) { func(entry.second); });
        mtx.unlock();
        return false;
    }

    /*!
     * \brief removeIf removes items satisfying the predicate
     * \param func the predicate to compare
     * \return true if an item was removed
     */
    bool removeIf(std::function<bool (T *)> func) {
        mtx.lock();
        int64_t size_before = map.size();
        map.erase(
                    std::remove_if(map.begin(), map.end(),
                                   [&](std::pair<std::string, T *> entry) {
                        return func(entry.second);
                    }), map.end());
        int64_t size_after = map.size();
        mtx.unlock();
        if (size_before != size_after) {
            return true;
        }
        return false;
    }

    /*!
     * \brief clear clears the storage
     */
    void clear() {
        mtx.lock();
        map.clear();
        mtx.unlock();
    }
};

template <typename T>
/*!
 * \brief The SynchronizedQueue struct
 * FIFO storage with synchronized access
 * access is not blocking
 */
struct SynchronizedQueue {
    //! underlying storage
    std::deque<T *> queue;
    //! mutex to synchronize the access
    std::mutex mtx;
    //! condition variable to signal readiness
    std::condition_variable cond;
    //! helper flag
    bool being_used;

    SynchronizedQueue(): being_used(false) {}

    /*!
     * \brief push pushes the item to the queue
     * \param item item to push
     */
    void push(T *item) {
        // no duplicates
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

    /*!
     * \brief getValues returns vector of all the values stored in the queue
     * \return vector of values
     */
    std::vector<T *> getValues() {
        std::vector<T *> values;
        mtx.lock();
        // copy all values
        for (auto &v : queue) {
            values.push_back(v);
        }
        mtx.unlock();
        return values;
    }

    /*!
     * \brief getSize gets count of elements in the queue
     * \return size of queue
     */
    int64_t getSize() {
        int64_t size;
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

    /*!
     * \brief pop pops the item from the queue, returning pointer
     * \return pointer to the item
     */
    T *pop() {
        T *item = nullptr;
        std::unique_lock<std::mutex> lck(mtx, std::defer_lock);
        lck.lock();
        // wait until the queue is empty - may be notified by signal method
        while (being_used || !queue.size()) {
            cond.wait(lck);
        }
        being_used = true;
        // gets the pointer
        item = queue.front();
        // pops the item
        queue.pop_front();
        being_used = false;
        lck.unlock();
        cond.notify_one();
        return item;
    }

    /*!
     * \brief clear clears the underlying storage
     */
    void clear() {
        std::unique_lock<std::mutex> lck(mtx, std::defer_lock);
        lck.lock();
        queue.clear();
        lck.unlock();
    }

    /*!
     * \brief remove removes item
     * \param item item whose string id will be used
     * \return true if an item was removed
     */
    bool remove(T *item) {
        std::unique_lock<std::mutex> lck(mtx, std::defer_lock);
        lck.lock();
        while (being_used) {
            cond.wait(lck);
        }
        being_used = true;
        int64_t size_before = queue.size();
        // erase corresponding items
        queue.erase(
            std::remove_if(queue.begin(), queue.end(), [=](T *it) {
                return (item->toString() == it->toString());
            }), queue.end());
        int64_t size_after = queue.size();
        being_used = false;
        lck.unlock();
        cond.notify_one();
        if (size_before != size_after) {
            return true;
        }
        return false;
    }

    /*!
     * \brief contains determines if an item with the given key is contained in the queue
     * \param item which is being asked about
     * \return true if item is contained in the queue
     */
    bool contains(T *item) {
        bool cont = false;
        std::unique_lock<std::mutex> lck(mtx, std::defer_lock);
        lck.lock();
        // true if contained
        cont = !(std::find(
                    queue.begin(), queue.end(), item) == queue.end());
        lck.unlock();
        return cont;
    }

    /*!
     * \brief remove removes items determined by predicate
     * \param func predicate to select items to remove
     * \return true if an item was removed
     */
    bool removeIf(std::function<bool (T *)> func) {
        std::unique_lock<std::mutex> lck(mtx, std::defer_lock);
        lck.lock();
        while (being_used) {
            cond.wait(lck);
        }
        being_used = true;
        int64_t size_before = queue.size();
        queue.erase(
            std::remove_if(queue.begin(), queue.end(), func), queue.end());
        being_used = false;
        int64_t size_after = queue.size();
        lck.unlock();
        cond.notify_one();
        if (size_before != size_after) {
            return true;
        }
        return false;
    }

    /*!
     * \brief signal signals that the status of the queue has changed
     */
    void signal() {
        std::unique_lock<std::mutex> lck(mtx, std::defer_lock);
        // acquires lock before signaling, so no race condition should occure
        lck.lock();
        lck.unlock();
        cond.notify_one();
    }
};

#endif // SYNCHRONIZEDQUEUE_H
