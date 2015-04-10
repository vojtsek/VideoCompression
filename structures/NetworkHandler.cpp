#include "structures/NetworkHandler.h"
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
                                    int64_t fdi, vector<CMDS> cmds, bool async, void *data) {
    std::thread handling_thread([=]() {
        CMDS action;
        int64_t fd = fdi;
        struct sockaddr_storage addr = addri;
        bool response;
				// traverse the commands to be spawned on the peer side
        for (auto cmd : cmds) {
						// sendCmd handles communication
            if ((sendCmd(fd, cmd)) == -1) {
                reportDebug("Failed to send command.", 2);
                break;
            }

						// command to be invoked
            if (recvSth(action, fd) == -1) {
                reportDebug("Failed to get response.", 2);
                break;
            }
            response = true;
						// end communication
            if (action == TERM) {
                sendSth(response, fd);
                break;
            }
						// try to invoke command
           try {
							// invalid action
              if (DATA->net_cmds.find(action) == DATA->net_cmds.end()) {
                  response = false;
                  if (sendSth(response, fd) == -1) {
                      reportDebug("Error while processing cmd.", 1);
                  }
                  break;
              }
							// get the command's structure
              NetworkCommand *command = DATA->net_cmds.at(action);
							// invalid request
              if (command == nullptr) {
                  response = false;
                  if (sendSth(response, fd) == -1) {
                      reportDebug("Error while processing cmd.", 1);
                  }
                  throw new std::out_of_range("Unrecognized command.");
               }

							 // send response to determince success
               if (sendSth(response, fd) == -1) {
                   reportDebug("Error while processing cmd.", 1);
                   break;
               }
							 // execute the command
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

		// don't wait for completion, asynchronous process
    if (async) {
        handling_thread.detach();
        return;
    }

		// wait for completion
    handling_thread.join();
}

void NetworkHandler::spawnIncomingConnection(struct sockaddr_storage addri,
                                    int64_t fdi, bool async) {
    std::thread handling_thread([=]() {
        CMDS action;
        ssize_t r;
        int64_t fd = fdi;
        struct sockaddr_storage addr = addri;
        bool response;
        // read action to process
        while ((r = read(fd, &action, sizeof (action)))
               == sizeof(action)) {
        response = true;
        // try to process the command
       try {
           if (DATA->net_cmds.find(action) == DATA->net_cmds.end()) {
               response = false;
               if (sendSth(response, fd) == -1) {
                   reportDebug("Error while processing cmd.", 1);
               }
               break;
           }
           NetworkCommand *cmd = DATA->net_cmds.at(action);
           reportDebug(cmd->getName(), 3);
           // invalid command
           if (cmd == nullptr) {
               response = false;
               if (sendSth(response, fd) == -1) {
               reportDebug("Error while processing cmd.", 1);
               }
               throw exception();
           }

           // inform about response
           if (sendSth(response, fd) == -1) {
               reportDebug("Error while processing cmd.", 1);
               break;
           }

           // execute it
           if (!cmd->execute(fd, addr, nullptr)) {
               reportDebug("Failed to process: " + cmd->getName(), 3);
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

		// don't wait for completion, asynchronous process
    if (async) {
        handling_thread.detach();
        return;
    }

		// wait for completion
    handling_thread.join();
}

int64_t NetworkHandler::start_listening(int64_t port) {
    int64_t sock, accepted, ip6_only = 0, reuse = 1;
    struct sockaddr_in6 in6;
    // initialize the storage structure
    socklen_t in6_size = sizeof (in6), psize = sizeof (struct sockaddr_storage);
    struct sockaddr_storage peer_addr;
    bzero(&in6, sizeof (in6));
    bzero(&peer_addr, in6_size);
    bzero(&in6.sin6_addr.s6_addr, 16);
    in6.sin6_family = AF_INET6;
    in6.sin6_port = htons(port);
    in6.sin6_addr.s6_addr[0] = 0;

    // open the socket
    if ((sock = socket(AF_INET6, SOCK_STREAM, 6)) == -1) {
        reportDebug("Failed to create listening socket." + std::string(strerror(errno)), 1);
        return (-1);
    }

    // set options
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
    // set timeout
    struct linger so_linger;
    so_linger.l_onoff = 1;
    so_linger.l_linger = 10;
    if (setsockopt(sock, SOL_SOCKET, SO_LINGER,
            &so_linger, sizeof (so_linger)) == -1) {
        reportDebug("Failed to set option to listening socket." + std::string(strerror(errno)), 1);
        return -1;
   }

    // bind to desired address:port
    if (bind(sock, (struct sockaddr *) &in6, in6_size) == -1) {
        reportDebug("Failed to bind the listening socket." + std::string(strerror(errno)), 1);
        return -1;
    }

    // start listening
    if (listen(sock, SOMAXCONN) == -1) {
        reportDebug("Failed to start listen on the socket." + std::string(strerror(errno)), 1);
        return -1;
    }

    // waits for connection, handles it in separate thread
    for (;;) {
        if ((accepted = accept(sock, (struct sockaddr *) &peer_addr, &psize)) == -1) {
            reportDebug("Failed to accept connection." + std::string(strerror(errno)), 1);
            continue;
        }
        spawnIncomingConnection(peer_addr, accepted, true);
    }
}

void NetworkHandler::contactSuperPeer() {
    struct sockaddr_storage addr;
    if (networkHelper::getSuperPeerAddr(addr) == -1) {
        return;
    }
    // spawn the connection
    int64_t sock =  checkNeighbor(addr);
    if (sock == -1){
        return;
    }
    // ping the superpeer
    spawnOutgoingConnection(addr, sock, { PING_PEER }, false, nullptr);
}

int64_t NetworkHandler::checkNeighbor(struct sockaddr_storage addr) {
    int64_t sock;
    // to use the function connectPeer
    CmdAskPeer cmd(nullptr, nullptr);
    // establish the connection
    if ((sock = cmd.connectPeer(&addr)) == -1) {
        reportDebug("Failed to check neighbor."  + MyAddr(addr).get(), 3);
        return -1;
    }
    // update the interval
    DATA->neighbors.setInterval(addr,
                                DATA->config.getIntValue("CHECK_INTERVALS"));
    return sock;
}

int64_t NetworkHandler::getPotentialNeighborsCount() {
    potential_mtx.lock();
    int64_t count = potential_neighbors.size();
    potential_mtx.unlock();
    return count;
}

void NetworkHandler::gatherNeighbors(int64_t TTL,
        const struct sockaddr_storage &requester_addr,
        const struct sockaddr_storage &ngh_addr) {
    int64_t sock;
    // failed to contact
    if ((sock = checkNeighbor(ngh_addr)) == -1) {
        return;
    }

    // will be deleted when the command is processed
    // is essential to spawn the execution asynchronous,
    // so it will not block while spreading
    MyAddr *requester_maddr = new MyAddr(requester_addr);
    requester_maddr->TTL = TTL;

    // spread the address among neighbors
    spawnOutgoingConnection(ngh_addr, sock, { GATHER_PEER }, true,
                (void *) requester_maddr);
}

void NetworkHandler::confirmNeighbor(struct sockaddr_storage addr) {
    int64_t sock =  checkNeighbor(addr);
    if (sock == -1) {
        return;
    }
    spawnOutgoingConnection(addr, sock, { PING_PEER, CONFIRM_PEER }, false, nullptr);
}

void NetworkHandler::addNewNeighbor(
        bool potential,const struct sockaddr_storage &addr) {
        if (DATA->neighbors.contains(addr)) {
            reportDebug("Already confirmed neighbor", 3);
            return;
        }
    // handles potential
    if (potential) {
        potential_mtx.lock();
        potential_neighbors.push_back(addr);
        potential_mtx.unlock();
    } else {
        DATA->neighbors.addNewNeighbor(addr);
    }
}

void NetworkHandler::obtainNeighbors() {
    // if nobody to ask to, try to earn some more addresses
    if (!getPotentialNeighborsCount()) {
        reportDebug("Need to collect more potential neighbors", 4);
        collectNeighbors();
    }

    struct sockaddr_storage addr;
    // still no one to ask to
        if (!getPotentialNeighborsCount()) {
            return;
        }
        // pop one potential neighbor
        // try to confirm him
        potential_mtx.lock();
            addr = potential_neighbors.back();
            potential_neighbors.pop_back();
        potential_mtx.unlock();
        confirmNeighbor(addr);
}

void NetworkHandler::collectNeighbors() {
    struct sockaddr_storage address;
    // has some neighbors
    if (DATA->neighbors.getNeighborCount()) {
        // traverse neighbors until gains some addresses
        for (auto &addr : DATA->neighbors.getNeighborAdresses(
                DATA->neighbors.getNeighborCount())) {
            MyAddr mad(addr);
            reportDebug("Trying neighbor. " + mad.get(), 4);
            // tries to gain addresses
            // has side effects -> adds neighbors
            askForAddresses(addr);
            if (getPotentialNeighborsCount()) {
                break;
            }
        }
    }

    // still no potential neighbors -> contacts super peer
    if ((!getPotentialNeighborsCount()) &&
            (!DATA->config.is_superpeer)) {
        reportDebug("Trying superpeer.", 4);
        if (networkHelper::getSuperPeerAddr(address) == -1) {
            reportDebug("Failed to obtain superpeer address.", 2);
            return;
        }
        askForAddresses(address);
    }
}

void NetworkHandler::askForAddresses(const struct sockaddr_storage &addr) {
    int64_t sock;
    // tries connect
    if ((sock = checkNeighbor(addr)) == -1) {
        reportDebug("Failed to establish connection.", 1);
        return;
    }
    spawnOutgoingConnection(addr, sock, { PING_PEER, ASK_PEER }, false, nullptr);
}
