#include "headers/defines.h"
#include "headers/include_list.h"

#include <arpa/inet.h>


void NeighborInfo::printInfo() {
    MyAddr mad(address);
    reportSuccess(mad.get() + "; quality: " +
                  utilities::m_itoa(quality) + "; seconds to check: " +
                  utilities::m_itoa(intervals));
}

void NeighborInfo::invoke(NetworkHandler &net_handler) {
    int32_t sock;
    if (--intervals <= 0) {
        sock = net_handler.checkNeighbor(address);
        if (sock == -1) {
            dirty = true;
            free = false;
            return;
        }
        net_handler.spawnOutgoingConnection(
                    address, sock, { PING_PEER }, true, nullptr);
    }
}

std::string NeighborInfo::toString() {
    std::string hash(networkHelper::storage2addr(address) +
                     utilities::m_itoa(((struct sockaddr_in *)&address)->sin_port));
    return hash;
}

bool NeighborInfo::equalsTo(Listener *that) {
    return (toString() == that->toString());
}
