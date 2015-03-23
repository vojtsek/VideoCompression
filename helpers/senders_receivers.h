#ifndef SENDERS_RECEIVERS_H
#define SENDERS_RECEIVERS_H
#include <string>
#include "headers/enums_types.h"

int32_t sendResponse(int32_t fd, RESPONSE_T &resp);
int32_t sendStruct(int32_t fd, const struct sockaddr_storage &st);
int32_t sendInt32(int32_t fd, int32_t i);
int32_t sendString(int32_t fd, std::string str);
int32_t sendFile(int32_t fd, std::string str);
int32_t sendCmd(int32_t fd, CMDS cmd);
int32_t receiveResponse(int32_t fd, RESPONSE_T &resp) ;
int32_t receiveStruct(int32_t fd, struct sockaddr_storage &st);
int32_t receiveInt32(int32_t fd, int32_t &i);
std::string receiveString(int32_t fd);
int32_t receiveFile(int32_t fd, std::string fn);

#endif // SENDERS_RECEIVERS_H
