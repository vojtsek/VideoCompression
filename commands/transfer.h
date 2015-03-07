#include "commands.h"
#ifndef TRANSFER_CMD_H
#define TRANSFER_CMD_H

class CmdDistributePeer: public NetworkCommand {
public:
    CmdDistributePeer(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
};

class CmdDistributeHost: public NetworkCommand {
public:
    CmdDistributeHost(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
};

class CmdReturnPeer: public NetworkCommand {
public:
    CmdReturnPeer(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
};

class CmdReturnHost: public NetworkCommand {
public:
    CmdReturnHost(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
};

#endif
