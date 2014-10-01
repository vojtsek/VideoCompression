#include "networking.h"
#include "common.h"
#include "commands.h"
#include "defines.h"

#include <mutex>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <err.h>
#include <string.h>

using namespace std;
using namespace common;

int sendCmd(int fd, CMDS cmd) {
    bool response;
    refresh();
    if (sendSth(cmd, fd) == -1)
        return (-1);
    if (recvSth(response, fd) == -1)
        return (-1);
    if (!response) {
        reportDebug("The command was not accepted.", 1);
        return (-1);
    }
    return (0);
}

void handlePeer(int fd, NeighborInfo *peer, int idx, NetworkHandle *handler,
                CONN_T conn, CMDS cmd) {
    CMDS action;
    ssize_t r;
    bool response;
    if (conn == OUTGOING) {
            if ((sendCmd(fd, cmd)) == -1) {
                reportError("Failed to process the action.");
                handler->closeConnection(conn, idx);
            }
    }

    while ((r = read(fd, &action, sizeof (action))) > 0) {
        response = true;
       try {
           if (Data::getInstance()->cmds.find(action) == Data::getInstance()->cmds.end()) {
               response = false;
               write(fd, &response, sizeof (response));
               return;
           }
           NetworkCommand *cmd = dynamic_cast<NetworkCommand *>(
                       Data::getInstance()->cmds.at(action));
           if (cmd == nullptr) {
               response = false;
               write(fd, &response, sizeof (response));
               throw 1;
           }
           write(fd, &response, sizeof (response));
           cmd->fd = fd;
           cmd->execute();
       } catch (...) {
           reportDebug("Error while communicating: Unrecognized command.", 1);
       }
    }
    //handler->closeConnection(conn, idx);
}

void NetworkHandle::closeConnection(CONN_T conn, int idx) {
    conns_mtx.lock();
    try {
        if (conn == INCOMING)
            connections_in.erase(connections_in.begin() + idx);
        else
            connections_out.erase(connections_out.begin() + idx);
    } catch (...) {
        //TODO: handle!
    }
    conns_mtx.unlock();
    if (conn == INCOMING)
    reportDebug("Connection closed.", 3);
}

void NetworkHandle::spawnConnection(CONN_T conn, NeighborInfo *neighbor,
                                    int fd, CMDS cmd, bool async) {
    vector<Connection>::iterator c;
    conns_mtx.lock();
    if (conn == INCOMING) {
        connections_in.emplace_back(fd, neighbor, connections_in.size(), this, conn, cmd, async);
        c = connections_in.end() - 1;
    } else {
        connections_out.emplace_back(fd, neighbor, connections_out.size(), this, conn, cmd, async);
        c = connections_out.end() - 1;
    }
    if (async)
        c->handling_thread.detach();
    conns_mtx.unlock();
    if (!async)
        c->handling_thread.join();
}

int NetworkHandle::start_listening(int port) {
    int sock, accepted, ip6_only = 0, reuse = 1;
    struct sockaddr_in6 in6;
    socklen_t in6_size = sizeof (in6), psize = sizeof (struct sockaddr_storage);
    struct sockaddr_storage peer_addr;
    bzero(&in6, sizeof (in6));
    bzero(&peer_addr, in6_size);
    bzero(&in6.sin6_addr.s6_addr, 16);
    in6.sin6_family = AF_INET6;
    in6.sin6_port = htons(port);
    in6.sin6_addr.s6_addr[0] = 0;

    if ((sock = socket(AF_INET6, SOCK_STREAM, 6)) == -1) {
        reportDebug("Failed to create listening socket." + string(strerror(errno)), 1);
        return (-1);
    }

    if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY,
                    &ip6_only, sizeof(ip6_only)) == -1) {
        reportDebug("Failed to set option to listening socket." + string(strerror(errno)), 1);
        return (-1);
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
            &reuse, sizeof (reuse)) == -1) {
        reportDebug("Failed to set option to listening socket." + string(strerror(errno)), 1);
        return (-1);
   }

    if (bind(sock, (struct sockaddr *) &in6, in6_size) == -1) {
        reportDebug("Failed to bind the listening socket." + string(strerror(errno)), 1);
        return (-1);
    }

    if (listen(sock, SOMAXCONN) == -1) {
        reportDebug("Failed to start listen on the socket." + string(strerror(errno)), 1);
        return (-1);
    }

    for (;;) {
        if ((accepted = accept(sock, (struct sockaddr *) &peer_addr, &psize)) == -1) {
            reportDebug("Failed to accept connection." + string(strerror(errno)), 1);
            continue;
        }
        spawnConnection(INCOMING, NULL, accepted, DEFCMD, true);
    }
}

