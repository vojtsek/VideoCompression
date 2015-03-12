#ifndef NETWORK_HELPER_H
#define NETWORK_HELPER_H

#include <errno.h>
#include <string>
#include <error.h>
#include <err.h>

struct sockaddr_storage addr2storage(const char* addr, int port, int family);
std::string storage2addr(struct sockaddr_storage &addr);
std::string receiveString(int fd);
void getHostAddr(struct sockaddr_storage &addr, int fd);
void getPeerAddr(struct sockaddr_storage &addr, int fd);
int sendString(int fd, std::string str);
int sendFile(int fd, std::string str);
int receiveFile(int fd, std::string fn);
int sendCmd(int fd, CMDS cmd);
bool addrIn(struct sockaddr_storage &st, neighbor_storageT &list);
bool cmpStorages(struct sockaddr_storage &s1, struct sockaddr_storage &s2);
template<typename T>
int sendSth(T what, int fd) {
    int w;
    if ((w = write(fd, &what, sizeof (T))) != sizeof(T)) {
        reportError("Problem occured while sending the data.");
        return (-1);
    }
    return (w);
}

template<typename T>
int sendSth(T *what, int fd, int len) {
    int w;
    if ((w = write(fd, what, len * sizeof(T))) == -1) {
        reportError("Problem occured while sending the data.");
        return (-1);
    }
    return (w);
}
template<typename T>
int recvSth(T &where, int fd) {
    int r;
     if ((r = read(fd, &where, sizeof (T))) != sizeof(T)) {
        reportError("Problem occured while accepting the data. ");
        return (-1);
    }
    return (r);
}
template<typename T>
int recvSth(T *where, int fd, int len) {
    int r;
    if ((r = read(fd, where, len * sizeof (T))) < 1) {
        reportError("Problem occured while accepting the data.");
        return (-1);
    }
    return (r);
}


#endif // NETWORK_HELPER_H
