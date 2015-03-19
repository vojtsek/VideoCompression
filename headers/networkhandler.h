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
    int listening_sock;
    void spawnOutgoingConnection(struct sockaddr_storage addr, int fd,
                                 std::vector<CMDS> cmds, bool async, void *data);
    void spawnIncomingConnection(struct sockaddr_storage addr, int fd, bool async);
    int start_listening(int port);
    int checkNeighbor(struct sockaddr_storage addr);
    int getPotentialNeighborsCount();
    void confirmNeighbor(struct sockaddr_storage addr);
    void obtainNeighbors();
    void collectNeighbors();
    void contactSuperPeer();
    void askForAddresses(struct sockaddr_storage &addr);
    void addNewNeighbor(bool potential, struct sockaddr_storage &addr);
};

#endif // NETWORKHANDLER_H
