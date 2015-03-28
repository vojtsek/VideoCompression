#ifndef NEIGHBORSTORAGE_H
#define NEIGHBORSTORAGE_H
#include <vector>
#include <functional>
#include <mutex>
#include <arpa/inet.h>
#include "headers/enums_types.h"

struct NeighborInfo;
class NeighborStorage
{
    neighbor_storageT neighbors;
    std::vector<NeighborInfo *> free_neighbors;
    std::vector<struct sockaddr_storage> potential_neighbors;
    int32_t listening_sock;
    std::mutex n_mtx;
public:
    int32_t getNeighborCount();
    NeighborInfo *getNeighborInfo(const struct sockaddr_storage &addr);
    int32_t getFreeNeighbor(struct sockaddr_storage &addr);
    int32_t getRandomNeighbor(struct sockaddr_storage &addr);
    std::vector<struct sockaddr_storage> getNeighborAdresses(int32_t count);
    int32_t removeNeighbor(const struct sockaddr_storage &addr);
    void removeDirty();
    void addNewNeighbor(const struct sockaddr_storage &addr);
    void setInterval(const struct sockaddr_storage &addr, int32_t i);
    void updateQuality(const struct sockaddr_storage &addr, int32_t q);
    void setNeighborFree(const struct sockaddr_storage &addr, bool free);
    void applyToNeighbors(
            std::function<void (std::pair<std::string, NeighborInfo *>)> func);
    NeighborStorage();
};

#endif // NEIGHBORSTORAGE_H
