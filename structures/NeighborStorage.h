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
    int listening_sock;
    std::mutex n_mtx;
public:
    int getNeighborCount();
    NeighborInfo *getNeighborInfo(const struct sockaddr_storage &addr);
    NeighborInfo *getFreeNeighbor();
    struct sockaddr_storage getRandomNeighbor();
    std::vector<struct sockaddr_storage> getNeighborAdresses(int count);
    int removeNeighbor(const struct sockaddr_storage addr);
    void addNewNeighbor(const struct sockaddr_storage &addr);
    void setInterval(const struct sockaddr_storage &addr, int i);
    void updateQuality(const struct sockaddr_storage &addr, int q);
    void setNeighborFree(struct sockaddr_storage &addr, bool free);
    void applyToNeighbors(
            std::function<void (std::pair<std::string, NeighborInfo *>)> func);
    NeighborStorage();
};

#endif // NEIGHBORSTORAGE_H
