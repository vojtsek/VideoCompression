#include "headers/networkhandler.h"
#include "headers/include_list.h"
#include "headers/commands.h"
#include "headers/handle_IO.h"
#include "headers/defines.h"

#include <mutex>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <algorithm>
#include <sys/fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <err.h>
#include <string.h>

using namespace std;
using namespace utilities;

void NetworkHandler::spawnOutgoingConnection(struct sockaddr_storage addri,
                                    int fdi, vector<CMDS> cmds, bool async, void *data) {
    std::thread handling_thread([=]() {
        CMDS action;
        int fd = fdi;
        struct sockaddr_storage addr = addri;
        bool response;
        for (auto cmd : cmds) {
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
                   throw new std::out_of_range("Unrecognized command.");
               }
               if (sendSth(response, fd) == -1) {
                   reportDebug("Error while processing cmd.", 1);
                   break;
               }
               if (!command->execute(fd, addr, data)) {
                   reportError(command->getName());
                   throw new std::runtime_error(
                                           "Command was not completed successfuly");
               }
            }catch (std::exception *e) {
               reportDebug("Error while communicating: " + std::string(e->what()), 1);
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

void NetworkHandler::spawnIncomingConnection(struct sockaddr_storage addri,
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
           if (!cmd->execute(fd, addr, nullptr)) {
               reportError(cmd->getName());
               throw exception();
           }
        }catch (exception e) {
           reportDebug("Error while communicating: Unrecognized command."
                       + std::string(e.what()), 1);
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

int NetworkHandler::getNeighborCount() {
    int size;
    n_mtx.lock();
    size = neighbors.size();
    n_mtx.unlock();
    return size;
}

neighbor_storageT NetworkHandler::getNeighbors() {
    n_mtx.lock();
    neighbor_storageT n = neighbors;
    n_mtx.unlock();
    return n;
}

int NetworkHandler::start_listening(int port) {
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
        reportDebug("Failed to create listening socket." + std::string(strerror(errno)), 1);
        return (-1);
    }

    if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY,
                    &ip6_only, sizeof(ip6_only)) == -1) {
        reportDebug("Failed to set option to listening socket." + std::string(strerror(errno)), 1);
        return (-1);
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
            &reuse, sizeof (reuse)) == -1) {
        reportDebug("Failed to set option to listening socket." + std::string(strerror(errno)), 1);
        return (-1);
   }
    struct linger so_linger;
    so_linger.l_onoff = 1;
    so_linger.l_linger = 10;
    if (setsockopt(sock, SOL_SOCKET, SO_LINGER,
            &so_linger, sizeof (so_linger)) == -1) {
        reportDebug("Failed to set option to listening socket." + std::string(strerror(errno)), 1);
        return (-1);
   }

    if (bind(sock, (struct sockaddr *) &in6, in6_size) == -1) {
        reportDebug("Failed to bind the listening socket." + std::string(strerror(errno)), 1);
        return (-1);
    }

    if (listen(sock, SOMAXCONN) == -1) {
        reportDebug("Failed to start listen on the socket." + std::string(strerror(errno)), 1);
        return (-1);
    }
    for (;;) {
        if ((accepted = accept(sock, (struct sockaddr *) &peer_addr, &psize)) == -1) {
            reportDebug("Failed to accept connection." + std::string(strerror(errno)), 1);
            continue;
        }
        spawnIncomingConnection(peer_addr, accepted, true);
    }
}

int NetworkHandler::removeNeighbor(sockaddr_storage addr) {
    if (getNeighborInfo(addr) == nullptr)
        return 0;
    reportError("Removing neighbor: " + MyAddr(addr).get());
    n_mtx.lock();
    free_neighbors.erase(
        std::remove_if(free_neighbors.begin(), free_neighbors.end(),
                   [&](NeighborInfo *ngh) {
        return cmpStorages(ngh->address, addr);
    }), free_neighbors.end());

    //todo: removing neighbor with algorithms?
    //todo: waiting_chunks
    for (auto it = neighbors.begin(); it != neighbors.end(); ++it) {
        if (cmpStorages(it->second->address, addr)) {
            DATA->periodic_listeners.erase(
                        it->second->getHash());
            DATA->chunks_to_send.removeIf(
                        [&](TransferInfo *ti) -> bool {
                return cmpStorages(ti->address, it->second->address);
            });
            delete it->second;
            neighbors.erase(it);
            n_mtx.unlock();
            return 1;
        }
    }
    n_mtx.unlock();
    return 0;
}

void NetworkHandler::setInterval(sockaddr_storage addr, int i) {
    n_mtx.lock();
    std::for_each (neighbors.begin(), neighbors.end(),
                   [&](std::pair<std::string, NeighborInfo *> entry) {
        if (cmpStorages(entry.second->address, addr)) {
            entry.second->intervals = i;
        }
    });
    n_mtx.unlock();
    return;
}

