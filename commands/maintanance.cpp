#include "headers/include_list.h"
#include <netinet/in.h>

bool CmdAskPeer::execute(int32_t fd, struct sockaddr_storage &address, void *) {
    reportDebug("ASKPEER", 5);
    CMDS action = ASK_HOST;
    int32_t neighborCount = DATA->neighbors.getNeighborCount();
    // count to be sent, should be sufficient if possible
    int32_t count = (neighborCount < DATA->config.getIntValue(
                         "MAX_NEIGHBOR_COUNT")) ?
                neighborCount : DATA->config.getIntValue("MAX_NEIGHBOR_COUNT");
    struct sockaddr_storage addr;
    if (sendCmd(fd, action) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }

    // superpeer sends just one -> no bothering and should be enough
    if (DATA->config.is_superpeer) {
        count = 1;
        if (!neighborCount) {
            reportError("No neighbors yet");
            return false;
        }

        if (DATA->neighbors.getRandomNeighbor(addr) == -1) {
            reportDebug("No neighbors!", 3);
            return false;
        }

        if (sendInt32(fd, count) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
        }

        if (sendAdrressStruct(fd, addr) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
        }
    } else {
        if (sendInt32(fd, count) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
        }

        // receive adresses which were advertised
        for (auto &address: DATA->neighbors.getNeighborAdresses(count)) {
            if (sendAdrressStruct(fd, address) == -1) {
                reportError("Error while communicating with peer." + MyAddr(address).get());
                return false;
            }
        }
    }
    return true;
}

bool CmdAskHost::execute(int32_t fd, struct sockaddr_storage &address, void *) {
    reportDebug("ASKHOST", 5);
    struct sockaddr_storage addr;
    int32_t count;
    // get count of addresses to be received
    if (receiveInt32(fd, count) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }
    // receive those addresses
        for (int32_t i = 0; i < count; ++i) {
            if (receiveStruct(fd, addr) == -1) {
                reportError("Error while communicating with peer." + MyAddr(address).get());
                return false;
            }
        addr.ss_family = AF_INET6;
        //TODO: compare storages
        // check whether it is not self
        if (((sockaddr_in6 *) &addr)->sin6_port != htons(
                    DATA->config.intValues.at("LISTENING_PORT")))
            handler->addNewNeighbor(true, addr);
    }
        reportDebug("ASKHOST END", 5);
        return true;
}

