#include "headers/defines.h"
#include "headers/include_list.h"

#include <arpa/inet.h>

void NeighborInfo::printInfo() {
    reportStatus(getInfo());
}

std::string NeighborInfo::getInfo() {
    MyAddr mad(address);
    std::string result(mad.get() +
                       "; quality: " + utilities::m_itoa(quality) +
                       "; seconds to check: " + utilities::m_itoa(intervals)
                       );
    return result;
}

void NeighborInfo::invoke(NetworkHandler &net_handler) {
    int32_t sock;
    // decrease intervals
    if (--intervals <= 0) {
        // if time is up, tries to connect
        sock = net_handler.checkNeighbor(address);
        // failed to connect, going to be remove
        if (sock == -1) {
            dirty = true;
            free = false;
            return;
        }
        // ping neighbor, so info is updated
        net_handler.spawnOutgoingConnection(
                    address, sock, { PING_PEER }, true, nullptr);
    }
}

std::string NeighborInfo::toString() {
    std::string hash(networkHelper::storage2addrstr(address) +
                     utilities::m_itoa(((struct sockaddr_in *)&address)->sin_port));
    return hash;
}

bool NeighborInfo::equalsTo(Listener *that) {
    return (toString() == that->toString());
}
