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
    if ((write(fd, &cmd, sizeof (cmd))) < 1) {
        reportError("Problem occured while sending the command.");
        return (-1);
    }
    if ((read(fd, &response, sizeof (response))) < 1) {
        reportError("Error reading the verifying response.");
        return (-1);
    }
    if (!response) {
        reportError("The command was not accepted.");
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
       if (Data::getInstance()->cmds.find(action) != Data::getInstance()->cmds.end())
           response = true;
       else
           response = false;
           write(fd, &response, sizeof (response));
       try {
           Data::getInstance()->cmds.at(action)->execute();
       } catch (...) {
           reportError("Error while communicating: Unrecognized command.");
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

void NetworkHandle::start_listening() {
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
        reportError("Failed to create listening socket." + string(errormsg));
        return;
    }

    if ((setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY,
                    &ip6_only, sizeof(ip6_only))) == -1) {
        strerror_r(errno, errormsg, BUF_LENGTH);
        errno = 0;
        reportError("Failed to set option to listening socket." + string(errormsg));
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
            &reuse, sizeof (reuse)) == -1) {
        strerror_r(errno, errormsg, BUF_LENGTH);
        errno = 0;
        reportError("Failed to set option to listening socket." + string(errormsg));
        return;
   }

    if ((bind(sock, (struct sockaddr *) &in6, in6_size) ) == -1) {
        strerror_r(errno, errormsg, BUF_LENGTH);
        errno = 0;
        reportError("Failed to bind the listening socket." + string(errormsg));
        return;
    }

    if ((listen(sock, SOMAXCONN)) == -1) {
        strerror_r(errno, errormsg, BUF_LENGTH);
        errno = 0;
        reportError("Failed to start listen on the socket." + string(errormsg));
        return;
    }

    for (;;) {
        if ((accepted = accept(sock, (struct sockaddr *) &peer_addr, &psize)) == -1) {
            strerror_r(errno, errormsg, BUF_LENGTH);
            errno = 0;
            reportError("Failed to accept connection." + string(errormsg));
            continue;
        }
        spawnConnection(INCOMING, NULL, accepted, DEFCMD);
    }
}