void NetworkHandle::confirmNeighbor(struct sockaddr_storage addr) {
    int sock;
    NetworkCommand cmd(NULL, 0, NULL);
    reportDebug("CONFIRMING " + addr2str(addr), 4);
    if ((sock = cmd.connectPeer(&addr)) == -1) {
        reportDebug("Failed to establish connection.", 2);
        return;
    }
    spawnConnection(OUTGOING, NULL, sock, CONFIRM_PEER, true);
}

void NetworkHandle::confirmNeighbors(int count) {
    // if nobody to ask to, try to earn some more addresses
    if (potential_neighbors.size() < count) {
        reportDebug("Need to collect more potential neighbors", 3);
        collectNeighbors(count);
    }
    struct sockaddr_storage addr;
    n_mtx.lock();
    for (int i = 0; i < count; ++i) {
        if (!potential_neighbors.size())
            break;
        addr = *(potential_neighbors.end() - 1);
        confirmNeighbor(addr);
        potential_neighbors.pop_back();
    }
    n_mtx.unlock();
}

void NetworkHandle::collectNeighbors(unsigned int desired) {
    struct sockaddr_storage addr;
    if (neighbors.size()) {
        n_mtx.lock();
        std::vector<NeighborInfo> nghrs = neighbors;
        n_mtx.unlock();
        reportDebug("Trying neighbors.", 4);
        for (NeighborInfo &n : nghrs) {
            addr = n.address;
            reportDebug("Trying neighbor. " + addr2str(addr), 4);
            askForAddresses(addr, MIN_NEIGHBOR_COUNT);
            if (potential_neighbors.size() >= desired)
                break;
        }
    }
    if ((potential_neighbors.size() < desired) && (potential_neighbors.size())) {
        n_mtx.lock();
        std::vector<struct sockaddr_storage> nghrs = potential_neighbors;
        n_mtx.unlock();
        reportDebug("Trying first potential neighbor.", 4);
        for (struct sockaddr_storage &addr : nghrs) {
            askForAddresses(addr, MIN_NEIGHBOR_COUNT);
            if (potential_neighbors.size() >= desired)
                break;
        }
    }
    if (potential_neighbors.size() < desired){
        reportDebug("Trying superpeer.", 4);
        addr = addr2storage(SUPERPEER_ADDR, SUPERPEER_PORT, AF_INET);
        askForAddresses(addr, MIN_NEIGHBOR_COUNT);
    }
}

void NetworkHandle::askForAddresses(struct sockaddr_storage &addr, int count) {
    int sock;
    NetworkCommand cmd(NULL, 0, NULL);
    if ((sock = cmd.connectPeer(&addr)) == -1) {
        reportDebug("Failed to establish connection.", 1);
        return;
    }
    spawnConnection(OUTGOING, NULL, sock, ASK_PEER, false);
}

void NetworkHandle::addNewNeighbor(bool potential, struct sockaddr_storage &addr) {
    n_mtx.lock();
    if (potential)
        potential_neighbors.push_back(addr);
    else {
        if (!addrIn(addr, neighbors)) {
            neighbors.emplace_back(addr);
            reportDebug("Neighbor added.", 3);
        } else
            reportDebug("Already known neighbor.", 3);
    }
    n_mtx.unlock();
}
