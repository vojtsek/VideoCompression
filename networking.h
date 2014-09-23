#include "defines.h"

#include <thread>
#include <mutex>
#include <vector>
#include <sys/socket.h>
#ifndef NETWORKING_H
#define NETWORKING_H

class NetworkHandle;

int sendCmd(int fd, CMDS cmd);
void handlePeer(int fd, NeighborInfo *peer,
                int idx, NetworkHandle *handler, CONN_T conn, CMDS cmd);

struct Connection {
    NeighborInfo *peer;
    std::thread handling_thread;

    Connection(int fd, NeighborInfo *neighbor,
               int idx, NetworkHandle *handler, CONN_T conn, CMDS cmd):
        handling_thread(handlePeer, fd, neighbor,
                       idx, handler, conn, cmd) {
        peer = neighbor;
        handling_thread.detach();
    }
};

class NetworkHandle {
    std::mutex conns_mtx;
    std::unique_lock<std::mutex> closed_conns_lck;
    std::vector<Connection> connections_out, connections_in;
    std::vector<NeighborInfo> neighbors, potential_neighbors;
    int listening_sock;

public:
    NeighborInfo *lookupNeighbor(struct sockaddr_storage&);
    void spawnConnection(CONN_T conn, NeighborInfo *neighbor, int fd, CMDS cmd);
    int start_listening();
    void closeConnection(CONN_T conn, int idx);
    void confirmNeighbor(struct sockaddr_storage &addr);
    void addNeighbor();
    int getNeighborCount() { return neighbors.size(); }
    std::vector<NeighborInfo> getNeighbors() { return neighbors; }
    void addNewNeighbor(bool potential, struct sockaddr_storage &addr);
};

#endif // NETWORKING_H
