#include "defines.h"

#include <thread>
#include <mutex>
#include <vector>
#include <curses.h>
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
    bool async;

    Connection(int fd, NeighborInfo *neighbor,
               int idx, NetworkHandle *handler, CONN_T conn, CMDS cmd, bool as):
        handling_thread(handlePeer, fd, neighbor,
                       idx, handler, conn, cmd), async(as) {
        peer = neighbor;
    }
};

class NetworkHandle {
    std::unique_lock<std::mutex> closed_conns_lck;
    std::vector<Connection> connections_out, connections_in;
    std::vector<NeighborInfo> neighbors;
    std::vector<struct sockaddr_storage> potential_neighbors;
    int listening_sock;

public:
    std::mutex conns_mtx, n_mtx;
    NeighborInfo *lookupNeighbor(struct sockaddr_storage&);
    void spawnConnection(CONN_T conn, NeighborInfo *neighbor, int fd, CMDS cmd, bool async);
    int start_listening(int port);
    void closeConnection(CONN_T conn, int idx);
    void confirmNeighbor(struct sockaddr_storage addr);
    void confirmNeighbors(int count);
    void collectNeighbors(unsigned int desired);
    void askForAddresses(struct sockaddr_storage &addr, int count);
    int getNeighborCount() { return neighbors.size(); }
    std::vector<NeighborInfo> getNeighbors() { return neighbors; }
    void addNewNeighbor(bool potential, struct sockaddr_storage &addr);
};

#endif // NETWORKING_H