bool CmdConfirmPeer::execute(int32_t fd, struct sockaddr_storage &address, void *) {
    reportDebug("CONFPEER " + MyAddr(address).get() , 5);
    CMDS action = CONFIRM_HOST;
    int32_t port, sock;
    // is it able to work?
    RESPONSE_T resp = networkHelper::isFree() ? ACK_FREE : ACK_BUSY;

    struct sockaddr_storage addr;
    if (sendCmd(fd, action) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

    // asking peer's port
    if (receiveInt32(fd, port) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }

    // get address on which I am communicating
    networkHelper::changeAddressPort(address, port);
    if ((sock = handler->checkNeighbor(address)) == -1) {
        reportDebug("Error getting host address.", 2);
        return false;
    }
    if ((networkHelper::getHostAddr(addr, sock)) == -1) {
        reportDebug("Error getting host address.", 2);
        close(sock);
        return false;
    }
    close(sock);
    networkHelper::changeAddressPort(addr,
                                     DATA->config.getIntValue("LISTENING_PORT"));

    if (sendResponse(fd, resp) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    // send my address & port
    if (sendAdrressStruct(fd, addr) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    return true;
}

bool CmdConfirmHost::execute(int32_t fd, struct sockaddr_storage &address, void *) {
    reportDebug("CONFHOST", 5);
    RESPONSE_T resp;
    struct sockaddr_storage addr;

    // send my port so peer can get appropriate address
    if (sendInt32(fd,
                  DATA->config.intValues.at("LISTENING_PORT")) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

    if (receiveResponse(fd, resp) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

    // not valid response
    if (resp != ACK_FREE && resp != ACK_BUSY) {
        reportDebug("Failed to confirm neighbor " + MyAddr(address).get(), 1);
        return false;
    }

    // receive peer's address
    if (receiveStruct(fd, addr) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }
    // add new neighbor
    handler->addNewNeighbor(false, addr);
    if (resp != ACK_FREE) {
        DATA->neighbors.setNeighborFree(address, false);
    } else {
        DATA->neighbors.setNeighborFree(address, true);
    }
    MyAddr mad(addr);
    reportDebug("Neighbor confirmed." + mad.get(), 4);
    return true;
}

bool CmdPingHost::execute(int32_t fd, struct sockaddr_storage &address, void *) {
    reportDebug("PONG", 5);
    // is able to do some work?
    RESPONSE_T resp = networkHelper::isFree() ? ACK_FREE : ACK_BUSY;

    if (sendResponse(fd, resp) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

    // receive neighbor's state
    if (receiveResponse(fd, resp) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

    // invalid response
    if (resp != ACK_FREE && resp != ACK_BUSY) {
        reportError("Failed to confirm neighbor " + MyAddr(address).get());
        DATA->neighbors.removeNeighbor(address);
        return false;
    }

    // adjust neighbor state
    if (resp != ACK_FREE) {
        DATA->neighbors.setNeighborFree(address, false);
    } else {
        DATA->neighbors.setNeighborFree(address, true);
    }

    // send my port so peer can add me
    if (sendInt32(fd,
                  DATA->config.intValues.at("LISTENING_PORT")) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    return true;
}

bool CmdPingPeer::execute(int32_t fd, struct sockaddr_storage &address, void *) {
    reportDebug("PING " + MyAddr(address).get() , 5);
    CMDS action = PING_HOST;
    int32_t peer_port;
    // is able to do some work?
    RESPONSE_T resp = networkHelper::isFree() ? ACK_FREE : ACK_BUSY;
    RESPONSE_T neighbor_state;

    if (sendCmd(fd, action) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

    // gets neighbor's state
    if (receiveResponse(fd, neighbor_state) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }

    if (sendResponse(fd, resp) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

    // receives port so can add the neighbor
    if (receiveInt32(fd, peer_port) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }
    networkHelper::changeAddressPort(address, peer_port);

    // in case that I am working, I want more neighbors
    if (DATA->config.is_superpeer ||
            DATA->state.working) {
        handler->addNewNeighbor(false, address);
    } else { // add just as a potential neighbor otherwise
        handler->addNewNeighbor(true, address);
    }

    // adjust state
    if (neighbor_state == ACK_FREE) {
        DATA->neighbors.setNeighborFree(address, true);
    } else {
        DATA->neighbors.setNeighborFree(address, false);
    }
    return true;
}

bool CmdGoodbyePeer::execute(int32_t fd, struct sockaddr_storage &address, void *) {
    reportDebug("GOODBYE PEER", 5);
    CMDS action = GOODBYE_HOST;
    RESPONSE_T resp = ACK_FREE;
    int32_t port;
    if (sendCmd(fd, action) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

    // receives port of the node being disconnected
    if (receiveInt32(fd, port) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }

    networkHelper::changeAddressPort(address, port);
    // removes the neighbor and confirms
    DATA->neighbors.removeNeighbor(address);
    if (sendResponse(fd, resp) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    return true;
}

bool CmdGoodbyeHost::execute(int32_t fd, sockaddr_storage &address, void *) {
    RESPONSE_T resp;

    // sends port
    if (sendInt32(fd,
                  DATA->config.intValues.at("LISTENING_PORT")) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }

    // waits for confirmation
    if (receiveResponse(fd, resp) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    reportDebug("I have informed neighbor: " + MyAddr(address).get(), 2);
    return true;

}

void CmdSayGoodbye::execute() {
    int32_t sock;
    // traverse through all neighbors
    // sends info about disconnecting to everyone
    for (auto &address : DATA->neighbors.getNeighborAdresses(
             DATA->neighbors.getNeighborCount())) {
        sock = handler->checkNeighbor(address);
        reportError("Saying goodbye to: "+MyAddr(address).get());
        handler->spawnOutgoingConnection(address,
                                         sock, { GOODBYE_PEER }, true, nullptr);
    }

    // informs also the superpeer
    struct sockaddr_storage address;
    if (DATA->config.IPv4_ONLY)
        address = networkHelper::addrstr2storage(DATA->config.superpeer_addr.c_str(), DATA->config.intValues.at("SUPERPEER_PORT"), AF_INET);
    else
        address = networkHelper::addrstr2storage(DATA->config.superpeer_addr.c_str(), DATA->config.intValues.at("SUPERPEER_PORT"), AF_INET6);
   sock = handler->checkNeighbor(address);
    handler->spawnOutgoingConnection(address,
                                     sock, { GOODBYE_PEER }, true, nullptr);
}
