#include "structures/NeighborStorage.h"
#include "structures/TransferInfo.h"
#include "helpers/network_helper.h"
#include "helpers/chunk_helper.h"
#include "helpers/OShelper.h"

#define SYNCHRONIZED_SECTION(CODE) n_mtx.lock(); CODE n_mtx.unlock();
NeighborStorage::NeighborStorage()
{
}

int64_t NeighborStorage::getNeighborCount() {
    int64_t size;
    n_mtx.lock();
    size = neighbors.size();
    n_mtx.unlock();
    return size;
}

int64_t NeighborStorage::removeNeighbor(
        const struct sockaddr_storage &addr) {
    // exists the neighbor?
    n_mtx.lock();
    if (!_contains(addr)) {
        n_mtx.unlock();
        return 0;
    }
    // erase from free negihbors
    free_neighbors.erase(
        std::remove_if(free_neighbors.begin(), free_neighbors.end(),
                   [&](NeighborInfo *ngh) {
                        return networkHelper::cmpStorages(ngh->address, addr);
    }), free_neighbors.end());

    NeighborInfo *ngh = _get(addr);
    if (ngh == nullptr) {
        n_mtx.unlock();
        return 0;
    }
        // remove from periodic listeners, checks no longer needed
        DATA->periodic_listeners.removeIf(
                                [&](Listener *listener) -> bool {
                return (listener->toString() == ngh->toString());
        });
        // remove from neighbors list
        neighbors.erase(neighbors.find(ngh->toString()));
        if (neighbors.size() < (unsigned)
                DATA->config.getIntValue("MAX_NEIGHBOR_COUNT")) {
                DATA->state.enough_neighbors = false;
        }
        reportError("Removed neighbor: " + MyAddr(addr).get());
        trashNeighborChunks(addr);
    for (const auto &ti : ngh->chunks_assigned) {
        DATA->periodic_listeners.remove((Listener *) ti);
        ti->assigned = false;
        reportDebug("Repushing " + ti->toString(), 2);
        chunkhelper::pushChunkSend(ti);
    }
    delete ngh;
    n_mtx.unlock();
        printNeighborsInfo();
    return 0;
}

void NeighborStorage::removeDirty() {
    std::vector<struct sockaddr_storage> to_remove;
    n_mtx.lock();
    // first earns neighbors to be remove
    for (const auto &n : neighbors) {
        if (n.second->dirty) {
            to_remove.push_back(n.second->address);
        }
    }
    n_mtx.unlock();
    // remove each one by one, so no deadlock occurs
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
        const struct sockaddr_storage &addr, int64_t i) {
    // synchronization assured
    applyToNeighbors([&](
                     std::pair<std::string, NeighborInfo *> entry) {
        if (networkHelper::cmpStorages(entry.second->address, addr)) {
            entry.second->intervals = i;
        }});
    return;
}

void NeighborStorage::updateQuality(
        const struct sockaddr_storage &addr, int64_t q) {
    // synchronization assured
        applyToNeighbors([&](
                     std::pair<std::string, NeighborInfo *> entry) {
            if (networkHelper::cmpStorages(entry.second->address, addr)) {
                entry.second->quality += q;
            }});
    return;
}

NeighborInfo *NeighborStorage::_get(const sockaddr_storage &addr) {
    NeighborInfo *res = nullptr;
    std::for_each (neighbors.begin(), neighbors.end(),
                   [&](std::pair<std::string, NeighborInfo *> entry) {
        // earns the NeighborInfo structure pointer
        if (networkHelper::cmpStorages(entry.second->address, addr)) {
            res = entry.second;
        }
    });
    return res;
}

NeighborInfo *NeighborStorage::getNeighborInfo(
        const sockaddr_storage &addr) {
    NeighborInfo *res = nullptr;
        n_mtx.lock();
        res = _get(addr);
        n_mtx.unlock();
    return res;
}

void NeighborStorage::setNeighborFree(const struct sockaddr_storage &addr,
                                      bool free) {
    NeighborInfo *ngh = getNeighborInfo(addr);
    n_mtx.lock();

    //the neighbor was removed propably
    if (!_contains(addr)) {
        n_mtx.unlock();
        return;
    }

    // no change needed
    if (ngh->free == free) {
        n_mtx.unlock();
       return;
    }

    if (!free) {
        // setting not free
        free_neighbors.erase(
            std::remove_if (free_neighbors.begin(), free_neighbors.end(),
               [&](NeighborInfo *ngh) {
            return (networkHelper::cmpStorages(ngh->address, addr));
        }), free_neighbors.end());
        reportDebug("Neighbor is not free: " + MyAddr(addr).get(), 2);
    } else {
        // set free
        reportDebug("Neighbor is now free: " + MyAddr(addr).get(), 2);
        free_neighbors.push_back(ngh);
    }
    ngh->free = free;
    n_mtx.unlock();
}

int64_t NeighborStorage::getFreeNeighbor(struct sockaddr_storage &addr) {
    n_mtx.lock();
    // no neighbors at all
    if (free_neighbors.empty()) {
        n_mtx.unlock();
        return 0;
    }
    // sort free neighbors with respect to quality
    std::sort(free_neighbors.begin(), free_neighbors.end(),
              [&](NeighborInfo *n1, NeighborInfo *n2) {
        return (n1->quality < n2->quality);
    });
    // pick the best one
    addr = (*free_neighbors.begin())->address;
    n_mtx.unlock();
    return 1;
}

