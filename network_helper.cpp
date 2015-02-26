#define _FILE_OFFSET_BITS 64
#include "common.h"
#include "handle_IO.h"
#include "commands.h"
#include "defines.h"

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
#include "network_helper.h"


bool cmpStorages(struct sockaddr_storage &s1, struct sockaddr_storage &s2) {
    if (((struct sockaddr *) &s1)->sa_family != ((struct sockaddr *) &s2)->sa_family)
        return false;
    if (((struct sockaddr *) &s1)->sa_family == AF_INET) {
        if ((((struct sockaddr_in *) &s1)->sin_port == ((struct sockaddr_in *) &s2)->sin_port))
                //(((struct sockaddr_in *) &s1)->sin_addr == ((struct sockaddr_in *) &s2)->sin_addr))
            return true;
        else
            return false;
    }
    //TODO: maybe bad
    if (((struct sockaddr *) &s1)->sa_family == AF_INET6) {
        if ((((struct sockaddr_in6 *) &s1)->sin6_port == ((struct sockaddr_in6 *) &s2)->sin6_port))
               // (((struct sockaddr_in6 *) &s1)->sin6_addr == ((struct sockaddr_in6 *) &s2)->sin6_addr))
            return true;
        else
            return false;
    }
    return false;
}

bool addrIn(struct sockaddr_storage &addr, std::vector<NeighborInfo *> &list) {

    for (auto n : list) {
        if (cmpStorages(n->address, addr))
            return true;
    }
    return false;
}

//TODO getsockname doesnt fail if short structure was supplied!
struct sockaddr_storage getHostAddr(int fd) {
    struct sockaddr_storage in4, in6;
    struct sockaddr_in *in4p = (struct sockaddr_in *) &in4;
    struct sockaddr_in6 *in6p = (struct sockaddr_in6 *) &in6;
    bzero(&in4p->sin_addr, INET_ADDRSTRLEN);
    bzero(&in6p->sin6_addr, INET6_ADDRSTRLEN);
    socklen_t size4 = sizeof (in4), size6 = sizeof(in6);
    if (DATA->config.IPv4_ONLY) {
        if (getsockname(fd, (struct sockaddr *) in4p, &size4) == -1) {
            reportDebug("Not an IPv4 addr.", 4);
        } else {
            in4p->sin_family = AF_INET;
            return (in4);
        }
    } else {
        in6p->sin6_family = AF_INET6;
        if (getsockname(fd, (struct sockaddr *) in6p, &size6) == -1) {
            reportDebug("Not an IPv6 addr.", 4);
        } else
            return (in6);
    }
    reportDebug("Failed to get host's address.", 2);
    throw new std::exception();
    return (in4);
}

struct sockaddr_storage getPeerAddr(int fd) {
    struct sockaddr_storage in4, in6;
    struct sockaddr_in *in4p = (struct sockaddr_in *) &in4;
    struct sockaddr_in6 *in6p = (struct sockaddr_in6 *) &in6;
    bzero(&in4p->sin_addr, INET_ADDRSTRLEN);
    bzero(&in6p->sin6_addr, INET6_ADDRSTRLEN);
    socklen_t size4 = sizeof (in4), size6 = sizeof(in6);
    if (getpeername(fd, (struct sockaddr *) in4p, &size4) == -1) {
        reportDebug("Not an IPv4 addr.", 4);
    } else {
        in4.ss_family = AF_INET;
        return (in4);
    }
    if (getpeername(fd, (struct sockaddr *) in6p, &size6) == -1) {
        reportDebug("Not an IPv6 addr.", 4);
    } else
        return (in4);
    reportDebug("Failed to get host's address.", 2);
    throw new std::exception();
    return (in4);
}

struct sockaddr_storage addr2storage(const char *addrstr, int port, int family) {
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

string storage2addr(sockaddr_storage &addr) {
    if (addr.ss_family == AF_INET) {
        char buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr, buf, INET_ADDRSTRLEN);
        return string(buf);
    } else {
        char buf[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &addr, buf, INET6_ADDRSTRLEN);
        return string(buf);
    }
}

int sendString(int fd, string str) {
    if (sendSth(str.length(), fd) == -1)
        return (-1);
    for (char c : str) {
        if (sendSth(c, fd) == -1)
            return (-1);
    }
    return (0);
}

string receiveString(int fd) {
    int len;
    string res;
    char c;
    if (recvSth(len, fd) == -1)
        return res;
    for (int i = 0; i < len; ++i) {
        if (recvSth(c, fd) == -1) {
            res.clear();
            return res;
        }
        res.push_back(c);
    }
    return res;
}
