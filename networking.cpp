#include "networking.h"
#include "common.h"
#include "commands.h"
#include "handle_IO.h"
#include "network_helper.h"
#include "defines.h"

#include <mutex>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <sys/fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <err.h>
#include <string.h>

using namespace std;
using namespace common;

int sendCmd(int fd, CMDS cmd) {
    bool response;
    if (sendSth(cmd, fd) == -1) {
        reportError("Error send CMD");
        return (-1);
    }
    if (recvSth(response, fd) == -1) {
        reportError("ERROR conf CMD");
        return (-1);
    }
    if (!response) {
        reportDebug("The command was not accepted.", 1);
        return (-1);
    }
    return (0);
}

void NetworkHandle::addConnection(int fd, struct sockaddr_storage addr) {
    conns_mtx.lock();
    connections.emplace(fd, addr);
    conns_mtx.unlock();
}

void NetworkHandle::deleteConnection(int fd) {
    conns_mtx.lock();
    try {
        connections.erase(connections.find(fd));
    } catch (std::out_of_range) {}
    conns_mtx.unlock();
}

void NetworkHandle::spawnOutgoingConnection(struct sockaddr_storage addri,
                                    int fdi, vector<CMDS> cmds, bool async) {
    std::thread handling_thread([=]() {
        CMDS action;
        int fd = fdi;
        struct sockaddr_storage addr = addri;
        bool response;
        for (auto cmd : cmds) {
            reportDebug("OUT", 1);
            if ((sendCmd(fd, cmd)) == -1) {
                reportError("Failed to process the action.");
                break;
            }

            if (recvSth(action, fd) == -1) {
                reportError("Failed to process the action.");
                break;
            }
            response = true;
            if (action == TERM){
                sendSth(response, fd);
                break;
            }
           try {
               if (DATA->net_cmds.find(action) == DATA->net_cmds.end()) {
                   response = false;
                   if (sendSth(response, fd) == -1) {
                   reportDebug("Error while processing cmd.", 1);
                   }
                   break;
               }
               NetworkCommand *command = DATA->net_cmds.at(action);
               if (command == nullptr) {
                   response = false;
                   if (sendSth(response, fd) == -1) {
                       reportDebug("Error while processing cmd.", 1);
                   }
                   throw exception();
               }
               if (sendSth(response, fd) == -1) {
                   reportDebug("Error while processing cmd.", 1);
                   break;
               }
               if (!command->execute(fd, addr))
                   throw exception();
            }catch (exception e) {
               reportDebug("Error while communicating: Unrecognized command." + string(e.what()), 1);
               break;
           }
        }
        close(fd);
    });
    if (async) {
        handling_thread.detach();
        return;
    }
    handling_thread.join();
}

void NetworkHandle::spawnIncomingConnection(struct sockaddr_storage addri,
                                    int fdi, bool async) {
    std::thread handling_thread([=]() {
        CMDS action;
        ssize_t r;
        int fd = fdi;
        struct sockaddr_storage addr = addri;
        bool response;
        while ((r = read(fd, &action, sizeof (action))) == sizeof(action)) {
        response = true;
       try {
           if (DATA->net_cmds.find(action) == DATA->net_cmds.end()) {
               response = false;
               if (sendSth(response, fd) == -1) {
                   reportDebug("Error while processing cmd.", 1);
               }
               break;
           }
           NetworkCommand *cmd = DATA->net_cmds.at(action);
           if (cmd == nullptr) {
               response = false;
               if (sendSth(response, fd) == -1) {
               reportDebug("Error while processing cmd.", 1);
               }
               throw exception();
           }
           if (sendSth(response, fd) == -1) {
               reportDebug("Error while processing cmd.", 1);
               break;
           }
           if (!cmd->execute(fd, addr)) {
               throw exception();
           }
        }catch (exception e) {
           reportDebug("Error while communicating: Unrecognized command." + string(e.what()), 1);
           break;
           }
        }
        close(fd);
    });
    if (async) {
        handling_thread.detach();
        return;
    }
    handling_thread.join();
}

std::vector<NeighborInfo> NetworkHandle::getNeighbors() {
    n_mtx.lock();
    std::vector<NeighborInfo> n = neighbors;
    n_mtx.unlock();
    return n;
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
    struct linger so_linger;
    so_linger.l_onoff = 1;
    so_linger.l_linger = 10;
    if (setsockopt(sock, SOL_SOCKET, SO_LINGER,
            &so_linger, sizeof (so_linger)) == -1) {
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
        spawnIncomingConnection(peer_addr, accepted, true);
    }
}

