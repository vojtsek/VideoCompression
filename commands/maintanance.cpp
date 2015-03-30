#include "headers/include_list.h"
#include <netinet/in.h>

bool CmdAskPeer::execute(int32_t fd, struct sockaddr_storage &address, void *) {
    reportDebug("ASKPEER", 5);
    CMDS action = ASK_HOST;
    int32_t size = DATA->neighbors.getNeighborCount();
    int32_t count = (size < DATA->config.getIntValue("MAX_NEIGHBOR_COUNT")) ? size : DATA->config.getIntValue("MAX_NEIGHBOR_COUNT");
    struct sockaddr_storage addr;
    if (sendCmd(fd, action) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }
    if (DATA->config.is_superpeer) {
        count = 1;
        if (!size) {
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
        if (sendStruct(fd, addr) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
        }
    } else {
    if (!count) {
        return false;
    }
        if (sendInt32(fd, count) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
        }
        for (auto &address: DATA->neighbors.getNeighborAdresses(count)) {
            if (sendStruct(fd, address) == -1) {
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
    if (receiveInt32(fd, count) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }
        for (int32_t i = 0; i < count; ++i) {
            if (receiveStruct(fd, addr) == -1) {
                reportError("Error while communicating with peer." + MyAddr(address).get());
                return false;
            }
        addr.ss_family = AF_INET6;
        //TODO: compare storages
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
    RESPONSE_T resp = ACK_FREE;
    int32_t can_accept = std::atomic_load(&DATA->state.can_accept);
    int32_t port, sock;
    if ((can_accept <= 0) || (DATA->state.working)) {
        resp = ACK_BUSY;
    }

    struct sockaddr_storage addr;
    if (sendCmd(fd, action) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

    if (receiveInt32(fd, port) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }
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
    if (sendStruct(fd, addr) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    return true;
}

bool CmdConfirmHost::execute(int32_t fd, struct sockaddr_storage &address, void *) {
    reportDebug("CONFHOST", 5);
    RESPONSE_T resp;
    struct sockaddr_storage addr;

    if (sendInt32(fd, DATA->config.intValues.at("LISTENING_PORT")) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

    if (receiveResponse(fd, resp) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    if (resp != ACK_FREE && resp != ACK_BUSY) {
        reportDebug("Failed to confirm neighbor " + MyAddr(address).get(), 1);
        return false;
    }
    if (receiveStruct(fd, addr) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }
    handler->addNewNeighbor(false, addr);
    if (resp != ACK_FREE)
        DATA->neighbors.setNeighborFree(address, false);
    else
        DATA->neighbors.setNeighborFree(address, true);
    MyAddr mad(addr);
    reportDebug("Neighbor confirmed." + mad.get(), 4);
    return true;
}

bool CmdPingHost::execute(int32_t fd, struct sockaddr_storage &address, void *) {
    reportDebug("PONG", 5);
    RESPONSE_T resp = ACK_FREE;
    int32_t can_accept = std::atomic_load(&DATA->state.can_accept);
    if ((can_accept <= 0) || (DATA->state.working)) {
        resp = ACK_BUSY;
    }
    if (sendResponse(fd, resp) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    if (receiveResponse(fd, resp) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    if (resp != ACK_FREE && resp != ACK_BUSY) {
        reportError("Failed to confirm neighbor " + MyAddr(address).get());
        DATA->neighbors.removeNeighbor(address);
        return false;
    }
    if (resp != ACK_FREE) {
        DATA->neighbors.setNeighborFree(address, false);
    } else {
        DATA->neighbors.setNeighborFree(address, true);
    }
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
    RESPONSE_T resp = ACK_FREE, neighbor_state;
    int32_t can_accept = std::atomic_load(&DATA->state.can_accept);

    if ((can_accept <= 0) || (DATA->state.working)) {
        resp = ACK_BUSY;
    }

    if (sendCmd(fd, action) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

    if (receiveResponse(fd, neighbor_state) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }

    if (sendResponse(fd, resp) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

    if (receiveInt32(fd, peer_port) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }

    networkHelper::changeAddressPort(address, peer_port);

    // in case that I am working, I want more neighbors
    if (DATA->config.is_superpeer ||
            DATA->state.working) {
        handler->addNewNeighbor(false, address);
    } else {
        handler->addNewNeighbor(true, address);
    }

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

    if (receiveInt32(fd, port) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }

    ((struct sockaddr_in6 *) &address)->sin6_port = htons(port);
    DATA->neighbors.removeNeighbor(address);
    if (sendResponse(fd, resp) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    return true;
}

bool CmdGoodbyeHost::execute(int32_t fd, sockaddr_storage &address, void *) {
    RESPONSE_T resp;

    if (sendInt32(fd, DATA->config.intValues.at(
                      "LISTENING_PORT")) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }

    if (receiveResponse(fd, resp) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    reportDebug("I have informed neighbor: " + MyAddr(address).get(), 2);
    return true;

}

void CmdSayGoodbye::execute() {
    int32_t sock;
    for (auto &address : DATA->neighbors.getNeighborAdresses(
             DATA->neighbors.getNeighborCount())) {
        sock = handler->checkNeighbor(address);
        reportError("Saying goodbye to: "+MyAddr(address).get());
        handler->spawnOutgoingConnection(address,
                                         sock, { GOODBYE_PEER }, false, nullptr);
    }
    struct sockaddr_storage address;
    if (DATA->config.IPv4_ONLY)
        address = networkHelper::addr2storage(DATA->config.superpeer_addr.c_str(), DATA->config.intValues.at("SUPERPEER_PORT"), AF_INET);
    else
        address = networkHelper::addr2storage(DATA->config.superpeer_addr.c_str(), DATA->config.intValues.at("SUPERPEER_PORT"), AF_INET6);
   sock = handler->checkNeighbor(address);
    handler->spawnOutgoingConnection(address,
                                     sock, { GOODBYE_PEER }, false, nullptr);
}