int64_t NeighborStorage::getRandomNeighbor(struct sockaddr_storage  &addr) {
    int count;
    // no neighbors at all
    if (!(count = getNeighborCount())) {
        return 0;
    }
    // get random neighbor
    int64_t rand_n = rand() % getNeighborCount();
    SYNCHRONIZED_SECTION(
        auto it = neighbors.begin();
        while (rand_n-- > 0) {
            it++;
        }
        addr = it->second->address;
    )
    return 1;
}

std::vector<struct sockaddr_storage>
        NeighborStorage::getNeighborAdresses(int64_t count) {
    SYNCHRONIZED_SECTION(
        std::vector<struct sockaddr_storage> result;
            // simply picks first $count neighbors
        for (auto neighbor : neighbors) {
            result.push_back(neighbor.second->address);
            if (!--count) {
                break;
            }
        })
    return result;
}

void NeighborStorage::trashNeighborChunks(
        const sockaddr_storage &addr) {
    // removes if there are some chunks addressed to this neighbor
            DATA->chunks_to_send.removeIf(
                        [&](TransferInfo *ti) -> bool {
                return ((ti->addressed && networkHelper::cmpStorages(ti->address, addr)) ||
                         (networkHelper::cmpStorages(ti->src_address, addr)));
            });

        // traverse chunks to be encoded
    for (auto &ti :
         DATA->chunks_received.getValues()) {
        // corresponding chunk
        if (networkHelper::cmpStorages(
                    ti->address, addr)) {
                    OSHelper::rmFile(TO_PROCESS_PATH + PATH_SEPARATOR +
                             ti->name + ti->original_extension);
                    OSHelper::rmFile(PROCESSED_PATH + PATH_SEPARATOR +
                             ti->name + ti->desired_extension);
                    chunkhelper::trashChunk(ti, true);
        }
    }
}

void NeighborStorage::removeNeighborChunk(NeighborInfo *ngh,
                                          TransferInfo *ti) {

    ngh->chunks_assigned.erase(
        std::remove_if (ngh->chunks_assigned.begin(), ngh->chunks_assigned.end(),
           [&](TransferInfo *that) {
        return (that->toString() == ti->toString());
    }), ngh->chunks_assigned.end());
}

void NeighborStorage::assignChunk(const sockaddr_storage &addr,
                                  bool assign, TransferInfo *ti) {
    n_mtx.lock();
    NeighborInfo *ngh = _get(addr);
    if (ngh == nullptr) {
        return;
    }
    if (assign) {
        ti->assigned = true;
        ngh->chunks_assigned.push_back(ti);
        reportDebug(MyAddr(ngh->address).get() + ": Assigning " + ti->toString(), 2);
    } else {
        reportDebug("Unassigning " + ti->toString(), 2);
        ti->assigned = false;
        removeNeighborChunk(ngh, ti);
    }
    n_mtx.unlock();
}

void NeighborStorage::printNeighborsInfo() {
    DATA->io_data.info_handler.clear();
    MSG_T type = PLAIN;
    n_mtx.lock();
    for (const auto &n : neighbors) {
        if (!n.second->free) {
            // busy neighbors are red
            type = ERROR;
        }
        DATA->io_data.info_handler.add(
                    n.second->getInfo(), type);
        type = PLAIN;
    }
    n_mtx.unlock();
    DATA->io_data.info_handler.print();
}

void NeighborStorage::addNewNeighbor(const struct sockaddr_storage &addr) {
    n_mtx.lock();
    // don't know this neighbor yet
    // also whether not self
    if ((!_contains(addr)) &&
         (!networkHelper::cmpStorages(
              DATA->config.my_IP.getAddress(), addr))) {
        // initialize the neighbor
        NeighborInfo *ngh = new NeighborInfo(addr);
        ngh->intervals = DATA->config.getIntValue(
            "NEIGHBOR_CHECK_TIMEOUT");

        // add to the list
        neighbors.emplace(
            ngh->toString(), ngh);

        // start checking
        DATA->periodic_listeners.push(ngh);
        free_neighbors.push_back(ngh);
        MyAddr mad(addr);
        reportDebug("Neighbor added; " + mad.get(), 1);
        if (neighbors.size() == (unsigned)
            DATA->config.getIntValue("MAX_NEIGHBOR_COUNT")) {
            reportSuccess("Enough neighbors gained.");
            DATA->state.enough_neighbors = true;
        }
    } else {
        reportDebug("Already known neighbor.", 3);
    }
    n_mtx.unlock();
    printNeighborsInfo();
}

void NeighborStorage::clear() {
    for (const auto &n : neighbors) {
        removeNeighbor(n.second->address);
    }
}


bool NeighborStorage::_contains(const sockaddr_storage &addr) {
    if (neighbors.find(_createHash(addr)) ==
            neighbors.end()) {
        return false;
    }
    return true;
}

bool NeighborStorage::contains(const sockaddr_storage &addr) {
    bool is_contained;
    n_mtx.lock();
    is_contained = _contains(addr);
    n_mtx.unlock();
    return is_contained;
}

std::string NeighborStorage::_createHash(
        const sockaddr_storage &addr) {
    return NeighborInfo(addr).toString();
}
