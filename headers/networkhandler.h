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
    std::vector<struct sockaddr_storage> potential_neighbors;
    std::mutex potential_mtx;
    int32_t listening_sock;
    void spawnOutgoingConnection(struct sockaddr_storage addr, int32_t fd,
                                 std::vector<CMDS> cmds, bool async, void *data);
    void spawnIncomingConnection(struct sockaddr_storage addr, int32_t fd, bool async);
    int32_t start_listening(int32_t port);
    int32_t checkNeighbor(struct sockaddr_storage addr);
    int32_t getPotentialNeighborsCount();
    void confirmNeighbor(struct sockaddr_storage addr);
    void obtainNeighbors();
    void collectNeighbors();
    void contactSuperPeer();
    void askForAddresses(struct sockaddr_storage &addr);
    void addNewNeighbor(bool potential, struct sockaddr_storage &addr);
};

#endif // NETWORKHANDLER_H
