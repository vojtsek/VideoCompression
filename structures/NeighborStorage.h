#ifndef NEIGHBORSTORAGE_H
#define NEIGHBORSTORAGE_H
#include "headers/defines.h"
#include "structures/NeighborInfo.h"

class NeighborStorage
{
    neighbor_storageT neighbors;
    std::vector<NeighborInfo *> free_neighbors;
    std::vector<struct sockaddr_storage> potential_neighbors;
    int listening_sock;
    std::mutex conns_mtx, n_mtx;
    void confirmNeighbor(struct sockaddr_storage addr);
    void obtainNeighbors();
    void collectNeighbors();
    int getNeighborCount();
    void addNewNeighbor(struct sockaddr_storage &addr);
    int removeNeighbor(struct sockaddr_storage addr);
    void setInterval(struct sockaddr_storage addr, int i);
    NeighborInfo *getNeighborInfo(struct sockaddr_storage &addr);
    void setNeighborFree(struct sockaddr_storage &addr, bool free);
    NeighborInfo *getFreeNeighbor();
applyToNeighbors(std::function<bool (T *)> func);
//add to DATA

    NeighborStorage();
};

#endif // NEIGHBORSTORAGE_H