int NetworkHandle::removeNeighbor(sockaddr_storage addr) {
    n_mtx.lock();
    for (auto it = neighbors.begin(); it < neighbors.end(); ++it) {
        if (cmpStorages((*it).address, addr)) {
            neighbors.erase(it);
            n_mtx.unlock();
            return 1;
            break;
        }
    }
    n_mtx.unlock();
    return 0;
}

void NetworkHandle::setInterval(sockaddr_storage addr, int i) {
    n_mtx.lock();
    for (auto it = neighbors.begin(); it < neighbors.end(); ++it) {
        if (cmpStorages((*it).address, addr)) {
            it->intervals = i;
            n_mtx.unlock();
            break;
        }
    }
    n_mtx.unlock();
    return;
}

void NetworkHandle::decrIntervals() {
    n_mtx.lock();
    for (auto it = neighbors.begin(); it < neighbors.end(); ++it) {
        it->intervals--;
    }
    n_mtx.unlock();
    return;
}

void NetworkHandle::contactSuperPeer() {
    struct sockaddr_storage addr;
    if (DATA->config.IPv4_ONLY)
        addr = addr2storage(DATA->config.superpeer_addr.c_str(), DATA->config.intValues.at("SUPERPEER_PORT"), AF_INET);
    else
        addr = addr2storage(DATA->config.superpeer_addr.c_str(), DATA->config.intValues.at("SUPERPEER_PORT"), AF_INET6);
    int sock =  checkNeighbor(addr);
    if (sock == -1) return;
    spawnOutgoingConnection(addr, sock, { PING_PEER }, false);
}

int NetworkHandle::checkNeighbor(struct sockaddr_storage addr) {
    int sock;
    CmdAsk cmd(nullptr, nullptr);
    if ((sock = cmd.connectPeer(&addr)) == -1) {
        reportError("Failed to check neighbor." +MyAddr(addr).get());
        removeNeighbor(addr);
        return -1;
    }
    setInterval(addr, CHECK_INTERVALS);
    return sock;
}

void NetworkHandle::confirmNeighbor(struct sockaddr_storage addr) {
    int sock =  checkNeighbor(addr);
    if (sock == -1) return;
    spawnOutgoingConnection(addr, sock, { CONFIRM_PEER }, false);
}

void NetworkHandle::obtainNeighbors(unsigned int count) {
    // if nobody to ask to, try to earn some more addresses
    if (!potential_neighbors.size()) {
        reportDebug("Need to collect more potential neighbors", 4);
        collectNeighbors();
    }
    struct sockaddr_storage addr;
    for (unsigned int i = 0; i < count; ++i) {
        if (!potential_neighbors.size()) {
            break;
        }
        n_mtx.lock();
        addr = *(potential_neighbors.end() - 1);
        potential_neighbors.pop_back();
        n_mtx.unlock();
        confirmNeighbor(addr);
    }
}

/* tries to collect more potential neighbors

*/
void NetworkHandle::collectNeighbors() {
    struct sockaddr_storage addr;
    if (neighbors.size()) {
        for (NeighborInfo &n : getNeighbors()) {
            addr = n.address;
            MyAddr mad(addr);
            reportDebug("Trying neighbor. " + mad.get(), 4);
            askForAddresses(addr);
            if (potential_neighbors.size())
                break;
        }
    }
    if ((!potential_neighbors.size()) && (!DATA->config.is_superpeer)) {
        reportDebug("Trying superpeer.", 4);
        if (DATA->config.IPv4_ONLY)
            addr = addr2storage(DATA->config.superpeer_addr.c_str(), DATA->config.intValues.at("SUPERPEER_PORT"), AF_INET);
        else
            addr = addr2storage(DATA->config.superpeer_addr.c_str(), DATA->config.intValues.at("SUPERPEER_PORT"), AF_INET6);
        askForAddresses(addr);
    }
}

void NetworkHandle::askForAddresses(struct sockaddr_storage &addr) {
    int sock;
    CmdAsk cmd(nullptr, nullptr);
    if ((sock = cmd.connectPeer(&addr)) == -1) {
        reportDebug("Failed to establish connection.", 1);
        return;
    }
    spawnOutgoingConnection(addr, sock, { PING_PEER, ASK_PEER }, false);
}

void NetworkHandle::addNewNeighbor(bool potential, struct sockaddr_storage &addr) {
    n_mtx.lock();
    if (potential)
        potential_neighbors.push_back(addr);
    else {
        if (!addrIn(addr, neighbors)) {
            neighbors.emplace_back(addr);
            MyAddr mad(addr);
            reportDebug("Neighbor added; " + mad.get(), 3);
            if (neighbors.size() == MIN_NEIGHBOR_COUNT)
                reportSuccess("Enough neighbors gained.");
        } else
            reportDebug("Already known neighbor.", 4);
    }
    n_mtx.unlock();
}
