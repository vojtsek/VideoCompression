#ifndef NETWORK_HELPER_H
#define NETWORK_HELPER_H

struct sockaddr_storage addr2storage(const char* addr, int port, int family);
struct sockaddr_storage getHostAddr(int fd);
struct sockaddr_storage getPeerAddr(int fd);
bool addrIn(struct sockaddr_storage &st, std::vector<NeighborInfo> list);
bool cmpStorages(struct sockaddr_storage &s1, struct sockaddr_storage &s2);
template<typename T>
int sendSth(T what, int fd) {
    int w;
    if ((w = write(fd, &what, sizeof (T))) < 1) {
        reportError("Problem occured while sending the data.");
        return (-1);
    }
    return (w);
}

template<typename T>
int sendSth(T *what, int fd, int len) {
    int w;
    if ((w = write(fd, what, len * sizeof(T))) < 1) {
        reportError("Problem occured while sending the data.");
        return (-1);
    }
    return (w);
}
template<typename T>
int recvSth(T &where, int fd) {
    int r;
     if ((r = read(fd, &where, sizeof (T))) < 1) {
        reportError("Problem occured while accepting the data.");
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
