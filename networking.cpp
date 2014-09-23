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
    if (sendSth(cmd, fd) == -1)
        return (-1);
    if (recvSth(response, fd) == -1)
        return (-1);
    if (!response) {
        reportDebug("The command was not accepted.");
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
           NetworkCommand *cmd = dynamic_cast<NetworkCommand *>(Data::getInstance()->cmds.at(action));
           if (cmd == nullptr) {
               response = false;
               write(fd, &response, sizeof (response));
               throw 1;
           }
           write(fd, &response, sizeof (response));
           cmd->fd = fd;
           cmd->execute();
       } catch (...) {
           reportDebug("Error while communicating: Unrecognized command.");
       }
    }
    handler->closeConnection(conn, idx);
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
    reportDebug("Connection closed.");
}

void NetworkHandle::spawnConnection(CONN_T conn, NeighborInfo *neighbor,
                                    int fd, CMDS cmd) {
    conns_mtx.lock();
    if (conn == INCOMING)
        connections_in.emplace_back(fd, neighbor, connections_in.size(), this, conn, cmd);
    else
        connections_out.emplace_back(fd, neighbor, connections_out.size(), this, conn, cmd);
    conns_mtx.unlock();
}

int NetworkHandle::start_listening() {
    int sock, accepted, ip6_only = 0, reuse = 1;
    char errormsg[BUF_LENGTH];
    struct sockaddr_in6 in6;
    socklen_t in6_size = sizeof (in6), psize = sizeof (struct sockaddr_storage);
    struct sockaddr_storage peer_addr;
    bzero(&in6, sizeof (in6));
    bzero(&peer_addr, in6_size);
    bzero(&in6.sin6_addr.s6_addr, 16);
    in6.sin6_family = AF_INET6;
    in6.sin6_port = htons(LISTENING_PORT);
    in6.sin6_addr.s6_addr[0] = 0;

    if ((sock = socket(AF_INET6, SOCK_STREAM, 6)) == -1) {
        strerror_r(errno, errormsg, BUF_LENGTH);
        errno = 0;
        reportDebug("Failed to create listening socket." + string(errormsg));
        return (-1);
    }

    if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY,
                    &ip6_only, sizeof(ip6_only)) == -1) {
        strerror_r(errno, errormsg, BUF_LENGTH);
        errno = 0;
        reportDebug("Failed to set option to listening socket." + string(errormsg));
        return (-1);
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
            &reuse, sizeof (reuse)) == -1) {
        perror("option");
        strerror_r(errno, errormsg, BUF_LENGTH);
        errno = 0;
        reportDebug("Failed to set option to listening socket." + string(errormsg));
        return (-1);
   }

    if (bind(sock, (struct sockaddr *) &in6, in6_size) == -1) {
        strerror_r(errno, errormsg, BUF_LENGTH);
        errno = 0;
        reportDebug("Failed to bind the listening socket." + string(errormsg));
        return (-1);
    }

    if (listen(sock, SOMAXCONN) == -1) {
        strerror_r(errno, errormsg, BUF_LENGTH);
        errno = 0;
        reportDebug("Failed to start listen on the socket." + string(errormsg));
        return (-1);
    }

    for (;;) {
        if ((accepted = accept(sock, (struct sockaddr *) &peer_addr, &psize)) == -1) {
            strerror_r(errno, errormsg, BUF_LENGTH);
            errno = 0;
            reportDebug("Failed to accept connection." + string(errormsg));
            continue;
        }
        spawnConnection(INCOMING, NULL, accepted, DEFCMD);
    }
}

void NetworkHandle::confirmNeighbor(struct sockaddr_storage &addr) {
    int sock;
    NetworkCommand cmd(NULL, 0, NULL);
    if ((sock = cmd.connectPeer(&addr)) == -1) {
        reportDebug("Failed to establish connection.");
        return;
    }
    spawnConnection(OUTGOING, NULL, sock, CONFIRM_PEER);
}


void NetworkHandle::addNeighbor() {
    struct sockaddr_storage addr;
    if (!potential_neighbors.size()) {
        addr = addr2storage(SUPERPEER_ADDR, SUPERPEER_PORT, AF_INET);
        confirmNeighbor(addr);
    }

    if(!potential_neighbors.size()) {
        reportError("Failed to retrieve host info (unable to contact superpeer).");
        return;
    }
    addr = potential_neighbors.at(potential_neighbors.size() - 1).address;
    confirmNeighbor(addr);
}

void NetworkHandle::addNewNeighbor(bool potential, struct sockaddr_storage &addr) {
    if (potential)
        potential_neighbors.emplace_back(addr);
    else
        neighbors.emplace_back(addr);
}