void NetworkHandler::contactSuperPeer() {
    struct sockaddr_storage addr;
    if (DATA->config.IPv4_ONLY)
        addr = addr2storage(DATA->config.superpeer_addr.c_str(), DATA->config.intValues.at("SUPERPEER_PORT"), AF_INET);
    else
        addr = addr2storage(DATA->config.superpeer_addr.c_str(), DATA->config.intValues.at("SUPERPEER_PORT"), AF_INET6);
    int sock =  checkNeighbor(addr);
    if (sock == -1) return;
    spawnOutgoingConnection(addr, sock, { PING_PEER }, false, nullptr);
}

int NetworkHandler::checkNeighbor(struct sockaddr_storage addr) {
    int sock;
    CmdAskPeer cmd(nullptr, nullptr);
    if ((sock = cmd.connectPeer(&addr)) == -1) {
        reportDebug("Failed to check neighbor."  + MyAddr(addr).get(), 3);
        removeNeighbor(addr);
        return -1;
    }
    setInterval(addr, DATA->config.getValue("CHECK_INTERVALS"));
    return sock;
}

void NetworkHandler::confirmNeighbor(struct sockaddr_storage addr) {
    int sock =  checkNeighbor(addr);
    if (sock == -1) return;
    spawnOutgoingConnection(addr, sock, { CONFIRM_PEER }, false, nullptr);
}

void NetworkHandler::obtainNeighbors() {
    // if nobody to ask to, try to earn some more addresses
    if (!potential_neighbors.size()) {
        reportDebug("Need to collect more potential neighbors", 4);
        collectNeighbors();
    }
    struct sockaddr_storage addr;
        if (!potential_neighbors.size()) {
            return;
        }
        n_mtx.lock();
        addr = *(potential_neighbors.end() - 1);
        potential_neighbors.pop_back();
        n_mtx.unlock();
        confirmNeighbor(addr);
}

/* tries to collect more potential neighbors

*/
void NetworkHandler::collectNeighbors() {
    struct sockaddr_storage addr;
    if (neighbors.size()) {
        for (auto n : getNeighbors()) {
            addr = n.second->address;
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

NeighborInfo *NetworkHandler::getNeighborInfo(sockaddr_storage &addr) {
    NeighborInfo *res = nullptr;
    n_mtx.lock();
    std::for_each (neighbors.begin(), neighbors.end(),
                   [&](std::pair<std::string, NeighborInfo *> entry) {
        if (cmpStorages(entry.second->address, addr)) {
            res = entry.second;
        }
    });
    n_mtx.unlock();
    return res;
}

void NetworkHandler::setNeighborFree(sockaddr_storage &addr, bool free) {
    NeighborInfo *ngh = getNeighborInfo(addr);
    if (ngh == nullptr) {
        return;
    }
    if (ngh->free == free) {
       return;
    }
    if (!free) {
        n_mtx.lock();
        free_neighbors.erase(
            std::remove_if (free_neighbors.begin(), free_neighbors.end(),
               [&](NeighborInfo *ngh) {
            return (cmpStorages(ngh->address, addr));
        }), free_neighbors.end());
        n_mtx.unlock();
    } else {
        reportSuccess("Neighbor is now free: " + MyAddr(addr).get());
        free_neighbors.push_back(ngh);
    }
    ngh->free = free;
}

NeighborInfo *NetworkHandler::getFreeNeighbor() {
    NeighborInfo *ngh = nullptr;
    n_mtx.lock();
    if (free_neighbors.empty()) {
        n_mtx.unlock();
        return ngh;
    }
    std::sort(free_neighbors.begin(), free_neighbors.end(),
              [&](NeighborInfo *n1, NeighborInfo *n2) {
        return (n1->quality < n2->quality);
    });
    ngh = *free_neighbors.begin();
        //free_neighbors.erase(free_neighbors.begin());
    n_mtx.unlock();
    return ngh;
}

void NetworkHandler::askForAddresses(struct sockaddr_storage &addr) {
    int sock;
    CmdAskPeer cmd(nullptr, nullptr);
    if ((sock = cmd.connectPeer(&addr)) == -1) {
        reportDebug("Failed to establish connection.", 1);
        return;
    }
    spawnOutgoingConnection(addr, sock, { PING_PEER, ASK_PEER }, false, nullptr);
}

void NetworkHandler::addNewNeighbor(bool potential, struct sockaddr_storage &addr) {
    n_mtx.lock();
    if (potential)
        potential_neighbors.push_back(addr);
    else {
        if (!addrIn(addr, neighbors)) {
            NeighborInfo *ngh = new NeighborInfo(addr);
            neighbors.insert(
                        std::make_pair(ngh->getHash(), ngh));
            DATA->periodic_listeners.insert(
                        std::make_pair(ngh->getHash(), ngh));
            free_neighbors.push_back(ngh);
            MyAddr mad(addr);
            reportDebug("Neighbor added; " + mad.get(), 3);
            if (neighbors.size() == (unsigned)
                    DATA->config.getValue("MAX_NEIGHBOR_COUNT")) {
                reportSuccess("Enough neighbors gained.");
            }
        } else
            reportDebug("Already known neighbor.", 4);
    }
    n_mtx.unlock();
}
