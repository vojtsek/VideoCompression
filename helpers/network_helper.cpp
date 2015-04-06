#define _FILE_OFFSET_BITS 64
#include "headers/include_list.h"
#include "headers/handle_IO.h"
#include "headers/commands.h"
#include "headers/defines.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <utility>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <mutex>
#include <condition_variable>
#include <stdexcept>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdarg.h>
#include <curses.h>
#include <deque>
#include <thread>
#include <string.h>
#include <sys/stat.h>
#include <err.h>
#include <fcntl.h>
#include "helpers/network_helper.h"


bool networkHelper::cmpStorages(
        const struct sockaddr_storage &s1,
                 const struct sockaddr_storage &s2) {
    // quite straightforward
    // get the address type first
    //IPv4
    if (((struct sockaddr *) &s1)->sa_family != ((struct sockaddr *) &s2)->sa_family)
        return false;
    if (((struct sockaddr *) &s1)->sa_family == AF_INET) {
        if ((((struct sockaddr_in *) &s1)->sin_port ==
             ((struct sockaddr_in *) &s2)->sin_port) &&
                (networkHelper::storage2addrstr(s1) ==
                 networkHelper::storage2addrstr(s2)))
            return true;
        else {
            return false;
        }
    }
    // IPv6
    if (((struct sockaddr *) &s1)->sa_family == AF_INET6) {
        if ((((struct sockaddr_in6 *) &s1)->sin6_port ==
             ((struct sockaddr_in6 *) &s2)->sin6_port) &&
                (networkHelper::storage2addrstr(s1) ==
                 networkHelper::storage2addrstr(s2))) {
            return true;
        } else {
            return false;
        }
    }
    return false;
}

bool networkHelper::addrIn(
        const struct sockaddr_storage &addr,
            neighbor_storageT &list) {
    // traverse the list and look for a match
    for (auto &n : list) {
        if (cmpStorages(n.second->address, addr))
            return true;
    }
    return false;
}

bool networkHelper::isFree() {
    int64_t can_accept = std::atomic_load(&DATA->state.can_accept);

    // able to do some work?
    if ((can_accept <= 0) || (DATA->state.working &&
                              !DATA->config.serve_while_working)) {
        return false;
    }
    return true;
}

int64_t networkHelper::getHostAddr(
        struct sockaddr_storage &addr, int64_t fd) {
            // prepare the structures
    struct sockaddr_in *in4p = (struct sockaddr_in *) &addr;
    struct sockaddr_in6 *in6p = (struct sockaddr_in6 *) &addr;
    bzero(&in4p->sin_addr, INET_ADDRSTRLEN);
    bzero(&in6p->sin6_addr, INET6_ADDRSTRLEN);
    socklen_t size4 = sizeof (*in4p), size6 = sizeof(*in6p);
            // IPv4 mode, return sockaddr_in
    if (DATA->config.IPv4_ONLY) {
        if (getsockname(fd, (struct sockaddr *) in4p, &size4) == -1) {
            reportDebug("Not an IPv4 addr.", 4);
        } else {
            in4p->sin_family = AF_INET;
            return 0;
        }
            // IPv6 mode, returns sockaddr_in6
    } else {
        in6p->sin6_family = AF_INET6;
        if (getsockname(fd, (struct sockaddr *) in6p, &size6) == -1) {
            reportDebug("Not an IPv6 addr.", 4);
        } else
            return 0;
    }
    reportDebug("Failed to get host's address.", 2);
    return -1;
}

