#ifndef NETWORKHANDLER_H
#define NETWORKHANDLER_H
#include "defines.h"

#include <thread>
#include <mutex>
#include <vector>
#include <map>
#include <curses.h>
#include <sys/socket.h>

class NetworkHandler {
    std::unique_lock<std::mutex> closed_conns_lck;
    std::map<int, struct sockaddr_storage> connections;
    std::vector<NeighborInfo *> neighbors;
    std::vector<struct sockaddr_storage> potential_neighbors;
    int listening_sock;

public:
    std::mutex conns_mtx, n_mtx;
    NeighborInfo *lookupNeighbor(struct sockaddr_storage&);
    void spawnOutgoingConnection(struct sockaddr_storage addr, int fd,
                                 std::vector<CMDS> cmds, bool async, void *data);
    void spawnIncomingConnection(struct sockaddr_storage addr, int fd, bool async);
    int start_listening(int port);
    void addConnection(int fd, struct sockaddr_storage addr);
    void deleteConnection(int fd);
    void confirmNeighbor(struct sockaddr_storage addr);
    void obtainNeighbors(unsigned int count);
    void collectNeighbors();
    void contactSuperPeer();
    void askForAddresses(struct sockaddr_storage &addr);
    void applyToNeighbors(void (*op)(NeighborInfo *));
    int getNeighborCount();
    std::vector<NeighborInfo *> getNeighbors();
    void addNewNeighbor(bool potential, struct sockaddr_storage &addr);
    int removeNeighbor(struct sockaddr_storage addr);
    void setInterval(struct sockaddr_storage addr, int i);
    void decrIntervals();
    NeighborInfo *getNeighborInfo(struct sockaddr_storage &addr);
    void setNeighborFree(struct sockaddr_storage &addr, bool free);
    int getFreeNeighbor(NeighborInfo  *&ngh);
    int checkNeighbor(struct sockaddr_storage addr);
};

#endif // NETWORKHANDLER_H