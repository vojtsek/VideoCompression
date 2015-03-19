#include "headers/include_list.h"
#include <netinet/in.h>

using namespace utilities;
bool CmdAskPeer::execute(int fd, struct sockaddr_storage &address, void *) {
    reportDebug("ASKPEER", 5);
    CMDS action = ASK_HOST;
    int size = DATA->neighbors.getNeighborCount();
    int count = (size < DATA->config.getValue("MAX_NEIGHBOR_COUNT")) ? size : DATA->config.getValue("MAX_NEIGHBOR_COUNT");
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
        addr = DATA->neighbors.getRandomNeighbor();
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
        for (auto &address: DATA->neighbors.getNeighborAdresses(count)) {
            if (sendSth(address, fd) == -1) {
                reportError("Error while communicating with peer." + MyAddr(address).get());
                return false;
            }
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
        addr.ss_family = AF_INET6;
        if (((sockaddr_in6 *) &addr)->sin6_port != htons(DATA->config.intValues.at("LISTENING_PORT")))
            handler->addNewNeighbor(true, 		addr);
    }
        reportDebug("ASKHOST END", 5);
        return true;
}

bool CmdConfirmPeer::execute(int fd, struct sockaddr_storage &address, void *) {
    reportDebug("CONFPEER " + MyAddr(address).get() , 5);
    CMDS action = CONFIRM_HOST;
    RESPONSE_T resp = ACK_FREE;
    int can_accept = std::atomic_load(&DATA->state.can_accept);
    int port, sock;
    if ((can_accept <= 0) || (DATA->state.working)) {
        resp = ACK_BUSY;
    }

    struct sockaddr_storage addr;
    if (sendCmd(fd, action) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

    if (recvSth(port, fd) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }
    ((struct sockaddr_in6 *) &address)->sin6_port = htons(port);
    if ((sock = handler->checkNeighbor(address)) == -1) {
        reportDebug("Error getting host address.", 2);
        return false;
    }
    if ((getHostAddr(addr, sock)) == -1) {
        reportDebug("Error getting host address.", 2);
        close(sock);
        return false;
    }
    close(sock);
    ((struct sockaddr_in6 *) &addr)->sin6_port = htons(DATA->config.getValue("LISTENING_PORT"));

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

    if (sendSth(DATA->config.intValues.at("LISTENING_PORT"), fd) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

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
        DATA->neighbors.setNeighborFree(address, false);
    else
        DATA->neighbors.setNeighborFree(address, true);
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
        DATA->neighbors.removeNeighbor(address);
        return false;
    }
    if (resp != ACK_FREE) {
        DATA->neighbors.setNeighborFree(address, false);
    } else {
        DATA->neighbors.setNeighborFree(address, true);
    }
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
    int peer_port, sock;
    RESPONSE_T resp = ACK_FREE;
    int can_accept = std::atomic_load(&DATA->state.can_accept);
    if ((can_accept <= 0) || (DATA->state.working)) {
        resp = ACK_BUSY;
    }
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

    ((struct sockaddr_in6 *) &address)->sin6_port = htons(peer_port);
    if ((sock = handler->checkNeighbor(address)) == -1) {
        reportDebug("Error getting host address.", 2);
        return false;
    }
    if ((getHostAddr(addr, sock)) == -1) {
        reportDebug("Error getting host address.", 2);
        close(sock);
        return false;
    }
    close(sock);
    ((struct sockaddr_in6 *) &addr)->sin6_port = htons(peer_port);

    if (DATA->config.is_superpeer)
        handler->addNewNeighbor(false, addr);
    else
        handler->addNewNeighbor(true, addr);
    return true;
}

bool CmdGoodbyePeer::execute(int fd, struct sockaddr_storage &address, void *) {
    reportDebug("GOODBYE PEER", 5);
    CMDS action = GOODBYE_HOST;
    RESPONSE_T resp = ACK_FREE;
    int port;
    if (sendCmd(fd, action) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

    if (recvSth(port, fd) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }

    ((struct sockaddr_in6 *) &address)->sin6_port = htons(port);
    DATA->neighbors.removeNeighbor(address);
    if (sendSth(resp, fd) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    return true;
}

bool CmdGoodbyeHost::execute(int fd, sockaddr_storage &address, void *) {
    RESPONSE_T resp;

    if (sendSth(DATA->config.intValues.at("LISTENING_PORT"), fd) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }

    if (recvSth(resp, fd) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    reportDebug("I have informed neighbor: " + MyAddr(address).get(), 2);
    return true;

}

void CmdSayGoodbye::execute() {
    int sock;
    for (auto &address : DATA->neighbors.getNeighborAdresses(
             DATA->neighbors.getNeighborCount())) {
        sock = handler->checkNeighbor(address);
        reportError("Saying goodbye to: "+MyAddr(address).get());
        handler->spawnOutgoingConnection(address,
                                         sock, { GOODBYE_PEER }, false, nullptr);
    }
    struct sockaddr_storage address;
    if (DATA->config.IPv4_ONLY)
        address = addr2storage(DATA->config.superpeer_addr.c_str(), DATA->config.intValues.at("SUPERPEER_PORT"), AF_INET);
    else
        address = addr2storage(DATA->config.superpeer_addr.c_str(), DATA->config.intValues.at("SUPERPEER_PORT"), AF_INET6);
   sock = handler->checkNeighbor(address);
    handler->spawnOutgoingConnection(address,
                                     sock, { GOODBYE_PEER }, false, nullptr);
}
