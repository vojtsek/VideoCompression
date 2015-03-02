#include "include_list.h"
#include "network_helper.h"
#include <netinet/in.h>

using namespace utilities;
bool CmdAskPeer::execute(int fd, struct sockaddr_storage &address, void *) {
    reportDebug("ASKPEER", 5);
    CMDS action = ASK_HOST;
    int rand_n;
    int size = handler->getNeighborCount();
    int count = (size < DATA->config.getValue("MIN_NEIGHBOR_COUNT")) ? size : DATA->config.getValue("MIN_NEIGHBOR_COUNT");
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
        rand_n = rand() % size;
        addr = handler->getNeighbors().at(rand_n)->address;
        if (sendSth(count, fd) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
        }
        if (sendSth(addr, fd) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
        }
    } else {
    if (!count) {
        return false;
    }
        if (sendSth(count, fd) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
        }
        for (auto neighbor : handler->getNeighbors()) {
            addr = neighbor->address;
            if (sendSth(addr, fd) == -1) {
                reportError("Error while communicating with peer." + MyAddr(address).get());
                return false;
            }
            if (!--count)
                break;
        }
    }
    return true;
}

bool CmdAskHost::execute(int fd, struct sockaddr_storage &address, void *) {
    reportDebug("ASKHOST", 5);
    struct sockaddr_storage addr;
    int count;
    if (recvSth(count, fd) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }
        for (int i = 0; i < count; ++i) {
            if (recvSth(addr, fd) == -1) {
                reportError("Error while communicating with peer." + MyAddr(address).get());
                return false;
            }
        addr.ss_family = AF_INET;
        if (((sockaddr_in *) &addr)->sin_port != htons(DATA->config.intValues.at("LISTENING_PORT")))
            handler->addNewNeighbor(true, addr);
    }
        reportDebug("ASKHOST END", 5);
        return true;
}

bool CmdConfirmPeer::execute(int fd, struct sockaddr_storage &address, void *) {
    reportDebug("CONFPEER " + MyAddr(address).get() , 5);
    CMDS action = CONFIRM_HOST;
    RESPONSE_T resp = state->working ? ACK_BUSY : ACK_FREE;
    struct sockaddr_storage addr;
    try {
        addr = getHostAddr(fd);
    } catch (std::exception *e) {
        reportDebug(e->what(), 1);
    }

    /* if (DATA->IPv4_ONLY)
        addr = addr2storage("127.0.0.1",
                                                DATA->config.intValues.at("LISTENING_PORT"), AF_INET);
    else
        addr = addr2storage("127.0.0.1",
                                                DATA->config.intValues.at("LISTENING_PORT"), AF_INET6);
    */
    if (sendCmd(fd, action) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    if (sendSth(resp, fd) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    if (sendSth(addr, fd) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    return true;
}

bool CmdConfirmHost::execute(int fd, struct sockaddr_storage &address, void *) {
    reportDebug("CONFHOST", 5);
    RESPONSE_T resp;
    struct sockaddr_storage addr;
    if (recvSth(resp, fd) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    if (resp != ACK_FREE && resp != ACK_BUSY) {
        reportDebug("Failed to confirm neighbor " + MyAddr(address).get(), 1);
        return false;
    }
    if (recvSth(addr, fd) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }
    handler->addNewNeighbor(false, addr);
    if (resp != ACK_FREE)
        handler->setNeighborFree(address, false);
    else
        handler->setNeighborFree(address, true);
    MyAddr mad(addr);
    reportDebug("Neighbor confirmed." + mad.get(), 4);
    return true;
}

bool CmdPingHost::execute(int fd, struct sockaddr_storage &address, void *) {
    reportDebug("PONG", 5);
    RESPONSE_T resp;
    if (recvSth(resp, fd) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    if (resp != ACK_FREE && resp != ACK_BUSY) {
        reportError("Failed to confirm neighbor " + MyAddr(address).get());
        handler->removeNeighbor(address);
        return false;
    }
    if (resp != ACK_FREE)
        handler->setNeighborFree(address, false);
    else
        handler->setNeighborFree(address, true);
    if (sendSth(DATA->config.intValues.at("LISTENING_PORT"), fd) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    return true;
}

bool CmdPingPeer::execute(int fd, struct sockaddr_storage &address, void *) {
    reportDebug("PING " + MyAddr(address).get() , 5);
    struct sockaddr_storage addr;
    CMDS action = PING_HOST;
    int peer_port;
    RESPONSE_T resp = state->working ? ACK_BUSY : ACK_FREE;
    if (sendCmd(fd, action) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    if (sendSth(resp, fd) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    if (recvSth(peer_port, fd) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }
    try {
        addr = getPeerAddr(fd);
    } catch (std::exception *e) {
        reportDebug(e->what(), 1);
    }

    ((struct sockaddr_in *) &addr)->sin_port = htons(peer_port);

    if (DATA->config.is_superpeer)
        handler->addNewNeighbor(false, addr);
    else
        handler->addNewNeighbor(true, addr);
    return true;
}
