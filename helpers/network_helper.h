#ifndef NETWORK_HELPER_H
#define NETWORK_HELPER_H

#include <errno.h>
#include <string>
#include <error.h>
#include <err.h>

struct sockaddr_storage addr2storage(const char* addr, int port, int family);
std::string storage2addr(const struct sockaddr_storage &addr);
std::string receiveString(int fd);
int getHostAddr(struct sockaddr_storage &addr, int fd);
int getPeerAddr(struct sockaddr_storage &addr, int fd);
int sendString(int fd, std::string str);
int sendFile(int fd, std::string str);
int receiveFile(int fd, std::string fn);
int sendCmd(int fd, CMDS cmd);
bool addrIn(const struct sockaddr_storage &st, neighbor_storageT &list);
bool cmpStorages(const struct sockaddr_storage &s1,
                 const struct sockaddr_storage &s2);

#endif // NETWORK_HELPER_H
