#include "headers/include_list.h"
#include <netinet/in.h>

bool CmdAskPeer::execute(int64_t fd, struct sockaddr_storage &address, void *) {
    reportDebug("ASKPEER", 5);
    CMDS action = CMDS::ASK_HOST;
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
            count = 0;
        }

        if (DATA->neighbors.getRandomNeighbor(addr) == 0) {
            reportDebug("No neighbors!", 3);
            count = 0;
        }

        if (sendInt64(fd, count) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
        }
        if (!count) {
            return true;
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

    struct sockaddr_storage addr, communicating_addr;
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
        // fails if the node is not alive
        if (networkHelper::getMyAddress(
                    addr, communicating_addr, handler) == -1) {
            reportDebug(
                        "Failed obtaining communicating address", 2);
            return false;
        }
        // check whether it is not self
        // also wants only unknown neighbors
                handler->addNewNeighbor(true, addr);

    }
        reportDebug("ASKHOST END", 5);
        return true;
}

bool CmdConfirmPeer::execute(int64_t fd, struct sockaddr_storage &address, void *) {
    reportDebug("CONFPEER " + MyAddr(address).get() , 5);
    CMDS action = CMDS::CONFIRM_HOST;
    // is it able to work?
    RESPONSE_T resp = networkHelper::isFree() ? RESPONSE_T::ACK_FREE : RESPONSE_T::ACK_BUSY;

    // add to potential neighbors
    handler->addNewNeighbor(true, address);
    if (sendCmd(fd, action) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

    if (sendResponse(fd, resp) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    return true;
}

bool CmdConfirmHost::execute(int64_t fd,
                             struct sockaddr_storage &address, void *) {
    reportDebug("CONFHOST", 5);
    RESPONSE_T resp;

    if (receiveResponse(fd, resp) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

    // not valid response
    if (resp != RESPONSE_T::ACK_FREE && resp != RESPONSE_T::ACK_BUSY) {
        reportDebug("Failed to confirm neighbor " + MyAddr(address).get(), 1);
        return false;
    }

    // add new neighbor
    handler->addNewNeighbor(false, address);
    if (resp != RESPONSE_T::ACK_FREE) {
        DATA->neighbors.setNeighborFree(address, false);
    } else {
        DATA->neighbors.setNeighborFree(address, true);
    }
    MyAddr mad(address);
    reportDebug("Neighbor confirmed." + mad.get(), 4);
    return true;
}

bool CmdCancelPeer::execute(int64_t fd, struct sockaddr_storage &address, void *) {
    CMDS action = CMDS::CANCEL_HOST;
    std::string id;
    if (sendCmd(fd, action) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

    if ((id = receiveString(fd)).empty()) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    DATA->chunks_to_encode.removeIf(
          [&](TransferInfo *t){ return t->toString() == id;});
    reportDebug("Removing " + id, 2);
    return true;
}

bool CmdCancelHost::execute(int64_t fd,
                             struct sockaddr_storage &, void *data) {
  TransferInfo *ti = (TransferInfo *) data;
        if (sendString(fd, ti->toString())) {
                reportError(ti->name + ": Failed to send cancellation.");
                return false;
        }
        return true;
}
bool CmdPingHost::execute(int64_t fd, struct sockaddr_storage &address, void *) {
    // is able to do some work?
    RESPONSE_T resp = networkHelper::isFree() ? RESPONSE_T::ACK_FREE : RESPONSE_T::ACK_BUSY;

    // inform the neighbor about my state
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
    if (resp != RESPONSE_T::ACK_FREE && resp != RESPONSE_T::ACK_BUSY) {
        reportError("Failed to ping neighbor " + MyAddr(address).get());
        DATA->neighbors.removeNeighbor(address);
        handler->addNewNeighbor(true, address);
        return false;
    }

    // adjust neighbor's state
    if (resp != RESPONSE_T::ACK_FREE) {
        DATA->neighbors.setNeighborFree(address, false);
    } else {
        DATA->neighbors.setNeighborFree(address, true);
    }
    return true;
}

bool CmdPingPeer::execute(int64_t fd, struct sockaddr_storage &address, void *) {
    reportDebug("PING " + MyAddr(address).get() , 5);
    CMDS action = CMDS::PING_HOST;
    // is able to do some work?
    RESPONSE_T resp = networkHelper::isFree() ? RESPONSE_T::ACK_FREE : RESPONSE_T::ACK_BUSY;
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

        handler->addNewNeighbor(true, address);

    // adjust state
    if (neighbor_state == RESPONSE_T::ACK_FREE) {
        DATA->neighbors.setNeighborFree(address, true);
    } else {
        DATA->neighbors.setNeighborFree(address, false);
    }
    return true;
}

bool CmdGoodbyePeer::execute(int64_t fd, struct sockaddr_storage &address, void *) {
    reportDebug("GOODBYE PEER", 5);
    CMDS action = CMDS::TERM;
    RESPONSE_T resp = RESPONSE_T::ACK_FREE;
    if (sendCmd(fd, action) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

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
        if (sock == -1) {
           // continue;
        }
        reportError("Saying goodbye to: " + MyAddr(address).get());
        handler->spawnOutgoingConnection(address,
                                         sock, { CMDS::GOODBYE_PEER }, true, nullptr);
    }

    // informs also the superpeer
    struct sockaddr_storage address;
    if (networkHelper::getSuperPeerAddr(address) == -1) {
        return false;
    }

   sock = handler->checkNeighbor(address);
    handler->spawnOutgoingConnection(address,
                                     sock, { CMDS::GOODBYE_PEER }, true, nullptr);
}
