#ifndef NETWORK_HELPER_H
#define NETWORK_HELPER_H

#include "structures/NetworkHandler.h"
#include <errno.h>
#include <string>
#include <error.h>
#include <err.h>

/*!
 * functions to help handle networking, manipulating address structures etc.
 */
namespace networkHelper {

/*!
 * \brief addrstr2storage from string representation of IP address creates address structure of desired family
 * \param addr string representation of the IP address
 * \param port port number
 * \param family what type of structure to create IPv4 or IPv6 (AF_INET[6] respectively)
 * \return created address structure, family set to AF_UNSPEC on failure
 */
struct sockaddr_storage addrstr2storage(const char* addr,
                                        int32_t port, int32_t family);

/*!
 * \brief storage2addrstr converts the address contained in the address structure to string representation
 * \param addr address structure to convert
 * \return string representation of the address, empty in case of failure
 */
std::string storage2addrstr(const struct sockaddr_storage &addr);

/*!
 * \brief getHostAddr loads the address on which is host communicating into the given structure
 * \param addr structure to load the address
 * \param fd file descriptor of the connection
 * \return zero on success
 */
int32_t getHostAddr(struct sockaddr_storage &addr, int32_t fd);

/*!
 * \brief getPeerAddr loads the address on which the peer is communicating into the given structure
 * \param addr structure to load the address
 * \param fd file descriptor of the connection
 * \return zero on success
 */
int32_t getPeerAddr(struct sockaddr_storage &addr, int32_t fd);

/*!
 * \brief getMyAddress loads the address on which the node is communicating
 * \param addr structure to receive the address
 * \param handler pointer to NetworkHandler instance
 * \return zero on successs
 * Loads the address of the current node,
 * spawns connection to some neighbor and
 * reads the address using getHostAddr()
 */
int32_t getMyAddress(
        struct sockaddr_storage &addr, NetworkHandler *handler);

/*!
 * \brief getSuperPeerAddress loads the superpeer address into the provided address structure
 * \param addr structure to load into
 * \return zero on success
 */
int32_t getSuperPeerAddr(struct sockaddr_storage &addr);

/*!
 * \brief addrIn determines, whether the given address is contained in the list
 * \param st address structure to look for
 * \param list list of structures
 * \return true if address has been found in the list
 */
bool addrIn(const struct sockaddr_storage &st,
            neighbor_storageT &list);

/*!
 * \brief cmpStorages compares two sockaddr structures of same family
 * \param s1 structure1
 * \param s2 structure2
 * \return true if the structures represent same address and port
 */
bool cmpStorages(const struct sockaddr_storage &s1,
                 const struct sockaddr_storage &s2);

/*!
 * \brief isFree tells, whether the current node is able to accept chunks for encoding
 * \return true if the node is free
 */
bool isFree();

/*!
 * \brief changeAddressPort changes port field of the passed address structure
 * \param storage address structure to change the port
 * \param port desired port number
 */
void changeAddressPort(
        struct sockaddr_storage &storage, int32_t port);
}
#endif // NETWORK_HELPER_H
