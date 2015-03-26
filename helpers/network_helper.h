#ifndef NETWORK_HELPER_H
#define NETWORK_HELPER_H

#include "headers/networkhandler.h"
#include <errno.h>
#include <string>
#include <error.h>
#include <err.h>

namespace networkHelper {
struct sockaddr_storage addr2storage(const char* addr, int32_t port, int32_t family);
std::string storage2addr(const struct sockaddr_storage &addr);
int32_t getHostAddr(struct sockaddr_storage &addr, int32_t fd);
int32_t getPeerAddr(struct sockaddr_storage &addr, int32_t fd);
int32_t getMyAddress(
        struct sockaddr_storage &addr, NetworkHandler *handler);
bool addrIn(const struct sockaddr_storage &st, neighbor_storageT &list);
bool cmpStorages(const struct sockaddr_storage &s1,
                 const struct sockaddr_storage &s2);
void changeAddressPort(struct sockaddr_storage &storage, int32_t port);
}
#endif // NETWORK_HELPER_H
