#ifndef SENDERS_RECEIVERS_H
#define SENDERS_RECEIVERS_H
#include <string>
#include "headers/enums_types.h"

int sendResponse(int fd, RESPONSE_T &resp);
int sendStruct(int fd, const struct sockaddr_storage &st);
int sendInt32(int fd, int32_t i);
int sendString(int fd, std::string str);
int sendFile(int fd, std::string str);
int sendCmd(int fd, CMDS cmd);
int receiveResponse(int fd, RESPONSE_T &resp) ;
int receiveStruct(int fd, struct sockaddr_storage &st);
int receiveInt32(int fd, int32_t &i);
std::string receiveString(int fd);
int receiveFile(int fd, std::string fn);

#endif // SENDERS_RECEIVERS_H
