#include "structures/NeighborStorage.h"
#include "structures/TransferInfo.h"
#include "helpers/network_helper.h"

#define SYNCHRONIZED_SECTION(CODE) n_mtx.lock(); CODE n_mtx.unlock();
NeighborStorage::NeighborStorage()
{
}

int32_t NeighborStorage::getNeighborCount() {
    int32_t size;
    n_mtx.lock();
    size = neighbors.size();
    n_mtx.unlock();
    return size;
}

int32_t NeighborStorage::removeNeighbor(
        const struct sockaddr_storage &addr) {
    //TODO: check differently
    if (getNeighborInfo(addr) == nullptr)
        return 0;
    n_mtx.lock();
    free_neighbors.erase(
        std::remove_if(free_neighbors.begin(), free_neighbors.end(),
                   [&](NeighborInfo *ngh) {
                        return networkHelper::cmpStorages(ngh->address, addr);
    }), free_neighbors.end());

    //todo: removing neighbor with algorithms?
    //todo: waiting_chunks
    for (auto it = neighbors.begin(); it != neighbors.end(); ++it) {
        if (networkHelper::cmpStorages(it->second->address, addr)) {
            //TODO: structures handling synchronization of listeners
          //  DATA->periodic_listeners.erase(
               //         it->second->toString());
            DATA->chunks_to_send.removeIf(
                        [&](TransferInfo *ti) -> bool {
                return networkHelper::cmpStorages(
                            ti->address, it->second->address);
            });

            DATA->periodic_listeners.removeIf(
                        [&](Listener *listener) -> bool {
                return (listener->toString() == it->second->toString());
            });
            //delete it->second;
            neighbors.erase(it);
            reportError("Removed neighbor: " + MyAddr(addr).get());
            n_mtx.unlock();
            return 1;
        }
    }
    n_mtx.unlock();
    return 0;
}

void NeighborStorage::removeDirty() {
    std::vector<struct sockaddr_storage> to_remove;
    n_mtx.lock();
    for (const auto &n : neighbors) {
        if (n.second->dirty) {
            to_remove.push_back(n.second->address);
        }
    }
    n_mtx.unlock();
    for (const auto &a : to_remove) {
        removeNeighbor(a);
    }
}

void NeighborStorage::applyToNeighbors(
        std::function<void (std::pair<std::string, NeighborInfo *>)> func) {
    n_mtx.lock();
    std::for_each (neighbors.begin(), neighbors.end(), func);
    n_mtx.unlock();
}

void NeighborStorage::setInterval(
        const struct sockaddr_storage &addr, int32_t i) {
    applyToNeighbors([&](
                     std::pair<std::string, NeighborInfo *> entry) {
        if (networkHelper::cmpStorages(entry.second->address, addr)) {
            entry.second->intervals = i;
        }});
    return;
}

void NeighborStorage::updateQuality(
        const struct sockaddr_storage &addr, int32_t q) {
        applyToNeighbors([&](
                     std::pair<std::string, NeighborInfo *> entry) {
            if (networkHelper::cmpStorages(entry.second->address, addr)) {
                entry.second->quality += q;
            }});
    return;
}

NeighborInfo *NeighborStorage::getNeighborInfo(
        const sockaddr_storage &addr) {
    NeighborInfo *res = nullptr;
    n_mtx.lock();
    std::for_each (neighbors.begin(), neighbors.end(),
                   [&](std::pair<std::string, NeighborInfo *> entry) {
        if (networkHelper::cmpStorages(entry.second->address, addr)) {
            res = entry.second;
        }
    });
    n_mtx.unlock();
    return res;
}

void NeighborStorage::setNeighborFree(const struct sockaddr_storage &addr,
                                      bool free) {
    NeighborInfo *ngh = getNeighborInfo(addr);
    if (ngh == nullptr) {
        return;
    }
    if (ngh->free == free) {
       return;
    }
    if (!free) {
        n_mtx.lock();
        free_neighbors.erase(
            std::remove_if (free_neighbors.begin(), free_neighbors.end(),
               [&](NeighborInfo *ngh) {
            return (networkHelper::cmpStorages(ngh->address, addr));
        }), free_neighbors.end());
        n_mtx.unlock();
    } else {
        reportSuccess("Neighbor is now free: " + MyAddr(addr).get());
        free_neighbors.push_back(ngh);
    }
    ngh->free = free;
}

int32_t NeighborStorage::getFreeNeighbor(struct sockaddr_storage &addr) {
    n_mtx.lock();
    if (free_neighbors.empty()) {
        n_mtx.unlock();
        return 0;
    }
    std::sort(free_neighbors.begin(), free_neighbors.end(),
              [&](NeighborInfo *n1, NeighborInfo *n2) {
        return (n1->quality < n2->quality);
    });
    addr = (*free_neighbors.begin())->address;
    n_mtx.unlock();
    return 1;
}

int32_t NeighborStorage::getRandomNeighbor(struct sockaddr_storage  &addr) {
    int count;
    if (!(count = getNeighborCount())) {
        return -1;
    }
    int32_t rand_n = rand() % getNeighborCount();
    SYNCHRONIZED_SECTION(
        auto it = neighbors.begin();
        while (rand_n--) {
            it++;
        }
    )
    addr = it->second->address;
    return 0;
}

std::vector<struct sockaddr_storage>
        NeighborStorage::getNeighborAdresses(int32_t count) {
    SYNCHRONIZED_SECTION(
        std::vector<struct sockaddr_storage> result;
        for (auto neighbor : neighbors) {
            result.push_back(neighbor.second->address);
            if (!--count) {
                break;
            }
        })
    return result;
}

void NeighborStorage::addNewNeighbor(const struct sockaddr_storage &addr) {
    n_mtx.lock();
    if (!networkHelper::addrIn(addr, neighbors)) {
        NeighborInfo *ngh = new NeighborInfo(addr);
        ngh->intervals = DATA->config.getIntValue(
            "NEIGHBOR_CHECK_TIMEOUT");
        neighbors.insert(
            std::make_pair(ngh->toString(), ngh));
        DATA->periodic_listeners.push(ngh);
        free_neighbors.push_back(ngh);
        MyAddr mad(addr);
        reportDebug("Neighbor added; " + mad.get(), 1);
        if (neighbors.size() == (unsigned)
            DATA->config.getIntValue("MAX_NEIGHBOR_COUNT")) {
            reportSuccess("Enough neighbors gained.");
        }
    } else {
        reportDebug("Already known neighbor.", 2);
    }
    n_mtx.unlock();
}
