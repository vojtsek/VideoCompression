#ifndef NETWORKHANDLER_H
#define NETWORKHANDLER_H
#include "headers/defines.h"
#include "structures/NeighborInfo.h"

#include <thread>
#include <mutex>
#include <vector>
#include <map>
#include <curses.h>
#include <sys/socket.h>

struct NetworkHandler {
    neighbor_storageT neighbors;
    std::vector<NeighborInfo *> free_neighbors;
    std::vector<struct sockaddr_storage> potential_neighbors;
    int listening_sock;
    std::mutex conns_mtx, n_mtx;
    NeighborInfo *lookupNeighbor(struct sockaddr_storage&);
    void spawnOutgoingConnection(struct sockaddr_storage addr, int fd,
                                 std::vector<CMDS> cmds, bool async, void *data);
    void spawnIncomingConnection(struct sockaddr_storage addr, int fd, bool async);
    int start_listening(int port);
    void confirmNeighbor(struct sockaddr_storage addr);
    void obtainNeighbors();
    void collectNeighbors();
    void contactSuperPeer();
    void askForAddresses(struct sockaddr_storage &addr);
    int getNeighborCount();
    neighbor_storageT getNeighbors();
    void addNewNeighbor(bool potential, struct sockaddr_storage &addr);
    int removeNeighbor(struct sockaddr_storage addr);
    void setInterval(struct sockaddr_storage addr, int i);
    NeighborInfo *getNeighborInfo(struct sockaddr_storage &addr);
    void setNeighborFree(struct sockaddr_storage &addr, bool free);
    NeighborInfo *getFreeNeighbor();
    int checkNeighbor(struct sockaddr_storage addr);
};

#endif // NETWORKHANDLER_H