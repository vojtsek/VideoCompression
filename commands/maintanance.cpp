#include "headers/include_list.h"
#include <netinet/in.h>

bool CmdAskPeer::execute(int64_t fd, struct sockaddr_storage &address, void *) {
    reportDebug("ASKPEER", 5);
    CMDS action = ASK_HOST;
    int64_t neighborCount = DATA->neighbors.getNeighborCount();
    // count to be sent, should be sufficient if possible
    int64_t count = (neighborCount < DATA->config.getIntValue(
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

        if (DATA->neighbors.getRandomNeighbor(addr) == 0) {
            reportDebug("No neighbors!", 3);
            return false;
        }

        if (sendInt64(fd, count) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
        }

        if (sendAdrressStruct(fd, addr) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
            reportSuccess(MyAddr(addr).get());
        }
    } else {
        if (sendInt64(fd, count) == -1) {
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

bool CmdAskHost::execute(int64_t fd, struct sockaddr_storage &address, void *) {
    reportDebug("ASKHOST", 5);
    struct sockaddr_storage addr;
    int64_t count;
    // get count of addresses to be received
    if (receiveInt64(fd, count) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }
    // receive those addresses
        for (int64_t i = 0; i < count; ++i) {
            if (receiveAddressStruct(fd, addr) == -1) {
                reportError("Error while communicating with peer." + MyAddr(address).get());
                return false;
            }
        addr.ss_family = AF_INET6;
        // check whether it is not self
        if (!networkHelper::cmpStorages(
                    DATA->config.my_IP.getAddress(), addr)) {
            handler->addNewNeighbor(true, addr);
        }
    }
        reportDebug("ASKHOST END", 5);
        return true;
}

bool CmdConfirmPeer::execute(int64_t fd, struct sockaddr_storage &address, void *) {
    reportDebug("CONFPEER " + MyAddr(address).get() , 5);
    CMDS action = CONFIRM_HOST;
    // is it able to work?
    RESPONSE_T resp = networkHelper::isFree() ? ACK_FREE : ACK_BUSY;

    if (sendCmd(fd, action) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

    if (sendResponse(fd, resp) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    // send my address & port
    if (sendInt64(fd,
                  DATA->config.getIntValue("LISTENING_PORT")) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    return true;
}

bool CmdConfirmHost::execute(int64_t fd,
                             struct sockaddr_storage &address, void *) {
    reportDebug("CONFHOST", 5);
    RESPONSE_T resp;
    int64_t port;

    if (receiveResponse(fd, resp) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

    // not valid response
    if (resp != ACK_FREE && resp != ACK_BUSY) {
        reportDebug("Failed to confirm neighbor " + MyAddr(address).get(), 1);
        return false;
    }

    // receive peer's listening port
    if (receiveInt64(fd, port) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }
    networkHelper::changeAddressPort(address, port);

    // add new neighbor
    handler->addNewNeighbor(false, address);
    if (resp != ACK_FREE) {
        DATA->neighbors.setNeighborFree(address, false);
    } else {
        DATA->neighbors.setNeighborFree(address, true);
    }
    MyAddr mad(address);
    reportDebug("Neighbor confirmed." + mad.get(), 4);
    return true;
}

bool CmdPingHost::execute(int64_t fd, struct sockaddr_storage &address, void *) {
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
    if (sendInt64(fd,
                  DATA->config.intValues.at("LISTENING_PORT")) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    return true;
}

bool CmdPingPeer::execute(int64_t fd, struct sockaddr_storage &address, void *) {
    reportDebug("PING " + MyAddr(address).get() , 5);
    CMDS action = PING_HOST;
    int64_t peer_port;
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
    if (receiveInt64(fd, peer_port) == -1) {
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

bool CmdGoodbyePeer::execute(int64_t fd, struct sockaddr_storage &address, void *) {
    reportDebug("GOODBYE PEER", 5);
    CMDS action = GOODBYE_HOST;
    RESPONSE_T resp = ACK_FREE;
    int64_t port;
    if (sendCmd(fd, action) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

    // receives port of the node being disconnected
    if (receiveInt64(fd, port) == -1) {
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

bool CmdGoodbyeHost::execute(int64_t fd, sockaddr_storage &address, void *) {
    RESPONSE_T resp;

    // sends port
    if (sendInt64(fd,
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
    int64_t sock;
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
    if (networkHelper::getSuperPeerAddr(address) == -1) {
        return false;
    }

   sock = handler->checkNeighbor(address);
    handler->spawnOutgoingConnection(address,
                                     sock, { GOODBYE_PEER }, true, nullptr);
}
