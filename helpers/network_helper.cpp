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
    if (((struct sockaddr *) &s1)->sa_family != ((struct sockaddr *) &s2)->sa_family)
        return false;
    if (((struct sockaddr *) &s1)->sa_family == AF_INET) {
        if ((((struct sockaddr_in *) &s1)->sin_port == ((struct sockaddr_in *) &s2)->sin_port) &&
                (networkHelper::storage2addr(s1) == networkHelper::storage2addr(s2)))
            return true;
        else {
            return false;
        }
    }
    if (((struct sockaddr *) &s1)->sa_family == AF_INET6) {
        if ((((struct sockaddr_in6 *) &s1)->sin6_port == ((struct sockaddr_in6 *) &s2)->sin6_port) &&
                (networkHelper::storage2addr(s1) == networkHelper::storage2addr(s2))) {
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
    for (auto &n : list) {
        if (cmpStorages(n.second->address, addr))
            return true;
    }
    return false;
}

int32_t networkHelper::getHostAddr(
        struct sockaddr_storage &addr, int32_t fd) {
    struct sockaddr_in *in4p = (struct sockaddr_in *) &addr;
    struct sockaddr_in6 *in6p = (struct sockaddr_in6 *) &addr;
    bzero(&in4p->sin_addr, INET_ADDRSTRLEN);
    bzero(&in6p->sin6_addr, INET6_ADDRSTRLEN);
    socklen_t size4 = sizeof (*in4p), size6 = sizeof(*in6p);
    if (DATA->config.IPv4_ONLY) {
        if (getsockname(fd, (struct sockaddr *) in4p, &size4) == -1) {
            reportDebug("Not an IPv4 addr.", 4);
        } else {
            in4p->sin_family = AF_INET;
            return 0;
        }
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

int32_t networkHelper::getPeerAddr(
        struct sockaddr_storage &addr, int32_t fd) {
    struct sockaddr_in *in4p = (struct sockaddr_in *) &addr;
    struct sockaddr_in6 *in6p = (struct sockaddr_in6 *) &addr;
    bzero(&in4p->sin_addr, INET_ADDRSTRLEN);
    bzero(&in6p->sin6_addr, INET6_ADDRSTRLEN);
    socklen_t size4 = sizeof (*in4p), size6 = sizeof(*in6p);
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

struct sockaddr_storage networkHelper::addr2storage(
        const char *addrstr, int32_t port, int32_t family) {
    struct sockaddr_storage addr;
    if (family == AF_INET) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *) &addr;
        addr4->sin_family = family;
        addr4->sin_port = htons(port);
        inet_pton(family, addrstr, &(addr4->sin_addr));
    } else {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *) &addr;
        addr6->sin6_family = family;
        addr6->sin6_port = htons(port);
        inet_pton(family, addrstr, &(addr6->sin6_addr));
    }
    return addr;
}

std::string networkHelper::storage2addr(
        const sockaddr_storage &addr) {
    if (addr.ss_family == AF_INET) {
        char buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &((struct sockaddr_in *)&addr)->sin_addr, buf, INET_ADDRSTRLEN);
        return std::string(buf);
    } else {
        char buf[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &((struct sockaddr_in6 *)&addr)->sin6_addr, buf, INET6_ADDRSTRLEN);
        return std::string(buf);
    }
}

void networkHelper::changeAddressPort(struct sockaddr_storage &addr, int32_t port) {
    port = (in_port_t) port;
    if (addr.ss_family == AF_INET) {
        ((struct sockaddr_in *) &addr)->sin_port =
                htons(port);
    } else {
        ((struct sockaddr_in6 *) &addr)->sin6_port =
                htons(port);
    }
}
