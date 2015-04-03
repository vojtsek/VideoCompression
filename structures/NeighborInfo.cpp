#include "headers/defines.h"
#include "headers/include_list.h"

#include <arpa/inet.h>

/*!
 * \brief NeighborInfo::printInfo prints information about neighbor
 */
void NeighborInfo::printInfo() {
    reportStatus(getInfo());
}

/*!
 * \brief NeighborInfo::getInfo returns brief neighbor information as a string
 * \return neighbor information stringified
 */
std::string NeighborInfo::getInfo() {
    MyAddr mad(address);
    std::string result(mad.get() +
                       "; quality: " + utilities::m_itoa(quality) +
                       "; seconds to check: " + utilities::m_itoa(intervals)
                       );
    return result;
}

/*!
 * \brief NeighborInfo::invoke checks the neighbor state
 * \param net_handler
 * is supposed to be invoked periodically;
 * decreases counter and when zero, checks the neighbor state
 */
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

/*!
 * \brief NeighborInfo::toString return unique string representation
 * \return string representation
 */
std::string NeighborInfo::toString() {
    std::string hash(networkHelper::storage2addrstr(address) +
                     utilities::m_itoa(((struct sockaddr_in *)&address)->sin_port));
    return hash;
}

/*!
 * \brief NeighborInfo::equalsTo checks the equality of string representations
 * \param that another Listener
 * \return true iff same representations
 */
bool NeighborInfo::equalsTo(Listener *that) {
    return (toString() == that->toString());
}
