#include "structures/NeighborStorage.h"
#include "structures/TransferInfo.h"
#include "helpers/network_helper.h"

#define SYNCHRONIZED_SECTION(CODE) n_mtx.lock(); CODE n_mtx.unlock();
NeighborStorage::NeighborStorage()
{
}

int NeighborStorage::getNeighborCount() {
    int size;
    n_mtx.lock();
    size = neighbors.size();
    n_mtx.unlock();
    return size;
}

int NeighborStorage::removeNeighbor(const struct sockaddr_storage &addr) {
    //TODO: check differently
    if (getNeighborInfo(addr) == nullptr)
        return 0;
    n_mtx.lock();
    free_neighbors.erase(
        std::remove_if(free_neighbors.begin(), free_neighbors.end(),
                   [&](NeighborInfo *ngh) {
                        return cmpStorages(ngh->address, addr);
    }), free_neighbors.end());

    //todo: removing neighbor with algorithms?
    //todo: waiting_chunks
    for (auto it = neighbors.begin(); it != neighbors.end(); ++it) {
        if (cmpStorages(it->second->address, addr)) {
            //TODO: structures handling synchronization of listeners
          //  DATA->periodic_listeners.erase(
               //         it->second->getHash());
            DATA->chunks_to_send.removeIf(
                        [&](TransferInfo *ti) -> bool {
                return cmpStorages(ti->address, it->second->address);
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

void NeighborStorage::applyToNeighbors(
        std::function<void (std::pair<std::string, NeighborInfo *>)> func) {
    n_mtx.lock();
    std::for_each (neighbors.begin(), neighbors.end(), func);
    n_mtx.unlock();
}

void NeighborStorage::setInterval(
        const struct sockaddr_storage &addr, int i) {
    applyToNeighbors([&](
                     std::pair<std::string, NeighborInfo *> entry) {
        if (cmpStorages(entry.second->address, addr)) {
            entry.second->intervals = i;
        }});
    return;
}

void NeighborStorage::updateQuality(
        const struct sockaddr_storage &addr, int q) {
        applyToNeighbors([&](
                     std::pair<std::string, NeighborInfo *> entry) {
            if (cmpStorages(entry.second->address, addr)) {
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
        if (cmpStorages(entry.second->address, addr)) {
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
            return (cmpStorages(ngh->address, addr));
        }), free_neighbors.end());
        n_mtx.unlock();
    } else {
        reportSuccess("Neighbor is now free: " + MyAddr(addr).get());
        free_neighbors.push_back(ngh);
    }
    ngh->free = free;
}

NeighborInfo *NeighborStorage::getFreeNeighbor() {
    NeighborInfo *ngh = nullptr;
    n_mtx.lock();
    if (free_neighbors.empty()) {
        n_mtx.unlock();
        return ngh;
    }
    std::sort(free_neighbors.begin(), free_neighbors.end(),
              [&](NeighborInfo *n1, NeighborInfo *n2) {
        return (n1->quality < n2->quality);
    });
    ngh = *free_neighbors.begin();
        //free_neighbors.erase(free_neighbors.begin());
    n_mtx.unlock();
    return ngh;
}

struct sockaddr_storage NeighborStorage::getRandomNeighbor() {
    int rand_n = rand() % getNeighborCount();
    SYNCHRONIZED_SECTION(
        auto it = neighbors.begin();
        while (rand_n--) {
            it++;
        }
    )
    return it->second->address;
}

std::vector<struct sockaddr_storage>
        NeighborStorage::getNeighborAdresses(int count) {
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
    if (!addrIn(addr, neighbors)) {
        NeighborInfo *ngh = new NeighborInfo(addr);
        ngh->intervals = DATA->config.getValue(
            "NEIGHBOR_CHECK_TIMEOUT");
        neighbors.insert(
            std::make_pair(ngh->getHash(), ngh));
        DATA->periodic_listeners.push(ngh);
        free_neighbors.push_back(ngh);
        MyAddr mad(addr);
        reportDebug("Neighbor added; " + mad.get(), 3);
        if (neighbors.size() == (unsigned)
            DATA->config.getValue("MAX_NEIGHBOR_COUNT")) {
        reportSuccess("Enough neighbors gained.");
        }
    } else
        reportDebug("Already known neighbor.", 4);
    n_mtx.unlock();
}