int64_t networkHelper::getPeerAddr(
        struct sockaddr_storage &addr, int64_t fd) {
    struct sockaddr_in *in4p = (struct sockaddr_in *) &addr;
    struct sockaddr_in6 *in6p = (struct sockaddr_in6 *) &addr;
    bzero(&in4p->sin_addr, INET_ADDRSTRLEN);
    bzero(&in6p->sin6_addr, INET6_ADDRSTRLEN);
    socklen_t size4 = sizeof (*in4p), size6 = sizeof(*in6p);
    // gets the IP address using the provided file descriptor
    // tries both IPv4 and IPv6
    if (getpeername(fd, (struct sockaddr *) in4p, &size4) == -1) {
        reportDebug("Not an IPv4 addr.", 4);
    } else {
        addr.ss_family = AF_INET;
        return 0;
    }
    if (getpeername(fd, (struct sockaddr *) in6p, &size6) == -1) {
        reportDebug("Not an IPv6 addr.", 4);
    } else {
        addr.ss_family = AF_INET6;
        return 0;
    }
    reportDebug("Failed to get host's address.", 2);
    return -1;
}

int64_t networkHelper::getMyAddress(struct sockaddr_storage &neighbor_addr,
        struct sockaddr_storage &addr, NetworkHandler *handler) {
    int64_t sock;

    // connect to it
    if ((sock = handler->checkNeighbor(neighbor_addr)) == -1) {
        reportDebug("Error getting host address.", 2);
        return -1;
    }

    // dig the addres from the file descriptor
    if ((networkHelper::getHostAddr(addr, sock)) == -1) {
        reportDebug("Error getting host address.", 2);
        close(sock);
        return -1;
    }
    close(sock);
    // finally set the port
    networkHelper::changeAddressPort(addr,
                                     DATA->config.getIntValue("LISTENING_PORT"));
    return 0;
}

int64_t networkHelper::getSuperPeerAddr(
        sockaddr_storage &address) {
    if (DATA->config.IPv4_ONLY) {
        address = networkHelper::addrstr2storage(
                    DATA->config.getStringValue("SUPERPEER_ADDR").c_str(),
                    DATA->config.getIntValue("SUPERPEER_PORT"), AF_INET);
    } else {
        address = networkHelper::addrstr2storage(
                    DATA->config.getStringValue("SUPERPEER_ADDR").c_str(),
                    DATA->config.getIntValue("SUPERPEER_PORT"), AF_INET6);
    }
    if (address.ss_family == AF_UNSPEC) {
        return -1;
    }
    return 0;
}

struct sockaddr_storage networkHelper::addrstr2storage(
        const char *addrstr, int64_t port, int64_t family) {
    struct sockaddr_storage addr;
    // IPv4
    if (family == AF_INET) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *) &addr;
        addr4->sin_family = family;
        addr4->sin_port = htons(port);
        if (inet_pton(family, addrstr, &(addr4->sin_addr)) == -1) {
            // determine error
            addr4->sin_family = AF_UNSPEC;
        }
     // IPv6
    } else {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *) &addr;
        addr6->sin6_family = family;
        addr6->sin6_port = htons(port);
        if (inet_pton(family, addrstr, &(addr6->sin6_addr)) == -1) {
            addr6->sin6_family = AF_UNSPEC;
        }
    }
    return addr;
}

std::string networkHelper::storage2addrstr(
        const sockaddr_storage &addr) {
    // IPv4
    if (addr.ss_family == AF_INET) {
        char buf[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &((struct sockaddr_in *)&addr)->sin_addr,
                      buf, INET_ADDRSTRLEN) == NULL) {
            // determine error
            return std::string("");
        }
        return std::string(buf);
     // IPv6
    } else {
        char buf[INET6_ADDRSTRLEN];
        if (inet_ntop(AF_INET6, &((struct sockaddr_in6 *)&addr)->sin6_addr,
                      buf, INET6_ADDRSTRLEN) == NULL) {
            // determine error
            return std::string("");
        }
        return std::string(buf);
    }
}

void networkHelper::changeAddressPort(
        struct sockaddr_storage &addr, int64_t port) {
    // port number in the structure has to be in network byte order
    port = (in_port_t) port;
    if (addr.ss_family == AF_INET) {
        ((struct sockaddr_in *) &addr)->sin_port =
                htons(port);
    } else {
        ((struct sockaddr_in6 *) &addr)->sin6_port =
                htons(port);
    }
}
